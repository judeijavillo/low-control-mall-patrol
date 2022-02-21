//
//  CUButton.cpp
//  Cornell University Game Library (CUGL)
//
//  This module provides support for a simple clickable button. The button may
//  either be represented by two images (one up and one down), or a single
//  image and two different color tints.
//
//  The button can track its own state, relieving you of having to manually
//  check mouse presses. However, it can only do this when the button is part
//  of a scene graph, as the scene graph maps mouse coordinates to screen
//  coordinates.
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
//  Version: 8/20/20
//
#include <cugl/input/cu_input.h>
#include <cugl/scene2/CUScene2.h>
#include <cugl/scene2/ui/CUButton.h>
#include <cugl/scene2/layout/CULayout.h>
#include <cugl/assets/CUScene2Loader.h>
#include <vector>

using namespace cugl::scene2;

/** To define the size of an empty button */
#define DEFAULT_SIZE 50

#pragma mark -
#pragma mark Constructors
/**
 * Creates an uninitialized button with no size or texture information.
 *
 * You must initialize this button before use.
 *
 * NEVER USE A CONSTRUCTOR WITH NEW. If you want to allocate a Node on the
 * heap, use one of the static constructors instead.
 */
Button::Button() : SceneNode(),
_down(false),
_mouse(false),
_active(false),
_toggle(false),
_upform(nullptr),
_downform(nullptr),
_inputkey(0),
_nextKey(1),
_upcolor(Color4::WHITE),
_downcolor(Color4::WHITE),
_upchild(""),
_downchild("") {
    _classname = "Button";
}

/**
 * Initializes a button with the given node and color
 *
 * The button will look exactly like the given node when it is not pressed.
 * When pressed, it will tint the up node by the specified color.
 *
 * @param up    The button when it is not pressed
 * @param down  The button tint when it is pressed
 *
 * @return true if the button is initialized properly, false otherwise.
 */
bool Button::init(const std::shared_ptr<SceneNode>& up, Color4 down) {
    CUAssertLog(up, "Up representation cannot be null");
    if (!SceneNode::init()) {
        return false;
    }

    _upnode = up;
    _downnode = nullptr;
    _upcolor   = _tintColor;
    _downcolor = down;

    Size size = _upnode->getContentSize();
    _upnode->setAnchor(Vec2::ANCHOR_CENTER);
    _upnode->setPosition(size.width/2.0f,size.height/2.0f);
    setContentSize(size);
    addChild(_upnode);

    return true;
}

/**
 * Initializes a button with the given nodes
 *
 * The button will look exactly like the up node when it is not pressed.
 * It will look like the down node when it is pressed.  The size of this
 * button is the size of the larger of the two nodes.
 *
 * @param up    The button when it is not pressed
 * @param down  The button when it is pressed
 *
 * @return true if the button is initialized properly, false otherwise.
 */
bool Button::init(const std::shared_ptr<SceneNode>& up, const std::shared_ptr<SceneNode>& down) {
    CUAssertLog(up, "Up representation cannot be null");
    if (!SceneNode::init()) {
        return false;
    }

    _upnode = up;
    _downnode = down;
    
    _upcolor  = up->getColor();
    _upnode->setAnchor(Vec2::ANCHOR_CENTER);
    Size size = _upnode->getContentSize();

    if (down != nullptr) {
        _downcolor  = down->getColor();
        _downnode->setAnchor(Vec2::ANCHOR_CENTER);
        _downnode->setVisible(_upnode == nullptr);

        Size dsize  = _downnode->getContentSize();
        size.width  = dsize.width  > size.width  ? dsize.width  : size.width;
        size.height = dsize.height > size.height ? dsize.height : size.height;
    } else {
        _downcolor = Color4::GRAY*_upcolor;
    }
    
    _upnode->setPosition(size.width/2.0f,size.height/2.0f);
    addChild(_upnode);
    if (_downnode) {
        _downnode->setPosition(size.width/2.0f,size.height/2.0f);
        addChild(_downnode);
    }
    
    setContentSize(size);
    return true;
}

