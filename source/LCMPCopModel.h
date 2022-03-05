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
#define COP_DAMPING 5.0f
/** The cop's maximum speed of this player */
#define COP_MAX_SPEED 10.0f
/** The cops's acceleration of this player */
#define COP_ACCELERATION 15.0f

class CopModel : public PlayerModel {
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
     * Initializes a Cop Model
     */
    bool init(const cugl::Vec2 pos, const cugl::Size size, float scale,
              const std::shared_ptr<cugl::scene2::SceneNode>& node,
              const std::shared_ptr<cugl::AssetManager>& assets);
    
//  MARK: - Methods
    
    /**
     * Returns the damping constant
     */
    float getDamping() override { return COP_DAMPING; }
    
    /**
     * Returns the max speed of a cop
     */
    float getMaxSpeed() override { return COP_MAX_SPEED; }
    
    /**
     * Returns the acceleration of a cop
     */
    float getAcceleration() override { return COP_ACCELERATION; }
};

#endif /* __LCMP_COP_MODEL_H__ */
