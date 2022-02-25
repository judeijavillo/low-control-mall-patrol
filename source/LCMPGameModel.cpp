//
//  LCMPGameModel.cpp
//  Low Control Mall Patrol
//
//  Author: Kevin Games
//  Version: 2/20/22
//

#include "LCMPGameModel.h"
#include "LCMPThiefModel.h"
#include "LCMPCopModel.h"
#include "LCMPTrapModel.h"
#include <cugl/assets/CUJsonLoader.h>
#include "LCMPLevelConstants.h"
#include <cmath>

/**
* Creates a new, empty level.
*/
GameModel::GameModel(void) : Asset(),
_root(nullptr),
_world(nullptr),
_worldnode(nullptr),
_debugnode(nullptr)
{
	_bounds.size.set(1.0f, 1.0f);
}

/**
* Destroys this level, releasing all resources.
*/
GameModel::~GameModel(void) {
	unload();
	clearRootNode();
}

/**
* Sets the drawing scale for this game level
*
* The drawing scale is the number of pixels to draw before Box2D unit. Because
* mass is a function of area in Box2D, we typically want the physics objects
* to be small.  So we decouple that scale from the physics object.  However,
* we must track the scale difference to communicate with the scene graph.
*
* We allow for the scaling factor to be non-uniform.
*
* @param value  the drawing scale for this game level
*/
void GameModel::setDrawScale(float value) {
	// if (_rocket != nullptr) {
	// 	_rocket->setDrawScale(value);
	// }
    // Not sure what to put here + not sure if this method is implemented for thiefs/ cops.
}

/**
* Clears the root scene graph node for this level
*/
void GameModel::clearRootNode() {
	if (_root == nullptr) {
		return;
	}
    _worldnode->removeFromParent();
	_worldnode->removeAllChildren();
    _worldnode = nullptr;
  
    _debugnode->removeFromParent();
	_debugnode->removeAllChildren();
	_debugnode = nullptr; 

	_root = nullptr;
}

/**
* Sets the scene graph node for drawing purposes.
*
* The scene graph is completely decoupled from the physics system.  The node
* does not have to be the same size as the physics body. We only guarantee
* that the node is positioned correctly according to the drawing scale.
*
* @param value  the scene graph node for drawing purposes.
*
* @retain  a reference to this scene graph node
* @release the previous scene graph node used by this object
*/
void GameModel::setRootNode(const std::shared_ptr<scene2::SceneNode>& node) {
	if (_root != nullptr) {
		clearRootNode();
	}

	_root = node;
	_scale.set(_root->getContentSize().width/_bounds.size.width,
             _root->getContentSize().height/_bounds.size.height);

	// Create, but transfer ownership to root
	_worldnode = scene2::SceneNode::alloc();
    _worldnode->setAnchor(Vec2::ANCHOR_BOTTOM_LEFT);
    _worldnode->setPosition(Vec2::ZERO);
  
	_debugnode = scene2::SceneNode::alloc();
    _debugnode->setScale(_scale); // Debug node draws in PHYSICS coordinates
    _debugnode->setAnchor(Vec2::ANCHOR_BOTTOM_LEFT);
    _debugnode->setPosition(Vec2::ZERO);
  
	_root->addChild(_worldnode);
	_root->addChild(_debugnode);

	// Add the individual elements
	std::shared_ptr<scene2::PolygonNode> poly;
	std::shared_ptr<scene2::WireNode> draw;

    // TODO: Add shit.

// 	if (_goalDoor != nullptr) {
//         auto sprite = scene2::PolygonNode::allocWithTexture(_assets->get<Texture>(_goalDoor->getTextureKey()));
// 		addObstacle(_goalDoor,sprite); // Put this at the very back
// 	}

// 	for(auto it = _crates.begin(); it != _crates.end(); ++it) {
// 		std::shared_ptr<CrateModel> crate = *it;
// 		auto sprite = scene2::PolygonNode::allocWithTexture(_assets->get<Texture>(crate->getTextureKey()));
// 		int indx = (std::rand() % 2 == 0 ? 2 : 1);
// 		addObstacle(crate,sprite);   // PUT SAME TEXTURES IN SAME LAYER!!!
// 	}

// 	for(auto it = _walls.begin(); it != _walls.end(); ++it) {
// 		std::shared_ptr<WallModel> wall = *it;
// 		auto sprite = scene2::PolygonNode::allocWithTexture(_assets->get<Texture>(wall->getTextureKey()),
//                                                             wall->getPolygon() * _scale);
// 		addObstacle(wall,sprite);  // All walls share the same texture
// 	}
  
//   if (_rocket != nullptr) {
// 		auto rocketNode = scene2::PolygonNode::allocWithTexture(_assets->get<Texture>(_rocket->getTextureKey()));
// 		_rocket->setShipNode(rocketNode, _assets);
//         _rocket->setDrawScale(_scale.x);

// 		// Create the polygon node (empty, as the model will initialize)
// 		_worldnode->addChild(rocketNode);
// 		_rocket->setDebugScene(_debugnode);
// 	}
}

