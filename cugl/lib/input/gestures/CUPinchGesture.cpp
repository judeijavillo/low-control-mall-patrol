//
//  CUPinchGesture.cpp
//  Cornell University Game Library (CUGL)
//
//  This class provides basic support for pinch gestures. A pinch gesture is
//  measured as the change of spread between the two fingers. So a pinch used
//  for a zoom out will have a positive change in spread, while a zoom in will
//  have a negative change in spread. Unlike the CoreGesture device, this will
//  always detect a pinch, even when other gestures are active.
//
//  This class is a singleton and should never be allocated directly.  It
//  should only be accessed via the Input dispatcher.
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
//  Version: 1/20/22
//
#include <cugl/base/CUBase.h>
#include <cugl/base/CUApplication.h>
#include <cugl/input/gestures/CUPinchGesture.h>
#include <algorithm>

/** The default stability for canceling pinches. */
#define DEFAULT_STABILITY 0.1f

using namespace cugl;

#pragma mark Constructor
/**
 * Creates and initializes a new pinch input device.
 *
 * WARNING: Never allocate a pinch input device directly.  Always use the
 * {@link Input#activate()} method instead.
 */
PinchGesture::PinchGesture() : InputDevice(),
_screen(false),
_active(false) {
#if defined CU_TOUCH_SCREEN
    _screen = true;
    Vec2 size = Application::get()->getDisplaySize();
    float diag = size.length();
#else
    float diag = sqrtf(2.0f);
#endif
    _stability = DEFAULT_STABILITY*diag;
}

/**
 * Deletes this input device, disposing of all resources
 */
void PinchGesture::dispose() {
    _active = false;
    _screen = false;
    _stability = 0;
}

/**
 * Returns true if this device is a touch screen.
 *
 * This device is not guaranteed to be a touch screen.  For example, the
 * trackpad on MacBooks support pinches. We do try to make our best guess
 * about whether or not a device is a touch screen, but on some devices
 * this may need to be set manually.
 *
 * If this value is true, all pinch information will scale with the display.
 * Otherwise, the pinch will be normalized to a unit square, where the
 * top left corner of the touch device is (0,0) and the lower right is
 * (1,1). You may want to set this value to false for true cross-platform
 * gesture support.
 *
 * @return true if this device is a touch screen.
 */
void PinchGesture::setTouchScreen(bool flag) {
    if (_screen != flag) {
        // Get the threshold adjustment
        Vec2 size = Application::get()->getDisplaySize();
        float factor = size.length()/sqrtf(2.0f);
        if (flag) {
            _stability *= factor;
        } else {
            _stability /= factor;
        }
        if (_active) {
            Timestamp now;
            cancelGesture(now);
        }
    }
    _screen = flag;
}

/**
 * Sets the movement stability of a pinch event.
 *
 * A pinch will be canceled if it encounters two much "lateral" movement.
 * Here lateral means perpendicular to the axis defined by the two fingers.
 * Movement along the axis will be ignored.
 *
 * If this device is a touch screen, this value should be measured in pixels.
 * Otherwise, this value should be set assuming a unit square, where the
 * top left corner of the touch device is (0,0) and the lower right is (1,1).
 * By default this value is 10% of the length of the diagonal of the touch
 * device.
 *
 * @param stability The movement stability of a pinch event.
 */
void PinchGesture::setStability(float stability) {
    CUAssertLog(stability >= 0, "Attempt to use negative stability %.3f",stability);
    _stability = stability;
}

#pragma mark -
#pragma mark Listeners
/**
 * Returns the cumulative pinch distance since the gesture began.
 *
 * This value is positive if the pinch is a zoom, and negative if it is
 * a true pinch.  A pinch can both zoom and pinch in a single gesture.
 *
 * @return the cumulative pinch distance since the gesture began.
 */
float PinchGesture::getPinch() const {
    if (_active) {
        return _event.currSpread-_event.origSpread;
    }
    return 0.0f;
}

