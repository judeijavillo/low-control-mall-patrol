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
#include <cugl/cugl.h>

/** The texture for the character avatar */
#define THIEF_TEXTURE    "thief"

class ThiefModel : public PlayerModel {
protected:
	/** The scene graph node for the cop. */
	std::shared_ptr<cugl::scene2::SceneNode> _thiefNode;

public:
	
	void update(float delta) override;

	void applyForce() override;

	void setMovement(Vec2 value) override;
};

#endif /* __LCMP_THIEF_MODEL_H__ */
