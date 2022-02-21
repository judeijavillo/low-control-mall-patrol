//
//  CUTexturedNode.h
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
#ifndef __CU_TEXTURED_NODE_H__
#define __CU_TEXTURED_NODE_H__

#include <string>
#include <cugl/math/CUPoly2.h>
#include <cugl/scene2/graph/CUSceneNode.h>
#include <cugl/render/CUTexture.h>
#include <cugl/render/CUSpriteVertex.h>
#include <cugl/render/CUMesh.h>

namespace cugl {

    /**
     * The classes to construct an 2-d scene graph.
     *
     * This namespace was chosen to future-proof the game engine. We will
     * eventually want to add 3-d scene graphs as well, and this namespace
     * will prevent any collisions with those scene graph nodes.
     */
    namespace scene2 {
    
/**
 * This class is an abstract scene graph node representing a textured shape.
 *
 * This class cannot be instantiated directly. Instead, you must use one of the
 * subclasses: {@link PolygonNode}, {@link SpriteNode}, {@link PathNode},
 * or {@link WireNode}.
 *
 * This abstract class only manages texture. It has no shape. The shape/mesh
 * of a textured node is determined by the appropriate subclass.  However, for
 * all subclasses, the shape is always specified in image coordinates. Image
 * coordinates are different from texture coordinates. Their origin is at the
 * bottom-left corner of the file, and each pixel is one unit. This design
 * choice is intended to make irregular shapes easier to use.
 *
 * This means that a solid polygon with vertices (0,0), (width,0), (width,height),
 * and (0,height) would be identical to a sprite node. However, a solid polygon
 * with vertices (0,0), (2*width,0), (2*width,2*height), and (0,2*height) would
 * tile the sprite (given the wrap settings) twice both horizontally and vertically.
 *
 * The content size of this node is defined by the size (but not the offset)
 * of the bounding box. The anchor point is relative to this content size.
 * The default anchor point in a TexturedNode is (0.5, 0.5).  This means that
 * a uniform translation of the the underlying shape (in contrast to the node
 * itself) will not move the node on the the screen. Instead, it will just change
 * the part of the texture it uses.
 *
 * For example, suppose the texture has given width and height.  We have one
 * polygon with vertices (0,0), (width/2,0), (width/2,height/2), and (0,height/2).
 * We have another polygon with vertices (width/2,height/2), (width,height/2),
 * (width,height), and (width/2,height).  Both polygons would create a rectangle
 * of size (width/2,height/2). centered at the node position.  However, the
 * first would use the bottom left part of the texture, while the second would
 * use the top right.
 *
 * You can disable these features at any time by setting the attribute
 * absolute to true.  Do this will place the polygon vertices in their
 * absolute positions in Node space.  This will also disable anchor functions
 * (setting the anchor as the bottom left corner), since anchors do not make
 * sense when we are drawing vertices directly into the coordinate space.
 */
class TexturedNode : public SceneNode {
#pragma mark Values
protected:
    /** Texture associated with this node */
    std::shared_ptr<Texture> _texture;
    /** The gradient to use for this node */
    std::shared_ptr<Gradient> _gradient;

    /** Whether to disable anchors and draw the underlying shape in absolute coordinates */
    bool _absolute;
    /** Texture offset to for shifting the image */
    Vec2 _offset;

    /** Whether we have generated render data for this node */
    bool _rendered;
    /** The render data for this node */
    Mesh<SpriteVertex2> _mesh;
    
    /** The blending equation for this texture */
    GLenum _blendEquation;
    /** The source factor for the blend function */
    GLenum _srcFactor;
    /** The destination factor for the blend function */
    GLenum _dstFactor;
    
    /** Whether or not to flip the texture horizontally */
    bool _flipHorizontal;
    /** Whether or not to flip the texture vertically */
    bool _flipVertical;
    

#pragma mark -
#pragma mark Constructors
public:
    /**
     * Creates an empty scene graph node with the degenerate texture.
     *
     * This constructor should never be called directly, as this is an abstract 
     * class.
     */
    TexturedNode();
    
    /**
     * Deletes this node, releasing all resources.
     */
    ~TexturedNode() { dispose(); }
    
    /**
     * Disposes all of the resources used by this node.
     *
     * A disposed Node can be safely reinitialized. Any children owned by this
     * node will be released.  They will be deleted if no other object owns them.
     *
     * It is unsafe to call this on a Node that is still currently inside of
     * a scene graph.
     */
    virtual void dispose() override;
    
