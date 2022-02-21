//
//  CUCapsuleObstacle.h
//  Cornell University Game Library (CUGL)
//
//  This class implements a capsule physics object. A capsule is a box with
//  semicircular ends along the major axis.  They are a popular physics objects,
//  particularly for character avatars.  The rounded ends means they are less
//  likely to snag, and they naturally fall off platforms when they go too far.
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
//  This file is based on the CS 3152 PhysicsDemo Lab by Don Holden, 2007
//
//  Author: Walker White
//  Version: 1/22/21
//
#ifndef __CU_CAPSULE_OBSTACLE_H__
#define __CU_CAPSULE_OBSTACLE_H__

#include <box2d/b2_polygon_shape.h>
#include <box2d/b2_circle_shape.h>
#include <box2d/b2_collision.h>
#include "CUSimpleObstacle.h"

namespace cugl {
    /**
     * The classes to represent 2-d physics.
     *
     * This namespace was chosen to future-proof the game engine. We will
     * eventually want to add a 3-d physics engine as well, and this namespace
     * will prevent any collisions with those scene graph nodes.
     */
    namespace physics2 {

#pragma mark -
#pragma mark Capsule Obstacle
/**
 * Capsule-shaped model to support collisions.
 *
 * A capsule is a box with semicircular ends along the major axis. They are a 
 * popular physics object, particularly for character avatars.  The rounded ends 
 * means they are less likely to snag, and they naturally fall off platforms
 * when they go too far.
 *
 * If width < height, the capsule will be oriented vertically with the rounded
 * portions at the top and bottom. Otherwise it will be oriented horizontally.
 * The constructors allow some control over the capsule shape.  You can have
 * half-capsules or full capsules.  In the case where width == height, the
 * capsule will be a circle or semicircle, depending on the capsula shape.
 */
class CapsuleObstacle : public SimpleObstacle {
protected:
    /** Shape information for this capsule core */
    b2PolygonShape _shape;
    /** Shape information for the end caps */
    b2CircleShape  _ends;
    /** AABB representation of capsule core for fast computation */
    b2AABB _center;
    
    /** A cache value for the center fixture (for resizing) */
    b2Fixture* _core;
    /** A cache value for the first end cap fixture (for resizing) */
    b2Fixture* _cap1;
    /** A cache value for the second end cap fixture (for resizing) */
    b2Fixture* _cap2;
    /** The width and height of the capsule */
    Size _dimension;
    /** The capsule shape */
    poly2::Capsule _orient;
    
    /** The seam offset of the core rectangle */
    float _seamEpsilon;
    
    
#pragma mark -
#pragma mark Scene Graph Methods
    /**
     * Resets the polygon vertices in the shape to match the dimension.
     *
     * This is an internal method and it does not mark the physics object as
     * dirty.
     *
     * @param  size The new dimension (width and height)
     */
    void resize(const Size size);
    
    /**
     * Creates the outline of the physics fixtures in the debug node
     *
     * The debug node is use to outline the fixtures attached to this object.
     * This is very useful when the fixtures have a very different shape than
     * the texture (e.g. a circular shape attached to a square texture).
     */
    virtual void resetDebug() override;
    
    
#pragma mark -
#pragma mark Constructors
public:
    /**
     * Creates a new capsule object at the origin.
     *
     * NEVER USE A CONSTRUCTOR WITH NEW. If you want to allocate an object on
     * the heap, use one of the static constructors instead.
     */
    CapsuleObstacle(void) : SimpleObstacle(),
    _core(nullptr), _cap1(nullptr), _cap2(nullptr), _seamEpsilon(0.0f) { }
    
    /**
     * Deletes this physics object and all of its resources.
     *
     * We have to make the destructor public so that we can polymorphically
     * delete physics objects.
     *
     * A non-default destructor is necessary since we must release all
     * claims on scene graph nodes.
     */
    virtual ~CapsuleObstacle() {
        CUAssertLog(_core == nullptr, "You must deactive physics before deleting an object");
    }
    
