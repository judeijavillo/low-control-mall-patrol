//
//  LCMPPlayerModel.h
//  Low Control Mall Patrol
//
//  Author: Kevin Games
//  Version: 2/20/22
//

#ifndef __LCMP_PLAYER_MODEL_H__
#define __LCMP_PLAYER_MODEL_H__
#include <cugl/cugl.h>
#include <cugl/scene2/actions/CUActionManager.h>
#include <cugl/scene2/actions/CUMoveAction.h>
#include <cugl/scene2/actions/CUScaleAction.h>
#include <cugl/scene2/actions/CUAnimateAction.h>
#include <cugl/math/CUEasingBezier.h>

/** Define the time settings for animation */
#define DURATION 3.0f
#define DISTANCE 200
#define WALKPACE 50
#define ACT_KEY  "current"
#define RIGHT_ANIM_KEY 0
#define BACK_ANIM_KEY 1
#define LEFT_ANIM_KEY 2
#define FRONT_ANIM_KEY 3

class PlayerModel : public cugl::physics2::CapsuleObstacle {
protected:
//  MARK: - Properties

    
    //Physics attributes that model movement
    float _damping;
    float _acceleration;
    float _maxspeed;



    /** A reference to the Action Manager */
    std::shared_ptr<cugl::scene2::ActionManager> _actions;

    // Views
    /** The top-level node for displaying the player */
    std::shared_ptr<cugl::scene2::SceneNode> _node;
    /** The child node for displaying the player's dropshadow */
    std::shared_ptr<cugl::scene2::PolygonNode> _dropshadow;
    /** The child nodes for displaying the player */
    std::shared_ptr<cugl::scene2::SpriteNode> _characterFront;
    std::shared_ptr<cugl::scene2::SpriteNode> _characterLeft;
    std::shared_ptr<cugl::scene2::SpriteNode> _characterRight;
    std::shared_ptr<cugl::scene2::SpriteNode> _characterBack;
    bool _rightCycle;
    bool _leftCycle;
    bool _frontCycle;
    bool _backCycle;
    /** The ratio to scale the textures. (SCENE UNITS / WORLD UNITS) */
    float _scale;
    /** The key for the collision sound */
    std::string _collisionSound;
    /** The key for the obstacle sound */
    std::string _obstacleSound;
    // MODELS
    /** The animation actions */
    std::shared_ptr<cugl::scene2::Animate> _north;
    std::shared_ptr<cugl::scene2::Animate> _south;
    std::shared_ptr<cugl::scene2::Animate> _east;
    std::shared_ptr<cugl::scene2::Animate> _west;
    /** Whether we are mid animation */
    bool _occupied;

    
public:
    /** A reference to the sprite showing the player running to the back */
    std::shared_ptr<cugl::scene2::SpriteNode> runBack;
    /** A reference to the sprite showing the player running to the front */
    std::shared_ptr<cugl::scene2::SpriteNode> runFront;
    /** A reference to the sprite showing the player running to the left */
    std::shared_ptr<cugl::scene2::SpriteNode> runLeft;
    /** A reference to the sprite showing the player running to the right */
    std::shared_ptr<cugl::scene2::SpriteNode> runRight;


//  MARK: - Constructors
    
    /**
     * Constructs a Player Model
     */
    PlayerModel() {};
    
    /**
     * Destructs a Player Model
     */
    ~PlayerModel() { dispose(); }
    
    /**
     * Disposes of all resources in this instance of Player Model
     */
    void dispose();
    
    /**
     * Initializes a Player Model
     */
    bool init(const cugl::Vec2 pos, const cugl::Size size, float scale,
              const std::shared_ptr<cugl::scene2::SceneNode>& node,
              std::shared_ptr<cugl::scene2::ActionManager>& actions);
    
//  MARK: - Methods
    
    /**
     * Gets the appropriate key for the sound for collision
     */
    const std::string& getCollisionSound() const { return _collisionSound; }

    /**
     * Sets the key for a collision sound
     */
    void setCollisionSound(const std::string& key) { _collisionSound = key; }
    
    /**
     * Gets the appropriate key for the sound for collision
     */
    const std::string& getObstacleSound() const { return _obstacleSound; }

    /**
     * Sets the key for a collision sound
     */
    void setObstacleSound(const std::string& key) { _obstacleSound = key; }
    
    /**
     * Returns the velocity of the player's body
     */
    cugl::Vec2 getVelocity() { return getLinearVelocity(); }
    
    /**
     * Returns the damping constant
     */
    virtual float getDamping() { return _damping; }

    /**
     * Sets the damping constant.
     */
    virtual void setDamping(float value) {  _damping = value; }
    
    /**
     * Returns the max speed of this player
     */
    virtual float getMaxSpeed() { return _maxspeed; }
    
    /**
     * Sets the max speed of this player.
     */
    virtual void setMaxSpeed(float value) { _maxspeed = value; }

    /**
     * Returns the acceleration of this player
     */
    virtual float getAcceleration() { return _acceleration; }

    /**
     * Sets the acceleration of this player.
     */
    virtual void setAcceleration(float value) { _acceleration = value; }

    /**
     * Returns the node of this player
     */
    std::shared_ptr<cugl::scene2::SceneNode> getNode() { return _node; }
    
    /**
     * Applies a force to the player (most likely for local updates)
     */
    void applyForce(cugl::Vec2 force);
    
    /**
     * Updates the position and velocity of the player (most likely for network updates)
     */
    void applyNetwork(cugl::Vec2 position, cugl::Vec2 velocity, cugl::Vec2 force);
    
    /**
     * Updates the player node based on this player's body
     */
    void update(float timestep) override;
    
    /**
     * Performs a film strip action
     */
    void playAnimation(cugl::Vec2 movement);
    
    int findDirection(cugl::Vec2 movement);
    
};

#endif /* __LCMP_PLAYER_MODEL_H__ */
