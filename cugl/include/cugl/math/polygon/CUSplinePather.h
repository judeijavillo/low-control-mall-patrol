//
//  CUSplinePather.h
//  Cornell University Game Library (CUGL)
//
//  This module is a factory for producing Path2 and Poly2 objects from a Spline2.
//  In previous interations, this functionality was embedded in the Spline2
//  class.  That made that class much more heavyweight than we wanted for a
//  a simple math class.  By separating this out as a factory, we allow ourselves
//  the option of moving these calculations to a worker thread if necessary.
//
//  Because math objects are intended to be on the stack, we do not provide
//  any shared pointer support in this class.
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
//  Version: 6/22/21
//
#ifndef __CU_SPLINE_PATHER_H__
#define __CU_SPLINE_PATHER_H__

#include <cugl/math/CUSpline2.h>
#include <cugl/math/CUVec2.h>
#include <vector>
#include <unordered_map>

/** The default tolerance for the polygon approximation functions */
#define DEFAULT_FLATNESS   0.5

namespace cugl {

// Forward declarations
class Poly2;
class Path2;

/**
 * This class is a factory for producing Poly2 objects from a Spline2.
 *
 * In order to draw a cubic spline, we must first convert it to a Poly2 
 * object.  All of our rendering tools are designed around the basic Poly2
 * class.  In addition to generating a Poly2 for the spline path, this class 
 * can also generate Poly2 objects for UI elements such as handles and anchors.
 *
 * As with all factories, the methods are broken up into three phases:
 * initialization, calculation, and materialization.  To use the factory, you
 * first set the data (in this case a pointer to a Spline2) with the
 * initialization methods.  You then call the calculation method.  Finally,
 * you use the materialization methods to access the data in several different
 * ways.
 *
 * This division allows us to support multithreaded calculation if the data
 * generation takes too long.  However, not that this factory keeps a pointer
 * to the spline, and it is unsafe to modify the spline while the calculation
 * is ongoing.  If you do multithread the calculation, you should force the
 * user to copy the spline first.
 */
class SplinePather {
#pragma mark Values
private:
    /** A pointer to the spline data */
    const Spline2* _spline;
    /** The control data created by the approximation */
    std::vector<Vec2>  _pointbuff;
    /** The parameter data created by the approximation */
    std::vector<float> _parambuff;
    /** The anchor indicators */
    std::unordered_map<size_t,size_t> _anchorpts;
    /** Whether the approximation curve is closed */
    bool _closed;
    /** Whether or not the calculation has been run */
    bool _calculated;
    /** The flatness tolerance for generating paths */
    float _tolerance;

public:
#pragma mark -
#pragma mark Constructors
    /**
     * Creates a spline approximator with no spline data.
     */
    SplinePather() :
    _spline(nullptr),
    _calculated(false),
    _tolerance(DEFAULT_FLATNESS) {
    }

    /**
     * Creates a spline approximator with the given spline as its initial data.
     *
     * @param spline    The spline to approximate
     */
    SplinePather(const Spline2* spline) :
    _spline(spline),
    _calculated(false),
    _tolerance(DEFAULT_FLATNESS) {
    }

    /**
     * Deletes this spline approximator, releasing all resources.
     */
    ~SplinePather() {}
    

#pragma mark -
#pragma mark Initialization
    /**
     * Sets the given spline as the data for this spline approximator.
     *
     * This method resets all interal data.  You will need to reperform the
     * calculation before accessing data.
     *
     * @param spline    The spline to approximate
     */
    void set(const Spline2* spline) {
        reset();
        _spline = spline;
    }

    /**
     * Clears all internal data, but still maintains a reference to the spline.
     *
     * Use this method when you want to reperform the approximation at a 
     * different resolution.  
     */
    void reset();

    /**
     * Clears all internal data, including the spline data.
     *
     * When this method is called, you will need to set a new spline before
     * calling calculate.
     */
    void clear();


#pragma mark -
#pragma mark Calculation
    /**
     * Performs an approximation of the current spline
     *
     * A polygon approximation is creating by recursively calling de Castlejau's
     * until we reach a stopping condition. Currently the only supported stopping
     * condition is the recursion depth.
     *
     * The calculation uses a reference to the spline; it does not copy it. 
     * Hence this method is not thread-safe. If you are using this method in
     * a task thread, you should copy the spline first before starting the
     * calculation.
     */
    void calculate();
    

#pragma mark -
#pragma mark Materialization
    /**
     * Returns a new path approximating this spline.
     *
     * @return a new polygon approximating this spline.
     */
    Path2 getPath() const;

    /**
     * Stores vertex information approximating this spline in the buffer.
     *
     * The vertices (and indices) will be appended to the the Path2 object
     * if it is not empty. You should clear the path first if you do not
     * want to preserve the original data.
     *
     * @param buffer    The buffer to store the vertex data
     *
     * @return a reference to the buffer for chaining.
     */
    Path2* getPath(Path2* buffer) const;

