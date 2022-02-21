//
//  CUAudioEngine.h
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
#ifndef __CU_AUDIO_ENGINE_H__
#define __CU_AUDIO_ENGINE_H__
#include <cugl/audio/CUAudioDevices.h>
#include <cugl/audio/CUSound.h>
#include <cugl/util/CUTimestamp.h>
#include <unordered_map>
#include <functional>
#include <vector>
#include <deque>
#include <queue>

/** The default fade setting for stopping and pausing */
#define DEFAULT_FADE     0.015

/** The default number of slots */
#define DEFAULT_SLOTSIZE    16

namespace cugl {
    /**
     * The audio graph classes.
     *
     * This internal namespace is for the audio graph clases.  It was chosen
     * to distinguish this graph from other graph class collections, such as the
     * scene graph collections in {@link scene2}.
     */
    namespace audio {
	    /** The mixer graph classes assigned to this class */
        class AudioNode;
        class AudioScheduler;
        class AudioMixer;
        class AudioFader;
        class AudioPanner;
    }

    /** AudioQueue for music support */
    class AudioQueue;

/**
 * Class provides a singleton audio engine
 *
 * This module is an attempt to combine the power of a modern DSP mixer graph
 * with a simple 2000-era interface. Like the legacy engines, it provides a
 * a flat slot-based structure for playing sounds, and controlling the fade
 * and pan of each slot. However, you are not limited to playing samples in
 * the slots.  You can also add arbitrary audio nodes as well.
 *
 * This class is primarily designed for the playing of sound effects.  These
 * are short sound effects that are often happening in parallel.  The engine
 * has a fixed number of slots for these sounds (historically 24) and it can
 * only play as many sounds simultaneously as it has slots. Slots are assigned
 * automatically by the engine.  However, when you play an effect, you must
 * assign it a unique key so that you can access it later (for volume changes,
 * panning, early termination, etc.).  This key eliminates any need for tracking
 * the slot assigned to an effect.
 *
 * Music is treated separately because seamless playback requires the ability
 * to queue up audio assets in order. As a result, this is supported through
 * the {@link AudioQueue} interface.  However, queues are owned by and acquired
 * from this engine. There is always one music queue available, though you
 * do have the ability to acquire more.
 *
 * You cannot create new instances of this class.  Instead, you should access
 * the singleton through the three static methods: {@link #start()}, {@link #stop()},
 * and {@link #get()}. Calling these methods will initialize the {@link AudioDevices}
 * singleton, if it is not already initialized.
 *
 * IMPORTANT: Like the OpenGL context, this class is not thread-safe.  It is
 * only safe to access this class in the main application thread.  This means
 * it should never be called in a call-back function as those are typically
 * executed in the host thread.  If you need to access the AudioEngine in a
 * callback function, you should use the {@link Application#schedule} method
 * to delay until the main thread is next available.
 */
class AudioEngine {
#pragma mark Sound State
public:
    /**
     * This enumeration provides a way to determine the state of a slot
     */
    enum class State {
        /** This sound channel is not actually active */
        INACTIVE,
        /** This sound is active and currently playing */
        PLAYING,
        /** This sound is active but is currently paused */
        PAUSED
    };

private:
    /** Reference to the audio engine singleton */
    static AudioEngine* _gEngine;

    /** Whether this method has ownership of the AudioDevices */
    bool _primary;
    
    // The mixer graph
    /** The number of supported audio slots */
    size_t _capacity;
    /** The audio graph output device */
    std::shared_ptr<audio::AudioOutput> _output;
    /** The audio graph mixer (which determines the number of channels) */
    std::shared_ptr<audio::AudioMixer>  _mixer;
    /** The channel wrappers for fading (pausing/stopping) slots */
    std::vector<std::shared_ptr<audio::AudioFader>>     _covers;
    /** The slot objects for sheduling sounds */
    std::vector<std::shared_ptr<audio::AudioScheduler>> _slots;

