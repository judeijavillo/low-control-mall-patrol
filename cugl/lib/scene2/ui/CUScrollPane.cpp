//
//  CUScrollPane.h
//  Cornell University Game Library (CUGL)
//
//  This module provides support for scroll pane that allows the user to
//  navigate a limited view of a larger backing view.  Classic examples
//  are a window that cannot show everything on the screen at once.
//
//  In many APIs, a scroll pane comes with sliders that allow you to
//  control the window position. However, while this is the natural
//  way to navigate a scroll pane in a desktop OS, it is not the natural
//  way to navigate it in a mobile device. In the latter case, gestures
//  are the preferred means of navigation.  As a result, this class is
//  not coupled with its navigation interface, and simply provides methods
//  for adjusting the position and orientation of the backing contents.
//
//  If you would like to have a more traditional navigation interface,
//  such as scroll bars, simply add slider object to the scene graph.
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
//  Version: 11/18/21
//
#include <cugl/scene2/ui/CUScrollPane.h>

using namespace cugl;
using namespace cugl::scene2;

/** A small number for round off errors */
#define EPSILON 0.001f
/** The default minimum cumulative zoom */
#define ZOOM_MIN 0.1f
/** The default maximum cumulative zoom */
#define ZOOM_MAX 5.0f

#pragma mark -
#pragma mark Geometry Functions
/**
 * Returns the given corner of the rectangle.
 *
 * This function allows us to iterate through the corners
 * of a rectangle. It starts at the lower left corner and
 * goes counter clockwise.
 *
 * @param rect  The rectangle to iterate
 * @param pos   The corner position
 *
 * @return the given corner of the rectangle.
 */
static Vec2 rect_corner(const Rect& rect, int pos) {
    pos = (pos % 4);
    if (pos < 0) {
        pos += 4;
    }
    Vec2 corner = rect.origin;
    if (pos > 0 && pos < 3) {
        corner.x += rect.size.width;
    }
    if (pos > 1) {
        corner.y += rect.size.height;
    }
    return corner;
}

/**
 * Modifies p to its orthogonal projection along ab
 *
 * This function false if ab is the zero vector.
 *
 * @param p The point to project
 * @param a The start of the projection vector
 * @param b The end of the projection vector
 *
 * @return true if the projection exists
 */
static bool ortho_proj(Vec2& p, const Vec2& a,const Vec2& b) {
    Vec2 c = b-a;
    if (c.isZero()) {
        return false;
    }

    Vec2 q = p-a;
    float n = q.dot(c);
    float d = c.dot(c);
    if (n/d > 1 || n/d < 0) {
        return false;
    }

    p = c*n/d+a;
    return true;
}

/**
 * Returns the ray intersection between pq and uv.
 *
 * The answer is stored in out if it exists.
 *
 * @param p     The start of the first ray
 * @param q     The continuation of the first ray
 * @param u     The start of the second ray
 * @param v     The continuation of the second ray
 * @param out   The variable to store the intersection
 *
 * @return true if the intersection exists
 */
static bool ray_intersect(const Vec2& p, const Vec2& q, const Vec2& u, const Vec2& v, Vec2* out) {
    float s, t;
    Vec2::doesLineIntersect(p,q,u,v,&s,&t);
    if (s >= 0.0 && s <= 1.0 && t >= 0) {
        *out = p+s*(q-p);
        return true;
    }
    return false;
}

/**
 * Computes the two points on the circle that contain p as a tangent
 *
 * Given a circle of the given radius centered at anchor, if p is outside
 * this circle, then there are two points on the circle whose tangent lines
 * intersect p. The values of these two points are stored in u and v if
 * they exist.
 *
 * @param center    The circle center
 * @param radius    The circle radius
 * @param p         The candidate point
 * @param u         The variable to store the first point
 * @param v         The variable to store the second point
 *
 * @return true if p is outside of the circle
 */
static bool tangents(const Vec2& center, float radius, const Vec2& p, Vec2& u, Vec2& v) {
    Vec2 delta = p-center;
    float dxr = -delta.y;
    float dyr = delta.x;
    float d = delta.length();
    
    if (d >= radius) {
        float rho = radius/d;
        float ad = rho*rho;
        float bd = rho*sqrt(1-rho*rho);
        
        u.x = center.x + ad*delta.x + bd*dxr;
        u.y = center.y + ad*delta.y + bd*dyr;
        v.x = center.x + ad*delta.x - bd*dxr;
        v.y = center.y + ad*delta.y - bd*dyr;
        return true;
    }
    
    return false;
}

