//
//  CUWireNode.cpp
//  Cornell University Game Library (CUGL)
//
//  This module provides a scene graph node that supports wireframes.  The
//  primary use case is to have a node that outlines physics bodies.
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
//  Version: 7/15/21
//
#include <cugl/scene2/graph/CUWireNode.h>
#include <cugl/assets/CUScene2Loader.h>
#include <cugl/assets/CUAssetManager.h>
#include <cugl/math/polygon/CUEarclipTriangulator.h>
#include <cugl/render/CUGradient.h>

using namespace cugl;
using namespace cugl::scene2;

/** For handling JSON issues */
#define UNKNOWN_STR "<unknown>"

#pragma mark Constructors
/**
 * Initializes a wire frame with the given polygon.
 *
 * This wireframe will perform an INTERIOR traversal of the given polygon.
 * This assumes that the polygon has been triangulated previously.
 *
 * You do not need to set the texture; rendering this into a SpriteBatch
 * will simply use the blank texture. Hence the wireframe will have a solid
 * color.
 *
 * @param poly  The polygon to traverse
 *
 * @return true if the wireframe is initialized properly, false otherwise.
 */
bool WireNode::initWithPoly(const Poly2& poly) {
    return initWithTraversal(poly,poly2::Traversal::INTERIOR);
}

/**
 * Initializes a wire frame with the given (solid) rectangle.
 *
 * This wireframe will perform an INTERIOR traversal of the standard
 * triangulation of this rectangle.
 *
 * You do not need to set the texture; rendering this into a SpriteBatch
 * will simply use the blank texture. Hence the wireframe will have a solid
 * color.
 *
 * @param rect  The rectangle to traverse
 *
 * @return true if the wireframe is initialized properly, false otherwise.
 */
bool WireNode::initWithPoly(const Rect rect) {
    return initWithTraversal(Poly2(rect),poly2::Traversal::INTERIOR);
}

/**
 * Initializes a wire frame with the given path.
 *
 * This wireframe will perform a traversal of the path.  The traversal will
 * either be OPEN or CLOSED depending upon the properties of the path.
 *
 * You do not need to set the texture; rendering this into a SpriteBatch
 * will simply use the blank texture. Hence the wireframe will have a solid
 * color.
 *
 * @param path  The path to traverse
 *
 * @return true if the wireframe is initialized properly, false otherwise.
 */
bool WireNode::initWithPath(const Path2& path) {
    if (TexturedNode::initWithTexture(nullptr)) {
        setPath(path);
        return true;
    }
    return false;
}

/**
 * Initializes a wire frame with the given rectangle (outline).
 *
 * This wireframe will perform a CLOSED traversal of the rectangle.
 *
 * You do not need to set the texture; rendering this into a SpriteBatch
 * will simply use the blank texture. Hence the wireframe will have a solid
 * color.
 *
 * @param rect  The rectangle to traverse
 *
 * @return true if the wireframe is initialized properly, false otherwise.
 */
bool WireNode::initWithPath(const Rect rect) {
    return initWithPath(Path2(rect));
}

/**
 * Initializes a wire frame with the given path.
 *
 * This wireframe will perform a CLOSED traversal of the given path.
 *
 * You do not need to set the texture; rendering this into a SpriteBatch
 * will simply use the blank texture. Hence the wireframe will have a solid
 * color.
 *
 * @param vertices  The path to traverse
 *
 * @return true if the wireframe is initialized properly, false otherwise.
 */
bool WireNode::initWithPath(const std::vector<Vec2>& vertices) {
    if (TexturedNode::initWithTexture(nullptr)) {
        _traversal = poly2::Traversal::CLOSED;
        setPath(vertices);
        return true;
    }
    return false;
}

/**
 * Intializes a wire frame with the given polygon and traversal
 *
 * The provided polygon will be used as the source for the traversal.
 * The traversal will be applied to the vertices (and potentially
 * indices) of thei polygon.
 *
 * You do not need to set the texture; rendering this into a SpriteBatch
 * will simply use the blank texture. Hence the wireframe will have a solid
 * color.
 *
 * @param poly      The triangulated polygon
 * @param traversal The path traversal for index generation
 *
 * @return true if the wireframe is initialized properly, false otherwise.
 */
