//
//  CUCoreGesture.h
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
#ifndef __CU_CORE_GESTURE_H__
#define __CU_CORE_GESTURE_H__

#include <cugl/input/CUInput.h>
#include <cugl/math/CUVec2.h>

namespace cugl {

#pragma mark CoreGestureEvent

/**
 * This enum represents a core gesture type.
 *
 * Core gestures are the primary two-finger gestures: pan, spin, and spread.
 * This simple enumeration indicates which gestures have been recognized.
 *
 * These types are currently exclusive (e.g. you cannot have a pan at the
 * same time as a pinch). This is because the purpose of the core gesture
 * device is to intelligently choose between gestures.
 */
enum class CoreGestureType : int {
    NONE  = 0,
    PAN   = 1,
    PINCH = 2,
    SPIN  = 3
};

/**
 * This simple class is a struct to hold a core gesture event.
 *
 * A core gesture is a gesture with duration. Hence this event stores
 * information about the start of the event, as well as the current
 * status of the event.
 *
 * A core gesture contains three bits of information: the position
 * of the fingers (computed as the average), the distance spread
 * between the fingers, and the current angle of the line between
 * the two fingers. All three values are continually updated
 * regardless of the gesture type.
 *
 * The gesture type is initially {@link CoreGestureType::NONE},
 * but transitions to one of the other types when a threshold is
 * passed. The threshold limits are defined in {@link CoreGesture}.
 *
 * The value of pan and pinch events is typically measured in pixels,
 * as it is assumed that the touch device is also the display (e.g.
 * most mobile devices). However, in situtations where that is not
 * the case (e.g. MacBook trackpad), the values will be measured
 * assuming the touch device is a unit square. In that case all pan
 * positions are (0,0) to (1,1), while all pinch spreads are 0 to
 * sqrt(2).
 */
class CoreGestureEvent {
public:
    /** The starting time of the gesture */
    Timestamp start;
    /** The current time of the gesture */
    Timestamp now;
    /** The gesture type */
    CoreGestureType type;
    /** The initial position of the gesture */
    Vec2 origPosition;
    /** The current position of the gesture */
    Vec2 currPosition;
    /** The initial finger separation of the gesture */
    float origSpread;
    /** The current finger separation of the gesture */
    float currSpread;
    /** The initial angle of the gesture */
    float origAngle;
    /** The current angle of the gesture */
    float currAngle;

    /**
     * Constructs a new gesture event with the default values
     */
    CoreGestureEvent() :
    origSpread(0.0f),
    currSpread(0.0f),
    origAngle(0.0f),
    currAngle(0.0f) {
        type = CoreGestureType::NONE;
    }
    
    /**
     * Constructs a new gesture event with the given timestamps
     *
     * @param start     The initial timestamp
     */
    CoreGestureEvent(const Timestamp& start) :
    origSpread(0.0f),
    currSpread(0.0f),
    origAngle(0.0f),
    currAngle(0.0f) {
        this->start = start;
        this->now = start;
        type = CoreGestureType::NONE;
    }

    /**
     * Constructs a new gesture event that is a copy of the given one
     *
     * @param event     The event to copy
     */
    CoreGestureEvent(const CoreGestureEvent& event) {
        start = event.start;
        now  = event.now;
        type = event.type;
        origPosition = event.origPosition;
        currPosition = event.origPosition;
        origSpread = event.origSpread;
        currSpread = event.currSpread;
        origAngle = event.origAngle;
        currAngle = event.currAngle;
    }
    
