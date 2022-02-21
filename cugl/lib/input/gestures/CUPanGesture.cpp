//
//  CUPanGesture.cpp
//  Cornell University Game Library (CUGL)
//
//  This class provides basic support for multifinger pan gestures. Unlike the
//  CoreGesture device, this will always detect a pan, even when other gestures
//  are active.  Futhermore, it is not limited to two-finger pans. It can detect
//  detect any pan of two or more fingers.
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
#include <cugl/input/gestures/CUPanGesture.h>
#include <algorithm>

using namespace cugl;

#pragma mark Constructor
/**
 * Creates and initializes a new pan input device.
 *
 * WARNING: Never allocate a pan input device directly.  Always use the
 * {@link Input#activate()} method instead.
 */
PanGesture::PanGesture() : InputDevice(),
_screen(false),
_active(false),
_fingery(true),
_updated(0) {
#if defined CU_TOUCH_SCREEN
    _screen = true;
#endif
}

/**
 * Deletes this input device, disposing of all resources
 */
void PanGesture::dispose() {
    _active = false;
    _screen = false;
    _updated = 0;
    _fingery = false;
}

/**
 * Returns true if this device is a touch screen.
 *
 * This device is not guaranteed to be a touch screen.  For example, the
 * trackpad on MacBooks support panning. We do try to make our best guess
 * about whether or not a device is a touch screen, but on some devices
 * this may need to be set manually.
 *
 * If this value is true, all pan information will scale with the display.
 * Otherwise, the pan will be normalized to a unit square, where the
 * top left corner of the touch device is (0,0) and the lower right is
 * (1,1). You may want to set this value to false for true cross-platform
 * gesture support.
 *
 * @return true if this device is a touch screen.
 */
void PanGesture::setTouchScreen(bool flag) {
    _screen = flag;
}

#pragma mark -
#pragma mark Listeners
/**
 * Returns the cumulative pan vector since the gesture began.
 *
 * @return the cumulative pan vector since the gesture began.
 */
