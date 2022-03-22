//
//  LCMPCopModel.cpp
//  Low Control Mall Patrol
//
//  Author: Kevin Games
//  Version: 2/20/22
//

#include "LCMPCopModel.h"

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
/** Keys for cop still textures */
#define COP_IDLE_RIGHT        "ss_cop_idle_right"
#define COP_IDLE_LEFT       "ss_cop_idle_left"

//  MARK: - Constructors

/**
 * Initializes a Cop Model
 *
 * @param scale screenunit/worldunit
 */
bool CopModel::init(float scale,
                      const std::shared_ptr<cugl::scene2::SceneNode>& node,
                      const std::shared_ptr<cugl::AssetManager>& assets,
                    std::shared_ptr<cugl::scene2::ActionManager>& actions) {
    // The cop has constant size
    Size size(COP_WIDTH, COP_HEIGHT);
    _animFrames = {8, 8, 8, 8, 4, 4};
    
    // Call the parent's initializer
    PlayerModel::init(Vec2::ZERO, size, scale, node, actions);
    
    // Set up the textures for all tackle directions
    _tackleDownTexture = assets->get<Texture>(COP_JUMP_DOWN);
    _tackleUpTexture = assets->get<Texture>(COP_JUMP_UP);
    _tackleLeftTexture = assets->get<Texture>(COP_JUMP_LEFT);
    _tackleRightTexture = assets->get<Texture>(COP_JUMP_RIGHT);
    _landDownTexture = assets->get<Texture>(COP_LAND_DOWN);
    _landUpTexture = assets->get<Texture>(COP_LAND_UP);
    _landLeftTexture = assets->get<Texture>(COP_LAND_LEFT);
    _landRightTexture = assets->get<Texture>(COP_LAND_RIGHT);
    
    _spriteSheets.push_back(assets->get<Texture>(COP_RUN_RIGHT));
    _spriteSheets.push_back(assets->get<Texture>(COP_RUN_BACK));
    _spriteSheets.push_back(assets->get<Texture>(COP_RUN_LEFT));
    _spriteSheets.push_back(assets->get<Texture>(COP_RUN_FRONT));
    _spriteSheets.push_back(assets->get<Texture>(COP_IDLE_RIGHT));
    _spriteSheets.push_back(assets->get<Texture>(COP_IDLE_LEFT));
        
    // Initialize the first texture. Note: width is in screen coordinates
    float width = size.width * scale * 1.5f;
    setSpriteNodes(width);
    
    _character = scene2::PolygonNode::allocWithTexture(_tackleDownTexture);
    _character->setScale(_spriteNodes[0]->getWidth() / _tackleDownTexture->getWidth());
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
    // Determine which direction the cop is facing
    int key = findDirection(direction);
    
    
    for (std::shared_ptr<scene2::SpriteNode> s : _spriteNodes) s->setVisible(false);
    

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
        b2Vec2 vel(normSwipe.x * COP_MAX_SPEED_DEFAULT * TACKLE_MOVEMENT_MULT,
                   -normSwipe.y * COP_MAX_SPEED_DEFAULT * TACKLE_MOVEMENT_MULT);
        _body->SetLinearVelocity(vel);
    }
    else {
        b2Vec2 b2damping(getVelocity().x * -getDamping() * TACKLE_DAMPING_MULT,
            getVelocity().y * -getDamping() * TACKLE_DAMPING_MULT);
        _body->ApplyForceToCenter(b2damping, true);
    }
}

void CopModel::playAnimation(Vec2 movement) {
    PlayerModel::playAnimation(movement);
    _character->setVisible(false);
}
