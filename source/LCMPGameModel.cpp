//
//  LCMPGameModel.cpp
//  Low Control Mall Patrol
//
//  Author: Kevin Games
//  Version: 2/20/22
//

#include "LCMPGameModel.h"

/**
 * Disposes of all resources in this instance of Game Model
 */
void GameModel::dispose() {
    
}

/**
 * initializes a Game Model
 */
bool GameModel::init(std::shared_ptr<ThiefModel>& thief,
                     std::unordered_map<int, std::shared_ptr<CopModel>> cops,
                     std::vector<std::shared_ptr<TrapModel>> traps) {
    _thief = thief;
    _cops = cops;
    _traps = traps;
    return true;
}

/**
 * Updates all game objects
 */
void GameModel::update(float timestep) {
    // Update the thief
    _thief->update(timestep);
    
    // Update all of the cops
    for (auto entry = _cops.begin(); entry != _cops.end(); entry++) {
        entry->second->update(timestep);
    }
}

/**
 * Applies an acceleration to the thief (most likely for local updates)
 */
void GameModel::updateThief(cugl::Vec2 acceleration) {
    _thief->applyForce(acceleration);
}

/**
 * Applies a force to a cop (most likely for local updates)
 */
void GameModel::updateCop(cugl::Vec2 acceleration, int copID) {
    _cops[copID]->applyForce(acceleration);
}

/**
 * Updates the position and velocity of the thief
 */
void GameModel::updateThief(cugl::Vec2 position, cugl::Vec2 velocity, cugl::Vec2 force) {
    _thief->applyNetwork(position, velocity, force);
}

/**
 * Updates the position and velocity of a cop
 */
void GameModel::updateCop(cugl::Vec2 position, cugl::Vec2 velocity, cugl::Vec2 force, int copID) {
    _cops[copID]->applyNetwork(position, velocity, force);
}
