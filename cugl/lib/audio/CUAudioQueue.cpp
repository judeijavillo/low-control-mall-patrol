//
//  CUAudioQueue.cpp
//  Cornell University Game Library (CUGL)
//
//  This module provides a queue for seamless audio playback. You can use
//  this queue to dynamically switch between music loops with no break in
//  the middle.  Typically there is only one audio queue, but it is possible
//  to have as many as you want.
//
//  Music queues are owned by the audio engine. Shutting down that engine
//  will shut down an associated audio queue as well.
//
//  CUGL MIT License:
//
//      This software is provided 'as-is', without any express or implied
//      warranty.  In no event will the authors be held liable for any damages
//      arising from the use of this software.
//
//      Permission is granted to anyone to use this software for any purpose,
//      including commercial applications, and to alter it and redistribute it
//      freely, subject to the following restrictions:
//
//      1. The origin of this software must not be misrepresented; you must not
//      claim that you wrote the original software. If you use this software
//      in a product, an acknowledgment in the product documentation would be
//      appreciated but is not required.
//
//      2. Altered source versions must be plainly marked as such, and must not
//      be misrepresented as being the original software.
//
//      3. This notice may not be removed or altered from any source distribution.
//
//  Author: Walker White
//  Version: 1/20/21
//
#include <cugl/cugl.h>
#include <algorithm>
#include <deque>

using namespace cugl;
using namespace cugl::audio;

/**
 * Creates, but does not initialize an audio queue.
 *
 * The queue must be initialized before is can be used.
 */
AudioQueue::AudioQueue() :
_cover(nullptr),
_queue(nullptr),
_callback(nullptr) {}

/**
 * Initializes the audio queue.
 *
 * This method initializes the audio queue, assigning it a single schedular node.
 * This node is still owned by the audio engine.  Hence changes in state to the
 * audio engine may affect this queue.
 *
 * @param slot  The audio engine slot for this queue
 *
 * @return true if the audio queue was successfully initialized.
 */
bool AudioQueue::init(const std::shared_ptr<audio::AudioFader>& slot) {
    _cover = slot;
    _queue = std::dynamic_pointer_cast<AudioScheduler>(slot->getInput());
    if (_queue) {
        // Start off with some preallocated faders and panners
        for(int ii = 0; ii <= 2; ii++) {
            _fadePool.push_back(AudioFader::alloc(_queue->getChannels(),_queue->getRate()));
            _panPool.push_back(AudioPanner::alloc(_queue->getChannels(),2,_queue->getRate()));
        }
        
		_queue->setCallback([=](const std::shared_ptr<cugl::audio::AudioNode>& node,
						 		cugl::audio::AudioNode::Action action) {
			if (action != cugl::audio::AudioNode::Action::LOOPBACK) {
				bool success = (action == cugl::audio::AudioNode::Action::COMPLETE);
				this->gcollect(node,success);
			}
		});
        
        return true;
    }
    return true;
}

/**
 * Releases all resources for this audio queue.
 *
 * Music tracks can no longer be queued. If you need to use the queue again,
 * you must call init().
 */
void AudioQueue::dispose() {
    if (_cover != nullptr) {
        clear();
        _fadePool.clear();
        _panPool.clear();
        _queue = nullptr;
        _cover = nullptr;
    }
}


#pragma mark -
#pragma mark Source Management
/**
 * Returns a playable audio node for a given audio instance
 *
 * Each playable asset needs a panner (for pan support, and to guarantee the
 * correct number of output channels) and a fader before it can be plugged
 * in to the mixer graph. This is true both for sound assets as well as
 * arbitrary audio subgraphs. This method uses the object pools to simplify
 * this process.
 *
 * This method will also allocated an {@link AudioResampler} if the sample
 * rate is not consistent with the engine.  However, these are extremely
 * heavy-weight and cannot be easily reused, and this is to be avoided if
 * at all possible.
 *
 * @param instance  The audio instance
 *
 * @return a playable audio node for a given a sound instance.
 */