    /**
     * Initializes a new box object at the origin with no size.
     *
     * @return  true if the obstacle is initialized properly, false otherwise.
     */
    virtual bool init() override { return init(Vec2::ZERO,Size::ZERO); }
    
    /**
     * Initializes a new capsule object at the given point with no size.
     *
     * The scene graph is completely decoupled from the physics system.
     * The node does not have to be the same size as the physics body. We
     * only guarantee that the scene graph node is positioned correctly
     * according to the drawing scale.
     *
     * @param  pos  Initial position in world coordinates
     *
     * @return  true if the obstacle is initialized properly, false otherwise.
     */
    virtual bool init(const Vec2 pos) override { return init(pos,Size::ZERO); }
    
    /**
     * Initializes a new capsule object of the given dimensions.
     *
     * The orientation of the capsule will be a full capsule along the
     * major axis.  If width == height, it will be a simple circle.
     *
     * The scene graph is completely decoupled from the physics system.
     * The node does not have to be the same size as the physics body. We
     * only guarantee that the scene graph node is positioned correctly
     * according to the drawing scale.
     *
     * @param  pos      Initial position in world coordinates
     * @param  size     The capsule size (width and height)
     *
     * @return  true if the obstacle is initialized properly, false otherwise.
     */
    virtual bool init(const Vec2 pos, const Size size) {
        return init(pos,size,poly2::Capsule::FULL);
    }
    
    /**
     * Initializes a new capsule object of the given dimensions.
     *
     * The orientation of the capsule is determined by the major axis. A `HALF`
     * capsule is rounded on the left for horizontal orientation and on the
     * bottom for vertical orientation.  A `HALF_REVERSE` capsule is the reverse.
     *
     * The scene graph is completely decoupled from the physics system.
     * The node does not have to be the same size as the physics body. We
     * only guarantee that the scene graph node is positioned correctly
     * according to the drawing scale.
     *
     * @param  pos  	Initial position in world coordinates
     * @param  size 	The capsule size (width and height)
     * @param  shape    The capsule shape/orientation
     *
     * @return  true if the obstacle is initialized properly, false otherwise.
     */
    virtual bool init(const Vec2 pos, const Size size, poly2::Capsule shape);
    
#pragma mark -
#pragma mark Static Constructors
    /**
     * Returns a new capsule object at the origin with no size.
     *
     * @return a new capsule object at the origin with no size.
     */
    static std::shared_ptr<CapsuleObstacle> alloc() {
        std::shared_ptr<CapsuleObstacle> result = std::make_shared<CapsuleObstacle>();
        return (result->init() ? result : nullptr);
    }
    
    /**
     * Returns a new capsule object at the given point with no size.
     *
     * The scene graph is completely decoupled from the physics system.
     * The node does not have to be the same size as the physics body. We
     * only guarantee that the scene graph node is positioned correctly
     * according to the drawing scale.
     *
     * @param pos   Initial position in world coordinates
     *
     * @return a new capsule object at the given point with no size.
     */
    static std::shared_ptr<CapsuleObstacle> alloc(const Vec2 pos) {
        std::shared_ptr<CapsuleObstacle> result = std::make_shared<CapsuleObstacle>();
        return (result->init(pos) ? result : nullptr);
    }
    
    /**
     * Returns a new capsule object of the given dimensions.
     *
     * The orientation of the capsule will be a full capsule along the
     * major axis.  If width == height, it will be a simple circle.
     *
     * The scene graph is completely decoupled from the physics system.
     * The node does not have to be the same size as the physics body. We
     * only guarantee that the scene graph node is positioned correctly
     * according to the drawing scale.
     *
     * @param pos       Initial position in world coordinates
     * @param size      The capsule size (width and height)
     *
     * @return a new capsule object of the given dimensions.
     */
    static std::shared_ptr<CapsuleObstacle> alloc(const Vec2 pos, const Size size) {
        std::shared_ptr<CapsuleObstacle> result = std::make_shared<CapsuleObstacle>();
        return (result->init(pos,size) ? result : nullptr);
    }
    
