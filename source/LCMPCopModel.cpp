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
#define COP_WIDTH       1.6f
/** The height of a cop body (its dropshadow) in world units */
#define COP_HEIGHT      0.8f

/** Keys for cop run textures */
#define COP_RUN_BACK        "ss_cop_up"
#define COP_RUN_FRONT       "ss_cop_down"
#define COP_RUN_LEFT        "ss_cop_left"
#define COP_RUN_RIGHT       "ss_cop_right"
#define COP_RUN_BACK_F      "ss_cop_up_f"
#define COP_RUN_FRONT_F     "ss_cop_down_f"
#define COP_RUN_LEFT_F      "ss_cop_left_f"
#define COP_RUN_RIGHT_F     "ss_cop_right_f"
/** Keys for cop tackle textures */
#define COP_JUMP_UP         "cop_jump_up"
#define COP_JUMP_DOWN       "cop_jump_down"
#define COP_JUMP_LEFT        "cop_jump_left"
#define COP_JUMP_RIGHT       "cop_jump_right"
#define COP_LAND_UP         "cop_land_up"
#define COP_LAND_DOWN       "cop_land_down"
#define COP_LAND_LEFT        "cop_land_left"
#define COP_LAND_RIGHT       "cop_land_right"
#define COP_JUMP_LEFT        "cop_jump_left_f"
#define COP_JUMP_RIGHT       "cop_jump_right_f"
#define COP_LAND_LEFT        "cop_land_left_f"
#define COP_LAND_RIGHT       "cop_land_right_f"
/** Keys for cop still textures */
#define COP_IDLE_RIGHT        "ss_cop_idle_right"
#define COP_IDLE_LEFT       "ss_cop_idle_left"
#define COP_IDLE_RIGHT_F        "ss_cop_idle_right_f"
#define COP_IDLE_LEFT_F       "ss_cop_idle_left_f"

//  MARK: - Constructors

/**
 * Initializes a Cop Model
 *
 * @param scale screenunit/worldunit
 */
bool CopModel::init(int copID, float scale,
                      const std::shared_ptr<cugl::scene2::SceneNode>& node,
                      const std::shared_ptr<cugl::AssetManager>& assets,
                    std::shared_ptr<cugl::scene2::ActionManager>& actions,
                    std::string skinKey, bool male) {
    // The cop has constant size
    Size size(COP_WIDTH, COP_HEIGHT);
    _animFrames = {8, 8, 8, 8, 4, 4};
    
    // Call the parent's initializer
    PlayerModel::init(copID, Vec2::ZERO, size, scale, node, assets, actions, skinKey, male);
    
    // Set up the textures for all tackle directions
    _tackleDownTexture = assets->get<Texture>(COP_JUMP_DOWN);
    _tackleUpTexture = assets->get<Texture>(COP_JUMP_UP);
    _tackleLeftTexture = assets->get<Texture>(COP_JUMP_LEFT);
    _tackleRightTexture = assets->get<Texture>(COP_JUMP_RIGHT);
    _landDownTexture = assets->get<Texture>(COP_LAND_DOWN);
    _landUpTexture = assets->get<Texture>(COP_LAND_UP);
    _landLeftTexture = assets->get<Texture>(COP_LAND_LEFT);
    _landRightTexture = assets->get<Texture>(COP_LAND_RIGHT);
    
    // Set up the textures for all directions
    _spriteSheets.push_back(assets->get<Texture>(male ? COP_RUN_RIGHT : COP_RUN_RIGHT_F));
    _spriteSheets.push_back(assets->get<Texture>(male ? COP_RUN_BACK : COP_RUN_BACK_F));
    _spriteSheets.push_back(assets->get<Texture>(male ? COP_RUN_LEFT : COP_RUN_LEFT_F));
    _spriteSheets.push_back(assets->get<Texture>(male ? COP_RUN_FRONT : COP_RUN_BACK_F));
    _spriteSheets.push_back(assets->get<Texture>(male ? COP_IDLE_RIGHT : COP_IDLE_RIGHT_F));
    _spriteSheets.push_back(assets->get<Texture>(male ? COP_IDLE_LEFT : COP_IDLE_LEFT_F));
        
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
    
    // Initialize tackle properties
    _tacklePosition = Vec2::ZERO;
    _tackleTime = 0;
    _tackling = false;
    _caughtThief = false;
    _tackleSuccessful = false;
    didTackle = false;
    didLand = false;

    b2Filter filter = b2Filter();
    filter.maskBits = COP_FILTER_BITS;
    filter.categoryBits = COP_FILTER_BITS;
    setFilterData(filter);
    
    return true;
}