std::shared_ptr<audio::AudioFader> AudioQueue::wrapInstance(const std::shared_ptr<audio::AudioNode>& instance) {
    std::shared_ptr<AudioFader> fader = nullptr;
    if (_fadePool.empty()) {
        fader = AudioFader::alloc(_queue->getChannels(),_queue->getRate());
    } else {
        fader = _fadePool.front();
        _fadePool.pop_front();
    }
    std::shared_ptr<AudioPanner> panner = nullptr;
    if (_panPool.empty()) {
        panner = AudioPanner::alloc(_queue->getChannels(),instance->getChannels(),_queue->getRate());
    } else {
        panner = _panPool.front();
        if (panner->getField() != instance->getChannels()) {
            panner->setField(instance->getChannels());
        }
        _panPool.pop_front();
    }
    fader->attach(panner);
    
    // Add a resampler if we have rate issues
    if (instance->getRate() == panner->getRate()) {
        panner->attach(instance);
    } else {
        std::shared_ptr<audio::AudioResampler> sampler;
        sampler = audio::AudioResampler::alloc(instance->getChannels(),panner->getRate());
        sampler->setName("__queue_resampler__");
        sampler->attach(instance);
        panner->attach(sampler);
    }
    return fader;
}

/**
 * Returns the sound instance for the given wrapped audio node.
 *
 * This method is the reverse of {@link wrapInstance}, allowing access to the
 * sound instance previously wrapped as an audio node. It does not distinguish
 * sound assets from arbitrary audio graphs.
 *
 * @param node  The audio node wrapping the sound instance
 *
 * @return the sound instance for the given wrapped audio node.
 */
std::shared_ptr<audio::AudioNode> AudioQueue::accessInstance(const std::shared_ptr<audio::AudioNode>& node) const {
    if (node) {
        auto fader = std::dynamic_pointer_cast<AudioFader>(node);
        auto panner  = std::dynamic_pointer_cast<AudioPanner>(fader->getInput());
        auto sampler = std::dynamic_pointer_cast <AudioResampler>(panner->getInput());
        if (sampler && sampler->getName() == "__queue_resampler__") {
            return sampler->getInput();
        }
        if (panner) {
            return panner->getInput();
        }
    }
    return nullptr;
}

/**
 * Disposes of the audio nodes wrapping a previously wrapped audio instance.
 *
 * Each playable asset needs a panner (for pan support, and to guarantee the
 * correct number of output channels) and a fader before it can be plugged
 * in to the mixer graph. This is true both for sound assets as well as
 * arbitrary audio subgraphs. This method is the reverse of {@link wrapInstance},
 * disposing (and recycling) those previously allocated nodes.
 *
 * @param node  The audio node wrapping the sound instance
 *
 * @return the inititial sound instance for the given playable audio node.
 */
std::shared_ptr<audio::AudioNode> AudioQueue::disposeWrapper(const std::shared_ptr<audio::AudioNode>& node) {
    if (node) {
        std::shared_ptr<AudioFader> fader   = std::dynamic_pointer_cast<AudioFader>(node);
        std::shared_ptr<AudioPanner> panner = std::dynamic_pointer_cast<AudioPanner>(fader->getInput());
        if (panner) {
            std::shared_ptr<AudioNode> source = panner->getInput();
            std::shared_ptr<AudioResampler> sampler = std::dynamic_pointer_cast<AudioResampler>(source);
            if (sampler && sampler->getName() == "__queue_resampler__") {
                source = sampler->getInput();
                sampler->detach();
                sampler->reset();
            }

            fader->detach();
            fader->fadeOut(-1);
            fader->reset();
            panner->detach();
            panner->reset();
            
            _fadePool.push_back(fader);
            _panPool.push_back(panner);
            return source;
        }
    }
    return nullptr;
}

