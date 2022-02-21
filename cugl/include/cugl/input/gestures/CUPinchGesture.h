//
//  CUPinchGesture.h
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
#ifndef __CU_PINCH_INPUT_H__
#define __CU_PINCH_INPUT_H__
#include <cugl/input/CUInput.h>
#include <cugl/math/CUVec2.h>

namespace cugl {

#pragma mark -
#pragma mark PinchEvent

/**
 * This simple class is a struct to hold a pinch event.
 *
 * A pinch event is a gesture with duration. Hence this event stores
 * information about the start of the event, as well as the current
 * status of the event.
 */

class PinchEvent {
public:
    /** The starting time of the gesture */
    Timestamp start;
    /** The current time of the gesture */
    Timestamp now;
    /** The normalized center of this pinch */
    Vec2 anchor;
    /** The initial finger separation of the gesture */
    float origSpread;
    /** The current finger separation of the gesture */
    float currSpread;
    /** The pinch delta since the last animation frame */
    float delta;
    
    /**
     * Constructs a new touch event with the default values
     */
    PinchEvent() :
        origSpread(0.0f),
        currSpread(0.0f),
        delta(0.0f) {}
    
    /**
     * Constructs a new pinch event with the given values
     *
     * @param stamp     The timestamp for the event
     * @param anchor    The normalized pinch center
     * @param distance  The distance between the two fingers
     */
    PinchEvent(const Timestamp& stamp, const Vec2 anchor, float distance) {
        this->start = stamp;
        this->now = stamp;
        this->anchor = anchor;
        origSpread = distance;
        currSpread = distance;
        delta = 0.0f;
    }
    
    /**
     * Constructs a new pinch event that is a copy of the given one
     *
     * @param event     The event to copy
     */
    PinchEvent(const PinchEvent& event) {
        start = event.start;
        now  = event.now;
        anchor = event.anchor;
        origSpread = event.origSpread;
        currSpread = event.currSpread;
        delta = event.delta;
    }
    
    /**
     * Clears the contents of this pinch event
     */
    void clear() {
        anchor.setZero();
        origSpread = 0.0f;
        currSpread = 0.0f;
        delta = 0.0f;
    }
};


#pragma mark -
#pragma mark PinchGesture

/**
 * This class is an input device recognizing pinch/zoom events.
 *
 * A pinch is a gesture where two fingers are pulled apart or brought
 * closer together.  Technically the latter is a pinch while the former is
 * a zoom. However, most UX designers lump these two gestures together.
 * While some platforms allow pinches with more than two fingers, currently
 * CUGL is limited to two-finger pinches. Multi-finger pinches are a
 * candidate for a future CUGL release.
 *
 * This input device is a touch device that supports multitouch gestures.
 * This is often the screen itself, but this is not always guaranteed.  For
 * example, the trackpad on MacBooks support pinches. For that reason, we
 * cannot guarantee that the touches scale with the display. Instead, all
 * gesture information is normalized, with the top left corner of the touch
 * device being (0,0) and the lower right being (1,1).
 *
 * If you know that the touch device is the screen, and would like to measure
 * the pinch in screen coordinates, you should set the screen attribute to
 * true with {@link setTouchScreen}. In this case, the pinch distance will
 * be scaled according to the display. In those cases where the device is
 * known to be the screen (Android, iOS devices), this value starts out
 * as true.
 *
 * As with most devices, we provide support for both listeners and polling the
 * mouse.  Polling the device will query the touch screen at the start of the
 * frame, but it may miss those case in there are multiple pinch changes in a
 * single animation frame.
 *
 * Listeners are guaranteed to catch all changes in the pinch size, as long as
 * they are detected by the OS.  However, listeners are not called as soon as
 * the event happens.  Instead, the events are queued and processed at the start
 * of the animation frame, before the method {@link Application#update(float)}
 * is called.
 */
class PinchGesture : public InputDevice {
public:
#pragma mark Listener
    /**
     * @typedef Listener
     *
     * This type represents a listener for a pinch/zoom in the {@link PinchGesture} class.
     *
     * In CUGL, listeners are implemented as a set of callback functions, not as
     * objects. This allows each listener to implement as much or as little
     * functionality as it wants. A listener is identified by a key which should
     * be a globally unique unsigned int.
     *
     * While pinch listeners do not traditionally require focus like a keyboard does,
     * we have included that functionality. While only one listener can have focus
     * at a time, all listeners will receive input from the PinchInput device.
     *
     * The function type is equivalent to
     *
     *      std::function<void(const PinchEvent& event, bool focus)>
     *
     * @param event     The touch event for this pinch/zoom
     * @param focus     Whether the listener currently has focus
     */
    typedef std::function<void(const PinchEvent& event, bool focus)> Listener;

protected:
    /** Whether or not this input device is a touch screen */
    bool  _screen;
    /** Whether or not there is an active pinch being processed */
    bool  _active;
    /** The movement stability for canceling a pinch event */
    float _stability;
    /** The pinch event data (stored whether or not there is an event) */
    PinchEvent _event;
    