    /** Active music queues */
    std::vector<std::shared_ptr<AudioQueue>> _queues;
    
    /** Map keys to identifiers */
    std::unordered_map<std::string,std::shared_ptr<audio::AudioFader>> _actives;
    /** A queue for slot eviction if necessary */
    std::deque<std::string> _evicts;

    /** An object pool of faders for individual sound instances */
    std::deque<std::shared_ptr<audio::AudioFader>>  _fadePool;
    /** An object pool of panners for panning sound assets */
    std::deque<std::shared_ptr<audio::AudioPanner>> _panPool;

    /**
     * Callback function for the sound effects
     *
     * This function is called whenever a sound effect completes. It is called
     * whether or not the sound completed normally or if it was terminated
     * manually.  However, the second parameter can be used to distinguish the
     * two cases.
     *
     * @param key       The key identifying this sound effect
     * @param status    True if the music terminated normally, false otherwise.
     */
    std::function<void(const std::string key , bool status)> _callback;

#pragma mark -
#pragma mark Constructors (Private)
    /**
     * Creates, but does not initialize the singleton audio engine
     *
     * The engine must be initialized before is can be used.
     */
    AudioEngine();

    /**
     * Disposes of the singleton audio engine.
     *
     * This destructor releases all of the resources associated with this
     * audio engine.
     */
    ~AudioEngine() { dispose(); }

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
    bool init(const std::shared_ptr<audio::AudioOutput>& device, Uint32 slots);

    /**
     * Releases all resources for this singleton audio engine.
     *
     * Sound effects may no longer be added, nor may queues be used or reallocated.
     * If you need to use the engine again, you must call init().
     */
    void dispose();


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
    void removeKey(const std::string key);

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
    void gcollect(const std::shared_ptr<audio::AudioNode>& sound, bool status);

#pragma mark -
#pragma mark Static Accessors
public:
    /**
     * Returns the singleton instance of the audio engine.
     *
     * If the audio engine has not been started, then this method will return
     * nullptr.
     *
     * @return the singleton instance of the audio engine.
     */
    static AudioEngine* get() { return _gEngine; }

    /**
     * Starts the singleton audio engine on the default audio device.
     *
     * Once this method is called, the method get() will no longer return
     * nullptr.  Calling the method multiple times (without calling stop) will
     * have no effect.
     *
     * This convenience method will start up the {@link AudioDevices} manager,
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
    static bool start(Uint32 slots=DEFAULT_SLOTSIZE);

    /**
     * Starts the singleton audio engine on the given audio device.
     *
     * Once this method is called, the method get() will no longer return
     * nullptr.  Calling the method multiple times (without calling stop) will
     * have no effect.
     *
     * This version of the method assumes that the programmer has already
     * started the {@link AudioDevices} manager. It will not restart the
     * manager, nor will it shutdown the audio manager when done.  This
     * version of the initializer is only for programmers that need
     * lower-level control over buffer size and sampling rate.
     *
     * The parameter `slots` indicates the number of simultaneously supported
     * sounds.  Attempting to play more than this number of sounds may fail,
     * it may eject a previously playing sound, depending on the settings.
     * The default number of slots is 16.
     *
     * @param device The audio device to use for this engine
     * @param slots  The maximum number of sound slots to support
     *
     * @return true if the engine was successfully initialized
     */
    static bool start(const std::shared_ptr<audio::AudioOutput>& device,
                      Uint32 slots=DEFAULT_SLOTSIZE);