/**
 * Callback function for when a music asset finishes
 *
 * This method is called when the active music completes. It disposes
 * any audio nodes (faders, panners), recycling them for later.  It also
 * invokes any callback functions associated with the music queue.
 *
 * This method is never intended to be accessed by general users.
 *
 * @param instance  The playback instance for the sound asset
 * @param status    True if the music terminated normally, false otherwise.
 */
void AudioQueue::gcollect(const std::shared_ptr<audio::AudioNode>& instance, bool status) {
    std::shared_ptr<audio::AudioNode> source = disposeWrapper(instance);
    if (_callback) {
        std::string id = source->getName();
        AudioPlayer* player = dynamic_cast<AudioPlayer*>(source.get());
        if (player && id == "__queue_playback__") {
            id = player->getSource()->getFile();
        }
        _callback(id,status);
    }
}


#pragma mark -
#pragma mark Music Playback
/**
 * Plays given music asset as a background track.
 *
 * Music is handled differently from sound effects. You can only play one
 * music asset at a time. However, it is possible to queue music assets
 * for immediate playback once the active asset is finished. Proper
 * queue management is the keep for smooth, uninterrupted playback that
 * responds to the user's actions.
 *
 * This method immediately plays the provided asset. Hence it overrides
 * and clears the music queue (though any cross fade setting is honored).
 * To safely play an asset without affecting the music queue, use the
 * method {@link enqueue} instead.
 *
 * When it begins playing, the music will start at full volume unless you
 * provide a number of seconds to fade in. Note that looping a song will
 * cause it to block the queue indefinitely until you turn off looping for
 * that asset {@see setLoop}. This can be desired behavior, as it gives you
 * a way to control the speed of the queue processing
 *
 * @param music     The music asset to play
 * @param loop      Whether to loop the music continuously
 * @param volume    The music volume (relative to the default asset volume)
 */
void AudioQueue::play(const std::shared_ptr<Sound>& music, bool loop,
                      float volume, float fade) {
    CUAssertLog(_cover != nullptr, "Attempt to use a disposed audio queue");
    std::shared_ptr<audio::AudioNode> player = music->createNode();
    player->setName("__queue_playback__");
    std::shared_ptr<AudioFader> fader = wrapInstance(player);
    fader->setGain(volume);
    if (fade > 0) {
        fader->fadeIn(fade);
    }
    _queue->play(fader, loop ? -1 : 0);
}

/**
 * Plays given audio graph as a background track.
 *
 * This alternate version of play allows the programmer to construct
 * custom composite audio graphs and play them as music tracks. Looping
 * behavior is supported if the audio node has a finite duration.
 *
 * This method immediately plays the provided asset. Hence it overrides
 * and clears the music queue (though any cross fade setting is honored).
 * To safely play an asset without affecting the music queue, use the
 * method {@link enqueue} instead.
 *
 * When it begins playing, the music will start at full volume unless you
 * provide a number of seconds to fade in. Note that looping a audio graph,
 * or providing audio graph with infinite duration will cause it to block
 * the queue indefinitely. You will need to turn off looping {@see #setLoop}
 * and/or manually advance the queue to progress further.
 *
 * @param graph     The audio node to play
 * @param loop      Whether to loop the music continuously
 * @param volume    The music volume (relative to the default instance volume)
 * @param fade      The number of seconds to fade in
 */
void AudioQueue::play(const std::shared_ptr<audio::AudioNode>& graph, bool loop,
                      float volume, float fade) {
    CUAssertLog(_cover != nullptr, "Attempt to use a disposed audio queue");
    CUAssertLog(graph->getName() != "__queue_playback__",  "Audio node uses reserved name '__queue_playback__'");
    CUAssertLog(graph->getName() != "__queue_resampler__", "Audio node uses reserved name '__queue_resampler__'");
    std::shared_ptr<AudioFader> fader = wrapInstance(graph);
    fader->setGain(volume);
    if (fade > 0) {
        fader->fadeIn(fade);
    }
    _queue->play(fader, loop ? -1 : 0);
}

