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
        
    /** Reference to the physics node of the scene graph */
    std::shared_ptr<cugl::scene2::SceneNode> _worldnode;
    /** Reference to the debug node of the scene graph */
    std::shared_ptr<cugl::scene2::SceneNode> _debugnode;
    /** Reference to the Box2D world */
    std::shared_ptr<cugl::physics2::ObstacleWorld> _world;
    
    /** The width of the map in Box2D coordinates */
    float _mapWidth;
    /** The height of the map in Box2D coordinates */
    float _mapHeight;
    /** The size of a tile in Tiled coordinates */
    float _tileSize;
    
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
    bool init(std::shared_ptr<cugl::physics2::ObstacleWorld>& world,
              std::shared_ptr<cugl::scene2::SceneNode>& worldnode,
              std::shared_ptr<cugl::scene2::SceneNode>& debugnode,
              const std::shared_ptr<cugl::AssetManager>& asssets,
              float scale, const std::string& file);
    
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
     * Returns the number of cops in the game.
    */
    int numberOfCops() { return (int)_cops.size(); }
    
    /**
     * Returns a reference to a given trap
     */
    std::shared_ptr<TrapModel> getTrap(int trapID) { return _traps[trapID]; }
    
    /**
     * Returns the number of traps in the game
     */
    int numberOfTraps() { return (int) _traps.size(); }
    
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
    void updateCop(cugl::Vec2 acceleration, int copID, bool onTackleCooldown);
    
    /**
     * Updates the position and velocity of the thief (most likely for network updates)
     */
    void updateThief(cugl::Vec2 position, cugl::Vec2 velocity, cugl::Vec2 force);
    
    /**
     * Updates the position and velocity of a cop (most likely for network updates)
     */
    void updateCop(cugl::Vec2 position, cugl::Vec2 velocity, cugl::Vec2 force, int copID);
    
    /**
     * Activates a trap
     */
    void activateTrap(int trapID);
    
private:
//  MARK: - Helpers
    
    /**
     * Initializes a thief
     */
    void initThief(float scale,
                   const std::shared_ptr<cugl::JsonValue>& spawn,
                   const std::shared_ptr<cugl::AssetManager>& assets);
    
    /**
     * Initializes a single cop
     */
    void initCop(int copID, float scale,
                 const std::shared_ptr<cugl::JsonValue>& spawns,
                 const std::shared_ptr<cugl::AssetManager>& assets);
    
    /**
     * Initializes a single wall
     */
    void initWall(const std::shared_ptr<cugl::JsonValue>& json, float scale);
    
    /**
     * Initializes a single trap
     */
    void initTrap(int trapID, cugl::Vec2 center, float scale,
                  const std::shared_ptr<cugl::AssetManager>& assets);
    
    /**
     * Initializes the border for the game
     */
    void initBorder(float scale);
    
};

#endif /* __LCMP_GAME_MODEL_H__ */
