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

/** The thief's damping coefficient */
#define THIEF_DAMPING 100.0f
/** The thief's maximum speed of this player */
#define THIEF_MAX_SPEED 10.0f
/** The thief's acceleration of this player */
#define THIEF_ACCELERATION 100.0f

class ThiefModel : public PlayerModel {
public:
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
    bool init(const cugl::Vec2 pos, const cugl::Size size, float scale,
              const std::shared_ptr<cugl::scene2::SceneNode>& node,
              const std::shared_ptr<cugl::AssetManager>& assets);
    
//  MARK: - Methods
    
    /**
     * Returns the damping constant
     */
    float getDamping() override { return THIEF_DAMPING; }
    
    /**
     * Returns the max speed of the thief
     */
    float getMaxSpeed() override { return THIEF_MAX_SPEED; }
    
    /**
     * Returns the acceleration of the thief
     */
    float getAcceleration() override { return THIEF_ACCELERATION; }
    
};

#endif /* __LCMP_THIEF_MODEL_H__ */
