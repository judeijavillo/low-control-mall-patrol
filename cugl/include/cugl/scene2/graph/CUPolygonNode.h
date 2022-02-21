//
//  CUPolygonNode.h
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
#ifndef __CU_POLYGON_NODE_H__
#define __CU_POLYGON_NODE_H__

#include <string>
#include <cugl/scene2/graph/CUTexturedNode.h>
#include <cugl/math/polygon/CUEarclipTriangulator.h>

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
 * This is a scene graph node representing a textured solid 2D polygon.
 *
 * The polygon is specified in image coordinates. Image coordinates are different
 * from texture coordinates. Their origin is at the bottom-left corner of the file,
 * and each pixel is one unit. This makes specifying a polygon more natural for
 * irregular shapes.
 *
 * This means that a polygon with vertices (0,0), (width,0), (width,height),
 * and (0,height) would be identical to a sprite node. However, a polygon with
 * vertices (0,0), (2*width,0), (2*width,2*height), and (0,2*height) would tile
 * the sprite (given the wrap settings) twice both horizontally and vertically.
 *
 * The content size of this node is defined by the size (but not the offset)
 * of the bounding box.  The anchor point is relative to this content size.
 * The default anchor point in a TexturedNode is (0.5, 0.5).  This means that a
 * uniform translation of the polygon (in contrast to the node itself) will not
 * move the shape on the the screen.  Instead, it will just change the part of
 * the texture it uses.
 *
 * For example, suppose the texture has given width and height.  We have one
 * polygon with vertices (0,0), (width/2,0), (width/2,height/2), and (0,height/2).
 * We have another polygon with vertices (width/2,height/2), (width,height/2),
 * (width,height), and (width/2,height).  Both polygons would create a rectangle
 * of size (width/2,height/2). centered at the node position.  However, the
 * first would use the bottom left part of the texture, while the second would
 * use the top right.
 *
 * You can disable these features at any time by setting the attribute absolute
 * to true.  Doing this will place the polygon vertices in their absolute positions
 * in Node space. This will also disable anchor functions (setting the anchor as
 * the bottom left corner), since anchors do not make sense when we are drawing
 * vertices directly into the coordinate space.
 */
class PolygonNode : public TexturedNode {
#pragma mark Values
protected:
    /** The underlying polygon */
    Poly2 _polygon;
    /** The border fringe for the mesh */
    float _fringe;

public:
#pragma mark -
#pragma mark Constructors
    /**
     * Creates an empty polygon with the degenerate texture.
     *
     * You must initialize this PolygonNode before use.
     *
     * NEVER USE A CONSTRUCTOR WITH NEW. If you want to allocate an object on
     * the heap, use one of the static constructors instead.
     */
    PolygonNode() : TexturedNode(), _fringe(0) {
        _classname = "PolygonNode";
    }

    /**
     * Releases all resources allocated with this node.
     *
     * This will release, but not necessarily delete the associated texture.
     * However, the polygon and drawing commands will be deleted and no
     * longer safe to use.
     */
    ~PolygonNode() { dispose(); }

    /**
     * Intializes a polygon node with the given vertices.
     *
     * You do not need to set the texture; rendering this into a SpriteBatch
     * will simply use the blank texture. Hence the polygon will have a solid
     * color.
     *
     * The vertices will be triangulated with {@link EarclipTriangulator}.
     *
     * @param vertices  The vertices to texture (expressed in image space)
     *
     * @return  true if the sprite is initialized properly, false otherwise.
     */
    bool initWithPoly(const std::vector<Vec2>& vertices) {
        return initWithTexturePoly(nullptr, vertices);
    }
    
    /**
     * Intializes a polygon node given polygon shape.
     *
     * You do not need to set the texture; rendering this into a SpriteBatch
     * will simply use the blank texture. Hence the polygon will have a solid
     * color.
     *
     * @param poly      The polygon to texture
     *
     * @return  true if the sprite is initialized properly, false otherwise.
     */
    bool  initWithPoly(const Poly2& poly) {
        return initWithTexturePoly(nullptr, poly);
    }
    
