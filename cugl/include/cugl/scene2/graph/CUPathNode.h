//
//  CUPathNode.h
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
#ifndef __CU_PATH_NODE_H__
#define __CU_PATH_NODE_H__

#include <string>
#include <cugl/math/CUPath2.h>
#include <cugl/scene2/graph/CUTexturedNode.h>
#include <cugl/math/polygon/CUSimpleExtruder.h>

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
 * This is a scene graph node to represent a path with width.
 *
 * At first glance, it would appear that this class is unnecessary. A path
 * with width, produced by {@link SimpleExtruder}, is a solid polygon.  This
 * polygon can, in turn, be used in conjunction with {@link PolygonNode}.
 *
 * However, there are some subtle issues. In particular, mitres and joints
 * may mean that a PathNode and {@link WireNode} at the same position will
 * not line up with one another. This is undesirable. Hence this is a special
 * polygon node that takes into account that it is an extruded path.
 *
 * One of the side effects of this is that the content size of the node is
 * defined by the path, NOT the extruded polygon.  If you want the bounds of
 * the extruded path (relative to Node space), you should use the method
 * {@link getExtrudedContentBounds()}. Additionally, the anchor point is
 * relative to the content size not the extruded size. This means that the
 * extruded path may be to the left of the origin even when the anchor is
 * at (0,0).
 *
 * Because paths have width, it is natural to texture them. However, generally
 * you will only want to create a path with the degenerate texture (to draw
 * a solid, colored path). Hence, none of the static constructors take a
 * texture. You are free to update the texture after creation, if you wish.
 *
 * The extruded polygon is specified in image coordinates. Image coordinates are
 * different from texture coordinates. Their origin is at the bottom-left corner
 * of the file, and each pixel is one unit. This makes specifying to polygon more
 * natural for irregular shapes.
 *
 * This means that an extrusion with vertices (0,0), (width,0), (width,height),
 * and (0,height) would be identical to a sprite node. However, an extrusion with
 * vertices (0,0), (2*width,0), (2*width,2*height), and (0,2*height) would tile
 * the sprite (given the wrap settings) twice both horizontally and vertically.
 * A uniform translation of the extrusion (in contrast to the node itself) will
 * not move the shape on the the screen.  Instead, it will just change the part
 * or the texture it uses.
 *
 * For example, suppose the texture has given width and height.  We have one
 * extrusion with vertices (0,0), (width/2,0), (width/2,height/2), and (0,height/2).
 * We have another extrusion with vertices (width/2,height/2), (width,height/2),
 * (width,height), and (width/2,height).  Both extrusion would have size
 * (width/2,height/2) and be centered at the node position.  However, the first
 * would use the bottom left part of the texture, while the second would use
 * the top right.
 *
 * You can disable these features at any time by setting the attribute absolute
 * to true.  Do this will place the path vertices in their absolute positions in
 * Node space.  This will also disable anchor functions (setting the anchor as
 * the bottom left corner), since anchors do not make sense when we are drawing
 * vertices directly into the coordinate space.
 */
class PathNode : public TexturedNode {
#pragma mark Values
protected:
    /** The path defining this node */
    Path2 _path;
    /** The extruded path as a solid polygon */
    Poly2 _polygon;
    /** The stroke width of this path. */
    float _stroke;
    /** The joint between segments of the path. */
    poly2::Joint  _joint;
    /** The shape of the two end caps of the path. */
    poly2::EndCap _endcap;
    /** The extruded bounds */
    Rect _extrabounds;

    /** The border fringe for the mesh */
    float _fringe;
    /** Whether to stencil the path (for overlaps) */
    bool _stencil;
    /** The extruder for this node */
    SimpleExtruder _extruder;
    /** The fringe mesh */
    Mesh<SpriteVertex2> _border;
    
public:
#pragma mark -
#pragma mark Constructors
    /**
     * Creates an empty path node.
     *
     * You must initialize this PathNode before use.
     *
     * NEVER USE A CONSTRUCTOR WITH NEW. If you want to allocate an object on
     * the heap, use one of the static constructors instead.
     */
    PathNode();
    
    /**
     * Releases all resources allocated with this node.
     *
     * This will release, but not necessarily delete the associated texture.
     * However, the polygon and drawing commands will be deleted and no
     * longer safe to use.
     */
    ~PathNode() { dispose(); }
    
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
    bool initWithPath(const std::vector<Vec2>& vertices, float stroke,
                      poly2::Joint joint = poly2::Joint::SQUARE,
                      poly2::EndCap cap = poly2::EndCap::BUTT,
                      bool closed=true);

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
    bool initWithPath(const Path2& path, float stroke,
                      poly2::Joint joint = poly2::Joint::SQUARE,
                      poly2::EndCap cap = poly2::EndCap::BUTT);

