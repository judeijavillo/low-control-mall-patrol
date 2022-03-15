//
//  LCMPCopModel.cpp
//  Low Control Mall Patrol
//
//  Author: Kevin Games
//  Version: 2/20/22
//

#include "LCMPCopModel.h"
#include "LCMPConstants.h"

using namespace cugl;
using namespace std;

//  MARK: - Constants

/** The width of a cop in world units */
#define COP_WIDTH       2.0f
/** The height of a cop body (its dropshadow) in world units */
#define COP_HEIGHT      1.0f

/** Keys for cop run textures */
#define COP_RUN_BACK        "ss_cop_up"
#define COP_RUN_FRONT       "ss_cop_down"
#define COP_RUN_LEFT        "ss_cop_left"
#define COP_RUN_RIGHT       "ss_cop_right"
/** Keys for cop tackle textures */
#define COP_JUMP_UP         "cop_jump_up"
#define COP_JUMP_DOWN       "cop_jump_down"
#define COP_JUMP_LEFT        "cop_jump_left"
#define COP_JUMP_RIGHT       "cop_jump_right"
#define COP_LAND_UP         "cop_land_up"
#define COP_LAND_DOWN       "cop_land_down"
#define COP_LAND_LEFT        "cop_land_left"
#define COP_LAND_RIGHT       "cop_land_right"

//  MARK: - Constructors

/**
 * Initializes a Cop Model
 *
 * @param scale screenunit/worldunit
 */
bool CopModel::init(float scale,
                      const std::shared_ptr<cugl::scene2::SceneNode>& node,
                      const std::shared_ptr<cugl::AssetManager>& assets) {
    // The cop has constant size
    Size size(COP_WIDTH, COP_HEIGHT);
    
    // Call the parent's initializer
    PlayerModel::init(Vec2::ZERO, size, scale, node);
    
    // Set movement attributes to their default values.
    setAcceleration(COP_ACCELERATION_DEFAULT);
    setDamping(COP_DAMPING_DEFAULT);
    setMaxSpeed(COP_MAX_SPEED_DEFAULT);

    // Up character movement
      std::vector<int> north;
      for(int ii = 0; ii < 8; ii++) {
          north.push_back(ii);
      }
      _north = scene2::Animate::alloc(north,DURATION);

      // Down character movement
      std::vector<int> south;
      for(int ii = 1; ii < 8; ii++) {
          south.push_back(ii);
      }
      _south = scene2::Animate::alloc(south,DURATION);
    
    // Set up the textures for all tackle directions
    _tackleDownTexture = assets->get<Texture>(COP_JUMP_DOWN);
    _tackleUpTexture = assets->get<Texture>(COP_JUMP_UP);
    _tackleLeftTexture = assets->get<Texture>(COP_JUMP_LEFT);
    _tackleRightTexture = assets->get<Texture>(COP_JUMP_RIGHT);
    _landDownTexture = assets->get<Texture>(COP_LAND_DOWN);
    _landUpTexture = assets->get<Texture>(COP_LAND_UP);
    _landLeftTexture = assets->get<Texture>(COP_LAND_LEFT);
    _landRightTexture = assets->get<Texture>(COP_LAND_RIGHT);
    
    runFront = scene2::SpriteNode::alloc(assets->get<Texture>(COP_RUN_FRONT), 1, 8);
    runBack = scene2::SpriteNode::alloc(assets->get<Texture>(COP_RUN_BACK), 1, 8);
    runRight = scene2::SpriteNode::alloc(assets->get<Texture>(COP_RUN_RIGHT), 1, 8);
    runLeft = scene2::SpriteNode::alloc(assets->get<Texture>(COP_RUN_LEFT), 1, 8);
    
    // Initialize the first texture. Note: width is in screen coordinates
    float width = size.width * scale * 1.5f;
    _characterLeft = runLeft;
    _characterLeft->setScale(0.25f);
    _characterLeft->setAnchor(Vec2::ANCHOR_CENTER);
    _characterLeft->setPosition(Vec2(0, width / 2.5f));
    _node->addChild(_characterLeft);
    _characterRight = runRight;
    _characterRight->setScale(0.25f);
    _characterRight->setAnchor(Vec2::ANCHOR_CENTER);
    _characterRight->setPosition(Vec2(0, width / 2.5f));
    _node->addChild(_characterRight);
    _characterFront = runFront;
    _characterFront->setScale(0.25f);
    _characterFront->setAnchor(Vec2::ANCHOR_CENTER);
    _characterFront->setPosition(Vec2(0, width / 2.5f));
    _node->addChild(_characterFront);
    _characterBack = runBack;
    _characterBack->setScale(0.25f);
    _characterBack->setAnchor(Vec2::ANCHOR_CENTER);
    _characterBack->setPosition(Vec2(0, width / 2.5f));
    _node->addChild(_characterBack);
    
    _character = scene2::PolygonNode::allocWithTexture(_tackleDownTexture);
    _character->setScale(_characterLeft->getWidth() / _tackleDownTexture->getWidth());
    _character->setAnchor(Vec2::ANCHOR_CENTER);
    _character->setPosition(Vec2(0, width / 2.5f));
    _character->setVisible(false);
    _node->addChild(_character);
    // TODO: Get rid of the magic numbers in the lines above.

    b2Filter filter = b2Filter();
    filter.maskBits = COP_FILTER_BITS;
    filter.categoryBits = COP_FILTER_BITS;
    setFilterData(filter);
    
    return true;
}