    /**
     * Intializes a polygon node with the given rect.
     *
     * You do not need to set the texture; rendering this into a SpriteBatch
     * will simply use the blank texture. Hence the polygon will have a solid
     * color.
     *
     * The rectangle will be triangulated with the standard two triangles.
     *
     * @param rect      The rectangle to texture
     *
     * @return  true if the sprite is initialized properly, false otherwise.
     */
    bool  initWithPoly(const Rect rect) {
        return initWithTexturePoly(nullptr, rect);
    }
    
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
    virtual bool initWithFile(const std::string& filename) override;
    
    /**
     * Initializes a polygon node from the image filename and the given vertices.
     *
     * The vertices will define the portion of the of the texture shown (in image 
     * space). The vertices will be triangulated with {@link EarclipTriangulator}.
     *
     * @param filename  A path to image file, e.g., "scene1/earthtile.png"
     * @param vertices  The vertices to texture (expressed in image space)
     *
     * @return  true if the sprite is initialized properly, false otherwise.
     */
    bool initWithFilePoly(const std::string& filename, const std::vector<Vec2>& vertices) {
        setPolygon(vertices);
        return TexturedNode::initWithFile(filename);
    }
    
    /**
     * Initializes a polygon node from the image filename and the given polygon.
     *
     * The polygon will define the portion of the of the texture shown (in image 
     * space).
     *
     * @param filename  A path to image file, e.g., "scene1/earthtile.png"
     * @param poly      The polygon to texture
     *
     * @return  true if the sprite is initialized properly, false otherwise.
     */
    bool initWithFilePoly(const std::string& filename, const Poly2& poly) {
        setPolygon(poly);
        return TexturedNode::initWithFile(filename);
    }
    
