//
//  CUTextInput.h
//  Cornell University Game Library (CUGL)
//
//  This class is an object-oriented interface to the SDL text input system.
//  We have tried to keep this class as minimal as possible to make it as
//  flexible as possible. Previous versions of this class did not accurately
//  reflect SDL text input.
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
//  Version: 8/1/21
//
#ifndef __CU_TEXT_INPUT_H__
#define __CU_TEXT_INPUT_H__
#include <cugl/input/CUInput.h>
#include <cugl/util/CUTimestamp.h>
#include <functional>

namespace cugl {
    
#pragma mark TextInputEvent
    
/**
 * This simple class is a struct to hold text input information
 *
 * Text input is sent one unicode character at a time. This is true even
 * when this is not how the OS handles it (e.g. macOS and Pinyin-Simplified).
 * When the input sends multiple characters, they are transmitted as a
 * sequence of TextInputEvents with ordered time stamps.
 */
class TextInputEvent {
public:
    /** The time of the text input event */
    Timestamp timestamp;
    /** The unicode character for this input */
    Uint32 unicode;
    
    /**
     * Constructs a new text input event with the default values
     */
    TextInputEvent() : unicode(0) {}

    /**
     * Constructs a new text input event with the given unicode value
     *
     * The timestamp will be the time of construction.
     *
     * @param code      The unicode character
     */
    TextInputEvent(Uint32 code);

    /**
     * Constructs a new text input event with the given values
     *
     * @param code      The unicode character
     * @param stamp     The timestamp for the event
     */
    TextInputEvent(Uint32 code, const Timestamp& stamp);

    /**
     * Deletes a text input event, releasing all resources
     */
    ~TextInputEvent() {}
};

/**
 * This simple class is a struct to hold text editing information
 *
 * Text input does not necessarily correspond to a single keystroke. Some
 * unicode characters are the results of intermediate keystrokes. Examples
 * include extended Latin characters like ü, or Chinese characters created
 * by Pinyin - Simplified on macOS. This event allows an application to
 * look at the keystroke buffer before it resolves into a unicode character.
 */
class TextEditEvent {
public:
    /** The time of the text edit event */
    Timestamp timestamp;
    /** The edit buffer for the intermediate text  */
    std::string buffer;
    /** The beginning position of the change */
    size_t begin;
    /** The end (not inclusive) position of the change */
    size_t end;

    /**
     * Constructs a new text edit event with the default values
     */
    TextEditEvent() : begin(0), end(0) {}
    
    /**
     * Constructs a new text edit event with the given buffer and edit positions.
     *
     * The timestamp will be the time of construction.
     *
     * @param text      The current text editing buffer
     * @param begin     The beginning edit position in the buffer
     * @param end       The ending (not inclusive) edit position
     */
    TextEditEvent(const std::string text, size_t begin, size_t end);
    
    /**
     * Constructs a new text edit event with the given values
     *
     * @param text      The current text editing buffer
     * @param begin     The beginning edit position in the buffer
     * @param end       The ending (not inclusive) edit position
     * @param stamp     The timestamp for the event
     */
    TextEditEvent(const std::string text, size_t begin, size_t end, const Timestamp& stamp);