    /**
     * Initializes a path node from the image filename and the path.
     *
     * @param filename  A path to image file, e.g., "scene1/earthtile.png"
     * @param path      The path to texture
     *
     * @return  true if the node is initialized properly, false otherwise.
     */
    bool initWithFilePath(const std::string& filename, const Path2& path);
    
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
    bool initWithFilePath(const std::string& filename, const Rect rect);
    
    /**
     * Initializes a path node from a texture and the given path.
     *
     * @param texture   A shared pointer to a Texture object.
     * @param path      The path to texture
     *
     * @return  true if the sprite is initialized properly, false otherwise.
     */
    bool initWithTexturePath(const std::shared_ptr<Texture>& texture, const Path2& path);
    
    /**
     * Initializes a path node from a texture and the given rect.
     *
     * The conversion of rectangle to polygon is subclass specific.
     *
     * @param texture   A shared pointer to a Texture object.
     * @param rect      The rectangle to texture
     *
     * @return  true if the sprite is initialized properly, false otherwise.
     */
    bool initWithTexturePath(const std::shared_ptr<Texture>& texture, const Rect rect);
    
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
     * Returns an empty path node.
     *
     * The underlying path is empty, and must be set via setPath().
     *
     * @return an empty path node.
     */
    static std::shared_ptr<PathNode> alloc() {
        std::shared_ptr<PathNode> node = std::make_shared<PathNode>();
        return (node->init() ? node : nullptr);
    }
    
    /**
     * Returns a new path node with the given vertices and stroke width.
     *
     * This method will extrude the vertices with the specified joint and cap.
     * This method uses {@link SimpleExtruder}, as it is safe for framerate
     * calculation.
     *
     * @param vertices  The vertices to texture (expressed in image space)
     * @param stroke    The stroke width of the extruded path.
     * @param joint     The joint between extrusion line segments.
     * @param cap       The end caps of the extruded paths.
     * @param closed    The whether the vertex path is open or closed.
     *
     * @return a new path node with the given vertices and stroke width.
     */
    static std::shared_ptr<PathNode> allocWithVertices(const std::vector<Vec2>& vertices, float stroke,
                                                       poly2::Joint joint = poly2::Joint::SQUARE,
                                                       poly2::EndCap cap = poly2::EndCap::BUTT,
                                                       bool closed = true) {
        std::shared_ptr<PathNode> node = std::make_shared<PathNode>();
        return (node->initWithPath(vertices,stroke,joint,cap,closed) ? node : nullptr);
    }
    
    /**
     * Returns a new path node with the given path and stroke width.
     *
     * This method will extrude the path with the specified joint and cap.
     * This method uses {@link SimpleExtruder}, as it is safe for framerate
     * calculation.
     *
     * @param path      The path to texture (expressed in image space)
     * @param stroke    The stroke width of the extruded path.
     * @param joint     The joint between extrusion line segments.
     * @param cap       The end caps of the extruded paths.
     *
     * @return a new path node with the given polygon and stroke width.
     */
    static std::shared_ptr<PathNode> allocWithPath(const Path2& path, float stroke,
                                                   poly2::Joint joint = poly2::Joint::SQUARE,
                                                   poly2::EndCap cap = poly2::EndCap::BUTT) {
        std::shared_ptr<PathNode> node = std::make_shared<PathNode>();
        return (node->initWithPath(path,stroke,joint,cap) ? node : nullptr);
    }
    
    /**
     * Returns a new path node with the given rect and stroke width.
     *
     * The rectangle will be converted into a Poly2, using the standard outline.
     * This is the same as passing Poly2(rect,false).  The traversal will be
     * CLOSED. It will then be extruded with the current joint and cap. PathNode
     * objects share a single extruder, so this constructor is not thread safe.
     *
     * @param rect      The rectangle for to texture.
     * @param stroke    The stroke width of the extruded path.
     * @param joint     The joint between extrusion line segments.
     * @param cap       The end caps of the extruded paths.
     *
     * @return a new path with the given rect and stroke width.
     */
    static std::shared_ptr<PathNode> allocWithRect(const Rect rect, float stroke,
                                                   poly2::Joint joint = poly2::Joint::SQUARE,
                                                   poly2::EndCap cap = poly2::EndCap::BUTT) {
        std::shared_ptr<PathNode> node = std::make_shared<PathNode>();
        return (node->initWithPath(Path2(rect),stroke,joint,cap) ? node : nullptr);
    }
    