/**
 * Returns the identifier for the track currently playing
 *
 * If the current playing track is an {@link Sound} asset, then the
 * identifier is the file name.  Otherwise, it is the name of the root
 * of the audio graph.  See {@link audio::AudioNode#getName}.
 *
 * @return the identifier for the track currently playing
 */
const std::string AudioQueue::current() const {
    CUAssertLog(_cover != nullptr, "Attempt to use a disposed audio queue");
    std::shared_ptr<audio::AudioNode> source = accessInstance(_queue->getCurrent());
    std::string id = source->getName();
    AudioPlayer* player = dynamic_cast<AudioPlayer*>(source.get());
    if (player && id == "__queue_playback__") {
        id = player->getSource()->getFile();
    }

    return id;
}

/**
 * Returns the current state of the background music
 *
 * @return the current state of the background music
 */
AudioEngine::State AudioQueue::getState() const {
    CUAssertLog(_cover != nullptr, "Attempt to use a disposed audio queue");
    if (!_queue->isPlaying()) {
        return AudioEngine::State::INACTIVE;
    }
    std::shared_ptr<audio::AudioNode> node = _queue->getCurrent();
    if (node->isPaused() || _queue->isPaused()) {
        return AudioEngine::State::PAUSED;
    }
    
    return AudioEngine::State::PLAYING;
}

/**
 * Returns true if the background music is in a continuous loop.
 *
 * If there is no active background music, this method will return false.
 *
 * @return true if the background music is in a continuous loop.
 */
bool AudioQueue::isLoop() const {
    CUAssertLog(_cover != nullptr, "Attempt to use a disposed audio queue");
    return _queue->getLoops() != 0;
}

/**
 * Sets whether the background music is on a continuous loop.
 *
 * If loop is true, this will block the queue until it is set to false
 * again. This can be desired behavior, as it gives you a way to control
 * the speed of the queue processing.
 *
 * If there is no active background music, this method will do nothing.
 *
 * @param  loop  whether the background music should be on a continuous loop
 */
void AudioQueue::setLoop(bool loop) {
    CUAssertLog(_cover != nullptr, "Attempt to use a disposed audio queue");
    _queue->setLoops(loop ? -1 : 0);
}

/**
 * Returns the volume of the background music
 *
 * The volume is a value 0 to 1, where 1 is maximum volume and 0 is complete
 * silence. If there is no active background music, this method will return 0.
 *
 * Note that this is the playback volume.  If the asset or audio graph had its
 * own initial volume setting, this is independent of this setting.  Indeed,
 * this value can be though of as the percentage of the default volume.
 *
 * @return the volume of the background music
 */
float AudioQueue::getVolume() const {
    CUAssertLog(_cover != nullptr, "Attempt to use a disposed audio queue");
    std::shared_ptr<AudioNode> node = _queue->getCurrent();
    if (node) {
        return node->getGain();
    }
    return 0;
}

/**
 * Sets the volume of the background music
 *
 * The volume is a value 0 to 1, where 1 is maximum volume and 0 is complete
 * silence. If there is no active background music, this method will have no effect.
 *
 * Note that this is the playback volume.  If the asset or audio graph had its
 * own initial volume setting, this is independent of this setting.  Indeed,
 * this value can be though of as the percentage of the default volume.
 *
 * @param  volume   the volume of the background music
 */
void AudioQueue::setVolume(float volume) {
    CUAssertLog(_cover != nullptr, "Attempt to use a disposed audio queue");
    CUAssertLog(volume >= 0 && volume <= 1, "Volume %f is out of range",volume);
    std::shared_ptr<AudioNode> node = _queue->getCurrent();
    if (node) {
        return node->setGain(volume);
    }
}

/**
 * Returns the stereo pan of the background music
 *
 * This audio engine provides limited (e.g. not full 3D) stereo panning
 * for simple effects. The pan value is a float from -1 to 1. A value
 * of 0 (default) plays to both channels (regardless of whether the
 * current effect is mono or stereo). A value of -1 will play to the
 * left channel only, while the right will play to the right channel
 * only. Channels beyond the first two are unaffected.
 *
 * In the case of stereo assets, panning to the left or right will mix
 * the audio feed; this process will never lose audio.
 *
 * @return the stereo pan of the background music
 */
