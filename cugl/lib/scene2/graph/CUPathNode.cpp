//
//  CUPathNode.cpp
//  Cornell University Game Library (CUGL)
//
//  This module provides a scene graph node that supports extruded paths. When
//  extruding paths, this node is better than PolygonNode, because it will align
//  the extruded path to the original wireframe.
//
//  This class uses SimpleExtruder to perform all extrusion. The user provides
//  a path and not a solid polygon.
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
#include <cugl/assets/CUAssetManager.h>
#include <cugl/assets/CUScene2Loader.h>
#include <cugl/scene2/graph/CUPathNode.h>
#include <cugl/util/CUDebug.h>
#include <cugl/render/CUGradient.h>

using namespace cugl::scene2;

/** For handling JSON issues */
#define UNKNOWN_STR "<unknown>"

#pragma mark Constructors
/**
 * Creates an empty path node.
 *
 * You must initialize this PathNode before use.
 *
 * NEVER USE A CONSTRUCTOR WITH NEW. If you want to allocate an object on
 * the heap, use one of the static constructors instead.
 */
PathNode::PathNode() : TexturedNode(),
_stroke(1.0f),
_fringe(0.0f),
_stencil(false),
_joint(poly2::Joint::SQUARE),
_endcap(poly2::EndCap::BUTT) {
    _classname = "PathNode";
}

/**
 * Intializes a path with the given vertices and stroke width.
 *
 * This method will extrude the vertices with the specified joint and cap.
 * This method uses {@link SimpleExtruder}, as it is safe for framerate
 * calculation.
 *
 * You do not need to set the texture; rendering this into a SpriteBatch
 * will simply use the blank texture. Hence the path will have a solid
 * color.
 *
 * @param vertices  The vertices to texture (expressed in image space)
 * @param stroke    The stroke width of the extruded path.
 * @param joint     The joint between extrusion line segments.
 * @param cap       The end caps of the extruded paths.
 * @param closed    The whether the vertex path is open or closed.
 *
 * @return  true if the path node is initialized properly, false otherwise.
 */
bool PathNode::initWithPath(const std::vector<Vec2>& vertices, float stroke,
                            poly2::Joint joint, poly2::EndCap cap, bool closed) {
    if (TexturedNode::initWithTexture(nullptr)) {
        _path.set(vertices);
        _joint  = joint;
        _endcap = cap;
        _stroke = stroke;
        setContentSize(_path.getBounds().size);
        updateExtrusion();
        return true;
    }
    return false;
}

/**
 * Intializes a path node with the given polygon and stroke width.
 *
 * This method will extrude the path with the specified joint and cap.
 * This method uses {@link SimpleExtruder}, as it is safe for framerate
 * calculation.
 *
 * You do not need to set the texture; rendering this into a SpriteBatch
 * will simply use the blank texture. Hence the path will have a solid
 * color.
 *
 * @param path      The path to texture (expressed in image space)
 * @param stroke    The stroke width of the extruded path.
 * @param joint     The joint between extrusion line segments.
 * @param cap       The end caps of the extruded paths.
 *
 * @return  true if the path node is initialized properly, false otherwise.
 */
bool PathNode::initWithPath(const Path2& path, float stroke,
                            poly2::Joint joint, poly2::EndCap cap) {
    if (TexturedNode::initWithTexture(nullptr)) {
        _path = path;
        _joint  = joint;
        _endcap = cap;
        _stroke = stroke;
        setContentSize(_path.getBounds().size);
        updateExtrusion();
        return true;
    }
    return false;
}

/**
 * Initializes a path node from the image filename and the path.
 *
 * @param filename  A path to image file, e.g., "scene1/earthtile.png"
 * @param path      The path to texture
 *
 * @return  true if the node is initialized properly, false otherwise.
 */
