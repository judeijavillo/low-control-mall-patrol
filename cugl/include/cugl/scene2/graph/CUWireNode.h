//
//  CUWireNode.h
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
#ifndef __CU_WIRE_NODE_H__
#define __CU_WIRE_NODE_H__

#include <string>
#include <cugl/math/CUPath2.h>
#include <cugl/math/CUPoly2.h>
#include <cugl/math/polygon/CUPolyEnums.h>
#include <cugl/scene2/graph/CUTexturedNode.h>

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
 * This is a scene graph node to represent a wireframe
 *
 * The wireframes are lines, but they can still be textured.  However, generally
 * you will only want to create a wireframe with the degenerate texture (to draw
 * a solid, colored line). Hence, none of the static constructors take a texture.
 * You are free to update the texture after creation, if you wish.
 *
 * The node shape is stored as polygon. The wireframe shape is determined by the
 * polygon traversal.  There are three options, defined in {@link poly2::Traversal}.
 *
 *     OPEN:     The traversal is in order, but does not close the ends.
 *
 *     CLOSED:   The traversal is in order, and closes the ends.
 *
 *     INTERIOR: The traverse will outline the default triangulation.
 *
 * The default is traversal is CLOSED.
 *
 * The wireframe can be textured (as lines can be textured).  The wireframe is
 * specified in image coordinates. Image coordinates are different from texture
 * coordinates. Their origin is at the bottom-left corner of the file, and each
 * pixel is one unit. This makes specifying to polygon more natural for
 * irregular shapes.
 *
 * This means that a wireframe with vertices (0,0), (width,0), (width,height),
 * and (0,height) would be the border of a sprite node. However, a wireframe
 * with vertices (0,0), (2*width,0), (2*width,2*height), and (0,2*height) would
 * tile the sprite texture (given the wrap settings) twice both horizontally
 * and vertically.
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
 * You can disable these features at any time by setting the attribute 
 * absolute to true.  Do this will place the polygon vertices in their 
 * absolute positions in Node space.  This will also disable anchor functions 
 * (setting the anchor as the bottom left corner), since anchors do not make 
 * sense when we are drawing vertices directly into the coordinate space.
 */
class WireNode : public TexturedNode {
#pragma mark Values
protected:
    /** The wireframe vertices */
	Poly2 _polygon;
    /** The wireframe indices */
    std::vector<Uint32> _indices;
    /** The current (known) traversal of this wireframe */
    poly2::Traversal _traversal;

public:
#pragma mark -
#pragma mark Constructors
    /**
     * Creates an empty wire frame with the degenerate texture.
     *
     * You must initialize this WireNode before use.
     *
     * NEVER USE A CONSTRUCTOR WITH NEW. If you want to allocate an object on
     * the heap, use one of the static constructors instead.
     */
    WireNode() : TexturedNode(), _traversal(poly2::Traversal::CLOSED) {
        _classname = "WireNode";
        _name = "WireNode";
    }
    
    /**
     * Releases all resources allocated with this node.
     *
     * This will release, but not necessarily delete the associated texture.
     * However, the polygon and drawing commands will be deleted and no
     * longer safe to use.
     */
    ~WireNode() { dispose(); }
    
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
    bool initWithPoly(const Poly2& poly);

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
    bool initWithPoly(const Rect rect);

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
    bool initWithPath(const Path2& path);

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
    bool initWithPath(const Rect rect);

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
    bool initWithPath(const std::vector<Vec2>& vertices);

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
    bool initWithTraversal(const Poly2& poly, poly2::Traversal traversal);
    
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
    bool initWithTraversal(const std::vector<Vec2>& vertices, const std::vector<Uint32>& indices);

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
    bool initWithTraversal(Vec2* vertices, size_t vsize, Uint32* indices, size_t isize);

    /**
     * Initializes a wire frame with the given JSON specificaton.
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
     * @return true if initialization was successful.
     */
    bool initWithData(const Scene2Loader* loader, const std::shared_ptr<JsonValue>& data) override;

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
     * Returns an empty wire frame.
     *
     * The underlying polygon is empty, and must be set via setPolygon.
     *
     * @return an empty wire frame.
     */
    static std::shared_ptr<WireNode> alloc() {
        std::shared_ptr<WireNode> node = std::make_shared<WireNode>();
        return (node->init() ? node : nullptr);
    }
    