    /**
     * Returns a list of parameters for a polygon approximation
     *
     * The parameters correspond to the generating values in the spline 
     * polynomial.  That is, if you evaluate the polynomial on the parameters,
     * {via {@link Spline2#getPoint()}, you will get the points in the
     * approximating polygon.
     *
     * @return a list of parameters for a polygon approximation
     */
    std::vector<float> getParameters() const;
    
    /**
     * Stores a list of parameters for the approximation in the buffer.
     *
     * The parameters correspond to the generating values in the spline
     * polynomial.  That is, if you evaluate the polynomial on the parameters,
     * {via {@link Spline2#getPoint()}, you will get the points in the
     * approximating polygon.
     *
     * The parameters will be appended to the buffer vector.  You should clear
     * the buffer first if you do not want to preserve the original data.
     *
     * @param buffer    The buffer to store the parameter data
     *
     * @return the number of elements added to the buffer
     */
    size_t getParameters(std::vector<float>& buffer);
    
    /**
     * Returns a list of tangents for a polygon approximation
     *
     * These tangent vectors are presented in control point order.  First, we 
     * have the right tangent of the first point, then the left tangent of the
     * second point, then the right, and so on.  Hence if the polygon contains
     * n points, this method will return 2(n-1) tangents.
     *
     * If calculate has not been called, this method will choose tangents
     * for the control points on the original spline.  This latter option
     * is useful when you want to draw a UI for the control point tangents.
     *
     * @return a list of tangents for a polygon approximation
     */
    std::vector<Vec2> getTangents() const;
    
    /**
     * Stores a list of tangents for the approximation in the buffer.
     *
     * These tangent vectors are presented in control point order.  First, we
     * have the right tangent of the first point, then the left tangent of the
     * second point, then the right, and so on.  Hence if the polygon contains
     * n points, this method will return 2(n-1) tangents.
     *
     * If calculate has not been called, this method will choose tangents
     * for the control points on the original spline.  This latter option
     * is useful when you want to draw a UI for the control point tangents.
     *
     * The tangents will be appended to the buffer vector.  You should clear
     * the buffer first if you do not want to preserve the original data.
     *
     * @return the number of elements added to the buffer
     */
    size_t getTangents(std::vector<Vec2>& buffer);

    /**
     * Returns a list of normals for a polygon approximation
     *
     * There is one normal per control point. If polygon contains n points, 
     * this method will also return n normals. The normals are determined by the
     * right tangents.  If the spline is open, then the normal of the last point
     * is determined by its left tangent.
     *
     * If calculate has not been called, this method will choose normals
     * for the control points on the original spline.  This latter option
     * is useful when you want to draw a UI for the control point normals.
     *
     * @return a list of normals for a polygon approximation
     */
    std::vector<Vec2> getNormals() const;

    /**
     * Stores a list of normals for the approximation in the buffer.
     *
     * There is one normal per control point. If polygon contains n points,
     * this method will also return n normals. The normals are determined by the
     * right tangents.  If the spline is open, then the normal of the last point
     * is determined by its left tangent.
     *
     * If calculate has not been called, this method will choose normals
     * for the control points on the original spline.  This latter option
     * is useful when you want to draw a UI for the control point normals.
     *
     * The normals will be appended to the buffer vector.  You should clear
     * the buffer first if you do not want to preserve the original data.
     *
     * @return the number of elements added to the buffer
     */
    size_t getNormals(std::vector<Vec2>& buffer);
    
    /**
     * Returns a Poly2 representing handles for the anchor points
     *
     * This method returns a collection of vertex information for handles at 
     * the anchor points.  Handles are circular shapes of a given radius. This 
     * information may be drawn to provide a visual representation of the
     * anchor points (as seen in Adobe Illustrator).
     *
     * If calculate has not been called, this method will choose anchors
     * for the control points on the original spline.  This latter option
     * is useful when you want to draw a UI for the original control points.
     *
     * @param  radius   the radius of each handle
     * @param  segments the number of segments in the handle "circle"
     *
     * @return a Poly2 representing handles for the anchor points
     */
    Poly2 getAnchors(float radius, int segments=4) const;
    
    /**
     * Stores vertex information representing the anchor points in the buffer.
     *
     * This method creates a collection of vertex information for handles at
     * the anchor points.  Handles are circular shapes of a given radius. This
     * information may be drawn to provide a visual representation of the
     * anchor points (as seen in Adobe Illustrator).
     *
     * If calculate has not been called, this method will choose anchors
     * for the control points on the original spline.  This latter option
     * is useful when you want to draw a UI for the original control points.
     *
     * The vertices (and indices) will be appended to the the Poly2 if it is
     * not empty. You should clear the Poly2 first if you do not want to
     * preserve the original data.
     *
     * @param  buffer   The buffer to store the vertex data
     * @param  radius   The radius of each handle
     * @param  segments The number of segments in the handle "circle"
     *
     * @return a reference to the buffer for chaining.
     */
    Poly2* getAnchors(Poly2* buffer, float radius, int segments=4) const;
    