float AudioQueue::getPanFactor() const  {
    CUAssertLog(_cover != nullptr, "Attempt to use a disposed audio queue");
    std::shared_ptr<AudioFader> fader = std::dynamic_pointer_cast<AudioFader>(_queue->getCurrent());
    if (fader) {
        std::shared_ptr<AudioPanner> panner = std::dynamic_pointer_cast<AudioPanner>(fader->getInput());
        if (panner->getField() == 1) {
            return panner->getPan(0,1)-panner->getPan(0,0);
        } else {
            return panner->getPan(1,1)-panner->getPan(0,0);
        }
    }
    return 0;
}


/**
 * Sets the stereo pan of the background music
 *
 * This audio engine provides limited (e.g. not full 3D) stereo panning
 * for simple effects. The pan value is a float from -1 to 1. A value
 * of 0 (default) plays to both channels (regardless of whether the
 * current effect is mono or stereo). A value of -1 will play to the
 * left channel only, while the right will play to the right channel
 * only. Channels beyond the first two are unaffected.
 *
 * In the case of stereo assets, panning to the left or right will mix
 * the audio feed; this process will never lose audio.
 *
 * @param pan   The stereo pan of the background music
 */
void AudioQueue::setPanFactor(float pan) {
    CUAssertLog(_cover != nullptr, "Attempt to use a disposed audio queue");
    CUAssertLog(pan >= -1 && pan <= 1, "Pan value %f is out of range",pan);
    std::shared_ptr<AudioFader> fader = std::dynamic_pointer_cast<AudioFader>(_queue->getCurrent());
    if (fader) {
        std::shared_ptr<AudioPanner> panner = std::dynamic_pointer_cast<AudioPanner>(fader->getInput());
        if (panner->getField() == 1) {
            panner->setPan(0,0,0.5-pan/2.0);
            panner->setPan(0,1,0.5+pan/2.0);
        } else {
            if (pan <= 0) {
                panner->setPan(0,0,1);
                panner->setPan(0,1,0);
                panner->setPan(1,0,-pan);
                panner->setPan(1,1,1+pan);
            } else {
                panner->setPan(1,1,1);
                panner->setPan(1,0,0);
                panner->setPan(0,0,1-pan);
                panner->setPan(0,1,pan);
            }
        }
    }
}

/**
 * Returns the length of background music, in seconds.
 *
 * This is only the duration of the active background music track. All other
 * music in the queue is ignored. If there is no active background music,
 * this method will return 0. If the current asset is an audio node with
 * undefined duration, this method will return -1.
 *
 * @return the length of background music, in seconds.
 */
float AudioQueue::getDuration() const {
    CUAssertLog(_cover != nullptr, "Attempt to use a disposed audio queue");
    std::shared_ptr<audio::AudioNode> source = accessInstance(_queue->getCurrent());
    AudioPlayer* player = dynamic_cast<AudioPlayer*>(source.get());
    if (player && player->getName() == "__queue_playback__") {
        return player->getSource()->getDuration();
    } else {
        double elapsed = source->getElapsed();
        double remains = source->getRemaining();
        if (elapsed >= 0 && remains >= 0) {
            return elapsed+remains;
        }
    }
    return -1;
}

/**
 * Returns the elapsed time of the background music, in seconds
 *
 * The elapsed time is the current position of the music from the beginning.
 * It does not include any time spent on a continuous loop. If there is no
 * active background music, this method will return 0. Given that the queue
 * processes streaming data in PCM format, the result of this method is
 * reasonably accurate, though it is affected by device latency.
 *
 * If the current asset is an audio node with undefined duration, this method
 * will return -1.
 *
 * @return the elapsed time of the background music, in seconds
 */
