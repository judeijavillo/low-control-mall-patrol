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

/** Keys for cop run textures */
#define COP_RUN_BACK        "cop_run_back"
#define COP_RUN_FRONT       "cop_run_front"
#define COP_RUN_LEFT        "cop_run_left"
#define COP_RUN_RIGHT       "cop_run_right"

//  MARK: - Constructors

/**
 * Initializes a Cop Model
 */
bool CopModel::init(const cugl::Vec2 pos, const cugl::Size size, float scale,
                      const std::shared_ptr<cugl::scene2::SceneNode>& node,
                      const std::shared_ptr<cugl::AssetManager>& assets) {
    // Call the parent's initializer
    PlayerModel::init(pos, size, scale, node);
    
    // Set up the textures for all directions
    _runBackTexture = assets->get<Texture>(COP_RUN_BACK);
    _runFrontTexture = assets->get<Texture>(COP_RUN_FRONT);
    _runLeftTexture = assets->get<Texture>(COP_RUN_LEFT);
    _runRightTexture = assets->get<Texture>(COP_RUN_RIGHT);
    
    // Initialize the first texture. Note: width is in screen coordinates
    float width = size.width * scale * 1.5f;
    _character = scene2::PolygonNode::allocWithTexture(_runRightTexture);
    _character->setScale(width / _runLeftTexture->getSize().width);
    _character->setAnchor(Vec2::ANCHOR_CENTER);
    _character->setPosition(Vec2(0, width / 2.5f));
    _node->addChild(_character);
    // TODO: Get rid of the magic numbers in the lines above.
    
    return true;
}
