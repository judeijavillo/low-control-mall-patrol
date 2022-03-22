//
//  LCMPCollisionController.cpp
//  Low Control Mall Patrol
//
//  Author: Kevin Games
//  Version: 3/14/22
//

#include "LCMPCollisionController.h"

//  MARK: - Constructors

/**
 * Initializes a Collision Controller
 */
bool CollisionController::init(const std::shared_ptr<GameModel> game) {
    _game = game;
    return true;
}

//  MARK: - Callbacks

/**
 * Callback for when two obstacles in the world begin colliding
 */
void CollisionController::beginContact(b2Contact* contact) {
    b2Body* body1 = contact->GetFixtureA()->GetBody();
    b2Body* body2 = contact->GetFixtureB()->GetBody();
    b2Body* thiefBody = _game->getThief()->getBody();

    // Check all of the cops
    for (int i = 0; i < 4; i++) {
        b2Body* copBody = _game->getCop(i)->getBody();
        if ((thiefBody == body1 && copBody == body2) ||
            (thiefBody == body2 && copBody == body1)) {
            if (!_game->isGameOver()) {
                _game->setGameOver(true);
                return;
            }
        }
    }

    // Check all of the traps
    for (int i = 0; i < _game->numberOfTraps(); i++) {
        shared_ptr<TrapModel> trap = _game->getTrap(i);
        b2Body* triggerBody = trap->getTriggerArea()->getBody();
        b2Body* thiefEffectBody = trap->getThiefEffectArea()->getBody();
        b2Body* copEffectBody = trap->getCopEffectArea()->getBody();

        if (trap->activated) {
            if ((thiefBody == body1 && thiefEffectBody == body2) ||
                (thiefBody == body2 && thiefEffectBody == body1)) {
                
                // TODO: Set multipliers based on colliding trap
                
                // Slow down
                _game->getThief()->setDampingMultiplier(2.5f);

                // Speed up
                // _game->getThief()->setMaxSpeedMultiplier(2.0f);
                // _game->getThief()->setAccelerationMultiplier(2.0f);
            }

            for (int i = 0; i < 4; i++) {
                b2Body* copBody = _game->getCop(i)->getBody();
                if ((copBody == body1 && copEffectBody == body2) ||
                    (copBody == body2 && copEffectBody == body1)) {

                    // TODO: Set multipliers based on colliding trap
                    _game->getCop(i)->setDampingMultiplier(2.5);
                }
            }
        }
        else {
            if ((thiefBody == body1 && triggerBody == body2) ||
                (thiefBody == body2 && triggerBody == body1)) {
                _game->getThief()->trapActivationFlag = trap->getTrapID();
            }
        }
    }
    
}

/**
 * Callback for when two obstacles in the world end colliding
 */
void CollisionController::endContact(b2Contact* contact) {
    b2Body* body1 = contact->GetFixtureA()->GetBody();
    b2Body* body2 = contact->GetFixtureB()->GetBody();
    b2Body* thiefBody = _game->getThief()->getBody();

    for (int i = 0; i < _game->numberOfTraps(); i++) {
        shared_ptr<TrapModel> trap = _game->getTrap(i);
        b2Body* triggerBody = trap->getTriggerArea()->getBody();
        b2Body* thiefEffectBody = trap->getThiefEffectArea()->getBody();
        b2Body* copEffectBody = trap->getCopEffectArea()->getBody();

        if (trap->activated) {
            if ((thiefBody == body1 && thiefEffectBody == body2) ||
                (thiefBody == body2 && thiefEffectBody == body1)) {
                
                // TODO: Set multipliers based on colliding trap
                
                // Undo slow down
                 _game->getThief()->setDampingMultiplier(1.0f);

                // Undo speed up
                // _game->getThief()->setMaxSpeedMultiplier(1.0f);
                // _game->getThief()->setAccelerationMultiplier(1.0f);

            }
            for (int i = 0; i < 4; i++) {
                b2Body* copBody = _game->getCop(i)->getBody();
                if ((copBody == body1 && copEffectBody == body2) ||
                    (copBody == body2 && copEffectBody == body1)) {
                    
                    // TODO: Set multipliers based on colliding trap
                    _game->getCop(i)->setDampingMultiplier(1.0f);
                }
            }
        }
        else {
            if ((thiefBody == body1 && triggerBody == body2) ||
                (thiefBody == body2 && triggerBody == body1)) {
                _game->getThief()->trapActivationFlag = -1;
            }
        }
    }
}

/**
 * Callback for determining if two obstacles in the world should collide.
 */
bool CollisionController::shouldCollide(b2Fixture* f1, b2Fixture* f2) {
    const b2Filter& filterA = f1->GetFilterData();
    const b2Filter& filterB = f2->GetFilterData();

    return filterA.maskBits & filterB.maskBits;
}