float AudioQueue::getTimeElapsed() const {
    CUAssertLog(_cover != nullptr, "Attempt to use a disposed audio queue");
    std::shared_ptr<AudioNode> node = _queue->getCurrent();
    if (node) {
        return (float)node->getElapsed();
    }
    return 0;
}

/**
 * Sets the elapsed time of the background music, in seconds
 *
 * The elapsed time is the current position of the music from the beginning.
 * It does not include any time spent on a continuous loop.
 *
 * If there is no active background music, this method will have no effect.
 *
 * @param  time  the new position of the background music
 */
void AudioQueue::setTimeElapsed(float time) {
    CUAssertLog(_cover != nullptr, "Attempt to use a disposed audio queue");
    std::shared_ptr<AudioNode> node = _queue->getCurrent();
    if (node) {
        node->setElapsed(time);
    }
}

/**
 * Returns the time remaining for the background music, in seconds
 *
 * The elapsed time is the current position of the music from the beginning.
 * It does not include any time spent on a continuous loop. If there is no
 * active background music, this method will return 0. Given that the queue
 * processes streaming data in PCM format, the result of this method is
 * reasonably accurate, though it is affected by device latency.
 *
 * If the current asset is an audio node with undefined duration, this method
 * will return -1.
 *
 * @return the time remaining for the background music, in seconds
 */
float AudioQueue::getTimeRemaining() const {
    CUAssertLog(_cover != nullptr, "Attempt to use a disposed audio queue");
    std::shared_ptr<AudioNode> node = _queue->getCurrent();
    if (node) {
        return (float)node->getRemaining();
    }
    return 0;
}

/**
 * Sets the time remaining for the background music, in seconds
 *
 * The time remaining is just duration-elapsed.  It does not take into
 * account whether the music is on a loop. It also does not include the
 * duration of any music waiting in the queue.
 *
 * This adjustment is not guaranteed to be accurate.  Attempting to time
 * the playback of streaming data (as opposed to a fully in-memory PCM
 * buffer) is very difficult and not cross-platform.  We have tried to be
 * reasonably accurate, but from our tests we can only guarantee accuracy
 * within a 10th of a second.
 *
 * If there is no active background music, this method will have no effect.
 *
 * @param  time  the new time remaining of the background music
 */
void AudioQueue::setTimeRemaining(float time) {
    CUAssertLog(_cover != nullptr, "Attempt to use a disposed audio queue");
    std::shared_ptr<AudioNode> node = _queue->getCurrent();
    if (node) {
        node->setRemaining(time);
    }
}

/**
 * Clears the entire queue, stopping the background music
 *
 * Before the music is stopped, this method gives the user an option to
 * fade out the music.  If the argument is 0, it will halt the music
 * immediately. Otherwise it will fade to completion over the given number
 * of seconds (or until the end of the song).  Only by fading can you
 * guarantee no audible clicks.
 *
 * This method also clears the queue of any further music.
 *
 * @param fade  The number of seconds to fade out
 */
void AudioQueue::clear(float fade) {
    CUAssertLog(_cover != nullptr, "Attempt to use a disposed audio queue");
    std::shared_ptr<AudioFader> fader = std::dynamic_pointer_cast<AudioFader>(_queue->getCurrent());
    if (fader) {
        if (fade >= 0) {
            _queue->setLoops(0);
            _queue->trim();
            fader->fadeOut(fade);
        } else {
            _queue->clear();
        }
    }
}

/**
 * Pauses the background music, allowing it to be resumed later.
 *
 * Before the music is stopped, this method gives the user an option to
 * fade out the music.  If the argument is 0, it will pause the music
 * immediately. Otherwise it will fade to completion over the given number
 * of seconds (or until the end of the song).  Only by fading can you
 * guarantee no audible clicks.
 *
 * This method has no effect on the music queue.
 *
 * @param fade  The number of seconds to fade out
 */
void AudioQueue::pause(float fade) {
    CUAssertLog(_cover != nullptr, "Attempt to use a disposed audio queue");
    if (fade > 0) {
        _cover->fadePause(fade);
    } else {
        _cover->pause();
    }
}

