//
//  CUObstacleWorld.cpp
//  Cornell University Game Library (CUGL)
//
//  This module provides a wrapper to Box2d that for use with CUGL obstacle
//  heirarchy.  Obstacles provide a simple and direct way to create physics
//  objects that does not require the multi-step approach of Box2D.  It also
//  supports shared pointers for simply memory management.
//
//  However, this class is not as flexible as Box2D.  Therefore, it may be
//  necessary to access Box2D directly at times.
//
//  This class uses our standard shared-pointer architecture.
//
//  1. The constructor does not perform any initialization; it just sets all
//     attributes to their defaults.
//
//  2. All initialization takes place via init methods, which can fail if an
//     object is initialized more than once.
//
//  3. All allocation takes place via static constructors which return a shared
//     pointer.
//
//  CUGL MIT License:
//      This software is provided 'as-is', without any express or implied
//      warranty.  In no event will the authors be held liable for any damages
//      arising from the use of this software.
//
//      Permission is granted to anyone to use this software for any purpose,
//      including commercial applications, and to alter it and redistribute it
//      freely, subject to the following restrictions:
//
//      1. The origin of this software must not be misrepresented; you must not
//      claim that you wrote the original software. If you use this software
//      in a product, an acknowledgment in the product documentation would be
//      appreciated but is not required.
//
//      2. Altered source versions must be plainly marked as such, and must not
//      be misrepresented as being the original software.
//
//      3. This notice may not be removed or altered from any source distribution.
//
//  Author: Walker White
//  Version: 11/1/16

#include <box2d/b2_world.h>
#include <box2d/b2_contact.h>
#include <box2d/b2_collision.h>
#include <cugl/physics2/CUObstacleWorld.h>
#include <cugl/physics2/CUObstacle.h>

using namespace cugl;
using namespace cugl::physics2;

#pragma mark Constants

/** The default value of gravity (going down) */
#define DEFAULT_GRAVITY -9.8f

#pragma mark -
#pragma mark Proxy Classes

/**
 * A lightweight b2QueryCallback proxy.
 *
 * This class allows us to replace the listener class with a modern closure.
 */
class QueryProxy : public b2QueryCallback {
public:
    /**
     * Creates a new query proxy
     */
    QueryProxy() { onQuery = nullptr; }

    /**
     * Returns false to terminate an AABB query
     *
     * This function is called for each fixture found in the query AABB.
     *
     * @param  fixture  the fixture selected
     *
     * @return false to terminate the query.
     */
    std::function<bool(b2Fixture* fixture)> onQuery;
    
    /**
     * Returns false to terminate an AABB query
     *
     * This function is called for each fixture found in the query AABB.
     *
     * @param  fixture  the fixture selected
     *
     * @return false to terminate the query.
     */
    bool ReportFixture(b2Fixture* fixture) override {
        if (onQuery != nullptr) {
            return onQuery(fixture);
        }
        return false;
    }
};

/**
 * A lightweight b2RayCastCallback proxy.
 *
 * This class allows us to replace the listener class with a modern closure.
 */
class RaycastProxy : public b2RayCastCallback {
public:
    /**
     * Creates a new raycast proxy
     */
    RaycastProxy() { onQuery = nullptr; }

    /**
     * Called for each fixture found in the query.
     *
     * This callback controls how the ray cast proceeds by returning a float.
     * If -1, it ignores this fixture and continues.  If 0, it terminates the
     * ray cast.  If 1, it does not clip the ray and continues.  Finally, for
     * any fraction, it clips the ray at that point.
     *
     * @param  fixture  the fixture hit by the ray
     * @param  point    the point of initial intersection
     * @param  normal   the normal vector at the point of intersection
     * @param  faction  the fraction to return
     *
     * @return -1 to filter, 0 to terminate, fraction to clip the ray, 1 to continue
     */
    std::function<float(b2Fixture* fixture, const Vec2 point, const Vec2 normal, float fraction)> onQuery;
    
