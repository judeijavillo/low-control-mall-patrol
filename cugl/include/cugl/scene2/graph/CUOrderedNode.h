//
//  CUOrderedNode.h
//  Cornell University Game Library (CUGL)
//
//  This module provides a scene graph node that can arbitrary reorder the
//  rendering of its children.  The lack of this feature has frustrated the
//  students for years. It is the primary issue that has limited the scene
//  graph to UI elements, and made it less suitable for character animation.
//
//  Render order is managed by the priority attribute in the base SceneNode
//  class. You should set these values to manually arrange your scene graph
//  elements
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
//  Version: 3/7/21
#ifndef __CU_ORDERED_NODE_H__
#define __CU_ORDERED_NODE_H__
#include <cugl/scene2/graph/CUSceneNode.h>

namespace cugl {
    namespace scene2 {
/**
 * This is a scene graph node to arbitrary render orders
 *
 * One of the drawbacks of a scene graph is that it must always render with
 * a pre-order traversal. This is the natural traversal for UI elements, but
 * it is not convenient for character animation (where child components may
 * need to be layered differently).
 *
 * This node is introduced to solve this problem. For the most part, this
 * node operates just like {@link SceneNode}. However, it allows you to
 * resort the render order of the descendents of this node. Simply choose
 * any one of the {@link OrderedNode#Order} values available. These orders
 * are applied to the {@link SceneNode#getPriority} value of each node in
 * the scene graph.
 *
 * Any order other than a pre-order traversal comes as a cost, as we must
 * cache the scene graph transform and color context of each node (these
 * values are computed naturally from the recursive calls of a pre-order
 * traversal). In addition, we must call std::sort (currently IntroSort)
 * on all of the descendants every single render pass. However, as long
 * as the number of children of this node is reasonably sized, this should
 * not be an issue.
 *
 * An OrderedNode is a render barrier. This means that if one OrderedNode
 * (the first node) is a descendant of another OrderedNode (the second node),
 * the first node will be rendered as a unit with the priority of that
 * node. So it is impossible to interleave other descendants of the second
 * node with descendants of the first node.  This is necessary as the
 * two OrderedNodes may have incompatible orderings.
 */
class OrderedNode : public SceneNode {
public:
    /**
     * This enum represents the possible render orders.
     *
     * The default render order is {@link #PRE_ORDER}. When this is set, this
     * node will act like a normal {@link SceneNode}. Other orders will create
     * a list of render contexts that will be sorted before rendering. This
     * will incur additional overhead, particularly if the number of descendant
     * nodes is large.
     */
    enum Order {
        /**
         * Render the nodes with a pre-order traversal (DEFAULT)
         *
         * In a pre-order traversal, the parent is rendered first and then the
         * children.  Children are rendered in the order that they are stored
         * in the node.
         */
        PRE_ORDER,

        /**
         * Render the nodes with a post-order traversal
         *
         * In a post-order traversal, the children are rendered first and then the
         * parent.  Children are rendered in the order that they are stored
         * in the node.
         */
        POST_ORDER,

        /**
         * Render the nodes in ascending order by priority
         *
         * Children with lower priorities will appear at the back of the scene.
         *
         * The algorithm for std::sort is an unstable sorting algorithm and does
         * not handle ties well. Hence all ties are broken by the pre-order
         * traveral value.
         */
        ASCEND,

        /**
         * Render the nodes in descending order by priority
         *
         * Children with higher priorities will appear at the back of the scene.
         *
         * The algorithm for std::sort is an unstable sorting algorithm and does
         * not handle ties well. Hence all ties are broken by the pre-order
         * traveral value.
         */
        DESCEND,

        /**
         * Render the nodes in a pre-order traversal, sorted on ascending priority
         *
         * This order is still a pre-order traversal where the parent is rendered
         * first and then the children. However, children are sorted with respect
         * to their priority.  Children with the lowest priority are drawn first.
         */
        PRE_ASCEND,

        /**
         * Render the nodes in a pre-order traversal, sorted on descending priority
         *
         * This order is still a pre-order traversal where the parent is rendered
         * first and then the children. However, children are sorted with respect
         * to their priority.  Children with the highest priority are drawn first.
         */
        PRE_DESCEND,

