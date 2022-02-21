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

//  MARK: - Drawing Constants
/** The texture for the character avatar */
#define COP_TEXTURE    "cop"
/** Identifier to allow us to track the sensor in ContactListener */
#define SENSOR_NAME     "copsensor"

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
    
};

#endif /* __LCMP_COP_MODEL_H__ */
