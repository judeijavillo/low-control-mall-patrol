//
//  CUAudioEngine.cpp
//  Cornell University Game Library (CUGL)
//
//  This module is an attempt to combine the power of a modern DSP mixer graph
//  with a simple 2000-era interface. Like the legacy engines, it provides a
//  a flat slot-based structure for playing sounds, and controlling the fade
//  and pan of each slot. It also provides support for music queues.
//
//  However, unlike legacy engines, this engine is not limited to playing
//  music samples. It also allows you to attach and play any arbitrary audio
//  node.  For example, you could combine multiple simultaneous source together
//  and play them together to provide vertical layering.  However, these
//  nodes are still wrapped in a top-level fader to prevent clicking like
//  you can get in OpenAL engines.
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

using namespace cugl;
using namespace cugl::audio;

/** Reference to the sound engine singleton */
AudioEngine* AudioEngine::_gEngine = nullptr;

#pragma mark -
#pragma mark Constructors
/**
 * Creates, but does not initialize the singleton audio engine
 *
 * The engine must be initialized before is can be used.
 */
AudioEngine::AudioEngine() :
_capacity(0),
_primary(false) {
    _output = nullptr;
    _mixer  = nullptr;
}

/**
 * Initializes the audio engine.
 *
 * This method initializes the audio engine and constructs the mixer graph
 * for the sound effect channels.  The provided parameter indicates the
 * number of simultaneously supported sounds.
 *
 * @param device    The audio device to use
 * @param slots     The maximum number of sound effect channels to support
 *
 * @return true if the audio engine was successfully initialized.
 */
bool AudioEngine::init(const std::shared_ptr<AudioOutput>& device, Uint32 slots) {
    if (!device) {
        CUAssertLog(device, "Error initializing the output device");
        return false;
    }
    
    _capacity = slots;
    _output = device;
    _mixer  = AudioMixer::alloc(_capacity+1,_output->getChannels(),_output->getRate());
    
    for(int ii = 0; ii <= _capacity; ii++) {
        std::shared_ptr<AudioScheduler> channel;
        channel = audio::AudioScheduler::alloc(_mixer->getChannels(),_mixer->getRate());
        channel->setTag(ii);
        _slots.push_back(channel);
        std::shared_ptr<AudioFader> cover;
        cover = audio::AudioFader::alloc(channel);
        cover->setTag(ii);
        _covers.push_back(cover);
        _mixer->attach(ii,cover);
        
        if (ii < _capacity) {
            channel->setCallback([=](const std::shared_ptr<cugl::audio::AudioNode>& node,
                                  cugl::audio::AudioNode::Action action) {
                if (action != cugl::audio::AudioNode::Action::LOOPBACK) {
                    bool success = (action == cugl::audio::AudioNode::Action::COMPLETE);
                    this->gcollect(node,success);
                }
            });
        } else {
            std::shared_ptr<AudioQueue> music = AudioQueue::alloc(cover);
            if (music != nullptr) {
                _queues.push_back(music);
            }
        }
    }
    
    // Pool needs a fader and panner for 2 times the number of slots
    for(int ii = 0; ii < 2*_capacity; ii++) {
        _fadePool.push_back(AudioFader::alloc(_mixer->getChannels(),_mixer->getRate()));
        _panPool.push_back(AudioPanner::alloc(_mixer->getChannels(),2,_mixer->getRate()));
    }
    
    _output->attach(_mixer);
    return true;
}

/**
 * Releases all resources for this singleton audio engine.
 *
 * Sounds and music assets can no longer be loaded. If you need to use the
 * engine again, you must call init().
 */
void AudioEngine::dispose() {
    if (_capacity) {
        if (_primary) {
            AudioDevices::get()->closeOutput(_output);
            AudioDevices::get()->deactivate();
            AudioDevices::stop();
            _primary = false;
        }

        _covers.clear();
        _slots.clear();
        
        _fadePool.clear();
        _panPool.clear();
        _capacity = 0;
        
		_output = nullptr;
        _mixer = nullptr;
        
        _queues.clear();
		_actives.clear();
        _evicts.clear();
	}
}


#pragma mark -
#pragma mark Internal Helpers
/**
 * Purges this key from the list of active effects.
 *
 * This method is not the same as stopping the channel. A channel may play a
 * little longer after the key is removed.  This is simply a clean-up method.
 *
 * @remove key  The key to purge from the list of active effects.
 */