        /**
         * Render the nodes in a post-order traversal, sorted on ascending priority
         *
         * This order is still a pre-order traversal where the children are rendered
         * first and then the parent. However, children are sorted with respect
         * to their priority.  Children with the lowest priority are drawn first.
         */
        POST_ASCEND,

        /**
         * Render the nodes in a post-order traversal, sorted on descending priority
         *
         * This order is still a pre-order traversal where the children are rendered
         * first and then the parent. However, children are sorted with respect
         * to their priority.  Children with the highest priority are drawn first.
         */
        POST_DESCEND
    };
    
protected:
    /**
     * A class storing the drawing context for the render queue.
     *
     * The challenge with reordering a scene graph is that you have state on the
     * stack that must be managed: the drawing transform, the tint color, and
     * the scissor value. Normally these are managed by the call stack during
     * a recursive call. To reorder rendering, we have to make this explicit.
     *
     * This class is essentially a struct with a sort order.
     */
    class Context {
    public:
        /** The parent of this inner class (as C++ does not have this Java feature) */
        OrderedNode* parent;
        /** The node to be drawn at this step */
        std::shared_ptr<SceneNode> node;
        /** The scissor value (possibly nullptr) */
        std::shared_ptr<Scissor> scissor;
        /** The drawing transform */
        Affine2 transform;
        /** The tint color */
        Color4 tint;
        /** The canonical order (for pre-order and post-order traversals) */
        Uint32 canonical;
        
        /**
         * Creates a drawing context with the given parent object
         *
         * @param parent    The parent object for the inner class
         */
        Context(OrderedNode* parent);
        
        /**
         * Creates a copy of the given drawing context
         *
         * @param copy      The drawing context to copy
         */
        Context(const Context& copy);

        /**
         * Deletes this drawing context, disposing of all resources.
         */
        ~Context();

        /**
         * Returns the value *a < *b
         *
         * This function implements a sort order on drawing contexts and
         * is used to sort the render queue.
         *
         * @param a        The first (pointer) to compare
         * @param b        The first (pointer) to compare
         *
         * @return the value *a < *b
         */
        static bool sortCompare(Context* a, Context* b);
    };

    /** The render queue (always use a deque for this functionality) */
    std::deque<Context*> _entries;
    /** The global scissor context (necessary as sprite batches manage this normally) */
    std::shared_ptr<Scissor> _viewport;
    /** The current render order */
    Order _order;
    
    /**
     * Adds the given node ot the render queue.
     *
     * This method replaces {@link #render} to provide a delayed render command
     * (via a queue of {@link Context} objects). This method is recursive.
     * However, it will stop when it encounters any other {@link OrderedNode}
     * objects.
     *
     * @param node      The descendant node to render.
     * @param transform The global transformation matrix.
     * @param tint      The tint to blend with the node color.
     */
    void visit(const std::shared_ptr<SceneNode>& node, const Affine2& transform, Color4 tint);
    
#pragma mark -
#pragma mark Constructors
public:
    /**
     * Creates an uninitialized ordered node.
     *
     * You must initialize this OrderedNode before use.
     *
     * NEVER USE A CONSTRUCTOR WITH NEW. If you want to allocate a OrderedNode
     * on the heap, use one of the static constructors instead.
     */
    OrderedNode();
    
    /**
     * Deletes this node, disposing all resources
     */
    ~OrderedNode() { dispose(); }
    
    /**
     * Disposes all of the resources used by this node.
     *
     * A disposed OrderedNode can be safely reinitialized. Any children owned by
     * this node will be released. They will be deleted if no other object owns them.
     *
     * It is unsafe to call this on a OrderedNode that is still currently inside of
     * a scene graph.
     */
    virtual void dispose() override;
    
    /**
     * Initializes an ordered node at the world origin with the given order.
     *
     * The node has both position and size (0,0).
     *
     * @param order The render order
     *
     * @return true if initialization was successful.
     */
    bool initWithOrder(Order order);

