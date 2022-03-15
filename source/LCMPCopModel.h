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