/**
 * Disposes of all resources in this instance of Cop Model
 */
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

/**
 * Attempts to tackle the thief and sets the appropriate properties depending on success/failure
 */
void CopModel::attemptTackle(Vec2 thiefPosition, Vec2 tackle) {
    // Set the basic tackle properties
    _tackling = true;
    _tackleTime = 0;
    _tacklePosition = getPosition();
    _tackleDirection = tackle;
    
    // Do some math between this cop, the the thief, and the tackle
    Vec2 dist = thiefPosition - _tacklePosition;
    float angle = abs(abs(tackle.getAngle()) - abs(dist.getAngle()));
    
    // See if the tackle was successful
    _tackleSuccessful =
        angle <= TACKLE_ANGLE_MAX_ERR
        && dist.lengthSquared() < TACKLE_HIT_RADIUS * TACKLE_HIT_RADIUS;
}

/**
 * Applies physics to cop when tackling
 */
void CopModel::applyTackle(float timestep, Vec2 thiefPosition) {
    didTackle = true;
    _tackleTime += timestep;
    applyTackleFailure();
    //_tackleSuccessful
    //    ? applyTackleSuccess(thiefPosition)
    //    : applyTackleFailure();
}

/**
 * Updates the position and velocity of the cop, applies forces, and updates tackle properties
 */
void CopModel::applyNetwork(cugl::Vec2 position, cugl::Vec2 velocity,
                            cugl::Vec2 force, cugl::Vec2 tackleDirection,
                            cugl::Vec2 tacklePosition,
                            float tackleTime,
                            bool tackling,
                            bool caughtThief,
                            bool tackleSuccessful) {
    PlayerModel::applyNetwork(position, velocity, force);
    _tackleDirection = tackleDirection;
    _tacklePosition = tacklePosition;
    _tackleTime = tackleTime;
    _tackling = tackling;
    _caughtThief = caughtThief;
    _tackleSuccessful = tackleSuccessful;
}

/**
 * Performs a film strip action
 */
void CopModel::playAnimation() {
    if (_tackling) playTackle();
    else {
        PlayerModel::playAnimation();
        _character->setVisible(false);
    }
}

//  MARK: - Helpers

/**
 * Applies physics during a failed tackle
 */
void CopModel::applyTackleFailure() {
    // The cop is still in the air
    if (_tackleTime <= TACKLE_AIR_TIME) {
        Vec2 normTackle = _tackleDirection.getNormalization();
        b2Vec2 vel(normTackle.x * COP_MAX_SPEED_DEFAULT * TACKLE_MOVEMENT_MULT,
                   normTackle.y * COP_MAX_SPEED_DEFAULT * TACKLE_MOVEMENT_MULT);
        _realbody->SetLinearVelocity(vel);
    }
    
    // The cop is on the floor
    else {
        didLand = true;
        b2Vec2 b2damping(
            getVelocity().x * -getDamping().x * TACKLE_DAMPING_MULT,
            getVelocity().y * -getDamping().y * TACKLE_DAMPING_MULT);
        _realbody->ApplyForceToCenter(b2damping, true);
    }
    
    // The cop can get off the floor
    if (_tackleTime >= TACKLE_COOLDOWN_TIME) _tackling = false;
}

/**
 * Applies physics during a successful tackle
 */
void CopModel::applyTackleSuccess(Vec2 thiefPosition) {
    // Perform interpolation between the thief and the starting tackle position
    Vec2 diff = thiefPosition - _tacklePosition;
    setPosition(_tacklePosition + (diff * (_tackleTime / TACKLE_AIR_TIME)));
    
    // Terminate the tackle
    if (_tackleTime >= TACKLE_AIR_TIME) {
        _tackling = false;
        _caughtThief = true;
    }
}

/**
 * Updates nodes to show tackle animation
 */
void CopModel::playTackle() {
    // Determine which direction the cop is facing
    int key = findDirection(_tackleDirection);
    
    // Determine whether the cop is in the air or not
    bool inAir = _tackleTime < TACKLE_AIR_TIME;
    
    // Hide the idle and movement animations
    for (std::shared_ptr<scene2::SpriteNode> s : _spriteNodes) s->setVisible(false);
    
    // Show the tackle texture
    _character->setVisible(true);
    
    // Show the appropriate direction
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