/**
 * Resumes the background music assuming that it was paused previously.
 *
 * This method has no effect on the music queue.
 */
void AudioQueue::resume() {
    CUAssertLog(_cover != nullptr, "Attempt to use a disposed audio queue");
    if (_cover->isPaused()) {
        _cover->resume();
    }
}

#pragma mark -
#pragma mark Music Queue Management
/**
 * Adds the given music asset to the background music queue
 *
 * Music is handled differently from sound effects. You can only play one
 * music asset at a time. However, it is possible to queue music assets
 * for immediate playback once the active asset is finished. Proper
 * queue management is the keep for smooth, uninterrupted playback that
 * responds to the user's actions.
 *
 * If the queue is empty and there is no active music, this method will
 * play the music immediately.  Otherwise, it will add the music to the
 * queue, and it will play as soon as it is removed from the queue.
 *
 * When the begins playing, the music will start at full volume unless you
 * provide a number of seconds to fade in. However, any cross-fade value
 * set in {@link #setOverlap} will be applied independently of the the
 * fade-in value.
 *
 * Note that looping a song will cause it to block the queue indefinitely
 * until you turn off looping for that asset {@see setLoop}. This can be
 * desired behavior, as it gives you a way to control the speed of the
 * queue processing.
 *
 * @param music     The music asset to queue
 * @param loop      Whether to loop the music continuously
 * @param volume    The music volume (< 0 to use asset default volume)
 * @param fade      The number of seconds to fade in
 */
void AudioQueue::enqueue(const std::shared_ptr<Sound>& music, bool loop, float volume, float fade) {
    CUAssertLog(_cover != nullptr, "Attempt to use a disposed audio queue");
    // Wrap up the music
    std::shared_ptr<audio::AudioNode> player = music->createNode();
    player->setName("__queue_playback__");
    std::shared_ptr<AudioFader> fader = wrapInstance(player);
    fader->setGain(volume);
    if (fade > 0) {
        fader->fadeIn(fade);
    }
    
    _queue->append(fader, loop ? -1 : 0);
}

/**
 * Plays given audio graph as a background track.
 *
 * This alternate version of play allows the programmer to construct
 * custom composite audio graphs and play them as music tracks. Looping
 * behavior is supported if the audio node has a finite duration.
 *
 * If the queue is empty and there is no active music, this method will
 * play the music immediately.  Otherwise, it will add the music to the
 * queue, and it will play as soon as it is removed from the queue.
 *
 * When the begins playing, the music will start at full volume unless you
 * provide a number of seconds to fade in. However, any cross-fade value
 * set in {@link #setOverlap} will be applied independently of the the
 * fade-in value.
 *
 * Note that looping a song will cause it to block the queue indefinitely
 * until you turn off looping for that asset {@see setLoop}. This can be
 * desired behavior, as it gives you a way to control the speed of the
 * queue processing.
 *
 * @param graph     The audio node to play
 * @param loop      Whether to loop the music continuously
 * @param volume    The music volume (relative to the default instance volume)
 * @param fade      The number of seconds to fade in
 */
void AudioQueue::enqueue(const std::shared_ptr<audio::AudioNode>& graph, bool loop,
                         float volume, float fade) {
    CUAssertLog(_cover != nullptr, "Attempt to use a disposed audio queue");
    CUAssertLog(graph->getName() != "__queue_playback__",  "Audio node uses reserved name '__queue_playback__'");
    CUAssertLog(graph->getName() != "__queue_resampler__", "Audio node uses reserved name '__queue_resampler__'");
    std::shared_ptr<AudioFader> fader = wrapInstance(graph);
    fader->setGain(volume);
    if (fade > 0) {
        fader->fadeIn(fade);
    }
    
    _queue->append(fader, loop ? -1 : 0);
}