/**
 * Returns the angle of the circle segment/arc from p and q
 *
 * The points p and q sit on the boundary of the circle centered at c.
 * The value sgn indicates the direction of the segment, as there are
 * two segments between any two points on a circle
 *
 * @param p     The starting point
 * @param q     The ending point
 * @param c     The circle origin
 * @param sgn   The angle direction (positive or negative)
 *
 * @return the angle of the circle segment/arc from p and q
 */
static float seg_angle(const Vec2& p, const Vec2& q, const Vec2& c, float sgn) {
    Vec2 temp = p-c;
    float a = atan2f(temp.y, temp.x);
    temp = q-c;
    float b = atan2f(temp.y, temp.x);
    float angle = b-a;
    
    if (sgn > 0) {
        while (angle >= 2*M_PI) {
            angle -= 2*M_PI;
        }
        while (angle < 0) {
            angle += 2*M_PI;
        }
    } else {
        while (angle >= 0) {
            angle -= 2*M_PI;
        }
        while (angle < -2*M_PI) {
            angle += 2*M_PI;
        }
    }
    return angle;
}

/**
 * Clamps an attempted spin so that it stays within bounds.
 *
 * This function compares a (transformed) contents against a viewport.
 * It assumes the contents are in bounds before a pan transformation.
 * It then checks if the given spin angle keeps the contents in bounds.
 * If not, it returns the closest legal angle (which may be zero) that
 * keeps the contents in bounds.
 *
 * @param viewport  The viewport to show the content
 * @param contents  The backing window of contents
 * @param transform The transform for the backing window
 * @param anchor    The origin for the spin
 * @param angle     The spin angle
 *
 * @return the amount zoomed
 */
static float clamp_spin(const Rect& viewport, const Rect& contents,
                        const Affine2& transform, const Vec2& anchor, float angle) {
    float result = angle;

    float maxrad = 0;
    for(int ii = 0; ii < 4; ii++) {
        Vec2 corner = rect_corner(viewport,ii);
        float dist = corner.distance(anchor);
        if (dist > maxrad) {
            maxrad = dist;
        }
    }
    
    Vec2 exclude[8];
    Vec2 a = rect_corner(contents,3)*transform;
    for(int ii = 0; ii < 4; ii++) {
        size_t exsize = 0;
        Vec2 b = rect_corner(contents,ii)*transform;
        Vec2 q = anchor;
        
        if (ortho_proj(q,a,b)) {
            float radius = anchor.distance(q);
            if (radius <= maxrad) {
                for(int jj = 0; jj < 4; jj++) {
                    Vec2 corner = rect_corner(viewport,jj);
                    float dist = corner.distance(anchor);
                    if (dist >= radius) {
                        Vec2 t1, t2;
                        if (tangents(anchor, radius, corner, t1, t2)) {
                            exclude[exsize  ] = t1;
                            exclude[exsize+1] = t2;
                            exsize += 2;
                        }
                    }
                }
            }
            
            // Now try
            for(int jj = 0; jj < exsize; jj += 2) {
                float u = seg_angle(q,exclude[jj  ],anchor,result);
                float v = seg_angle(q,exclude[jj+1],anchor,result);
                float s, t;
                if (result > 0) {
                    s = u < v ? u : v;
                    t = u > v ? u : v;
                } else {
                    s = u > v ? u : v;
                    t = u < v ? u : v;
                }
                if (fabsf(s) < EPSILON) {
                    s = 0;
                }
                
                // Check if degenerate
                if (fabsf(t-s) > M_PI) {
                    if (s != 0 || t*result < 0) {
                        result = 0;
                    }
                } else if (result > 0 && result > s) {
                    result = s;
                } else if (result < 0 && result < s) {
                    result = s;
                }
            }
        }
        a = b;
    }
    
    return result;
}

/**
 * Clamps an attempted zoom so that it stays within bounds.
 *
 * This function compares a (transformed) contents against a viewport.
 * It assumes the contents are in bounds before a pan transformation.
 * It then checks if the given scaling factor keeps the contents in
 * bounds. If not, it returns the closest legal zoom (which may be 1)
 * that keeps the contents in bounds.
 *
 * @param viewport  The viewport to show the content
 * @param contents  The backing window of contents
 * @param transform The transform for the backing window
 * @param anchor    The origin for the zooming operation
 * @param scale     The scaling factor for the zoom
 *
 * @return the amount zoomed
 */