/**
 * Initializes a node with the given JSON specificaton.
 *
 * This initializer is designed to receive the "data" object from the
 * JSON passed to {@link SceneLoader}.  This JSON format supports all
 * of the attribute values of its parent class.  In addition, it supports
 * the following additional attributes:
 *
 *      "up":       A string referencing the name of a child node OR
 *                  a 4-element integer array with values from 0..255
 *      "down":     A string referencing the name of a child node OR
 *                  a 4-element integer array with values from 0..255
 *      "pushable": An even array of polygon vertices (numbers)
 *
 * The attribute 'up' is REQUIRED.  All other attributes are optional.
 *
 * @param loader    The scene loader passing this JSON file
 * @param data      The JSON object specifying the node
 *
 * @return true if initialization was successful.
 */
bool Button::initWithData(const Scene2Loader* loader, const std::shared_ptr<JsonValue>& data) {
    if (!data) {
        return init();
    } else if (!SceneNode::initWithData(loader,data)) {
        return false;
    }
    
    if (getContentSize() == Size::ZERO) {
        setContentSize(Size(DEFAULT_SIZE,DEFAULT_SIZE));
    }
    
    _toggle = data->getBool("toggle");
    std::shared_ptr<JsonValue> upnode = data->get("upnode");
    _upchild = "";
    _upcolor = Color4::WHITE;
    if (upnode != nullptr && upnode->size() > 0) {
        CUAssertLog(upnode->size() == 4, "The color 'up' must be a 4-element array of numbers 0..255.");
        _upcolor.r = upnode->get(0)->asInt(0);
        _upcolor.g = upnode->get(1)->asInt(0);
        _upcolor.b = upnode->get(2)->asInt(0);
        _upcolor.a = upnode->get(3)->asInt(0);
    } else if (upnode != nullptr) {
        _upchild = upnode->asString("");
    }

    std::shared_ptr<JsonValue> downnode = data->get("downnode");
    _downchild = "";
    _downcolor = Color4::CLEAR;
    if (downnode != nullptr && downnode->size() > 0) {
        CUAssertLog(downnode->size() == 4, "The color 'down' must be a 4-element array of numbers 0..255.");
        _downcolor.r = downnode->get(0)->asInt(0);
        _downcolor.g = downnode->get(1)->asInt(0);
        _downcolor.b = downnode->get(2)->asInt(0);
        _downcolor.a = downnode->get(3)->asInt(0);
    } else if (downnode != nullptr) {
        _downchild = downnode->asString("");
    }

    if (data->has("pushable")) {
        _bounds.set(data->get("pushable"));
    }
    
    return true;
}

/**
 * Disposes all of the resources used by this node.
 *
 * A disposed button can be safely reinitialized. Any children owned by
 * this node will be released.  They will be deleted if no other object
 * owns them.
 *
 * It is unsafe to call this on a button that is still currently inside
 * of a scene graph.
 */
void Button::dispose() {
    if (_active) {
        deactivate();
    }
    
    _upnode = nullptr;
    _downnode = nullptr;
    _upcolor = Color4::WHITE;
    _downcolor = Color4::WHITE;
    _bounds.clear();
    _listeners.clear();
    _nextKey = 1;
    _inputkey = 0;
    SceneNode::dispose();
}


#pragma mark -
#pragma mark Listeners
/**
 * Activates this button to listen for mouse/touch events.
 *
 * This method attaches a listener to either the {@link Mouse} or
 * {@link Touchscreen} inputs to monitor when the button is pressed and/or
 * released. The button will favor the mouse, but will use the touch screen
 * if no mouse input is active.  If neither input is active, this method
 * will fail.
 *
 * When active, the button will change its state on its own, without
 * requiring the user to use {@link setDown(bool)}.  If there is a
 * {@link Listener} attached, it will call that function upon any
 * state changes.
 *
 * @return true if the button was successfully activated
 */
