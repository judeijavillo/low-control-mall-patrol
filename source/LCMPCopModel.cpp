//
//  LCMPCopModel.cpp
//  Low Control Mall Patrol
//
//  Author: Kevin Games
//  Version: 2/20/22
//

#include "LCMPCopModel.h"

#define MAX_SPEED       20000.0f
#define ACCELERATION    20.0f

using namespace cugl;

void CopModel::setCopNode(const std::shared_ptr<scene2::SceneNode>& node) {
    _copNode = node;
}

void CopModel::applyForce() {
    if (!isEnabled()) {
        return;
    }
    
    Vec4 netforce(_force.x,_force.y,0.0f,1.0f);
    CULog("x y z f: (%f, %f, %f, %f)", netforce.x, netforce.y, netforce.z, netforce.w);
    
    Mat4::createRotationZ(getAngle(),&_affine);
    netforce *= _affine;
    CULog("x y z f: (%f, %f, %f, %f)", netforce.x, netforce.y, netforce.z, netforce.w);

    
    _body->ApplyForceToCenter(b2Vec2(netforce.x,netforce.y), true);
    
    return;
}

void CopModel::update(float delta) {
    CapsuleObstacle::update(delta);
    if (_copNode != nullptr) {
        
        _copNode->setPosition(getPosition()*_drawscale);
        _copNode->setAngle(getAngle());
    }
}
