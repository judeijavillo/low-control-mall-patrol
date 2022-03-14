//
//  LCMPAudioController.cpp
//  Low Control Mall Patrol
//
//  Author: Kevin Games
//  Version: 3/11/22
//

#include "LCMPAudioController.h"

using namespace cugl;

//  MARK: - Constructors

/**
 * Constructs an Audio Controller
 */
AudioController::AudioController() {
    AudioEngine::start(24);
}

/**
 * Destructs an Audio Controller
 */
void AudioController::dispose() {
    AudioEngine::stop();
}

//  MARK: - Methods

/**
 * Plays a sound effect
 */
void AudioController::playSound(const std::shared_ptr<cugl::AssetManager>& assets, string key, float time) {
    const std::shared_ptr<Sound>& source = assets->get<Sound>(key);
    AudioEngine::get()->play(key,source,true,source->getVolume());
    AudioEngine::get()->setTimeRemaining(key, time);
}