const Vec2 PanGesture::getPan() const {
    if (_active) {
        return _event.currPosition-_event.origPosition;
    }
    return Vec2::ZERO;
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
bool PanGesture::requestFocus(Uint32 key) {
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
 * pan begin, pan end, or pan change.
 *
 * @param key   The identifier for the listener
 *
 * @return true if key represents a listener object
 */
bool PanGesture::isListener(Uint32 key) const {
    bool result = _beginListeners.find(key) != _beginListeners.end();
    result = result || _finishListeners.find(key) != _finishListeners.end();
    result = result || _motionListeners.find(key) != _motionListeners.end();
    return result;
}

/**
 * Returns the pan begin listener for the given object key
 *
 * This listener is invoked when pan crosses the distance threshold.
 *
 * If there is no listener for the given key, it returns nullptr.
 *
 * @param key   The identifier for the listener
 *
 * @return the pan begin listener for the given object key
 */
const PanGesture::Listener PanGesture::getBeginListener(Uint32 key) const {
    if (_beginListeners.find(key) != _beginListeners.end()) {
        return (_beginListeners.at(key));
    }
    return nullptr;
}

/**
 * Returns the pan end listener for the given object key
 *
 * This listener is invoked when all (but one) fingers in an active pan
 * are released.
 *
 * If there is no listener for the given key, it returns nullptr.
 *
 * @param key   The identifier for the listener
 *
 * @return the pan end listener for the given object key
 */
const PanGesture::Listener PanGesture::getEndListener(Uint32 key) const {
    if (_finishListeners.find(key) != _finishListeners.end()) {
        return (_finishListeners.at(key));
    }
    return nullptr;
}

/**
 * Returns the pan change listener for the given object key
 *
 * This listener is invoked when the pan distance changes.
 *
 * @param key   The identifier for the listener
 *
 * @return the pan change listener for the given object key
 */
const PanGesture::Listener PanGesture::getChangeListener(Uint32 key) const {
    if (_motionListeners.find(key) != _motionListeners.end()) {
        return (_motionListeners.at(key));
    }
    return nullptr;
}

/**
 * Adds a pan begin listener for the given object key
 *
 * There can only be one listener for a given key.  If there is already
 * a listener for the key, the method will fail and return false.  You
 * must remove a listener before adding a new one for the same key.
 *
 * This listener is invoked when pan crosses the distance threshold.
 *
 * @param key       The identifier for the listener
 * @param listener  The listener to add
 *
 * @return true if the listener was succesfully added
 */
bool PanGesture::addBeginListener(Uint32 key, PanGesture::Listener listener) {
    if (_beginListeners.find(key) == _beginListeners.end()) {
        _beginListeners[key] = listener;
        return true;
    }
    return false;
}

/**
 * Adds a pan end listener for the given object key
 *
 * There can only be one listener for a given key.  If there is already
 * a listener for the key, the method will fail and return false.  You
 * must remove a listener before adding a new one for the same key.
 *
 * This listener is invoked when all (but one) fingers in an active pan
 * are released.
 *
 * @param key       The identifier for the listener
 * @param listener  The listener to add
 *
 * @return true if the listener was succesfully added
 */
bool PanGesture::addEndListener(Uint32 key, PanGesture::Listener listener) {
    if (_finishListeners.find(key) == _finishListeners.end()) {
        _finishListeners[key] = listener;
        return true;
    }
    return false;
}

/**
 * Adds a pan change listener for the given object key
 *
 * There can only be one listener for a given key.  If there is already
 * a listener for the key, the method will fail and return false.  You
 * must remove a listener before adding a new one for the same key.
 *
 * This listener is invoked when the pan distance changes.
 *
 * @param key       The identifier for the listener
 * @param listener  The listener to add
 *
 * @return true if the listener was succesfully added
 */
bool PanGesture::addChangeListener(Uint32 key, PanGesture::Listener listener) {
    if (_motionListeners.find(key) == _motionListeners.end()) {
        _motionListeners[key] = listener;
        return true;
    }
    return false;
}

/**
 * Removes the pan begin listener for the given object key
 *
 * If there is no active listener for the given key, this method fails and
 * returns false.
 *
 * This listener is invoked when pan crosses the distance threshold.
 *
 * @param key   The identifier for the listener
 *
 * @return true if the listener was succesfully removed
 */
bool PanGesture::removeBeginListener(Uint32 key) {
    if (_beginListeners.find(key) != _beginListeners.end()) {
        _beginListeners.erase(key);
        return true;
    }
    return false;
}

/**
 * Removes the pan end listener for the given object key
 *
 * If there is no active listener for the given key, this method fails and
 * returns false.
 *
 * This listener is invoked when all (but one) fingers in an active pan
 * are released.
 *
 * @param key   The identifier for the listener
 *
 * @return true if the listener was succesfully removed
 */
bool PanGesture::removeEndListener(Uint32 key) {
    if (_finishListeners.find(key) != _finishListeners.end()) {
        _finishListeners.erase(key);
        return true;
    }
    return false;
}

/**
 * Removes the pan change listener for the given object key
 *
 * If there is no active listener for the given key, this method fails and
 * returns false.
 *
 * This listener is invoked when the pan distance changes.
 *
 * @param key   The identifier for the listener
 *
 * @return true if the listener was succesfully removed
 */
bool PanGesture::removeChangeListener(Uint32 key) {
    if (_motionListeners.find(key) != _motionListeners.end()) {
        _motionListeners.erase(key);
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
void PanGesture::clearState() {
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
bool PanGesture::updateState(const SDL_Event& event, const Timestamp& stamp) {
    switch (event.type) {
        case SDL_FINGERDOWN:
            _fingers[event.tfinger.fingerId] = getScaledPosition(event.tfinger.x,event.tfinger.y);
            // We need at least two fingers
            if (_event.fingers >= 2) {
                if (_active) {
                    _event.now = stamp;
                    if (_fingery) {
                        // Reboot events when we add fingers
                        for(auto it = _finishListeners.begin(); it != _finishListeners.end(); ++it) {
                            it->second(_event,it->first == _focus);
                        }
                        _event.fingers++;
                        _event.currPosition = computeCentroid();
                        for(auto it = _beginListeners.begin(); it != _beginListeners.end(); ++it) {
                            it->second(_event,it->first == _focus);
                        }
                    } else {
                        _event.fingers++;
                    }
                } else {
                    startGesture(stamp);
                }
            }
            break;
        case SDL_FINGERUP:
        {
            auto finger = _fingers.find(event.tfinger.fingerId);
            if (finger != _fingers.end()) {
                _fingers.erase(finger);
                if (_active) {
                    if (_event.fingers <= 2) {
                        cancelGesture(stamp);
                    } else if (_fingery) {
                        _event.now = stamp;
                        // Reboot events when we remove fingers
                        for(auto it = _finishListeners.begin(); it != _finishListeners.end(); ++it) {
                            it->second(_event,it->first == _focus);
                        }
                        _event.fingers--;
                        _event.currPosition = computeCentroid();
                        for(auto it = _beginListeners.begin(); it != _beginListeners.end(); ++it) {
                            it->second(_event,it->first == _focus);
                        }
                    }
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
                if (_active && _updated == _fingers.size()) {
                    Vec2 position = computeCentroid();
                    _event.delta = position-_event.currPosition;
                    _event.currPosition = position;
                    _event.now = stamp;
                    
                    for(auto it = _motionListeners.begin(); it != _motionListeners.end(); ++it) {
                        it->second(_event,_focus);
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
Vec2 PanGesture::getScaledPosition(float x, float y) {
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
void PanGesture::queryEvents(std::vector<Uint32>& eventset) {
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
Vec2 PanGesture::computeCentroid() const {
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
 * Reinitializes the pan event for a new pan gesture.
 *
 * This method calls all of the begin listeners with the new
 * gesture information.
 *
 * @param stamp The initial timestamp of the new gesture
 */
void PanGesture::startGesture(const Timestamp& stamp) {
    _active = true;
    _event.start = stamp;
    _event.now = stamp;
    _event.origPosition = computeCentroid();
    _event.currPosition = _event.origPosition;
    _event.delta.setZero();
    _event.fingers = (Uint32)_fingers.size();

    for(auto it = _beginListeners.begin(); it != _beginListeners.end(); ++it) {
        it->second(_event,_focus);
    }
}

/**
 * Finalizes the pan event, preparing for a new pan gesture.
 *
 * This method calls all of the end listeners with the final
 * gesture information.
 *
 * @param stamp The final timestamp of the gesture
 */
void PanGesture::cancelGesture(const Timestamp& stamp) {
    _event.now = stamp;
    for(auto it = _finishListeners.begin(); it != _finishListeners.end(); ++it) {
        it->second(_event,_focus);
    }
    _event.clear();
    _event.start = stamp;
    _active = false;
}
