//
//  LCMPGameModel.cpp
//  Low Control Mall Patrol
//
//  Author: Kevin Games
//  Version: 2/20/22
//

#include "LCMPGameModel.h"
#include "LCMPConstants.h"

using namespace cugl;

/**
 * Disposes of all resources in this instance of Game Model
 */
void GameModel::dispose() {
    
}

/**
 * initializes a Game Model
 */
bool GameModel::init(std::shared_ptr<cugl::physics2::ObstacleWorld>& world,
                     std::shared_ptr<cugl::scene2::SceneNode>& worldnode,
                     std::shared_ptr<cugl::scene2::SceneNode>& debugnode,
                     const std::shared_ptr<cugl::AssetManager>& assets,
                     float scale, const std::string& file) {
    _world = world;
    _worldnode = worldnode;
    _debugnode = debugnode;
    _gameover = false;
    
    std::shared_ptr<JsonReader> reader = JsonReader::allocWithAsset(file);
    std::shared_ptr<JsonValue> json = reader->readJson();
    
    _mapWidth = json->getFloat(WIDTH_FIELD);
    _mapHeight = json->getFloat(HEIGHT_FIELD);
    _tileSize = json->getFloat(T_SIZE_FIELD);
    
    std::shared_ptr<JsonValue> layers = json->get(LAYERS_FIELD);
    std::shared_ptr<JsonValue> walls = layers->get(WALLS_FIELD)->get(OBJECTS_FIELD);
    std::shared_ptr<JsonValue> copsSpawn = layers->get(COPS_FIELD)->get(OBJECTS_FIELD);
    std::shared_ptr<JsonValue> thiefSpawn = layers->get(THIEF_FIELD)->get(OBJECTS_FIELD);
    
    // Initialize thief
    initThief(scale, thiefSpawn, assets);
    
    // Initialize cops
    for (int i = 0; i < 4; i++) initCop(i, scale, copsSpawn, assets);
    
    // Initialize walls
    for (int i = 0; i < walls->size(); i++) initWall(walls->get(i), scale);
    
    // Initialize traps
    // TODO: Make this JSON Reading
    Vec2 traps[] = { Vec2(20, 30), Vec2(50, 30), Vec2(80, 30) };
    for (int i = 0; i < 3; i++) initTrap(i, traps[i], scale, assets);
    
    // Initialize borders
    initBorder(scale);
    
    return true;
}

/**
 * Updates all game objects
 */
void GameModel::update(float timestep) {
    // Update the thief
    _thief->update(timestep);
    
    // Update all of the cops
    for (auto entry = _cops.begin(); entry != _cops.end(); entry++) {
        entry->second->update(timestep);
    }
}

/**
 * Applies an acceleration to the thief (most likely for local updates)
 */
void GameModel::updateThief(cugl::Vec2 acceleration) {
    _thief->applyForce(acceleration);
}

/**
 * Applies a force to a cop (most likely for local updates)
 */
void GameModel::updateCop(cugl::Vec2 acceleration, int copID, bool onTackleCooldown) {
    if(!onTackleCooldown)
        _cops[copID]->applyForce(acceleration);
}

/**
 * Updates the position and velocity of the thief
 */
void GameModel::updateThief(cugl::Vec2 position, cugl::Vec2 velocity, cugl::Vec2 force) {
    _thief->applyNetwork(position, velocity, force);
}

/**
 * Updates the position and velocity of a cop
 */
void GameModel::updateCop(cugl::Vec2 position, cugl::Vec2 velocity, cugl::Vec2 force, int copID) {
    _cops[copID]->applyNetwork(position, velocity, force);
}

/**
 * Activates a trap
 */
void GameModel::activateTrap(int trapID) {
    _traps[trapID]->activate();
}

//  MARK: - Helpers

void GameModel::initThief(float scale,
                          const std::shared_ptr<JsonValue>& spawn,
                          const std::shared_ptr<cugl::AssetManager>& assets) {
    // Create thief node
    std::shared_ptr<scene2::SceneNode> thiefNode = scene2::SceneNode::alloc();
    thiefNode->setAnchor(Vec2::ANCHOR_CENTER);
    _worldnode->addChildWithName(thiefNode, "thief");
    
    // Create thief
    _thief = std::make_shared<ThiefModel>();
    _thief->init(scale, thiefNode, assets);
    _thief->setDebugScene(_debugnode);
    _thief->setCollisionSound("fuck");
    _world->addObstacle(_thief);
    
    // Position thief afterwards to not have to deal with changing world size
    _thief->setPosition(Vec2(spawn->get(0)->getFloat(X_FIELD) / _tileSize,
                             _mapHeight - spawn->get(0)->getFloat(Y_FIELD) / _tileSize));
}