    /**
     * Initializes an ordered node with the given position and order.
     *
     * The node has size (0,0). As a result, the position is identified with
     * the origin of the node space.
     *
     * @param order     The render order
     * @param pos       The origin of the node in parent space
     *
     * @return true if initialization was successful.
     */
    bool initWithOrder(Order order, Vec2 pos);

    /**
     * Initializes an ordered node with the given bounds.
     *
     * The rectangle origin is the bottom left corner of the node in parent
     * space, and corresponds to the origin of the Node space. The size defines
     * its content width and height. The node is anchored in the center and has
     * position origin-(width/2,height/2) in parent space.
     *
     * Because the bounding box is explicit, this is the preferred initializer
     * for Nodes that will explicitly contain other Nodes.
     *
     * @param order     The render order
     * @param bounds    The bounds of the node in parent space
     *
     * @return true if initialization was successful.
     */
    bool initWithOrder(Order order, Rect bounds);

    /**
     * Initializes a node with the given JSON specificaton.
     *
     * This initializer is designed to receive the "data" object from the
     * JSON passed to {@link Scene2Loader}. This JSON format supports all
     * of the attribute values of its parent class. In addition, it supports
     * the following additional attributes:
     *
     *      "order":    The sort order of this node.
     *
     * Sort orders are specified as lower case strings representing the names
     * of the enum with dashes in place of underscores (e.g. "pre-order",
     * "post-ascend"). All attributes are optional. There are no required
     * attributes.
     *
     * @param loader    The scene loader passing this JSON file
     * @param data      The JSON object specifying the node
     *
     * @return true if initialization was successful.
     */
    virtual bool initWithData(const Scene2Loader* loader, const std::shared_ptr<JsonValue>& data) override;

#pragma mark -
#pragma mark Static Constructors
    /**
     * Returns a newly allocated ordered node at the world origin.
     *
     * The node has both position and size (0,0).
     *
     * The node will use a pre-order traversal, unless the order
     * is changed with {@link #setOrder}.
     *
     * @return a newly allocated ordered node at the world origin.
     */
    static std::shared_ptr<SceneNode> alloc() {
        std::shared_ptr<SceneNode> result = std::make_shared<SceneNode>();
        return (result->init() ? result : nullptr);
    }
    
    /**
     * Returns a newly allocated ordered node at the given position.
     *
     * The node has size (0,0).  As a result, the position is identified with
     * the origin of the node space.
     *
     * The node will use a pre-order traversal, unless the order
     * is changed with {@link #setOrder}.
     *
     * @param pos   The origin of the node in parent space
     *
     * @return a newly allocated ordered node at the given position.
     */
    static std::shared_ptr<SceneNode> allocWithPosition(const Vec2 pos) {
        std::shared_ptr<SceneNode> result = std::make_shared<SceneNode>();
        return (result->initWithPosition(pos) ? result : nullptr);
    }
    
    /**
     * Returns a newly allocated ordered node at the given position.
     *
     * The node has size (0,0). As a result, the position is identified with
     * the origin of the node space.
     *
     * The node will use a pre-order traversal, unless the order
     * is changed with {@link #setOrder}.
     *
     * @param x     The x-coordinate of the node in parent space
     * @param y     The y-coordinate of the node in parent space
     *
     * @return a newly allocated ordered node at the given position.
     */
    static std::shared_ptr<SceneNode> allocWithPosition(float x, float y) {
        std::shared_ptr<SceneNode> result = std::make_shared<SceneNode>();
        return (result->initWithPosition(x,y) ? result : nullptr);
    }

    /**
     * Returns a newly allocated ordered node with the given size.
     *
     * The size defines the content size. The bounding box of the node is
     * (0,0,width,height) and is anchored in the bottom left corner (0,0).
     * The node is positioned at the origin in parent space.
     *
     * The node will use a pre-order traversal, unless the order
     * is changed with {@link #setOrder}.
     *
     * @param size  The size of the node in parent space
     *
     * @return a newly allocated ordered node with the given size.
     */
    static std::shared_ptr<SceneNode> allocWithBounds(const Size size) {
        std::shared_ptr<SceneNode> result = std::make_shared<SceneNode>();
        return (result->initWithBounds(size) ? result : nullptr);
    }
    