    /**
     * Returns a newly allocated wire frame with the given polygon.
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
     * @return a newly allocated wire frame with the given polygon.
     */
    static std::shared_ptr<WireNode> allocWithPoly(const Poly2& poly) {
        std::shared_ptr<WireNode> node = std::make_shared<WireNode>();
        return (node->initWithPoly(poly) ? node : nullptr);
    }
    
    /**
     * Returns a newly allocated wire frame with the given (solid) rectangle.
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
     * @return a newly allocatedwire frame with the given (solid) rectangle.
     */
    static std::shared_ptr<WireNode> allocWithPoly(const Rect rect) {
        std::shared_ptr<WireNode> node = std::make_shared<WireNode>();
        return (node->initWithPoly(rect) ? node : nullptr);
    }
    
    /**
     * Returns a newly allocated wire frame with the given path.
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
     * @return a newly allocated wire frame with the given path.
     */
    static std::shared_ptr<WireNode> allocWithPath(const Path2& path) {
        std::shared_ptr<WireNode> node = std::make_shared<WireNode>();
        return (node->initWithPath(path) ? node : nullptr);
    }

    /**
     * Returns a newly allocated wire frame with the given rectangle (outline).
     *
     * This wireframe will perform a CLOSED traversal of the rectangle.
     *
     * You do not need to set the texture; rendering this into a SpriteBatch
     * will simply use the blank texture. Hence the wireframe will have a solid
     * color.
     *
     * @param rect  The rectangle to traverse
     *
     * @return a newly allocated wire frame with the given rectangle (outline).
     */
    static std::shared_ptr<WireNode> allocWithPath(const Rect rect) {
        std::shared_ptr<WireNode> node = std::make_shared<WireNode>();
        return (node->initWithPath(rect) ? node : nullptr);
    }

    /**
     * Returns a newly allocated wire frame with the given path.
     *
     * This wireframe will perform a CLOSED traversal of the given path.
     *
     * You do not need to set the texture; rendering this into a SpriteBatch
     * will simply use the blank texture. Hence the wireframe will have a solid
     * color.
     *
     * @param vertices  The path to traverse
     *
     * @return a newly allocated wire frame with the given path.
     */
    static std::shared_ptr<WireNode> allocWithPath(const std::vector<Vec2>& vertices) {
        std::shared_ptr<WireNode> node = std::make_shared<WireNode>();
        return (node->initWithPath(vertices) ? node : nullptr);
    }
   
    /**
     * Returns a newly allocated wire frame with the given polygon and traversal.
     *
     * The provided polygon will be used as the source for the traversal.
     * The traversal will be defined exactly as the one provided by
     * {@link PathFactory#makeTraversal}.
     *
     * You do not need to set the texture; rendering this into a SpriteBatch
     * will simply use the blank texture. Hence the wireframe will have a solid
     * color.
     *
     * @param poly      The triangulated polygon
     * @param traversal The path traversal for index generation
     *
     * @return a newly allocated wire frame with the given polygon and traversal.
     */
    static std::shared_ptr<WireNode> allocWithTraversal(const Poly2& poly,
                                                        poly2::Traversal traversal) {
        std::shared_ptr<WireNode> node = std::make_shared<WireNode>();
        return (node->initWithTraversal(poly,traversal) ? node : nullptr);
    }
    
    /**
     * Returns a newly allocated wire frame with the given vertices and indices
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
     * @return a newly allocated wire frame with the given vertices and indices
     */
    static std::shared_ptr<WireNode> allocWithTraversal(const std::vector<Vec2>& vertices,
                                                        const std::vector<Uint32>& indices) {
        std::shared_ptr<WireNode> node = std::make_shared<WireNode>();
        return (node->initWithTraversal(vertices,indices) ? node : nullptr);
    }
    