    /**
     * Deletes a text input event, releasing all resources
     */
    ~TextEditEvent() {}
};

#pragma mark -
/**
 * This class is a service that extracts UTF8 text from typing.
 *
 * You never want to use a keyboard device to gather text. That is because
 * unicode characters can correspond to several keystrokes. This device
 * abstracts this process, to make it easier to gather text for password
 * fields, text boxes, or the like.
 *
 * This class is an object-oriented abstraction build on top of the SDL
 * Text Input API.  For a tutorial of this API see
 *
 *      https://wiki.libsdl.org/Tutorials/TextInput
 *
 * While this class abstracts aways the SDL calls, the process remains the
 * same. First you start a text input sequence with {@link begin()}. All
 * input is sent via a {@link TextInputEvent} to the appropriate listeners
 * as soon as the input resolves. Unlike SDL, we guarantee that input is
 * sent one unicode character at a time, in the order that the unicode is
 * processed.
 *
 * Like SDL it is also possible to attach listeners to the editing process.
 * Some characters may involve typing multiple keystrokes before the input
 * resolves. This includes extended Latin characters like ü, or Chinese
 * characters created by Pinyin - Simplified on macOS. This is in case you
 * would like to give the user visual feedback on the intermediate editing
 * process.
 *
 * Listeners are guaranteed to be called at the start of an animation frame,
 * before the method {@link Application#update(float)} is called.
 *
 * Unlike {@link Keyboard}, this class is fine to use with mobile devices.
 * On many devices, calling the method {@link begin()} will create a virtual
 * keyboard to input text.
 */
class TextInput : public InputDevice {
public:
#pragma mark Listeners
    /**
     * @typedef InputListener
     *
     * This type represents an input listener for the {@link TextInput} class.
     *
     * In CUGL, listeners are implemented as a set of callback functions, not
     * as objects. This allows each listener to implement as much or as little
     * functionality as it wants. A listener is identified by a key which should
     * be a globally unique unsigned int.
     *
     * This listener is called whenever a unicode character resolves as input.
     * A TextInput is designed to send input to a focused object (e.g. a text
     * field or other UI widget). While only one listener can have focus at a
     * time, all input listeners will be invoked by the TextInput.
     *
     * Listeners are guaranteed to be called at the start of an animation frame,
     * before the method {@link Application#update(float) }.
     *
     * The function type is equivalent to
     *
     *      std::function<void(const TextInputEvent& event, bool focus)>
     *
     * @param event     The input event for this append to the buffer
     * @param focus     Whether the listener currently has focus
     */
    typedef std::function<void(const TextInputEvent& event, bool focus)> InputListener;

    /**
     * @typedef EditListener
     *
     * This type represents an editing listener for the {@link TextInput} class.
     *
     * In CUGL, listeners are implemented as a set of callback functions, not
     * as objects. This allows each listener to implement as much or as little
     * functionality as it wants. A listener is identified by a key which should
     * be a globally unique unsigned int.
     *
     * While a TextInput is primarily designed to send Unicode characters, some
     * characters are the result of multiple keystrokes. This includes extended
     * Latin characters like ü, or characters created by Pinyin - Simplified on
     * macOS. Editing listeners intercept these intermediate keystrokes before
     * the input resolved as a unicode character.
     *
     * Unlike text input, editing input
     * A TextInput is designed to send input to a focused object (e.g. a text
     * field or other UI widget). While only one listener can have focus at a
     * time, all edit listeners will be invoked by the TextInput.

     * Listeners are guaranteed to be called at the start of an animation frame,
     * before the method {@link Application#update(float) }.
     *
     * The function type is equivalent to
     *
     *      std::function<void(const TextEditEvent& event, bool focus)>
     *
     * @param event     The input event for this append to the buffer
     * @param focus     Whether the listener currently has focus
     */
    typedef std::function<void(const TextEditEvent& event, bool focus)> EditListener;
    
#pragma mark Values
protected:
    /** Whether the input device is actively receiving text input */
    bool _active;
    
    /** The set of input listeners called whenever we resolve a character */
    std::unordered_map<Uint32, InputListener> _inputListeners;
    /** The set of edit listeners called for intermediate keystrokes */
    std::unordered_map<Uint32, EditListener> _editListeners;

#pragma mark -
#pragma mark Constructor
    /**
     * Creates and initializes a new text input device.
     *
     * WARNING: Never allocate a text input device directly. Always use the
     * {@link Input#activate()} method instead.
     */
    TextInput();

    /**
     * Deletes this input device, disposing of all resources
     */
    virtual ~TextInput() {}
    
    /**
     * Initializes this device, acquiring any necessary resources
     *
     * @return true if initialization was successful
     */
    bool init() { return initWithName("Text Input"); }
    
    /**
     * Unintializes this device, returning it to its default state
     *
     * An uninitialized device may not work without reinitialization.
     */
    virtual void dispose() override;

public:
#pragma mark -
#pragma mark Activation
    /**
     * Start accepting text with this device
     *
     * Until this method is called, no input will ever resolve (though the key
     * strokes may still be detected by the {@link Keyboard device}). Once the
     * method is called, input will continue resolve until the method
     * {@link end()} is called.
     *
     * This device maintains no internal state. All input is communicated
     * immediately to the listeners as soon as it resolves.
     */
    void begin();
    
