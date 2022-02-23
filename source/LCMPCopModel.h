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
#include <cugl/cugl.h>

//  MARK: - Drawing Constants
/** Identifier to allow us to track the sensor in ContactListener */

//  MARK: - Cop Model
class CopModel : public PlayerModel {
protected:
    /** How long until we can tackle again */
    int  _tackleCooldown;
    /** Whether we are actively tackling */
    bool _isTackling;
    /** The scene graph node for the cop. */
    std::shared_ptr<cugl::scene2::SceneNode> _copNode;

public:
//  MARK: - Constructors
    
    virtual bool init(const cugl::Vec2 pos, const cugl::Size size);
    
    virtual void update(float delta);
        
    virtual void applyForce();
};

#endif /* __LCMP_COP_MODEL_H__ */