static float clamp_zoom(const Rect& viewport, const Rect& contents,
                        const Affine2& transform, const Vec2& anchor, float scale) {
    if (scale > 1) {
        return scale;
    }

    float result = scale;
    for(int ii = 0; ii < 4; ii++) {
        Vec2 v = rect_corner(viewport, ii);

        Vec2 a = rect_corner(contents,3)*transform;
        for(int jj = 0; jj < 4; jj++) {
            Vec2 b = rect_corner(contents,jj)*transform;
            Vec2 q = anchor;
            Vec2 p;
            if (ray_intersect(a, b, q, v, &p)) {
                // Find the percentage
                float r = q.distance(p);
                float s = q.distance(v);
                float percent = 0;
                if (s > 0) {
                    percent = (r*result)/s;
                }
                if (percent == 0 || s == 0) {
                    result = 1;
                } else if (percent < 1) {
                    result = s/r;
                }
            }
            a = b;
        }
    }

    return result;
}

/**
 * Clamps an attempted pan so that it stays within bounds.
 *
 * This function compares a (transformed) contents against a viewport.
 * It assumes the contents are in bounds before a pan transformation.
 * It then checks if the given offset keeps the contents in bounds.
 * If not, it returns the closest legal pan (which may be zero) that
 * keeps the contents in bounds.
 *
 * This version of the clamp assumes that the content has not been
 * spun or zoomed. It is slightly faster than {@link #clamp_pan2}.
 *
 * @param viewport  The viewport to show the content
 * @param contents  The backing window of contents
 * @param transform The transform for the backing window
 * @param offset    The pan offset to attempt
 *
 * @return the amount panned
 */
static Vec2 clamp_pan1(const Rect& viewport, const Rect& contents,
                       const Affine2& transform, const Vec2& offset) {
    Vec2 result = offset;
    Vec2 orig(transform.m[4],transform.m[5]);
    Vec2 bl = contents.origin+orig+offset;
    Vec2 tr = bl+contents.size;
    Vec2 vbl = viewport.origin;
    Vec2 vtr = vbl+viewport.size;
    if (bl.x > vbl.x) {
        result.x -= (bl.x-vbl.x);
    } else if (tr.x < vtr.x) {
        result.x += (vtr.x-tr.x);
    }
    if (bl.y > vbl.y) {
        result.y -= (bl.y-vbl.y);
    } else if (tr.y < vtr.y) {
        result.y += (vtr.y-tr.y);
    }
    return result;
}

/**
 * Clamps an attempted pan so that it stays within bounds.
 *
 * This function compares a (transformed) contents against a viewport.
 * It assumes the contents are in bounds before a pan transformation.
 * It then checks if the given offset keeps the contents in bounds.
 * If not, it returns the closest legal pan (which may be zero) that
 * keeps the contents in bounds.
 *
 * This version of the clamps works even if the content has been zoomed
 * or spun about.
 *
 * @param viewport  The viewport to show the content
 * @param contents  The backing window of contents
 * @param transform The transform for the backing window
 * @param offset    The pan offset to attempt
 *
 * @return the amount panned
 */
static Vec2 clamp_pan2(const Rect& viewport, const Rect& contents,
                       const Affine2& transform, const Vec2& offset) {
    Vec2 result = offset;
    Affine2 inverse = transform.getInverse();
    Vec2 shifted;
    for (int ii = 0; ii < 4; ii++) {
        Vec2 corner = rect_corner(viewport,ii);
        Affine2::transform(inverse, corner-result, &shifted);
        bool alter = false;
        if (shifted.x < contents.origin.x) {
            shifted.x = contents.origin.x;
            alter = true;
        } else if (shifted.x > contents.origin.x+contents.size.width) {
            shifted.x = contents.origin.x+contents.size.width;
            alter = true;
        }
        if (shifted.y < contents.origin.y) {
            shifted.y = contents.origin.y;
            alter = true;
        } else if (shifted.y > contents.origin.y+contents.size.height) {
            shifted.y = contents.origin.y+contents.size.height;
            alter = true;
        }
        if (alter) {
            Affine2::transform(transform, shifted, &result);
            result = corner-result;
        }
    }
    
    return result;
}