    /**
     * Shutsdown the singleton audio engine, releasing all resources.
     *
     * Once this method is called, the method get() will return nullptr.
     * Calling the method multiple times (without calling stop) will have
     * no effect.
     *
     * If the engine was started with the convenience method {@link #start(Uint32)},
     * then this method will also stop the {@link AudioDevices} manager.
     * Otherwise, it is the responsibility of the programmer to shutdown
     * the device manager.
     */
    static void stop();


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
    std::shared_ptr<AudioQueue> getMusicQueue() const;

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
     * use {@link audio::AudioMixer}.
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
    std::shared_ptr<AudioQueue> allocQueue();
    
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
     * @param queue	The audio queue to free
     */
    void freeQueue(const std::shared_ptr<AudioQueue>& queue);


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
    bool play(const std::string key, const std::shared_ptr<Sound>& sound,
              bool loop=false, float volume=1.0f, bool force=false);

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
    bool play(const std::string key, const std::shared_ptr<audio::AudioNode>& graph,
              bool loop=false, float volume=1.0f, bool force=false);
    
    /**
     * Returns the number of slots available for sound effects.
     *
     * There are a limited number of slots available for sound effects.  If
     * all slots are in use, this method will return 0. If you go over the
     * number available, you cannot play another sound unless you force it.
     * In that case, it will grab the slot from the longest playing sound
     * effect.
     *
     * @return the number of slots available for sound effects.
     */
    size_t getAvailableSlots() const {
        return _capacity-_actives.size();
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
    State getState(const std::string key) const;

    /**
     * Returns true if the key is associated with an active sound.
     *
     * @param  key  the reference key for the sound effect
     *
     * @return true if the key is associated with an active sound.
     */
    bool isActive(const std::string key) const {
        return _actives.find(key) != _actives.end();
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
    const std::string getSource(const std::string key) const;

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
    bool isLoop(const std::string key) const;

    /**
     * Sets whether the sound effect is in a continuous loop.
     *
     * If the key does not correspond to an active sound effect, this
     * method does nothing.
     *
     * @param  key  the reference key for the sound effect
     * @param  loop whether the sound effect is in a continuous loop
     */
    void setLoop(const std::string key, bool loop);

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
    float getVolume(const std::string key) const;

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
    void setVolume(const std::string key, float volume);

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
    float getPanFactor(const std::string& key) const;

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
    void setPanFactor(const std::string key, float pan);

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
    float getDuration(const std::string key) const;

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
    float getTimeElapsed(const std::string key) const;

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
    void setTimeElapsed(const std::string key, float time);

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
    float geTimeRemaining(const std::string key) const;

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
    void setTimeRemaining(const std::string key, float time);

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
    void clear(const std::string key,float fade=DEFAULT_FADE);

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
    void pause(const std::string key,float fade=DEFAULT_FADE);

    /**
     * Resumes the sound effect for the given key.
     *
     * If the key does not correspond to a channel, this method does nothing.
     *
     * @param  key  the reference key for the sound effect
     */
    void resume(std::string key);

    /**
     * Sets the callback for sound effects
     *
     * This callback function is called whenever a sound effect completes. It
     * is called whether or not the sound completed normally or if it was
     * terminated manually.  However, the second parameter can be used to
     * distinguish the two cases.
     *
     * @param callback  The callback for sound effects
     */
    void setListener(std::function<void(const std::string key,bool)> callback) {
        _callback = callback;
    }

    /**
     * Returns the callback for sound effects
     *
     * This callback function is called whenever a sound effect completes. It
     * is called whether or not the sound completed normally or if it was
     * terminated manually.  However, the second parameter can be used to
     * distinguish the two cases.
     *
     * @return the callback for sound effects
     */
    std::function<void(const std::string key,bool)> getListener() const {
        return _callback;
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
    void clearEffects(float fade=DEFAULT_FADE);

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
    void pauseEffects(float fade=DEFAULT_FADE);

    /**
     * Resumes all paused sound effects.
     *
     * This method has no effect on the music queues.
     */
    void resumeEffects();
    
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
    void clear(float fade=DEFAULT_FADE);

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
    void pause(float fade=DEFAULT_FADE);

    /**
     * Resumes all paused sounds, both music and sound effects.
     *
     * You should generally call this method right after the app returns
     * from the background.
     */
    void resume();
};

}


#endif /* __CU_AUDIO_CHANNELS_H__ */

