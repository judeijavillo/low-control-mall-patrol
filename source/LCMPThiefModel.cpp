//
//  LCMPThiefModel.cpp
//  Low Control Mall Patrol
//
//  Author: Kevin Games
//  Version: 2/20/22
//

#include "LCMPThiefModel.h"

using namespace cugl;
using namespace std;

//  MARK: - Constants

/** The width of the thief body (its dropshadow) in world units */
#define THIEF_WIDTH     2.0f
/** The height of the thief body (its dropshadow) in world units */
#define THIEF_HEIGHT    1.0f

/** Keys for thief run textures */
#define THIEF_RUN_BACK      "ss_thief_up"
#define THIEF_RUN_FRONT     "ss_thief_down"
#define THIEF_RUN_LEFT      "ss_thief_left"
#define THIEF_RUN_RIGHT     "ss_thief_right"

//  MARK: - Constructors

/**
 * Initializes a Thief Model
 */
bool ThiefModel::init(float scale, 
                      const std::shared_ptr<cugl::scene2::SceneNode>& node,
                      const std::shared_ptr<cugl::AssetManager>& assets) {
    // The thief has constant size
    Size size(THIEF_WIDTH, THIEF_HEIGHT);
    
    // Call the parent's initializer
    PlayerModel::init(Vec2::ZERO, size, scale, node);
    
    // Up character movement
    std::vector<int> north;
    for(int ii = 0; ii < 6; ii++) {
        north.push_back(ii);
    }
    _north = scene2::Animate::alloc(north,DURATION);

    // Down character movement
    std::vector<int> south;
    for(int ii = 1; ii < 6; ii++) {
        south.push_back(ii);
    }
    _south = scene2::Animate::alloc(south,DURATION);
    
    // Set up the textures for all directions
    runFront = scene2::SpriteNode::alloc(assets->get<Texture>(THIEF_RUN_FRONT), 1, 6);
    runBack = scene2::SpriteNode::alloc(assets->get<Texture>(THIEF_RUN_BACK), 1, 6);
    runRight = scene2::SpriteNode::alloc(assets->get<Texture>(THIEF_RUN_RIGHT), 1, 8);
    runLeft = scene2::SpriteNode::alloc(assets->get<Texture>(THIEF_RUN_LEFT), 1, 8);
    
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
    fitler.maskBits = THIEF_FILTER_BITS;
    fitler.categoryBits = THIEF_FILTER_BITS;
    setFilterData(fitler);
    
    return true;
}