bool Button::activate() {
    if (_active) {
        return false;
    }
    
    Mouse* mouse = Input::get<Mouse>();
    Touchscreen* touch = Input::get<Touchscreen>();
    CUAssertLog(mouse || touch,  "Neither mouse nor touch input is enabled");
    
    if (mouse) {
        _mouse = true;
        if (!_inputkey) { _inputkey = mouse->acquireKey(); }
        
        // Add the mouse listeners
        bool down = mouse->addPressListener(_inputkey, [=](const MouseEvent& event, Uint8 clicks, bool focus) {
            if (this->containsScreen(event.position)) {
                if (this->_toggle) {
                    this->setDown(!this->isDown());
                } else {
                    this->setDown(true);
                }
            }
        });
        
        bool up = false;
        if (down) {
            up = mouse->addReleaseListener(_inputkey, [=](const MouseEvent& event, Uint8 clicks, bool focus) {
                if (this->isDown()) {
                    if (!this->_toggle) {
                        this->setDown(false);
                    }
                }
            });
            if (!up) {
                mouse->removePressListener(_inputkey);
            }
        }
        
        _active = up & down;
    } else {
        _mouse = false;
        if (!_inputkey) { _inputkey = touch->acquireKey(); }

        // Add the mouse listeners
        bool down = touch->addBeginListener(_inputkey, [=](const TouchEvent& event, bool focus) {
            if (this->containsScreen(event.position)) {
                if (this->_toggle) {
                    this->setDown(!this->isDown());
                } else {
                    this->setDown(true);
                }
            }
        });
        
        bool up = false;
        if (down) {
            up = touch->addEndListener(_inputkey, [=](const TouchEvent& event, bool focus) {
                if (this->isDown()) {
                    if (!this->_toggle) {
                        this->setDown(false);
                    }
                }
            });
            if (!up) {
                touch->removeBeginListener(_inputkey);
            }
        }
        
        _active = up & down;
    }

    return _active;
}

/**
 * Deactivates this button, ignoring future mouse/touch events.
 *
 * This method removes its internal listener from either the {@link Mouse}
 * or {@link Touchscreen} inputs to monitor when the button is pressed and/or
 * released.  The input affected is the one that received the listener
 * upon activation.
 *
 * When deactivated, the button will no longer change its state on its own.
 * However, the user can still change the state manually with the
 * {@link setDown(bool)} method.  In addition, any {@link Listener}
 * attached will still respond to manual state changes.
 *
 * @return true if the button was successfully deactivated
 */
bool Button::deactivate() {
    if (!_active) {
        return false;
    }

    bool success = false;
    if (_mouse) {
        Mouse* mouse = Input::get<Mouse>();
        CUAssertLog(mouse,  "Mouse input is no longer enabled");
        success = mouse->removePressListener(_inputkey);
        success = mouse->removeReleaseListener(_inputkey) && success;
    } else {
        Touchscreen* touch = Input::get<Touchscreen>();
        CUAssertLog(touch,  "Touch input is no longer enabled");
        success = touch->removeBeginListener(_inputkey);
        success = touch->removeEndListener(_inputkey) && success;
    }

    _active = false;
    _mouse = false;

    return success;
}

/**
 * Returns the listener for the given key
 *
 * This listener is invoked when the button state changes (up or down).
 *
 * If there is no listener for the given key, it returns nullptr.
 *
 * @param key   The identifier for the listener
 *
 * @return the listener for the given key
 */
const Button:: Listener Button::getListener(Uint32 key) const {
    auto item = _listeners.find(key);
    if (item == _listeners.end()) {
        return nullptr;
    }
    return item->second;
}

/**
 * Returns all listeners for this button
 *
 * These listeners are invoked when the button state changes (up or down).
 *
 * @return the listener (if any) for this button
 */
const std::vector<Button::Listener> Button::getListeners() const {
    std::vector<Listener> result;
    result.reserve(_listeners.size());
    for(auto kv : _listeners) {
        result.push_back(kv.second);
    }
    return result;
}