/**
 * Returns the list of asset identifiers for the music queue
 *
 * For each music track, the identifer is either a file name (if it is
 * a sound sample) or the name of the root node of the sound instance.
 *
 * This list only includes the pending elements in queue, and does
 * not include the asset currently playing.
 *
 * @return the list of asset identifiers for the music queue
 */
const std::vector<std::string> AudioQueue::getElements() const {
    CUAssertLog(_cover != nullptr, "Attempt to use a disposed audio queue");
    std::vector<std::string> result;
    std::deque<std::shared_ptr<AudioNode>> list = _queue->getTail();
    for(auto it = list.begin(); it != list.end(); ++it) {
        std::shared_ptr<AudioNode> source = accessInstance(*it);
        std::string id = source->getName();
        AudioPlayer* player = dynamic_cast<AudioPlayer*>(source.get());
        if (player && id == "__queue_playback__") {
            id = player->getSource()->getFile();
        }
        result.push_back(id);
    }
    return result;
}

/**
 * Returns the size of the music queue
 *
 * @return the size of the music queue
 */
size_t AudioQueue::getPending() const {
    CUAssertLog(_cover != nullptr, "Attempt to use a disposed audio queue");
    return _queue->getTailSize();
}

/**
 * Returns the overlap time in seconds.
 *
 * The overlap time is the amount of time to cross-fade between a music
 * asset and the next. It does not apply to looped music; music assets
 * can never cross-fade with themselves.
 *
 * By default, this value is 0.  Assets play sequentially but do not
 * overlap.  However, you may get smoother transitions between musical
 * segments if you adjust this value. The overlap should be chosen with
 * care.  If the play length of an asset is less than the overlap, the
 * results are undefined.
 *
 * @return the overlap time in seconds.
 */
float AudioQueue::getOverlap() const {
    CUAssertLog(_cover != nullptr, "Attempt to use a disposed audio queue");
    return _queue->getOverlap();
}

/**
 * Sets the overlap time in seconds.
 *
 * The overlap time is the amount of time to cross-fade between a music
 * asset and the next. It does not apply to looped music; music assets
 * can never cross-fade with themselves.
 *
 * By default, this value is 0.  Assets play sequentially but do not
 * overlap.  However, you may get smoother transitions between musical
 * segments if you adjust this value. The overlap should be chosen with
 * care.  If the play length of an asset is less than the overlap, the
 * results are undefined.
 *
 * @param time  The overlap time in seconds.
 */
void AudioQueue::setOverlap(float time)  {
    CUAssertLog(_cover != nullptr, "Attempt to use a disposed audio queue");
    _queue->setOverlap(time);
}

/**
 * Advances ahead in the music queue.
 *
 * The value `step` is the number of songs to skip over. A value of 0 will
 * simply skip over the active music to the next element of the queue. Each
 * value above 0 will skip over one more element in the queue.  If this
 * skipping empties the queue, no music will play.
 *
 * The value `fade` is the number of seconds to fade out the currently
 * playing music assets (if any).  This is to ensure a smooth transition
 * to the next song.  If the music ends naturally, before this time, the
 * fadeout will not carry over to later entries in the queue.
 *
 * @param  steps    The number of elements to skip in the queue
 * @param  fade     The number of seconds to fade out the current asset
 */
void AudioQueue::advance(unsigned int steps, float fade) {
    CUAssertLog(_cover != nullptr, "Attempt to use a disposed audio queue");
    _queue->setLoops(0);
    if (fade > 0.0f) {
        std::shared_ptr<AudioFader> fader = std::dynamic_pointer_cast<AudioFader>(_queue->getCurrent());
        if (fader) {
            fader->fadeOut(fade);
        }
    } else {
        _queue->skip();
    }
    if (steps) {
        _queue->trim(steps);
    }
}

/**
 * Clears the music queue, but does not release any other resources.
 *
 * This method does not stop the current background music from playing. It
 * only clears pending music assets from the queue.
 */
void AudioQueue::clearPending() {
    CUAssertLog(_cover != nullptr, "Attempt to use a disposed audio queue");
    _queue->trim();
}