/**
 * Requests focus for the given identifier
 *
 * Only a listener can have focus.  This method returns false if key
 * does not refer to an active listener
 *
 * @param key   The identifier for the focus object
 *
 * @return false if key does not refer to an active listener
 */
bool PinchGesture::requestFocus(Uint32 key) {
    if (isListener(key)) {
        _focus = key;
        return true;
    }
    return false;
}

/**
 * Returns true if key represents a listener object
 *
 * An object is a listener if it is a listener for any of the three actions:
 * pinch begin, pinch end, or pinch change.
 *
 * @param key   The identifier for the listener
 *
 * @return true if key represents a listener object
 */
bool PinchGesture::isListener(Uint32 key) const {
    bool result = _beginListeners.find(key) != _beginListeners.end();
    result = result || _finishListeners.find(key) != _finishListeners.end();
    result = result || _changeListeners.find(key) != _changeListeners.end();
    return result;
}

/**
 * Returns the pinch begin listener for the given object key
 *
 * This listener is invoked when pinch crosses the distance threshold.
 *
 * If there is no listener for the given key, it returns nullptr.
 *
 * @param key   The identifier for the listener
 *
 * @return the pinch begin listener for the given object key
 */
const PinchGesture::Listener PinchGesture::getBeginListener(Uint32 key) const {
    if (_beginListeners.find(key) != _beginListeners.end()) {
        return (_beginListeners.at(key));
    }
    return nullptr;
}

/**
 * Returns the pinch end listener for the given object key
 *
 * This listener is invoked when all (but one) fingers in an active pinch
 * are released.
 *
 * If there is no listener for the given key, it returns nullptr.
 *
 * @param key   The identifier for the listener
 *
 * @return the pinch end listener for the given object key
 */
const PinchGesture::Listener PinchGesture::getEndListener(Uint32 key) const {
    if (_finishListeners.find(key) != _finishListeners.end()) {
        return (_finishListeners.at(key));
    }
    return nullptr;
}

/**
 * Returns the pinch change listener for the given object key
 *
 * This listener is invoked when the pinch distance changes.
 *
 * @param key   The identifier for the listener
 *
 * @return the pinch change listener for the given object key
 */
const PinchGesture::Listener PinchGesture::getChangeListener(Uint32 key) const {
    if (_changeListeners.find(key) != _changeListeners.end()) {
        return (_changeListeners.at(key));
    }
    return nullptr;
}

/**
 * Adds a pinch begin listener for the given object key
 *
 * There can only be one listener for a given key.  If there is already
 * a listener for the key, the method will fail and return false.  You
 * must remove a listener before adding a new one for the same key.
 *
 * This listener is invoked when pinch crosses the distance threshold.
 *
 * @param key       The identifier for the listener
 * @param listener  The listener to add
 *
 * @return true if the listener was succesfully added
 */
bool PinchGesture::addBeginListener(Uint32 key, PinchGesture::Listener listener) {
    if (_beginListeners.find(key) == _beginListeners.end()) {
        _beginListeners[key] = listener;
        return true;
    }
    return false;
}

/**
 * Adds a pinch end listener for the given object key
 *
 * There can only be one listener for a given key.  If there is already
 * a listener for the key, the method will fail and return false.  You
 * must remove a listener before adding a new one for the same key.
 *
 * This listener is invoked when all (but one) fingers in an active pinch
 * are released.
 *
 * @param key       The identifier for the listener
 * @param listener  The listener to add
 *
 * @return true if the listener was succesfully added
 */
bool PinchGesture::addEndListener(Uint32 key, PinchGesture::Listener listener) {
    if (_finishListeners.find(key) == _finishListeners.end()) {
        _finishListeners[key] = listener;
        return true;
    }
    return false;
}

/**
 * Adds a pinch change listener for the given object key
 *
 * There can only be one listener for a given key.  If there is already
 * a listener for the key, the method will fail and return false.  You
 * must remove a listener before adding a new one for the same key.
 *
 * This listener is invoked when the pinch distance changes.
 *
 * @param key       The identifier for the listener
 * @param listener  The listener to add
 *
 * @return true if the listener was succesfully added
 */
