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
    
    // Set up the textures for all directions
    runFront = scene2::SpriteNode::alloc(assets->get<Texture>(COP_RUN_FRONT), 1, 8);
    runBack = scene2::SpriteNode::alloc(assets->get<Texture>(COP_RUN_BACK), 1, 8);
    runRight = scene2::SpriteNode::alloc(assets->get<Texture>(COP_RUN_RIGHT), 1, 8);
    runLeft = scene2::SpriteNode::alloc(assets->get<Texture>(COP_RUN_LEFT), 1, 8);
    
    // Initialize the first texture. Note: width is in screen coordinates
    float width = size.width * scale * 1.5f;
    _characterLeft = runLeft;
    _characterLeft->setScale(0.3f);
    _characterLeft->setAnchor(Vec2::ANCHOR_CENTER);
    _characterLeft->setPosition(Vec2(0, width / 2.5f));
    _node->addChild(_characterLeft);
    _characterRight = runRight;
    _characterRight->setScale(0.3f);
    _characterRight->setAnchor(Vec2::ANCHOR_CENTER);
    _characterRight->setPosition(Vec2(0, width / 2.5f));
    _node->addChild(_characterRight);
    _characterFront = runFront;
    _characterFront->setScale(0.3f);
    _characterFront->setAnchor(Vec2::ANCHOR_CENTER);
    _characterFront->setPosition(Vec2(0, width / 2.5f));
    _node->addChild(_characterFront);
    _characterBack = runBack;
    _characterBack->setScale(0.3f);
    _characterBack->setAnchor(Vec2::ANCHOR_CENTER);
    _characterBack->setPosition(Vec2(0, width / 2.5f));
    _node->addChild(_characterBack);
    // TODO: Get rid of the magic numbers in the lines above.

    b2Filter fitler = b2Filter();
    fitler.maskBits = COP_FILTER_BITS;
    fitler.categoryBits = COP_FILTER_BITS;
    setFilterData(fitler);
    
    return true;
}

void CopModel::failedTackle(float timer, cugl::Vec2 swipe) {
    if (timer <= TACKLE_AIR_TIME) {
        Vec2 normSwipe = swipe.getNormalization();
        b2Vec2 vel(normSwipe.x * COP_MAX_SPEED *TACKLE_MOVEMENT_MULT, -normSwipe.y * COP_MAX_SPEED * TACKLE_MOVEMENT_MULT);
        _body->SetLinearVelocity(vel);
    }
    else {
        b2Vec2 b2damping(getVelocity().x * -getDamping() * TACKLE_DAMPING_MULT,
            getVelocity().y * -getDamping() * TACKLE_DAMPING_MULT);
        _body->ApplyForceToCenter(b2damping, true);
    }
}
