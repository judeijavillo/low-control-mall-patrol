//
//  LCMPGameModel.h
//  Low Control Mall Patrol
//
//  Author: Kevin Games
//  Version: 2/20/22
//

#ifndef __LCMP_GAME_MODEL_H__
#define __LCMP_GAME_MODEL_H__
#include <cugl/cugl.h>
#include "LCMPThiefModel.h"
#include "LCMPCopModel.h"
#include "LCMPTrapModel.h"
#include "LCMPObstacleModel.h"

class GameModel {
protected:
//  MARK: - Properties
    
    /** A reference to the thief */
    std::shared_ptr<ThiefModel> _thief;
    /** A vector of references to the cops */
    std::unordered_map<int, std::shared_ptr<CopModel>> _cops;
    /** A vector of references to the traps */
    std::vector<std::shared_ptr<TrapModel>> _traps;
    /** A vector of references to obstacles */
    std::vector<std::shared_ptr<ObstacleModel>> _obstacles;
    
public:
//  MARK: - Constructors
    
    /**
     * Constructs a Game Model
     */
    GameModel() {}
    
    /**
     * Destructs a Game Model
     */
    ~GameModel() { dispose(); }
    
    /**
     * Disposes of all resources in this instance of Game Model
     */
    void dispose();
    
    /**
     * initializes a Game Model
     */
    bool init(std::shared_ptr<ThiefModel>& thief,
              std::unordered_map<int, std::shared_ptr<CopModel>> cops,
              std::vector<std::shared_ptr<TrapModel>> traps);
    
//  MARK: - Methods
    
    /**
     * Returns a reference to the thief
     */
    std::shared_ptr<ThiefModel> getThief() { return _thief; }
    
    /**
     * Returns a reference to a given cop
     */
    std::shared_ptr<CopModel> getCop(int copID) { return _cops[copID]; }
    
    /**
     * Updates all game objects
     */
    void update(float timestep);
    
    /**
     * Applies an acceleration to the thief (most likely for local updates)
     */
    void updateThief(cugl::Vec2 acceleration);
    
    /**
     * Applies an acceleration to a cop (most likely for local updates)
     */
    void updateCop(cugl::Vec2 acceleration, int copID);
    
    /**
     * Updates the position and velocity of the thief (most likely for network updates)
     */
    void updateThief(cugl::Vec2 position, cugl::Vec2 velocity, cugl::Vec2 force);
    
    /**
     * Updates the position and velocity of a cop (most likely for network updates)
     */
    void updateCop(cugl::Vec2 position, cugl::Vec2 velocity, cugl::Vec2 force, int copID);
    
};

#endif /* __LCMP_GAME_MODEL_H__ */