bool WireNode::initWithTraversal(const Poly2& poly, poly2::Traversal traversal) {
    if (TexturedNode::initWithTexture(nullptr)) {
        _traversal = traversal;
        _polygon = poly;
        makeTraversal(poly, _traversal);
        return true;
    }
    return false;
}

/**
 * Intializes a wire frame with the given vertices and indices
 *
 * This initializer will set the given vertices to of the polygon to be
 * the one specified.  However, it will not triangulate the polygon, or
 * attempt to traverse it.  Instead, it will use the provided indices as
 * the final traversal. Hence this is a way of creating a custom traversal.
 * To work properly, the indices should have an even number of elements
 * and define a sequence of line segments.
 *
 * You do not need to set the texture; rendering this into a SpriteBatch
 * will simply use the blank texture. Hence the wireframe will have a solid
 * color.
 *
 * @param vertices  The vertices to texture (expressed in image space)
 * @param indices   The traversal indices
 *
 * @return true if the wireframe is initialized properly, false otherwise.
 */
bool WireNode::initWithTraversal(const std::vector<Vec2>& vertices, const std::vector<Uint32>& indices) {
    if (TexturedNode::initWithTexture(nullptr)) {
        _traversal = poly2::Traversal::NONE;
        _polygon.set(vertices);
        _indices = indices;
        return true;
    }
    return false;
}

/**
 * Intializes a wire frame with the given vertices and indices
 *
 * This initializer will set the given vertices to of the polygon to be
 * the one specified.  However, it will not triangulate the polygon, or
 * attempt to traverse it.  Instead, it will use the provided indices as
 * the final traversal. Hence this is a way of creating a custom traversal.
 * To work properly, the indices should have an even number of elements
 * and define a sequence of line segments.
 *
 * You do not need to set the texture; rendering this into a SpriteBatch
 * will simply use the blank texture. Hence the wireframe will have a solid
 * color.
 *
 * @param vertices  The vertices to texture (expressed in image space)
 * @param vsize     The number of vertices
 * @param indices   The traversal indices
 * @param isize     The number of indices
 *
 * @return true if the wireframe is initialized properly, false otherwise.
 */
bool WireNode::initWithTraversal(Vec2* vertices, size_t vsize, Uint32* indices, size_t isize) {
    if (TexturedNode::initWithTexture(nullptr)) {
        _traversal = poly2::Traversal::NONE;
        _polygon.set(vertices,vsize);
        _indices.insert(_indices.begin(), indices, indices+isize);
        return true;
    }
    return false;
}

/**
 * Returns a newly allocated wire frame with the given JSON specificaton.
 *
 * This initializer is designed to receive the "data" object from the
 * JSON passed to {@link Scene2Loader}.  This JSON format supports all
 * of the attribute values of its parent class.  In addition, it supports
 * the following additional attributes:
 *
 *      "traversal": One of 'none', 'open', 'closed', or 'interior'
 *      "polygon":   A JSON object defining a polygon. See {@link Poly2}.
 *      "wireframe": An even array of numbers defining the wireframe indices.
 *
 * All attributes are optional. If the polygon is not specified, the node will
 * use a rectangle with the dimensions of the texture. For more information,
 * see {@link TexturedNode#initWithData}.
 *
 * If you do specify the wire frame, the traversal algorithm will be ignored.
 * If both the wireframe and the traversal algorithm are omitted, this will
 * perform an INTERIOR traversal on the polygon.
 *
 * @param loader    The scene loader passing this JSON file
 * @param data      The JSON object specifying the node
 *
 * @return a newly allocated wire frame with the given JSON specificaton.
 */