    /**
     * Returns a newly allocated node with the given size.
     *
     * The size defines the content size. The bounding box of the node is
     * (0,0,width,height) and is anchored in the bottom left corner (0,0).
     * The node is positioned at the origin in parent space.
     *
     * The node will use a pre-order traversal, unless the order
     * is changed with {@link #setOrder}.
     *
     * @param width     The width of the node in parent space
     * @param height    The height of the node in parent space
     *
     * @return a newly allocated ordered node with the given size.
     */
    static std::shared_ptr<SceneNode> allocWithBounds(float width, float height) {
        std::shared_ptr<SceneNode> result = std::make_shared<SceneNode>();
        return (result->initWithBounds(width,height) ? result : nullptr);
    }
    
    /**
     * Returns a newly allocated ordered node with the given bounds.
     *
     * The rectangle origin is the bottom left corner of the node in parent
     * space, and corresponds to the origin of the Node space. The size
     * defines its content width and height in node space. The node anchor
     * is placed in the bottom left corner.
     *
     * Because the bounding box is explicit, this is the preferred constructor
     * for nodes that will explicitly contain other nodes.
     *
     * The node will use a pre-order traversal, unless the order
     * is changed with {@link #setOrder}.
     *
     * @param rect  The bounds of the node in parent space
     *
     * @return a newly allocated ordered node with the given bounds.
     */
    static std::shared_ptr<SceneNode> allocWithBounds(const Rect rect) {
        std::shared_ptr<SceneNode> result = std::make_shared<SceneNode>();
        return (result->initWithBounds(rect) ? result : nullptr);
    }
    
    /**
     * Returns a newly allocated ordered node with the given bounds.
     *
     * The rectangle origin is the bottom left corner of the node in parent
     * space, and corresponds to the origin of the Node space. The size
     * defines its content width and height in node space. The node anchor
     * is placed in the bottom left corner.
     *
     * Because the bounding box is explicit, this is the preferred constructor
     * for Nodes that will explicitly contain other Nodes.
     *
     * The node will use a pre-order traversal, unless the order
     * is changed with {@link #setOrder}.
     *
     * @param x         The x-coordinate of the node origin in parent space
     * @param y         The y-coordinate of the node origin in parent space
     * @param width     The width of the node in parent space
     * @param height    The height of the node in parent space
     *
     * @return a newly allocated node with the given bounds.
     */
    static std::shared_ptr<SceneNode> allocWithBounds(float x, float y, float width, float height) {
        std::shared_ptr<SceneNode> result = std::make_shared<SceneNode>();
        return (result->initWithBounds(x,y,width,height) ? result : nullptr);
    }
    
    /**
     * Returns a newly allocated ordered node at the world origin with the given order.
     *
     * The node has both position and size (0,0).
     *
     * @param order The render order
     *
     * @return a newly allocated ordered node at the world origin with the given order.
     */
    static std::shared_ptr<OrderedNode> allocWithOrder(Order order) {
        std::shared_ptr<OrderedNode> result = std::make_shared<OrderedNode>();
        return (result->initWithOrder(order) ? result : nullptr);
    }

    /**
     * Returns a newly allocated ordered node with the given position and order.
     *
     * The node has size (0,0). As a result, the position is identified with
     * the origin of the node space.
     *
     * @param order     The render order
     * @param pos       The origin of the node in parent space
     *
     * @return a newly allocated ordered node with the given position and order.
     */
    static std::shared_ptr<OrderedNode> allocWithOrder(Order order, Vec2 pos) {
        std::shared_ptr<OrderedNode> result = std::make_shared<OrderedNode>();
        return (result->initWithOrder(order,pos) ? result : nullptr);
    }

