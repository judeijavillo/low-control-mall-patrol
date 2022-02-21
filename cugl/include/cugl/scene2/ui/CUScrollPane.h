//
//  CUScrollPane.h
//  Cornell University Game Library (CUGL)
//
//  This module provides support for scroll pane that allows the user to
//  navigate a limited view of a larger backing view.  Classic examples
//  are a window that cannot show everything on the screen at once.
//
//  In many APIs, a scroll pane comes with sliders that allow you to
//  control the window position. However, while this is the natural
//  way to navigate a scroll pane in a desktop OS, it is not the natural
//  way to navigate it in a mobile device. In the latter case, gestures
//  are the preferred means of navigation.  As a result, this class is
//  not coupled with its navigation interface, and simply provides methods
//  for adjusting the position and orientation of the backing contents.
//
//  If you would like to have a more traditional navigation interface,
//  such as scroll bars, simply add slider object to the scene graph.
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
//  Version: 11/18/21
//
#ifndef __CU_SCROLL_PANE_H__
#define __CU_SCROLL_PANE_H__
#include <cugl/scene2/graph/CUSceneNode.h>

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
 * This class is a node implements a scroll pane.
 *
 * A scroll pane is a node that contains a larger backing view. The scroll
 * pane uses an implicit scissor to guarantee that the user only sees what
 * is in the content bounds of this node (e.g the rectangle that starts at
 * (0,0) and has {@link #getContentSize()}). This allows you to create
 * internal windows that only show a portion of the backing contents.
 *
 * The contents of a scroll pane are its children. However, the scroll pane
 * also has the concept of {@link #getInterior}, representing the dimension
 * and location of the backing window. These bounds should be large enough
 * to contain all of the children, but this is not enforced. The significance
 * of the backing bounds is that the scroll pane will never go outside of
 * these bounds unless {@link #isConstrained} is true. Panning and zooming
 * will stop once it hits one of the boundary edges. This allows you to
 * prevent the user from going "out of bounds" when navigating the scroll pane.
 *
 * Scroll panes support layout managers just like any other scene graph node.
 * However, layout is performed with respect to the interior bounds and not
 * the content bounds.
 *
 * Scroll panes typically have scroll bars that allow you to navigate their
 * contents. While this makes sense on a desktop computer, it does not make
 * sense on a mobile device. On mobile devices, scroll panes are navigated
 * with gestures, such as panning, pinching, or rotating.
 *
 * As a result, this class does not contain any visual features for navigating
 * a scroll pane. It only has methods for controlling the position and
 * orientation of the the backing window. If you need visual interfaces
 * like a scroll bar, you can attach them separately to the scene graph.
 * Scroll bars are just instances of {@link Slider}.
 *
 * This scroll pane is generalized enough that it is not limited to panning.
 * It supports all of the core mobile navigation gestures: panning, zooming,
 * and spinning. These are controlled by the methods {@link #applyPan},
 * {@link #applyZoom}, and {@link #applySpin}. At first glance, these might
 * appear to be redundant with the transform methods {@link #setPosition},
 * {@link #setScale}, and {@link #setAngle}. But they are not.
 *
 * First of all, the transform methods are applied to this node while the
 * navigation methods are applied to the contents (i.e. the children). More
 * importantly, the navigation methods are applied to all of the children
 * uniformly. For example, when we call {@link #setScale} or {@link #setAngle}
 * on any child, it is with respect to the anchor of that child. However,
 * the navigation methods use the anchor of **this** node, which is often
 * reassigned by the gesture.
 *
 * These distinctions mean that it is quite tricky to implement navigation
 * features by manipulating the children directly. Indeed, this class was
 * created in reaction to the difficulties that developers had when they
 * added zoom features to their scrolling windows. The classic example of
 * this is a large game map that the user navigates by panning, zooming in,
 * and zooming out.
 *
 * The trickiest feature of the scroll pane is rotation (spinning). Because
 * the backing bounds are a rectangle, and pane does not show anything
 * outside of these bounds, the edges can catch on the visible bounds and
 * block the rotation. If this is a problem you should either ignore spin
 * input in your application or set {@link #setConstrained} to false. However,
 * the latter will mean that the user can navigate outside of the backing area.
 */
class ScrollPane : public SceneNode {
#pragma mark Values
protected:
    /** The interior rectangle representing the internal content bounds */
    Rect _interior;
    
    /** The transform to apply to the interior rectangle */
    Affine2 _panetrans;

    /** Whether the node is constrained, forcing the interior within bounds*/
    bool _constrained;
    /** Whether any transforms have been applied to the interior */
    bool _reoriented;
    /** Whether the only transform applied to the interior is panning */
    bool _simple;
    
    /** The minimum supported zoom amount */
    float _zoommin;
    /** The maximum supported zoom amount */
    float _zoommax;
    /** The current cumulative zoom value */
    float _zoomamt;

    /** The masking scissor for this scroll pane */
    std::shared_ptr<Scissor> _panemask;

#pragma mark -
#pragma mark Constructors
public:
    /**
     * Creates an uninitialized node.
     *
     * You must initialize this Node before use.
     *
     * NEVER USE A CONSTRUCTOR WITH NEW. If you want to allocate a Node on the
     * heap, use one of the static constructors instead.
     */
    ScrollPane();
    
    /**
     * Deletes this node, disposing all resources
     */
    ~ScrollPane() { dispose(); }
    
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
     * Initializes a node with the given size.
     *
     * The size defines the content size. The bounding box of the node is
     * (0,0,width,height) and is anchored in the bottom left corner (0,0).
     * The node is positioned at the origin in parent space.
     *
     * The interior bounds will be equivalent to the content bounds. That
     * means that no scrolling can happen until {@link #setInterior} is
     * called. In addition, masking will be turned off and the interior
 * is unconstrained.
     *
     * @param size  The size of the node in parent space
     *
     * @return true if initialization was successful.
     */
    virtual bool initWithBounds(const Size size) override;

    /**
     * Initializes a node with the given size.
     *
     * The size defines the content size. The bounding box of the node is
     * (0,0,width,height) and is anchored in the bottom left corner (0,0).
     * The node is positioned at the origin in parent space.
     *
     * The interior bounds will be equivalent to the content bounds. That
     * means that no scrolling can happen until {@link #setInterior} is
     * called. In addition, masking will be turned off and the interior
     * is unconstrained.
     *
     * @param width     The width of the node in parent space
     * @param height    The height of the node in parent space
     *
     * @return true if initialization was successful.
     */
    virtual bool initWithBounds(float width, float height) override {
        return initWithBounds(Size(width,height));
    }

    /**
     * Initializes a node with the given bounds.
     *
     * The rectangle origin is the bottom left corner of the node in parent
     * space, and corresponds to the origin of the Node space. The size
     * defines its content width and height in node space. The node anchor
     * is placed in the bottom left corner.
     *
     * The interior bounds will be equivalent to the content bounds. That
     * means that no scrolling can happen until {@link #setInterior} is
     * called. In addition, masking will be turned off and the interior
     * is unconstrained.
     *
     * @param rect  The bounds of the node in parent space
     *
     * @return true if initialization was successful.
     */
    virtual bool initWithBounds(const Rect rect) override;
    
    /**
     * Initializes a node with the given bounds.
     *
     * The rectangle origin is the bottom left corner of the node in parent
     * space, and corresponds to the origin of the Node space. The size
     * defines its content width and height in node space. The node anchor
     * is placed in the bottom left corner.
     *
     * The interior bounds will be equivalent to the content bounds. That
     * means that no scrolling can happen until {@link #setInterior} is
     * called. In addition, masking will be turned off and the interior
     * is unconstrained.
     *
     * @param x         The x-coordinate of the node origin in parent space
     * @param y         The y-coordinate of the node origin in parent space
     * @param width     The width of the node in parent space
     * @param height    The height of the node in parent space
     *
     * @return true if initialization was successful.
     */
    virtual bool initWithBounds(float x, float y, float width, float height) override {
        return initWithBounds(Rect(x,y,width,height));
    }
    
    /**
     * Initializes a node with the given size and interior
     *
     * The size defines the content size. The bounding box of the node is
     * (0,0,width,height) and is anchored in the bottom left corner (0,0).
     * The node is positioned at the origin in parent space.
     *
     * The interior bounds will be as given. This interior will be placed
     * in the default orientation as defined by {@link #resetPane}. The
     * interior will start off as constrained unless this is impossible
     * (e.g. the interior is smaller than the content bounds).
     *
     * The optional masking value can be set to false to allow the area
     * outside of the content bounds to be visible.
     *
     * @param size      The size of the node in parent space
     * @param interior  The bounds of the interior in node space
     * @param mask      Whether to mask contents outside of node bounds
     *
     * @return true if initialization was successful.
     */
    bool initWithInterior(const Size size, const Rect interior, bool mask=true);

    /**
     * Initializes a node with the given bounds.
     *
     * The rectangle origin is the bottom left corner of the node in parent
     * space, and corresponds to the origin of the Node space. The size
     * defines its content width and height in node space. The node anchor
     * is placed in the bottom left corner.
     *
     * The interior bounds will be as given. This interior will be placed
     * in the default orientation as defined by {@link #resetPane}. The
     * interior will start off as constrained unless this is impossible
     * (e.g. the interior is smaller than the content bounds).
     *
     * The optional masking value can be set to false to allow the area
     * outside of the content bounds to be visible.
     *
     * @param bounds    The bounds of the node in parent space
     * @param interior  The bounds of the interior in node space
     * @param mask      Whether to mask contents outside of node bounds
     *
     * @return true if initialization was successful.
     */
    bool initWithInterior(const Rect bounds, const Rect interior, bool mask=true);

    /**
     * Initializes a node with the given JSON specificaton.
     *
     * This initializer is designed to receive the "data" object from the
     * JSON passed to {@link Scene2Loader}. This JSON format supports all
     * of the attribute values of its parent class. In addition, it supports
     * the following additional attributes:
     *
     *      "interior":  A two or four-element number array for the interior bounds.
     *                   A two-element array indicates the bounds start at the origin.
     *      "constrain": A boolean value, indicating whether to keep the interior in bounds.
     *      "mask":      A boolean value, indicating whether to hide out-of-bounds contents.
     *      "pan":       A two-element number array, representing the initial pan offset.
     *      "spin"       A float represeting the initial spin angle in radians.
     *      "zoom"       A float representing the initial zoom factor.
     *      "zoom max":  A float representing the maximum supported cumulative zoom.
     *      "zoom min":  A float representing the minimum supported cumulative zoom.
     *
     * All attributes are optional. There are no required attributes. If any scroll
     * adjustments are set (pan, spin, zoom) they will be applied in the following
     * order: spin, then zoom, then pan.
     *
     * Note that if the interior size and/or position pushes it outside of the content
     * bounds, then the constrain setting will be ignored and set to false.
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
     * Returns a newly allocated node with the given size.
     *
     * The size defines the content size. The bounding box of the node is
     * (0,0,width,height) and is anchored in the bottom left corner (0,0).
     * The node is positioned at the origin in parent space.
     *
     * The interior bounds will be equivalent to the content bounds. That
     * means that no scrolling can happen until {@link #setInterior} is
     * called. In addition, masking will be turned off.
     *
     * @param size  The size of the node in parent space
     *
     * @return a newly allocated node with the given size.
     */
    static std::shared_ptr<ScrollPane> allocWithBounds(const Size size) {
        std::shared_ptr<ScrollPane> result = std::make_shared<ScrollPane>();
        return (result->initWithBounds(size) ? result : nullptr);
    }
    
    /**
     * Returns a newly allocated node with the given size.
     *
     * The size defines the content size. The bounding box of the node is
     * (0,0,width,height) and is anchored in the bottom left corner (0,0).
     * The node is positioned at the origin in parent space.
     *
     * The interior bounds will be equivalent to the content bounds. That
     * means that no scrolling can happen until {@link #setInterior} is
     * called. In addition, masking will be turned off.
     *
     * @param width     The width of the node in parent space
     * @param height    The height of the node in parent space
     *
     * @return a newly allocated node with the given size.
     */
    static std::shared_ptr<ScrollPane> allocWithBounds(float width, float height) {
        std::shared_ptr<ScrollPane> result = std::make_shared<ScrollPane>();
        return (result->initWithBounds(width,height) ? result : nullptr);
    }
    
    /**
     * Returns a newly allocated node with the given bounds.
     *
     * The rectangle origin is the bottom left corner of the node in parent
     * space, and corresponds to the origin of the Node space. The size
     * defines its content width and height in node space. The node anchor
     * is placed in the bottom left corner.
     *
     * The interior bounds will be equivalent to the content bounds. That
     * means that no scrolling can happen until {@link #setInterior} is
     * called. In addition, masking will be turned off.
     *
     * @param rect  The bounds of the node in parent space
     *
     * @return a newly allocated node with the given bounds.
     */
    static std::shared_ptr<ScrollPane> allocWithBounds(const Rect rect) {
        std::shared_ptr<ScrollPane> result = std::make_shared<ScrollPane>();
        return (result->initWithBounds(rect) ? result : nullptr);
    }

    
    /**
     * Returns a newly allocated node with the given bounds.
     *
     * The rectangle origin is the bottom left corner of the node in parent
     * space, and corresponds to the origin of the Node space. The size
     * defines its content width and height in node space. The node anchor
     * is placed in the bottom left corner.
     *
     * The interior bounds will be equivalent to the content bounds. That
     * means that no scrolling can happen until {@link #setInterior} is
     * called. In addition, masking will be turned off.
     *
     * @param x         The x-coordinate of the node origin in parent space
     * @param y         The y-coordinate of the node origin in parent space
     * @param width     The width of the node in parent space
     * @param height    The height of the node in parent space
     *
     * @return a newly allocated node with the given bounds.
     */
    static std::shared_ptr<ScrollPane> allocWithBounds(float x, float y, float width, float height) {
        std::shared_ptr<ScrollPane> result = std::make_shared<ScrollPane>();
        return (result->initWithBounds(x,y,width,height) ? result : nullptr);
    }
    
    /**
     * Returns a newly allocated node with the given size and interior
     *
     * The size defines the content size. The bounding box of the node is
     * (0,0,width,height) and is anchored in the bottom left corner (0,0).
     * The node is positioned at the origin in parent space.
     *
     * The interior bounds will be as given. This interior will be placed
     * in the default orientation as defined by {@link #resetPane}. The
     * interior will start off as constrained unless this is impossible
     * (e.g. the interior is smaller than the content bounds).
     *
     * The optional masking value can be set to false to allow the area
     * outside of the content bounds to be visible.
     *
     * @param size      The size of the node in parent space
     * @param interior  The bounds of the interior in node space
     * @param mask      Whether to mask contents outside of node bounds
     *
     * @return a newly allocated node with the given size and interior
     */
    static std::shared_ptr<ScrollPane> allocWithInterior(const Size size, const Rect interior, bool mask=true) {
        std::shared_ptr<ScrollPane> result = std::make_shared<ScrollPane>();
        return (result->initWithInterior(size,interior,mask) ? result : nullptr);
    }

    /**
     * Returns a newly allocated node with the given bounds.
     *
     * The rectangle origin is the bottom left corner of the node in parent
     * space, and corresponds to the origin of the Node space. The size
     * defines its content width and height in node space. The node anchor
     * is placed in the bottom left corner.
     *
     * The interior bounds will be as given. This interior will be placed
     * in the default orientation as defined by {@link #resetPane}. The
     * interior will start off as constrained unless this is impossible
     * (e.g. the interior is smaller than the content bounds).
     *
     * The optional masking value can be set to false to allow the area
     * outside of the content bounds to be visible.
     *
     * @param bounds    The bounds of the node in parent space
     * @param interior  The bounds of the interior in node space
     * @param mask      Whether to mask contents outside of node bounds
     *
     * @return a newly allocated node with the given bounds.
     */
    static std::shared_ptr<ScrollPane> allocWithInterior(const Rect bounds, const Rect interior, bool mask=true) {
        std::shared_ptr<ScrollPane> result = std::make_shared<ScrollPane>();
        return (result->initWithInterior(bounds,interior,mask) ? result : nullptr);
    }

    /**
     * Returns a newly node with the given JSON specification.
     *
     * This initializer is designed to receive the "data" object from the
     * JSON passed to {@link Scene2Loader}. This JSON format supports all
     * of the attribute values of its parent class. In addition, it supports
     * the following additional attributes:
     *
     *      "interior":  A two or four-element number array for the interior bounds.
     *                   A two-element array indicates the bounds start at the origin.
     *      "constrain": A boolean value, indicating whether to keep the interior in bounds.
     *      "mask":      A boolean value, indicating whether to hide out-of-bounds contents.
     *      "pan":       A two-element number array, representing the initial pan offset.
     *      "spin"       A float represeting the initial spin angle in radians.
     *      "zoom"       A float representing the initial zoom factor.
     *      "zoom max":  A float representing the maximum supported cumulative zoom.
     *      "zoom min":  A float representing the minimum supported cumulative zoom.
     *
     * All attributes are optional. There are no required attributes. If any scroll
     * adjustments are set (pan, spin, zoom) they will be applied in the following
     * order: spin, then zoom, then pan.
     *
     * Note that if the interior size and/or position pushes it outside of the content
     * bounds, then the constrain setting will be ignored and set to false.
     *
     * @param loader    The scene loader passing this JSON file
     * @param data      The JSON object specifying the node
     *
     * @return a newly node with the given JSON specification.
     */
    static std::shared_ptr<ScrollPane> allocWithData(const Scene2Loader* loader,
                                                     const std::shared_ptr<JsonValue>& data) {
        std::shared_ptr<ScrollPane> result = std::make_shared<ScrollPane>();
        return (result->initWithData(loader,data) ? result : nullptr);
    }
        
#pragma mark -
#pragma mark Attributes
    /**
     * Returns the interior bounds of this scroll pane.
     *
     * A scroll pane is essentially a view port that shows a portion of
     * a content area. This content area is defined by the interior
     * rectangle. All children positions are defined relative to this
     * content area and not the node itself.
     *
     * If the node is constrained, it will never show any area outside of
     * the interior bounds. Any attempts to pan, spin, or zoom the interior
     * in such a way to violate this are prohibited. For example, if a pan
     * pushes the interior so far that an edge of the interior would be
     * showing in the content bounds, then this pan will be aborted.
     *
     * @return the interior bounds of this scroll pane.
     */
    const Rect& getInterior() const { return _interior; }
    
    /**
     * Sets the interior bounds of this scroll pane.
     *
     * A scroll pane is essentially a view port that shows a portion of
     * a content area. This content area is defined by the interior
     * rectangle. All children positions are defined relative to this
     * content area and not the node itself.
     *
     * If the node is constrained, it will never show any area outside of
     * the interior bounds. Any attempts to pan, spin, or zoom the interior
     * in such a way to violate this are prohibited. For example, if a pan
     * pushes the interior so far that an edge of the interior would be
     * showing in the content bounds, then this pan will be aborted.
     *
     * If the interior position would force a constrained node to show
     * any out-of-bounds content (e.g. the interior does not properly
     * contain all four corners of the content bounds), then the pan value
     * will be adjusted so that the bottom left corner is at the origin.
     * If the interior is still too small to cover the content bounds then
     * this method will set constrained to false.
     */
    void setInterior(const Rect& bounds);
    
    /**
     * Returns the untransformed bounds of this node for layout purposes.
     *
     * The layout bounds are used by the layout managers to place children in
     * this node. For example, an anchored layout will put a child with anchors
     * "top" and "right" at the top right corners of these bounds.
     *
     * In this class, the layout bounds correspond to {@link #getInterior}.
     * This allows the layout managers to properly layout the content regardless
     * of the current scroll settings.
     */
    virtual Rect getLayoutBounds() const override { return _interior; }

    /**
     * Returns true if this scroll pane is constrained.
     *
     * A constrained scroll pane will never show any area outside of the
     * interior bounds. Any attempts to pan, spin, or zoom the interior in
     * such a way to violate this are prohibited. For example, if a pan
     * pushes the interior so far that an edge of the interior would be
     * showing in the content bounds, then this pan will be aborted.
     *
     * @return true if this scroll pane is constrained
     */
    bool isConstrained() const { return _constrained; }

    /**
     * Sets the constrained attribute, returning true on success.
     *
     * A constrained scroll pane will never show any area outside of the
     * interior bounds. Any attempts to pan, spin, or zoom the interior in
     * such a way to violate this are prohibited. For example, if a pan
     * pushes the interior so far that an edge of the interior would be
     * showing in the content bounds, then this pan will be aborted.
     *
     * If the scroll pane is currently showing any out-of-bound areas
     * (e.g. the interior does not properly contain all four corners of
     * the content bounds), then the interior will be reset to its
     * untransformed position.  If this still shows any out-of-bound
     * areas (e.g. the interior is too small), then this method will
     * fail.
     *
     * @param value Whether to constrain this scroll pane
     *
     * @return true if the constrained attribute was changed
     */
    bool setConstrained(bool value);
    
    /**
     * Returns true if this scroll pane is masked.
     *
     * A masked scroll pane will never show any content outside of the
     * content bounds. This node will activate an implicit scissor whose
     * bounds are the size of the content bounds to avoid drawing this
     * content.
     *
     * A scroll pane has only one scissor at a given time. So if a scissor
     * is masked, then it will ignore the {@link SceneNode#getScissor}
     * attribute defined in its parent class. The scissor should be unmasked
     * if you want to support a custom scissor.
     *
     * @return true if this scroll pane is masked.
     */
    bool isMasked() const { return _panemask != nullptr; }
    
    /**
     * Sets whether this scroll pane is masked.
     *
     * A masked scroll pane will never show any content outside of the
     * content bounds. This node will activate an implicit scissor whose
     * bounds are the size of the content bounds to avoid drawing this
     * content.
     *
     * A scroll pane has only one scissor at a given time. So if a scissor
     * is masked, then it will ignore the {@link SceneNode#getScissor}
     * attribute defined in its parent class. The scissor should be unmasked
     * if you want to support a custom scissor. Setting this value to true
     * will only ignore the custom scissor; it will not erase it.
     *
     * @return value    Whether this scroll pane is masked.
     */
    void setMasked(bool value);
   
    /**
     * Returns the current cumulative zoom for this node.
     *
     * This value is useful when you want to control the rate of change
     * of zoom relative to what has been applied.
     *
     * @return the current cumulative zoom for this node.
     */
    float getZoom() const {
        return _zoomamt;
    }
    /**
     * Returns the minimum supported cumulative zoom
     *
     * A zoom factor is a scaling factor. Values greater than 1
     * zoom in, enlarging the content. Values less than 1 (but
     * still postive) zoom out, shrinking the content. Negative
     * zoom values are not supported.
     *
     * At all times, the scroll pane keeps track of the cumulative
     * zoom in or out. This cumulative zoom must stay always be
     * greater than the minimum zoom. Attempts to zoom further out
     * than this amount will be denied.
     *
     * @return the minimum supported cumulative zoom
     */
    float getMinZoom() const { return _zoommin; }
    
    /**
     * Attempts to set the minimum supported cumulative zoom
     *
     * A zoom factor is a scaling factor. Values greater than 1
     * zoom in, enlarging the content. Values less than 1 (but
     * still postive) zoom out, shrinking the content. Negative
     * zoom values are not supported.
     *
     * At all times, the scroll pane keeps track of the cumulative
     * zoom in or out. This cumulative zoom must stay always be
     * greater than the minimum zoom. Attempts to zoom further out
     * than this amount will be denied.
     *
     * This setter will fail if either the minimum zoom factor is
     * negative, or the value is greater than the current cumulative
     * zoom in use.
     *
     * @param value The minimum supported cumulative zoom
     *
     * @return true If this minimum zoom successfully changed
     */
    bool setMinZoom(float value);
    
    /**
     * Returns the maximum supported cumulative zoom
     *
     * A zoom factor is a scaling factor. Values greater than 1
     * zoom in, enlarging the content. Values less than 1 (but
     * still postive) zoom out, shrinking the content. Negative
     * zoom values are not supported.
     *
     * At all times, the scroll pane keeps track of the cumulative
     * zoom in or out. This cumulative zoom must stay always be
     * less than the maximum zoom. Attempts to zoom further in
     * than this amount will be denied.
     *
     * @return the maximum supported cumulative zoom
     */
    float getMaxZoom() const { return _zoommax; }

    /**
     * Attempts to set the maximum supported cumulative zoom
     *
     * A zoom factor is a scaling factor. Values greater than 1
     * zoom in, enlarging the content. Values less than 1 (but
     * still postive) zoom out, shrinking the content. Negative
     * zoom values are not supported.
     *
     * At all times, the scroll pane keeps track of the cumulative
     * zoom in or out. This cumulative zoom must stay always be
     * less than the maximum zoom. Attempts to zoom further in
     * than this amount will be denied.
     *
     * This setter will fail if the value is less than the current
     * cumulative zoom in use.
     *
     * @param value The maximum supported cumulative zoom
     *
     * @return true If this maximum zoom successfully changed
     */
    bool setMaxZoom(float value);

    /**
     * Returns the transform matrix for the interior.
     *
     * The transform matrix is applied to the interior to transform
     * the contents from the node coordinate space to the interior
     * window coordinate space. This transform is adjusted by calling
     * {@link #applyPan}, {@link #applyZoom}, and {@link #applySpin}.
     *
     * This matrix will be the identity if {@link #resetPane} is called
     * on an unconstrained node. However, if the node is constrained
     * then the default transform will insure that the interior is
     * within bounds.
     *
     * @return the transform matrix for the interior.
     */
    const Affine2& getPaneTransform() const { return _panetrans; }
    
#pragma mark -
#pragma mark Navigation
    /**
     * Attempts to apply the given pan to the scroll pane.
     *
     * The pan is an offset value applied to the interior bounds.
     * Pan values are applied incrementally. You cannot set the
     * absolute pan, as it is impossible to do this while supporting
     * both zooming and spinning.  If you want to set an absolute
     * pan, you should call {@link #resetPane} and then apply the
     * pan.
     *
     * If the scroll pane is constrained, then any pan that would
     * show edges of the interior (such as going too far to the left
     * or the right) will not be allowed. Instead, it is replaced with
     * the closest legal pan (which is returned).
     *
     * @param delta The pan offset to apply
     *
     * @return the actual pan applied
     */
    Vec2 applyPan(const Vec2& delta);

    /**
     * Attempts to apply the given pan to the scroll pane.
     *
     * The pan is an offset value applied to the interior bounds.
     * Pan values are applied incrementally. You cannot set the
     * absolute pan, as it is impossible to do this while supporting
     * both zooming and spinning.  If you want to set an absolute
     * pan, you should call {@link #resetPane} and then apply the
     * pan.
     *
     * If the scroll pane is constrained, then any pan that would
     * show edges of the interior (such as going too far to the left
     * or the right) will not be allowed. Instead, it is replaced with
     * the closest legal pan (which is returned).
     *
     * @param dx    The x offset to apply
     * @param dy    The y offset to apply
     *
     * @return the actual pan applied
     */
    Vec2 applyPan(float dx, float dy);
    
    /**
     * Attempts to apply the given spin to the scroll pane.
     *
     * The spin is an angle of rotation. The angle is measured in
     * radians, just like {@link SceneNode#getAngle}. However,
     * the angle is applied to the interior content, and not the
     * scroll pane itself.
     *
     * If the scroll pane is constrained, then any spin angle that
     * would show edges of the interior (typically when the size
     * of the interior is smaller than the circumscribed circle
     * containing the content bounds) will not be allowed. Instead,
     * it is replaced with the closest legal angle in the same
     * direction (which is returned).
     *
     * The value {@link SceneNode#getAnchor} defines the origin of
     * the spin. The input handler should reset the anchor to spin
     * about different portions of the content region.
     *
     * @param angle  The spin angle in radians
     *
     * @return the actual spin applied
     */
    float applySpin(float angle);

    /**
     * Attempts to apply the given zoom to the scroll pane.
     *
     * A zoom factor is a scaling factor. Values greater than 1
     * zoom in, enlarging the content. Values less than 1 (but
     * still postive) zoom out, shrinking the content. Negative
     * zoom values are not supported.
     *
     * If the scroll pane is constrained, then any zoom factor
     * that would show edges of the interior (typically when
     * zooming out) will not be allowed. Instead, it is replaced
     * with the closest legal zoom factor (which is returned).
     *
     * At all times, the scroll pane keeps track of the cumulative
     * zoom in or out. This cumulative zoom must stay with the
     * range of {@link #getMinZoom} and {@link #getMaxZoom}.
     * If it were to go outside this range, it is replaced with
     * the closest legal zoom factor (which is returned).
     *
     * The value {@link SceneNode#getAnchor} defines the origin of
     * the zoom. The input handler should reset the anchor to zoom
     * in on different portions of the content region.
     *
     * @param zoom  The zoom factor
     *
     * @return the actual zoom factor applied
     */
    float applyZoom(float zoom);
        
    /**
     * Resets the interior to the default position.
     *
     * The default position is typically the untransformed interior,
     * meaing that {@link #getPaneTransform} returns the identity.
     * However, if the pane is constrained, a small or badly-positioned
     * interior may cause some out-of-bounds areas to be shown. In
     * that case, this method will pan (not zoom or spin) the interior
     * so that it is in bounds.
     *
     * If this cannot be accomplised (because the interior is too small),
     * then this method will set the constrained attribute to false.
     */
    void resetPane();

#pragma mark -
#pragma mark Rendering
    /**
     * Draws this Node and all of its children with the given SpriteBatch.
     *
     * You almost never need to override this method.  You should override the
     * method draw(shared_ptr<SpriteBatch>,const Affine2&,Color4) if you need to
     * define custom drawing code. In fact, overriding this method can break
     * the functionality of {@link OrderedNode}.
     *
     * @param batch     The SpriteBatch to draw with.
     * @param transform The global transformation matrix.
     * @param tint      The tint to blend with the Node color.
     */
    virtual void render(const std::shared_ptr<SpriteBatch>& batch, const Affine2& transform, Color4 tint) override;

    /**
     * Draws this Node and all of its children with the given SpriteBatch.
     *
     * You almost never need to override this method.  You should override the
     * method draw(shared_ptr<SpriteBatch>,const Affine2&,Color4) if you need to
     * define custom drawing code. In fact, overriding this method can break
     * the functionality of {@link OrderedNode}.
     *
     * @param batch     The SpriteBatch to draw with.
     */
    virtual void render(const std::shared_ptr<SpriteBatch>& batch) override {
        render(batch,Affine2::IDENTITY,Color4::WHITE);
    }
};
    }
}
#endif /* __CU_SCROLL_PANE_H__ */