    /** The current finger positions */
    std::unordered_map<Sint64, Vec2> _fingers;
    /** The number of fingers updated this pass */
    size_t _updated;

    /** The set of listeners called whenever a pinch begins */
    std::unordered_map<Uint32, Listener> _beginListeners;
    /** The set of listeners called whenever a pinch ends */
    std::unordered_map<Uint32, Listener> _finishListeners;
    /** The set of listeners called whenever a pinch is moved */
    std::unordered_map<Uint32, Listener> _changeListeners;


#pragma mark Constructor
    /**
     * Creates and initializes a new pinch input device.
     *
     * WARNING: Never allocate a pinch input device directly.  Always use the
     * {@link Input#activate()} method instead.
     */
    PinchGesture();
    
    /**
     * Deletes this input device, disposing of all resources
     */
    virtual ~PinchGesture() {}
    
    /**
     * Initializes this device, acquiring any necessary resources
     *
     * @return true if initialization was successful
     */
    bool init() { return initWithName("Pinch Gesture"); }

    
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
    bool isTouchScreen() const { return _screen; }

    /**
     * Sets whether this device is a touch screen.
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
     * @param flag  Whether this device is a touch screen.
     */
    void setTouchScreen(bool flag);
    
    /**
     * Returns the movement stability of a pinch event.
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
     * @return the movement stability of a pinch event.
     */
    float getStability() const { return _stability; }
    
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
    void setStability(float stability);
    
#pragma mark Data Polling
    /**
     * Returns true if the device is in the middle of an active pinch.
     *
     * If the device is not an in active pinch, all other polling methods 
     * will return the default value.
     *
     * @return true if the device is in the middle of an active pinch.
     */
    bool isActive() const { return _active; }
    
    /**
     * Returns the change in the pinch distance since the last animation frame.
     *
     * This value is positive if the pinch is a zoom, and negative if it is
     * a true pinch.
     *
     * @return the change in the pinch distance since the last animation frame.
     */
    float getDelta() const { return _active ? _event.delta : 0.0f; }
    
    /**
     * Returns the cumulative pinch distance since the gesture began.
     *
     * This value is positive if the pinch is a zoom, and negative if it is
     * a true pinch.  A pinch can both zoom and pinch in a single gesture.
     *
     * @return the cumulative pinch distance since the gesture began.
     */
    float getPinch() const;
    
    /**
     * Returns the normalized center of the pinch.
     *
     * This value is defined at the start of the pinch gesture and remains
     * unchanged.
     *
     * @return the normalized center of the pinch.
     */
    const Vec2 getPosition() const { return _active ? _event.anchor : Vec2::ZERO; }
    
    
#pragma mark Listeners
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
    virtual bool requestFocus(Uint32 key) override;
    
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
    bool isListener(Uint32 key) const;
    
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
    const Listener getBeginListener(Uint32 key) const;
    
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
    const Listener getEndListener(Uint32 key) const;
    
    /**
     * Returns the pinch change listener for the given object key
     *
     * This listener is invoked when the pinch distance changes.
     *
     * @param key   The identifier for the listener
     *
     * @return the pinch change listener for the given object key
     */
    const Listener getChangeListener(Uint32 key) const;
    
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
    bool addBeginListener(Uint32 key, Listener listener);
    
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
    bool addEndListener(Uint32 key, Listener listener);
    
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
    bool addChangeListener(Uint32 key, Listener listener);
    
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
    bool removeBeginListener(Uint32 key);
    
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
    bool removeEndListener(Uint32 key);
    
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
    bool removeChangeListener(Uint32 key);

    
protected:
#pragma mark Input Device
    /**
     * Clears the state of this input device, readying it for the next frame.
     *
     * Many devices keep track of what happened "this" frame.  This method is
     * necessary to advance the frame.
     */
    virtual void clearState() override;
    
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
     * The value returned depends on the value of attribute _screen.  If this
     * attribute is false, the position is normalized to the unit square.
     * Otherwise it is scaled to the touch screen.
     *
     * @return the scale/unscaled touch position.
     */
    Vec2 getScaledPosition(float x, float y) const;
    
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
    Vec2 computeAxis() const;

    /**
     * Reinitializes the pinch event for a new gesture.
     *
     * This method calls all of the begin listeners with the new
     * gesture information.
     *
     * @param stamp The initial timestamp of the new gesture
     */
    void startGesture(const Timestamp& stamp);

    /**
     * Finalizes the pinch event, preparing for a new gesture.
     *
     * This method calls all of the end listeners with the final
     * gesture information.
     *
     * @param stamp The final timestamp of the gesture
     */
    void cancelGesture(const Timestamp& stamp);
    
    // Apparently friends are not inherited
    friend class Input;
};
    
}
#endif /* __CU_PINCH_INPUT_H__ */
