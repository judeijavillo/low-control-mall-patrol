//
//  CUPolygonNode.cpp
//  Cornell University Game Library (CUGL)
//
//  This module provides a scene graph node that supports basic sprite graphics.
//  The sprites do not have to be rectangular. They may be any shape represented
//  by Poly2.
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
#include <algorithm>
#include <cugl/scene2/graph/CUPolygonNode.h>
#include <cugl/render/CUGradient.h>
#include <cugl/assets/CUScene2Loader.h>
#include <cugl/assets/CUAssetManager.h>
#include <cugl/math/polygon/CUSimpleExtruder.h>
#include <cugl/util/CUTimestamp.h>

using namespace cugl::scene2;

#pragma mark Initialization
/**
 * Intializes a polygon node from the image filename.
 *
 * After creation, the polygon will be a rectangle. The vertices of this
 * polygon will be the corners of the image. The rectangle will be
 * triangulated with the standard two triangles.
 *
 * @param filename  A path to image file, e.g., "scene1/earthtile.png"
 *
 * @return  true if the sprite is initialized properly, false otherwise.
 */
bool PolygonNode::initWithFile(const std::string& filename) {
    if (TexturedNode::initWithFile(filename)) {
        Size size = _texture == nullptr ? Size::ZERO : _texture->getSize();
        setPolygon(Rect(Vec2::ZERO,size));
        return true;
    }
    return false;
}

/**
 * Initializes a polygon node from a Texture object.
 *
 * After creation, the polygon will be a rectangle. The vertices of this
 * polygon will be the corners of the image. The rectangle will be
 * triangulated with the standard two triangles.
 *
 * @param texture   A shared pointer to a Texture object.
 *
 * @return  true if the sprite is initialized properly, false otherwise.
 */
bool PolygonNode::initWithTexture(const std::shared_ptr<Texture>& texture) {
    if (TexturedNode::initWithTexture(texture)) {
        Size size = _texture == nullptr ? Size::ZERO : _texture->getSize();
        setPolygon(Rect(Vec2::ZERO,size));
        return true;
    }
    return false;
}

/**
 * Initializes a polygon node with the given JSON specificaton.
 *
 * This initializer is designed to receive the "data" object from the
 * JSON passed to {@link Scene2Loader}. This JSON format supports all
 * of the attribute values of its parent class. In addition, it supports
 * the following additional attributes:
 *
 *      "polygon":  A JSON object defining a polygon. See {@link Poly2}.
 *      "fringe":   A number indicating the size of the border fringe.
 *
 * All attributes are optional. If the polygon is not specified, the node will
 * use a rectangle with the dimensions of the texture. For more information,
 * see {@link TexturedNode#initWithData}.
 *
 * @param loader    The scene loader passing this JSON file
 * @param data      The JSON object specifying the node
 *
 * @return true if initialization was successful.
 */
bool PolygonNode::initWithData(const Scene2Loader* loader, const std::shared_ptr<JsonValue>& data) {
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
    _fringe = data->getFloat("fringe",0.0f);

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
std::shared_ptr<SceneNode> PolygonNode::copy(const std::shared_ptr<SceneNode>& dst) const {
    TexturedNode::copy(dst);
    std::shared_ptr<PolygonNode> node = std::dynamic_pointer_cast<PolygonNode>(dst);
    if (node) {
        node->_polygon  = _polygon;
        node->_fringe = _fringe;
    }
    return dst;
}

#pragma mark -
#pragma mark Polygon Attributes
/**
 * Sets the polgon to the vertices expressed in texture space.
 *
 * The vertices will be triangulated with {@link EarclipTriangulator}.
 *
 * @param vertices  The vertices to texture
 */
void PolygonNode::setPolygon(const std::vector<Vec2>& vertices) {
    EarclipTriangulator triangulator;
    _polygon.set(vertices);
    _polygon.indices.clear();
    triangulator.set(vertices);
    triangulator.calculate();
    triangulator.getTriangulation(_polygon.indices);
    
    setContentSize(_polygon.getBounds().size);
    updateTextureCoords();
}

/**
 * Sets the polygon to the given one in texture space.
 *
 * @param poly  The polygon to texture
 */
void PolygonNode::setPolygon(const Poly2& poly) {
    if (&_polygon != &poly) {
        _polygon.set(poly);
    }
    
    setContentSize(_polygon.getBounds().size);
    updateTextureCoords();
}

/**
 * Sets the texture polygon to one equivalent to the given rect.
 *
 * The rectangle will be triangulated with the standard two triangles.
 *
 * @param rect  The rectangle to texture
 */
void PolygonNode::setPolygon(const Rect rect) {
    _polygon.set(rect);
    setContentSize(_polygon.getBounds().size);
    updateTextureCoords();
}


#pragma mark -
#pragma mark Rendering Methods
/**
 * Draws this polygon node via the given SpriteBatch.
 *
 * This method only worries about drawing the current node.  It does not
 * attempt to render the children.
 *
 * @param batch     The SpriteBatch to draw with.
 * @param matrix    The global transformation matrix.
 * @param tint      The tint to blend with the Node color.
 */
void PolygonNode::draw(const std::shared_ptr<SpriteBatch>& batch, const Affine2& transform, Color4 tint) {
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
    if (_gradient) {
        batch->setGradient(nullptr);
    }
}

/**
 * Allocate the render data necessary to render this node.
 */
void PolygonNode::generateRenderData() {
    CUAssertLog(!_rendered, "Render data is already present");
    if (_texture == nullptr) {
        return;
    }
    
    _mesh.set(_polygon);
    _mesh.command = GL_TRIANGLES;
    
    // Antialias the boundaries (if required)
    if (_fringe > 0) {
        SimpleExtruder extruder;
        std::vector<std::vector<Uint32>> boundaries = _polygon.boundaries();
        Color4 clear = Color4(255,255,255,0);
        for(auto it = boundaries.begin(); it != boundaries.end(); ++it) {
            std::vector<Vec2> border;
            border.reserve(it->size());
            for (auto jt = it->begin(); jt != it->end(); ++jt) {
                border.push_back(_polygon.vertices[*jt]);
            }
            extruder.clear();
            extruder.set(border,true);
            extruder.setJoint(poly2::Joint::SQUARE);
            // Interior is to the left
            extruder.calculate(0,_fringe);
            extruder.getMesh(&_mesh,Color4::WHITE,clear);
        }
    }

    // Adjust the mesh as necesary
    Size nsize = getContentSize();
    Size bsize = _polygon.getBounds().size;

    Mat4 shift;
    bool adjust = false;
    if (nsize != bsize) {
        adjust = true;
        shift.scale((bsize.width > 0  ? nsize.width/bsize.width : 0),
                    (bsize.height > 0 ? nsize.height/bsize.height : 0), 1);
    }

    const Vec2 offset = _polygon.getBounds().origin;
    if (!_absolute && !offset.isZero()) {
        adjust = true;
        shift.translate(-offset.x,-offset.y,0);
    }
    
    if (adjust) {
        _mesh *= shift;
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
void PolygonNode::updateTextureCoords() {
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