    /**
     * Called for each fixture found in the query.
     *
     * This callback controls how the ray cast proceeds by returning a float.
     * If -1, it ignores this fixture and continues.  If 0, it terminates the
     * ray cast.  If 1, it does not clip the ray and continues.  Finally, for
     * any fraction, it clips the ray at that point.
     *
     * @param  fixture  the fixture hit by the ray
     * @param  point    the point of initial intersection
     * @param  normal   the normal vector at the point of intersection
     * @param  fraction the fraction to return
     *
     * @return -1 to filter, 0 to terminate, fraction to clip the ray, 1 to continue
     */
    float ReportFixture(b2Fixture* fixture, const b2Vec2& point, const b2Vec2& normal, float fraction) override {
        if (onQuery != nullptr) {
            return onQuery(fixture,Vec2(point.x,point.y),Vec2(normal.x,normal.y),fraction);
        }
        return -1;
    }
};


#pragma mark -
#pragma mark Constructors

/**
 * Creates an inactive world controller
 *
 * The Box2d world will not be created until the appropriate init is called.
 */
ObstacleWorld::ObstacleWorld() :
_realworld(nullptr),
_drawworld(nullptr),
_collide(false),
_filters(false),
_destroy(false) {
    _stepsplits = DEFAULT_SPLITS;
    _stepssize  = DEFAULT_WORLD_STEP;
    _itvelocity = DEFAULT_WORLD_VELOC;
    _itposition = DEFAULT_WORLD_POSIT;
    _gravity = Vec2(0,DEFAULT_GRAVITY);
    _objectsAdded = 0;
    _remainingtime = 0;
    
    onBeginContact = nullptr;
    onEndContact   = nullptr;
    beforeSolve    = nullptr;
    afterSolve     = nullptr;
    shouldCollide  = nullptr;
    destroyFixture = nullptr;
    destroyJoint   = nullptr;
}

/**
 * Dispose of all resources allocated to this controller.
 */
void ObstacleWorld::dispose() {
    clear();
    if (_realworld != nullptr) {
        delete _realworld;
        _realworld  = nullptr;
    }
    if (_drawworld != nullptr) {
        delete _drawworld;
        _drawworld = nullptr;
    }
    onBeginContact = nullptr;
    onEndContact   = nullptr;
    beforeSolve    = nullptr;
    afterSolve     = nullptr;
    shouldCollide  = nullptr;
    destroyFixture = nullptr;
    destroyJoint   = nullptr;
}

/**
 * Initializes a new physics world
 *
 * The specified bounds are in terms of the Box2d world, not the screen.
 * A few attached to this Box2d world should have ways to convert between
 * the coordinate systems.
 *
 * This constructor will use the default gravitational value.
 *
 * @param  bounds   The game bounds in Box2d coordinates
 * @param  gravity  The gravitational force on this Box2d world
 *
 * @return  true if the controller is initialized properly, false otherwise.
 */
bool ObstacleWorld::init(const Rect bounds) {
    return init(bounds,_gravity);
}

/**
 * Initializes a new physics world
 *
 * The specified bounds are in terms of the Box2d world, not the screen.
 * A few attached to this Box2d world should have ways to convert between
 * the coordinate systems.
 *
 * @param  bounds   The game bounds in Box2d coordinates
 * @param  gravity  The gravitational force on this Box2d world
 *
 * @return  true if the controller is initialized properly, false otherwise.
 */
bool ObstacleWorld::init(const Rect bounds, const Vec2 gravity) {
    CUAssertLog(!_realworld,"Attempt to reinitialize and active world");
    CUAssertLog(!_drawworld, "Attempt to reinitialize and active world");
    _bounds = bounds;
    _realworld = new b2World(b2Vec2(gravity.x,gravity.y));
    _drawworld = new b2World(b2Vec2(gravity.x, gravity.y));
    if (_realworld && _drawworld) {
        return true;
    }
    return false;
}


#pragma mark -
#pragma mark Object Management
/**
 * Immediately adds the obstacle to the physics world
 *
 * Adding an obstacle activates the underlying physics.  It will now have
 * a body.  In the case of a {@link ComplexObstacle}, joints will be added
 * between the obstacles.  The physics world will include the obstacle in
 * its next call to update.
 *
 * The obstacle will be retained by this world, preventing it from being
 * garbage collected.
 *
 * param obj The obstacle to add
 */