bool WireNode::initWithData(const Scene2Loader* loader, const std::shared_ptr<JsonValue>& data) {
    if (_texture != nullptr) {
        CUAssertLog(false, "%s is already initialized",_classname.c_str());
        return false;
    } else if (!data) {
        return init();
    } else if (!TexturedNode::initWithData(loader, data)) {
        return false;
    }
    
    // All of the code that follows can corrupt the position.
    Vec2 coord = getPosition();

    // If size was set explicitly, we will need to restore it after polygon
    bool sizefit = data->has("size");
    Size size = getSize();
    
    if (data->has("polygon")) {
        _polygon.set(data->get("polygon"));
    } else {
        Rect bounds = Rect::ZERO;
        if (_texture != nullptr) {
            bounds.size = _texture->getSize();
        } else {
            bounds.size = getContentSize();
        }
        _polygon.set(bounds);
    }
    setContentSize(_polygon.getBounds().size);
    
    if (data->has("wireframe")) {
        JsonValue* inds = data->get("wireframe").get();
        _indices.reserve(inds->size());
        for(Uint32 ii = 0; ii < inds->size(); ii++) {
            _indices.push_back(inds->get(ii)->asInt(0));
        }
    } else {
        std::string traverse = data->getString("traversal",UNKNOWN_STR);
        if (traverse == "open") {
            _traversal = poly2::Traversal::OPEN;
        } else if (traverse == "closed") {
            _traversal = poly2::Traversal::CLOSED;
        } else if (traverse == "interior") {
            _traversal = poly2::Traversal::INTERIOR;
        } else {
            _traversal = poly2::Traversal::NONE;
        }
        makeTraversal(_polygon, _traversal);
    }
    
    // Redo the size if necessary
    if (sizefit) {
        setContentSize(size);
    }
    
    // Now redo the position
    setPosition(coord);
    return true;
}

/**
 * Performs a shallow copy of this Node into dst.
 *
 * No children from this node are copied, and no children of dst are
 * modified. In addition, the parents of both Nodes are unchanged. However,
 * all other attributes of this node are copied.
 *
 * @param dst   The Node to copy into
 *
 * @return A reference to dst for chaining.
 */
std::shared_ptr<SceneNode> WireNode::copy(const std::shared_ptr<SceneNode>& dst) const {
    TexturedNode::copy(dst);
    std::shared_ptr<WireNode> node = std::dynamic_pointer_cast<WireNode>(dst);
    if (node) {
        node->_polygon = _polygon;
        node->_indices = _indices;
        node->_traversal = _traversal;
    }
    return dst;
}

#pragma mark -
#pragma mark Vertices
/**
 * Sets the wire frame polygon to the given one.
 *
 * The provided polygon will be used as the source for the traversal.
 * If the traversal algorithm is not NONE, setting this value will
 * generate a new set of traversal indices.
 *
 * The polygon is specified in image coordinates. The origin is at the
 * bottom-left corner of the image, and each pixel is one unit. If no
 * texture is specified, this node will draw the wire frame with a
 * solid color.
 *
 * It is not necessary for this polygon to be triangulated (e.g. have
 * indices).  A triangulation is only necessary for an INTERIOR
 * traversal.
 *
 * @param poly  The wire frame polygon
 */
void WireNode::setPolygon(const Poly2& poly) {
    _polygon = poly;
    if (_traversal != poly2::Traversal::NONE) {
        makeTraversal(poly, _traversal);
    }
    clearRenderData();
}

/**
 * Sets the wire frame polygon to the given (solid) rect.
 *
 * The rectangle will be converted into a Poly2, with the traditional
 * two-element triagulation. If the traversal algorithm is not NONE,
 * setting this value will generate a new set of traversal indices.
 * traversal method.
 *
 * The rectangle is specified in image coordinates. The origin is at the
 * bottom-left corner of the image, and each pixel is one unit. If no
 * texture is specified, this node will draw the wire frame with a
 * solid color.
 *
 * @param rect  The wire frame rectangle
 */
void WireNode::setPolygon(const Rect rect) {
    _polygon = rect;
    if (_traversal != poly2::Traversal::NONE) {
        makeTraversal(_polygon, _traversal);
    }
    clearRenderData();
}

