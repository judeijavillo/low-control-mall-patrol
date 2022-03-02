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
#include <vector>
#include <cugl/cugl.h>
#include <cugl/physics2/CUObstacleWorld.h>
#include <cugl/assets/CUAsset.h>
#include <cugl/io/CUJsonReader.h>

using namespace cugl;

class GameModel : public Asset {
protected:
//  MARK: - Properties
    
    //Physics:
    /** A reference to the thief */
    std::shared_ptr<ThiefModel> _thief;
    /** A vector of references to the cops */
    std::vector<std::shared_ptr<CopModel>> _cops;
    /** A vector of references to the traps */
    std::vector<std::shared_ptr<TrapModel>> _traps;
    /** The physics word; part of the model (though listeners elsewhere) */
    std::shared_ptr<physics2::ObstacleWorld> _world;


    /** The root node of this level */
    std::shared_ptr<scene2::SceneNode> _root;
    /** The global gravity for this level */
    Vec2 _gravity;
    /** The bounds of this level in physics coordinates */
    Rect _bounds;
    /** The level drawing scale (difference between physics and drawing coordinates) */
    Vec2 _scale;

    /** Reference to the physics root of the scene graph */
    std::shared_ptr<scene2::SceneNode> _worldnode;
    /** Reference to the debug root of the scene graph */
    std::shared_ptr<scene2::SceneNode> _debugnode;


    /** Reference to the obstacles */
    std::vector<std::shared_ptr<physics2::PolygonObstacle>> _obstacles;
    /** Reference to the walls */
    std::vector<std::shared_ptr<physics2::BoxObstacle>> _walls;

    /** The AssetManager for the game mode */
    std::shared_ptr<cugl::AssetManager> _assets;

    /**
     * Loads an wall from the JSON
     *
     * These are only walls.
     *
     * @param  json   a JSON Value with the json for related objects.
     *
     * @return true if the objects were loaded successfully.
     */
    bool loadWalls(const std::vector<int> walls, int width, int height, int t_width, int t_height);
    
    /**
     * Loads an object from the JSON
     *
     * These are only obstacles.
     *
     * @param  json   a JSON Value with the json for related objects.
     *
     * @return true if the objects were loaded successfully.
     */
    bool loadObstacle(const std::shared_ptr<JsonValue>& json);

    /**
     * Converts the string to a color
     *
     * Right now we only support the following colors: yellow, red, blur, green,
     * black, and grey.
     *
     * @param  name the color name
     *
     * @return the color for the string
     */
    Color4 parseColor(std::string name);

    /**
     * Clears the root scene graph node for this level
     */
    void clearRootNode();

public:
#pragma mark Static Constructors
    /**
     * Creates a new game level with no source file.
     *
     * The source file can be set at any time via the setFile() method. This method
     * does NOT load the asset.  You must call the load() method to do that.
     *
     * @return  an autoreleased level file
     */
    static std::shared_ptr<GameModel> alloc() {
        std::shared_ptr<GameModel> result = std::make_shared<GameModel>();
        return (result->init("") ? result : nullptr);
    }

    /**
     * Creates a new game level with the given source file.
     *
     * This method does NOT load the level. You must call the load() method to do that.
     * This method returns false if file does not exist.
     *
     * @return  an autoreleased level file
     */
    static std::shared_ptr<GameModel> alloc(std::string file) {
        std::shared_ptr<GameModel> result = std::make_shared<GameModel>();
        return (result->init(file) ? result : nullptr);
    }
    
    /**
     * Adds the physics object to the physics world and loosely couples it to the scene graph
     *
     *
     * param obj    The physics object to add
     * param node   The scene graph node to attach it to
     */
    void addObstacle(const std::shared_ptr<cugl::physics2::Obstacle>& obj,
                     const std::shared_ptr<cugl::scene2::SceneNode>& node);

    /**
     * Returns the cops in this game level
     *
     * @return the cops in this game level
     */
    const std::vector<std::shared_ptr<CopModel>>& getCop() { return _cops; }

    /**
     * Returns the thief in this game level
     *
     * @return the thief in this game level
     */
    const std::shared_ptr<ThiefModel>& getThief() { return _thief; }