bool PinchGesture::addChangeListener(Uint32 key, PinchGesture::Listener listener) {
    if (_changeListeners.find(key) == _changeListeners.end()) {
        _changeListeners[key] = listener;
        return true;
    }
    return false;
}

/**
 * Removes the pinch begin listener for the given object key
 *
 * If there is no active listener for the given key, this method fails and
 * returns false.
 *
 * This listener is invoked when pinch crosses the distance threshold.
 *
 * @param key   The identifier for the listener
 *
 * @return true if the listener was succesfully removed
 */
bool PinchGesture::removeBeginListener(Uint32 key) {
    if (_beginListeners.find(key) != _beginListeners.end()) {
        _beginListeners.erase(key);
        return true;
    }
    return false;
}

/**
 * Removes the pinch end listener for the given object key
 *
 * If there is no active listener for the given key, this method fails and
 * returns false.
 *
 * This listener is invoked when all (but one) fingers in an active pinch
 * are released.
 *
 * @param key   The identifier for the listener
 *
 * @return true if the listener was succesfully removed
 */
bool PinchGesture::removeEndListener(Uint32 key) {
    if (_finishListeners.find(key) != _finishListeners.end()) {
        _finishListeners.erase(key);
        return true;
    }
    return false;
}

/**
 * Removes the pinch change listener for the given object key
 *
 * If there is no active listener for the given key, this method fails and
 * returns false.
 *
 * This listener is invoked when the pinch distance changes.
 *
 * @param key   The identifier for the listener
 *
 * @return true if the listener was succesfully removed
 */
bool PinchGesture::removeChangeListener(Uint32 key) {
    if (_changeListeners.find(key) != _changeListeners.end()) {
        _changeListeners.erase(key);
        return true;
    }
    return false;
}


#pragma mark -
#pragma mark Input Device
/**
 * Clears the state of this input device, readying it for the next frame.
 *
 * Many devices keep track of what happened "this" frame.  This method is
 * necessary to advance the frame.
 */
void PinchGesture::clearState() {
    _updated = 0;
}

/**
 * Processes an SDL_Event
 *
 * The dispatcher guarantees that an input device only receives events that
 * it subscribes to.
 *
 * @param event The input event to process
 * @param stamp The event timestamp in CUGL time
 *
 * @return false if the input indicates that the application should quit.
 */
bool PinchGesture::updateState(const SDL_Event& event, const Timestamp& stamp) {
    switch (event.type) {
        case SDL_FINGERDOWN:
            _fingers[event.tfinger.fingerId] = getScaledPosition(event.tfinger.x,event.tfinger.y);
            if (_fingers.size() == 2) {
                startGesture(stamp);
            } else if (_active) {
                cancelGesture(stamp);
            }
            break;
        case SDL_FINGERUP:
        {
            auto finger = _fingers.find(event.tfinger.fingerId);
            if (finger != _fingers.end()) {
                _fingers.erase(finger);
                if (_fingers.size() == 2) {
                    startGesture(stamp);
                } else if (_active) {
                    cancelGesture(stamp);
                }
            }
        }
            break;
        case SDL_FINGERMOTION:
        {
            auto finger = _fingers.find(event.tfinger.fingerId);
            if (finger != _fingers.end()) {
                finger->second = getScaledPosition(event.tfinger.x,event.tfinger.y);
                _updated++;
                if (_active && _updated == 2) {
                    Vec2 move = computeCentroid()-_event.anchor;
                    Vec2 axis = computeAxis();
                    Vec2 perp = axis.getPerp();
                    move.project(perp);
                    if (move.lengthSquared() > _stability*_stability) {
                        cancelGesture(stamp);
                    } else {
                        float spread = axis.length();
                        _event.delta = spread-_event.currSpread;
                        _event.currSpread = spread;
                        _event.now = stamp;
                        for(auto it = _changeListeners.begin(); it != _changeListeners.end(); ++it) {
                            it->second(_event,_focus);
                        }
                    }
                }
            }

        }
            break;
        default:
            // Do nothing
            break;
    }
    return true;
}