/**
 * Returns a key for a listener after adding it to this button.
 *
 * This listener is invoked when the button state changes (up or down).
 *
 * C++ cannot hash functions types.  Therefore, the listener will
 * be identified by a unique key, returned by this function.  You should
 * remember this key to remove the listener if necessary.
 *
 * This also means that adding a listener twice will add it for an additional
 * key, causing the listener to be called multiple times on a state change.
 *
 * @param listener  The listener to add
 *
 * @return the key for the listener
 */
Uint32 Button::addListener(Listener listener) {
    CUAssertLog(_nextKey < (Uint32)-1, "No more available listener slots");
    _listeners[_nextKey++] = listener;
    return _nextKey;
}

/**
 * Removes a listener from this button.
 *
 * This listener is invoked when the button state changes (up or down).
 *
 * Listeners must be identified by the key returned by the {@link #addListener}
 * method. If this button does not have a listener for the given key,
 * this method will fail.
 *
 * @param key  The key of the listener to remove
 *
 * @return true if the listener was succesfully removed
 */
bool Button::removeListener(Uint32 key) {
    if (_listeners.find(key) == _listeners.end()) {
        return false;
    }
    _listeners.erase(key);
    return true;
}

/**
 * Clears all listeners for this button.
 *
 * These listeners are invoked when the button state changes (up or down).
 * This method does not require you to remember the keys assigned to the
 * individual listeners.
 *
 * @return true if the listener was succesfully removed
 */
void Button::clearListeners() {
    _listeners.clear();
}


#pragma mark -
#pragma mark Button Attributes
/**
 * Sets the color tinting this node.
 *
 * This color will be multiplied with the parent (this node on top) if
 * hasRelativeColor() is true.
 *
 * The default color is white, which means that all children have their
 * natural color.
 *
 * @param color the color tinting this node.
 */
void Button::setColor(Color4 color) {
    _upcolor = color;
    if (!_down || _downnode) {
        _tintColor = color;
    }
}

/**
 * Sets the region responding to mouse clicks.
 *
 * The pushable region is the area of this node that responds to mouse
 * clicks. By allowing it to be an arbitrary path, we are capable of
 * defining buttons with complex shapes. The path should be specified
 * counter-clockwise.
 *
 * @param bounds    The region responding to mouse clicks.
 */
void Button::setPushable(const Path2& bounds) {
    _bounds = bounds;
}

/**
 * Sets the region responding to mouse clicks.
 *
 * The pushable region is the area of this node that responds to mouse
 * clicks. By allowing it to be an arbitrary path, we are capable of
 * defining buttons with complex shapes. The path should be specified
 * counter-clockwise.
 *
 * @param vertices  The region responding to mouse clicks.
 */
void Button::setPushable(const std::vector<Vec2>& vertices) {
    _bounds.clear();
    _bounds.vertices = vertices;
    _bounds.closed = true;
}

#pragma mark -
#pragma mark Button State
/**
 * Sets whether this button is currently down.
 *
 * Buttons only have two states: up and down.  The default state is up.
 *
 * Changing this value will change how the button is displayed on the
 * screen.  It will also invoke the {@link Listener} if one is
 * currently attached.
 *
 * @param down  Whether this button is currently down.
 */
void Button::setDown(bool down) {
    if (_down == down) {
        return;
    }
    
    _down = down;
    if (down && _downnode && _upnode) {
        _upnode->setVisible(false);
        _downnode->setVisible(true);
    } else if (down) {
        _tintColor = _downcolor;
    }
    
    if (!down && _downnode && _upnode) {
        _upnode->setVisible(true);
        _downnode->setVisible(false);
    } else if (!down) {
        _tintColor = _upcolor;
    }
    
    for(auto it = _listeners.begin(); it != _listeners.end(); ++it) {
        it->second(getName(),down);
    }
}