bool PathNode::initWithFilePath(const std::string &filename, const Path2& path) {
    CUAssertLog(filename.size() > 0, "Invalid filename for texture");
    
    std::shared_ptr<Texture> texture =  Texture::allocWithFile(filename);
    if (texture != nullptr) {
        return initWithTexturePath(texture, path);
    }
    
    return false;
}

/**
 * Initializes a path node from the image filename and the given rect.
 *
 * The rectangle will be extruded using a mitre joint.
 *
 * @param filename  A path to image file, e.g., "scene1/earthtile.png"
 * @param rect      The rectangle to texture
 *
 * @return  true if the node is initialized properly, false otherwise.
 */
bool PathNode::initWithFilePath(const std::string &filename, const Rect rect) {
    CUAssertLog(filename.size() > 0, "Invalid filename for texture");
    
    std::shared_ptr<Texture> texture =  Texture::allocWithFile(filename);
    if (texture != nullptr) {
        return initWithTexturePath(texture, rect);
    }
    
    return false;
}


/**
 * Initializes a path node from a texture and the given path.
 *
 * @param texture   A shared pointer to a Texture object.
 * @param path      The path to texture
 *
 * @return  true if the sprite is initialized properly, false otherwise.
 */
bool PathNode::initWithTexturePath(const std::shared_ptr<Texture>& texture, const Path2& path) {
    if (_texture != nullptr) {
        CUAssertLog(false, "%s is already initialized",_classname.c_str());
        return false;
    }
    
    if (TexturedNode::initWithTexture(texture)) {
        // default transform anchor: center
        setAnchor(Vec2(0.5f, 0.5f));
        
        // Update texture (Sets texture coordinates)
        _path = path;
        setContentSize(_path.getBounds().size);
        updateExtrusion();
        return true;
    }
    
    return false;
}

/**
 * Initializes a textured polygon from a Texture2D object and the given rect.
 *
 * The rectangle will be converted into a Poly2.  There is little benefit to
 * using a TexturedNode in this way over a normal Sprite. The option is here
 * only for completion.
 *
 * @param   texture  A pointer to an existing Texture2D object.
 *                   You can use a Texture2D object for many sprites.
 * @param   rect     The rectangle to texture
 *
 * @retain  a reference to this texture
 * @return  true if the sprite is initialized properly, false otherwise.
 */
bool PathNode::initWithTexturePath(const std::shared_ptr<Texture>& texture, const Rect rect) {
    if (_texture != nullptr) {
        CUAssertLog(false, "%s is already initialized",_classname.c_str());
        return false;
    }

    if (TexturedNode::initWithTexture(texture)) {
        // default transform anchor: center
        setAnchor(Vec2(0.5f, 0.5f));
        
        // Update texture (Sets texture coordinates)
        _path.set(rect);
        setContentSize(_path.getBounds().size);
        updateExtrusion();
        return true;
    }
    
    return false;
}

/**
 * Initializes a path node with the given JSON specificaton.
 *
 * This initializer is designed to receive the "data" object from the
 * JSON passed to {@link Scene2Loader}.  This JSON format supports all
 * of the attribute values of its parent class.  In addition, it supports
 * the following additional attributes:
 *
 *      "path":     A JSON object defining a path. See {@link Path2}.
 *      "stroke":   A number specifying the stroke width.
 *      "joint":    One of "mitre', "bevel", "square", or "round".
 *      "endcap":   One of "square", "round", or "butt".
 *      "fringe":   A number indicating the size of the border fringe.
 *      "stencil"   A boolean indicating whether to stencil the path.
 *
 * All attributes are optional. If the path is not specified, the node will
 * use a rectangle with the dimensions of the texture. For more information,
 * see {@link TexturedNode#initWithData}.
 *
 * @param loader    The scene loader passing this JSON file
 * @param data      The JSON object specifying the node
 *
 * @return true if initialization was successful.
 */