void GameModel::initCop(int copID, float scale,
                        const std::shared_ptr<JsonValue>& spawns,
                        const std::shared_ptr<cugl::AssetManager>& assets) {
    // Create cop node
    std::shared_ptr<scene2::SceneNode> copNode = scene2::SceneNode::alloc();
    copNode->setAnchor(Vec2::ANCHOR_CENTER);
    _worldnode->addChildWithName(copNode, "cop" + to_string(copID));
    
    // Create cop
    std::shared_ptr<CopModel> cop = std::make_shared<CopModel>();
    cop->init(scale, copNode, assets);
    cop->setDebugScene(_debugnode);
    cop->setCollisionSound("dude");
    _world->addObstacle(cop);
    
    // Position cop afterwards to not have to deal with changing world size
    
    std::shared_ptr<JsonValue> spawn = spawns->get(copID);
    cop->setPosition(Vec2(spawn->getFloat(X_FIELD) / _tileSize,
                          _mapHeight - spawn->getFloat(Y_FIELD) / _tileSize));
    
    // Add the cop to the mapping of cops
    _cops[copID] = cop;
}

/**
 * Initializes a single wall
 */
void GameModel::initWall(const std::shared_ptr<JsonValue>& json, float scale) {
    std::shared_ptr<JsonValue> polygon = json->get(POLYGON_FIELD);
    bool ellipse = json->getBool(ELLIPSE_FIELD);
    float x = json->getFloat(X_FIELD) / _tileSize;
    float y = json->getFloat(Y_FIELD) / _tileSize;
    float width = json->getFloat(WIDTH_FIELD) / _tileSize;
    float height = json->getFloat(HEIGHT_FIELD) / _tileSize;
    
    // We want to populate the following obstacle and node
    shared_ptr<physics2::PolygonObstacle> wall;
    shared_ptr<scene2::PolygonNode> node;
    
    // The wall component is an ellipse
    if (ellipse) {
        // Adjust the coordinates to match CUGL
        x = x + width / 2;
        y = _mapHeight - (height / 2) - y;
        
        // Make a wall and a corresponding node from a polygon
        // TODO: Maybe make this a model or a type of ObstacleModel
        Poly2 poly = PolyFactory().makeEllipse(Vec2::ZERO, Vec2(width,height));
        wall = physics2::PolygonObstacle::alloc(poly);
        node = scene2::PolygonNode::allocWithPoly(poly);
        node->setPosition(x * scale, y * scale);
    }
    
    // The wall component is a polygon
    else if (polygon != nullptr) {
        // Adjust the coordinates to match CUGL
        y = _mapHeight - y;
        
        // Gather all of the vertices scaled to Box2D coordinates
        vector<Vec2> vertices;
        shared_ptr<JsonValue> points = json->get(POLYGON_FIELD);
        for (int ii = 0; ii < points->size(); ii++) {
            shared_ptr<JsonValue> vertex = points->get(ii);
            float localx = vertex->getFloat(X_FIELD) / _tileSize;
            float localy = -vertex->getFloat(Y_FIELD) / _tileSize;
            vertices.push_back(Vec2(localx,localy));
        }
                
        // Create a path in the counter clockwise direction, give up if not valid
        Path2 path(vertices);
        if (path.orientation() == 0) return;
        else if (path.orientation() != -1) path.reverse();
        
        // Create a polygon from that path
        EarclipTriangulator triangulator;
        triangulator.set(path);
        triangulator.calculate();
        Poly2 poly = triangulator.getPolygon();

        // Anchor the node on the relative position of the first node
        Rect bounds = poly.getBounds();
        Vec2 range(bounds.getMaxX() + abs(bounds.getMinX()),
                   bounds.getMaxY() + abs(bounds.getMinY()));
        Vec2 anchor(abs(bounds.getMinX()) / range.x,
                    abs(bounds.getMinY()) / range.y);
        
        // Make the wall and the node from the polygon
        wall = physics2::PolygonObstacle::alloc(poly);
        node = scene2::PolygonNode::allocWithPoly(poly);
        node->setAnchor(anchor);
        node->setPosition(x * scale, y * scale);
    }
    
    // The wall component is a rectangle
    else {
        // Flip the y coordinate
        y = _mapHeight - height - y;
        
        // Make a wall and a corresponding node from a polygon
        // TODO: Maybe make this a model or a type of ObstacleModel
        Poly2 poly = PolyFactory().makeRect(Vec2::ZERO, Vec2(width, height));
        wall = physics2::PolygonObstacle::alloc(poly);
        node = scene2::PolygonNode::allocWithPoly(poly);
        node->setPosition((x + width / 2) * scale, (y + height / 2) * scale);
    }
    
    // Add the wall to the world
    wall->setDebugScene(_debugnode);
    wall->setDebugColor(Color4::RED);
    _world->addObstacle(wall);
    
    // Set the position afterwards in case there's something weird with bounds
    wall->setPosition(x, y);
    
    // Add the node to the world node
    node->setScale(scale);
    node->setColor(Color4::GRAY);
    _worldnode->addChild(node);

}

