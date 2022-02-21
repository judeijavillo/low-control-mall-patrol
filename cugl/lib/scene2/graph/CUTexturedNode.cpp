//
//  CUTexturedNode.cpp
//  Cornell University Game Library (CUGL)
//
//  This module provides an abstract class for textured scene graph nodes. It
//  is the core scene graph node in CUGL.
//
//  You should never instantiate an object of this class.  Instead, you should
//  use one of the concrete subclasses: WireNode, PathNode, PolygonNode, or
//  SpriteNode. Because it is abstract, it has no allocators. It only has
//  initializers.
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
#include <cugl/scene2/graph/CUTexturedNode.h>
#include <cugl/util/CUStrings.h>
#include <cugl/assets/CUScene2Loader.h>
#include <cugl/assets/CUAssetManager.h>
#include <cugl/render/CUGradient.h>
#include <sstream>

using namespace cugl::scene2;

/** For handling JSON issues */
#define UNKNOWN_STR "<unknown>"

static GLenum blendEq(std::string value) {
    if (value == "GL_FUNC_SUBTRACT") {
        return GL_FUNC_SUBTRACT;
    } else if (value == "GL_FUNC_REVERSE_SUBTRACT") {
        return GL_FUNC_REVERSE_SUBTRACT;
    } else if (value == "GL_MAX") {
        return GL_MAX;
    } else if (value == "GL_MIN") {
        return GL_MIN;
    }
    return GL_FUNC_ADD;
}

static GLenum blendFunc(std::string value) {
    if (value == "GL_SRC_COLOR") {
        return GL_SRC_COLOR;
    } else if (value == "GL_ONE_MINUS_SRC_COLOR") {
        return GL_ONE_MINUS_SRC_COLOR;
    } else if (value == "GL_ONE_MINUS_SRC_COLOR") {
        return GL_ONE_MINUS_SRC_COLOR;
    } else if (value == "GL_DST_COLOR") {
        return GL_DST_COLOR;
    } else if (value == "GL_ONE_MINUS_DST_COLOR") {
        return GL_ONE_MINUS_DST_COLOR;
    } else if (value == "GL_SRC_ALPHA") {
        return GL_SRC_ALPHA;
    } else if (value == "GL_ONE_MINUS_SRC_ALPHA") {
        return GL_ONE_MINUS_SRC_ALPHA;
    } else if (value == "GL_DST_ALPHA") {
        return GL_DST_ALPHA;
    } else if (value == "GL_ONE_MINUS_DST_ALPHA") {
        return GL_ONE_MINUS_DST_ALPHA;
    } else if (value == "GL_ONE") {
        return GL_ONE;
    } else if (value == "GL_CONSTANT_COLOR") {
        return GL_CONSTANT_COLOR;
    } else if (value == "GL_ONE_MINUS_CONSTANT_COLOR") {
        return GL_ONE_MINUS_CONSTANT_COLOR;
    } else if (value == "GL_CONSTANT_ALPHA") {
        return GL_CONSTANT_ALPHA;
    } else if (value == "GL_ONE_MINUS_CONSTANT_ALPHA") {
        return GL_ONE_MINUS_CONSTANT_ALPHA;
    }
    return GL_ZERO;
}

#pragma mark Constructors

/**
 * Creates an empty scene graph node with the degenerate texture.
 *
 * This constructor should never be called directly, as this is an abstract
 * class.
 */
