//
//  LCMPGameModel.cpp
//  Low Control Mall Patrol
//
//  Author: Kevin Games
//  Version: 2/20/22
//

#include "LCMPGameModel.h"
#include "LCMPConstants.h"
#include <map>

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
                     float scale, const std::string& file,
                     std::shared_ptr<cugl::scene2::ActionManager>& actions) {
    _world = world;
    _worldnode = worldnode;
    _debugnode = debugnode;
    _gameover = false;
    
    _actions = actions;
    std::shared_ptr<JsonReader> reader = JsonReader::allocWithAsset(file);
    std::shared_ptr<JsonValue> json = reader->readJson();

    //init the map that converts Json Strings into Json Values
    constantsMap["activated"] = ACTIVATED; 
    constantsMap["copCollide"] = COP_COLLIDE;
    constantsMap["copEffect"] = COP_EFFECT;
    constantsMap["copLingerDuration"] = COP_LINGER_DURATION;
    constantsMap["copLingerEffect"] = COP_LINGER_EFFECT;
    constantsMap["effectArea"] = EFFECT_AREA;
    constantsMap["numUsages"] = NUM_USAGES;
    constantsMap["thiefCollide"] = THIEF_COLLIDE;
    constantsMap["thiefEffect"] = THIEF_EFFECT;
    constantsMap["thiefLingerDuration"] = THIEF_LINGER_DURATION;
    constantsMap["thiefLingerEffect"] = THIEF_LINGER_EFFECT;
    constantsMap["triggerArea"] = TRIGGER_AREA;
    constantsMap["triggerDeactivationArea"] = TRIGGER_DEACTIVATION_AREA;

    constantsMap["Escalator"] = ESCALATOR;
    constantsMap["Teleport"] = TELEPORT;
    constantsMap["Stairs"] = STAIRS;
    constantsMap["Velocity Modifier"] = VELOCITY_MODIFIER;

    constantsMap["NULL"] = NIL;
    
    _mapWidth = json->getFloat(WIDTH_FIELD);
    _mapHeight = json->getFloat(HEIGHT_FIELD);
    _tileSize = json->getFloat(T_SIZE_FIELD);
    
    std::shared_ptr<JsonValue> layers = json->get(LAYERS_FIELD);
    std::shared_ptr<JsonValue> walls = layers->get(WALLS_FIELD)->get(OBJECTS_FIELD);
    std::shared_ptr<JsonValue> copsSpawn = layers->get(COPS_FIELD)->get(OBJECTS_FIELD);
    std::shared_ptr<JsonValue> thiefSpawn = layers->get(THIEF_FIELD)->get(OBJECTS_FIELD);
    std::shared_ptr<JsonValue> traps = layers->get(TRAPS_FIELD)->get(OBJECTS_FIELD);
    
    // Initialize thief
    initThief(scale, thiefSpawn, assets, _actions);
    
    // Initialize cops
    for (int i = 0; i < 4; i++) initCop(i, scale, copsSpawn, assets, _actions);
    
    // Initialize walls
    for (int i = 0; i < walls->size(); i++) initWall(walls->get(i), scale);
    
    // Initialize traps
    // TODO: Make this JSON Reading
    //Vec2 traps[] = { Vec2(20, 30), Vec2(50, 30), Vec2(80, 30) };

    vector<shared_ptr<JsonValue>> trapsJson = vector<shared_ptr<JsonValue>>();
    vector<shared_ptr<JsonValue>> trapsObstaclesJson = vector<shared_ptr<JsonValue>>();

    for (int i = 0; i < traps->size(); i++) {

        shared_ptr<JsonValue> trap = traps->get(i); // Will either be a trap or an obstacle within the trap.

        bool point = trap->getBool(POINT_FIELD);

        if (point) {
            trapsJson.push_back(trap);
        }
        else {
            trapsObstaclesJson.push_back(trap);
        }

    }
    map<int, ObstacleNode_x_Y_struct> obstacleMap;
    map<int, ObstacleNode_x_Y_struct> obstacleMap2; // This is extremely scuff but we're not sure how to copy an obstacle so we are making all the obstacles twice.
    for (int i = 0; i < trapsObstaclesJson.size(); i++) { // On the bright side it gets only called once a game

        ObstacleNode_x_Y_struct trapObstacle = readJsonShape(trapsObstaclesJson.at(i), scale);
        obstacleMap.insert(pair<int, ObstacleNode_x_Y_struct>(trapsObstaclesJson.at(i)->getInt(ID_FIELD), trapObstacle));

        // Tony: I wanna cry... Omg I'm gonna cry
        ObstacleNode_x_Y_struct trapObstacle2 = readJsonShape(trapsObstaclesJson.at(i), scale);
        obstacleMap2.insert(pair<int, ObstacleNode_x_Y_struct>(trapsObstaclesJson.at(i)->getInt(ID_FIELD), trapObstacle2));
        //obstacleMap[trapsObstaclesJson.at(i)->getInt(ID_FIELD)] = trapObstacle.obstacle;

    }


    for (int i = 0; i < trapsJson.size(); i++) initTrap(i, trapsJson[i], obstacleMap, obstacleMap2, scale, assets);
    
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
    _thief->playAnimation();
    
    // Update all of the cops
    for (auto entry = _cops.begin(); entry != _cops.end(); entry++) {
        entry->second->update(timestep);
        entry->second->playAnimation();
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
void GameModel::updateCop(Vec2 acceleration, Vec2 thiefPosition, int copID, float timestep) {
    shared_ptr<CopModel> cop = _cops[copID];
    if (cop->getTackling()) cop->applyTackle(timestep, thiefPosition);
    else cop->applyForce(acceleration);
}

/**
 * Updates the position and velocity of the thief
 */
void GameModel::updateThief(cugl::Vec2 position, cugl::Vec2 velocity, cugl::Vec2 force) {
    _thief->applyNetwork(position, velocity, force);
}
cugl::Vec2 _tackleDirection;
cugl::Vec2 _tacklePosition;
float _tackleTime;
bool _tackling;
bool _caughtThief;
bool _tackleSuccessful;

/**
 * Updates the position and velocity of a cop
 */
void GameModel::updateCop(cugl::Vec2 position, cugl::Vec2 velocity,
                          cugl::Vec2 force, cugl::Vec2 tackleDirection,
                          cugl::Vec2 tacklePosition,
                          float tackleTime,
                          bool tackling,
                          bool caughtThief,
                          bool tackleSuccessful,
                          int copID) {
    shared_ptr<CopModel> cop = _cops[copID];
    cop->applyNetwork(position, velocity, force, tackleDirection, tacklePosition,
                      tackleTime, tackling, caughtThief, tackleSuccessful);
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
                          const std::shared_ptr<cugl::AssetManager>& assets,
                          std::shared_ptr<cugl::scene2::ActionManager>& actions) {
    // Create thief node
    std::shared_ptr<scene2::SceneNode> thiefNode = scene2::SceneNode::alloc();
    thiefNode->setAnchor(Vec2::ANCHOR_CENTER);
    _worldnode->addChildWithName(thiefNode, "thief");
    
    // Create thief
    _thief = std::make_shared<ThiefModel>();
    _thief->init(scale, thiefNode, assets, actions);
    _thief->setDebugScene(_debugnode);
    _thief->setCollisionSound(THIEF_COLLISION_SFX);
    _thief->setObstacleSound(OBJ_COLLISION_SFX);
    _world->addObstacle(_thief);
    
    // Position thief afterwards to not have to deal with changing world size
    _thief->setPosition(Vec2(spawn->get(0)->getFloat(X_FIELD) / _tileSize,
                             _mapHeight - spawn->get(0)->getFloat(Y_FIELD) / _tileSize));
}

void GameModel::initCop(int copID, float scale,
                        const std::shared_ptr<JsonValue>& spawns,
                        const std::shared_ptr<cugl::AssetManager>& assets,
                        std::shared_ptr<cugl::scene2::ActionManager>& actions) {
    // Create cop node
    std::shared_ptr<scene2::SceneNode> copNode = scene2::SceneNode::alloc();
    copNode->setAnchor(Vec2::ANCHOR_CENTER);
    _worldnode->addChildWithName(copNode, "cop" + to_string(copID));
    
    // Create cop
    std::shared_ptr<CopModel> cop = std::make_shared<CopModel>();
    cop->init(copID, scale, copNode, assets, actions);
    cop->setDebugScene(_debugnode);
    cop->setCollisionSound(COP_COLLISION_SFX);
    cop->setObstacleSound(OBJ_COLLISION_SFX);
    _world->addObstacle(cop);
    
    // Position cop afterwards to not have to deal with changing world size
    
    std::shared_ptr<JsonValue> spawn = spawns->get(copID);
    cop->setPosition(Vec2(spawn->getFloat(X_FIELD) / _tileSize,
                          _mapHeight - spawn->getFloat(Y_FIELD) / _tileSize));
    
    // Add the cop to the mapping of cops
    _cops[copID] = cop;
}

/**
 Reads PolygonObstacle and PolygonNode from Tiled Json object.
 Does not add either to the world/debugscene or assign color/texture, and obstacle position is 0,0.
 */
GameModel::ObstacleNode_x_Y_struct GameModel::readJsonShape(const shared_ptr<JsonValue>& json, float scale){
    std::shared_ptr<JsonValue> polygon = json->get(POLYGON_FIELD);
    bool ellipse = json->getBool(ELLIPSE_FIELD);
    float x = json->getFloat(X_FIELD) / _tileSize;
    float y = json->getFloat(Y_FIELD) / _tileSize;
    float width = json->getFloat(WIDTH_FIELD) / _tileSize;
    float height = json->getFloat(HEIGHT_FIELD) / _tileSize;
    
    // We want to populate the following obstacle and node
    shared_ptr<physics2::PolygonObstacle> obstacle;
    shared_ptr<scene2::PolygonNode> node;
    
    // The wall component is an ellipse
    if (ellipse) {
        // Adjust the coordinates to match CUGL
        x = x + width / 2;
        y = _mapHeight - (height / 2) - y;
        
        // Make a wall and a corresponding node from a polygon
        // TODO: Maybe make this a model or a type of ObstacleModel
        Poly2 poly = PolyFactory().makeEllipse(Vec2::ZERO, Vec2(width,height));
        obstacle = physics2::PolygonObstacle::alloc(poly);
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
        if (path.orientation() == 0) throw runtime_error("non-closed path");
        if (path.orientation() != -1) path.reverse();
        
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
        obstacle = physics2::PolygonObstacle::alloc(poly);
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
        obstacle = physics2::PolygonObstacle::alloc(poly);
        node = scene2::PolygonNode::allocWithPoly(poly);
        node->setPosition((x + width / 2) * scale, (y + height / 2) * scale);
    }

    node->setScale(scale);
    
    GameModel::ObstacleNode_x_Y_struct o = {obstacle, node, x, y};
    return o;
}

/**
 * Initializes a single wall
 */
void GameModel::initWall(const std::shared_ptr<JsonValue>& json, float scale) {
//    float x = json->getFloat(X_FIELD) / _tileSize;
//    float y = json->getFloat(Y_FIELD) / _tileSize;
    
    auto shape = readJsonShape(json, scale);
    
    auto wall = shape.obstacle;
    auto node = shape.node;
    float x = shape.x;
    float y = shape.y;
    
    // Add the wall to the world
    wall->setDebugScene(_debugnode);
    wall->setDebugColor(Color4::RED);
    _world->addObstacle(wall);
    
    // Set the position afterwards in case there's something weird with bounds
    wall->setPosition(x, y);
    
    // Add the node to the world node
    node->setColor(Color4::GRAY);
    _worldnode->addChild(node);

}


/**
 * Initializes a single trap
 */
void GameModel::initTrap(int trapID,
                         const std::shared_ptr<cugl::JsonValue>& json,
                         const map<int, ObstacleNode_x_Y_struct>& map1,
                         const map<int, ObstacleNode_x_Y_struct>& map2,
                         float scale,
                         const std::shared_ptr<cugl::AssetManager>& assets) {
    
    // Create the trap from the json

    //init instances

    shared_ptr<TrapModel> trap = std::make_shared<TrapModel>();
    shared_ptr<TrapModel::Effect> copEffect = std::make_shared<TrapModel::Effect>();
    shared_ptr<TrapModel::Effect> thiefEffect = std::make_shared<TrapModel::Effect>();
    shared_ptr<TrapModel::Effect> copLingerEffect = std::make_shared<TrapModel::Effect>();
    shared_ptr<TrapModel::Effect> thiefLingerEffect = std::make_shared<TrapModel::Effect>();

    std::shared_ptr<cugl::JsonValue> properties = json->get(PROPERTIES_FIELD);
    vector<std::shared_ptr<cugl::JsonValue>> children = properties->children();

    bool activated = false;
    bool copCollide = false;
    bool thiefCollide = false;
    int effectObjectId = -1;
    int triggerObjectId = -1;
    int deactivationObjectId = -1;
    int numUses = -1;
    int copEffectLingerDur = 0;
    int thiefEffectLingerDur = 0;

    //CULog("%s", children.at(3)->get(NAME_FIELD)->asString());
    // read in the JSON values and match it to the proper property
    for (int i = 0; i < children.size(); i++) {
        std::shared_ptr<cugl::JsonValue> elem = children.at(i);

        switch (constantsMap[elem->getString(NAME_FIELD, "NULL")])
        {
        

        case ACTIVATED:
            activated = elem->getBool(VALUE_FIELD);
            break;

        case COP_COLLIDE:
            copCollide = elem->getBool(VALUE_FIELD);
            break;
        
        case COP_EFFECT:
            copEffect = readJsonEffect(elem);
            break;
        
        case COP_LINGER_DURATION:
            copEffectLingerDur = elem->getFloat(VALUE_FIELD);
            break;

        case COP_LINGER_EFFECT:
            copLingerEffect = readJsonEffect(elem);
            break;

        case EFFECT_AREA:
            effectObjectId = elem->getInt(VALUE_FIELD);
            break;

        case NUM_USAGES:
            numUses = elem->getInt(VALUE_FIELD);
            break;

        case THIEF_COLLIDE:
            thiefCollide = elem->getBool(VALUE_FIELD);
            break;

        case THIEF_EFFECT:
            thiefEffect = readJsonEffect(elem);
            break;

        case THIEF_LINGER_DURATION:
            thiefEffectLingerDur = elem->getFloat(VALUE_FIELD);
            break;

        case THIEF_LINGER_EFFECT:
            thiefLingerEffect = readJsonEffect(elem);
            break;

        case TRIGGER_AREA:
            triggerObjectId = elem->getInt(VALUE_FIELD);
            break;

        case TRIGGER_DEACTIVATION_AREA:
            deactivationObjectId = elem->getInt(VALUE_FIELD);
            break;

        case NIL:
        default:
            break;
        }
    }


    shared_ptr<cugl::physics2::PolygonObstacle> thiefEffectArea = map1.at(effectObjectId).obstacle;
    shared_ptr<cugl::physics2::PolygonObstacle> copEffectArea = map2.at(effectObjectId).obstacle;
    shared_ptr<cugl::physics2::PolygonObstacle> triggerArea = map1.at(triggerObjectId).obstacle;
    shared_ptr<cugl::physics2::PolygonObstacle> deactivationArea = map2.at(deactivationObjectId).obstacle;

    std::shared_ptr<cugl::Vec2> triggerPosition = make_shared<cugl::Vec2>(json->getInt(X_FIELD), json->getInt(Y_FIELD));


    // Create the parameters to create a trap

    //std::shared_ptr<cugl::Vec2> triggerPosition = make_shared<cugl::Vec2>(center);
    //bool copSolid = true;
    //bool thiefSolid = false;
    //int numUses = 1;
    //float lingerDur = 0.3;
    //std::shared_ptr<cugl::Affine2> thiefVelMod = make_shared<cugl::Affine2>(1, 0, 0, 1, 0, 0);
    //std::shared_ptr<cugl::Affine2> copVelMod = make_shared<cugl::Affine2>(1, 0, 0, 1, 0, 0);

    // Initialize a trap

    trap->init(trapID,
                thiefEffectArea, copEffectArea,
                triggerArea, deactivationArea,
                triggerPosition,
                copCollide, thiefCollide,
                numUses,
                copEffectLingerDur,
                thiefEffectLingerDur,
                copEffect,
                thiefEffect,
                copLingerEffect,
                thiefLingerEffect);
    
    // Configure physics
    _world->addObstacle(thiefEffectArea);
    _world->addObstacle(copEffectArea);
    _world->addObstacle(triggerArea);
    thiefEffectArea->setPosition(Vec2(map1.at(effectObjectId).x, map1.at(effectObjectId).y));
    copEffectArea->setPosition(Vec2(map2.at(effectObjectId).x, map2.at(effectObjectId).y));
    triggerArea->setPosition(Vec2(map1.at(triggerObjectId).x, map1.at(triggerObjectId).y));
    deactivationArea->setPosition(Vec2(map2.at(deactivationObjectId).x, map2.at(deactivationObjectId).y));
    deactivationArea->setSensor(true);
    triggerArea->setSensor(true);
    thiefEffectArea->setSensor(true);
    copEffectArea->setSensor(true);
    
    // TODO: fix this once assets are written
    trap->setAssets(scale, _worldnode, assets, TrapModel::Teleport);
    trap->setDebugScene(_debugnode);
    
    // Add the trap to the vector of traps
    _traps.push_back(trap);
}

/**
 * Takes a JsonValue pointing towards an Effect object and parses it
 */
shared_ptr<TrapModel::Effect> GameModel::readJsonEffect(shared_ptr<JsonValue> effect) {
    std::shared_ptr<cugl::Vec2> effectVec = make_shared<cugl::Vec2>(0, 0);
    TrapModel::TrapType effectType = TrapModel::TrapType::Teleport;
    float effectx = 0.0;
    float effecty = 0.0;

    if (effect->get(VALUE_FIELD)->children().size() > 0) {
        switch (constantsMap[effect->get(VALUE_FIELD)->getString(TRAP_TYPE, "NULL")])
        {

        case ESCALATOR:
            effectType = TrapModel::TrapType::Moving_Platform;
            effectx = effect->get(VALUE_FIELD)->get(ESCALATOR_VELOCITY)->getFloat(X_FIELD, 0);
            effecty = effect->get(VALUE_FIELD)->get(ESCALATOR_VELOCITY)->getFloat(Y_FIELD, 0);
            effectVec = make_shared<cugl::Vec2>(effectx, effecty);
            break;

        case TELEPORT:
            effectType = TrapModel::TrapType::Teleport;
            effectx = effect->get(VALUE_FIELD)->get(TELEPORT_LOCATION)->getFloat(X_FIELD, 0) / _tileSize;
            effecty = _mapHeight - effect->get(VALUE_FIELD)->get(TELEPORT_LOCATION)->getFloat(Y_FIELD, 0) / _tileSize;
            effectVec = make_shared<cugl::Vec2>(effectx, effecty);
            break;

        case STAIRS:
            effectType = TrapModel::TrapType::Directional_VelMod;
            effectx = effect->get(VALUE_FIELD)->get(STAIRCASE_VELOCITY)->getFloat(X_FIELD, 0);
            effecty = effect->get(VALUE_FIELD)->get(STAIRCASE_VELOCITY)->getFloat(Y_FIELD, 0);
            effectVec = make_shared<cugl::Vec2>(effectx, effecty);
            break;

        case VELOCITY_MODIFIER:
            effectType = TrapModel::TrapType::VelMod;
            effectx = effect->get(VALUE_FIELD)->get(SPEED_MODIFIER)->getFloat(X_FIELD, 0);
            effecty = effect->get(VALUE_FIELD)->get(SPEED_MODIFIER)->getFloat(Y_FIELD, 0);
            effectVec = make_shared<cugl::Vec2>(effectx, effecty);
            break;

        case NIL:
        default:
            break;
        }
    }

    shared_ptr<TrapModel::Effect> temp = make_shared<TrapModel::Effect>();
    temp->init(effectType, effectVec);
    return temp;
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