#pragma mark -
#pragma mark Constructors
/**
 * Creates an uninitialized node.
 *
 * You must initialize this Node before use.
 *
 * NEVER USE A CONSTRUCTOR WITH NEW. If you want to allocate a Node on the
 * heap, use one of the static constructors instead.
 */
ScrollPane::ScrollPane() :
_zoommin(ZOOM_MIN),
_zoommax(ZOOM_MAX),
_zoomamt(1.0f),
_panemask(nullptr),
_constrained(true),
_reoriented(false),
_simple(true) {
    _panetrans.setIdentity();
    _classname = "ScrollPane";
}

/**
 * Disposes all of the resources used by this node.
 *
 * A disposed Node can be safely reinitialized. Any children owned by this
 * node will be released.  They will be deleted if no other object owns them.
 *
 * It is unsafe to call this on a Node that is still currently inside of
 * a scene graph.
 */
void ScrollPane::dispose() {
    _zoommin = ZOOM_MIN;
    _zoommax = ZOOM_MAX;
    _zoomamt = 1.0f;
    _panetrans.setIdentity();
    _constrained = true;
    _reoriented = false;
    _simple = true;
    _panemask = nullptr;
    SceneNode::dispose();
}

/**
 * Initializes a node with the given size.
 *
 * The size defines the content size. The bounding box of the node is
 * (0,0,width,height) and is anchored in the bottom left corner (0,0).
 * The node is positioned at the origin in parent space.
 *
 * The interior bounds will be equivalent to the content bounds. That
 * means that no scrolling can happen until {@link #setInterior} is
 * called. In addition, masking will be turned off and the interior
 * is unconstrained.
 *
 * @param size  The size of the node in parent space
 *
 * @return true if initialization was successful.
 */
bool ScrollPane::initWithBounds(const Size size) {
    if (SceneNode::initWithBounds(size)) {
        _interior.size = size;
        _constrained = false;
        _panemask = nullptr;
        return true;
    }
    return false;

}

/**
 * Initializes a node with the given bounds.
 *
 * The rectangle origin is the bottom left corner of the node in parent
 * space, and corresponds to the origin of the Node space. The size
 * defines its content width and height in node space. The node anchor
 * is placed in the bottom left corner.
 *
 * The interior bounds will be equivalent to the content bounds. That
 * means that no scrolling can happen until {@link #setInterior} is
 * called. In addition, masking will be turned off and the interior
 * is unconstrained.
 *
 * @param rect  The bounds of the node in parent space
 *
 * @return true if initialization was successful.
 */
bool ScrollPane::initWithBounds(const Rect rect) {
    if (SceneNode::initWithBounds(rect)) {
        _interior.size = rect.size;
        _constrained = false;
        _panemask = nullptr;
        return true;
    }
    return false;
}

/**
 * Initializes a node with the given size and interior
 *
 * The size defines the content size. The bounding box of the node is
 * (0,0,width,height) and is anchored in the bottom left corner (0,0).
 * The node is positioned at the origin in parent space.
 *
 * The interior bounds will be as given. This interior will be placed
 * in the default orientation as defined by {@link #resetPane}. The
 * interior will start off as constrained unless this is impossible
 * (e.g. the interior is smaller than the content bounds).
 *
 * The optional masking value can be set to false to allow the area
 * outside of the content bounds to be visible.
 *
 * @param size      The size of the node in parent space
 * @param interior  The bounds of the interior in node space
 * @param mask      Whether to mask contents outside of node bounds
 *
 * @return true if initialization was successful.
 */
bool ScrollPane::initWithInterior(const Size size, const Rect interior, bool mask) {
    if (SceneNode::initWithBounds(size)) {
        _interior = interior;
        _constrained = true;
        resetPane();
        setMasked(mask);
        return true;
    }
    return false;
}

/**
 * Initializes a node with the given bounds.
 *
 * The rectangle origin is the bottom left corner of the node in parent
 * space, and corresponds to the origin of the Node space. The size
 * defines its content width and height in node space. The node anchor
 * is placed in the bottom left corner.
 *
 * The interior bounds will be as given. This interior will be placed
 * in the default orientation as defined by {@link #resetPane}. The
 * interior will start off as constrained unless this is impossible
 * (e.g. the interior is smaller than the content bounds).
 *
 * The optional masking value can be set to false to allow the area
 * outside of the content bounds to be visible.
 *
 * @param bounds    The bounds of the node in parent space
 * @param interior  The bounds of the interior in node space
 * @param mask      Whether to mask contents outside of node bounds
 *
 * @return true if initialization was successful.
 */