TexturedNode::TexturedNode() :
SceneNode(),
_texture(nullptr),
_gradient(nullptr),
_absolute(false),
_rendered(false),
_blendEquation(GL_FUNC_ADD),
_srcFactor(GL_SRC_ALPHA),
_dstFactor(GL_ONE_MINUS_SRC_ALPHA),
_flipHorizontal(false),
_flipVertical(false) {
    _classname = "TexturedNode";
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
void TexturedNode::dispose() {
    _classname = "TexturedNode";
    _texture = nullptr;
    _absolute = false;
    _rendered = false;
    _blendEquation = GL_FUNC_ADD;
    _srcFactor = GL_SRC_ALPHA;
    _dstFactor = GL_ONE_MINUS_SRC_ALPHA;
    _flipHorizontal = false;
    _flipVertical = false;
    _mesh.clear();
    SceneNode::dispose();
}

/**
 * Intializes a scene graph node with the image filename.
 *
 * This method will fail if the file does not exist or is not a valid
 * image file.  Even if the texture is successfully loaded, this is an
 * an abstract class, so it will not apply any geometry to the texture.
 * You will need to use one of the appropriate subclasses.
 *
 * @param filename  A path to image file, e.g., "scene1/earthtile.png"
 *
 * @return  true if the sprite is initialized properly, false otherwise.
 */
bool TexturedNode::initWithFile(const std::string& filename) {
    if (_texture != nullptr) {
        CUAssertLog(false, "%s is already initialized",_classname.c_str());
        return false;
    }

    CUAssertLog(filename.size() > 0, "Invalid filename for texture");
    std::shared_ptr<Texture> texture =  Texture::allocWithFile(filename);

    if (SceneNode::init()) {
        // default transform anchor: center
        setAnchor(Vec2(0.5f, 0.5f));
        
        // Update texture (Sets texture coordinates)
        setTexture(texture);
        return true;
    }
    return false;
}

/**
 * Initializes a textured polygon from a Texture object.
 *
 * If the texture is null, this node will use {@link Texture#getBlank}
 * instead, which is suitable for drawing solid shapes. Regardless, this
 * is an abstract class, so it will not apply any geometry to the texture.
 * You will need to use one of the appropriate subclasses.
 *
 * @param texture   A shared pointer to a Texture object.
 *
 * @return  true if the sprite is initialized properly, false otherwise.
 */
bool TexturedNode::initWithTexture(const std::shared_ptr<Texture>& texture) {
    if (_texture != nullptr) {
        CUAssertLog(false, "%s is already initialized",_classname.c_str());
        return false;
    }

    if (SceneNode::init()) {
        // default transform anchor: center
        setAnchor(Vec2(0.5f, 0.5f));
        
        // Update texture (Sets texture coordinates)
        setTexture(texture);
        return true;
    }
    return false;
}

/**
 * Initializes a node with the given JSON specificaton.
 *
 * This initializer is designed to receive the "data" object from the
 * JSON passed to {@link Scene2Loader}.  This JSON format supports all
 * of the attribute values of its parent class.  In addition, it supports
 * the following additional attribute:
 *
 *      "texture":  A string with the name of a previously loaded texture asset
 *      "gradient": A JSON object defining a gradient. See {@link Gradient}.
 *      "absolute": A boolean indicating whether to use absolute coordinates
 *      "blendeq":  A string matching a valid OpenGL blending equation.
 *                  See glBlendEquation in the OpenGL documentation.
 *      "blendsrc": A string mathcing a valid OpenGL blending function. See
 *                  glBlendFunc in the OpenGL documentation.
 *      "blenddst": A string mathcing a valid OpenGL blending function. See
 *                  glBlendFunc in the OpenGL documentation.
 *      "flip":     One of "horizontal", "vertical", "both", or "none".
 *
 * All attributes are optional. If the texture is missing, this node will use
 * {@link Texture#getBlank} instead, which is suitable for drawing solid shapes.
 *
 * Note that this is an abstract class, so it will not apply any geometry to the
 * texture. You will need to use one of the appropriate subclasses.
 *
 * @param loader    The scene loader passing this JSON file
 * @param data      The JSON object specifying the node
 *
 * @return true if initialization was successful.
 */
bool TexturedNode::initWithData(const Scene2Loader* loader, const std::shared_ptr<JsonValue>& data) {
    if (_texture != nullptr) {
        CUAssertLog(false, "%s is already initialized",_classname.c_str());
        return false;
    } else if (!data) {
        return init();
    } else if (!SceneNode::initWithData(loader, data)) {
        return false;
    }
    
    // Set the texture (it might be null)
    const AssetManager* assets = loader->getManager();
    setTexture(assets->get<Texture>(data->getString("texture",UNKNOWN_STR)));
    
    // Set the other properties
    if (data->has("gradient")) {
        _gradient = Gradient::allocWithData(data->get("gradient"));
    }
    _absolute = data->getBool("absolute",false);
    _blendEquation = blendEq(data->getString("blendeq","GL_FUNC_ADD"));
    _srcFactor = blendFunc(data->getString("blendsrc","GL_SRC_ALPHA"));
    _dstFactor = blendFunc(data->getString("blenddst","GL_ONE_MINUS_SRC_ALPHA"));
    
    std::string flip = data->getString("flip","none");
    _flipHorizontal = flip == "horizontal" || flip == "both";
    _flipVertical   = flip == "vertical" || flip == "both";

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
std::shared_ptr<SceneNode> TexturedNode::copy(const std::shared_ptr<SceneNode>& dst) const {
    SceneNode::copy(dst);
    std::shared_ptr<TexturedNode> node = std::dynamic_pointer_cast<TexturedNode>(dst);
    if (node) {
        node->_texture  = _texture;
        node->_gradient = _gradient;
        node->_absolute = _absolute;
        node->_rendered = _rendered;
        node->_offset = _offset;
        node->_mesh   = _mesh;

        node->_blendEquation = _blendEquation;
        node->_srcFactor = _srcFactor;
        node->_dstFactor = _dstFactor;

        node->_flipHorizontal = _flipHorizontal;
        node->_flipVertical   = _flipVertical;
    }
    return dst;
}

#pragma mark -
#pragma mark Attributes
/**
 * Sets the node texture to the one specified.
 *
 * This method will have no effect on the polygon vertices.  Unlike Sprite,
 * TexturedNode decouples the geometry from the texture.  That is because
 * we expect the vertices to not match the texture perfectly.
 *
 * @param   texture  A pointer to an existing Texture2D object.
 *                   You can use a Texture2D object for many sprites.
 *
 * @retain  a reference to this texture
 * @release the previous scene texture used by this object
 */
void TexturedNode::setTexture(const std::shared_ptr<Texture>& texture) {
    std::shared_ptr<Texture> temp = (texture == nullptr ? Texture::getBlank() : texture);
    if (_texture != temp) {
        _texture = temp;
        updateTextureCoords();
    }
}

/**
 * Sets the gradient to use for this polygon.
 *
 * Unlike colors, gradients are local. They do not apply to the children
 * of this node.
 *
 * @param gradient  The gradient to use for this polygon.
 */
void TexturedNode::setGradient(const std::shared_ptr<Gradient>& gradient) {
    _gradient = gradient;
    clearRenderData();
}

/**
 * Translates the texture image by the given amount.
 *
 * This method has no effect on the shape or position of the node. It
 * simply shifts the texture coordinates of the underlying mesh by the
 * given amount (measured in pixels). Hence this method can be used
 * for animation and filmstrips.  This method is faster than redefining
 * the shape.
 *
 * If the node has a gradient, this will shift the gradient image by
 * the same amount.
 *
 * @param dx    The amount to shift horizontally.
 * @param dy    The amount to shift horizontally.
 */
void TexturedNode::shiftTexture(float dx, float dy) {
    _offset.x += dx;
    _offset.y += dy;
    updateTextureCoords();
}

/**
 * Returns a string representation of this node for debugging purposes.
 *
 * If verbose is true, the string will include class information.  This
 * allows us to unambiguously identify the class.
 *
 * @param verbose Whether to include class information
 *
 * @return a string representation of this node for debuggging purposes.
 */
std::string TexturedNode::toString(bool verbose) const {
    std::stringstream ss;
    if (verbose) {
        ss << "cugl::";
        ss << _classname;
    }
    int texid = (_texture == nullptr ? -1 : _texture->getBuffer());
    ss << "(tag:";
    ss <<  strtool::to_string(_tag);
    ss << ", name:" << _name;
    ss << ", texture:";
    ss <<  strtool::to_string(texid);
    ss << ")";
    return ss.str();
}

/**
 * Sets the untransformed size of the node.
 *
 * The content size remains the same no matter how the node is scaled or
 * rotated. All nodes must have a size, though it may be degenerate (0,0).
 *
 * By default, the content size of a TexturedNode is the size of the
 * bounding box of the defining polygon.  Resizing a texture node will
 * stretch the image to fill in the new size.
 *
 * @param size  The untransformed size of the node.
 */
void TexturedNode::setContentSize(const Size size) {
    SceneNode::setContentSize(size);
    clearRenderData();
}

#pragma mark -
#pragma mark Internal Helpers

/**
 * Clears the render data, releasing all vertices and indices.
 */
void TexturedNode::clearRenderData() {
    _mesh.clear();
    _rendered = false;
}


