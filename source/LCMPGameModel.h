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
#include <map>

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
    /** A vector of references to the nodes holding the map textures */
    std::vector<std::shared_ptr<cugl::scene2::PolygonNode>> _mapChunks;
    /** Reference to the Action Manager */
    std::shared_ptr<cugl::scene2::ActionManager> _actions;
    /** Reference to the background node of the scene graph */
    std::shared_ptr<cugl::scene2::SceneNode> _floornode;
    /** Reference to the physics node of the scene graph */
    std::shared_ptr<cugl::scene2::SceneNode> _worldnode;
    /** Reference to the debug node of the scene graph */
    std::shared_ptr<cugl::scene2::SceneNode> _debugnode;
    /** Reference to the Box2D world */
    std::shared_ptr<cugl::physics2::ObstacleWorld> _world;
    // Map to associate the json strings with the json enum values
    std::map<std::string, JsonConstants> constantsMap;
    
    /** The width of the map in Box2D coordinates */
    float _mapWidth;
    /** The height of the map in Box2D coordinates */
    float _mapHeight;
    /** The size of a tile in Tiled coordinates */
    float _tileSize;
    /** A flag indicating whether the game is over */
    bool _gameover;

    /** The chosen customization skin  */
    string _skinKey;
    
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
              std::shared_ptr<cugl::scene2::SceneNode>& floornode,
              std::shared_ptr<cugl::scene2::SceneNode>& worldnode,
              std::shared_ptr<cugl::scene2::SceneNode>& debugnode,
              const std::shared_ptr<cugl::AssetManager>& assets,
              float scale, const std::string& file,
              std::shared_ptr<cugl::scene2::ActionManager>& actions,
              string skinKey);
    
//  MARK: - Methods
    
    /**
     * Returns true iff the game is over (cops won)
     */
    bool isGameOver() { return _gameover; }
    
    /**
     * Sets a flag indicating game over
     */
    void setGameOver(bool value) { _gameover = value; }
    
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
    void updateCop(cugl::Vec2 acceleration, cugl::Vec2 thiefPosition, int copID, float timestep);

    /**
     * Updates the position and velocity of the thief (most likely for network updates)
     */
    void updateThief(cugl::Vec2 position, cugl::Vec2 velocity, cugl::Vec2 force);
    
    /**
     * Updates the position and velocity of a cop (most likely for network updates)
     */
    void updateCop(cugl::Vec2 position, cugl::Vec2 velocity,
                   cugl::Vec2 force, cugl::Vec2 tackleDirection,
                   cugl::Vec2 tacklePosition,
                   float tackleTime,
                   bool tackling,
                   bool caughtThief,
                   bool tackleSuccessful,
                   int copID);
    
    /**
     * Activates a trap
     */
    void activateTrap(int trapID);

    /**
    * deactivates a trap
    */
    void deactivateTrap(int trapID);
    
private:
//  MARK: - Helpers
    
    /**
     * Initializes the background of the map
     */
    void initBackdrop(float scale,
                      int rows,
                      int cols,
                      const std::shared_ptr<cugl::AssetManager>& assets);
    
    /**
     * Initializes a thief
     */
    void initThief(float scale,
                   const std::shared_ptr<cugl::JsonValue>& spawn,
                   const std::shared_ptr<cugl::AssetManager>& assets,
                   std::shared_ptr<cugl::scene2::ActionManager>& actions);
    
    /**
     * Initializes a single cop
     */
    void initCop(int copID, float scale,
                 const std::shared_ptr<cugl::JsonValue>& spawns,
                 const std::shared_ptr<cugl::AssetManager>& assets,
                 std::shared_ptr<cugl::scene2::ActionManager>& actions);
    
    struct TileData{
        string assetName;
        std::shared_ptr<cugl::JsonValue> hitboxes;
        bool animated = false;
        int anim_rows = 0;
        int anim_cols = 0;
    };
    
    map<int,GameModel::TileData> buildTileDataMap(const shared_ptr<cugl::JsonValue>& propTileset);
    
    /**
     Places all props into the world
     */
    void initProps(const std::shared_ptr<cugl::JsonValue>& props,
                   int props_firstgid,
                   map<int,GameModel::TileData> idToTileData,
                   const std::shared_ptr<cugl::AssetManager>& assets,
                   float scale);
    
    /**
     Places a single prop into the world
     */
    void initProp(const std::shared_ptr<cugl::JsonValue>& prop,
                  int props_firstGid,
                  const std::shared_ptr<cugl::JsonValue>& propTileset,
                  const std::shared_ptr<cugl::AssetManager>& assets,
                  float scale);
    
    struct ObstacleNode_x_Y_struct{
        std::shared_ptr<cugl::physics2::PolygonObstacle> obstacle;
        std::shared_ptr<cugl::scene2::PolygonNode> node;
        float x, y;
    };
    
    GameModel::ObstacleNode_x_Y_struct readJsonShape(const std::shared_ptr<cugl::JsonValue>& json, float scale);
    
    /**
     * Initializes a single wall
     */
    void initWall(const std::shared_ptr<cugl::JsonValue>& json, float scale);
    
    /**
     * Initializes a single trap
     */
    void initTrap(int trapID,
                  const std::shared_ptr<cugl::JsonValue>& json,
                  const map<int, ObstacleNode_x_Y_struct>& map1,
                  const map<int, ObstacleNode_x_Y_struct>& map2,
                  float scale,
                  const std::shared_ptr<cugl::AssetManager>& assets);


    /**
    * Takes a JsonValue pointing towards an Effect object and parses it
    */
    shared_ptr<TrapModel::Effect> readJsonEffect(shared_ptr<cugl::JsonValue> effect);
    
    /**
     * Initializes the border for the game
     */
    void initBorder(float scale);
    
};

#endif /* __LCMP_GAME_MODEL_H__ */