    /**
     * Intializes a scene graph node with the degenerate texture.
     *
     * This will make the texture be {@link Texture#getBlank}, which is suitable
     * for drawing solid shapes.  However, this is an abstract class, so it will
     * not apply any geometry to the texture. You will need to use one of the
     * appropriate subclasses.
     *
     * @return true if the sprite is initialized properly, false otherwise.
     */
    virtual bool init() override {
        return initWithTexture(nullptr);
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
    virtual bool initWithFile(const std::string& filename);
    
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
    virtual bool initWithTexture(const std::shared_ptr<Texture>& texture);
    
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
    virtual bool initWithData(const Scene2Loader* loader, const std::shared_ptr<JsonValue>& data) override;

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
    virtual std::shared_ptr<SceneNode> copy(const std::shared_ptr<SceneNode>& dst) const override;
    
#pragma mark -
#pragma mark Texture Attributes
    /**
     * Sets the node texture to a new one allocated from a filename.
     *
     * This method will have no effect on the underlying geometry. This class
     * decouples the geometry from the texture.  That is because we do not 
     * expect the vertices to match the texture perfectly.
     *
     * @param filename  A path to image file, e.g., "scene1/earthtile.png"
     */
    void setTexture(const std::string &filename) {
        std::shared_ptr<Texture> texture =  Texture::allocWithFile(filename);
        setTexture(texture);
    }
    
    /**
     * Sets the node texture to the one specified.
     *
     * This method will have no effect on the underlying geometry. This class
     * decouples the geometry from the texture.  That is because we do not
     * expect the vertices to match the texture perfectly.
     *
     * @param texture   A shared pointer to a Texture object.
     */
    void setTexture(const std::shared_ptr<Texture>& texture);
    
    /**
     * Returns the texture used by this node.
     *
     * @return the texture used by this node
     */
    std::shared_ptr<Texture>& getTexture() { return _texture; }

    /**
     * Returns the texture used by this node.
     *
     * @return the texture used by this node
     */
    const std::shared_ptr<Texture>& getTexture() const { return _texture; }

    /**
     * Returns the gradient to use for this node.
     *
     * Unlike colors, gradients are local. They do not apply to the children
     * of this node.  Gradients are independent of textures; a texture node
     * may have both a gradient and a texture.
     *
     * @return the gradient to use for this node.
     */
    const std::shared_ptr<Gradient>& getGradient() const { return _gradient; }

    /**
     * Sets the gradient to use for this node.
     *
     * Unlike colors, gradients are local. They do not apply to the children
     * of this node.  Gradients are independent of textures; a texture node
     * may have both a gradient and a texture.
     *
     * @param gradient  The gradient to use for this node.
     */
    void setGradient(const std::shared_ptr<Gradient>& gradient);

    /**
     * Translates the texture image by the given amount.
     *
     * This method has no effect on the shape or position of the node. It
     * simply shifts the texture coordinates of the underlying mesh by the
     * given amount (measured in pixels). Hence this method can be used
     * for animation and filmstrips. This method is faster than redefining
     * the shape.
     *
     * If the node has a gradient, this will shift the gradient image by
     * the same amount.
     *
     * @param dx    The amount to shift horizontally.
     * @param dy    The amount to shift horizontally.
     */
    void shiftTexture(float dx, float dy);
    
#pragma mark -
#pragma mark Drawing Attributes
    /**
     * Sets the blending function for this texture node.
     *
     * The enums are the standard ones supported by OpenGL.  See
     *
     *      https://www.opengl.org/sdk/docs/man/html/glBlendFunc.xhtml
     *
     * However, this setter does not do any error checking to verify that
     * the enums are valid.  By default, srcFactor is GL_SRC_ALPHA while
     * dstFactor is GL_ONE_MINUS_SRC_ALPHA. This corresponds to non-premultiplied
     * alpha blending.
     *
     * This blending factor only affects the texture of the current node.  It
     * does not affect any children of the node.
     *
     * @param srcFactor Specifies how the source blending factors are computed
     * @param dstFactor Specifies how the destination blending factors are computed.
     */
    void setBlendFunc(GLenum srcFactor, GLenum dstFactor) { _srcFactor = srcFactor; _dstFactor = dstFactor; }
    
    /**
     * Returns the source blending factor
     *
     * By default this value is GL_SRC_ALPHA. For other options, see
     *
     *      https://www.opengl.org/sdk/docs/man/html/glBlendFunc.xhtml
     *
     * This blending factor only affects the texture of the current node.  It
     * does not affect any children of the node.
     *
     * @return the source blending factor
     */
    GLenum getSourceBlendFactor() const { return _srcFactor; }
    
    /**
     * Returns the destination blending factor
     *
     * By default this value is GL_ONE_MINUS_SRC_ALPHA. For other options, see
     *
     *      https://www.opengl.org/sdk/docs/man/html/glBlendFunc.xhtml
     *
     * This blending factor only affects the texture of the current node.  It
     * does not affect any children of the node.
     *
     * @return the destination blending factor
     */
    GLenum getDestinationBlendFactor() const { return _srcFactor; }
    
    /**
     * Sets the blending equation for this textured node
     *
     * The enum must be a standard ones supported by OpenGL.  See
     *
     *      https://www.opengl.org/sdk/docs/man/html/glBlendEquation.xhtml
     *
     * However, this setter does not do any error checking to verify that
     * the input is valid.  By default, the equation is GL_FUNC_ADD.
     *
     * This blending equation only affects the texture of the current node.  It
     * does not affect any children of the node.
     *
     * @param equation  Specifies how source and destination colors are combined
     */
    void setBlendEquation(GLenum equation) { _blendEquation = equation; }
    
    /**
     * Returns the blending equation for this textured node
     *
     * By default this value is GL_FUNC_ADD. For other options, see
     *
     *      https://www.opengl.org/sdk/docs/man/html/glBlendEquation.xhtml
     *
     * This blending equation only affects the texture of the current node.  It
     * does not affect any children of the node.
     *
     * @return the blending equation for this sprite batch
     */
    GLenum getBlendEquation() const { return _blendEquation; }
    
    /**
     * Flips the texture coordinates horizontally if flag is true.
     *
     * Flipping the texture coordinates replaces each u coordinate with
     * 1-u.  Hence this operation is defined even if the texture coordinates
     * are outside the range 0..1.
     *
     * @param  flag whether to flip the coordinates horizontally
     */
    void flipHorizontal(bool flag) { _flipHorizontal = flag; updateTextureCoords(); }
    
    /**
     * Returns true if the texture coordinates are flipped horizontally.
     *
     * @return true if the texture coordinates are flipped horizontally.
     */
    bool isFlipHorizontal() const { return _flipHorizontal; }
    
    /**
     * Flips the texture coordinates vertically if flag is true.
     *
     * Flipping the texture coordinates replaces each v coordinate with
     * 1-v.  Hence this operation is defined even if the texture coordinates
     * are outside the range 0..1.
     *
     * @param  flag whether to flip the coordinates vertically
     */
    void flipVertical(bool flag) { _flipVertical = flag; updateTextureCoords(); }
    
    /**
     * Returns true if the texture coordinates are flipped vertically.
     *
     * @return true if the texture coordinates are flipped vertically.
     */
    bool isFlipVertical() const { return _flipVertical; }

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
    virtual std::string toString(bool verbose=false) const override;
    
#pragma mark -
#pragma mark Scaling Attributes
    /**
     * Returns true if this node is using absolute positioning.
     *
     * In absolute positioning, the vertices are draw in their correct
     * position with respect to the node origin.  We no longer try to
     * offset the polygon with respect to the anchors.  This is useful
     * when you need a scene graph node that has an external relationship
     * to a non-child node.
     *
     * Setting this value to true will disable anchor functions (and set
     * the anchor to the bottom left).  That is because anchors do not
     * make sense when we are using absolute positioning.
     *
     * @return true if this node is using absolute positioning.
     */
    bool isAbsolute() const { return _absolute; }
    
    /**
     * Sets whether this node is using absolute positioning.
     *
     * In absolute positioning, the vertices are draw in their correct
     * position with respect to the node origin.  We no longer try to
     * offset the polygon with respect to the anchors.  This is useful
     * when you need a scene graph node that has an external relationship
     * to a non-child node.
     *
     * Setting this value to true will disable anchor functions (and set
     * the anchor to the bottom left).  That is because anchors do not
     * make sense when we are using absolute positioning.
     *
     * @param flag  Whether if this node is using absolute positioning.
     */
    void setAbsolute(bool flag) {
        _absolute = flag;
        _anchor = Vec2::ANCHOR_BOTTOM_LEFT;
    }
    
    /**
     * Sets the anchor point in percentages.
     *
     * The anchor point defines the relative origin of Node with respect to its
     * parent.  It is a "pin" where the Node is attached to its parent.  In
     * effect, the translation of a Node is defined by its position plus
     * anchor point.
     *
     * The anchorPoint is normalized, like a percentage. (0,0) means the
     * bottom-left corner and (1,1) means the top-right corner. There are many
     * anchor point constants defined in {@link Vec2}.  However, there is
     * nothing preventing an anchor point higher than (1,1) or lower than (0,0).
     *
     * The default anchorPoint is (0.5,0.5), so it starts in the center of
     * the node. Changing the anchor will not move the contents of the node in
     * the parent space, but it will change the value of the Node position.
     *
     * This function is disabled if the node is using absolute positioning.
     * That is because anchors do not make sense when we are drawing polygons
     * directly to the screen.
     *
     * @param anchor    The anchor point of node.
     */
    virtual void setAnchor(const Vec2 anchor) override {
        if (!_absolute) { SceneNode::setAnchor(anchor); }
    }
    
    /**
     * Sets the anchor point in percentages.
     *
     * The anchor point defines the relative origin of Node with respect to its
     * parent.  It is a "pin" where the Node is attached to its parent.  In
     * effect, the translation of a Node is defined by its position plus
     * anchor point.
     *
     * The anchorPoint is normalized, like a percentage. (0,0) means the
     * bottom-left corner and (1,1) means the top-right corner. There are many
     * anchor point constants defined in {@link Vec2}.  However, there is
     * nothing preventing an anchor point higher than (1,1) or lower than (0,0).
     *
     * The default anchorPoint is (0.5,0.5), so it starts in the center of
     * the node. Changing the anchor will not move the contents of the node in
     * the parent space, but it will change the value of the Node position.
     *
     * This function is disabled if the node is using absolute positioning.
     * That is because anchors do not make sense when we are drawing polygons
     * directly to the screen.
     *
     * @param x     The horizontal anchor percentage.
     * @param y     The vertical anchor percentage.
     */
    virtual void setAnchor(float x, float y) override { setAnchor(Vec2(x,y)); }

    /**
     * Sets the untransformed size of the node.
     *
     * The content size remains the same no matter how the node is scaled or
     * rotated. All nodes must have a size, though it may be degenerate (0,0).
     *
     * By default, the content size of a TexturedNode is the size of the
     * bounding box of the defining polygon. Resizing a texture node will
     * stretch the image to fill in the new size.
     *
     * @param size  The untransformed size of the node.
     */
    virtual void setContentSize(const Size size) override;
    
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
     * @param width     The untransformed width of the node.
     * @param height    The untransformed height of the node.
     */
    virtual void setContentSize(float width, float height) override {
        setContentSize(Size(width, height));
    }
    
#pragma mark -
#pragma mark Rendering
    /**
     * Draws this Node via the given SpriteBatch.
     *
     * This method only worries about drawing the current node.  It does not
     * attempt to render the children.
     *
     * This is the method that you should override to implement your custom
     * drawing code.  You are welcome to use any OpenGL commands that you wish.
     * You can even skip use of the SpriteBatch.  However, if you do so, you
     * must flush the SpriteBatch by calling end() at the start of the method.
     * in addition, you should remember to call begin() at the start of the
     * method.
     *
     * This method provides the correct transformation matrix and tint color.
     * You do not need to worry about whether the node uses relative color.
     * This method is called by render() and these values are guaranteed to be
     * correct.  In addition, this method does not need to check for visibility,
     * as it is guaranteed to only be called when the node is visible.
     *
     * @param batch     The SpriteBatch to draw with.
     * @param transform The global transformation matrix.
     * @param tint      The tint to blend with the Node color.
     */
    virtual void draw(const std::shared_ptr<SpriteBatch>& batch,
                      const Affine2& transform, Color4 tint) override = 0;
    
    /**
     * Refreshes this node to restore the render data.
     */
    void refresh() { clearRenderData(); generateRenderData(); }

    
protected:
    /**
     * Allocates the render data necessary to render this node.
     */
    virtual void generateRenderData() = 0;

    /**
     * Updates the texture coordinates for this polygon
     *
     * The texture coordinates are computed assuming that the polygon is
     * defined in image space, with the origin in the bottom left corner
     * of the texture.
     */
    virtual void updateTextureCoords() = 0;

    /**
     * Clears the render data, releasing all vertices and indices.
     */
    void clearRenderData();

    /** This macro disables the copy constructor (not allowed on scene graphs) */
    CU_DISALLOW_COPY_AND_ASSIGN(TexturedNode);

};
	}
}

#endif /* __CU_TEXTURED_NODE_H__ */