/**
 * Returns the scale/unscaled touch position.
 *
 * The value returned depends on the value of attribute _screen.  If this
 * attribute is false, the position is normalized to the unit square.
 * Otherwise it is scaled to the touch screen.
 *
 * @return the scale/unscaled touch position.
 */
Vec2 PinchGesture::getScaledPosition(float x, float y) const {
    Vec2 result(x,y);
    if (_screen) {
        Rect bounds = Application::get()->getDisplayBounds();
        result *= bounds.size;
        result += bounds.origin;
    }
    return result;
}

/**
 * Determine the SDL events of relevance and store there types in eventset.
 *
 * An SDL_EventType is really Uint32.  This method stores the SDL event
 * types for this input device into the vector eventset, appending them
 * to the end. The Input dispatcher then uses this information to set up
 * subscriptions.
 *
 * @param eventset  The set to store the event types.
 */
void PinchGesture::queryEvents(std::vector<Uint32>& eventset) {
    eventset.push_back((Uint32)SDL_FINGERDOWN);
    eventset.push_back((Uint32)SDL_FINGERUP);
    eventset.push_back((Uint32)SDL_FINGERMOTION);
}


#pragma mark -
#pragma mark Internal Helpers
/**
 * Returns the centroid of the fingers
 *
 * The centroid is the average of all the fingers on the touch device.
 *
 * @return the centroid of the fingers
 */
Vec2 PinchGesture::computeCentroid() const {
    Vec2 result;
    int size = 0;

    for (auto it = _fingers.begin(); it != _fingers.end(); ++it) {
        result += it->second;
        size++;
    }
    result /= size;

    return result;
}

/**
 * Returns the axis of the fingers
 *
 * Naively, the axis is defined as the vector from the first finger
 * to the second. However, to enforce some stability, this method
 * assumes the axis always travels through the initial centroid of
 * the gesture. As the centroid may drift, this is not always the
 * case. To compensate, this method computes the weighted average
 * from the first finger to the centroid, and from the centroid to
 * the second finger.
 *
 * @return the axis of the fingers
 */
Vec2 PinchGesture::computeAxis() const {
    auto it = _fingers.begin();
    auto jt = _fingers.begin();
    ++jt;
    
    Vec2 v1 = it->second-_event.anchor;
    Vec2 v2 = _event.anchor-jt->second;
    
    float len = v1.length()+v2.length();
    v1.normalize();
    v2.normalize();
    v1 += v2;
    v1 *= len/2;

    return v1;
}

/**
 * Reinitializes the pinch event for a new gesture.
 *
 * This method calls all of the begin listeners with the new
 * gesture information.
 *
 * @param stamp The initial timestamp of the new gesture
 */
void PinchGesture::startGesture(const Timestamp& stamp) {
    _active = true;
    _event.start = stamp;
    _event.now = stamp;
    _event.anchor = computeCentroid();

    Vec2 axis = computeAxis();
    _event.origSpread = axis.length();
    _event.currSpread = _event.origSpread;
    _event.delta = 0;

    for(auto it = _beginListeners.begin(); it != _beginListeners.end(); ++it) {
        it->second(_event,_focus);
    }
}

/**
 * Finalizes the pinch event, preparing for a new gesture.
 *
 * This method calls all of the end listeners with the final
 * gesture information.
 *
 * @param stamp The final timestamp of the gesture
 */
void PinchGesture::cancelGesture(const Timestamp& stamp) {
    _event.now = stamp;
    for(auto it = _finishListeners.begin(); it != _finishListeners.end(); ++it) {
        it->second(_event,_focus);
    }
    _event.clear();
    _event.start = stamp;
    _active = false;
}