    /**
     * Returns a newly allocated ordered node with the given bounds.
     *
     * The rectangle origin is the bottom left corner of the node in parent
     * space, and corresponds to the origin of the Node space. The size defines
     * its content width and height. The node is anchored in the center and has
     * position origin-(width/2,height/2) in parent space.
     *
     * Because the bounding box is explicit, this is the preferred initializer
     * for Nodes that will explicitly contain other Nodes.
     *
     * @param order     The render order
     * @param bounds    The bounds of the node in parent space
     *
     * @return a newly allocated ordered node with the given bounds.
     */
    static std::shared_ptr<OrderedNode> allocWithOrder(Order order, Rect bounds) {
        std::shared_ptr<OrderedNode> result = std::make_shared<OrderedNode>();
        return (result->initWithOrder(order,bounds) ? result : nullptr);
    }

    /**
     * Returns a newly allocated ordered node with the given JSON specificaton.
     *
     * This initializer is designed to receive the "data" object from the
     * JSON passed to {@link Scene2Loader}. This JSON format supports all
     * of the attribute values of its parent class. In addition, it supports
     * the following additional attributes:
     *
     *      "order":    The sort order of this node.
     *
     * Sort orders are specified as lower case strings representing the names
     * of the enum with dashes in place of underscores (e.g. "pre-order",
     * "post-ascend"). All attributes are optional. There are no required
     * attributes.
     *
     * @param loader    The scene loader passing this JSON file
     * @param data      The JSON object specifying the node
     *
     * @return a newly allocated ordered node with the given JSON specificaton.
     */
    static std::shared_ptr<OrderedNode> allocWithData(const Scene2Loader* loader, const std::shared_ptr<JsonValue>& data) {
        std::shared_ptr<OrderedNode> result = std::make_shared<OrderedNode>();
        return (result->initWithData(loader,data) ? result : nullptr);
    }

#pragma mark -
#pragma mark Attributes
    /**
     * Returns the render order of this node
     *
     * This render order will be applied to all descendants of this node. However,
     * other instances of {@link OrderedNode} constitute a render boundary. While
     * the ordered nodes themselves will be resorted, their children will not.
     *
     * The default value of {@link Order#PRE_ORDER} will default to the normal
     * render algorithm and is therefore the most efficient.
     *
     * @return the render order of this node
     */
    Order getOrder() const { return _order; }
    
    /**
     * Sets the render order of this node
     *
     * This render order will be applied to all descendants of this node. However,
     * other instances of {@link OrderedNode} constitute a render boundary. While
     * the ordered nodes themselves will be resorted, their children will not.
     *
     * The default value of {@link Order#PRE_ORDER} will default to the normal
     * render algorithm and is therefore the most efficient.
     *
     * @param order The render order of this node
     */
    void setOrder(Order order) { _order = order; }

    /**
     * Draws this node and all of its children with the given SpriteBatch.
     *
     * By default, this will revert to the render method of {@link SceneNode}.
     * However if the order is anything other than {@link Order#PRE_ORDER},
     * it will construct a render queue of all children.  This render queue
     * will bypass all calls to {@link SceneNode#render} and instead call
     * {@link SceneNode#draw}. This is why it is important for all custom
     * subclasses of SceneNode to override draw instead of render.
     *
     * @param batch     The SpriteBatch to draw with.
     * @param transform The global transformation matrix.
     * @param tint      The tint to blend with the node color.
     */
    void render(const std::shared_ptr<SpriteBatch>& batch, const Affine2& transform, Color4 tint) override;
    
    /**
     * Draws this node and all of its children with the given SpriteBatch.
     *
     * By default, this will revert to the render method of {@link SceneNode}.
     * However if the order is anything other than {@link Order#PRE_ORDER},
     * it will construct a render queue of all children.  This render queue
     * will bypass all calls to {@link SceneNode#render} and instead call
     * {@link SceneNode#draw}. This is why it is important for all custom
     * subclasses of SceneNode to override draw instead of render.
     *
     * @param batch     The SpriteBatch to draw with.
     */
    virtual void render(const std::shared_ptr<SpriteBatch>& batch) override {
        render(batch,Affine2::IDENTITY,Color4::WHITE);
    }

    /** This macro disables the copy constructor (not allowed on scene graphs) */
    CU_DISALLOW_COPY_AND_ASSIGN(OrderedNode);
};
    }

}
#endif /* __CU_ORDERED_NODE_H__ */
