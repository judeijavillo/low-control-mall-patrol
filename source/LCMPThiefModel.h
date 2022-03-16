//
//  LCMPThiefModel.h
//  Low Control Mall Patrol
//
//  Author: Kevin Games
//  Version: 2/20/22
//

#ifndef __LCMP_THIEF_MODEL_H__ 
#define __LCMP_THIEF_MODEL_H__
#include "LCMPPlayerModel.h"

/** Defining the filter bits for the thief model*/
#define THIEF_FILTER_BITS 0b10101

class ThiefModel : public PlayerModel {
public:
//  MARK: - Properties

    /** Flag for whether thief is in range to activate a trap. -1 indicates out of range, otherwise it indicates the id of the trap the thief will activate */
    int trapActivationFlag = -1;


//  MARK: - Constructors
    
    /**
     * Constructs a Thief Model
     */
    ThiefModel() {};
    
    /**
     * Destructs a Thief Model
     */
    ~ThiefModel() { dispose(); }
    
    /**
     * Initializes a Thief Model
     */
    bool init(float scale,
              const std::shared_ptr<cugl::scene2::SceneNode>& node,
              const std::shared_ptr<cugl::AssetManager>& assets,
              std::shared_ptr<cugl::scene2::ActionManager>& actions);
    
//  MARK: - Methods
    
    ///**
    // * Returns the damping constant
    // */
    //float getDamping() override { return THIEF_DAMPING; }
    //
    ///**
    // * Returns the max speed of the thief
    // */
    //float getMaxSpeed() override { return THIEF_MAX_SPEED; }
    //
    ///**
    // * Returns the acceleration of the thief
    // */
    //float getAcceleration() override { return THIEF_ACCELERATION; }
    
};

#endif /* __LCMP_THIEF_MODEL_H__ */
