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
/** Keys for thief still textures */
#define THIEF_IDLE_RIGHT        "ss_thief_idle_right"
#define THIEF_IDLE_LEFT       "ss_thief_idle_left"

//  MARK: - Constructors

/**
 * Initializes a Thief Model
 */
bool ThiefModel::init(float scale, 
                      const std::shared_ptr<cugl::scene2::SceneNode>& node,
                      const std::shared_ptr<cugl::AssetManager>& assets,
                      std::shared_ptr<cugl::scene2::ActionManager>& actions) {
    // The thief has constant size
    Size size(THIEF_WIDTH, THIEF_HEIGHT);
    _animFrames = {8, 6, 8, 6, 4, 4};
    
    // Call the parent's initializer
    PlayerModel::init(-1, Vec2::ZERO, size, scale, node, actions);
    
    // Set up the textures for all directions
    _spriteSheets.push_back(assets->get<Texture>(THIEF_RUN_RIGHT));
    _spriteSheets.push_back(assets->get<Texture>(THIEF_RUN_BACK));
    _spriteSheets.push_back(assets->get<Texture>(THIEF_RUN_LEFT));
    _spriteSheets.push_back(assets->get<Texture>(THIEF_RUN_FRONT));
    _spriteSheets.push_back(assets->get<Texture>(THIEF_IDLE_RIGHT));
    _spriteSheets.push_back(assets->get<Texture>(THIEF_IDLE_LEFT));
    
    // Initialize the first texture. Note: width is in screen coordinates
    float width = size.width * scale * 1.5f;
    setSpriteNodes(width);
    // TODO: Get rid of the magic numbers in the lines above.

    b2Filter fitler = b2Filter();
    fitler.maskBits = THIEF_FILTER_BITS;
    fitler.categoryBits = THIEF_FILTER_BITS;
    setFilterData(fitler);
    
    return true;
}