    /**
     * Stop accepting text with this device
     *
     * Once the method is called, no more input will resolve (though the key
     * strokes may still be detected by the {@link Keyboard device}).
     */
    void end();

    /**
     * Returns true if this device is actively receiving input.
     *
     * This method will return true after {@link begin()} is called, but
     * before {@link end()} is called.
     *
     * @return true if this device is actively receiving input.
     */
    bool isActive() const { return _active; }
    
#pragma mark -
#pragma mark Listeners
    /**
     * Requests focus for the given identifier
     *
     * Only an active listener can have focus. This method returns false if
     * the key does not refer to an active listener (of any type). Note that
     * keys may be shared across listeners of different types, but must be
     * unique for each listener type.
     *
     * @param key   The identifier for the focus object
     *
     * @return false if key does not refer to an active listener
     */
    virtual bool requestFocus(Uint32 key) override;

    /**
     * Returns true if key represents a listener object
     *
     * An object is a listener if it is a listener for either editing or input.
     *
     * @param key   The identifier for the listener
     *
     * @return true if key represents a listener object
     */
    bool isListener(Uint32 key) const;
    
    /**
     * Returns the text input listener for the given object key
     *
     * If there is no listener for the given key, it returns nullptr.
     *
     * This listener is invoked when input resolves to a unicode character.
     *
     * @param key   The identifier for the listener
     *
     * @return the text input listener for the given object key
     */
    const InputListener getInputListener(Uint32 key) const;
    
    /**
     * Returns the text editing listener for the given object key
     *
     * If there is no listener for the given key, it returns nullptr.
     *
     * This listener is invoked when the text input has received keystrokes
     * starting a unicode character, but the character has not yet resolved.
     *
     * @param key   The identifier for the listener
     *
     * @return the text editing listener for the given object key
     */
    const EditListener getEditListener(Uint32 key) const;
    
    /**
     * Adds a text input listener for the given object key
     *
     * There can only be one input listener for a given key (though you may
     * share keys across other listener types). If a listener already exists
     * for the key, the method will fail and return false. You must remove
     * a listener before adding a new one for the same key.
     *
     * This listener is invoked when input resolves to a unicode character.
     *
     * @param key   	The identifier for the input listener
     * @param listener	The listener to add
     *
     * @return true if the listener was succesfully added
     */
    bool addInputListener(Uint32 key, InputListener listener);

    /**
     * Adds a text editing listener for the given object key
     *
     * There can only be one edit listener for a given key (though you may
     * share keys across other listener types). If a listener already exists
     * for the key, the method will fail and return false. You must remove
     * a listener before adding a new one for the same key.
     *
     * This listener is invoked when the text input has received keystrokes
     * starting a unicode character, but the character has not yet resolved.
     *
     * @param key       The identifier for the edit listener
     * @param listener  The listener to add
     *
     * @return true if the listener was succesfully added
     */
    bool addEditListener(Uint32 key, EditListener listener);

    /**
     * Removes the text input listener for the given object key
     *
     * If there is no active listener for the given key, this method fails and
     * returns false.
     *
     * This listener is invoked when input resolves to a unicode character.
     *
     * @param key   The identifier for the listener
     *
     * @return true if the listener was succesfully removed
     */
    bool removeInputListener(Uint32 key);
    
    /**
     * Removes the text edit listener for the given object key
     *
     * If there is no active listener for the given key, this method fails and
     * returns false.
     *
     * This listener is invoked when the text input has received keystrokes
     * starting a unicode character, but the character has not yet resolved.
     *
     * @param key   The identifier for the listener
     *
     * @return true if the listener was succesfully removed
     */
    bool removeEditListener(Uint32 key);

#pragma mark -
#pragma mark Input Device
    /**
     * Clears the state of this input device, readying it for the next frame.
     *
     * Many devices keep track of what happened "this" frame.  This method is
     * necessary to advance the frame.
     */
    virtual void clearState() override {}

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
    
    // Apparently friends are not inherited
    friend class Input;
};

}

#endif /* __CU_TEXT_INPUT_H__ */