    /**
     * Returns the Obstacle world in this game level 
     *
     * @return the obstacle world in this game level
     */
    const std::shared_ptr<physics2::ObstacleWorld>& getWorld() { return _world; }

    /** 
     * Returns the bounds of this level in physics coordinates
     *
     * @return the bounds of this level in physics coordinates
     */
    const Rect& getBounds() const   { return _bounds; }
    
     /**
     * Returns the global gravity for this level
     *
     * @return the global gravity for this level
     */
    const Vec2& getGravity() const { return _gravity; }

    /**
     * Returns the drawing scale for this game level
     *
     * The drawing scale is the number of pixels to draw before Box2D unit. Because
     * mass is a function of area in Box2D, we typically want the physics objects
     * to be small.  So we decouple that scale from the physics object.  However,
     * we must track the scale difference to communicate with the scene graph.
     *
     * We allow for the scaling factor to be non-uniform.
     *
     * @return the drawing scale for this game level
     */
    const Vec2& getDrawScale() const { return _scale; }

    /**
     * Sets the drawing scale for this game level
     *
     * The drawing scale is the number of pixels to draw before Box2D unit. Because
     * mass is a function of area in Box2D, we typically want the physics objects
     * to be small.  So we decouple that scale from the physics object.  However,
     * we must track the scale difference to communicate with the scene graph.
     *
     * We allow for the scaling factor to be non-uniform.
     *
     * @param value  the drawing scale for this game level
     */
    void setDrawScale(float value);

    /**
     * Returns the scene graph node for drawing purposes.
     *
     * The scene graph is completely decoupled from the physics system.  The node
     * does not have to be the same size as the physics body. We only guarantee
     * that the node is positioned correctly according to the drawing scale.
     *
     * @return the scene graph node for drawing purposes.
     */
    const std::shared_ptr<scene2::SceneNode>& getRootNode() const { return _root; }

    /**
     * Sets the scene graph node for drawing purposes.
     *
     * The scene graph is completely decoupled from the physics system.  The node
     * does not have to be the same size as the physics body. We only guarantee
     * that the node is positioned correctly according to the drawing scale.
     *
     * @param value  the scene graph node for drawing purposes.
     *
     * @retain  a reference to this scene graph node
     * @release the previous scene graph node used by this object
     */
    void setRootNode(const std::shared_ptr<scene2::SceneNode>& root);

    /**
     * Sets the loaded assets for this game level
     *
     * @param assets the loaded assets for this game level
     */
    void setAssets(const std::shared_ptr<AssetManager>& assets) { _assets = assets;  }

    /**
     * Toggles whether to show the debug layer of this game world.
     *
     * The debug layer displays wireframe outlines of the physics fixtures.
     *
     * @param  flag whether to show the debug layer of this game world
     */
    void showDebug(bool flag);
    
    /**
     * Loads this game level from the source file
     *
     * This load method should NEVER access the AssetManager.  Assets are loaded in
     * parallel, not in sequence.  If an asset (like a game level) has references to
     * other assets, then these should be connected later, during scene initialization.
     *
     * @param file the name of the source file to load from
     *
     * @return true if successfully loaded the asset from a file
     */
    virtual bool preload(const std::string& file) override;


    /**
     * Loads this game level from a JsonValue containing all data from a source Json file.
     *
     * This load method should NEVER access the AssetManager.  Assets are loaded in
     * parallel, not in sequence.  If an asset (like a game level) has references to
     * other assets, then these should be connected later, during scene initialization.
     *
     * @param json the json loaded from the source file to use when loading this game level
     *
     * @return true if successfully loaded the asset from the input JsonValue
     */
    virtual bool preload(const std::shared_ptr<cugl::JsonValue>& json) override;

    /**
     * Unloads this game level, releasing all sources
     *
     * This load method should NEVER access the AssetManager.  Assets are loaded and
     * unloaded in parallel, not in sequence.  If an asset (like a game level) has
     * references to other assets, then these should be disconnected earlier.
     */
    void unload();
    /**
     * Creates a new, empty level.
     */
    GameModel(void);

    /**
     * Destroys this level, releasing all resources.
     */
    virtual ~GameModel(void);

};

#endif /* __LCMP_GAME_MODEL_H__ */