bool ScrollPane::initWithInterior(const Rect bounds, const Rect interior, bool mask) {
    if (SceneNode::initWithBounds(bounds)) {
        _interior = interior;
        _constrained = true;
        resetPane();
        setMasked(mask);
        return true;
    }
    return false;
}


/**
 * Initializes a node with the given JSON specificaton.
 *
 * This initializer is designed to receive the "data" object from the
 * JSON passed to {@link Scene2Loader}. This JSON format supports all
 * of the attribute values of its parent class. In addition, it supports
 * the following additional attributes:
 *
 *      "interior":  A two or four-element number array for the interior bounds.
 *                   A two-element array indicates the bounds start at the origin.
 *      "constrain": A boolean value, indicating whether to keep the interior in bounds.
 *      "mask":      A boolean value, indicating whether to hide out-of-bounds contents.
 *      "pan":       A two-element number array, representing the initial pan offset.
 *      "spin"       A float represeting the initial spin angle in radians.
 *      "zoom"       A float representing the initial zoom factor.
 *      "zoom max":  A float representing the maximum supported cumulative zoom.
 *      "zoom min":  A float representing the minimum supported cumulative zoom.
 *
 * All attributes are optional. There are no required attributes. If any scroll
 * adjustments are set (pan, spin, zoom) they will be applied in the following
 * order: spin, then zoom, then pan.
 *
 * Note that if the interior size and/or position pushes it outside of the content
 * bounds, then the constrain setting will be ignored and set to false.
 *
 * @param loader    The scene loader passing this JSON file
 * @param data      The JSON object specifying the node
 *
 * @return true if initialization was successful.
 */
bool ScrollPane::initWithData(const Scene2Loader* loader, const std::shared_ptr<JsonValue>& data) {
    if (SceneNode::initWithData(loader, data)) {
        if (data->has("interior")) {
            JsonValue* rect = data->get("interior").get();
            CUAssertLog(rect->size() == 2 || rect->size() == 4, "'interior' must be a two or four element number array");
            if (rect->size() == 2) {
                _interior.size.width  = rect->get(0)->asFloat(0.0f);
                _interior.size.height = rect->get(1)->asFloat(0.0f);
            } else if (rect->size() == 4) {
                _interior.origin.x = rect->get(0)->asFloat(0.0f);
                _interior.origin.y = rect->get(1)->asFloat(0.0f);
                _interior.size.width  = rect->get(2)->asFloat(0.0f);
                _interior.size.height = rect->get(3)->asFloat(0.0f);
            }
        } else {
            _interior.size = getSize();
        }

        if (data->has("constrain")) {
            _constrained = data->getBool("constrain",true);
        }

        if (data->has("mask")) {
            setMasked(data->getBool("mask",false));
        } else {
            setMasked(false);
        }

        if (data->has("zoom min")) {
            setMinZoom(data->getFloat("zoom min",ZOOM_MIN));
        }

        if (data->has("zoom max")) {
            setMaxZoom(data->getFloat("zoom max",ZOOM_MAX));
        }

        Vec2 pan;
        if (data->has("pan")) {
            JsonValue* pos = data->get("pan").get();
            CUAssertLog(pos->size() == 2, "'pan' must be a two element number array");
            pan.x = pos->get(0)->asFloat(0.0f);
            pan.y = pos->get(1)->asFloat(0.0f);
        }
        
        float zoom = 1.0;
        if (data->has("zoom")) {
            zoom = data->getFloat("zoom",1.0);
            if (zoom < _zoommin) {
                zoom = _zoommin;
            } else if (zoom > _zoommax) {
                zoom = _zoommax;
            }
        }

        float spin = 0.0;
        if (data->has("spin")) {
            zoom = data->getFloat("spin",0.0);
        }

        applySpin(spin);
        applyZoom(zoom);
        applyPan(pan);
        
        // Check if constrained is violated
        if (_constrained) {
            Affine2 inverse = _panetrans.getInverse();
            Rect viewport(Vec2::ZERO,_contentSize);
            Vec2 shifted;
            bool valid = true;
            for (int ii = 0; valid && ii < 4; ii++) {
                Vec2 corner = rect_corner(viewport,ii);
                Affine2::transform(inverse, corner, &shifted);
                if (shifted.x < _interior.origin.x) {
                    valid = false;
                } else if (shifted.x > _interior.size.width) {
                    valid = false;
                }
                if (shifted.y < _interior.origin.y) {
                    valid = false;
                } else if (shifted.y > _interior.size.height) {
                    valid = false;
                }
            }
            _constrained = valid;
        }
        return true;
    }
    return false;
}