/**
 * Returns true if this button contains the given screen point
 *
 * This method is used to manually check for mouse presses/touches.  It
 * converts a point in screen coordinates to the node coordinates and
 * checks if it is in the bounds of the button.
 *
 * @param point The point in screen coordinates
 *
 * @return true if this button contains the given screen point
 */
bool Button::containsScreen(const Vec2 point) {
    Vec2 local = screenToNodeCoords(point);
    if (_bounds.size() > 0) {
        return _bounds.contains(local);
    }
    return Rect(Vec2::ZERO, getContentSize()).contains(local);
}

#pragma mark -
#pragma mark Button Layout

/**
 * Arranges the child of this node using the layout manager.
 *
 * This process occurs recursively and top-down. A layout manager may end
 * up resizing the children.  That is why the parent must finish its layout
 * before we can apply a layout manager to the children.
 */
void Button::doLayout() {
    // Revision for 2019: Lazy attachment of up and down nodes.
    if (_upnode == nullptr) {
        // All of the code that follows can corrupt the position.
        Vec2 coord = getPosition();
        Size osize = getContentSize();
        Size size = osize;
        
        if (_upchild != "") {
            _upnode = getChildByName(_upchild);
        }
        if (_downchild != "") {
            _downnode = getChildByName(_downchild);
        }
        
        // Compute the sizes first
        if (_upnode == nullptr) {
            _upnode  = PolygonNode::allocWithTexture(Texture::getBlank());
            // Color was set during the init
            Size curr = _upnode->getContentSize();
            if (_json == nullptr || !_json->has("size")) {
                size.set(DEFAULT_SIZE, DEFAULT_SIZE);
            }
            _upnode->setScale(size.width/curr.width,size.height/curr.height);
        } else {
            _upcolor = _upnode->getColor();
            if (_json == nullptr || !_json->has("size")) {
                size = _upnode->getSize();
            }
        }
        if (_downnode != nullptr) {
            _downcolor = _downnode->getColor();
            _downnode->setVisible(_upnode == nullptr);
            Size dsize  = _downnode->getSize();
            if (_json == nullptr || !_json->has("size")) {
                size.width  = dsize.width  > size.width  ? dsize.width  : size.width;
                size.height = dsize.height > size.height ? dsize.height : size.height;
            }
        } else if (_downcolor == Color4::CLEAR) {
            _downcolor = _upcolor*Color4::GRAY;
        }
        
        osize = size;
        setContentSize(size);
        
        // Now position them
        if (_upnode != nullptr) {
            bool anchor = false;
            bool position = false;
            if (_json->has("children") && _json->get("children")->has(_upnode->getName())) {
                std::shared_ptr<JsonValue> data = _json->get("children")->get(_upnode->getName());
                if (data->has("data")) {
                    data = data->get("data");
                    anchor = data->has("anchor");
                    position = data->has("position");
                }
            }
            if (!anchor) {
                _upnode->setAnchor(Vec2::ANCHOR_CENTER);
            }
            if (!position) {
                _upnode->setPosition(size.width/2.0f,size.height/2.0f);
            }
        }
        
        if (_downnode != nullptr) {
            bool anchor = false;
            bool position = false;
            if (_json->has("children") && _json->get("children")->has(_downnode->getName())) {
                std::shared_ptr<JsonValue> data = _json->get("children")->get(_downnode->getName());
                if (data->has("data")) {
                    data = data->get("data");
                    anchor = data->has("anchor");
                    position = data->has("position");
                }
            }
            if (!anchor) {
                _downnode->setAnchor(Vec2::ANCHOR_CENTER);
            }
            if (!position) {
                _downnode->setPosition(size.width/2.0f,size.height/2.0f);
            }
        }
        
        if (_bounds.size() > 0) {
            Vec2 scale;
            scale.x = (osize.width > 0 ? size.width/osize.width : 0);
            scale.y = (osize.height > 0 ? size.height/osize.height : 0);
            _bounds *= scale;
        }
        
        // Now redo the position
        setPosition(coord);
    }
    SceneNode::doLayout();
}