    /**
     * Initializes a polygon node from the image filename and the given rect.
     *
     * The vertices of the rectangle will define the portion of the texture shown
     * (in image space). The rectangle will be triangulated with the standard two 
     * triangles. 
     *
     * @param filename  A path to image file, e.g., "scene1/earthtile.png"
     * @param rect      The rectangle to texture
     *
     * @return  true if the sprite is initialized properly, false otherwise.
     */
    bool initWithFilePoly(const std::string& filename, const Rect rect) {
        setPolygon(rect);
        return TexturedNode::initWithFile(filename);
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
    virtual bool initWithTexture(const std::shared_ptr<Texture>& texture) override;
    
    /**
     * Initializes a polygon node from a Texture object and the given vertices.
     *
     * The vertices will define the portion of the of the texture shown (in image 
     * space). The vertices will be triangulated with {@link EarclipTriangulator}.
     *
     * @param texture    A shared pointer to a Texture object.
     * @param vertices   The vertices to texture (expressed in image space)
     *
     * @return  true if the sprite is initialized properly, false otherwise.
     */
    bool initWithTexturePoly(const std::shared_ptr<Texture>&  texture, const std::vector<Vec2>& vertices) {
        setPolygon(vertices);
        return TexturedNode::initWithTexture(texture);
    }
    
    /**
     * Initializes a polygon node from a Texture object and the given polygon.
     *
     * The polygon will define the portion of the of the texture shown (in image 
     * space).
     *
     * @param texture   A shared pointer to a Texture object.
     * @param poly      The polygon to texture
     *
     * @return  true if the sprite is initialized properly, false otherwise.
     */
    bool initWithTexturePoly(const std::shared_ptr<Texture>& texture, const Poly2& poly) {
        setPolygon(poly);
        return TexturedNode::initWithTexture(texture);
    }
    
    /**
     * Initializes a polygon node from a Texture object and the given rect.
     *
     * The vertices of the rectangle will define the portion of the texture shown
     * (in image space). The rectangle will be triangulated with the standard two 
     * triangles. 
     *
     * @param texture   A shared pointer to a Texture object.
     * @param rect      The rectangle to texture
     *
     * @return  true if the sprite is initialized properly, false otherwise.
     */
    bool initWithTexturePoly(const std::shared_ptr<Texture>& texture, const Rect rect) {
        setPolygon(rect);
        return TexturedNode::initWithTexture(texture);
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
#pragma mark Static Constructors
    /**
     * Returns an empty polygon with the degenerate texture.
     *
     * You do not need to set the texture; rendering this into a SpriteBatch
     * will simply use the blank texture. The polygon, however, will also be
     * empty, and must be set via setPolygon.
     *
     * @return an empty polygon with the degenerate texture.
     */
    static std::shared_ptr<PolygonNode> alloc() {
        std::shared_ptr<PolygonNode> node = std::make_shared<PolygonNode>();
        return (node->init() ? node : nullptr);
    }
    
    /**
     * Returns a new polygon node with the given vertices.
     *
     * You do not need to set the texture; rendering this into a SpriteBatch
     * will simply use the blank texture. Hence the polygon will have a solid
     * color.
     *
     * The vertices will be triangulated with {@link EarclipTriangulator}.
     *
     * @param vertices  The vertices to texture (expressed in image space)
     *
     * @return a new polygon node with the given vertices.
     */
    static std::shared_ptr<PolygonNode> allocWithPoly(const std::vector<Vec2>& vertices) {
        std::shared_ptr<PolygonNode> node = std::make_shared<PolygonNode>();
        return (node->initWithPoly(vertices) ? node : nullptr);
    }
    
    /**
     * Returns a new polygon node with the given shape.
     *
     * You do not need to set the texture; rendering this into a SpriteBatch
     * will simply use the blank texture. Hence the polygon will have a solid
     * color.
     *
     * @param poly  The polygon to texture
     *
     * @return a new polygon node with the given shape.
     */
    static std::shared_ptr<PolygonNode> allocWithPoly(const Poly2& poly) {
        std::shared_ptr<PolygonNode> node = std::make_shared<PolygonNode>();
        return (node->initWithPoly(poly) ? node : nullptr);
    }
    
    /**
     * Returns a new polygon node with the given rect.
     *
     * You do not need to set the texture; rendering this into a SpriteBatch
     * will simply use the blank texture. Hence the polygon will have a solid
     * color.
     *
     * The rectangle will be triangulated with the standard two triangles.
     *
     * @param rect  The rectangle to texture
     *
     * @return a new polygon node with the given rect.
     */
    static std::shared_ptr<PolygonNode> allocWithPoly(const Rect rect) {
        std::shared_ptr<PolygonNode> node = std::make_shared<PolygonNode>();
        return (node->initWithPoly(rect) ? node : nullptr);
    }
    
    /**
     * Returns a new polygon node from the image filename.
     *
     * After creation, the polygon will be a rectangle. The vertices of this
     * polygon will be the corners of the image. The rectangle will be
     * triangulated with the standard two triangles.
     *
     * @param filename  A path to image file, e.g., "scene1/earthtile.png"
     *
     * @return a new polygon node from the image filename.
     */
    static std::shared_ptr<PolygonNode> allocWithFile(const std::string& filename) {
        std::shared_ptr<PolygonNode> node = std::make_shared<PolygonNode>();
        return (node->initWithFile(filename) ? node : nullptr);
    }
    
    /**
     * Returns a new polygon node from the image filename and the given vertices.
     *
     * The vertices will define the portion of the of the texture shown (in image
     * space). The vertices will be triangulated with {@link EarclipTriangulator}.
     *
     * @param filename  A path to image file, e.g., "scene1/earthtile.png"
     * @param vertices  The vertices to texture (expressed in image space)
     *
     * @return a new polygon node from the image filename and the given vertices.
     */
    static std::shared_ptr<PolygonNode> allocWithFilePoly(const std::string& filename,
                                                          const std::vector<Vec2>& vertices) {
        std::shared_ptr<PolygonNode> node = std::make_shared<PolygonNode>();
        return (node->initWithFilePoly(filename,vertices) ? node : nullptr);
    }
    
    /**
     * Returns a new polygon node from the image filename and the given polygon.
     *
     * The polygon will define the portion of the of the texture shown (in image
     * space).
     *
     * @param filename  A path to image file, e.g., "scene1/earthtile.png"
     * @param poly      The polygon to texture
     *
     * @return a new polygon node from the image filename and the given polygon.
     */
    static std::shared_ptr<PolygonNode> allocWithFilePoly(const std::string& filename,
                                                          const Poly2& poly) {
        std::shared_ptr<PolygonNode> node = std::make_shared<PolygonNode>();
        return (node->initWithFilePoly(filename,poly) ? node : nullptr);
    }
    
    /**
     * Returns a new polygon node from the image filename and the given rect.
     *
     * The vertices of the rectangle will define the portion of the texture shown
     * (in image space). The rectangle will be triangulated with the standard two
     * triangles.
     *
     * @param filename  A path to image file, e.g., "scene1/earthtile.png"
     * @param rect      The rectangle to texture
     *
     * @return a new polygon node from the image filename and the given rect.
     */
    static std::shared_ptr<PolygonNode> allocWithFilePoly(const std::string& filename,
                                                          const Rect rect) {
        std::shared_ptr<PolygonNode> node = std::make_shared<PolygonNode>();
        return (node->initWithFilePoly(filename,rect) ? node : nullptr);
    }
    
    /**
     * Returns a new polygon node from a Texture object.
     *
     * After creation, the polygon will be a rectangle. The vertices of this
     * polygon will be the corners of the image. The rectangle will be
     * triangulated with the standard two triangles.
     *
     * @param texture   A shared pointer to a Texture object.
     *
     * @return a new polygon node from a Texture object.
     */
    static std::shared_ptr<PolygonNode> allocWithTexture(const std::shared_ptr<Texture>& texture) {
        std::shared_ptr<PolygonNode> node = std::make_shared<PolygonNode>();
        return (node->initWithTexture(texture) ? node : nullptr);
    }
    
    /**
     * Returns a new polygon node from a Texture object and the given vertices.
     *
     * The vertices will define the portion of the of the texture shown (in image
     * space). The vertices will be triangulated with {@link EarclipTriangulator}.
     *
     * @param texture   A shared pointer to a Texture object.
     * @param vertices  The vertices to texture (expressed in image space)
     *
     * @return a new polygon node from a Texture object and the given vertices.
     */
    static std::shared_ptr<PolygonNode> allocWithTexture(const std::shared_ptr<Texture>& texture,
                                                         const std::vector<Vec2>& vertices) {
        std::shared_ptr<PolygonNode> node = std::make_shared<PolygonNode>();
        return (node->initWithTexturePoly(texture,vertices) ? node : nullptr);
    }
    /**
     * Returns a new polygon node from a Texture object and the given polygon.
     *
     * The polygon will define the portion of the of the texture shown (in image
     * space).
     *
     * @param texture   A shared pointer to a Texture object.
     * @param poly      The polygon to texture
     *
     * @return a new polygon node from a Texture object and the given polygon.
     */
    static std::shared_ptr<PolygonNode> allocWithTexture(const std::shared_ptr<Texture>& texture,
                                                         const Poly2& poly) {
        std::shared_ptr<PolygonNode> node = std::make_shared<PolygonNode>();
        return (node->initWithTexturePoly(texture,poly) ? node : nullptr);
    }
    
    /**
     * Returns a new polygon node from a Texture object and the given rect.
     *
     * The vertices of the rectangle will define the portion of the texture shown
     * (in image space). The rectangle will be triangulated with the standard two
     * triangles.
     *
     * @param texture   A shared pointer to a Texture object.
     * @param rect      The rectangle to texture
     *
     * @return a new polygon node from a Texture object and the given rect.
     */
    static std::shared_ptr<PolygonNode> allocWithTexture(const std::shared_ptr<Texture>& texture,
                                                         const Rect rect)  {
        std::shared_ptr<PolygonNode> node = std::make_shared<PolygonNode>();
        return (node->initWithTexturePoly(texture,rect) ? node : nullptr);
    }

    /**
     * Returns a new polygon node with the given JSON specificaton.
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
     * @return a new polygon with the given JSON specificaton.
     */
    static std::shared_ptr<SceneNode> allocWithData(const Scene2Loader* loader,
                                                    const std::shared_ptr<JsonValue>& data) {
        std::shared_ptr<PolygonNode> result = std::make_shared<PolygonNode>();
        if (!result->initWithData(loader,data)) { result = nullptr; }
        return std::dynamic_pointer_cast<SceneNode>(result);
    }
    
#pragma mark -
#pragma mark Polygon Attributes
    /**
     * Returns the antialiasing fringe for this polygon node
     *
     * If this value is non-zero, the node will surround the polygon with a
     * stroke the width of the fringe. The stroke will fade to transparent
     * on the outside edge. This is a way of providing antialiasing that is
     * significantly better than multisampling. Furthermore, this works on
     * OpenGLES, which does not support multisampling.
     *
     * Creating a fringe does introduce significant overhead (tenths of a
     * millisecond). The algorithm must detriangulate the polygon to find
     * the borders and then extrude those borders. In addition, this effect
     * is often unnecessary on retina/high-dpi displays. As a result, the
     * default fringe value is 0.
     *
     * A fringe value should be >= 0.5 to have noticeable effects. In
     * practice, values between 1 and 2 work best.
     *
     * @return the antialiasing fringe for this polygon node
     */
    float getFringe() const { return _fringe; }
    
    /**
     * Sets the antialiasing fringe for this polygon node
     *
     * If this value is non-zero, the node will surround the polygon with a
     * stroke the width of the fringe. The stroke will fade to transparent
     * on the outside edge. This is a way of providing antialiasing that is
     * significantly better than multisampling. Furthermore, this works on
     * OpenGLES, which does not support multisampling.
     *
     * Creating a fringe does introduce significant overhead (tenths of a
     * millisecond). The algorithm must detriangulate the polygon to find
     * the borders and then extrude those borders. In addition, this effect
     * is often unnecessary on retina/high-dpi displays. As a result, the
     * default fringe value is 0.
     *
     * A fringe value should be >= 0.5 to have noticeable effects. In
     * practice, values between 1 and 2 work best.
     *
     * @param fringe    The antialiasing fringe for this polygon node
     */
    void setFringe(float fringe) { _fringe = fringe; clearRenderData(); }
    
    /**
     * Sets the polgon to the vertices expressed in texture space.
     *
     * The vertices will be triangulated with {@link EarclipTriangulator}.
     *
     * @param vertices    The vertices to texture
     */
    virtual void setPolygon(const std::vector<Vec2>& vertices);
    
    /**
     * Sets the polygon to the given one in texture space.
     *
     * @param poly  The polygon to texture
     */
    virtual void setPolygon(const Poly2& poly);
    
    /**
     * Sets the texture polygon to one equivalent to the given rect.
     *
     * The rectangle will be triangulated with the standard two triangles.
     *
     * @param rect  The rectangle to texture
     */
    virtual void setPolygon(const Rect rect);
    
    /**
     * Returns the texture polygon for this scene graph node
     *
     * @returns the texture polygon for this scene graph node
     */
    const Poly2& getPolygon() const { return _polygon; }

    /**
     * Returns the rect of the Polygon in points
     *
     * The bounding rect is the smallest rectangle containing all
     * of the points in the polygon.
     *
     * This value also defines the content size of the node. The
     * polygon will be shifted so that its bounding rect is centered
     * at the node center.
     */
    const Rect getBoundingRect() const { return _polygon.getBounds(); }
    
    
#pragma mark -
#pragma mark Rendering
    /**
     * Draws this polygon node via the given SpriteBatch.
     *
     * This method only worries about drawing the current node.  It does not
     * attempt to render the children.
     *
     * @param batch     The SpriteBatch to draw with.
     * @param transform The global transformation matrix.
     * @param tint      The tint to blend with the Node color.
     */
    virtual void draw(const std::shared_ptr<SpriteBatch>& batch, const Affine2& transform, Color4 tint) override;

    
#pragma mark -
#pragma mark Internal Helpers
private:
    /**
     * Allocate the render data necessary to render this node.
     */
    virtual void generateRenderData() override;

    /**
     * Updates the texture coordinates for this polygon
     *
     * The texture coordinates are computed assuming that the polygon is
     * defined in image space, with the origin in the bottom left corner
     * of the texture.
     */
    virtual void updateTextureCoords() override;

    /** This macro disables the copy constructor (not allowed on scene graphs) */
    CU_DISALLOW_COPY_AND_ASSIGN(PolygonNode);

};
    }
}

#endif /* __CU_POLYGON_NODE_H__ */