/**
 * Sets the wire frame polygon to the given path.
 *
 * The resulting polygon will not have an triangulation vertices, so
 * any INTERIOR travesal will fail (generate no indices).  However,
 * any other traversal will work normally. In addition, setting this
 * value will change the traversal algorithm to either OPEN or CLOSED,
 * depending on the nature of the path.
 *
 * The path is specified in image coordinates. The origin is at the
 * bottom-left corner of the image, and each pixel is one unit. If no
 * texture is specified, this node will draw the wire frame with a
 * solid color.
 *
 * @param path  The wire frame path
 */
void WireNode::setPath(const Path2& path) {
    _traversal = path.isClosed() ? poly2::Traversal::CLOSED : poly2::Traversal::OPEN;
    _polygon.vertices = path.vertices;
    _indices.reserve(2*path.size());
    for(Uint32 ii = 0; ii < path.size()-1; ii++) {
        _indices.push_back(ii  );
        _indices.push_back(ii+1);
    }
    if (path.isClosed()) {
        _indices.push_back((Uint32)path.size()-1);
        _indices.push_back(0);
    }
}

/**
 * Sets the wire frame polygon to the given rect (outline).
 *
 * The resulting polygon will not have an triangulation vertices, so
 * any INTERIOR travesal will fail (generate no indices).  However,
 * any other traversal will work normally. In addition, setting this
 * value will change the traversal algorithm to CLOSED.
 *
 * The rectangle is specified in image coordinates. The origin is at the
 * bottom-left corner of the image, and each pixel is one unit. If no
 * texture is specified, this node will draw the wire frame with a
 * solid color.
 *
 * @param rect  The wire frame rectangle
 */
void WireNode::setPath(const Rect rect) {
    setPath(Path2(rect));
}

/**
 * Sets the wire frame polygon to the given path.
 *
 * The resulting polygon will not have an triangulation vertices, so
 * any INTERIOR travesal will fail (generate no indices).  However,
 * any other traversal will work normally.
 *
 * The path is specified in image coordinates. The origin is at the
 * bottom-left corner of the image, and each pixel is one unit. If no
 * texture is specified, this node will draw the wire frame with a
 * solid color.
 *
 * @param path  The wire frame path
 */
void WireNode::setPath(const std::vector<Vec2>& vertices) {
    _traversal = poly2::Traversal::CLOSED;
    _polygon.vertices = vertices;
    _indices.reserve(2*vertices.size());
    for(Uint32 ii = 0; ii < vertices.size()-1; ii++) {
        _indices.push_back(ii  );
        _indices.push_back(ii+1);
    }
    _indices.push_back((Uint32)vertices.size()-1);
    _indices.push_back(0);
}

#pragma mark -
#pragma mark Traversals
/**
 * Sets the traversal algorithm of this wire frame.
 *
 * If the traversal algorithm is different from the current one, it will
 * recompute the traversal indices.
 *
 * @param traversal The new traversal algorithm
 */
void WireNode::setTraversal(poly2::Traversal traversal) {
    if (_traversal == traversal) {
        return;
    }
    _traversal = traversal;
    makeTraversal(_polygon, _traversal);
    clearRenderData();
}

/**
 * Sets the traversal indices of this path.
 *
 * The indices are a manual traversal of the underlying polygon. The argument
 * should contain an even number of indices, defining a sequence of line
 * segments. In addition, the indices should all represent valid vertices
 * in the polygon. However, this method provides no validation, so violating
 * these requirements causes undefined behavior.
 *
 * Setting indices manually will cause the underlying traveral algorithm to
 * be NONE.
 *
 * @param indices   A custom traversal of the polygon.
 */
void WireNode::setTraversal(const std::vector<Uint32>& indices) {
    _traversal = poly2::Traversal::NONE;
    _indices = indices;
    clearRenderData();
}