    /**
     * Returns a new capsule object of the given dimensions and orientation.
     *
     * The orientation of the capsule is determined by the major axis. A `HALF`
     * capsule is rounded on the left for horizontal orientation and on the
     * bottom for vertical orientation.  A `HALF_REVERSE` capsule is the reverse.
     *
     * The scene graph is completely decoupled from the physics system.
     * The node does not have to be the same size as the physics body. We
     * only guarantee that the scene graph node is positioned correctly
     * according to the drawing scale.
     *
     * @param pos     Initial position in world coordinates
     * @param size    The capsule size (width and height)
     * @param shape   The capsule shape/orientation
     *
     * @return a new capsule object of the given dimensions and orientation.
     */
    static std::shared_ptr<CapsuleObstacle> alloc(const Vec2 pos, const Size size, poly2::Capsule shape) {
        std::shared_ptr<CapsuleObstacle> result = std::make_shared<CapsuleObstacle>();
        return (result->init(pos,size,shape) ? result : nullptr);
    }
    
    
#pragma mark -
#pragma mark Dimensions
    /**
     * Returns the dimensions of this capsule
     *
     * @return the dimensions of this capsule
     */
    const Size getDimension() const { return _dimension; }
    
    /**
     * Sets the dimensions of this capsule
     *
     * @param value  the dimensions of this capsule
     */
    void setDimension(const Size value) { resize(value); markDirty(true); }
    
    /**
     * Sets the dimensions of this capsule
     *
     * @param width   The width of this capsule
     * @param height  The height of this capsule
     */
    void setDimension(float width, float height) { setDimension(Size(width,height)); }
    
    /**
     * Returns the capsule width
     *
     * @return the capsule width
     */
    float getWidth() const { return _dimension.width; }
    
    /**
     * Sets the capsule width
     *
     * @param value  the capsule width
     */
    void setWidth(float value) { setDimension(value,_dimension.height); }
    
    /**
     * Returns the capsule height
     *
     * @return the capsule height
     */
    float getHeight() const { return _dimension.height; }
    
    /**
     * Sets the capsule height
     *
     * @param value  the capsule height
     */
    void setHeight(float value) { setDimension(_dimension.width,value); }
    
    /**
     * Returns the shape/orientation of this capsule
     *
     * @return the shape/orientation of this capsule
     */
    poly2::Capsule getShape() const { return _orient; }
    
    /**
     * Sets the shape/orientation of this capsule.
     *
     * @param value  the shape/orientation of this capsule
     */
    void setShape(poly2::Capsule value);
    
    
#pragma mark -
#pragma mark Physics Methods
    /**
     * Sets the seam offset of the core rectangle
     *
     * If the center rectangle is exactly the same size as the circle radius,
     * you may get catching at the seems.  To prevent this, you should make
     * the center rectangle epsilon narrower so that everything rolls off the
     * round shape. This parameter is that epsilon value
     *
     * @param  value the seam offset of the core rectangle
     */
    void setSeamOffset(float value);
    
    /**
     * Returns the seam offset of the core rectangle
     *
     * If the center rectangle is exactly the same size as the circle radius,
     * you may get catching at the seems.  To prevent this, you should make
     * the center rectangle epsilon narrower so that everything rolls off the
     * round shape. This parameter is that epsilon value
     *
     * @return the seam offset of the core rectangle
     */
    float getSeamOffset() const { return _seamEpsilon; }
    
    /**
     * Sets the density of this body
     *
     * The density is typically measured in usually in kg/m^2. The density can be zero or
     * positive. You should generally use similar densities for all your fixtures. This
     * will improve stacking stability.
     *
     * @param value  the density of this body
     */
    virtual void setDensity(float value) override;
    
    /**
     * Create new fixtures for this body, defining the shape
     *
     * This is the primary method to override for custom physics objects
     */
    virtual void createFixtures() override;
    
    /**
     * Release the fixtures for this body, reseting the shape
     *
     * This is the primary method to override for custom physics objects
     */
    virtual void releaseFixtures() override;
    
};
	}
}
#endif /* __CU_CAPSULE_OBSTACLE_H__ */
