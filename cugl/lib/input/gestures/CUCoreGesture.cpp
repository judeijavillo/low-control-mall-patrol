//
//  CUCoreGesture.cpp
//  CUGL
//
//  This class provides basic support for the standard two-finger gestures provided
//  by all mobile platforms: pan, pinch, and spin.  While there is native code for
//  managing these on each platform, this device ensures that the gestures are processed
//  in a consistent way across all platforms.
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
#include <cugl/input/gestures/CUCoreGesture.h>
#include <algorithm>

/** The default percentage used for thresholds. */
#define DEFAULT_PERCENTAGE 0.05f
/** The default angle threshold. */
#define DEFAULT_ANGLE M_PI/45

using namespace cugl;

#pragma mark Constructor
/**
 * Creates and initializes a new pinch input device.
 *
 * WARNING: Never allocate a pinch input device directly.  Always use the
 * {@link Input#activate()} method instead.
 */
CoreGesture::CoreGesture() : InputDevice(),
_screen(false),
_active(false),
_activePan(true),
_activePinch(true),
_activeSpin(true),
_updated(0) {
#if defined CU_TOUCH_SCREEN
    _screen = true;
    Vec2 size = Application::get()->getDisplaySize();
    float diag = size.length();
#else
    float diag = sqrtf(2.0f);
#endif
    _panThreshold = DEFAULT_PERCENTAGE*diag;
    _pinchThreshold = DEFAULT_PERCENTAGE*diag;
    _spinRadius = 2*DEFAULT_PERCENTAGE*diag;
    _spinThreshold = DEFAULT_ANGLE;
}

/**
 * Deletes this input device, disposing of all resources
 */