    /**
     * Returns a newly allocated wire frame with the given vertices and indices
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
     * @return a newly allocated wire frame with the given vertices and indices
     */
	static std::shared_ptr<WireNode> allocWithTraversal(Vec2* vertices, size_t vsize,
                                                        Uint32* indices, size_t isize) {
        std::shared_ptr<WireNode> node = std::make_shared<WireNode>();
        return (node->initWithTraversal(vertices,vsize,indices,isize) ? node : nullptr);
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
    static std::shared_ptr<SceneNode> allocWithData(const Scene2Loader* loader,
                                                    const std::shared_ptr<JsonValue>& data) {
        std::shared_ptr<WireNode> result = std::make_shared<WireNode>();
        if (!result->initWithData(loader,data)) { result = nullptr; }
        return std::dynamic_pointer_cast<SceneNode>(result);
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
    void setPolygon(const Poly2& poly);
    
    /**
     * Returns the wire frame polygon.
     *
     * The provided polygon will be used as the source for the traversal.
     * The polygon itself may or may not have any indices, as the polygon
     * indices are not the same as the traversal indices.
     *
     * The polygon is specified in image coordinates. The origin is at the
     * bottom-left corner of the image, and each pixel is one unit. If no
     * texture is specified, this node will draw the wire frame with a
     * solid color.
     *
     * @return the wire frame polygon
     */
    const Poly2& getPolygon() const { return _polygon; }
    
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
    void setPolygon(const Rect rect);

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
    void setPath(const Path2& path);

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
    void setPath(const Rect rect);
    
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
     * @param vertices  The wire frame path
     */
    void setPath(const std::vector<Vec2>& vertices);

    
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
    void setTraversal(poly2::Traversal traversal);

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
    void setTraversal(const std::vector<Uint32>& indices);
    
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
    void setTraversal(Uint32* indices, size_t isize);

    /**
     * Returns the current traversal algorithm of this path.
     *
     * If the traversal was defined by a custom set of indices, this method
     * returns NONE.
     *
     * @return the current traversal algorithm of this path.
     */
    poly2::Traversal getTraversal() const { return _traversal; }
    
    /**
     * Returns the current vertices of this wire frame.
     *
     * @return the current vertices of this wire frame.
     */
    const std::vector<Vec2>& getVertices() const { return _polygon.vertices; }

    /**
     * Returns the traversal indices of this wire frame.
     *
     * @return the traversal indices of this wire frame.
     */
    const std::vector<Uint32>& getIndices() const { return _indices; }
    

#pragma mark -
#pragma mark Rendering
    /**
     * Draws this wireframe via the given SpriteBatch.
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
    
#pragma mark -
#pragma mark Traversal Methods
    /**
     * Stores a wire frame of an existing polygon in the provided buffer.
     *
     * This method provides four types of traversals: `NONE`, `OPEN`, `CLOSED`
     * and `INTERIOR`. No traversal simply copies the given polygon. The open
     * and closed traversals apply to the boundary of the polygon. If there is
     * more than one boundary, then the closed traversal connects the boundaries
     * together in a single loop. Finally, the interior traversal creates a wire
     * frame a polygon triangulation.
     *
     * The traversal will be appended to the buffer.  You should clear the buffer
     * first if you do not want to preserve the original data.
     *
     * @param poly  The polygon to store the result
     * @param src   The source polygon to traverse
     * @param type  The traversal type
     *
     * @return a reference to the buffer for chaining.
     */
    void makeTraversal(const Poly2& src, poly2::Traversal type);
    
    /**
     * Stores a wire frame of an existing polygon in the provided buffer.
     *
     * This method is dedicated to an `OPEN` traversal.  See the description
     * of {@link #makeTraversal} for more information.  This method simply
     * exists to make the code more readable.
     *
     * The traversal will be appended to the buffer.  You should clear the buffer
     * first if you do not want to preserve the original data.
     *
     * @param poly  The polygon to store the result
     * @param src   The source polygon to traverse
     *
     * @return a reference to the buffer for chaining.
     */
    void makeBoundaryTraversal(const Poly2& src, bool closed=false);

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
     * @param poly  The polygon to store the result
     * @param src   The source polygon to traverse
     *
     * @return a reference to the buffer for chaining.
     */
    void makeInteriorTraversal(const Poly2& src);
    
    /** This macro disables the copy constructor (not allowed on scene graphs) */
    CU_DISALLOW_COPY_AND_ASSIGN(WireNode);
};
    
    }  
}


#endif /* __CU_WIRE_NODE_H__ */
