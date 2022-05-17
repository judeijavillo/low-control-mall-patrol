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

#include <time.h>

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
                     std::shared_ptr<cugl::scene2::SceneNode>& floornode,
                     std::shared_ptr<cugl::scene2::SceneNode>& worldnode,
                     std::shared_ptr<cugl::scene2::SceneNode>& debugnode,
                     const std::shared_ptr<cugl::AssetManager>& assets,
                     float scale, const std::string& file,
                     std::shared_ptr<cugl::scene2::ActionManager>& actions,
                     string skinKey) {
    
    time_t timer = time(NULL);
    struct tm * timeinfo = localtime (&timer);
//    CULog("starting initialization %s", asctime(timeinfo));
    
    _world = world;
    _floornode = floornode;
    _worldnode = worldnode;
    _debugnode = debugnode;
    _gameover = false;
    _skinKey = skinKey;
    
    _actions = actions;
    CULog("opening file %s", file.data());
    std::shared_ptr<JsonReader> reader = JsonReader::allocWithAsset(file);
    std::shared_ptr<JsonValue> json = reader->readJson();
    
    _mapWidth = json->getFloat(WIDTH_FIELD);
    _mapHeight = json->getFloat(HEIGHT_FIELD);
    _tileSize = json->getFloat(T_SIZE_FIELD);
    
    reader = JsonReader::allocWithAsset(PROPS_FILE);
    shared_ptr<JsonValue> propTileset = reader->readJson();
    map<int,GameModel::TileData> idToTileData = buildTileDataMap(propTileset, scale);
    
    timer = time(NULL);
    timeinfo = localtime (&timer);
//    CULog("done reading tileset %s", asctime(timeinfo));
    
//    for(auto it = idToTileData.begin(); it != idToTileData.end(); it++){
//        int key = it->first;
//        TileData val = it->second;
//        CULog("%d:\t%s", key, val.assetName.data());
//    }

    //init the map that converts Json Strings into Json Values

    //CULog("-4");
    constantsMap["activated"] = ACTIVATED;
    constantsMap["collisionSound"] = COLLISION_SOUND;
    constantsMap["copCollide"] = COP_COLLIDE;
    constantsMap["copEffect"] = COP_EFFECT;
    constantsMap["copLingerDuration"] = COP_LINGER_DURATION;
    constantsMap["copLingerEffect"] = COP_LINGER_EFFECT;
    constantsMap["effectArea"] = EFFECT_AREA;
    constantsMap["idleActivatedAnimation"] = IDLE_ACTIVATED_ANIMATION,
    constantsMap["idleDeactivatedAnimation"] = IDLE_DEACTIVATED_ANIMATION,
    constantsMap["numUsages"] = NUM_USAGES;
    constantsMap["textureActivationTrigger"] = TEXTURE_ACTIVATION_TRIGGER;
    constantsMap["textureAsset"] = TEXTURE_ASSET;
    constantsMap["textureDeactivationTrigger"] = TEXTURE_DEACTIVATION_TRIGGER;
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
    
    std::shared_ptr<JsonValue> layers = json->get(LAYERS_FIELD);
    std::shared_ptr<JsonValue> props = layers->get(PROPS_FIELD)->get(OBJECTS_FIELD);
    std::shared_ptr<JsonValue> walls = layers->get(WALLS_FIELD)->get(OBJECTS_FIELD);
    std::shared_ptr<JsonValue> copsSpawn = layers->get(COPS_FIELD)->get(OBJECTS_FIELD);
    std::shared_ptr<JsonValue> thiefSpawn = layers->get(THIEF_FIELD)->get(OBJECTS_FIELD);
    std::shared_ptr<JsonValue> traps = layers->get(TRAPS_FIELD)->get(OBJECTS_FIELD);
    
    // Initialize backdrop

    //float backdropScale = (scale / _tileSize) * 1.12; // 1.12 is the scale of the floor plan relative to the tiled map
    float backdropScale = (scale / _tileSize);

    initBackdrop(backdropScale, 5, 5, assets);

    // Initialize thief
    initThief(scale, thiefSpawn, assets, _actions);
    
    // Initialize cops
    for (int i = 0; i < 4; i++) initCop(i, scale, copsSpawn, assets, _actions);

    _obstacles = std::vector<std::shared_ptr<physics2::PolygonObstacle>>();
    
    // Initialize walls
    for (int i = 0; i < walls->size(); i++) initWall(walls->get(i), scale);
    
    // Initialize props
    int prop_firstGid = 0;
    auto tilesets = json->get("tilesets");
    for(int i = 0; i < tilesets->size(); i++){
        //TODO: make these constants
        if(tilesets->get(i)->getString("source") == "PropsAndTraps.tsj"){
            prop_firstGid = tilesets->get(i)->getInt("firstgid");
            break;
        }
    }
    if(prop_firstGid > 0){
//    for (int i = 0; i < props->size(); i++) initProp(props->get(i), prop_firstGid, propTileset, assets, scale);
        initProps(props, prop_firstGid, idToTileData, assets, scale);
    }
    
    timer = time(NULL);
    timeinfo = localtime (&timer);
//    CULog("done initializing backdrop, players, walls, and props %s", asctime(timeinfo));

    obstaclesInGrid = map<int, set<shared_ptr<physics2::PolygonObstacle>>>();

    for(auto ob : _obstacles){
        auto boundingbox = ob->getPolygon().getBounds();
        for(int i = (boundingbox.getMinX()+ob->getX())/GRID_SIZE-1; i <= (boundingbox.getMaxX()+ob->getX())/GRID_SIZE+2; i++)
            for(int j = (boundingbox.getMinY()+ob->getY())/GRID_SIZE-1; j <= (boundingbox.getMaxY()+ob->getY())/GRID_SIZE+2; j++){
                obstaclesInGrid[hash(i,j)].insert(ob);
            }
//        ob->setEnabled(false);
    }
//    shared_ptr<PlayerModel> player = _thief;
//    auto gridcell = player->getPosition();
//    for(int d1 = -1; d1 <= 1; d1++)
//        for(int d2 = -1; d2 <= 1; d2++){
//            CULog("enabling for hash %d", hash(gridcell+Vec2(d1,d2)));
//            for(auto ob : obstaclesInGrid[hash(gridcell+Vec2(d1,d2))]){
//                ob->setEnabled(true);
//                CULog("enabled obstacle for thief");
//            }
//        }
//    for(auto it = _cops.begin(); it != _cops.end(); it++){
//        player = it->second;
//        gridcell = player->getPosition();
//        for(int d1 = -1; d1 <= 1; d1++)
//            for(int d2 = -1; d2 <= 1; d2++)
//                for(auto ob : obstaclesInGrid[hash(gridcell+Vec2(d1,d2))]){
//                    ob->setEnabled(true);
//                    CULog("enabled obstacle for cop");
//                }
//    }
    
    // Initialize traps

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
    //CULog("-1");

    map<int, ObstacleNode_x_Y_Gid_struct> obstacleMap;
    map<int, ObstacleNode_x_Y_Gid_struct> obstacleMap2; // This is extremely scuff but we're not sure how to copy an obstacle so we are making all the obstacles twice.
    for (int i = 0; i < trapsObstaclesJson.size(); i++) { // On the bright side it gets only called once a game
        
//        CULog("calling readJsonShape twice in traps");
        
        ObstacleNode_x_Y_Gid_struct trapObstacle = readJsonShape(trapsObstaclesJson.at(i), scale);
        obstacleMap.insert(pair<int, ObstacleNode_x_Y_Gid_struct>(trapsObstaclesJson.at(i)->getInt(ID_FIELD), trapObstacle));

        // Tony: I wanna cry... Omg I'm gonna cry
        ObstacleNode_x_Y_Gid_struct trapObstacle2 = readJsonShape(trapsObstaclesJson.at(i), scale);
        obstacleMap2.insert(pair<int, ObstacleNode_x_Y_Gid_struct>(trapsObstaclesJson.at(i)->getInt(ID_FIELD), trapObstacle2));
        //obstacleMap[trapsObstaclesJson.at(i)->getInt(ID_FIELD)] = trapObstacle.obstacle;

    }
    
    timer = time(NULL);
    timeinfo = localtime (&timer);
//    CULog("done reading trap info %s", asctime(timeinfo));

    for (int i = 0; i < trapsJson.size(); i++) initTrap(i, trapsJson[i], obstacleMap, obstacleMap2, idToTileData, prop_firstGid,scale, assets);
    
    timer = time(NULL);
    timeinfo = localtime (&timer);
//    CULog("done initializing traps %s", asctime(timeinfo));
    
    // Initialize borders
    initBorder(scale);
    
    timer = time(NULL);
    timeinfo = localtime (&timer);
//    CULog("done with initialization %s", asctime(timeinfo));
    
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
    
    for (auto ob : _obstacles)
        ob->setEnabled(false);
    
    shared_ptr<PlayerModel> player = _thief;
    auto gridcell = player->getPosition()/GRID_SIZE;
            for(auto ob : obstaclesInGrid[hash(gridcell)])
                ob->setEnabled(true);
//                CULog("enabled obstacle for thief");
        
    for(auto it = _cops.begin(); it != _cops.end(); it++){
        player = it->second;
        gridcell = player->getPosition()/GRID_SIZE;
                if(obstaclesInGrid[hash(gridcell)].size() > 0)
//                CULog("enabling %d obs for hash %d", obstaclesInGrid[hash(gridcell)].size(), hash(gridcell));
                for(auto ob : obstaclesInGrid[hash(gridcell)])
                    ob->setEnabled(true);

    }

    // update the traps
    updateTraps(timestep);
}

