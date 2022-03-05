//
//  LCMPObstacleModel.cpp
//  LowControlMallPatrol
//
//  Author: Kevin Games
//  Version: 3/04/22
//

#include "LCMPObstacleModel.h"

using namespace cugl;
using namespace std;

//  MARK: - Constants

/** Keys for obstacle textures */
#define TREE_TEXTURE    "tree"
#define BUSH_TEXTURE    "bush"
#define FARIS_TEXTURE   "faris"

/** The sizes of obstacles in Box2D units */
#define TREE_WIDTH      10
#define BUSH_WIDTH      10
#define FARIS_WIDTH     10

//  MARK: - Constructors

/**
 * Initializes an Obstacle Model
 */
bool ObstacleModel::init(float scale, shared_ptr<Texture>& texture, Type type) {
    PolyFactory pf;
    switch (type) {
        case TREE:
            PolygonObstacle::init(pf.makeRect(0, 0, TREE_WIDTH, TREE_WIDTH / 2));
            break;
        case BUSH:
            PolygonObstacle::init(pf.makeRect(0, 0, BUSH_WIDTH, BUSH_WIDTH / 4));
            break;
        case FARIS:
            PolygonObstacle::init(pf.makeRect(0, 0, FARIS_WIDTH, FARIS_WIDTH / 3));
            break;
    }
    setDebugColor(Color4::RED);
    return true;
}