void AudioEngine::removeKey(const std::string key) {
    _actives.erase(key);
    for(auto it = _evicts.begin(); it != _evicts.end(); ) {
        if (*it == key) {
            it = _evicts.erase(it);
            break;
        } else {
            it++;
        }
    }
}

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
std::shared_ptr<audio::AudioFader> AudioEngine::wrapInstance(const std::shared_ptr<audio::AudioNode>& instance) {
    std::shared_ptr<AudioFader> fader = nullptr;
    if (_fadePool.empty()) {
        fader = AudioFader::alloc(_mixer->getChannels(),_mixer->getRate());
    } else {
        fader = _fadePool.front();
        _fadePool.pop_front();
    }
    std::shared_ptr<AudioPanner> panner = nullptr;
    if (_panPool.empty()) {
        panner = AudioPanner::alloc(_mixer->getChannels(),instance->getChannels(),_mixer->getRate());
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
        sampler->setName("__engine_resampler__");
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
std::shared_ptr<audio::AudioNode> AudioEngine::accessInstance(const std::shared_ptr<audio::AudioNode>& node) const {
    if (node) {
        AudioFader* fader = dynamic_cast<AudioFader*>(node.get());
        AudioPanner* panner = dynamic_cast<AudioPanner*>(fader->getInput().get());
        AudioResampler* sampler = dynamic_cast<AudioResampler*>(panner->getInput().get());
        if (sampler && sampler->getName() == "__engine_resampler__") {
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
std::shared_ptr<audio::AudioNode> AudioEngine::disposeWrapper(const std::shared_ptr<audio::AudioNode>& node) {
    if (node) {
        std::shared_ptr<AudioFader> fader   = std::dynamic_pointer_cast<AudioFader>(node);
        std::shared_ptr<AudioPanner> panner = std::dynamic_pointer_cast<AudioPanner>(fader->getInput());
        if (panner) {
            std::shared_ptr<AudioNode> source = panner->getInput();
            std::shared_ptr<AudioResampler> sampler = std::dynamic_pointer_cast<AudioResampler>(source);
            if (sampler && sampler->getName() == "__engine_resampler__") {
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
 * Callback function for when a sound effect channel finishes
 *
 * This method is called when the active sound effect completes. It disposes
 * any audio nodes (faders, panners), recycling them for later.  It also
 * allows the key to be reused for later effects.  Finally, it invokes any
 * callback functions associated with the sound effect channels.
 *
 * This method is never intended to be accessed by general users.
 *
 * @param sound     The playback instance for the sound asset
 * @param status    True if the music terminated normally, false otherwise.
 */
void AudioEngine::gcollect(const std::shared_ptr<audio::AudioNode>& sound, bool status) {
    std::string key = sound->getName();
    disposeWrapper(sound);
    removeKey(key);
    if (_callback) {
        _callback(key,status);
    }
}

#pragma mark -
#pragma mark Static Accessors
/**
 * Starts the singleton audio engine on the default audio device.
 *
 * Once this method is called, the method get() will no longer return
 * nullptr.  Calling the method multiple times (without calling stop) will
 * have no effect.
 *
 * This convenience method will start up the {@link AudioDevice} manager,
 * and take responsibility for shutting it down when done. As a result,
 * it will fail if the audio device manager is already active or cannot
 * be initialized.  If you need more control of the audio devices (such
 * as to change the audio sampling rate or the buffer size), you should
 * use start the engine with a specific output device.
 *
 * The engine initialized by this method has a uniform sampling rate of
 * 48000 Hz.  This is the standard rate for phone games. However, keep
 * in mind that CD audio is typically sampled at 44100 Hz.
 *
 * The parameter `slots` indicates the number of simultaneously supported
 * sounds.  Attempting to play more than this number of sounds may fail,
 * it may eject a previously playing sound, depending on the settings.
 * The default number of slots is 16.
 *
 * @param slots  The maximum number of sound slots to support
 *
 * @return true if the engine was successfully initialized
 */
bool AudioEngine::start(Uint32 slots) {
    if (_gEngine != nullptr) {
        return false;
    } else if (AudioDevices::get()) {
        CUAssertLog(false,"Audio devices are currently in use");
        return false;
    }
    
    AudioDevices::start();
    std::shared_ptr<AudioOutput> device = AudioDevices::get()->openOutput();
    _gEngine = new AudioEngine();
    
    if (!_gEngine->init(device,slots)) {
        delete _gEngine;
        _gEngine = nullptr;
        AudioDevices::get()->closeOutput(device);
        AudioDevices::stop();
        CUAssertLog(false,"Audio engine failed to initialize");
    }
    _gEngine->_primary = true;
    AudioDevices::get()->activate();
    return true;

}

/**
 * Starts the singleton audio engine on the given audio device.
 *
 * Once this method is called, the method get() will no longer return
 * nullptr.  Calling the method multiple times (without calling stop) will
 * have no effect.
 *
 * This version of the method assumes that the programmer has already
 * started the {@link AudioDevice} manager. It will not restart the
 * manager, nor will it shutdown the audio manager when done.  This
 * version of the initializer is only for programmers that need
 * lower-level control over buffer size and sampling rate.
 *
 * The parameter `slots` indicates the number of simultaneously supported
 * sounds.  Attempting to play more than this number of sounds may fail,
 * it may eject a previously playing sound, depending on the settings.
 * The default number of slots is 16.
 *
 * @param slots  The maximum number of sound slots to support
 *
 * @return true if the engine was successfully initialized
 */
bool AudioEngine::start(const std::shared_ptr<audio::AudioOutput>& device, Uint32 slots) {
    if (_gEngine != nullptr) {
        return false;
    }
    _gEngine = new AudioEngine();
    if (!_gEngine->init(device,slots)) {
        delete _gEngine;
        _gEngine = nullptr;
        CUAssertLog(false,"Audio engine failed to initialize");
    }
    return true;
}

/**
 * Shutsdown the singleton audio engine, releasing all resources.
 *
 * Once this method is called, the method get() will return nullptr.
 * Calling the method multiple times (without calling stop) will have
 * no effect.
 *
 * If the engine was started with the convenience method {@link #start(Uint32)},
 * then this method will also stop the {@link AudioDevice} manager.
 * Otherwise, it is the responsibility of the programmer to shutdown
 * the device manager.
 */
void AudioEngine::stop() {
    if (_gEngine == nullptr) {
        return;
    }
    _gEngine->dispose();
    delete _gEngine;
    _gEngine = nullptr;
}

#pragma mark -
#pragma mark Music Playback
/**
 * Returns the default music queue for this audio engine
 *
 * Music is managed through audio queues.The audio engine has one
 * by default, though you can allocate more with {@link #allocQueue}.
 *
 * Music is handled differently from sound effects. You can only play one
 * music asset at a time. However, it is possible to queue music assets
 * for immediate playback once the active asset is finished. Proper
 * queue management is the keep for smooth, uninterrupted playback that
 * responds to the user's actions.
 */
std::shared_ptr<AudioQueue> AudioEngine::getMusicQueue() const {
    return _queues[0];
}

/**
 * Allocates a new queue for managing audio.
 *
 * Music is handled differently from sound effects. You can only play one
 * music asset at a time. However, it is possible to queue music assets
 * for immediate playback once the active asset is finished. Proper
 * queue management is the keep for smooth, uninterrupted playback that
 * responds to the user's actions.
 *
 * This method allocates a secondary music queue that can be played in
 * tandem with the primary music queue.  This allows for slightly more
 * complex music mixing.  However, for true vertical layering, you should
 * use {@link AudioSequencer}.
 *
 * It is the programmer's responsibility to free all secondary music
 * queues with {@link #freeQueue}.  However, all queues are automatically
 * freed when this audio engine is stopped.
 *
 * Calling this method will briefly pause the audio engine, if it is
 * actively playing.
 *
 * @return a newly allocated audio queue
 */
std::shared_ptr<AudioQueue> AudioEngine::allocQueue() {
    CUAssertLog(_mixer->getWidth() < (Uint8)-1, "Mixer width exceeds maximum capacity");
    bool paused = _output->isPaused();
    if (!paused) {
        _output->pause();
    }

    std::shared_ptr<AudioScheduler> channel;
    channel = audio::AudioScheduler::alloc(_mixer->getChannels(),_mixer->getRate());
    _slots.push_back(channel);
    std::shared_ptr<AudioFader> cover;
    cover = audio::AudioFader::alloc(channel);

    Uint32 ii = _mixer->getWidth();
    channel->setTag(ii);
    cover->setTag(ii);
    _mixer->setWidth(_mixer->getWidth()+1);
    _mixer->attach(ii,cover);


    std::shared_ptr<AudioQueue> music = std::make_shared<AudioQueue>();
    if (music->init(cover)) {
        _queues.push_back(music);
    }
    
    if (paused) {
        _output->resume();
    }
    
    return music;
}

/**
 * Frees a previously allocated audio queue.
 *
 * This method should be called to free any audio queue created by
 * {@link #allocQueue}. It is the programmer's responsibility to free
 * all secondary music queues. However, all queues are automatically
 * freed when this audio engine is stopped.
 *
 * This method cannot be used to free the default music queue.
 *
 * @param the audio queue to free
 */
void AudioEngine::freeQueue(const std::shared_ptr<AudioQueue>& queue) {
    // Make sure this is a non-default queue
     auto it = find(_queues.begin(), _queues.end(), queue);
    size_t pos = 0;
    if (it == _queues.end()) {
        CUAssertLog(false, "Provided queue is not valid");
        return;
    } else {
        pos = it-_queues.begin();
    }
    
    if (pos == 0) {
        CUAssertLog(false, "Attempt to release default queue");
        return;
    }
    
    bool paused = _output->isPaused();
    if (!paused) {
        _output->pause();
    }

    _mixer->detach(pos+_capacity);
    for(size_t ii = pos+1; ii < _mixer->getWidth(); ii++) {
        std::shared_ptr<AudioFader> fader = std::dynamic_pointer_cast<AudioFader>(_mixer->detach(ii+_capacity));
        fader->setTag((Uint32)(ii-1));
        fader->getInput()->setTag((Uint32)(ii-1));
        _mixer->attach(ii+_capacity-1, fader);
    }
    _mixer->setWidth(_mixer->getWidth()-1);
    _queues.erase(_queues.begin()+pos);
    queue->dispose();
}


#pragma mark -
#pragma mark Sound Management
/**
 * Plays the given sound, and associates it with the specified key.
 *
 * Sounds are associated with a reference key. This allows the application
 * to easily reference the sound state without having to internally manage
 * pointers to the audio channel.
 *
 * If the key is already associated with an active sound effect, this
 * method will stop the existing sound and replace it with this one. It
 * is the responsibility of the application layer to manage key usage.
 *
 * There are a limited number of slots available for sounds. If you go
 * over the number available, the sound will not play unless `force` is
 * true. In that case, it will grab the channel from the longest playing
 * sound effect.
 *
 * @param  key      The reference key for the sound effect
 * @param  sound    The sound effect to play
 * @param  loop     Whether to loop the sound effect continuously
 * @param  volume   The music volume (relative to the default asset volume)
 * @param  force    Whether to force another sound to stop.
 *
 * @return true if there was an available channel for the sound
 */
bool AudioEngine::play(const std::string key, const std::shared_ptr<Sound>& sound,
                       bool loop, float volume, bool force) {
    CUAssertLog(_output != nullptr, "Attempt to use an unintiatialized audio engine");

    if (isActive(key)) {
        if (force) {
            clear(key,0);
            removeKey(key);
        } else {
            CULogError("Sound effect key is in use");
            return false;
        }
    }
    
    // Find an empty scheduler
    int audioID = -1;
    for(auto it = _slots.begin()+1; audioID == -1 && it != _slots.end(); ++it) {
        if (!(*it)->isPlaying()) {
            audioID = (*it)->getTag();
        }
    }
    
    // Try again for soon to be deleted.
    for(auto it = _actives.begin(); audioID == -1 && it != _actives.end(); ++it) {
        if (it->second->isFadeOut()) {
            Uint32 tag = it->second->getTag();
            if (!_slots[tag]->getTailSize()) {
                audioID = tag;
            }
        }
    }
    
    // TODO: Apparently not being evicted fast enough
    if (audioID == -1) {
        if (force && !_evicts.empty()) {
            std::string altkey = _evicts.front();
            audioID = _actives[altkey]->getTag();
            clear(altkey);
        } else {
            // Fail if nothing available
            CULogError("No available sound channels");
            return false;
        }
    }
    
    std::shared_ptr<audio::AudioNode> player = sound->createNode();
    player->setName("__engine_playback__");

    std::shared_ptr<AudioFader> fader = wrapInstance(player);
    fader->setGain(volume);
    fader->setTag(audioID);
    fader->setName(key);
    _slots[audioID]->play(fader, loop ? -1 : 0);
    _actives.emplace(key,fader);
    _evicts.push_back(key);
    return true;
}

/**
 * Plays the given audio node, and associates it with the specified key.
 *
 * This alternate version of play allows the programmer to construct
 * custom composite audio graphs and play them as sound effects. Looping
 * behavior is supported if the audio node has a finite duration.
 *
 * As with traditional sounds, the audio node is assigned a key to allow
 * the application to easily reference the sound state without having to
 * internally manage pointers to the audio channel. In particular, if
 * the audio node provided does not have a fixed duration, and can be
 * played indefinitely, then the key must be used to stop the sound.
 *
 * If the key is already associated with an active sound effect, this
 * method will stop the existing sound and replace it with this one. It
 * is the responsibility of the application layer to manage key usage.
 *
 * There are a limited number of slots available for sounds. If you go
 * over the number available, the sound will not play unless `force` is
 * true. In that case, it will grab the channel from the longest playing
 * sound effect.
 *
 * @param  key      The reference key for the sound effect
 * @param  graph    The audio graph to play
 * @param  loop     Whether to loop the sound effect continuously
 * @param  volume   The music volume (relative to the default instance volume)
 * @param  force    Whether to force another sound to stop.
 *
 * @return true if there was an available channel for the sound
 */
bool AudioEngine::play(const std::string key, const std::shared_ptr<audio::AudioNode>& graph,
                       bool loop, float volume, bool force) {
    CUAssertLog(_output != nullptr, "Attempt to use an unintiatialized audio engine");
    CUAssertLog(graph->getName() != "__engine_playback__",  "Audio node uses reserved name '__engine_playback__'");
    CUAssertLog(graph->getName() != "__engine_resampler__", "Audio node uses reserved name '__engine_resampler__'");

    if (isActive(key)) {
        if (force) {
            clear(key,0);
            removeKey(key);
        } else {
            CULogError("Sound effect key is in use");
            return false;
        }
    }
    
    // Find an empty scheduler
    int audioID = -1;
    for(auto it = _slots.begin()+1; audioID == -1 && it != _slots.end(); ++it) {
        if (!(*it)->isPlaying()) {
            audioID = (*it)->getTag();
        }
    }
    
    // Try again for soon to be deleted.
    for(auto it = _actives.begin(); audioID == -1 && it != _actives.end(); ++it) {
        if (it->second->isFadeOut()) {
            Uint32 tag = it->second->getTag();
            if (!_slots[tag]->getTailSize()) {
                audioID = tag;
            }
        }
    }
    
    // TODO: Apparently not being evicted fast enough
    if (audioID == -1) {
        if (force && !_evicts.empty()) {
            std::string altkey = _evicts.front();
            audioID = _actives[altkey]->getTag();
            clear(altkey);
        } else {
            // Fail if nothing available
            CULogError("No available sound channels");
            return false;
        }
    }

    std::shared_ptr<AudioFader> fader = wrapInstance(graph);
    fader->setGain(volume);
    fader->setTag(audioID);
    fader->setName(key);
    _slots[audioID]->play(fader, loop ? -1 : 0);
    _actives.emplace(key,fader);
    _evicts.push_back(key);
    return true;
}


/**
 * Returns the current state of the sound effect for the given key.
 *
 * If there is no sound effect for the given key, it returns
 * State::INACTIVE.
 *
 * @param  key  the reference key for the sound effect
 *
 * @return the current state of the sound effect for the given key.
 */
AudioEngine::State AudioEngine::getState(const std::string key) const {
    CUAssertLog(_output != nullptr, "Attempt to use an unintiatialized audio engine");
    if (_actives.find(key) == _actives.end()) {
        return State::INACTIVE;
    }
    
    std::shared_ptr<AudioNode> node = _actives.at(key);
    std::shared_ptr<audio::AudioScheduler> slot = _slots.at(node->getTag());
    if (!slot->isPlaying()) {
        return State::INACTIVE;
    } else if (node->isPaused() || slot->isPaused()) {
        return State::PAUSED;
    }
    
    return State::PLAYING;
}

/**
 * Returns the identifier for the asset attached to the given key.
 *
 * If the current playing track is an {@link Sound} asset, then the
 * identifier is the file name.  Otherwise, it is the name of the root
 * of the audio graph.  See {@link audio::AudioNode#getName}.
 *
 * @param  key  the reference key for the sound effect
 *
 * @return the identifier for the asset attached to the given key.
 */
const std::string AudioEngine::getSource(const std::string key) const {
    CUAssertLog(_output != nullptr, "Attempt to use an unintiatialized audio engine");
    if (_actives.find(key) == _actives.end()) {
        return std::string();
    }

    std::shared_ptr<AudioNode> source = accessInstance(_actives.at(key));
    std::string id = source->getName();
    AudioPlayer* player = dynamic_cast<AudioPlayer*>(source.get());
    if (player && id == "__engine_playback__") {
        id = player->getSource()->getFile();
    }

    return id;
}

/**
 * Returns true if the sound effect is in a continuous loop.
 *
 * If the key does not correspond to an active sound effect, this
 * method returns false.
 *
 * @param  key  the reference key for the sound effect
 *
 * @return true if the sound effect is in a continuous loop.
 */
bool AudioEngine::isLoop(const std::string key) const {
    CUAssertLog(_output != nullptr, "Attempt to use an unintiatialized audio engine");
    if (_actives.find(key) != _actives.end()) {
        std::shared_ptr<AudioNode> node = _actives.at(key);
        return _slots.at(node->getTag())->getLoops() != 0;
    }
    return false;

}

/**
 * Sets whether the sound effect is in a continuous loop.
 *
 * If the key does not correspond to an active sound effect, this
 * method does nothing.
 *
 * @param  key  the reference key for the sound effect
 * @param  loop whether the sound effect is in a continuous loop
 */
void AudioEngine::setLoop(const std::string key, bool loop) {
    CUAssertLog(_output != nullptr, "Attempt to use an unintiatialized audio engine");
    if (_actives.find(key) != _actives.end()) {
        std::shared_ptr<AudioNode> node = _actives.at(key);
        _slots[node->getTag()]->setLoops(loop ? -1 : 0);
    }
}

/**
 * Returns the current volume of the sound effect.
 *
 * The volume is a value 0 to 1, where 1 is maximum volume and 0 is
 * complete silence. If the key does not correspond to an active
 * sound effect, this method returns 0.
 *
 * Note that this is the playback volume.  If the asset or audio graph
 * had its own initial volume setting, this is independent of this setting.
 * Indeed, this value can be though of as the percentage of the default
 * volume.
 *
 * @param  key  the reference key for the sound effect
 *
 * @return the current volume of the sound effect
 */
float AudioEngine::getVolume(const std::string key) const {
    CUAssertLog(_output != nullptr, "Attempt to use an unintiatialized audio engine");
    if (_actives.find(key) != _actives.end()) {
        Uint32 tag = _actives.at(key)->getTag();
        std::shared_ptr<AudioNode> node = _slots[tag]->getCurrent();
        return node->getGain();
    }
    return 0;
}

/**
 * Sets the current volume of the sound effect.
 *
 * The volume is a value 0 to 1, where 1 is maximum volume and 0 is
 * complete silence. If the key does not correspond to an active
 * sound effect, this method does nothing.
 *
 * Note that this is the playback volume.  If the asset or audio graph
 * had its own initial volume setting, this is independent of this setting.
 * Indeed, this value can be though of as the percentage of the default
 * volume.
 *
 * @param  key      the reference key for the sound effect
 * @param  volume   the current volume of the sound effect
 */
void AudioEngine::setVolume(const std::string key, float volume) {
    CUAssertLog(_output != nullptr, "Attempt to use an unintiatialized audio engine");
    if (_actives.find(key) != _actives.end()) {
        Uint32 tag = _actives.at(key)->getTag();
        std::shared_ptr<AudioNode> node = _slots[tag]->getCurrent();
        node->setGain(volume);
    }
}

/**
 * Returns the stereo pan of the sound effect.
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
 * If the key does not correspond to an active sound effect, this
 * method returns 0.
 *
 * @param  key      the reference key for the sound effect
 *
 * @return the stereo pan of the sound effect
 */
float AudioEngine::getPanFactor(const std::string& key) const {
    CUAssertLog(_output != nullptr, "Attempt to use an unintiatialized audio engine");
    if (_actives.find(key) != _actives.end()) {
        std::shared_ptr<AudioFader> fader = _actives.at(key);
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
 * Returns the stereo pan of the sound effect.
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
 * If the key does not correspond to an active sound effect, this
 * method does nothing.
 *
 * @param  key  the reference key for the sound effect
 * @param  pan  the stereo pan of the sound effect
 */
void AudioEngine::setPanFactor(const std::string key, float pan) {
    CUAssertLog(_output != nullptr, "Attempt to use an unintiatialized audio engine");
    CUAssertLog(pan >= -1 && pan <= 1, "Pan value %f is out of range",pan);
    if (_actives.find(key) != _actives.end()) {
        std::shared_ptr<AudioFader> fader = _actives.at(key);
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
 * Returns the duration of the sound effect, in seconds.
 *
 * Because most sound effects are fully decompressed at load time,
 * the result of this method is reasonably accurate.
 *
 * If the key does not correspond to an active sound effect, this
 * method returns -1.
 *
 * @param  key  the reference key for the sound effect
 *
 * @return the duration of the sound effect, in seconds.
 */
float AudioEngine::getDuration(const std::string key) const  {
    CUAssertLog(_output != nullptr, "Attempt to use an unintiatialized audio engine");
    if (_actives.find(key) != _actives.end()) {
        std::shared_ptr<audio::AudioNode> source = accessInstance(_actives.at(key));
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
    return -1;
}

/**
 * Returns the elapsed time of the sound effect, in seconds
 *
 * The elapsed time is the current position of the sound from the beginning.
 * It does not include any time spent on a continuous loop. Because most
 * sound effects are fully decompressed at load time, the result of this
 * method is reasonably accurate, though it is affected by device latency.

 *
 * If the key does not correspond to an active sound effect, or if the
 * sound effect is an audio node with undefined duration, this method
 * returns -1.
 *
 * @param  key  the reference key for the sound effect
 *
 * @return the elapsed time of the sound effect, in seconds
 */
float AudioEngine::getTimeElapsed(const std::string key) const {
    CUAssertLog(_output != nullptr, "Attempt to use an unintiatialized audio engine");
    if (_actives.find(key) != _actives.end()) {
        return _actives.at(key)->getElapsed();
    }
    return -1;
}


/**
 * Sets the elapsed time of the sound effect, in seconds
 *
 * The elapsed time is the current position of the sound from the beginning.
 * It does not include any time spent on a continuous loop.  Because most
 * sound effects are fully decompressed at load time, the result of this
 * method is reasonably accurate, though it is affected by device latency.

 *
 * If the key does not correspond to an active sound effect, or if the
 * sound effect is an audio node with undefined duration, this method
 * does nothing.
 *
 * @param  key  the reference key for the sound effect
 * @param  time the new position of the sound effect
 */
void AudioEngine::setTimeElapsed(const std::string key, float time) {
    CUAssertLog(_output != nullptr, "Attempt to use an unintiatialized audio engine");
    if (_actives.find(key) != _actives.end()) {
        _actives.at(key)->setElapsed(time);
    }
}

/**
 * Returns the time remaining for the sound effect, in seconds
 *
 * The time remaining is just duration-elapsed.  This method does not take
 * into account whether the sound is on a loop. Because most sound effects
 * are fully decompressed at load time, the result of this method is
 * reasonably accurate, though it is affected by device latency.
 *
 * If the key does not correspond to an active sound effect, or if the
 * sound effect is an audio node with undefined duration, this method
 * method returns -1.
 *
 * @param  key  the reference key for the sound effect
 *
 * @return the time remaining for the sound effect, in seconds
 */
float AudioEngine::geTimeRemaining(const std::string key) const  {
    CUAssertLog(_output != nullptr, "Attempt to use an unintiatialized audio engine");
    if (_actives.find(key) != _actives.end()) {
        return _actives.at(key)->getRemaining();
    }
    return -1;
}

/**
 * Sets the time remaining for the sound effect, in seconds
 *
 * The time remaining is just duration-elapsed.  This method does not take
 * into account whether the sound is on a loop. Because most sound effects
 * are fully decompressed at load time, the result of this method is
 * reasonably accurate, though it is affected by device latency.
 *
 * If the key does not correspond to an active sound effect, or if the
 * sound effect is an audio node with undefined duration, this method
 * does nothing.
 *
 * @param  key  the reference key for the sound effect
 * @param  time the new time remaining for the sound effect
 */
void AudioEngine::setTimeRemaining(const std::string key, float time) {
    CUAssertLog(_output != nullptr, "Attempt to use an unintiatialized audio engine");
    if (_actives.find(key) != _actives.end()) {
        _actives.at(key)->setRemaining(time);
    }
}


/**
 * Removes the sound effect for the given key, stopping it immediately
 *
 * The effect will be removed from the audio engine entirely. You will need
 * to add it again if you wish to replay it.
 *
 * Before the effect is stopped, this method gives the user an option to
 * fade out the effect.  If the argument is 0, it will halt the sound
 * immediately. Otherwise it will fade to completion over the given number
 * of seconds (or until the end of the effect).  Only by fading can you
 * guarantee no audible clicks.
 *
 * If the key does not correspond to an active sound effect, this method
 * does nothing.
 *
 * @param  key  the reference key for the sound effect
 * @param fade  the number of seconds to fade out
 */
void AudioEngine::clear(const std::string key,float fade) {
    CUAssertLog(_output != nullptr, "Attempt to use an unintiatialized audio engine");
    if (_actives.find(key) != _actives.end()) {
        std::shared_ptr<AudioFader> node = _actives.at(key);
        _slots[node->getTag()]->setLoops(0);
        node->fadeOut(fade);
    }

}


/**
 * Pauses the sound effect for the given key.
 *
 * Before the effect is paused, this method gives the user an option to
 * fade out the effect.  If the argument is 0, it will pause the sound
 * immediately. Otherwise it will fade to completion over the given number
 * of seconds (or until the end of the effect).  Only by fading can you
 * guarantee no audible clicks.
 *
 * If the key does not correspond to an active sound effect, this method
 * does nothing.
 *
 * @param  key  the reference key for the sound effect
 * @param fade  the number of seconds to fade out
 */
void AudioEngine::pause(const std::string key,float fade) {
    CUAssertLog(_output != nullptr, "Attempt to use an unintiatialized audio engine");
    if (_actives.find(key) != _actives.end()) {
        std::shared_ptr<AudioFader> node = _actives.at(key);
        node->fadePause(fade);
    }
}

/**
 * Resumes the sound effect for the given key.
 *
 * If the key does not correspond to a channel, this method does nothing.
 *
 * @param  key  the reference key for the sound effect
 */
void AudioEngine::resume(std::string key) {
    CUAssertLog(_output != nullptr, "Attempt to use an unintiatialized audio engine");
    if (_actives.find(key) != _actives.end()) {
        std::shared_ptr<AudioFader> node = _actives.at(key);
        node->resume();
    }
}


#pragma mark -
#pragma mark Global Management
/**
 * Removes all sound effects from the engine, stopping them immediately.
 *
 * Before the effects are stopped, this method gives the user an option to
 * fade out the effect.  If the argument is 0, it will halt all effects
 * immediately. Otherwise it will fade the to completion over the given
 * number of seconds (or until the end of the effect).  Only by fading can
 * you guarantee no audible clicks.
 *
 * You will need to add the effects again if you wish to replay them.
 * This method has no effect on the music queues.
 *
 * @param fade the number of seconds to fade out
 */
void AudioEngine::clearEffects(float fade) {
    CUAssertLog(_output != nullptr, "Attempt to use an unintiatialized audio engine");
    for(auto it = _actives.begin(); it != _actives.end(); ++it) {
        it->second->fadeOut(fade);
    }
    _actives.clear();
    _evicts.clear();
}

/**
 * Pauses all sound effects, allowing them to be resumed later.
 *
 * Before the effects are paused, this method gives the user an option to
 * fade out the effect.  If the argument is 0, it will pause all effects
 * immediately. Otherwise it will fade the to completion over the given
 * number of seconds (or until the end of the effect).  Only by fading can
 * you guarantee no audible clicks.
 *
 * Sound effects already paused will remain paused. This method has no
 * effect on the music queues.
 *
 * @param fade      the number of seconds to fade out
 */
void AudioEngine::pauseEffects(float fade) {
    CUAssertLog(_output != nullptr, "Attempt to use an unintiatialized audio engine");
    for(size_t ii = 0; ii < _capacity; ii++) {
        if (fade) {
            _covers[ii]->fadePause(fade);
        } else {
            _covers[ii]->pause();
        }
    }
}


/**
 * Resumes all paused sound effects.
 *
 * This method has no effect on the music queues.
 */
void AudioEngine::resumeEffects() {
    CUAssertLog(_output != nullptr, "Attempt to use an unintiatialized audio engine");
    for(size_t ii = 0; ii < _capacity; ii++) {
        _covers[ii]->resume();
    }
}

/**
 * Clears all active playing sounds, both music and sound effects.
 *
 * Before the sounds are stopped, this method gives the user an option to
 * fade out the effect. If the argument is 0, it will halt all sounds
 * immediately. Otherwise it will fade the to completion over the given
 * number of seconds (or until the end of the effect).  Only by fading can
 * you guarantee no audible clicks.
 *
 * @param fade the number of seconds to fade out
 */
void AudioEngine::clear(float fade) {
    CUAssertLog(_output != nullptr, "Attempt to use an unintiatialized audio engine");
    clearEffects(fade);
    for(auto it = _queues.begin(); it != _queues.end(); ++it) {
        (*it)->clear(fade);
    }
}


/**
 * Pauses all sounds, both music and sound effects.
 *
 * Before the sounds are paused, this method gives the user an option to
 * fade out everything.  If the argument is 0, it will pause the sounds
 * immediately. Otherwise it will fade everythign to completion over the
 * given number of seconds (or until the end of each sound).  Only by
 * fading can you guarantee no audible clicks.
 *
 * This method allows them to be resumed later. You should generally
 * call this method just before the app pages to the background.
 *
 * @param fade      the number of seconds to fade out
 */
void AudioEngine::pause(float fade)  {
    CUAssertLog(_output != nullptr, "Attempt to use an unintiatialized audio engine");
    pauseEffects(fade);
    for(auto it = _queues.begin(); it != _queues.end(); ++it) {
        (*it)->pause();
    }
}

/**
 * Resumes all paused sounds, both music and sound effects.
 *
 * You should generally call this method right after the app returns
 * from the background.
 */
void AudioEngine::resume()  {
    CUAssertLog(_output != nullptr, "Attempt to use an unintiatialized audio engine");
    resumeEffects();
    for(auto it = _queues.begin(); it != _queues.end(); ++it) {
        (*it)->resume();
    }
}

