//
//  CUPathSmoother.h
//  Cornell University Game Library (CUGL)
//
//  This module is a factory taking a continuous path of points and smoothing it
//  into a path with less points. We realized that this class was necessary
//  when students kept writing code to track a finger gesture and stored all the
//  points, no matter how close they were together (and then tried to draw that
//  path, overwhelming the poor early versions of the SpriteBatch).
//
//  This class uses the Douglas-Peuker algorithm, as described here:
//
//       https://en.wikipedia.org/wiki/Ramer–Douglas–Peucker_algorithm
//
//  Because math objects are intended to be on the stack, we do not provide
//  any shared pointer support in this class.
//
//  This implementation is largely inspired by the LibGDX implementation from
//  Nicolas Gramlich, Eric Spits, Thomas Cate, and Nathan Sweet.
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
//  Version: 7/12/21
//
#ifndef __CU_PATH_SMOOTHER_H__
#define __CU_PATH_SMOOTHER_H__
#include <cugl/math/CUVec2.h>
#include <cugl/math/CUPath2.h>
#include <vector>

namespace cugl {

/**
 * This class smooths a continuous path of points, reducing the number needed.
 *
 * A common temptation with mobile games is to track the player's finger gesture
 * by recording all of the finger positions over time.  Except that this is a lot
 * of points (and attempting to draw all these points exposed some serious flaws
 * in earlier versions of {@link SpriteBatch}).  If points are too close together,
 * then some of them can be safely removed without altering the shape of the path.
 *
 * This class uses the Douglas-Peuker algorithm, as described here:
 *
 *      https://en.wikipedia.org/wiki/Ramer–Douglas–Peucker_algorithm
 *
 * The correct epsilon value to use should be found with experimentation. In
 * particular, it depends on the scale of the path being smoothed.
 *
 * As with all factories, the methods are broken up into three phases:
 * initialization, calculation, and materialization.  To use the factory, you
 * first set the data (in this case a set of vertices or another Poly2) with the
 * initialization methods.  You then call the calculation method.  Finally,
 * you use the materialization methods to access the data in several different
 * ways.
 *
 * This division allows us to support multithreaded calculation if the data
 * generation takes too long.  However, note that this factory is not thread
 * safe in that you cannot access data while it is still in mid-calculation.
 */
class PathSmoother {
#pragma mark Values
private:
    /** The set of vertices to use in the calculation */
    std::vector<Vec2> _input;
    /** The set of vertices after smoothing */
    std::vector<Vec2> _output;
    /** The epsilon value of the Douglas-Peucker algorithm */
    float _epsilon;
    /** Whether or not the calculation has been run */
    bool _calculated;

#pragma mark -
#pragma mark Constructors
public:
    /**
     * Creates a path smoother with no vertex data.
     */
    PathSmoother();

    /**
     * Creates a path smoother with the given vertex data.
     *
     * The vertex data is copied.  The smother does not retain any references
     * to the original data.
     *
     * @param points    The vertices to triangulate
     */
    PathSmoother(const std::vector<Vec2>& points);
    
    /**
     * Deletes this path smoother, releasing all resources.
     */
    ~PathSmoother() {}
    
#pragma mark -
#pragma mark Initialization
    /**
     * Sets the vertex data for this path smoother.
     *
     * The vertex data is copied. The smother does not retain any references
     * to the original data.
     *
     * This method resets all interal data. You will need to reperform the
     * calculation before accessing data.
     *
     * @param points    The vertices to triangulate
     */
    void set(const std::vector<Vec2>& points) {
        reset();
        _input = points;
    }
    
    /**
     * Sets the vertex data for this path smoother.
     *
     * The vertex data is copied. The smother does not retain any references
     * to the original data.  In addition, only the vertex data is copied.
     * Whether or not the path is closed is ignored.
     *
     * This method resets all interal data. You will need to reperform the
     * calculation before accessing data.
     *
     * @param path    The path to smooth
     */
    void set(const Path2& path) {
        reset();
        _input = path.vertices;
    }

    /**
     * Sets the epsilon value for the smoothing algorithm.
     *
     * The epsilon value specifies the tolerance for the algorithm.  At each
     * step, any point that is with epsilon of a line segment is considered
     * to be part of that line segment.
     *
     * Typically this value is found by experimentation. However, because this
     * is typically used to smooth touch paths (which have integer coordinates),
     * the value should be at least 1 (which is the default).
     *
     * @param epsilon   The epsilon value for the smoothing algorithm.
     */
    void setEpsilon(float epsilon) {
        _epsilon = epsilon;
    }

    /**
     * Returns the epsilon value for the smoothing algorithm.
     *
     * The epsilon value specifies the tolerance for the algorithm.  At each
     * step, any point that is with epsilon of a line segment is considered
     * to be part of that line segment.
     *
     * Typically this value is found by experimentation. However, because this
     * is typically used to smooth touch paths (which have integer coordinates),
     * the value should be at least 1 (which is the default).
     *
     * @return the epsilon value for the smoothing algorithm.
     */
    float getEpsilon() const {
        return _epsilon;
    }

#pragma mark -
#pragma mark Calculation
    /**
     * Clears all internal data, but still maintains the initial vertex data.
     */
    void reset();
    
    /**
     * Clears all internal data, the initial vertex data.
     *
     * When this method is called, you will need to set a new vertices before
     * calling calculate.
     */
    void clear();
    
    /**
     * Performs a triangulation of the current vertex data.
     */
    void calculate();
    
#pragma mark -
#pragma mark Materialization
    /**
     * Returns a list of points representing the smoothed path.
     *
     * The result is guaranteed to be a subset of the original vertex path,
     * order preserved. The smoother does not retain a reference to the returned
     * list; it is safe to modify it.
     *
     * If the calculation is not yet performed, this method will return the
     * empty list.
     *
     * @return a list of indices representing the triangulation.
     */
    std::vector<Vec2> getPoints() const;

    /**
     * Stores the triangulation indices in the given buffer.
     *
     * The result is guaranteed to be a subset of the original vertex path,
     * order preserved. The points will be appended to the provided vector.
     * You should clear the vector first if you do not want to preserve the
     * original data.
     *
     * If the calculation is not yet performed, this method will do nothing.
     *
     * @return the number of elements added to the buffer
     */
    size_t getPoints(std::vector<Vec2>& buffer) const;

    /**
     * Returns a path object representing the smoothed result.
     *
     * The resulting path is open.
     *
     * If the calculation is not yet performed, this method will return the
     * empty path.
     *
     * @return a polygon representing the triangulation.
     */
    Path2 getPath() const;
    
    /**
     * Stores the smoothed path in the given buffer.
     *
     * If the buffer is not empty, the vertices will be appended to
     * the end of the buffer.
     *
     * If the calculation is not yet performed, this method will do nothing.
     *
     * @param buffer    The buffer to store the smoothed path
     *
     * @return a reference to the buffer for chaining.
     */
    Path2* getPath(Path2* buffer) const;

#pragma mark -
#pragma mark Internal Data Generation
private:
    /**
     * Recursively performs Douglas-Peuker on the given input segment
     *
     * The results will be pulled from _input and placed in _output.
     *
     * @param start The first position in _input to process
     * @param end   The last position in _input to process
     *
     * @return the number of points preserved in smoothing
     */
    size_t douglasPeucker(size_t start, size_t end);
};

}


#endif /* __CU_PATH_SMOOTHER_H__ */