#pragma mark -
#pragma mark Attributes
/**
 * Sets the interior bounds of this scroll pane.
 *
 * A scroll pane is essentially a view port that shows a portion of
 * a content area. This content area is defined by the interior
 * rectangle. All children positions are defined relative to this
 * content area and not the node itself.
 *
 * If the node is constrained, it will never show any area outside of
 * the interior bounds. Any attempts to pan, spin, or zoom the interior
 * in such a way to violate this are prohibited. For example, if a pan
 * pushes the interior so far that an edge of the interior would be
 * showing in the content bounds, then this pan will be aborted.
 *
 * If the interior position would force a constrained node to show
 * any out-of-bounds content (e.g. the interior does not properly
 * contain all four corners of the content bounds), then the pan value
 * will be adjusted so that the bottom left corner is at the origin.
 * If the interior is still too small to cover the content bounds then
 * this method will set constrained to false.
 */
void ScrollPane::setInterior(const Rect& bounds) {
    _interior = bounds;
    if (_layout) {
        doLayout();
    }
    resetPane();
}

/**
 * Sets the constrained attribute, returning true on success.
 *
 * A constrained scroll pane will never show any area outside of the
 * interior bounds. Any attempts to pan, spin, or zoom the interior in
 * such a way to violate this are prohibited. For example, if a pan
 * pushes the interior so far that an edge of the interior would be
 * showing in the content bounds, then this pan will be aborted.
 *
 * If the scroll pane is currently showing any out-of-bound areas
 * (e.g. the interior does not properly contain all four corners of
 * the content bounds), then the interior will be reset to its
 * untransformed position.  If this still shows any out-of-bound
 * areas (e.g. the interior is too small), then this method will
 * fail.
 *
 * @param value Whether to constrain this scroll pane
 *
 * @return true if the constrained attribute was changed
 */
bool ScrollPane::setConstrained(bool value) {
    bool oldval = _constrained;
    _constrained = value;
    if (value && value != oldval) {
        resetPane();
    }
    return oldval != value;
}

/**
 * Sets whether this scroll pane is masked.
 *
 * A masked scroll pane will never show any content outside of the
 * content bounds. This node will activate an implicit scissor whose
 * bounds are the size of the content bounds to avoid drawing this
 * content.
 *
 * A scroll pane has only one scissor at a given time. So if a scissor
 * is masked, then it will ignore the {@link SceneNode#getScissor}
 * attribute defined in its parent class. The scissor should be unmasked
 * if you want to support a custom scissor. Setting this value to true
 * will only ignore the custom scissor; it will not erase it.
 *
 * @return value    Whether this scroll pane is masked.
 */
void ScrollPane::setMasked(bool value) {
    if (value) {
        _panemask = Scissor::alloc(_contentSize);
    } else {
        _panemask = nullptr;
    }
}

/**
 * Attempts to set the minimum supported cumulative zoom
 *
 * A zoom factor is a scaling factor. Values greater than 1
 * zoom in, enlarging the content. Values less than 1 (but
 * still postive) zoom out, shrinking the content. Negative
 * zoom values are not supported.
 *
 * At all times, the scroll pane keeps track of the cumulative
 * zoom in or out. This cumulative zoom must stay always be
 * greater than the minimum zoom. Attempts to zoom further out
 * than this amount will be denied.
 *
 * This setter will fail if either the minimum zoom factor is
 * negative, or the value is greater than the current cumulative
 * zoom in use.
 *
 * @param value The minimum supported cumulative zoom
 *
 * @return true If this minimum zoom successfully changed
 */
bool ScrollPane::setMinZoom(float scale) {
    CUAssertLog(scale > 0, "minimum zoom must be positive");
    if (scale < 0) {
        return false;
    } else if (scale < _zoomamt) {
        _zoommin = scale;
        return true;
    }
    return false;
}

