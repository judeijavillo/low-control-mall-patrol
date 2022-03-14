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

class PlayerModel : public cugl::physics2::CapsuleObstacle {
protected:
//  MARK: - Properties
    
    // Views
    /** The top-level node for displaying the player */
    std::shared_ptr<cugl::scene2::SceneNode> _node;
    /** The child node for displaying the player's dropshadow */
    std::shared_ptr<cugl::scene2::PolygonNode> _dropshadow;
    /** The child node for displaying the player */
    std::shared_ptr<cugl::scene2::PolygonNode> _character;
    
    // Attributes
    /** A reference to the texture showing the player running to the back */
    std::shared_ptr<cugl::Texture> _runBackTexture;
    /** A reference to the texture showing the player running to the front */
    std::shared_ptr<cugl::Texture> _runFrontTexture;
    /** A reference to the texture showing the player running to the left */
    std::shared_ptr<cugl::Texture> _runLeftTexture;
    /** A reference to the texture showing the player running to the right */
    std::shared_ptr<cugl::Texture> _runRightTexture;
    /** The ratio to scale the textures. (SCENE UNITS / WORLD UNITS) */
    float _scale;
    /** The key for the collision sound */
    std::string _collisionSound;
    
public:
//  MARK: - Constructors
    
    /**
     * Constructs a Player Model
     */
    PlayerModel() {}
    
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
              const std::shared_ptr<cugl::scene2::SceneNode>& node);
    
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
     * Returns the velocity of the player's body
     */
    cugl::Vec2 getVelocity() { return getLinearVelocity(); }
    
    /**
     * Returns the damping constant
     */
    virtual float getDamping() { return 10.0f; }
    
    /**
     * Returns the max speed of this player
     */
    virtual float getMaxSpeed() { return 10.0f; }
    
    /**
     * Returns the acceleration of this player
     */
    virtual float getAcceleration() { return 10.0f; }

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
    
};

#endif /* __LCMP_PLAYER_MODEL_H__ */