    /** 
     * Clears the contents of this gesture event
     */
    void clear() {
        type = CoreGestureType::NONE;
        origPosition.setZero();
        currPosition.setZero();
        origSpread = 0;
        currSpread = 0;
        origAngle = 0;
        currAngle = 0;
    }

};


#pragma mark -
#pragma mark CoreGesture
/**
 * This class is an input device recognizing the core two-finger gestures.
 *
 * There are three code two-finger gestures: pan (for translating a scene),
 * pinch (for zooming in or out of a scene), and spin (for rotating a scene).
 * We do have gesture inputs for each type: {@link PanGesture},
 * {@link PinchGesture}, and {@link SpinGesture}.  However, the problem with
 * using these inputs individually is that this would allow the user to
 * perform these gestures simultaneously. This is typically not what you
 * want, as it could cause the scene to rotate unstably while the user tries
 * to pan or pinch.
 *
 * Instead, most applications attempt to detect which of three gestures the
 * user is trying to perform and chose the best one. Once the gesture is
 * type is detected, the other gesture types will not be recognized until
 * the gesture is completed. Doing this requires combining all gestures into
 * a single input device, which is exactly what this class does.
 *
 * In order to destinguish between the three gesture types, this device has
 * several threshold factors that can be defined by the user.  For example,
 * a {@link CoreGestureType::PAN} is recognized when the cumulative distance
 * exceeds the {@link #getPanThreshold}. A {@link CoreGestureType::PINCH}
 * is recognized when the change in pinch distance exceeds
 * {@link #getPinchThreshold}. And a {@link CoreGestureType::SPIN} is
 * recognized when the change in angle exceeds {@link #getSpinThreshold}.
 * The priority is to recognize a pan before a pinch and a pinch before a
 * spin.
 *
 * By default all pan and pinch information will scale with the display,
 * and be measured in pixels. However, this assumes that the touch input
 * and the display are one and the same (e.g. most mobile devices). However,
 * there are some touch devices, like the MacBook trackpad, that cannot be
 * measure in pixels. For those types of devices, any pan or pinch will be
 * normalized to a unit square, where the top left corner of the touch
 * device is (0,0) and the lower right is (1,1).
 */
class CoreGesture : public InputDevice {
public:
    /**
     * @typedef Listener
     *
     * This type represents a listener for a two-finger gesture.
     *
     * In CUGL, listeners are implemented as a set of callback functions, not as
     * objects. This allows each listener to implement as much or as little
     * functionality as it wants. A listener is identified by a key which should
     * be a globally unique unsigned int.
     *
     * While gesture listeners do not traditionally require focus like a keyboard
     * does, we have included that functionality. While only one listener can have
     * focus at a time, all listeners will receive input from the CoreGesture device.
     *
     * The function type is equivalent to
     *
     *      std::function<void(const CoreGestureEvent& event, bool focus)>
     *
     * @param event     The touch event for this pinch/zoom
     * @param focus     Whether the listener currently has focus
     */
    typedef std::function<void(const CoreGestureEvent& event, bool focus)> Listener;
    
protected:
    /** Whether or not this input device is a touch screen */
    bool  _screen;
    /** Whether or not a gesture is currently being processed */
    bool  _active;
    /** The current core gesture information (continually updated) */
    CoreGestureEvent _data;

    /** Whether pan recognition is active */
    bool _activePan;
    /** Whether pinch recognition is active */
    bool _activePinch;
    /** Whether spin recognition is active */
    bool _activeSpin;

    /** The movement threshold for generating a pan event */
    float _panThreshold;
    /** The pinch distance threshold for generating a pinch event. */
    float _pinchThreshold;
    /** The angle threshold for generating a spin event. */
    float _spinThreshold;
    /** The minimum radius for a spin event */
    float _spinRadius;

    /** The current finger positions */
    std::unordered_map<Sint64, Vec2> _fingers;
    /** The number of fingers updated this pass */
    size_t _updated;
    
    /** The set of listeners called whenever a gesture begins */
    std::unordered_map<Uint32, Listener> _startListeners;
    /** The set of listeners called whenever a gesture updates */
    std::unordered_map<Uint32, Listener> _deltaListeners;
    /** The set of listeners called whenever a gesture ends */
    std::unordered_map<Uint32, Listener> _endListeners;

#pragma mark Constructor
    /**
     * Creates and initializes a new input device to detect core gestures.
     *
     * WARNING: Never allocate a gesture input device directly. Always use the
     * {@link Input#activate()} method instead.
     */
    CoreGesture();
    
    /**
     * Deletes this input device, disposing of all resources
     */
    virtual ~CoreGesture() {}
    
    /**
     * Initializes this device, acquiring any necessary resources
     *
     * @return true if initialization was successful
     */
    bool init() { return initWithName("Core Gestures"); }