/**
 * Applies an acceleration to the thief (most likely for local updates)
 */
void GameModel::updateThief(cugl::Vec2 acceleration) {
    _thief->applyForce(acceleration);
//    CULog("Thief node pos: (%f, %f)", _thief->getNode()->getPositionX(), _thief->getNode()->getPositionY());
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

/**
 * Activates a trap
 */
void GameModel::deactivateTrap(int trapID) {
    _traps[trapID]->deactivate();
}

//  MARK: - Helpers

void GameModel::initBackdrop(float scale, int rows, int cols,
                             const shared_ptr<AssetManager>& assets) {

    // Create textured node to store texture for each map chunk
    std::shared_ptr<cugl::scene2::PolygonNode> chunkNode;

    Vec2 origin = Vec2(0.0f, 0.0f);

    // Cycle through all map chunks
    for (int y = 1; y <= cols; y++) {
        for (int x = 1; x <= rows; x++) {

            // Retrieve map chunk from assets
            string assetName = strtool::format("row-%d-column-%d", y, x);
            auto mapChunk = assets->get<Texture>(assetName);

            // Create textured node for chunk
            chunkNode = scene2::PolygonNode::allocWithTexture(mapChunk);
            chunkNode->setAnchor(0.0f, 0.0f);
            chunkNode->setScale(scale);
            chunkNode->setPositionX(origin.x + mapChunk->getWidth() * scale * (x - 1));
            chunkNode->setPositionY(origin.y + mapChunk->getHeight() * scale * (y - 1));
            _floornode->addChild(chunkNode);
        }
    }
}

void GameModel::initThief(float scale,
                          const shared_ptr<JsonValue>& spawn,
                          const shared_ptr<AssetManager>& assets,
                          shared_ptr<scene2::ActionManager>& actions) {
    // Create thief node
    std::shared_ptr<scene2::SceneNode> thiefNode = scene2::SceneNode::alloc();
    thiefNode->setAnchor(Vec2::ANCHOR_CENTER);
    _worldnode->addChildWithName(thiefNode, "thief");
    
    // Create thief
    _thief = std::make_shared<ThiefModel>();
    _thief->init(scale, thiefNode, assets, actions, _skinKey);
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
    cop->init(copID, scale, copNode, assets, actions, _skinKey);
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

map<int,GameModel::TileData> GameModel::buildTileDataMap(const shared_ptr<JsonValue>& propTileset, float scale){
    // build map from tile id to asset name
    map<int, GameModel::TileData> idToTileData {};
    auto tiles = propTileset->get("tiles");
    int tilecount = propTileset->getInt("tilecount");
    for(int i = 0; i < tilecount; i++){
        auto tile = tiles->get(i);
        int id = tile->get("id")->asInt();
        //CULog("%d", id);
        auto properties = tile->get("properties");
        // get value of property with matching names
        // the other way to do this is tony's travesty
        // and there's only 4 things so if/elif is fine
        string assetName = "";
        bool animated = false;
        int anim_rows = 0;
        int anim_cols = 0;
        for(int j = 0; j < properties->size(); j++){
            auto p = properties->get(j);
            if(p == nullptr) break;
            auto pname = p->getString("name");
            if(pname == "name") {
                assetName = p->getString("value");
            } else if(pname == "animated") {
                animated = p->getBool("value");
            } else if(pname == "anim_rows") {
                anim_rows = p->getInt("value");
            } else if(pname == "anim_cols") {
                anim_cols = p->getInt("value");
            }
        }
//        CULog("reading tile %d, name %s", id, assetName.data());
        vector<shared_ptr<ObstacleNode_x_Y_Gid_struct>> hitboxes;
        if (tile->get("objectgroup") != nullptr) {
            auto hitbox_jsons = tile->get("objectgroup")->get("objects");
            for(int j = 0; j < hitbox_jsons->size(); j++){
                auto hb_json = hitbox_jsons->get(j);
//                CULog("contents %s", hb_json->toString().data());
//                CULog("calling readJsonShape in buildTileDataMap");
                auto s = make_shared<ObstacleNode_x_Y_Gid_struct>(readJsonShape(hb_json, scale));
                hitboxes.push_back(s);
            }
        }
        
        idToTileData[id] = {assetName, hitboxes, animated, anim_rows, anim_cols};
//        CULog("%s has hitboxes %s", assetName.data(), hitboxes->toString().data());
        // this is correct
    }
    
    return idToTileData;
}


void GameModel::initProps(const shared_ptr<JsonValue>& props,
                          int props_firstgid,
                          map<int,GameModel::TileData> idToTileData,
                          const shared_ptr<AssetManager>& assets,
                          float scale){
    for(int i = 0; i < props->size(); i++) {
        auto prop = props->get(i);
        
        float x = prop->getFloat(X_FIELD) / _tileSize;
        float y = prop->getFloat(Y_FIELD) / _tileSize;
        float width = prop->getFloat(WIDTH_FIELD) / _tileSize;
        float height = prop->getFloat(HEIGHT_FIELD) / _tileSize;
        y = _mapHeight - y;
//        CULog("prop x,y %f %f", x,y);
//        CULog("prop dims %f %f", width, height);
        
        int gid = prop->getInt("gid");
        //TODO: incorporate rotation/reflection using tiled flags
        gid &= CLEAR_FLAGS_FILTER;
        int id = gid - props_firstgid;
        
        TileData data = idToTileData[id];
        
//        CULog("%d asset name %s", id, data.assetName.data());
        auto texture = assets->get<Texture>(data.assetName);
        //TODO: get this working
        Vec2 scale_ = Vec2(width/texture->getWidth(), height/texture->getHeight());
//        Vec2 scale_ = Vec2::ONE;
        // size of image in world in box2d units / size of texture in pixels
        // * pixels in world / pixels
        
        // add node to world
        shared_ptr<scene2::PolygonNode> node;
        if(data.animated){
            node = scene2::SpriteNode::alloc(texture, data.anim_rows, data.anim_cols);
//            CULog("prop is animated");
        } else {
            node = scene2::PolygonNode::allocWithTexture(texture);
//            CULog("prop is not animated");
        }
        node->setScale(scale_.x * scale, scale_.y * scale);
        _worldnode->addChild(node);
        node->setPosition((x + width / 2) * scale, (y + height / 2) * scale);
//        CULog("prop node position %f %f", (x + width / 2) * scale, (y + height / 2) * scale);
        
        // add hitboxes to world
        auto size = data.hitboxes.size();
        for(int j = 0; j < size; j++){
            auto shape = data.hitboxes[j];
            
            auto obstacle = scaleHitbox(shape, scale_, x, y, height);
//            obstacle->setDebugScene(_debugnode);
//            CULog("prop obst scaleHitbox call\n\t\tscale_ %f %f\n\t\tx %f\n\t\ty %f\n\t\theight %f",
//                  scale_.x, scale_.y,
//                  x, y,
//                  height);
//            CULog("prop obst position %f %f", obstacle->getX(), obstacle->getY());
            _obstacles.push_back(obstacle);
            obstacle->setEnabled(false);
        }
    }
}

std::shared_ptr<cugl::physics2::PolygonObstacle> GameModel::scaleHitbox(std::shared_ptr<GameModel::ObstacleNode_x_Y_Gid_struct> shape, Vec2 scale_, float x, float y, float height) {
    auto x_ = shape->x;
    auto y_ = shape->y;
    y_ -= _mapHeight;

    auto reference_obstacle = shape->obstacle;
    auto poly = Poly2(reference_obstacle->getPolygon());
    poly *= scale_ * _tileSize;

    auto obstacle = physics2::PolygonObstacle::alloc(poly);
    obstacle->setDebugScene(_debugnode);
//    obstacle->setDebugScene(nullptr);
    _world->addObstacle(obstacle);
    obstacle->setPosition(x_ * scale_.x * _tileSize + x,
        y_ * scale_.y * _tileSize + y
        + height);

    return obstacle;
}

/**
 Reads PolygonObstacle and PolygonNode from Tiled Json object.
 Does not add either to the world/debugscene or assign color/texture, and obstacle position is 0,0.
 */
GameModel::ObstacleNode_x_Y_Gid_struct GameModel::readJsonShape(const shared_ptr<JsonValue>& json, float scale){
    std::shared_ptr<JsonValue> polygon = json->get(POLYGON_FIELD);
    bool ellipse = json->getBool(ELLIPSE_FIELD);
    float x = json->getFloat(X_FIELD) / _tileSize;
    float y = json->getFloat(Y_FIELD) / _tileSize;
//    CULog("x,y %f %f", x*_tileSize, y*_tileSize);
    float width = json->getFloat(WIDTH_FIELD) / _tileSize;
    float height = json->getFloat(HEIGHT_FIELD) / _tileSize;
    int gid = json->getInt(GID_FIELD);
    
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
        Poly2 poly = PolyFactory(POLYFACTORY_TOLERANCE).makeEllipse(Vec2::ZERO, Vec2(width,height));
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
        Poly2 poly = PolyFactory(POLYFACTORY_TOLERANCE).makeRect(Vec2::ZERO, Vec2(width, height));
        obstacle = physics2::PolygonObstacle::alloc(poly);
        node = scene2::PolygonNode::allocWithPoly(poly);
        node->setPosition((x + width / 2) * scale, (y + height / 2) * scale);
    }

    node->setScale(scale);
    
    GameModel::ObstacleNode_x_Y_Gid_struct o = {obstacle, node, x, y, gid};
    return o;
}

/**
 * Initializes a single wall
 */
void GameModel::initWall(const std::shared_ptr<JsonValue>& json, float scale) {
//    float x = json->getFloat(X_FIELD) / _tileSize;
//    float y = json->getFloat(Y_FIELD) / _tileSize;
    
//    CULog("calling readJsonShape in initWall");
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

    
    _obstacles.push_back(wall);
    
    // Uncomment if you need to see the walls on the map
//    _worldnode->addChild(node);

}


/**
 * Initializes a single trap
 */
void GameModel::initTrap(int trapID,
                         const std::shared_ptr<cugl::JsonValue>& json,
                         const map<int, ObstacleNode_x_Y_Gid_struct>& map1,
                         const map<int, ObstacleNode_x_Y_Gid_struct>& map2,
                         const map<int, GameModel::TileData> idToTileData, const int prop_firstGid,
                         float scale,
                         const std::shared_ptr<cugl::AssetManager>& assets) {
    
    // Create the trap from the json

    //init variables

    //CULog("0");

    shared_ptr<TrapModel> trap = std::make_shared<TrapModel>();
    shared_ptr<TrapModel::Effect> copEffect = std::make_shared<TrapModel::Effect>();
    shared_ptr<TrapModel::Effect> thiefEffect = std::make_shared<TrapModel::Effect>();
    shared_ptr<TrapModel::Effect> copLingerEffect = std::make_shared<TrapModel::Effect>();
    shared_ptr<TrapModel::Effect> thiefLingerEffect = std::make_shared<TrapModel::Effect>();

    shared_ptr<Texture> activationTriggerTexture = std::make_shared<Texture>();
    shared_ptr<Texture> deactivationTriggerTexture = std::make_shared<Texture>();
    shared_ptr<Texture> assetTexture = std::make_shared<Texture>();

    shared_ptr<Vec2> activationTriggerTextureScale = std::make_shared<Vec2>();
    shared_ptr<Vec2> deactivationTriggerTextureScale = std::make_shared<Vec2>();
    shared_ptr<Vec2> assetTextureScale = std::make_shared<Vec2>();

    shared_ptr<cugl::physics2::PolygonObstacle> thiefEffectArea = std::make_shared<cugl::physics2::PolygonObstacle>();
    shared_ptr<cugl::physics2::PolygonObstacle> copEffectArea = std::make_shared<cugl::physics2::PolygonObstacle>();
    shared_ptr<cugl::physics2::PolygonObstacle> tempObst;

    std::shared_ptr<cugl::JsonValue> properties = json->get(PROPERTIES_FIELD);
    vector<std::shared_ptr<cugl::JsonValue>> children = properties->children();

    bool activated = false;
    bool copCollide = false;
    bool thiefCollide = false;
    bool idleActivatedAnimation = false;
    bool idleDeactivatedAnimation = false;
    //int effectObjectId = -1;
    float effectAreaX = 0;
    float effectAreaY = 0;
    int triggerObjectId = -1;
    int deactivationObjectId = -1;
    int numUses = -1;
    int copEffectLingerDur = 0;
    int thiefEffectLingerDur = 0;
    bool sfxOn = false;
    std::string sfxKey = "";

    TileData td;
    string assetName = "";
    bool animated = false;
    int anim_rows = 0;
    int anim_cols = 0;

    Vec2 scale_ = Vec2();
    
    float x = 0;
    float y = 0;
    
    float height = 0;
    float width = 0;

    //CULog("%s", children.at(3)->get(NAME_FIELD)->asString());
  
    // read in the JSON values and match it to the proper property
    for (int i = 0; i < children.size(); i++) {
        std::shared_ptr<cugl::JsonValue> elem = children.at(i);
        std::string name = elem->getString(NAME_FIELD, "NULL");
        switch (constantsMap[name])
        {

        case ACTIVATED:
            //CULog("in activated");
            activated = elem->getBool(VALUE_FIELD);
            //CULog("%d", activated);
            break;
                
        case COLLISION_SOUND:
            sfxKey = elem->getString(VALUE_FIELD);
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
            //effectObjectId = elem->getInt(VALUE_FIELD);
            break;

        case IDLE_ACTIVATED_ANIMATION:
            idleActivatedAnimation = elem->getBool(VALUE_FIELD);
            break;

        case IDLE_DEACTIVATED_ANIMATION:
            idleDeactivatedAnimation = elem->getBool(VALUE_FIELD);
            break;

        case NUM_USAGES:
            numUses = elem->getInt(VALUE_FIELD);
            break;

        case THIEF_COLLIDE:
            thiefCollide = elem->getBool(VALUE_FIELD);
            break;

        case TEXTURE_ACTIVATION_TRIGGER:
            activationTriggerTexture = assets->get<Texture>(elem->getString(VALUE_FIELD));
            break;

        case TEXTURE_ASSET:
            //CULog("%d", map1.at(elem->getInt(VALUE_FIELD)).gid);
            //auto fuck = map1.at(elem->getInt(VALUE_FIELD));

            td = idToTileData.at(map1.at(elem->getInt(VALUE_FIELD)).gid - prop_firstGid);
            animated = td.animated;
            anim_rows = td.anim_rows;
            anim_cols = td.anim_cols;
            assetName = td.assetName;

            assetTexture = assets->get<Texture>(assetName);

            scale_ = Vec2(
                          map1.at(elem->getInt(VALUE_FIELD)).obstacle->getSize().width * anim_cols /assetTexture->getWidth(),
                          map1.at(elem->getInt(VALUE_FIELD)).obstacle->getSize().height * anim_rows/ assetTexture->getHeight());

            tempObst = scaleHitbox(td.hitboxes[0], scale_, map1.at(elem->getInt(VALUE_FIELD)).x, map1.at(elem->getInt(VALUE_FIELD)).y + map1.at(elem->getInt(VALUE_FIELD)).obstacle->getSize().height, map1.at(elem->getInt(VALUE_FIELD)).obstacle->getSize().height);
                effectAreaX = tempObst->getX();
                effectAreaY = tempObst->getY();
                tempObst->setDebugScene(nullptr);
                _world->removeObstacle(&*tempObst);
                
                x = map1.at(elem->getInt(VALUE_FIELD)).x;
                y = map1.at(elem->getInt(VALUE_FIELD)).y + map1.at(elem->getInt(VALUE_FIELD)).obstacle->getSize().height;
                height = map1.at(elem->getInt(VALUE_FIELD)).obstacle->getSize().height;
                width = map1.at(elem->getInt(VALUE_FIELD)).obstacle->getSize().width;
//                CULog("trap x,y %f %f", x,y);
//                CULog("trap dims %f %f", width,height);
                
//                CULog("trap obst scaleHitbox call\n\t\tscale_ %f %f\n\t\tx %f\n\t\ty %f\n\t\theight %f",
//                      scale_.x, scale_.y,
//                      x, y,
//                      height);
//                CULog("trap obst position %f %f", effectAreaX, effectAreaY);
                
                thiefEffectArea = cugl::physics2::PolygonObstacle::alloc(tempObst->getPolygon());
//                thiefEffectArea->setDebugScene(_debugnode);
//                thiefEffectArea = physics2::PolygonObstacle::alloc(PolyFactory().makeRect(0,0,1,1));
            
            copEffectArea = cugl::physics2::PolygonObstacle::alloc(tempObst->getPolygon());
//                copEffectArea = physics2::PolygonObstacle::alloc(PolyFactory().makeRect(0,0,1,1));
//                copEffectArea->setDebugScene(_debugnode);



//            effectAreaX = (map1.at(elem->getInt(VALUE_FIELD)).x + map1.at(elem->getInt(VALUE_FIELD)).obstacle->getSize().width/2);
//            effectAreaY = (map1.at(elem->getInt(VALUE_FIELD)).y + map1.at(elem->getInt(VALUE_FIELD)).obstacle->getSize().height/2) + assetTexture->getHeight()*scale_.y;
                
                
//          CULog("scale_ %f %f", scale_.x, scale_.y);

            
            break;

        case TEXTURE_DEACTIVATION_TRIGGER:
            deactivationTriggerTexture = assets->get<Texture>(elem->getString(VALUE_FIELD));
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

    //CULog("1");

    //shared_ptr<cugl::physics2::PolygonObstacle> thiefEffectArea = map1.at(effectObjectId).obstacle;
    //shared_ptr<cugl::physics2::PolygonObstacle> copEffectArea = map2.at(effectObjectId).obstacle;
    shared_ptr<cugl::physics2::PolygonObstacle> triggerArea = map1.at(triggerObjectId).obstacle;
    shared_ptr<cugl::physics2::PolygonObstacle> deactivationArea = map2.at(deactivationObjectId).obstacle;

    std::shared_ptr<cugl::Vec2> triggerPosition = make_shared<cugl::Vec2>(json->getFloat(X_FIELD)/_tileSize, _mapHeight - json->getFloat(Y_FIELD)/_tileSize);


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
                activated,
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
                thiefLingerEffect,
                idleActivatedAnimation,
                idleDeactivatedAnimation,
                sfxOn,
                sfxKey);

    //CULog("2");
    
    // Configure physics
    _world->addObstacle(thiefEffectArea);
    _world->addObstacle(copEffectArea);
    _world->addObstacle(triggerArea);
    thiefEffectArea->setPosition(Vec2(effectAreaX, effectAreaY));
    copEffectArea->setPosition(Vec2(effectAreaX, effectAreaY));

    triggerArea->setPosition(Vec2(map1.at(triggerObjectId).x, map1.at(triggerObjectId).y));
    deactivationArea->setPosition(Vec2(map2.at(deactivationObjectId).x, map2.at(deactivationObjectId).y));

    deactivationArea->setSensor(true);
    triggerArea->setSensor(true);
    thiefEffectArea->setSensor(true);
    copEffectArea->setSensor(true);

    tuple <bool, int, int, std::string> assetInfo = make_tuple(animated, anim_rows, anim_cols, assetName);
    
    // TODO: fix this once assets are written


    trap->setAssets(Vec2(x,y), Vec2(width,height),
                    scale, scale_, _tileSize, _worldnode, assets,
                    activationTriggerTexture,deactivationTriggerTexture,
                    assetInfo);
    trap->setDebugScene(_debugnode);
    
    // Add the trap to the vector of traps
    _traps.push_back(trap);
    //CULog("3");
}

/**
 * Takes a JsonValue pointing towards an Effect object and parses it
 */
shared_ptr<TrapModel::Effect> GameModel::readJsonEffect(shared_ptr<JsonValue> effect) {
    std::shared_ptr<cugl::Vec2> effectVec = make_shared<cugl::Vec2>(0, 0);
    TrapModel::TrapType effectType = TrapModel::TrapType::Moving_Platform;
    float effectx = 0.0;
    float effecty = 0.0;

    if (effect->get(VALUE_FIELD)->children().size() > 0) {
        switch (constantsMap[effect->get(VALUE_FIELD)->getString(TRAP_TYPE, "NULL")])
        {

        case ESCALATOR:
            effectType = TrapModel::TrapType::Moving_Platform;
            if (effect->get(VALUE_FIELD)->get(ESCALATOR_VELOCITY) != nullptr) {
                effectx = effect->get(VALUE_FIELD)->get(ESCALATOR_VELOCITY)->getFloat(X_FIELD, 0);
                effecty = effect->get(VALUE_FIELD)->get(ESCALATOR_VELOCITY)->getFloat(Y_FIELD, 0);
            }
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
 * Checks all the traps and see if they are in the middle of activating. If so, increment their animation.
 */
void GameModel::updateTraps(float timestep) {
    for (int i = 0; i < _traps.size(); i++) {
        _traps[i]->updateTrap(timestep);
    }
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
            Poly2 poly = PolyFactory(POLYFACTORY_TOLERANCE).makeRect(rect);
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
