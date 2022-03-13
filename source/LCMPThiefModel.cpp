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
#define THIEF_RUN_BACK      "thief_run_back"
#define THIEF_RUN_FRONT     "thief_run_front"
#define THIEF_RUN_LEFT      "thief_run_left"
#define THIEF_RUN_RIGHT     "thief_run_right"

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
    
    // Set up the textures for all directions
    _runBackTexture = assets->get<Texture>(THIEF_RUN_BACK);
    _runFrontTexture = assets->get<Texture>(THIEF_RUN_FRONT);
    _runLeftTexture = assets->get<Texture>(THIEF_RUN_LEFT);
    _runRightTexture = assets->get<Texture>(THIEF_RUN_RIGHT);
    
    // Initialize the first texture. Note: width is in screen coordinates
    float width = size.width * scale * 1.5f;
    _character = scene2::PolygonNode::allocWithTexture(_runLeftTexture);
    _character->setScale(width / _runLeftTexture->getSize().width);
    _character->setAnchor(Vec2::ANCHOR_CENTER);
    _character->setPosition(Vec2(0, width / 2.5f));
    _node->addChild(_character);
    // TODO: Get rid of the magic numbers in the lines above.

    b2Filter fitler = b2Filter();
    fitler.maskBits = THIEF_FILTER_BITS;
    fitler.categoryBits = THIEF_FILTER_BITS;
    setFilterData(fitler);
    
    return true;
}