/**
 * Sets the traversal indices of this path.
 *
 * The indices are a manual traversal of the underlying polygon. The argument
 * should contain an even number of indices, defining a sequence of line
 * segments. In addition, the indices should all represent valid vertices
 * in the polygon. However, this method provides no validation, so violating
 * these requirements causes undefined behavior.
 *
 * Setting indices manually will cause the underlying traveral algorithm to
 * be NONE.
 *
 * @param indices   A custom traversal of the polygon.
 * @param isize     The number of indices
 */
void WireNode::setTraversal(Uint32* indices, size_t isize) {
    _traversal = poly2::Traversal::NONE;
    _indices.insert(_indices.begin(), indices, indices+isize);
    clearRenderData();
}

#pragma mark -
#pragma mark Rendering
/**
 * Draws this wireframe via the given SpriteBatch.
 *
 * This method only worries about drawing the current node.  It does not
 * attempt to render the children.
 *
 * @param batch     The SpriteBatch to draw with.
 * @param matrix    The global transformation matrix.
 * @param tint      The tint to blend with the Node color.
 */
void WireNode::draw(const std::shared_ptr<SpriteBatch>& batch, const Affine2& transform, Color4 tint) {
    if (!_rendered) {
        generateRenderData();
    }
    
    batch->setColor(tint);
    batch->setTexture(_texture);
    if (_gradient) {
        batch->setGradient(_gradient);
    }
    batch->setBlendEquation(_blendEquation);
    batch->setSrcBlendFunc(_srcFactor);
    batch->setDstBlendFunc(_dstFactor);
    batch->drawMesh(_mesh, transform);
    batch->setGradient(nullptr);
}

/**
 * Allocate the render data necessary to render this node.
 */
void WireNode::generateRenderData() {
    CUAssertLog(!_rendered, "Render data is already present");
    if (_texture == nullptr) {
        return;
    }
    
    // Adjust the mesh as necesary
    Size nsize = getContentSize();
    Size bsize = _polygon.getBounds().size;

    Affine2 shift;
    bool adjust = false;
    if (nsize != bsize) {
        adjust = true;
        shift.scale((bsize.width > 0 ? nsize.width/bsize.width : 0),
                    (bsize.height > 0 ? nsize.height/bsize.height : 0));
    }
    const Vec2 offset = _polygon.getBounds().origin;
    if (!_absolute && !offset.isZero()) {
        adjust = true;
        shift.translate(-offset.x,-offset.y);
    }
    
    // This time we do not have any built-in mesh generation
    _mesh.vertices.reserve(_polygon.vertices.size());
    for(auto it = _polygon.vertices.begin(); it != _polygon.vertices.end(); ++it) {
        SpriteVertex2 vert;
        vert.position = *it;
        if (adjust) {
            vert.position *= shift;
        }
        vert.color = Color4::WHITE.getPacked();
        _mesh.vertices.push_back(vert);
    }
    _mesh.indices = _indices;
    _mesh.command = GL_LINES;
    
    _rendered = true;
    updateTextureCoords();
}

/**
 * Updates the texture coordinates for this polygon
 *
 * The texture coordinates are computed assuming that the polygon is
 * defined in image space, with the origin in the bottom left corner
 * of the texture.
 */
void WireNode::updateTextureCoords() {
    if (!_rendered) {
        return;
    }
    
    Size tsize = _texture->getSize();
    Vec2 off = _offset+_polygon.getBounds().origin;
    for(auto it = _mesh.vertices.begin(); it != _mesh.vertices.end(); ++it) {
        float s = (it->position.x+off.x)/tsize.width;
        float t = (it->position.y+off.y)/tsize.height;

        if (_flipHorizontal) { s = 1-s; }
        if (!_flipVertical)  { t = 1-t; }

        it->texcoord.x = s*_texture->getMaxS()+(1-s)*_texture->getMinS();
        it->texcoord.y = t*_texture->getMaxT()+(1-t)*_texture->getMinT();
    
        if (_gradient) {
            s = (it->position.x+off.x)/_polygon.getBounds().size.width;
            t = (it->position.x+off.x)/_polygon.getBounds().size.height;

            if (_flipHorizontal) { s = 1-s; }
            if (!_flipVertical)  { t = 1-t; }
            it->gradcoord.x = s;
            it->gradcoord.y = t;
        }
    }
}