/**
* Toggles whether to show the debug layer of this game world.
*
* The debug layer displays wireframe outlines of the physics fixtures.
*
* @param  flag whether to show the debug layer of this game world
*/
void GameModel::showDebug(bool flag) {
	if (_debugnode != nullptr) {
		_debugnode->setVisible(flag);
	}
}

/**
 * Loads this game level from the source file
 *
 * This load method should NEVER access the AssetManager.  Assets are loaded in
 * parallel, not in sequence.  If an asset (like a game level) has references to
 * other assets, then these should be connected later, during scene initialization.
 *
 * @return true if successfully loaded the asset from a file
 */
bool GameModel::preload(const std::string& file) {
	std::shared_ptr<JsonReader> reader = JsonReader::allocWithAsset(file);
	return preload(reader->readJson());
}


/**
 * Loads this game level from the source file
 *
 * This load method should NEVER access the AssetManager.  Assets are loaded in
 * parallel, not in sequence.  If an asset (like a game level) has references to
 * other assets, then these should be connected later, during scene initialization.
 *
 * @return true if successfully loaded the asset from a file
 */
bool GameModel:: preload(const std::shared_ptr<cugl::JsonValue>& json) {
	if (json == nullptr) {
		CUAssertLog(false, "Failed to load level file");
		return false;
	}
	// Initial geometry

	auto layers = json->get(LAYERS_FIELD);

	auto tile_layer = layers->get(TILES_FIELD);
	auto object_layer = layers->get(OBJECTS_FIELD);

	float w = tile_layer->get(WIDTH_FIELD)->asFloat();
	float h = tile_layer->get(HEIGHT_FIELD)->asFloat();

	
	auto walls = tile_layer->get(WALLS_FIELD);
	auto obstacles = object_layer->get(OBSTACLES_FIELD);

	_bounds.size.set(w, h);
	_gravity.set(0, 0);

	/** Create the physics world */
	_world = physics2::ObstacleWorld::alloc(getBounds(), getGravity());

	// Load Wall.

	if (obstacles != nullptr) {
		// Convert the object to an array so we can see keys and values
		int osize = (int)obstacles->size();
		for(int ii = 0; ii < osize; ii++) {
			loadObstacle(obstacles->get(ii));
		}
	} else {
	 	CUAssertLog(false, "Failed to load walls");
	 	return false;
	}

    // TODO Load shit.

	// // Parse the rocket
	// if (!loadRocket(json)) {
	// 	CUAssertLog(false, "Failed to load rocket");
	// 	return false;
	// }

	// if (!loadGoalDoor(json)) {
	// 	CUAssertLog(false, "Failed to load goal door");
	// 	return false;
	// }

	// auto walls = json->get(WALLS_FIELD);
	// if (walls != nullptr) {
	// 	// Convert the object to an array so we can see keys and values
	// 	int wsize = (int)walls->size();
	// 	for(int ii = 0; ii < wsize; ii++) {
	// 		loadWall(walls->get(ii));
	// 	}
	// } else {
	// 	CUAssertLog(false, "Failed to load walls");
	// 	return false;
	// }
	// auto crates = json->get(CRATES_FIELD);
	// if (crates != nullptr) {
	// 	// Convert the object to an array so we can see keys and values
	// 	int csize = (int)crates->size();
	// 	for(int ii = 0; ii < csize; ii++) {
	// 		loadCrate(crates->get(ii));
	// 	}
	// } else {
	// 	CUAssertLog(false, "Failed to load crates");
	// 	return false;
	// }

	return true;
}

/**
* Unloads this game level, releasing all sources
*
* This load method should NEVER access the AssetManager.  Assets are loaded and
* unloaded in parallel, not in sequence.  If an asset (like a game level) has
* references to other assets, then these should be disconnected earlier.
*/
void GameModel::unload() {
	//if (_rocket != nullptr) {
	//	if (_world != nullptr) {
	//		_world->removeObstacle(_rocket.get());
	//	}
	//	_rocket = nullptr;
	//}
	//if (_goalDoor != nullptr) {
	//	if (_world != nullptr) {
	//		_world->removeObstacle(_goalDoor.get());
	//	}
	//	_goalDoor = nullptr;
	//}
	//for(auto it = _crates.begin(); it != _crates.end(); ++it) {
	//	if (_world != nullptr) {
	//		_world->removeObstacle((*it).get());
	//	}
 //   (*it) = nullptr;
	//}
	//_crates.clear();
	//for(auto it = _walls.begin(); it != _walls.end(); ++it) {
	//	if (_world != nullptr) {
	//		_world->removeObstacle((*it).get());
	//	}
 //   (*it) = nullptr;
	//}
	//_walls.clear();
	//if (_world != nullptr) {
	//	_world->clear();
	//	_world = nullptr;
	//}
}


/**
 * Loads an object from the JSON
 *
 * These are only obstacles.
 *
 * @param  json   a JSON Value with the json for related objects.
 *
 * @return true if the objects were loaded successfully.
 */
