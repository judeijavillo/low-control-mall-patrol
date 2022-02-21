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

//  MARK: - Physics Constants
/** The factor to multiply by the input */
#define PLAYER_FORCE      20.0f
/** The amount to slow the character down */
#define PLAYER_DAMPING    10.0f
/** The maximum character speed */
#define PLAYER_MAXSPEED   5.0f

class PlayerModel : public cugl::physics2::CapsuleObstacle {
    
protected:    
    float _maxSpeed;
    
    float _acceleration;
    
    /** The current horizontal movement of the character */
    float _movement;
    /** Which direction is the character facing */
    bool _faceRight;
    /** Whether our feet are on the ground */
    bool _isGrounded;
    
    /** Ground sensor to represent our feet */
    b2Fixture*  _sensorFixture;
    /** Reference to the sensor name (since a constant cannot have a pointer) */
    std::string _sensorName;
    /** The node for debugging the sensor */
    std::shared_ptr<cugl::scene2::WireNode> _sensorNode;
    
    /** The scene graph node for a player */
    std::shared_ptr<cugl::scene2::SceneNode> _playerNode;
    
    /** Cache object for transforming the force according the object angle */
    cugl::Mat4 _affine;
    float _drawscale;
    
public:
    
    //  MARK: - Constructors

    PlayerModel(void) : CapsuleObstacle(), _drawscale(1.0f) { }
    
    /**
     * Destroys this player, releasing all resources.
     */
    virtual ~PlayerModel(void) { dispose(); }
    
    /**
     * Disposes all resources and assets of this rocket
     *
     * Any assets owned by this object will be immediately released.
     */
    void dispose();
    
    /**
     * Initializes a new player with the given position and size.
     *
     * The player size is specified in world coordinates.
     *
     * The scene graph is completely decoupled from the physics system.
     * The node does not have to be the same size as the physics body. We
     * only guarantee that the scene graph node is positioned correctly
     * according to the drawing scale.
     *
     * @param  pos      Initial position in world coordinates
     * @param  size       The dimensions of the box.
     *
     * @return  true if the obstacle is initialized properly, false otherwise.
     */
    virtual bool init(const cugl::Vec2 pos, const cugl::Size size) override;

    //  MARK: - Static Constructors
    /**
     * Returns a newly allocated rocket with the given position and size
     *
     * The player size is specified in world coordinates.
     *
     * The scene graph is completely decoupled from the physics system.
     * The node does not have to be the same size as the physics body. We
     * only guarantee that the scene graph node is positioned correctly
     * according to the drawing scale.
     *
     * @param pos   Initial position in world coordinates
     * @param size  The dimensions of the box.
     *
     * @return a newly allocated rocket with the given position
     */
    static std::shared_ptr<PlayerModel> alloc(const cugl::Vec2& pos, const cugl::Size& size) {
        std::shared_ptr<PlayerModel> result = std::make_shared<PlayerModel>();
        return (result->init(pos,size) ? result : nullptr);
    }
    
    //  MARK: - Accessors
    /**
     * Returns left/right movement of this character.
     *
     * This is the result of input times player force.
     *
     * @return left/right movement of this character.
     */
    float getMovement() const { return _movement; }
    
    /**
     * Sets left/right movement of this character.
     *
     * This is the result of input times player force.
     *
     * @param value left/right movement of this character.
     */
    void setMovement(float value);
    
    /**
     * Returns true if the dude is on the ground.
     *
     * @return true if the dude is on the ground.
     */
    bool isGrounded() const { return _isGrounded; }
    
    /**
     * Returns ow hard the brakes are applied to get a dude to stop moving
     *
     * @return ow hard the brakes are applied to get a dude to stop moving
     */
    float getDamping() const { return PLAYER_DAMPING; }
    
    /**
     * Returns the force applied to this player.
     *
     * Remember to modify the input values by the thrust amount before assigning
     * the value to force.
     *
     * @return the force applied to this rocket.
     */
    float getForce() const { return PLAYER_FORCE; }
    
    /**
     * Returns the max speed applied to this player.
     *
     * Remember to modify the input values by the thrust amount before assigning
     * the value to force.
     *
     * @return the max speed applied to this rocket.
     */
    const float getMaxSpeed() const { return _maxSpeed; }

    /**
     * Sets the max speed applied to this player.
     *
     * Remember to modify the input values by the thrust amount before assigning
     * the value to force.
     *
     * @param value  the max speed applied to this rocket.
     */
    void setMaxSpeed(const float value) { _maxSpeed = value; }
    
    /**
     * Returns the acceleration applied to this player.
     *
     * Remember to modify the input values by the thrust amount before assigning
     * the value to force.
     *
     * @return the acceleration applied to this rocket.
     */
    const float getAcceleration() const { return _acceleration; }

    /**
     * Sets the acceleration applied to this player.
     *
     * Remember to modify the input values by the thrust amount before assigning
     * the value to force.
     *
     * @param value  the acceleration applied to this rocket.
     */
    void setAcceleration(const float value) { _acceleration = value; }

    //  MARK: - Physics
    /**
     * Applies the force to the body of this player
     *
     * This method should be called after the force attribute is set.
     */
    void applyForce();

    /**
     * Updates the object's physics state (NOT GAME LOGIC).
     *
     * This method is called AFTER the collision resolution state. Therefore, it
     * should not be used to process actions or any other gameplay information.
     * Its primary purpose is to adjust changes to the fixture, which have to
     * take place after collision.
     *
     * In other words, this is the method that updates the scene graph.  If you
     * forget to call it, it will not draw your changes.
     *
     * @param delta Timing values from parent loop
     */
    virtual void update(float delta) override;
    
};

#endif /* __LCMP_PLAYER_MODEL_H__ */