/**
 * Initializes a single trap
 */
void GameModel::initTrap(int trapID, Vec2 center, float scale,
                         const std::shared_ptr<cugl::AssetManager>& assets) {
    // TODO: Change this to be JSON loading
    
    // Create hard-coded example trap
    shared_ptr<TrapModel> trap = std::make_shared<TrapModel>();
    
    // Create the parameters to create a trap
    std::shared_ptr<cugl::physics2::SimpleObstacle> thiefEffectArea = physics2::WheelObstacle::alloc(Vec2::ZERO, 5);
    std::shared_ptr<cugl::physics2::SimpleObstacle> copEffectArea = physics2::WheelObstacle::alloc(Vec2::ZERO, 5);
    std::shared_ptr<cugl::physics2::SimpleObstacle> triggerArea = physics2::WheelObstacle::alloc(Vec2::ZERO, 3);
    std::shared_ptr<cugl::Vec2> triggerPosition = make_shared<cugl::Vec2>(center);
    bool copSolid = true;
    bool thiefSolid = false;
    int numUses = 1;
    float lingerDur = 0.3;
    std::shared_ptr<cugl::Affine2> thiefVelMod = make_shared<cugl::Affine2>(1, 0, 0, 1, 0, 0);
    std::shared_ptr<cugl::Affine2> copVelMod = make_shared<cugl::Affine2>(1, 0, 0, 1, 0, 0);

    // Initialize a trap
    trap->init(trapID,
                thiefEffectArea, copEffectArea,
                triggerArea,
                triggerPosition,
                copSolid, thiefSolid,
                numUses,
                lingerDur,
                thiefVelMod, copVelMod);
    
    // Configure physics
    _world->addObstacle(thiefEffectArea);
    _world->addObstacle(copEffectArea);
    _world->addObstacle(triggerArea);
    thiefEffectArea->setPosition(center);
    copEffectArea->setPosition(center);
    triggerArea->setPosition(center);
    triggerArea->setSensor(true);
    thiefEffectArea->setSensor(true);
    copEffectArea->setSensor(true);
    
    // Set the appropriate visual elements
    trap->setAssets(scale, _worldnode, assets, TrapModel::MopBucket);
    trap->setDebugScene(_debugnode);
    
    // Add the trap to the vector of traps
    _traps.push_back(trap);
}

/**
 * Initializes the border for the game
 */
void GameModel::initBorder(float scale) {
    Rect bounds = _world->getBounds();
    for (int i = -1; i < 2; i ++) {
        for (int j = -1; j < 2; j ++) {
            // Don't put a border in the middle
            if (i == 0 && j == 0) continue;
                
            // Get the positioning and sizing of the borders
            float x = bounds.origin.x + i * _mapWidth;
            float y = bounds.origin.y + j * _mapHeight;
            float width = _mapWidth;
            float height = _mapHeight;
            
            // Create the appropriate wall and node using the rectangle polygon
            Rect rect = Rect(x, y, width, height);
            Poly2 poly = PolyFactory().makeRect(rect);
            shared_ptr<physics2::PolygonObstacle> wall = physics2::PolygonObstacle::alloc(poly);
            shared_ptr<scene2::PolygonNode> node = scene2::PolygonNode::allocWithPoly(poly);
            
            // Add the wall to the world
            wall->setDebugScene(_debugnode);
            wall->setDebugColor(Color4::RED);
            _world->addObstacle(wall);
            
            // Add the node to the world node
            node->setScale(scale);
            node->setColor(Color4::GRAY);
            node->setPosition((x + width / 2) * scale, (y + height / 2) * scale);
            _worldnode->addChild(node);
        }
    }
    
}