void ObstacleWorld::addObstacle(const std::shared_ptr<Obstacle>& obj) {
    CUAssertLog(inBounds(obj.get()), "Obstacle is not in bounds");
    obj->setId(_objectsAdded);
    _objects[obj->getId()] = obj;
    obj->activatePhysics(*_realworld, *_drawworld);
    _objectsAdded++;
}

/**
 * Immediately removes object to the physics world
 *
 * The object will be released immediately.  If no more objects assert ownership,
 * then the object will be garbage collected.
 *
 * This method of removing objects is very heavy weight, and should only be used
 * for single object removal.  If you want to remove multiple objects, then you
 * should mark them for removal and call garbageCollect.
 *
 * param obj The object to remove
 *
 * @release a reference to the obstacle
 */
void ObstacleWorld::removeObstacle(Obstacle* obj) {
    obj->deactivatePhysics(*_realworld, *_drawworld);
    auto result = _objects.erase(obj->getId());
    // Result 3 means that zero elements were removed
    if (result == 3) CUAssertLog(false, "Physics object not present in world");
}

/**
 * Remove all objects marked for removal.
 *
 * The objects will be released immediately. If no more objects assert ownership,
 * then the objects will be garbage collected.
 *
 * This method is the efficient, preferred way to remove objects.
 */
void ObstacleWorld::garbageCollect() {
    for (auto it : _objects) {
        if (it.second->isRemoved()) {
            it.second->deactivatePhysics(*_realworld, *_drawworld);
            // #TODO This may be unwise. Find a way to use erase instead at some point? Doing this
            // makes the hash map just get bigger and bigger with longs pointing to null elements (I think?)
            it.second = nullptr;
        }
    }
}

/**
 * Remove all objects, emptying this controller.
 *
 * This method is different from a constructor in that the controller can still
 * receive new objects.
 */
void ObstacleWorld::clear() {
    for(auto it : _objects) {
        Obstacle* obj = it.second.get();
        obj->deactivatePhysics(*_realworld, *_drawworld);
    }
    _objects.clear();
    update(0);
}


#pragma mark -
#pragma mark Physics Handling

/**
 * Sets the global gravity vector.
 *
 * Any change will take effect at the time of the next call to update.
 *
 * @param  gravity  the global gravity vector.
 */
void ObstacleWorld::setGravity(const Vec2 gravity) {
    _gravity = gravity;
    if (_realworld != nullptr) {
        _realworld->SetGravity(b2Vec2(gravity.x,gravity.y));
    }
    if (_drawworld != nullptr) {
        _drawworld->SetGravity(b2Vec2(gravity.x, gravity.y));
    }
}

/**
 * Executes a single step of the physics engine.
 *
 * This method contains the specific update code for this mini-game. It does
 * not handle collisions, as those are managed by the parent class WorldController.
 * This method is called after input is read, but before collisions are resolved.
 * The very last thing that it should do is apply forces to the appropriate objects.
 *
 * Once the update phase is over, but before we draw, we are ready to handle
 * physics.  The primary method is the step() method in world.  This implementation
 * works for all applications and should not need to be overwritten.
 *
 * @param delta Number of seconds since last animation frame
 */
void ObstacleWorld::update(float dt) {
    // Turn the physics engine crank.

    // The mini step size. This is the "mini" steps we will use to get "close enough" to the amount of time that has actually passed.
    float ministep = _stepssize / (float)_stepsplits;
    // The total time needed to simulate
    float totaltime = _remainingtime + dt;
    // The total sim time (needed for obj->update)
    float totalsimtime = _remainingtime + dt;
    while (totaltime > ministep) {
        _realworld->Step(ministep, _itvelocity, _itposition);
        totaltime -= ministep;
    }

    // Now our real world is in the right state. Make one final step to set up the draw world and remember the remaining time from this frame
    _remainingtime = totaltime;
    // Sync real body to draw body
    for (auto it : _objects) {
        it.second->syncBodies();
    }
    // Step the draw world by the remaining time
    _drawworld->Step(_remainingtime, _itvelocity, _itposition);
    
    // Post process all objects after physics (this updates graphics)
    for(auto it : _objects) {
        Obstacle* obj = it.second.get();
        obj->update(totalsimtime);
    }
}

