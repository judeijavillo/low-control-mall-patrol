//
//  LCMPCopModel.cpp
//  Low Control Mall Patrol
//
//  Author: Kevin Games
//  Version: 2/20/22
//

#include "LCMPCopModel.h"

using namespace cugl;

bool CopModel::init(const cugl::Vec2 pos, const cugl::Size size) {
    return true;
}

void CopModel::applyForce() {
    return;
}

void CopModel::update(float delta) {
    CapsuleObstacle::update(delta);
    if (_copNode != nullptr) {
        _copNode->setPosition(getPosition()*_drawscale);
        _copNode->setAngle(getAngle());
    }
}