    /**
     * Returns a Poly2 representing handles for the tangent points
     *
     * This method returns vertex information for handles at the tangent
     * points.  Handles are circular shapes of a given radius. This information
     * may be passed to a PolygonNode to provide a visual representation of the
     * tangent points (as seen in Adobe Illustrator).
     *
     * If calculate has not been called, this method will choose the tangents
     * from the control points on the original spline.  This latter option
     * is useful when you want to draw a UI for the original tangent points.
     *
     * @param  radius   the radius of each handle
     * @param  segments the number of segments in the handle "circle"
     *
     * @return a Poly2 representing handles for the tangent points
     */
    Poly2 getHandles(float radius, int segments=4) const;

    /**
     * Stores vertex information representing tangent point handles in the buffer.
     *
     * This method creates vertex information for handles at the tangent
     * points.  Handles are circular shapes of a given radius. This information
     * may be passed to a PolygonNode to provide a visual representation of the
     * tangent points (as seen in Adobe Illustrator).
     *
     * If calculate has not been called, this method will choose the tangents
     * from the control points on the original spline.  This latter option
     * is useful when you want to draw a UI for the original tangent points.
     *
     * The vertices (and indices) will be appended to the the Poly2 if it is
     * not empty. You should clear the Poly2 first if you do not want to
     * preserve the original data.
     *
     * @param  buffer   The buffer to store the vertex data
     * @param  radius   the radius of each handle
     * @param  segments the number of segments in the handle "circle"
     *
     * @return a reference to the buffer for chaining.
     */
    Poly2* getHandles(Poly2* buffer, float radius, int segments=4) const;

    /**
     * Returns an expanded version of this spline
     *
     * When we use de Castlejau's to approximate the spline, it produces a list
     * of control points that are geometrically equal to this spline (e.g. ignoring
     * parameterization). Instead of flattening this information to a polygon,
     * this method presents this data as a new spline.
     *
     * If calculate has not been called, this method will copy the original spline.
     *
     * @return an expanded version of this spline
     */
    Spline2 getRefinement() const;
    
    /**
     * Stores an expanded version of this spline in the given buffer.
     *
     * When we use de Castlejau's to approximate the spline, it produces a list
     * of control points that are geometrically equal to this spline (e.g. ignoring
     * parameterization). Instead of flattening this information to a polygon,
     * this method presents this data as a new spline.
     *
     * The control points will be appended to the the spline if it is not
     * empty. You should clear the spline first if you do not want to preserve 
     * the original data.
     *
     * If calculate has not been called, this method will copy the original spline.
     *
     * @param  buffer   The buffer to store the vertex data
     *
     * @return a reference to the buffer for chaining.
     */
    Spline2* getRefinement(Spline2* buffer) const;

#pragma mark -
#pragma mark Internal Data Generation
private:
    /**
     * Generates data via recursive use of de Castlejau's
     *
     * This method subdivides the spline at the given segment. The results
     * are put in the output buffers.
     *
     * @param  t        the parameter for the (start of) this segment
     * @param  p0       the left anchor of this segment
     * @param  p1       the left tangent of this segment
     * @param  p2       the right tangent of this segment
     * @param  p3       the right anchor of this segment
     * @param  depth    the current depth of the recursive call
     *
     * @return The number of (anchor) points generated by this recursive call.
     */
    int generate(float t, const Vec2* p0, const Vec2* p1, const Vec2* p2, const Vec2* p3, int depth);

    /**
     * Returns the currently "active" control points.
     *
     * If the calculation has been run, this is the data for the calculation.
     * Otherwise, it is the control points of the original spline
     */
    const std::vector<Vec2>* getActivePoints() const {
        return (_calculated ? &_pointbuff : (_spline ? &(_spline->_points) : nullptr));
    }
    
    /** 
     * Returns true if the current approximation is closed.
     *
     * @return true if the current approximation is closed.
     */
    bool isClosed() const {
        return (_calculated ? _closed : (_spline ? _spline->_closed : false));
    }
    
    /**
     * Returns true if the point at the given position is an anchor
     *
     * This value is always true if calculate has not been called
     *
     * @param pos   The position to check
     *
     * @return true if the point at the given position is an anchor
     */
    bool isAnchor(size_t pos) const {
        return (_calculated ? _anchorpts.find(pos) != _anchorpts.end() : true);
    }
};

}
#endif /* __CU_SPLINE_PATHER_H__ */