/**
 * Returns true if the object is in bounds.
 *
 * This assertion is useful for debugging the physics.
 *
 * @param obj The object to check.
 *
 * @return true if the object is in bounds.
 */
bool ObstacleWorld::inBounds(Obstacle* obj) {
    bool horiz = (_bounds.origin.x <= obj->getX() &&
                  obj->getX() <= _bounds.origin.x+_bounds.size.width);
    bool vert  = (_bounds.origin.y <= obj->getY() &&
                  obj->getY() <= _bounds.origin.y+_bounds.size.height);
    return horiz && vert;
}

#pragma mark -
#pragma mark Callback Activation

/**
 * Activates the collision callbacks.
 *
 * If flag is false, then the collision callbacks (even if defined) will be ignored.
 * Otherwise, the callbacks will be executed (on collision) if they are defined.
 *
 * @param  flag whether to activate the collision callbacks.
 */
void ObstacleWorld::activateCollisionCallbacks(bool flag) {
    if (_collide == flag) {
        return;
    }
    
    _realworld->SetContactListener(flag ? this : nullptr);
    _collide = flag;
}

/**
 * Activates the collision filter callbacks.
 *
 * If flag is false, then the collision filter callbacks (even if defined) will be
 * ignored. Otherwise, the callbacks will be executed (to test a collision) if they
 * are defined.
 *
 * @param  flag whether to activate the collision callbacks.
 */
void ObstacleWorld::activateFilterCallbacks(bool flag) {
    if (_filters == flag) {
        return;
    }
    
    _realworld->SetContactFilter(flag ? this : nullptr);
    _filters = flag;
}

/**
 * Activates the destruction callbacks.
 *
 * If flag is false, then the destruction callbacks (even if defined) will be ignored.
 * Otherwise, the callbacks will be executed (on body destruction) if they are defined.
 *
 * @param  flag whether to activate the collision callbacks.
 */
void ObstacleWorld::activateDestructionCallbacks(bool flag) {
    if (_destroy == flag) {
        return;
    }
    
    _realworld->SetDestructionListener(flag ? this : nullptr);
    _destroy = flag;
}


#pragma mark -
#pragma mark Query Functions

/**
 * Query the world for all fixtures that potentially overlap the provided AABB.
 *
 * The AABB is specified by a Cocos2d rectangle.
 *
 * @param  callback a user implemented callback function.
 * @param  rect     the axis-aligned bounding box
 */
void ObstacleWorld::queryAABB(std::function<bool(b2Fixture* fixture)> callback, const Rect aabb) const {
    b2AABB b2box;
    b2box.lowerBound.Set(aabb.origin.x, aabb.origin.y);
    b2box.upperBound.Set(aabb.origin.x+aabb.size.width, aabb.origin.y+aabb.size.height);
    QueryProxy proxy;
    proxy.onQuery = callback;
    _realworld->QueryAABB(&proxy, b2box);
}

/**
 * Ray-cast the world for all fixtures in the path of the ray.
 *
 * The callback controls whether you get the closest point, any point, or n-points.
 * The ray-cast ignores shapes that contain the starting point.
 *
 * @param  callback a user implemented callback function.
 * @param  point1   the ray starting point
 * @param  point2   the ray ending point
 */
void ObstacleWorld::rayCast(std::function<float(b2Fixture* fixture, const Vec2 point, const Vec2 normal, float fraction)> callback,
                              const Vec2 point1, const Vec2 point2) const {
    RaycastProxy proxy;
    proxy.onQuery = callback;
    _realworld->RayCast(&proxy, b2Vec2(point1.x,point1.y), b2Vec2(point2.x,point2.y));
}

std::vector<BodyNetData> ObstacleWorld::getState() {
    std::vector<BodyNetData> state;
    for (auto it : _objects) {
        std::shared_ptr<Obstacle> obs = it.second;
        // Only record non-static bodies. No need to update state on static bodies.
        if(obs->getBodyType() != b2_staticBody) state.push_back(obs->getBodyData());
    }
    return state;
}

void ObstacleWorld::updateFromState(std::vector<BodyNetData> data) {
    for (BodyNetData body : data) {
        _objects[body.id]->setBodyFromData(body);
    }
}
