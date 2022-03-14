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

/** The key for collisions sounds */
#define DUDE_SOUND     "dude"
#define F_SOUND     "fuck"
#define WHY_SOUND     "why"
#define OOH_SOUND           "ooh"

class AudioController {
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

    /**
     * Plays a sound effect
     */
    void playSound(const std::shared_ptr<cugl::AssetManager>& assets, string key, float time);
    
};;

#endif /* __LCMP_AUDIO_CONTROLLER_H__ */
