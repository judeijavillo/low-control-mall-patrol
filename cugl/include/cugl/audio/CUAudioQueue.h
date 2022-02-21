//
//  CUAudioQueue.h
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
#ifndef __CU_AUDIO_QUEUE_H__
#define __CU_AUDIO_QUEUE_H__
#include <cugl/audio/CUAudioEngine.h>
#include <functional>
#include <memory>
#include <vector>

namespace cugl {
/**
 * Creates a queue for seamless audio playback.
 *
 * Unless your game music consists of a single audio loop, you want to be
 * able to dynamically switch up your audio on the fly. But this is not
 * possible in the default {@link AudioEngine}. That engine is optimized
 * for playing sounds simultaneously. Its design does not allow you to
 * stop one sound and start another immediately. Either the two sounds
 * will overlap a little bit or there will be a noticeable gap in-between.
 *
 * To manage seamless playback, you have to use a queue.  This is true even
 * in legacy audio engines like OpenAL. The queue guarantees that the next
 * sample in the queue will be played sequentially with no break in audio.
 * The queue provides some minor crossfade support via {@link #setOverlap}
 * for loops that do not align perfectly.
 *
 * The primary difference between this class and an OpenAL style audio
 * queue is that it is not limited to sample playback.  As with
 * {@link AudioEngine}, you can add any audio node graphs to the queue
 * for playback. However, be careful with this, as any infinite-playing
 * node can stall the queue.  Fortunately, the method {@link #advance}
 * allows the programmer to manually progress through the queue.
 *
 * IMPORTANT: Like the OpenGL context, this class is not thread-safe.  It is
 * only safe to access this class in the main application thread.  This means
 * it should never be called in a call-back function as those are typically
 * executed in the host thread.  If you need to access the an audio queue in a
 * callback function, you should use the {@link Application#schedule} method
 * to delay until the main thread is next available.
 */
class AudioQueue {
private:
    /** The global fader for this queue */
    std::shared_ptr<audio::AudioFader> _cover;
    /** The queue scheduler (the primary queue interface) */
    std::shared_ptr<audio::AudioScheduler> _queue;
    
    /** An object pool of faders for individual sound instances */
    std::deque<std::shared_ptr<audio::AudioFader>>  _fadePool;
    /** An object pool of panners for panning sound assets */
    std::deque<std::shared_ptr<audio::AudioPanner>> _panPool;
    
    /**
     * Callback function for background music
     *
     * This function is called whenever a background music track completes.
     * It is called whether or not the music completed normally or if it
     * was terminated manually.  However, the second parameter can be used
     * to distinguish the two cases.
     *
     * The asset identifier is the file name if the music is an audio sample.
     * If it is an arbitrary audio graph, it is the name of the root node of
     * that graph.  See {@link audio::AudionNode#getName}.
     *
     * @param asset     The identifier for the music asset that just completed
     * @param status    True if the music terminated normally, false otherwise.
     */
    std::function<void(const std::string asset, bool status)> _callback;

#pragma mark -
#pragma mark Constructors (Private)
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
    bool init(const std::shared_ptr<audio::AudioFader>& slot);

    /**
     * Releases all resources for this audio queue.
     *
     * Music tracks can no longer be queued. If you need to use the queue again,
     * you must call init().
     */
    void dispose();
    
    /**
     * Allocates the audio queue.
     *
     * This method initializes the audio queue, assigning it a single schedular node.
     * This node is still owned by the audio engine.  Hence changes in state to the
     * audio engine may affect this queue.
     *
     * @param slot  The audio engine slot for this queue
     *
     * @return the newly allocated audio queue
     */
    static std::shared_ptr<AudioQueue> alloc(const std::shared_ptr<audio::AudioFader>& slot) {
        std::shared_ptr<AudioQueue> result = std::make_shared<AudioQueue>();
        return (result->init(slot) ? result : nullptr);

    }


#pragma mark -
#pragma mark Internal Helpers
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
    std::shared_ptr<audio::AudioFader> wrapInstance(const std::shared_ptr<audio::AudioNode>& instance);

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
    std::shared_ptr<audio::AudioNode> accessInstance(const std::shared_ptr<audio::AudioNode>& node) const;

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
    std::shared_ptr<audio::AudioNode> disposeWrapper(const std::shared_ptr<audio::AudioNode>& node);
    
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
    void gcollect(const std::shared_ptr<audio::AudioNode>& instance, bool status);

