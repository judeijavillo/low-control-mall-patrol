//
//  LCMPCopModel.h
//  Low Control Mall Patrol
//
//  Author: Kevin Games
//  Version: 2/20/22
//

#ifndef __LCMP_COP_MODEL_H__
#define __LCMP_COP_MODEL_H__
#include "LCMPPlayerModel.h"

/** The cop's damping coefficient */
#define COP_DAMPING_DEFAULT 7.5f
/** The cop's maximum speed of this player */
#define COP_MAX_SPEED_DEFAULT 10.0f
/** The cops's acceleration of this player */
#define COP_ACCELERATION_DEFAULT 75.0f

/** The amount of time that the cop will be in the air while tackling */
#define TACKLE_AIR_TIME 0.25f
/** The movement multiplier for the cop during tackle */
#define TACKLE_MOVEMENT_MULT 2.0f
/** The damping multiplier for the cop during tackle */
#define TACKLE_DAMPING_MULT 2.5f

/** Defining the filter bits for the cop model*/
#define COP_FILTER_BITS 0b01001

class CopModel : public PlayerModel {
protected:
    std::shared_ptr<cugl::scene2::PolygonNode> _character;
    std::shared_ptr<cugl::Texture> _tackleDownTexture;
    std::shared_ptr<cugl::Texture> _tackleUpTexture;
    std::shared_ptr<cugl::Texture> _tackleRightTexture;
    std::shared_ptr<cugl::Texture> _tackleLeftTexture;
    std::shared_ptr<cugl::Texture> _landDownTexture;
    std::shared_ptr<cugl::Texture> _landUpTexture;
    std::shared_ptr<cugl::Texture> _landRightTexture;
    std::shared_ptr<cugl::Texture> _landLeftTexture;

public:
//  MARK: - Constructors
    
    /**
     * Constructs a Cop Model
     */
    CopModel() {};
    
    /**
     * Destructs a Cop Model
     */
    ~CopModel() { dispose(); }
    
    /**
     * Disposes of all resources in this instance of Player Model
     */
    void dispose();
    
    /**
     * Initializes a Cop Model
     */
    bool init(float scale,
              const std::shared_ptr<cugl::scene2::SceneNode>& node,
              const std::shared_ptr<cugl::AssetManager>& assets);
    
//  MARK: - Methods
    
    ///**
    // * Returns the damping constant
    // */
    //float getDamping() override { return COP_DAMPING; }
    //
    ///**
    // * Returns the max speed of a cop
    // */
    //float getMaxSpeed() override { return COP_MAX_SPEED; }
    //
    ///**
    // * Returns the acceleration of a cop
    // */
    //float getAcceleration() override { return COP_ACCELERATION; }
    
    /**
     * Sets cop speeds according to a missed tackle
    */
    void failedTackle(float timer, cugl::Vec2 swipe);
    
    void playAnimation(std::shared_ptr<cugl::scene2::ActionManager>& actions, cugl::Vec2 movement);
    
    void showTackle(cugl::Vec2 direction, bool inAir);

    void hideTackle();

};

#endif /* __LCMP_COP_MODEL_H__ */