/**
 * Attempts to set the maximum supported cumulative zoom
 *
 * A zoom factor is a scaling factor. Values greater than 1
 * zoom in, enlarging the content. Values less than 1 (but
 * still postive) zoom out, shrinking the content. Negative
 * zoom values are not supported.
 *
 * At all times, the scroll pane keeps track of the cumulative
 * zoom in or out. This cumulative zoom must stay always be
 * less than the maximum zoom. Attempts to zoom further in
 * than this amount will be denied.
 *
 * This setter will fail if the value is less than the current
 * cumulative zoom in use.
 *
 * @param value The maximum supported cumulative zoom
 *
 * @return true If this maximum zoom successfully changed
 */
bool ScrollPane::setMaxZoom(float scale) {
    if (scale > _zoomamt) {
        _zoommax = scale;
        return true;
    }
    return false;
}

#pragma mark -
#pragma mark Navigation
/**
 * Attempts to apply the given pan to the scroll pane.
 *
 * The pan is an offset value applied to the interior bounds.
 * Pan values are applied incrementally. You cannot set the
 * absolute pan, as it is impossible to do this while supporting
 * both zooming and spinning.  If you want to set an absolute
 * pan, you should call {@link #resetPane} and then apply the
 * pan.
 *
 * If the scroll pane is constrained, then any pan that would
 * show edges of the interior (such as going too far to the left
 * or the right) will not be allowed. Instead, it is replaced with
 * the closest legal pan (which is returned).
 *
 * @param delta The pan offset to apply
 *
 * @return the actual pan applied
 */
Vec2 ScrollPane::applyPan(const Vec2& delta) {
    _reoriented = true;
    Vec2 result = delta;
    if (_constrained) {
        Rect view(Vec2::ZERO,_contentSize);
        if (_simple) {
            result = clamp_pan1(view, _interior, _panetrans, delta);
            _panetrans.translate(result);
        } else {
            result = clamp_pan2(view, _interior, _panetrans, delta);
            _panetrans.translate(result);
        }
    } else {
        _panetrans.translate(delta);
    }
    return result;
}

/**
 * Attempts to apply the given pan to the scroll pane.
 *
 * The pan is an offset value applied to the interior bounds.
 * Pan values are applied incrementally. You cannot set the
 * absolute pan, as it is impossible to do this while supporting
 * both zooming and spinning.  If you want to set an absolute
 * pan, you should call {@link #resetPane} and then apply the
 * pan.
 *
 * If the scroll pane is constrained, then any pan that would
 * show edges of the interior (such as going too far to the left
 * or the right) will not be allowed. Instead, it is replaced with
 * the closest legal pan (which is returned).
 *
 * @param dx    The x offset to apply
 * @param dy    The y offset to apply
 *
 * @return the actual pan applied
 */
Vec2 ScrollPane::applyPan(float dx, float dy) {
    Vec2 delta(dx,dy);
    return applyPan(delta);
}

/**
 * Attempts to apply the given spin to the scroll pane.
 *
 * The spin is an angle of rotation. The angle is measured in
 * radius, just like {@link SceneNode#getRotation}. However,
 * the angle is applied to the interior content, and not the
 * scroll pane itself.
 *
 * If the scroll pane is constrained, then any spin angle that
 * would show edges of the interior (typically when the size
 * of the interior is smaller than the circumscribed circle
 * containing the content bounds) will not be allowed. Instead,
 * it is replaced with the closest legal angle in the same
 * direction (which is returned).
 *
 * The value {@link SceneNode#getAnchor} defines the origin of
 * the spin. The input handler should reset the anchor to spin
 * about different portions of the content region.
 *
 * @param spin  The spin angle in radians
 *
 * @return the actual spin applied
 */
float ScrollPane::applySpin(float angle) {
    _reoriented = true;
    _simple = false;
    Vec2 center = _anchor*_contentSize;
    if (_constrained) {
        Rect view(Vec2::ZERO,_contentSize);
        angle = clamp_spin(view, _interior, _panetrans, center, angle);
    }
    _panetrans.translate(-center.x, -center.y);
    _panetrans.rotate(angle);
    _panetrans.translate(center.x, center.y);
    return angle;
}