    /** Allow the audio engine access */
    friend class AudioEngine;
    
#pragma mark -
#pragma mark Music Playback
public:
    /**
     * Creates, but does not initialize an audio queue.
     *
     * The queue must be initialized before is can be used.
     */
    AudioQueue();

    /**
     * Disposes of the singleton audio engine.
     *
     * This destructor releases all of the resources associated with this
     * audio queue.
     */
    ~AudioQueue() { dispose(); }
    
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
     * @param fade      The number of seconds to fade in
     */
    void play(const std::shared_ptr<Sound>& music, bool loop=false,
              float volume=1.0f, float fade=0.0f);

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
    void play(const std::shared_ptr<audio::AudioNode>& graph, bool loop=false,
              float volume=1.0f, float fade=0.0f);

    /**
     * Returns the identifier for the track currently playing
     *
     * If the current playing track is an {@link Sound} asset, then the
     * identifier is the file name.  Otherwise, it is the name of the root
     * of the audio graph.  See {@link audio::AudioNode#getName}.
     *
     * @return the identifier for the track currently playing
     */
    const std::string current() const;

    /**
     * Returns the current state of the background music
     *
     * @return the current state of the background music
     */
    AudioEngine::State getState() const;

    /**
     * Returns true if the background music is in a continuous loop.
     *
     * If there is no active background music, this method will return false.
     *
     * @return true if the background music is in a continuous loop.
     */
    bool isLoop() const;

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
    void setLoop(bool loop);

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
    float getVolume() const;

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
    void setVolume(float volume);

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
    float getPanFactor() const;

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
    void setPanFactor(float pan);

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
    float getDuration() const;

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
    float getTimeElapsed() const;

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
    void setTimeElapsed(float time);

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
    float getTimeRemaining() const;


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
    void setTimeRemaining(float time);

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
    void clear(float fade=DEFAULT_FADE);

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
    void pause(float fade=DEFAULT_FADE);

    /**
     * Resumes the background music assuming that it was paused previously.
     *
     * This method has no effect on the music queue.
     */
    void resume();

    /**
     * Sets the callback for background music
     *
     * This callback function is called whenever a background music track
     * completes. It is called whether or not the music completed normally or
     * if it was terminated manually.  However, the second parameter can be
     * used to distinguish the two cases.
     *
     * @param callback The callback for background music
     */
    void setListener(std::function<void(const std::string,bool)> callback) {
        _callback = callback;
    }

    /**
     * Returns the callback for background music
     *
     * This callback function is called whenever a background music track
     * completes. It is called whether or not the music completed normally or
     * if it was terminated manually.  However, the second parameter can be
     * used to distinguish the two cases.
     *
     * @return the callback for background music
     */
    std::function<void(const std::string,bool)> getMusicListener() const {
        return _callback;
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
     * @param volume    The music volume (relative to the default instance volume)
     * @param fade      The number of seconds to fade in
     */
    void enqueue(const std::shared_ptr<Sound>& music, bool loop=false,
                 float volume=1.0f, float fade=0.0f);

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
    void enqueue(const std::shared_ptr<audio::AudioNode>& graph, bool loop=false,
                 float volume=1.0f, float fade=0.0f);

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
    const std::vector<std::string> getElements() const;

    /**
     * Returns the size of the music queue
     *
     * @return the size of the music queue
     */
    size_t getPending() const;

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
    float getOverlap() const;

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
    void setOverlap(float time);

    /**
     * Advances ahead in the music queue.
     *
     * The value `fade` is the number of seconds to fade out the currently
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
     * @param  fade     The number of seconds to fade out the current asset
     * @param  steps    The number of elements to skip in the queue
     */
    void advance(unsigned int steps=0,float fade=DEFAULT_FADE);

    /**
     * Clears the music queue, but does not release any other resources.
     *
     * This method does not stop the current background music from playing. It
     * only clears pending music assets from the queue.
     */
    void clearPending();
};

}


#endif /* __CU_AUDIO_QUEUE_H__ */