bool loadObstacle(const std::shared_ptr<JsonValue>& json) {
    bool success = true;
	
	bool ellipse = json->get(ELLIPSE_FIELD)->asBool();
	auto polygon = json->get(POLYGON_FIELD);
    
    auto height = json->get(HEIGHT_FIELD)->asInt();
    auto width = json->get(WIDTH_FIELD)->asInt();
    auto rotation = json->get(ROTATION_FIELD)->asFloat();
    auto x = json->get(X_FIELD)->asFloat();
    auto y = json->get(Y_FIELD)->asFloat();
    
    std::shared_ptr<physics2::SimpleObstacle> obstacle;
	if (ellipse) { // circle or ellipse
        if (height == width) { // circle case, uses wheelobstacle
            obstacle = physics2::WheelObstacle::alloc(Vec2(x,y), height/2);
        } else { // ellipse case, uses extruded path2
            auto poly = PolyFactory().makeEllipse(Vec2(x,y), Vec2(width,height));
            obstacle = physics2::PolygonObstacle::alloc(poly);
            obstacle->setAngle((rotation*M_PI)/(180));
        }
	}
	else if (polygon != nullptr) { // polygon
		std::vector<float> verts = json->get(VERTICES)->asFloatArray();
		EarclipTriangulator triangulator;
		triangulator.set(Poly2(reinterpret_cast<Vec2*>(&verts[0]), (int)verts.size() / 2));
		triangulator.calculate();
		obstacle = triangulator.getPolygon();
	}
	else { // rectangle
		auto rect = physics2::BoxObstacle::alloc(Vec2(x, y), Vec2(width, height));
		obstacle = physics2::PolygonObstacle::alloc(poly);
		obstacle->setAngle((rotation * M_PI) / (180));
	}



	int polysize = json->getInt(VERTICES_FIELD);
	success = success && polysize > 0;

	std::vector<float> vertices = json->get(BOUNDARY_FIELD)->asFloatArray();
	success = success && 2*polysize == vertices.size();

    Vec2* verts = reinterpret_cast<Vec2*>(&vertices[0]);
	Poly2 wall(verts,(int)vertices.size()/2);
	//EarclipTriangulator triangulator;
	triangulator.set(wall.vertices);
	triangulator.calculate();
	wall.setIndices(triangulator.getTriangulation());
    triangulator.clear();
	
	// Get the object, which is automatically retained
	std::shared_ptr<WallModel> wallobj = WallModel::alloc(wall);
	wallobj->setName(json->key());

	std::string btype = json->getString(BODYTYPE_FIELD);
	if (btype == STATIC_VALUE) {
		wallobj->setBodyType(b2_staticBody);
	}

	wallobj->setDensity(json->getDouble(DENSITY_FIELD));
	wallobj->setFriction(json->getDouble(FRICTION_FIELD));
	wallobj->setRestitution(json->getDouble(RESTITUTION_FIELD));

	// Set the texture value
	success = success && json->get(TEXTURE_FIELD)->isString();
	wallobj->setTextureKey(json->getString(TEXTURE_FIELD));
	wallobj->setDebugColor(parseColor(json->getString(DEBUG_COLOR_FIELD)));

	if (success) {
		_walls.push_back(wallobj);
	} else {
		wallobj = nullptr;
	}

	vertices.clear();
	return success;
}















/**
* Converts the string to a color
*
* Right now we only support the following colors: yellow, red, blur, green,
* black, and grey.
*
* @param  name the color name
*
* @return the color for the string
*/
Color4 GameModel::parseColor(std::string name) {
	if (name == "yellow") {
		return Color4::YELLOW;
	} else if (name == "red") {
		return Color4::RED;
	} else if (name == "green") {
		return Color4::GREEN;
	} else if (name == "blue") {
		return Color4::BLUE;
	} else if (name == "black") {
		return Color4::BLACK;
	} else if (name == "gray") {
		return Color4::GRAY;
	}
	return Color4::WHITE;
}

/**
 * Adds the physics object to the physics world and loosely couples it to the scene graph
 *
 * There are two ways to link a physics object to a scene graph node on the
 * screen.  One way is to make a subclass of a physics object, like we did
 * with rocket.  The other is to use callback functions to loosely couple
 * the two.  This function is an example of the latter.
 *
 *
 * param obj    The physics object to add
 * param node   The scene graph node to attach it to
 */
void GameModel::addObstacle(const std::shared_ptr<cugl::physics2::Obstacle>& obj,
                             const std::shared_ptr<cugl::scene2::SceneNode>& node) {
	_world->addObstacle(obj);
	obj->setDebugScene(_debugnode);

	// Position the scene graph node (enough for static objects)
	node->setPosition(obj->getPosition()*_scale);
	_worldnode->addChild(node);


    // Currently no dynamo bodies so no can do.

	// Dynamic objects need constant updating
	// if (obj->getBodyType() == b2_dynamicBody) {
    //     scene2::SceneNode* weak = node.get(); // No need for smart pointer in callback
	// 	obj->setListener([=](physics2::Obstacle* obs){
	// 		weak->setPosition(obs->getPosition()*_scale);
	// 		weak->setAngle(obs->getAngle());
	// 	});
	// }
}

