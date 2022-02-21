//
//  CUOrderedNode.cpp
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
#include <cugl/scene2/graph/CUOrderedNode.h>
#include <cugl/render/CUScissor.h>

using namespace cugl;
using namespace cugl::scene2;

#pragma mark Context
/**
 * Creates a drawing context with the given parent object
 *
 * @param parent    The parent object for the inner class
 */
OrderedNode::Context::Context(OrderedNode* parent) :
node(nullptr),
scissor(nullptr),
canonical(0) {
    this->parent = parent;
    tint = Color4::WHITE;
}
       

/**
 * Creates a copy of the given drawing context
 *
 * @param copy      The drawing context to copy
 */
OrderedNode::Context::Context(const Context& copy) {
    node = copy.node;
    scissor = copy.scissor;
    canonical = copy.canonical;
    transform = copy.transform;
    tint = copy.tint;
}

/**
 * Deletes this drawing context, disposing of all resources.
 */
OrderedNode::Context::~Context() {
    node = nullptr;
    scissor = nullptr;
}

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
bool OrderedNode::Context::sortCompare(Context* a, Context* b) {
    // NOTE: Pre or post is determined by canonical order
    switch (a->parent->_order) {
        case PRE_ORDER:
        case POST_ORDER:
            return a->canonical < b->canonical;
        case ASCEND:
            if (a->node->getPriority() == b->node->getPriority()) {
                return a->canonical < b->canonical;
            }
            return a->node->getPriority() < b->node->getPriority();
        case PRE_ASCEND:
        case POST_ASCEND:
            if (a->node->getParent() != b->node->getParent()) {
                return a->canonical < b->canonical;
            } else if (a->node->getPriority() == b->node->getPriority()) {
                return a->canonical < b->canonical;
            }
            return a->node->getPriority() < b->node->getPriority();
        case DESCEND:
            if (a->node->getPriority() == b->node->getPriority()) {
                return a->canonical < b->canonical;
            }
            return a->node->getPriority() > b->node->getPriority();
        case PRE_DESCEND:
        case POST_DESCEND:
            if (a->node->getParent() != b->node->getParent()) {
                return a->canonical < b->canonical;
            } else if (a->node->getPriority() == b->node->getPriority()) {
                return a->canonical < b->canonical;
            }
            return a->node->getPriority() > b->node->getPriority();
    }
    return false;
}

#pragma mark -
#pragma mark Ordered Node
/**
 * Creates an uninitialized ordered node.
 *
 * You must initialize this OrderedNode before use.
 *
 * NEVER USE A CONSTRUCTOR WITH NEW. If you want to allocate a OrderedNode
 * on the heap, use one of the static constructors instead.
 */
