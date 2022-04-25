//
//  LCMPAudioController.h
//  Low Control Mall Patrol
//
//  Author: Kevin Games
//  Version: 3/11/22
//

#ifndef __LCMP_AUDIO_CONTROLLER_H__
#define __LCMP_AUDIO_CONTROLLER_H__

#include <cugl/cugl.h>

class AudioController {
protected:
    std::shared_ptr<cugl::AudioQueue> _queue;
    std::vector<pair<float, std::string>> _heap;
    std::unordered_map<string, std::shared_ptr<cugl::audio::AudioPlayer>> _audioPlayers;

public:

//  MARK: - Constructors
    
    /**
     * Constructs an Audio Controller
     */
    AudioController();
    
    /**
     * Destructs an Audio Controller
     */
    ~AudioController() { dispose(); }
    
    /**
     * Disposes of all resources in this instance of Audio Controller
     */
    void dispose();

//  MARK: - Methods
    
    std::shared_ptr<cugl::AudioQueue> getQueue() { return _queue; }
    
    /** Plays a sound effect */
    void playSfx(const std::shared_ptr<cugl::AssetManager>& assets, std::string key, float gameTime);
    
    /** Plays music */
    void playMusic(const std::shared_ptr<cugl::AssetManager>& assets, std::string key);
    
    /** Pauses a music track */
    void pauseMusic(string key);
    
    /** Stops a music track */
    void stopMusic(string key);
    
    /** Stops a sound effect */
    void stopSfx(string key);
    
};;

#endif /* __LCMP_AUDIO_CONTROLLER_H__ */