/**
 * Stores a wire frame of an existing polygon in the provided buffer.
 *
 * Traversals generate not just one path, but a sequence of paths (which
 * may all be either open or closed). This method provides four types of
 * traversals: `NONE`, `OPEN`, `CLOSED` and `INTERIOR`. The open and closed
 * traversals apply to the boundary of the polygon. If there is more than
 * one boundary, then each boundary is traversed separately.
 *
 * The interior traversal creates a wire frame a polygon triangulation.
 * That means it will generate a separate path for each triangle.
 *
 * The traversal will be appended to the buffer.  You should clear the buffer
 * first if you do not want to preserve the original data.
 *
 * @param paths The container to store the result
 * @param src   The source polygon to traverse
 * @param type  The traversal type
 *
 * @return a reference to the buffer for chaining.
 */
void WireNode::makeTraversal(const Poly2& src, poly2::Traversal type) {
    switch (type) {
        case poly2::Traversal::NONE:
            // Do nothing
            break;
        case poly2::Traversal::OPEN:
            makeBoundaryTraversal(src, false);
            break;
        case poly2::Traversal::CLOSED:
            makeBoundaryTraversal(src, true);
            break;
        case poly2::Traversal::INTERIOR:
            makeInteriorTraversal(src);
            break;
    }
}
    
/**
 * Stores a wire frame of an existing polygon in the provided buffer.
 *
 * This method is dedicated to either an `OPEN` or `CLOSED` traversal.
 * It creates a path for each boundary. These paths are either open or
 * closed as specified.
 *
 * The traversal will be appended to the buffer.  You should clear the buffer
 * first if you do not want to preserve the original data.
 *
 * @param paths     The container to store the result
 * @param src       The source polygon to traverse
 * @param closed    Whether to close the paths
 *
 * @return a reference to the buffer for chaining.
 */
void WireNode::makeBoundaryTraversal(const Poly2& src, bool closed) {
    if (src.indices.empty()) {
        _indices.reserve(2*src.vertices.size());
        for(Uint32 ii = 0; ii < src.vertices.size()-1; ii++) {
            _indices.push_back(ii  );
            _indices.push_back(ii+1);
        }
        if (closed) {
            _indices.push_back((Uint32)src.vertices.size()-1);
            _indices.push_back(0);
        }
    } else {
        std::vector<std::vector<Uint32>> bounds = src.boundaries();
        for(auto it = bounds.begin(); it != bounds.end(); ++it) {
            size_t isize = _indices.size();
            _indices.reserve(isize+2*it->size());
            for(Uint32 pos = 0; pos < it->size()-1; pos++) {
                _indices.push_back(it->at(pos  ));
                _indices.push_back(it->at(pos+1));
            }
            if (closed) {
                _indices.push_back(it->at(it->size()-1));
                _indices.push_back(it->at(0));
            }
        }
    }
}

/**
 * Stores a wire frame of an existing polygon in the provided buffer.
 *
 * This method is dedicated to an `INTERIOR` traversal.  See the description
 * of {@link #makeTraversal} for more information.  This method simply
 * exists to make the code more readable.
 *
 * The traversal will be appended to the buffer.  You should clear the buffer
 * first if you do not want to preserve the original data.
 *
 * @param paths The container to store the result
 * @param src   The source polygon to traverse
 *
 * @return a reference to the buffer for chaining.
 */
void WireNode::makeInteriorTraversal(const Poly2& src) {
    const std::vector<Uint32>* indxs = &(src.indices);
    _indices.reserve(2*indxs->size());
    for(int ii = 0; ii < indxs->size(); ii+=3) {
        _indices.push_back(indxs->at(ii  ));
        _indices.push_back(indxs->at(ii+1));
        _indices.push_back(indxs->at(ii+1));
        _indices.push_back(indxs->at(ii+2));
        _indices.push_back(indxs->at(ii+2));
        _indices.push_back(indxs->at(ii  ));
    }
}