/**
 * Attempts to apply the given zoom to the scroll pane.
 *
 * A zoom factor is a scaling factor. Values greater than 1
 * zoom in, enlarging the content. Values less than 1 (but
 * still postive) zoom out, shrinking the content. Negative
 * zoom values are not supported.
 *
 * If the scroll pane is constrained, then any zoom factor
 * that would show edges of the interior (typically when
 * zooming out) will not be allowed. Instead, it is replaced
 * with the closest legal zoom factor (which is returned).
 *
 * At all times, the scroll pane keeps track of the cumulative
 * zoom in or out. This cumulative zoom must stay with the
 * range of {@link #getMinZoomMin} and {@link #getMaxZoom}.
 * If it were to go outside this range, it is replaced with
 * the closest legal zoom factor (which is returned).
 *
 * The value {@link SceneNode#getAnchor} defines the origin of
 * the zoom. The input handler should reset the anchor to zoom
 * in on different portions of the content region.
 *
 * @param zoom  The zoom factor
 *
 * @return the actual zoom factor applied
 */
float ScrollPane::applyZoom(float scale) {
    _reoriented = true;
    _simple = false;
    Vec2 center = _anchor*_contentSize;
    if (_constrained) {
        Rect view(Vec2::ZERO,_contentSize);
        scale = clamp_zoom(view, _interior, _panetrans, center, scale);
    }
    
    // Enforce zoom limits
    float total = _zoomamt*scale;
    if (total < _zoommin) {
        scale = _zoommin/_zoomamt;
    } else if (total > _zoommax) {
        scale = _zoommax/_zoomamt;
    }
    
    _zoomamt *= scale;
    _panetrans.translate(-center.x, -center.y);
    _panetrans.scale(scale,scale);
    _panetrans.translate(center.x, center.y);
    return scale;
}

/**
 * Resets the interior to the default position.
 *
 * The default position is typically the untransformed interior,
 * meaing that {@link #getPaneTransform} returns the identity.
 * However, if the pane is constrained, a small or badly-positioned
 * interior may cause some out-of-bounds areas to be shown. In
 * that case, this method will pan (not zoom or spin) the interior
 * so that it is in bounds.
 *
 * If this cannot be accomplised (because the interior is too small),
 * then this method will set the constrained attribute to false.
 */
void ScrollPane::resetPane() {
    _panetrans.setIdentity();
    if (_interior.size.width < _contentSize.width ||
        _interior.size.height < _contentSize.height) {
        _constrained = false;
    } else if (_constrained) {
        Vec2 offset;
        if (_interior.origin.x > 0) {
            offset.x = -_interior.origin.x;
        } else if (_interior.origin.x+_interior.size.width < _contentSize.width) {
            offset.x = _contentSize.width-_interior.origin.x-_interior.size.width;
        }
        
        if (_interior.origin.y > 0) {
            offset.y = -_interior.origin.y;
        } else if (_interior.origin.y+_interior.size.height < _contentSize.height) {
            offset.y = _contentSize.height-_interior.origin.y-_interior.size.height;
        }
        
        _panetrans.translate(offset);
    }
}
    
#pragma mark -
#pragma mark Rendering
/**
 * Draws this Node and all of its children with the given SpriteBatch.
 *
 * You almost never need to override this method.  You should override the
 * method draw(shared_ptr<SpriteBatch>,const Affine2&,Color4) if you need to
 * define custom drawing code. In fact, overriding this method can break
 * the functionality of {@link OrderedNode}.
 *
 * @param batch     The SpriteBatch to draw with.
 * @param transform The global transformation matrix.
 * @param tint      The tint to blend with the Node color.
 */
void ScrollPane::render(const std::shared_ptr<SpriteBatch>& batch, const Affine2& transform, Color4 tint) {
    if (!_isVisible) { return; }
    
    Affine2 matrix;
    Affine2::multiply(_combined,transform,&matrix);
    Color4 color = _tintColor;
    if (_hasParentColor) {
        color *= tint;
    }
    
    std::shared_ptr<Scissor> active = batch->getScissor();
    if (_panemask) {
        std::shared_ptr<Scissor> local = Scissor::alloc(_panemask);
        local->multiply(matrix);
        if (active) {
            local->intersect(active);
        }
        batch->setScissor(local);
    } else if (_scissor) {
        std::shared_ptr<Scissor> local = Scissor::alloc(_scissor);
        local->multiply(matrix);
        if (active) {
            local->intersect(active);
        }
        batch->setScissor(local);
    }

    draw(batch,matrix,color);
    Affine2::multiply(_panetrans,matrix,&matrix);
    for(auto it = _children.begin(); it != _children.end(); ++it) {
        (*it)->render(batch, matrix, color);
    }

    if (_scissor) {
        batch->setScissor(active);
    }
}