void CopModel::dispose() {
    PlayerModel::dispose();
    _tackleDownTexture = nullptr;
    _tackleUpTexture = nullptr;
    _tackleLeftTexture = nullptr;
    _tackleRightTexture = nullptr;
    _landDownTexture = nullptr;
    _landUpTexture = nullptr;
    _landLeftTexture = nullptr;
    _landRightTexture = nullptr;
    _character = nullptr;
}

void CopModel::showTackle(Vec2 direction, bool inAir) {
    int key = findDirection(direction);
    _characterRight->setVisible(false);
    _characterLeft->setVisible(false);
    _characterFront->setVisible(false);
    _characterBack->setVisible(false);
    _character->setVisible(true);
    switch (key) {
        case RIGHT_ANIM_KEY:
            _character->setTexture(inAir ? _tackleRightTexture : _landRightTexture);
            break;
        case BACK_ANIM_KEY:
            _character->setTexture(inAir ? _tackleUpTexture : _landUpTexture);
            break;
        case LEFT_ANIM_KEY:
            _character->setTexture(inAir ? _tackleLeftTexture : _landLeftTexture);
            break;
        case FRONT_ANIM_KEY:
            _character->setTexture(inAir ? _tackleDownTexture : _landDownTexture);
            break;
        default:
            break;
    }
}

void CopModel::hideTackle() {
    _character->setVisible(false);
}

void CopModel::failedTackle(float timer, cugl::Vec2 swipe) {
    if (timer <= TACKLE_AIR_TIME) {
        Vec2 normSwipe = swipe.getNormalization();
        b2Vec2 vel(normSwipe.x * COP_MAX_SPEED_DEFAULT * TACKLE_MOVEMENT_MULT, -normSwipe.y * COP_MAX_SPEED_DEFAULT * TACKLE_MOVEMENT_MULT);
        _body->SetLinearVelocity(vel);
    }
    else {
        b2Vec2 b2damping(getVelocity().x * -getDamping() * TACKLE_DAMPING_MULT,
            getVelocity().y * -getDamping() * TACKLE_DAMPING_MULT);
        _body->ApplyForceToCenter(b2damping, true);
    }
}

void CopModel::playAnimation(std::shared_ptr<scene2::ActionManager>& actions, Vec2 movement) {
    PlayerModel::playAnimation(actions, movement);
    _character->setVisible(false);
}