    /**
     * Returns a new path node with the given JSON specificaton.
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
     * @return a new path node with the given JSON specificaton.
     */
    static std::shared_ptr<SceneNode> allocWithData(const Scene2Loader* loader,
                                                    const std::shared_ptr<JsonValue>& data) {
        std::shared_ptr<PathNode> result = std::make_shared<PathNode>();
        if (!result->initWithData(loader,data)) { result = nullptr; }
        return std::dynamic_pointer_cast<SceneNode>(result);
    }
    
    
#pragma mark -
#pragma mark Extrusion Attributes
    /**
     * Sets the stroke width of the path.
     *
     * This method affects the extruded polygon, but not the source path
     * polygon.
     *
     * @param stroke    The stroke width of the path
     */
    void setStroke(float stroke);
    
    /**
     * Returns the stroke width of the path.
     *
     * @return the stroke width of the path.
     */
    float getStroke() const { return _stroke; }
    
    /**
     * Sets whether the path is closed.
     *
     * If set to true, this will smooth the polygon to remove all gaps,
     * regardless of the original inidices in the polygon. Furthermore,
     * previous information about existing gaps is lost, so that setting
     * the value back to false will only open the curve at the end.
     *
     * @param closed  Whether the path is closed.
     */
    void setClosed(bool closed);
    
    /**
     * Returns whether the path is closed.
     *
     * If set to true, this will smooth the polygon to remove all gaps,
     * regardless of the original inidices in the polygon. Furthermore,
     * previous information about existing gaps is lost, so that setting
     * the value back to false will only open the curve at the end.
     *
     * @return whether the path is closed.
     */
    bool getClosed() const { return _path.isClosed(); }
    
    /**
     * Sets the joint type between path segments.
     *
     * This method affects the extruded polygon, but not the original path
     * polygon.
     *
     * @param joint The joint type between path segments
     */
    void setJoint(poly2::Joint joint);
    
    /**
     * Returns the joint type between path segments.
     *
     * @return the joint type between path segments.
     */
    poly2::Joint getJoint() const { return _joint; }
    
    /**
     * Sets the cap shape at the ends of the path.
     *
     * This method affects the extruded polygon, but not the original path
     * polygon.
     *
     * @param cap   The cap shape at the ends of the path.
     */
    void setCap(poly2::EndCap cap);
    
    /**
     * Returns the cap shape at the ends of the path.
     *
     * @return the cap shape at the ends of the path.
     */
    poly2::EndCap getCap() const { return _endcap; }
    
    /**
     * Returns the antialiasing fringe for this polygon node
     *
     * If this value is non-zero, the node will surround the stroke with an
     * additional stroke the width of the fringe. The second stroke will fade
     * to transparent on the outside edge. This is a way of providing anti-
     * aliasing that is significantly better than multisampling. Furthermore,
     * this works on OpenGLES, which does not support multisampling.
     *
     * Creating a fringe does introduce some overhead. The extruder must do
     * a second pass on the boundary of the first stroke (which was computed
     * during the first extrusion). In addition, this effect is often
     * unnecessary on retina/high-dpi displays. As a result, the default
     * fringe value is 0.
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
     * If this value is non-zero, the node will surround the stroke with an
     * additional stroke the width of the fringe. The second stroke will fade
     * to transparent on the outside edge. This is a way of providing anti-
     * aliasing that is significantly better than multisampling. Furthermore,
     * this works on OpenGLES, which does not support multisampling.
     *
     * Creating a fringe does introduce some overhead. The extruder must do
     * a second pass on the boundary of the first stroke (which was computed
     * during the first extrusion). In addition, this effect is often
     * unnecessary on retina/high-dpi displays. As a result, the default
     * fringe value is 0.
     *
     * A fringe value should be >= 0.5 to have noticeable effects. In
     * practice, values between 1 and 2 work best.
     *
     * @param fringe    The antialiasing fringe for this polygon node
     */
    void setFringe(float fringe) { _fringe = fringe; clearRenderData(); }
    
    /**
     * Returns true if this node uses stencil effects
     *
     * Stencil effects are only necessary if the stroke both overlaps itself
     * and has transparency. These overlaps can cause weird artifacts in the
     * transparent regions, as they blend together. The stencil effect makes
     * sure that the stroke appears as one uniform polygon with no overlaps.
     *
     * By default this value is false. However, adding a fringe to a stroke
     * guarantees a transparent region.  Therefore, we recommend turning it
     * on when using a fringe.
     *
     * @return true if this node uses stencil effects
     */
    bool hasStencil() const { return _stencil; }
    
