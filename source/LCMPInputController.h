//
//  LCMPInputController.h
//  Low Control Mall Patrol
//
//  Author: Kevin Games
//  Version: 2/20/22
//

#ifndef __LCMP_INPUT_CONTROLLER_H__
#define __LCMP_INPUT_CONTROLLER_H__
#include <cugl/cugl.h>

/** The event swipe length */
#define EVENT_SWIPE_LENGTH  100
/** Double tap threshold */
#define TAP_THRESHOLD 200

/**
 * The controller that handles all inputs to the device
 */
class InputController {
protected:
//  MARK: - Attributes
    
    /** The bounds of the entire game screen (in touch coordinates) */
    cugl::Rect _tbounds;
    /** The bounds of the entire game screen (in scene coordinates) */
    cugl::Rect _sbounds;
    /** The bounds of the left touch zone */
    cugl::Rect _lzone;
    /** The bounds of the right touch zone */
    cugl::Rect _rzone;
    
    /** Whether the player has swiped */
    bool _didSwipe;
    /** Whether the player has double tapped to switch characters */
    bool _didSwitch;
    /** Whether this input controller is active */
    bool _isActive;
    /** Whether the player has pressed the screen to use the joystick */
    bool _joystickPressed;
    /** The id of the finger that activated the joystick */
    cugl::TouchID _joystickID;
    /** The position of the outer portion of the joystick */
    cugl::Vec2 _joystickOrigin;
    /** The position of the inner portion of the joystick */
    cugl::Vec2 _joystickPosition;
    /** The vector that represents the direction that the player is trying to move */
    cugl::Vec2 _acceleration;
    /** The direction vector of the swipe */
    cugl::Vec2 _swipe;
    /** The offset for the accelerometer */
    cugl::Vec2 _accelOffset;

    /** Information representing a single "touch" (possibly multi-finger) */
    struct TouchInstance {
        /** The anchor touch position (on start) */
        cugl::Vec2 position;
        /** The current touch time */
        cugl::Timestamp timestamp;
        /** The touch id(s) for future reference */
        std::unordered_set<Uint64> touchids;
    };
    
    /** Enumeration identifying a zone for the current touch */
    enum class Zone {
        /** The touch was not inside the screen bounds */
        UNDEFINED,
        /** The touch was in the left zone (as shown above) */
        LEFT,
        /** The touch was in the right zone (as shown above) */
        RIGHT
    };
    
    /** The current touch location for the left zone */
    TouchInstance _ltouch;
    /** The current touch location for the right zone */
    TouchInstance _rtouch;
    /** The oldest touch location (used for cop tackle) */
    TouchInstance _mtouch;
    
public:
//  MARK: - Constructors
    /**
     * Populates the initial values of the input TouchInstance
     */
    void clearTouchInstance(TouchInstance& touchInstance);

    /**
     * Constructs an Input Controller
     */
    InputController();
    
    /**
     * Destructs an Input Controller
     */
    ~InputController() { dispose(); }
    
    /**
     * Disposes of all resources in this instance of Input Controller
     */
    void dispose();
    
    /**
     * Initializes an Input Controller
     */
    bool init(const cugl::Rect bounds);
    
//  MARK: - Detection
    
    /**
     * Updates the Input Controller
     */
    void update(float timestep);
    
    /**
     * Clears any buffered inputs so that we may start fresh.
    */
    void clear();
    
//  MARK: - Results
    
    /**
     Returns true iff the player switched characters
     */
    bool didSwitch() const { return _didSwitch; }
    
    /**
     Returns true iff the player swiped
     */
    bool didSwipe() const { return _didSwipe; }
    
    /**
     * Returns true iff the player is using the joystick
     */
    bool didPressJoystick() const { return _joystickPressed; }
    
    /**
     * Returns the origin of the outer portion of the joystick
     */
    cugl::Vec2 const getJoystickOrigin() { return _joystickOrigin; }
    
    /**
     * Returns the position of the inner portion of the joystick
     */
    cugl::Vec2 const getJoystickPosition() { return _joystickPosition; }
    
    /**
     * Returns the direction of the swipe
     */
    cugl::Vec2 const getSwipe() { return _swipe; }
    
    /**
     * Converts from touch screen coordinates to screen coordinates
     */
    cugl::Vec2 const touch2Screen(const cugl::Vec2 pos) const;
    
    /**
     * Returns the appropriate vector that determines where
     */
    cugl::Vec2 const getMovementVector(bool isThief) const;
    
//  MARK: - Callbacks
    
    /**
     * Callback for detecting that the player has pressed the touchscreen
     */
    void touchBeganCB(const cugl::TouchEvent& event, bool focus);
    
    /**
     * Callback for detecting that the player has released the touchscreen
     */
    void touchEndedCB(const cugl::TouchEvent& event, bool focus);
    
    /**
     * Callback for detecting that the player has dragged accross the touchscreen
     */
    void touchMovedCB(const cugl::TouchEvent& event,
                      const cugl::Vec2& previous, bool focus);
    
//  MARK: - Helpers
    
    /**
     * Initializes zones for inputs
     */
    void initZones();
    
    /**
     * Returns the correct zone for the given position.
     */
    Zone getZone(const cugl::Vec2 pos) const;
    
};

#endif /* __LCMP_INPUT_CONTROLLER_H__ */