OrderedNode::OrderedNode() :
_viewport(nullptr),
_order(PRE_ORDER) {
    _classname = "OrderedNode";
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
void OrderedNode::dispose() {
    for(auto it = _entries.begin(); it != _entries.end(); ++it) {
        delete *it;
        *it = nullptr;
    }
    _entries.clear();
    _viewport = nullptr;
    SceneNode::dispose();
}

/**
 * Initializes an ordered node at the world origin with the given order.
 *
 * The node has both position and size (0,0).
 *
 * @param order The render order
 *
 * @return true if initialization was successful.
 */
bool OrderedNode::initWithOrder(Order order) {
    if (init()) {
        _order = order;
        return true;
    }
    return false;
}

/**
 * Initializes an ordered node with the given position and order.
 *
 * The node has size (0,0). As a result, the position is identified with
 * the origin of the node space.
 *
 * @param order The render order
 * @param pos   The origin of the node in parent space
 *
 * @return true if initialization was successful.
 */
bool OrderedNode::initWithOrder(Order order, Vec2 pos) {
    if (initWithPosition(pos)) {
        _order = order;
        return true;
    }
    return false;
}

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
 * @param order The render order
 * @param rect  The bounds of the node in parent space
 *
 * @return true if initialization was successful.
 */
bool OrderedNode::initWithOrder(Order order, Rect bounds) {
    if (initWithBounds(bounds)) {
        _order = order;
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
bool OrderedNode::initWithData(const Scene2Loader* loader, const std::shared_ptr<JsonValue>& data) {
    if (SceneNode::initWithData(loader, data)) {
        _order = Order::PRE_ORDER;
        if (data->has("order")) {
            std::string value = data->getString("order", "pre-order");
            if (value == "pre-order") {
                _order = Order::PRE_ORDER;
            } else if (value == "post-order") {
                _order = Order::POST_ORDER;
            } else if (value == "ascend") {
                _order = Order::ASCEND;
            } else if (value == "descend") {
                _order = Order::DESCEND;
            } else if (value == "pre-ascend") {
                _order = Order::PRE_ASCEND;
            } else if (value == "pre-descend") {
                _order = Order::PRE_DESCEND;
            } else if (value == "post-ascend") {
                _order = Order::POST_ASCEND;
            } else if (value == "post-descend") {
                _order = Order::POST_DESCEND;
            }
        }
        return true;
    }
    return false;
}

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
void OrderedNode::visit(const std::shared_ptr<SceneNode>& node, const Affine2& transform, Color4 tint) {
    if (!node->isVisible()) { return; }

    Affine2 matrix;
    Affine2::multiply(node->getTransform(),transform,&matrix);
    Color4 color = node->getColor();
    if (node->hasRelativeColor()) {
        color *= tint;
    }
    
    // We need to capture the important sprite batch state
    std::shared_ptr<Scissor> previous = _viewport;
    std::shared_ptr<Scissor> current = nullptr;
    if (node->getScissor()) {
        current = Scissor::alloc(node->getScissor());
        current->setTransform(matrix);
        if (previous) {
            current->intersect(previous);
        }
        _viewport = current;
    }
    
    // Identify pre or post. Block at child ordered nodes
    bool ispost = (_order == POST_ORDER || _order == POST_ASCEND || _order == POST_DESCEND);
    bool barrier = node->getClassName() == getClassName();
    if (ispost && !barrier) {
        auto children = node->getChildren();
        for(auto it = children.begin(); it != children.end(); ++it) {
            visit(*it, matrix, color);
        }
    }
    
    // Capture pre or post order traversal
    Uint32 canonical = (_entries.empty() ? 0 : _entries.back()->canonical+1);
    
    Context* context = new Context(this);
    _entries.push_back(context);
    context->node = node;
    context->transform = barrier ? transform : matrix;
    context->scissor = _viewport;
    context->tint = barrier ? tint : color;
    context->canonical = canonical;
    
    if (!ispost && !barrier) {
        auto children = node->getChildren();
        for(auto it = children.begin(); it != children.end(); ++it) {
            visit(*it, matrix, color);
        }
    }

    _viewport = previous;
}

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
void OrderedNode::render(const std::shared_ptr<SpriteBatch>& batch, const Affine2& transform, Color4 tint) {
    if (!_isVisible) { return; }
    
    if (_order == PRE_ORDER) {
        // Drop to standard for efficiency
        SceneNode::render(batch,transform,tint);
    } else {
        Affine2 matrix;
        Affine2::multiply(_combined,transform,&matrix);
        Color4 color = _tintColor;
        if (_hasParentColor) {
            color *= tint;
        }
        
        // Capture sprite batch context
        std::shared_ptr<Scissor> active = batch->getScissor();
        _viewport = active;
        if (_scissor) {
            std::shared_ptr<Scissor> local = Scissor::alloc(_scissor);
            local->setTransform(matrix);
            if (active) {
                local->intersect(active);
            }
            _viewport = local;
        }

        // Build and sort
        for(auto it = _children.begin(); it != _children.end(); ++it) {
            visit(*it, matrix, color);
        }

        std::sort(_entries.begin(), _entries.end(), Context::sortCompare);
        for(auto it = _entries.begin(); it != _entries.end(); ++it) {
            Context* context = *it;
            batch->setScissor(context->scissor); // This is in render, so must be applied
            if (context->node->getClassName() == getClassName()) {
                // Render barrier at an ordered node
                context->node->render(batch, context->transform, context->tint);
            } else {
                context->node->draw(batch, context->transform, context->tint);
            }
        }

        // Clean up and restore state
        for(auto it = _entries.begin(); it != _entries.end(); ++it) {
            delete *it;
            *it = nullptr;
        }
        _entries.clear();
        _viewport = nullptr;
        batch->setScissor(active);
    }
}