bool PathNode::initWithData(const Scene2Loader* loader,
                            const std::shared_ptr<JsonValue>& data) {

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
    
    // Get the geometry
    if (data->has("path")) {
        _path.set(data->get("path"));
    } else {
        Rect bounds = Rect::ZERO;
        if (_texture != nullptr) {
            bounds.size = _texture->getSize();
        } else {
            bounds.size = getContentSize();
        }
        _path.set(bounds);
    }
    setContentSize(_path.getBounds().size);

    _stroke = data->getFloat("stroke", 1.0f);
    std::string joint = data->getString("joint",UNKNOWN_STR);
    if (joint == "mitre") {
        _joint = poly2::Joint::MITRE;
    } else if (joint == "round") {
        _joint = poly2::Joint::ROUND;
    } else {
        _joint = poly2::Joint::SQUARE;
    }

    std::string endcap = data->getString("endcap",UNKNOWN_STR);
    if (endcap == "square") {
        _endcap = poly2::EndCap::SQUARE;
    } else if (endcap == "round") {
        _endcap = poly2::EndCap::ROUND;
    } else {
        _endcap = poly2::EndCap::BUTT;
    }
    
    _fringe  = data->getFloat("fringe",0.0f);
    _stencil = data->getBool("stencil",false);

    // Redo the size if necessary
    if (sizefit) {
        setContentSize(size);
    }

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
std::shared_ptr<SceneNode> PathNode::copy(const std::shared_ptr<SceneNode>& dst) const {
    TexturedNode::copy(dst);
    std::shared_ptr<PathNode> node = std::dynamic_pointer_cast<PathNode>(dst);
    if (node) {
        node->_path = _path;
        node->_polygon = _polygon;
        node->_stroke  = _stroke;
        node->_joint   = _joint;
        node->_endcap  = _endcap;
        node->_fringe  = _fringe;
        node->_stencil = _stencil;
        node->_border  = _border;
        node->_extruder = _extruder;
        node->_extrabounds = _extrabounds;
    }
    return dst;
}

#pragma mark -
#pragma mark Attributes
/**
 * Sets the stroke width of the path.
 *
 * This method affects the extruded polygon, but not the original path
 * polygon.
 *
 * @param stroke    The stroke width of the path
 */
void PathNode::setStroke(float stroke) {
    CUAssertLog(stroke >= 0, "Stroke width is invalid");
    bool changed = (stroke != _stroke);
    _stroke = stroke;

    if (changed) {
        clearRenderData();
    }
}

/**
 * Sets whether the path is closed.
 *
 * If set to true, this will smooth the polygon to remove all gaps,
 * regardless of the original inidices in the polygon. Furthermore,
 * previous information about existing gaps is lost, so that setting
 * the value back to false will only open the curve at the end.
 *
 * @param closed    Whether the path is closed.
 */
void PathNode::setClosed(bool closed) {
    bool changed = (closed != _path.closed);
    _path.closed = closed;
    
    if (changed) {
        clearRenderData();
    }
}

/**
 * Sets the joint type between path segments.
 *
 * This method affects the extruded polygon, but not the original path
 * polygon.
 *
 * @param joint The joint type between path segments
 */
void PathNode::setJoint(poly2::Joint joint) {
    bool changed = (joint != _joint);
    _joint = joint;
    
    if (changed && _stroke > 0) {
        clearRenderData();
    }
}

/**
 * Sets the cap shape at the ends of the path.
 *
 * This method affects the extruded polygon, but not the original path
 * polygon.
 *
 * @param cap   The cap shape at the ends of the path.
 */
void PathNode::setCap(poly2::EndCap cap) {
    bool changed = (cap != _endcap);
    _endcap = cap;
    
    if (changed && _stroke > 0) {
        clearRenderData();
    }
}


#pragma mark -
#pragma mark Polygons
/**
 * Sets the path to the vertices expressed in texture space.
 *
 * @param vertices  The vertices to texture
 */
void PathNode::setPath(const std::vector<Vec2>& vertices, bool closed) {
    CUAssertLog(vertices.size() > 1, "Path must have at least two vertices");
    _path.set(vertices);
    _path.closed = closed;
    setContentSize(_path.getBounds().size);
    clearRenderData();
}

/**
 * Sets the polygon to the given one in texture space.
 *
 * This method will extrude that polygon with the current joint and cap.
 * The polygon is assumed to be closed if the number of indices is twice
 * the number of vertices. PathNode objects share a single extruder, so
 * this method is not thread safe.
 *
 * @param poly  The polygon to texture
 */
void PathNode::setPath(const Path2& path) {
    CUAssertLog(path.vertices.size() > 1, "Path must have at least two vertices");
    _path = path;
    setContentSize(_path.getBounds().size);
    clearRenderData();
}

/**
 * Sets the texture polygon to one equivalent to the given rect.
 *
 * The rectangle will be converted into a Poly2, using the standard outline.
 * This is the same as passing Poly2(rect,false). It will then be extruded
 * with the current joint and cap. PathNode objects share a single extruder,
 * so this method is not thread safe.
 *
 * @param rect  The rectangle to texture
 */
void PathNode::setPath(const Rect rect) {
    _path.set(rect);
    setContentSize(_path.getBounds().size);
    clearRenderData();
}

#pragma mark -
#pragma mark Rendering
/**
 * Draws this path node via the given SpriteBatch.
 *
 * This method only worries about drawing the current node.  It does not
 * attempt to render the children.
 *
 * @param batch     The SpriteBatch to draw with.
 * @param matrix    The global transformation matrix.
 * @param tint      The tint to blend with the Node color.
 */
void PathNode::draw(const std::shared_ptr<SpriteBatch>& batch, const Affine2& transform, Color4 tint) {
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

    if (_stencil) {
        batch->setStencilEffect(StencilEffect::CLAMP_NONE);
    }
    batch->drawMesh(_mesh, transform);
    if (_fringe > 0) {
        if (_stencil) {
            batch->setStencilEffect(StencilEffect::MASK_NONE);
        }
        batch->drawMesh(_border, transform);
    }
    
    if (_stencil) {
        batch->clearStencil();
    }
    batch->setGradient(nullptr);
}

/**
 * Allocate the render data necessary to render this node.
 */
void PathNode::generateRenderData() {
    CUAssertLog(!_rendered, "Render data is already present");
    if (_texture == nullptr) {
        return;
    }
    
    updateExtrusion();
    Size nsize = getContentSize();
    Size bsize = _path.getBounds().size;

    Affine2 shift;
    bool adjust = false;
    if (nsize != bsize) {
        adjust = true;
        shift.scale((bsize.width > 0 ? nsize.width/bsize.width : 0),
                    (bsize.height > 0 ? nsize.height/bsize.height : 0));
    }
    const Vec2 offset = _path.getBounds().origin;
    if (!_absolute && !offset.isZero()) {
        adjust = true;
        shift.translate(-offset.x,-offset.y);
    }

    if (adjust) {
        _mesh *= shift;
        _border *= shift;
    }
    
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
void PathNode::updateTextureCoords() {
    if (!_rendered) {
        return;
    }
    
    // First the interior stroke
    Size tsize = _texture->getSize();
    Vec2 off = _offset+_path.getBounds().origin;
    for(auto it = _mesh.vertices.begin(); it != _mesh.vertices.end(); ++it) {
        float s = (it->position.x+off.x)/tsize.width;
        float t = (it->position.y+off.y)/tsize.height;

        if (_flipHorizontal) { s = 1-s; }
        if (!_flipVertical)  { t = 1-t; }
        it->color = Color4::WHITE.getPacked();
        it->texcoord.x = s*_texture->getMaxS()+(1-s)*_texture->getMinS();
        it->texcoord.y = t*_texture->getMaxT()+(1-t)*_texture->getMinT();
    
        if (_gradient) {
            s = (it->position.x+off.x)/_path.getBounds().size.width;
            t = (it->position.x+off.x)/_path.getBounds().size.height;

            if (_flipHorizontal) { s = 1-s; }
            if (!_flipVertical)  { t = 1-t; }
            it->gradcoord.x = s;
            it->gradcoord.y = t;
        }
    }

    // Now the border
    for(auto it = _border.vertices.begin(); it != _border.vertices.end(); ++it) {
        float s = (it->position.x+off.x)/tsize.width;
        float t = (it->position.y+off.y)/tsize.height;

        if (_flipHorizontal) { s = 1-s; }
        if (!_flipVertical)  { t = 1-t; }

        it->texcoord.x = s*_texture->getMaxS()+(1-s)*_texture->getMinS();
        it->texcoord.y = t*_texture->getMaxT()+(1-t)*_texture->getMinT();
    
        if (_gradient) {
            s = (it->position.x+off.x)/_path.getBounds().size.width;
            t = (it->position.x+off.x)/_path.getBounds().size.height;

            if (_flipHorizontal) { s = 1-s; }
            if (!_flipVertical)  { t = 1-t; }
            it->gradcoord.x = s;
            it->gradcoord.y = t;
        }
    }
}

/**
 * Updates the extrusion polygon, based on the current settings.
 */
void PathNode::updateExtrusion() {
    _border.clear();
    _mesh.clear();
    _polygon.clear();

    Color4 clear = Color4(255,255,255,0);
    if (_stroke > _fringe) {
        _extruder.clear();
        _extruder.set(_path);
        _extruder.setJoint(_joint);
        _extruder.setEndCap(_endcap);
        _extruder.calculate(_stroke);

        _extruder.getPolygon(&_polygon);
        _extrabounds = _polygon.getBounds();
        _extrabounds.origin += _path.getBounds().origin;
        _mesh.set(_polygon);
        
        if (_fringe > 0) {
            std::vector<Path2> outlines;
            _extruder.getBorder(outlines);
            _border.command = GL_TRIANGLES;
            for(auto it = outlines.begin(); it != outlines.end(); ++it) {
                _extruder.clear();
                _extruder.set(*it);
                _extruder.setJoint(poly2::Joint::MITRE);
                _extruder.setEndCap(poly2::EndCap::BUTT);
                _extruder.calculate(0,_fringe);
                _extruder.getMesh(&_border,Color4::WHITE,clear);
            }
        }
    } else if (_fringe > 0) {
        Path2 outline;
        size_t size = _path.vertices.size();
        outline.vertices.reserve(2*size);
        outline.vertices = _path.vertices;
        for(size_t ii = 2; ii < size; ii++) {
            outline.vertices.push_back(_path.vertices[size-ii]);
        }
        outline.closed = true;
        _extruder.clear();
        _extruder.set(outline);
        _extruder.setJoint(poly2::Joint::MITRE);
        _extruder.setEndCap(poly2::EndCap::BUTT);
        _extruder.calculate(0,_fringe);
        _border.command = GL_TRIANGLES;
        _extruder.getPolygon(&_polygon);
        _extruder.getMesh(&_border,Color4::WHITE,clear);
        _extrabounds = _polygon.getBounds();
        _extrabounds.origin += _path.getBounds().origin;
    } else {
        // Just make a wireframe
        for (auto it = _path.vertices.begin(); it != _path.vertices.end(); ++it) {
            SpriteVertex2 vert;
            vert.position = *it;
            vert.color = Color4::WHITE.getPacked();
            _mesh.vertices.push_back(vert);
        }
        for(Uint32 ii = 0; ii < _path.vertices.size()-1; ii++) {
            _mesh.indices.push_back(ii  );
            _mesh.indices.push_back(ii+1);
        }
        if (_path.isClosed()) {
            _mesh.indices.push_back((Uint32)_path.vertices.size()-1);
            _mesh.indices.push_back(0);
        }
        _extrabounds = _path.getBounds();
    }
}