    /**
     * Unintializes this device, returning it to its default state
     *
     * An uninitialized device may not work without reinitialization.
     */
    virtual void dispose() override;
   
#pragma mark Device Attributes
public:
    /**
     * Returns true if this device is a touch screen.
     *
     * This device is not guaranteed to be a touch screen. For example, the
     * trackpad on MacBooks support pinches. We do try to make our best guess
     * about whether or not a device is a touch screen, but on some devices
     * this may need to be set manually.
     *
     * If this value is true, all pan and pinch information will scale with
     * the display (and be measured in pixels) Otherwise, any pan or pinch
     * will be normalized to a unit square, where the top left corner of the
     * touch device is (0,0) and the lower right is (1,1).
     *
     * You may want to set this value to false for true cross-platform gesture
     * support.
     *
     * @return true if this device is a touch screen.
     */
    bool isTouchScreen() const { return _screen; }

    /**
     * Sets whether this device is a touch screen.
     *
     * This device is not guaranteed to be a touch screen.  For example, the
     * trackpad on MacBooks support pinches. We do try to make our best guess
     * about whether or not a device is a touch screen, but on some devices
     * this may need to be set manually.
     *
     * If this value is true, all pan and pinch information will scale with
     * the display (and be measured in pixels) Otherwise, any pan or pinch
     * will be normalized to a unit square, where the top left corner of the
     * touch device is (0,0) and the lower right is (1,1).
     *
     * You may want to set this value to false for true cross-platform gesture
     * support.
     *
     * @param flag  Whether this device is a touch screen.
     */
    void setTouchScreen(bool flag);
    
    /**
     * Returns true if pan detection is active.
     *
     * If pan detection is not active, all pan events will be ignored. By
     * default, this value is true.
     *
     * @return true if pan detection is active.
     */
    bool isPanActive() const { return _activePan; }
    
    /**
     * Sets whether pan detection is active.
     *
     * If pan detection is not active, all pan events will be ignored. By
     * default, this value is true.
     *
     * @param flag  Whether pan detection is active.
     */
    void setPanActive(bool flag) { _activePan = flag; }

    /**
     * Returns true if pinch detection is active.
     *
     * If pan detection is not active, all pan events will be ignored. By
     * default, this value is true.
     *
     * @return true if pinch detection is active.
     */
    bool isPinchActive() const { return _activePinch; }
    
    /**
     * Sets whether pinch detection is active.
     *
     * If pan detection is not active, all pinch events will be ignored. By
     * default, this value is true.
     *
     * @param flag  Whether pinch detection is active.
     */
    void setPinchActive(bool flag) { _activePinch = flag; }

    /**
     * Returns true if spin detection is active.
     *
     * If pan detection is not active, all spin events will be ignored. By
     * default, this value is true.
     *
     * @return true if spin detection is active.
     */
    bool isSpinActive() const { return _activeSpin; }
    
    /**
     * Sets whether pinch detection is active.
     *
     * If pan detection is not active, all spin events will be ignored. By
     * default, this value is true.
     *
     * @param flag  Whether spin detection is active.
     */
    void setSpinActive(bool flag) { _activeSpin = flag; }
    
    /**
     * Returns the distance threshold for pan events.
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
     * @return the distance threshold for pan events.
     */
    float getPanThreshold() const { return _panThreshold; }

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
    void setPanThreshold(float threshold);

    /**
     * Returns the distance threshold for pinch events.
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
     * @return the distance threshold for pinch events.
     */
    float getPinchThreshold() const { return _pinchThreshold; }

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
    void setPinchThreshold(float threshold);

    /**
     * Returns the angle threshold for spin events.
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
     * The default value is 4 degrees.
     *
     * @return the angle threshold for spin events.
     */
    float getSpinThreshold() const { return _spinThreshold; }

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
     * The default value is 4 degrees.
     *
     * @param threshold The angle threshold for spin events.
     */
    void setSpinThreshold(float threshold);
    
    /**
     * Returns the minimum radius for a spin event.
     *
     * All spins have an additional rquirement that all the fingers must be
     * separated by a minimum distance. This is a natural requirement for
     * spins, and it greatly reduces the possibility of accidental spins.
     *
     * If this device is a touch screen, this value should be measured in pixels.
     * Otherwise, this value should be set assuming a unit square, where the
     * top left corner of the touch device is (0,0) and the lower right is (1,1).
     * By default this value is 10% of the length of the diagonal of the touch
     * device.
     *
     * @return the minimum radius for a spin event.
     */
    float getSpinRadius() const { return _spinRadius; }

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
     * By default this value is 10% of the length of the diagonal of the touch
     * device.
     *
     * @param radius    The minimum radius for a spin event.
     */
    void setSpinRadius(float radius);
    
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
    virtual bool requestFocus(Uint32 key) override;
    
