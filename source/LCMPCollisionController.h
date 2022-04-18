//
//  LCMPCollisionController.h
//  Low Control Mall Patrol
//
//  Author: Kevin Games
//  Version: 3/11/22
//

#ifndef __LCMP_COLLISION_CONTROLLER_H__ 
#define __LCMP_COLLISION_CONTROLLER_H__
#include <box2d/b2_world.h>
#include <box2d/b2_contact.h>
#include <box2d/b2_collision.h>
#include "LCMPGameModel.h"

class CollisionController {
protected:
//  MARK: - Properties
    
    // Models
    /** A reference to the model to represent all models within the game */
    std::shared_ptr<GameModel> _game;
    
public:
    bool didHitObstacle;
    bool didHitTrap;
    
//  MARK: - Constructors
    
    /**
     * Constructs a Collision Controller
     */
    CollisionController() {};
    
    /**
     * Destructs a Collision Controller
     */
    ~CollisionController() { dispose(); }
    
    /**
     * Disposes of all resources in this instance of Collision Controller
     */
    void dispose() { _game = nullptr; }
    
    /**
     * Initializes a Collision Controller
     */
    bool init(const std::shared_ptr<GameModel> game);
    
    
//  MARK: - Callbacks
    
    /**
     * Callback for when two obstacles in the world begin colliding
     */
    void beginContact(b2Contact* contact);
    
    /**
     * Callback for when two obstacles in the world end colliding
     */
    void endContact(b2Contact* contact);

    /**
     * Callback for determining if two obstacles in the world should collide.
     */
    bool shouldCollide(b2Fixture* f1, b2Fixture* f2);
    
};

#endif /* __LCMP_COLLISION_CONTROLLER_H__ */
