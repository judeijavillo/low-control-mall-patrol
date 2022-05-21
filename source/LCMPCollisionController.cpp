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
    didHitObstacle = false;
    didHitTrap = false;
    return true;
}

//  MARK: - Callbacks

/**
 * Callback for when two obstacles in the world begin colliding
 */
void CollisionController::beginContact(b2Contact* contact) {
    b2Body* body1 = contact->GetFixtureA()->GetBody();
    b2Body* body2 = contact->GetFixtureB()->GetBody();
    b2Body* thiefBody = _game->getThief()->getRealBody();
    
    if (body1 == thiefBody || body2 == thiefBody) {
        didHitObstacle = true;
    }

    // Check all of the cops
    for (int i = 0; i < _game->numberOfCops(); i++) {
        b2Body* copBody = _game->getCop(i)->getRealBody();
        
        if (copBody == body1 || copBody == body2) {
            didHitObstacle = true;
        }
        
        if ((thiefBody == body1 && copBody == body2) ||
            (thiefBody == body2 && copBody == body1)) {
            if (!_game->isGameOver()) {
                _game->getCop(i)->setCaughtThief(true);
            }
        }
    }

    // Check all of the traps
    for (int i = 0; i < _game->numberOfTraps(); i++) {
        shared_ptr<TrapModel> trap = _game->getTrap(i);
        
        b2Body* triggerBody;
        b2Body* deactivationBody;
        if (trap->hasTrigger) {
            triggerBody = trap->getTriggerArea()->getRealBody();
            deactivationBody = trap->getDeactivationArea()->getRealBody();
        }


        b2Body* thiefEffectBody = trap->getThiefEffectArea()->getRealBody();
        b2Body* copEffectBody = trap->getCopEffectArea()->getRealBody();

        if (trap->activated) {
            if ((thiefBody == body1 && thiefEffectBody == body2) ||
                (thiefBody == body2 && thiefEffectBody == body1)) {
                _game->getThief()->act(trap->getTrapID(), trap->getThiefEffect());
                didHitTrap = true;
            }

            for (int i = 0; i < _game->numberOfCops(); i++) {
                b2Body* copBody = _game->getCop(i)->getRealBody();
                if ((copBody == body1 && copEffectBody == body2) ||
                    (copBody == body2 && copEffectBody == body1)) {
                    _game->getCop(i)->act(trap->getTrapID(), trap->getCopEffect());
                    didHitTrap = true;
                }
                if (trap->hasTrigger) {
                    if ((copBody == body1 && deactivationBody == body2) ||
                        (copBody == body2 && deactivationBody == body1)) {
                        _game->getCop(i)->trapDeactivationFlag = trap->getTrapID();
                    }
                }
                
            }
        }
        else {
            if (trap->hasTrigger) {
                if ((thiefBody == body1 && triggerBody == body2) ||
                    (thiefBody == body2 && triggerBody == body1)) {
                    _game->getThief()->trapActivationFlag = trap->getTrapID();
                    _game->getThief()->trapActivationPolygons = _game->getThief()->trapActivationPolygons + 1;
                }
            }
        }
    }
    
    didHitObstacle = didHitObstacle && !didHitTrap;
}

/**
 * Callback for when two obstacles in the world end colliding
 */
void CollisionController::endContact(b2Contact* contact) {
    b2Body* body1 = contact->GetFixtureA()->GetBody();
    b2Body* body2 = contact->GetFixtureB()->GetBody();
    b2Body* thiefBody = _game->getThief()->getRealBody();

    for (int i = 0; i < _game->numberOfTraps(); i++) {
        shared_ptr<TrapModel> trap = _game->getTrap(i);
        b2Body* triggerBody;
        b2Body* deactivationBody;

        if (trap->hasTrigger) {
            triggerBody = trap->getTriggerArea()->getRealBody();
            deactivationBody = trap->getDeactivationArea()->getRealBody();
        }

        b2Body* thiefEffectBody = trap->getThiefEffectArea()->getRealBody();
        b2Body* copEffectBody = trap->getCopEffectArea()->getRealBody();

        if (trap->activated) {
            didHitTrap = false;
            if ((thiefBody == body1 && thiefEffectBody == body2) ||
                (thiefBody == body2 && thiefEffectBody == body1)) {
                _game->getThief()->unact(trap->getTrapID(), trap->getThiefEffect());
            }
            for (int i = 0; i < _game->numberOfCops(); i++) {
                b2Body* copBody = _game->getCop(i)->getRealBody();
                if ((copBody == body1 && copEffectBody == body2) ||
                    (copBody == body2 && copEffectBody == body1)) {
                    _game->getCop(i)->unact( trap->getTrapID(), trap->getCopEffect());
                }

                if (trap->hasTrigger) {
                    if ((copBody == body1 && deactivationBody == body2) ||
                        (copBody == body2 && deactivationBody == body1)) {
                        _game->getCop(i)->trapDeactivationFlag = -1;
                    }
                }
            }
        }
        else {
            if (trap->hasTrigger) {
                if ((thiefBody == body1 && triggerBody == body2) ||
                    (thiefBody == body2 && triggerBody == body1)) {
                    _game->getThief()->trapActivationPolygons = _game->getThief()->trapActivationPolygons - 1;
                    if (_game->getThief()->trapActivationPolygons <= 0) {
                        _game->getThief()->trapActivationFlag = -1;
                        _game->getThief()->trapActivationPolygons = 0;
                    }
                }
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