    /**
     * Returns true if key represents a listener object
     *
     * An object is a listener if it is a listener for any of the three actions:
     * gesture begin, gesture end, or gesture change.
     *
     * @param key   The identifier for the listener
     *
     * @return true if key represents a listener object
     */
    bool isListener(Uint32 key) const;

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
    const Listener getBeginListener(Uint32 key) const;

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
    const Listener getEndListener(Uint32 key) const;

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
    const Listener getChangeListener(Uint32 key) const;

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
    bool addBeginListener(Uint32 key, Listener listener);

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
    bool addEndListener(Uint32 key, Listener listener);
    
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
    bool addChangeListener(Uint32 key, Listener listener);

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
    bool removeBeginListener(Uint32 key);
    
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
    bool removeEndListener(Uint32 key);

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
    bool removeChangeListener(Uint32 key);

protected:
#pragma mark Input Device
    /**
     * Clears the state of this input device, readying it for the next frame.
     *
     * Many devices keep track of what happened "this" frame. This method is
     * necessary to advance the frame.
     */
    virtual void clearState() override {
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
    virtual bool updateState(const SDL_Event& event, const Timestamp& stamp) override;

    /**
     * Returns the scale/unscaled touch position.
     *
     * The value returned depends on the value of attribute _screen. If this
     * attribute is false, the position is normalized to the unit square.
     * Otherwise it is scaled to the touch screen.
     *
     * @return the scale/unscaled touch position.
     */
    Vec2 getScaledPosition(float x, float y) const;
    
    /**
     * Determine the SDL events of relevance and store there types in eventset.
     *
     * An SDL_EventType is really a Uint32. This method stores the SDL event
     * types for this input device into the vector eventset, appending them
     * to the end. The Input dispatcher then uses this information to set up
     * subscriptions.
     *
     * @param eventset  The set to store the event types.
     */
    virtual void queryEvents(std::vector<Uint32>& eventset) override;

#pragma mark Internal Helpers
private:
    /**
     * Returns the centroid of the fingers
     *
     * The centroid is the average of all the fingers on the touch device.
     *
     * @return the centroid of the fingers
     */
    Vec2 computeCentroid() const;

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
    Vec2 computeAxis() const;

    /**
     * Reinitializes the gesture event for a new gesture.
     *
     * This method calls all of the begin listeners with the new
     * gesture information.
     *
     * @param stamp The initial timestamp of the new gesture
     */
    void startGesture(const Timestamp& stamp);

    /**
     * Finalizes the gesture event, preparing for a new gesture.
     *
     * This method calls all of the end listeners with the final
     * gesture information.
     *
     * @param stamp The final timestamp of the gesture
     */
    void cancelGesture(const Timestamp& stamp);

    /**
     * Processes a pan event.
     *
     * This method will update the position information of the current
     * gesture. It will do this regardless of the gesture type. However,
     * if the type is {@link CoreGestureType::NONE}, it will evaluate
     * this gesture to see if it can become a {@link CoreGestureType::PAN},
     * and update its type if so.
     */
    void processPan();

    /**
     * Processes a pinch event.
     *
     * This method will update the pinch information of the current
     * gesture. It will do this regardless of the gesture type. However,
     * if the type is {@link CoreGestureType::NONE}, it will evaluate
     * this gesture to see if it can become a {@link CoreGestureType::PINCH},
     * and update its type if so.
     */
    void processPinch();

    /**
     * Processes a spin event.
     *
     * This method will update the pinch information of the current
     * gesture. It will do this regardless of the gesture type. However,
     * if the type is {@link CoreGestureType::NONE}, it will evaluate
     * this gesture to see if it can become a {@link CoreGestureType::PINCH},
     * and update its type if so.
     */
    void processSpin();
    
    // Apparently friends are not inherited
    friend class Input;
};
    
}
#endif /* __CU_CORE_GESTURE_H__ */