void CoreGesture::dispose() {
    _active = false;
    _screen = false;
    _panThreshold = 0;
    _pinchThreshold = 0;
    _spinRadius = 0;
    _spinThreshold = 0;
    _updated = 0;
    _activePan = false;
    _activePinch = false;
    _activeSpin = false;
    _fingers.clear();
    _startListeners.clear();
    _deltaListeners.clear();
    _endListeners.clear();
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
void CoreGesture::setTouchScreen(bool flag) {
    if (_screen != flag) {
        // Get the threshold adjustment
        Vec2 size = Application::get()->getDisplaySize();
        float factor = size.length()/sqrtf(2.0f);
        if (flag) {
            _panThreshold *= factor;
            _pinchThreshold *= factor;
            _spinRadius *= factor;
        } else {
            _panThreshold /= factor;
            _pinchThreshold /= factor;
            _spinRadius /= factor;
        }
        if (_active) {
            Timestamp now;
            cancelGesture(now);
        }
    }
    _screen = flag;
}

#pragma mark -
#pragma mark Thresholds
/**
 * Sets the delta threshold for pan events.
 *
 * In order to separate the gestures, pan events have an initial resistance.
 * The device will only recognize a gesture as a pan event once the cumulative
 * pan has exceeded the provided threshold. Once the device has recognized the
 * gesture as a pan event, it will not recognize it as a pinch or spin, no
 * matter how the fingers are moved. The user will need to remove one or both
 * fingers to reset the gesture.
 *
 * If this device is a touch screen, this value should be measured in pixels.
 * Otherwise, this value should be set assuming a unit square, where the
 * top left corner of the touch device is (0,0) and the lower right is (1,1).
 * By default this value is 5% of the length of the diagonal of the touch
 * device.
 *
 * @param threshold The distance threshold for pan events.
 */
void CoreGesture::setPanThreshold(float threshold) {
    CUAssertLog(threshold >= 0, "Attempt to use negative threshold %.3f",threshold);
    _panThreshold = threshold;
}
/**
 * Sets the distance threshold for pinch events.
 *
 * In order to separate the gestures, pinch events have an initial resistance.
 * The device will only recognize a gesture as a pinch event once the absolute
 * value of the difference between the current pinch and the initial pinch has
 * has exceeded the provided threshold. Once the device has recognized the
 * gesture as a pinch event, it will not recognize it as a pan or spin, no
 * matter how the fingers are moved. The user will need to remove one or both
 * fingers to reset the gesture.
 *
 * If this device is a touch screen, this value should be measured in pixels.
 * Otherwise, this value should be set assuming a unit square, where the
 * top left corner of the touch device is (0,0) and the lower right is (1,1).
 * By default this value is 5% of the length of the diagonal of the touch
 * device.
 *
 * @param threshold The distance threshold for pinch events.
 */
void CoreGesture::setPinchThreshold(float threshold) {
    CUAssertLog(threshold >= 0, "Attempt to use negative threshold %.3f",threshold);
    _pinchThreshold = threshold;
}

/**
 * Sets the angle threshold for spin events.
 *
 * In order to separate the gestures, spin events have an initial resistance.
 * The device will only recognize a gesture as a spin event once the absolute
 * value of the difference between the current angle and the initial angle has
 * has exceeded the provided threshold. Once the device has recognized the
 * gesture as a pinch event, it will not recognize it as a pan or spin, no
 * matter how the fingers are moved. The user will need to remove one or both
 * fingers to reset the gesture.
 *
 * Since angles measurements do not depend on the size of the touch device,
 * this threshold is the same regardless of whether or not this devices is a
 * touch screen. However, spins have an addition requirement that the fingers
 * must be separated by a minimum distance, as given by {@link #getSpinRadius}.
 * The default value is 2 degrees.
 *
 * @param threshold The angle threshold for spin events.
 */
void CoreGesture::setSpinThreshold(float threshold) {
    CUAssertLog(threshold >= 0, "Attempt to use negative threshold %.3f",threshold);
    _spinThreshold = threshold;
}

/**
 * Sets the minimum radius for a spin event.
 *
 * All spins have an additional rquirement that all the fingers must be
 * separated by a minimum distance. This is a natural requirement for
 * spins, and it greatly reduces the possibility of accidental spins.
 *
 * If this device is a touch screen, this value should be measured in pixels.
 * Otherwise, this value should be set assuming a unit square, where the
 * top left corner of the touch device is (0,0) and the lower right is (1,1).
 * By default this value is 5% of the length of the diagonal of the touch
 * device.
 *
 * @param radius    The minimum radius for a spin event.
 */
void CoreGesture::setSpinRadius(float radius) {
    CUAssertLog(radius >= 0, "Attempt to use negative radius %.3f",radius);
    _spinRadius = radius;
}

#pragma mark -
#pragma mark Listeners
/**
 * Requests focus for the given identifier
 *
 * Only a listener can have focus. This method returns false if key does
 * not refer to an active listener
 *
 * @param key   The identifier for the focus object
 *
 * @return false if key does not refer to an active listener
 */
bool CoreGesture::requestFocus(Uint32 key) {
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
bool CoreGesture::isListener(Uint32 key) const {
    bool result = _startListeners.find(key) != _startListeners.end();
    result = result | _endListeners.find(key) != _endListeners.end();
    result = result | _deltaListeners.find(key) != _deltaListeners.end();
    return result;
}

/**
 * Returns the gesture begin listener for the given object key
 *
 * This listener is invoked when two fingers are detected on the device.
 * Note that the gesture type is rarely determined at the start. Instead,
 * the gesture type is only assigned (via a change listener) once it
 * crosses a certain threshold.
 *
 * If there is no listener for the given key, this method returns nullptr.
 *
 * @param key   The identifier for the begin listener
 *
 * @return the gesture begin listener for the given object key
 */
const CoreGesture::Listener CoreGesture::getBeginListener(Uint32 key) const {
    if (_startListeners.find(key) != _startListeners.end()) {
        return (_startListeners.at(key));
    }
    return nullptr;
}

/**
 * Returns the gesture end listener for the given object key
 *
 * This listener is invoked when there are no longer only two fingers
 * on the device. This could mean that one or more fingers was removed.
 * Or it could mean that a third finger (or more) was added.
 *
 * If there is no listener for the given key, this method returns nullptr.
 *
 * @param key   The identifier for the end listener
 *
 * @return the gesture begin listener for the given object key
 */
const CoreGesture::Listener CoreGesture::getEndListener(Uint32 key) const {
    if (_endListeners.find(key) != _endListeners.end()) {
        return (_endListeners.at(key));
    }
    return nullptr;
}

/**
 * Returns the gesture end listener for the given object key
 *
 * This listener is invoked when the gesture is updated. A gesture is
 * only updated once it has a definitive type. Hence the first time
 * this listener is called after a start event, the gesture will have
 * an assigned type.
 *
 * If there is no listener for the given key, this method returns nullptr.
 *
 * @param key   The identifier for the change listener
 *
 * @return the gesture begin listener for the given object key
 */
const CoreGesture::Listener CoreGesture::getChangeListener(Uint32 key) const {
    if (_deltaListeners.find(key) != _deltaListeners.end()) {
        return (_deltaListeners.at(key));
    }
    return nullptr;
}

/**
 * Adds a gesture begin listener for the given object key
 *
 * There can only be one begin listener for a given key. If there is
 * already a listener for the key, the method will fail and return false.
 * You must remove a listener before adding a new one for the same key.
 *
 * This listener is invoked when two fingers are detected on the device.
 * Note that the gesture type is rarely determined at the start. Instead,
 * the gesture type is only assigned (via a change listener) once it
 * crosses a certain threshold.
 *
 * @param key       The identifier for the begin listener
 * @param listener  The listener to add
 *
 * @return true if the listener was succesfully added
 */
bool CoreGesture::addBeginListener(Uint32 key, Listener listener) {
    if (_startListeners.find(key) == _startListeners.end()) {
        _startListeners[key] = listener;
        return true;
    }
    return false;
}

/**
 * Adds a gesture end listener for the given object key
 *
 * There can only be one end listener for a given key.  If there is
 * already a listener for the key, the method will fail and return false.
 * You must remove a listener before adding a new one for the same key.
 *
 * This listener is invoked when there are no longer only two fingers
 * on the device. This could mean that one or more fingers was removed.
 * Or it could mean that a third finger (or more) was added.
 *
 * @param key       The identifier for the end listener
 * @param listener  The listener to add
 *
 * @return true if the listener was succesfully added
 */
bool CoreGesture::addEndListener(Uint32 key, Listener listener) {
    if (_endListeners.find(key) == _endListeners.end()) {
        _endListeners[key] = listener;
        return true;
    }
    return false;
}

/**
 * Adds a gesture change listener for the given object key
 *
 * There can only be one change listener for a given key.  If there is
 * already a listener for the key, the method will fail and return false.
 * You must remove a listener before adding a new one for the same key.
 *
 * This listener is invoked when the gesture is updated. A gesture is
 * only updated once it has a definitive type. Hence the first time
 * this listener is called after a start event, the gesture will have
 * an assigned type.
 *
 * @param key       The identifier for the change listener
 * @param listener  The listener to add
 *
 * @return true if the listener was succesfully added
 */
bool CoreGesture::addChangeListener(Uint32 key, Listener listener) {
    if (_deltaListeners.find(key) == _deltaListeners.end()) {
        _deltaListeners[key] = listener;
        return true;
    }
    return false;
}

/**
 * Removes a gesture begin listener for the given object key
 *
 * If there is no active listener for the given key, this method fails
 * and returns false.
 *
 * This listener is invoked when the gesture is updated. A gesture is
 * only updated once it has a definitive type. Hence the first time
 * this listener is called after a start event, the gesture will have
 * an assigned type.
 *
 * @param key       The identifier for the begin listener
 *
 * @return true if the listener was succesfully removed
 */
bool CoreGesture::removeBeginListener(Uint32 key) {
    if (_startListeners.find(key) == _startListeners.end()) {
        _startListeners.erase(key);
        return true;
    }
    return false;
}

/**
 * Removes a gesture end listener for the given object key
 *
 * If there is no active listener for the given key, this method fails
 * and returns false.
 *
 * This listener is invoked when the gesture is updated. A gesture is
 * only updated once it has a definitive type. Hence the first time
 * this listener is called after a start event, the gesture will have
 * an assigned type.
 *
 * @param key       The identifier for the begin listener
 *
 * @return true if the listener was succesfully removed
 */
bool CoreGesture::removeEndListener(Uint32 key) {
    if (_endListeners.find(key) == _endListeners.end()) {
        _endListeners.erase(key);
        return true;
    }
    return false;
}

/**
 * Removes a gesture change listener for the given object key
 *
 * If there is no active listener for the given key, this method fails
 * and returns false.
 *
 * This listener is invoked when the gesture is updated. A gesture is
 * only updated once it has a definitive type. Hence the first time
 * this listener is called after a start event, the gesture will have
 * an assigned type.
 *
 * @param key       The identifier for the begin listener
 *
 * @return true if the listener was succesfully removed
 */
bool CoreGesture::removeChangeListener(Uint32 key) {
    if (_deltaListeners.find(key) == _deltaListeners.end()) {
        _deltaListeners.erase(key);
        return true;
    }
    return false;
}

#pragma mark -
#pragma mark Input Device
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
bool CoreGesture::updateState(const SDL_Event& event, const Timestamp& stamp) {
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
					processPan();
					processPinch();
					processSpin();
					_data.now = stamp;
					for(auto it = _deltaListeners.begin(); it != _deltaListeners.end(); ++it) {
						it->second(_data,_focus);
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
Vec2 CoreGesture::getScaledPosition(float x, float y) const {
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
void CoreGesture::queryEvents(std::vector<Uint32>& eventset) {
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
Vec2 CoreGesture::computeCentroid() const {
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
 * The axis is only defined when there are exactly two fingers on the
 * touch device. Naively, the axis is defined as the vector from the
 * first finger to the second.
 *
 * However, to enforce some stability, this method assumes the axis
 * always travels through the initial centroid of the gesture. As the
 * centroid may drift, this is not always the case.  To compensate,
 * this method computes the weighted average from the first finger
 * to the centroid, and from the centroid to the second finger.
 *
 * @return the axis of the fingers
 */
Vec2 CoreGesture::computeAxis() const {
    auto it = _fingers.begin();
    auto jt = _fingers.begin();
    ++jt;
    
    Vec2 v1 = it->second-_data.origPosition;
    Vec2 v2 = _data.origPosition-jt->second;
    
    float len = v1.length()+v2.length();
    v1.normalize();
    v2.normalize();
    v1 += v2;
    v1 *= len/2;

    return v1;
}

/**
 * Reinitializes the gesture event for a new gesture.
 *
 * This method calls all of the begin listeners with the new
 * gesture information.
 *
 * @param stamp The initial timestamp of the new gesture
 */
void CoreGesture::startGesture(const Timestamp& stamp) {
    _active = true;
    _data.type = CoreGestureType::NONE;
    _data.start = stamp;
    _data.now = stamp;
    _data.origPosition = computeCentroid();
    _data.currPosition = _data.origPosition;

    Vec2 axis = computeAxis();
    _data.origAngle = -axis.getAngle(); // Remember y reversal
    _data.currAngle = _data.origAngle;
    _data.origSpread = axis.length();
    _data.currSpread = _data.origSpread;

    for(auto it = _startListeners.begin(); it != _startListeners.end(); ++it) {
        it->second(_data,_focus);
    }
}

/**
 * Finalizes the gesture event, preparing for a new gesture.
 *
 * This method calls all of the end listeners with the final
 * gesture information.
 *
 * @param stamp The final timestamp of the gesture
 */
void CoreGesture::cancelGesture(const Timestamp& stamp) {
    _data.now = stamp;
    for(auto it = _endListeners.begin(); it != _endListeners.end(); ++it) {
        it->second(_data,_focus);
    }
    _data.clear();
    _data.start = stamp;
    _active = false;
}

/**
 * Processes a pan event.
 *
 * This method will update the position information of the current
 * gesture. It will do this regardless of the gesture type. However,
 * if the type is {@link CoreGestureType::NONE}, it will evaluate
 * this gesture to see if it can become a {@link CoreGestureType::PAN},
 * and update its type if so.
 */
void CoreGesture::processPan() {
    _data.currPosition = computeCentroid();
    if (_activePan && _data.type == CoreGestureType::NONE) {
        Vec2 dist = _data.currPosition-_data.origPosition;
        if (dist.lengthSquared() > _panThreshold*_panThreshold) {
            _data.type = CoreGestureType::PAN;
        }
    }
}

/**
 * Processes a pinch event.
 *
 * This method will update the pinch information of the current
 * gesture. It will do this regardless of the gesture type. However,
 * if the type is {@link CoreGestureType::NONE}, it will evaluate
 * this gesture to see if it can become a {@link CoreGestureType::PINCH},
 * and update its type if so.
 */
void CoreGesture::processPinch() {
    Vec2 axis = computeAxis();
    _data.currSpread = axis.length();
    if (_activePinch && _data.type == CoreGestureType::NONE) {
        float diff = _data.currSpread-_data.origSpread;
        if (diff > _pinchThreshold || diff < -_pinchThreshold) {
            _data.type = CoreGestureType::PINCH;
        }
    }
}

/**
 * Processes a spin event.
 *
 * This method will update the pinch information of the current
 * gesture. It will do this regardless of the gesture type. However,
 * if the type is {@link CoreGestureType::NONE}, it will evaluate
 * this gesture to see if it can become a {@link CoreGestureType::PINCH},
 * and update its type if so.
 */
void CoreGesture::processSpin() {
    Vec2 axis = computeAxis();
    _data.currAngle = -axis.getAngle(); // Remember y reversal
    if (_activeSpin && _data.type == CoreGestureType::NONE) {
        float radius = axis.length();
        float diff = _data.currAngle-_data.origAngle;
        if (radius >= _spinRadius && (diff > _spinThreshold || diff < -_spinThreshold)) {
            _data.type = CoreGestureType::SPIN;
        }
    }
}
