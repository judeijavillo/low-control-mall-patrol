//
//  LCMPCopModel.cpp
//  Low Control Mall Patrol
//
//  Author: Kevin Games
//  Version: 2/20/22
//

#include "LCMPCopModel.h"

using namespace cugl;

void CopModel::applyForce() {
    return;
}

void CopModel::update(float delta) {
    CapsuleObstacle::update(delta);
    if (_playerNode != nullptr) {
        _playerNode->setPosition(getPosition()*_drawscale);
        _playerNode->setAngle(getAngle());
    }
}
