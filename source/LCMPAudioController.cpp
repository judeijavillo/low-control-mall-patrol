//
//  LCMPAudioController.cpp
//  Low Control Mall Patrol
//
//  Author: Kevin Games
//  Version: 3/11/22
//

#include "LCMPAudioController.h"
#include "LCMPConstants.h"

using namespace cugl;

//  MARK: - Constructors

/**
 * Constructs an Audio Controller
 */
AudioController::AudioController() {
    AudioEngine::start(DEFAULT_SLOTSIZE);
    _queue = AudioEngine::get()->getMusicQueue();
    make_heap(_heap.begin(), _heap.end());
}

/**
 * Destructs an Audio Controller
 */
void AudioController::dispose() {
    _queue->clear();
    _audioPlayers.clear();
    AudioEngine::stop();
}

//  MARK: - Methods

void AudioController::playSfx(const std::shared_ptr<cugl::AssetManager>& assets, std::string key, float gameTime) {
    const std::shared_ptr<Sound>& source = assets->get<Sound>(key);
    
    while (_heap.size() > 0 && (gameTime == 0 || _heap.front().first >= -gameTime)) {
//      cout << "pop " << key << " at " << gameTime << "\n";
        AudioEngine::get()->clear(_heap.front().second);
        _heap.pop_back();
    }
    
    // Play new sfx and add to heap
    if (gameTime == 0 || AudioEngine::get()->getState(key) != AudioEngine::State::PLAYING) {
        float timeRemaining = -gameTime - source->getDuration() - SFX_COOLDOWN;
        _heap.push_back(make_pair(timeRemaining, key));
        sort_heap(_heap.begin(), _heap.end());
    
        // Adjust sfx volume
        for (int i = 0; i < _heap.size(); i++) {
            AudioEngine::get()->setVolume(_heap.at(i).second,SFX_VOLUME/_heap.size());
        }
    
        AudioEngine::get()->play(key,source,true,SFX_VOLUME/_heap.size());
        AudioEngine::get()->setTimeRemaining(key, source->getDuration() + SFX_COOLDOWN);
        
//      cout << "adding " << key << " at " << gameTime << " volume: " << SFX_VOLUME / _heap.size() << "\n";
    }
}

void AudioController::playMusic(const std::shared_ptr<cugl::AssetManager>& assets, std::string key) {
    const std::shared_ptr<Sound>& source = assets->get<Sound>(key);
    
    std::shared_ptr<AudioSample> sample = AudioSample::alloc(source->getFile());
    sample->init(source->getFile(), true);
    _audioPlayers[key] = audio::AudioPlayer::alloc(sample);
    std::shared_ptr<audio::AudioResampler> resample = audio::AudioResampler::alloc(_audioPlayers[key], 48000);
    float *buffer = sample->getBuffer();
    float frames = sample->getDecoder()->pagein(buffer);
    if (frames != -1) {
        // Repeat if loading or menu themes
        if (key == LOADING_MUSIC || key == MENU_MUSIC) {
            _queue->play(source, true, MUSIC_VOLUME, DEFAULT_FADE);
        }
        else {
            _queue->play(source, false, MUSIC_VOLUME, DEFAULT_FADE);
        }
    }
    else {
        CULog("error");
    }
}

/** Stops a sound effect */
void AudioController::stopSfx(string key) {
    AudioEngine::get()->clear(key);
}

/** Pauses a music track */
void AudioController::pauseMusic(string key) {
    if (AudioEngine::get()->isActive(key)) {
        AudioEngine::get()->pause(key);
    }
}

/** Stops a music track */
void AudioController::stopMusic(string key) {
    if (_audioPlayers.find(key) != _audioPlayers.end()) {
        _audioPlayers[key]->dispose();
        _audioPlayers.erase(key);
    }
}