    /**
     * Sets whether to use stencil effects in this node
     *
     * Stencil effects are only necessary if the stroke both overlaps itself
     * and has transparency. These overlaps can cause weird artifacts in the
     * transparent regions, as they blend together. The stencil effect makes
     * sure that the stroke appears as one uniform polygon with no overlaps.
     *
     * By default this value is false. However, adding a fringe to a stroke
     * guarantees a transparent region.  Therefore, we recommend turning it
     * on when using a fringe.
     *
     * @param stencil    Whether to use stencil effects in this node
     */
    void setStencil(bool stencil) { _stencil = stencil; }

#pragma mark -
#pragma mark Path Attributes
    /**
     * Sets the path to the vertices expressed in texture space.
     *
     * @param vertices  The vertices to texture
     * @param closed    Whether the path is closed
     */
    void setPath(const std::vector<Vec2>& vertices, bool closed=true);
    
    /**
     * Sets the path to the given one in texture space.
     *
     * @param path  The path to texture
     */
    void setPath(const Path2& path);
    
    /**
     * Sets the texture path to one equivalent to the given rect.
     *
     * The rectangle will be extruded with mitre joints.
     *
     * @param rect  The rectangle to texture
     */
    void setPath(const Rect rect);
    
    /**
     * Returns the base path for this scene graph node
     *
     * @returns the base path for this scene graph node
     */
    const Path2& getPath() const { return _path; }

    /**
     * Returns the extruded path for this scene graph node
     *
     * @returns the extruded path for this scene graph node
     */
    const Poly2& getExtrusion() const { return _polygon; }

    
#pragma mark -
#pragma mark Bounds

    /**
     * Returns the rect of the Polygon in points
     *
     * The bounding rect is the smallest rectangle containing all
     * of the points in the polygon.
     *
     * This value also defines the content size of the node.  The
     * polygon will be shifted so that its bounding rect is centered
     * at the node center.
     */
    const Rect getBoundingRect() const { return _path.getBounds(); }

    /**
     * Returns the width of the extruded content.
     *
     * This method is an alternative to {@link getContentWidth()}. That method
     * only returns the content width of the path polygon; it does not include
     * the stroke width, mitres, and caps.  This method includes the extra
     * width of the extruded path.
     *
     * @return the width of the extruded content.
     */
    float getExtrudedContentWidth() const { return _extrabounds.size.width; }
    
    /**
     * Returns the height of the extruded content.
     *
     * This method is an alternative to {@link getContentHeight()}. That method
     * only returns the content height of the path polygon; it does not include
     * the stroke width, mitres, and caps.  This method includes the extra
     * height of the extruded path.
     *
     * @return the height of the extruded content.
     */
    float getExtrudedContentHeight() const { return _extrabounds.size.height; }
    
    /**
     * Returns the size of the extruded content.
     *
     * This method is an alternative to {@link getContentSize()}. That method
     * only returns the content size of the path polygon; it does not include
     * the stroke width, mitres, and caps.  This method includes the extra
     * size of the extruded path.
     *
     * @return the size of the extruded content.
     */
    const Size getExtrudedContentSize() const { return _extrabounds.size; }
    
    /**
     * Returns the bounding box of the extruded content.
     *
     * This method is similar to {@link getExtrudedContentSize()} in that it
     * returns the extra content size created by the stroke width, mitres, and
     * caps. In addition, it returns the actual bounds of the path within
     * Node space.
     *
     * Unlike {@link getBoundingBox()}, this method is unaffected by any
     * transforms on this node.
     *
     * @return the bounding box of the extruded content.
     */
    const Rect getExtrudedContentBounds() const { return _extrabounds; }

    
#pragma mark -
#pragma mark Rendering
    /**
     * Draws this path node via the given SpriteBatch.
     *
     * This method only worries about drawing the current node.  It does not
     * attempt to render the children.
     *
     * @param batch     The SpriteBatch to draw with.
     * @param transform The global transformation matrix.
     * @param tint      The tint to blend with the Node color.
     */
    virtual void draw(const std::shared_ptr<SpriteBatch>& batch, const Affine2& transform, Color4 tint) override;

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
    
    /**
     * Updates the extrusion polygon, based on the current settings.
     *
     * This method uses {@link SimpleExtruder}, as it is safe for framerate
     * calculation.
     */
    void updateExtrusion();

    /** This macro disables the copy constructor (not allowed on scene graphs) */
    CU_DISALLOW_COPY_AND_ASSIGN(PathNode);
};
    }
}

#endif /* __CU_PATH_NODE_H__ */
