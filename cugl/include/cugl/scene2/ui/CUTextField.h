//
//  CUTextField.h
//  Cornell University Game Library (CUGL)
//
//  This module provides support for a single line text field. It is useful
//  for providing input forms for your application, such as saved games or
//  player settings.  Because it is only a single line, it is a subclass of
//  label.  A multiline text input would be a TextArea, and that is not
//  currently supported.
//
//  To make use of a TextField, BOTH Keyboard and TextInput input devices
//  must be activated.  In particular, TextInput allows the support of
//  virtual keyboards on mobile devices.
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
//  Author: Walker White and Enze Zhou
//  Version: 8/20/20
//
#ifndef __CU_TEXT_FIELD_H__
#define __CU_TEXT_FIELD_H__

#include <cugl/scene2/ui/CULabel.h>
#include <cugl/input/CUTextInput.h>
#include <cugl/input/CUKeyboard.h>
#include <string>

namespace cugl {
    /**
     * The classes to construct an 2-d scene graph.
     *
     * This namespace was chosen to future-proof the game engine. We will
     * eventually want to add 3-d scene graphs as well, and this namespace
     * will prevent any collisions with those scene graph nodes.
     */
    namespace scene2 {

#pragma mark -
#pragma mark TextField

/**
 * This class represents a single line text field.
 *
 * A text field is a subclass of {link Label} that permits the user to edit
 * it when focused (e.g. when it is selected to receive keyboard events).
 * A focused field displays a blinking cursor with the current edit position.
 * There is no cursor displayed when the field does not have focus.
 *
 * The textfield can track its own state, via the {@link #activate()} method,
 * relieving you of having to manually the keyboard.  In addition, it also
 * reponds to mouse/touch input, allowing you to reposition the cursor and
 * either focus or unfocus the text field. However, the appropriate input
 * devices must be active before you can activate the text fields, as it needs
 * to attach internal listeners.  These devices include {@link TextInput},
 * {@link Keyboard}, and either {@link Mouse} or {@link Touchscreen}.
 *
 * The text field supports two category of listeners.  The first tracks any
 * changes to the text.  The second only updates when the field loses focus,
 * such as when the user presses return.
 *
 * As with {@link Label}, a text field is able to support multiline text. In
 * addition, the user can navigate this text with the arrow keys or by using
 * the mouse/touch to reposition the cursor.  With that said, this class is
 * is designed for small-to-medium sized segments of text. It is not designed
 * to be an all-purpose text editor for managing large strings. That is
 * because every single edit (no matter how small) will reformat the entire
 * text.
 */
class TextField : public Label {
public:
#pragma mark TextFieldListener
    
    /**
     * @typedef Listener
     *
     * This type represents a listener for text change in the {@link TextField} class.
     *
     * In CUGL, listeners are implemented as a set of callback functions, not as
     * objects. This allows each listener to implement as much or as little
     * functionality as it wants. For simplicity, TextField nodes only support a
     * single listener. If you wish for more than one listener, then your listener
     * should handle its own dispatch.
     *
     * The function type is equivalent to
     *
     *      std::function<void (const std::string& name, const std::string& current)>
     *
     * @param name          The text field name
     * @param current       Text after editing
     */
    typedef std::function<void (const std::string& name, const std::string& current)> Listener;
    
protected:
#pragma mark -
#pragma mark Values
    /** The current number of characters in the input */
    size_t _charsize;
    
    // Cursor managment
    /** The current cursor rectangle, */
    Rect _cursor;
    /** Timer for blinking the cursor. */
    int _cursorBlink;
    /** Cursor position indexed from the start of the text. 0 (before) start. */
    size_t _cursorIndex;
    /** Whether to show the cursor (as opposed to just tracking the position) */
    bool   _showCursor;
    /** The width of the cursor rectangle */
    float  _cursorWidth;
    /** The current cursor color */
    Color4 _cursorColor;
    
    // Listener Management
    /** Whether the field is actively checking for state changes */
    bool _active;
    /** Whether the field is actively receiving keyboad events */
    bool _focused;
    /** Whether we are using the mouse (as opposed to the touch screen) */
    bool _mouse;
    /** The (master) text input key when the text field is checking for events */
	Uint32 _tkey;
    /** The (master) keyboard key when the text field is checking for events */
    Uint32 _kkey;
    /** The (master) focus key when the text field is checking for events */
    Uint32 _fkey;
    /** The key distributer for user-level listeners */
    Uint32 _nextKey;
    /** The listener callbacks for text changes */
	std::unordered_map<Uint32,Listener> _typeListeners;
    /** The listener callbacks for loss of focus */
    std::unordered_map<Uint32,Listener> _exitListeners;

    // Keystroke Management
	/** Whether the Alt key is down (used for word level editing) */
	bool _altDown;
	/** Whether the Meta key is down (used for line level editing) */
	bool _metaDown;
    /** Whether the Shift key is down (used for line level editing) */
    bool _shiftDown;
    /** A timer to safely implement key hold-downs */
    size_t _keyCount;

public:
#pragma mark -
#pragma mark Constructors
    /**
     * Creates an uninitialized text field with no size or font.
     *
     * You must initialize this field before use.
     *
     * NEVER USE A CONSTRUCTOR WITH NEW. If you want to allocate a Node on the
     * heap, use one of the static constructors instead.
     */
    TextField();

    /**
     * Deletes this text field, disposing all resources
     *
     * It is unsafe to call this on a text field that is still currently
     * inside of a scene graph.
     */
	~TextField() { dispose(); }

    /**
     * Disposes all of the resources used by this text field.
     *
     * A disposed text field can be safely reinitialized. Any child will
     * be released. They will be deleted if no other object owns them.
     *
     * It is unsafe to call this on a text field that is still currently
     * inside of a scene graph.
     */
    virtual void dispose() override;

    /**
     * Initializes a node with the given JSON specificaton.
     *
     * This initializer is designed to receive the "data" object from the
     * JSON passed to {@link Scene2Loader}.  This JSON format supports all
     * of the attribute values of its parent class. In addition, it supports
     * the following additional attributes:
     *
     *     "cursor":        A boolean indicating whether to show the cursor (when active)
     *     "cursorwidth":   A number indicating the width of the cursor rectangle
     *     "cursorcolor":   Either a four-element integer array (values 0..255) or a string
     *                      Any string should be a web color or a Tkinter color name.
     *
     * All attributes are optional. There are no required attributes.
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
     * Returns a newly allocated text field with the given size.
     *
     * The size defines the content size. The bounding box of the node is
     * (0,0,width,height) and is anchored in the bottom left corner (0,0).
     * The node is positioned at the origin in parent space.
     *
     * The label will empty, as it has no font or text.
     *
     * @param size  The size of the text field in parent space
     *
     * @return a newly allocated text field with the given size.
     */
    static std::shared_ptr<TextField> allocWithBounds(const Size size) {
        std::shared_ptr<TextField> result = std::make_shared<TextField>();
        return (result->initWithBounds(size) ? result : nullptr);
    }
    
    /**
     * Returns a newly allocated text field with the given size.
     *
     * The size defines the content size. The bounding box of the node is
     * (0,0,width,height) and is anchored in the bottom left corner (0,0).
     * The node is positioned at the origin in parent space.
     *
     * The label will empty, as it has no font or text.
     *
     * @param width     The width of the text field in parent space
     * @param height    The height of the text field in parent space
     *
     * @return a newly allocated text field with the given size.
     */
    static std::shared_ptr<TextField> allocWithBounds(float width, float height) {
        std::shared_ptr<TextField> result = std::make_shared<TextField>();
        return (result->initWithBounds(width,height) ? result : nullptr);
    }
    
    /**
     * Returns a newly allocated text field with the given bounds.
     *
     * The rectangle origin is the bottom left corner of the node in parent
     * space, and corresponds to the origin of the Node space. The size
     * defines its content width and height in node space. The node anchor
     * is placed in the bottom left corner.
     *
     * The label will empty, as it has no font or text.
     *
     * @param rect  The bounds of the text field in parent space
     *
     * @return a newly allocated text field with the given bounds.
     */
    static std::shared_ptr<TextField> allocWithBounds(const Rect rect) {
        std::shared_ptr<TextField> result = std::make_shared<TextField>();
        return (result->initWithBounds(rect) ? result : nullptr);
    }
    
    /**
     * Returns a newly allocated text field with the given bounds.
     *
     * The rectangle origin is the bottom left corner of the node in parent
     * space, and corresponds to the origin of the Node space. The size
     * defines its content width and height in node space. The node anchor
     * is placed in the bottom left corner.
     *
     * The label will empty, as it has no font or text.
     *
     * @param x         The x-coordinate of the origin in parent space
     * @param y         The y-coordinate of the origin in parent space
     * @param width     The width of the text field in parent space
     * @param height    The height of the text field in parent space
     *
     * @return a newly allocated text field with the given bounds.
     */
    static std::shared_ptr<TextField> allocWithBounds(float x, float y, float width, float height) {
        std::shared_ptr<TextField> result = std::make_shared<TextField>();
        return (result->initWithBounds(x,y,width,height) ? result : nullptr);
    }

    /**
     * Returns a newly allocated text field with the given text and font
     *
     * The text field will be sized to fit the rendered text exactly. That is,
     * the height will be the maximum height of the font, and the width will be
     * the sum of the advance of the rendered characters. That means that there
     * may be some natural spacing around the characters.
     *
     * By default, the text will formated so that the origin is on the left
     * edge of the baseline (of the top line). The text will take up a single
     * line unless there are newline characters in the string. If any glyphs
     * are missing from the font atlas, they will not be rendered.
     *
     * The text will be placed at the origin of the parent and will be anchored
     * in the bottom left corner.
     *
     * @param text  The text to display in the text field
     * @param font  The font for this text field
     *
     * @return a newly allocated text field with the given text and font atlas
     */
    static std::shared_ptr<TextField> allocWithText(const std::string text,
                                                    const std::shared_ptr<Font>& font) {
        std::shared_ptr<TextField> result = std::make_shared<TextField>();
        return (result->initWithText(text,font) ? result : nullptr);
    }

    /**
     * Returns a newly allocated text field with the given text and font
     *
     * The text field will be sized to fit the rendered text exactly. That is,
     * the height will be the maximum height of the font, and the width will be
     * the sum of the advance of the rendered characters. That means that there
     * may be some natural spacing around the characters.
     *
     * By default, the text will formated so that the origin is on the left
     * edge of the baseline (of the top line). The text will take up a single
     * line unless there are newline characters in the string. If any glyphs
     * are missing from the font atlas, they will not be rendered.
     *
     * The text will be placed at the origin of the parent and will be anchored
     * in the bottom left corner.
     *
     * @param position  The text field position
     * @param text      The text to display in the text field
     * @param font      The font for this text field
     *
     * @return a newly allocated text field with the given text and font atlas
     */
    static std::shared_ptr<TextField> allocWithText(const Vec2 position, const std::string text,
                                                    const std::shared_ptr<Font>& font) {
        std::shared_ptr<TextField> result = std::make_shared<TextField>();
        return (result->initWithText(position,text,font) ? result : nullptr);
    }

    /**
     * Returns a newly allocated multiline text field with the given dimensions
     *
     * The text field will use the size specified and attempt to fit the text
     * in this region. Lines will be broken at white space locations to keep
     * each line within the size width. However, this may result in so many
     * lines that glyphs at the bottom are cut out. A label will never render
     * text outside of its content bounds.
     *
     * By default, a multiline text field is aligned to the top and left. It
     * has a line spacing of 1 (single-spaced).
     *
     * The label will be placed at the origin of the parent and will be anchored
     * in the bottom left.
     *
     * @param size  The size of the text field to display
     * @param text  The text to display in the text field
     * @param font  The font for this text field
     *
     * @return a newly allocated multiline text field with the given dimensions
     */
    static std::shared_ptr<TextField> allocWithTextBox(const Size size, const std::string text,
                                                       const std::shared_ptr<Font>& font) {
        std::shared_ptr<TextField> result = std::make_shared<TextField>();
        return (result->initWithTextBox(size,text,font) ? result : nullptr);
    }
    
    /**
     * Returns a newly allocated multiline text field with the given dimensions
     *
     * The text field will use the size specified and attempt to fit the text
     * in this region. Lines will be broken at white space locations to keep
     * each line within the size width. However, this may result in so many
     * lines that glyphs at the bottom are cut out. A label will never render
     * text outside of its content bounds.
     *
     * By default, a multiline text field is aligned to the top and left. It
     * has a line spacing of 1 (single-spaced).
     *
     * The label will use the rectangle origin to position this label in its
     * parent. It will be anchored in the bottom left.
     *
     * @param rect  The bounds of the text field to display
     * @param text  The text to display in the text field
     * @param font  The font for this text field
     *
     * @return a newly allocated multiline text field with the given dimensions
     */
    static std::shared_ptr<TextField> allocWithTextBox(const Rect rect, const std::string text,
                                                       const std::shared_ptr<Font>& font) {
        std::shared_ptr<TextField> result = std::make_shared<TextField>();
        return (result->initWithTextBox(rect,text,font) ? result : nullptr);
    }
    
    /**
     * Returns a newly allocated node with the given JSON specificaton.
     *
     * This initializer is designed to receive the "data" object from the
     * JSON passed to {@link Scene2Loader}.  This JSON format supports all
     * of the attribute values of its parent class. In addition, it supports
     * the following additional attributes:
     *
     *     "cursor":        A boolean indicating whether to show the cursor (when active)
     *     "cursorwidth":   A number indicating the width of the cursor rectangle
     *     "cursorcolor":   Either a four-element integer array (values 0..255) or a string
     *                      Any string should be a web color or a Tkinter color name.
     *
     * All attributes are optional. There are no required attributes.
     *
     * @param loader    The scene loader passing this JSON file
     * @param data      The JSON object specifying the node
     *
     * @return a newly allocated node with the given JSON specificaton.
     */
    static std::shared_ptr<SceneNode> allocWithData(const Scene2Loader* loader,
                                                    const std::shared_ptr<JsonValue> data) {
        std::shared_ptr<TextField> node = std::make_shared<TextField>();
        if (!node->initWithData(loader,data)) { node = nullptr; }
        return std::dynamic_pointer_cast<SceneNode>(node);
    }


#pragma mark -
#pragma mark Listeners
    /**
     * Returns true if this text field has a type listener
     *
     * This listener is invoked when the text changes.
     *
     * @return true if this text field has a type listener
     */
    bool hasTypeListener() const { return !_typeListeners.empty(); }
      
    /**
     * Returns the type listener for the given key
     *
     * This listener is invoked when the text changes.
     *
     * If there is no listener for the given key, it returns nullptr.
     *
     * @param key   The identifier for the listener
     *
     * @return the type listener for the given key
     */
    const Listener getTypeListener(Uint32 key) const;

    /**
     * Returns all type listeners for this text field
     *
     * This listener is invoked when the text changes.
     *
     * @return the type listeners for this text field
     */
    const std::vector<Listener> getTypeListeners() const;

    /**
     * Returns a key for a type listener after adding it to this text field.
     *
     * This listener is invoked when the text changes.
     *
     * C++ cannot hash functions types.  Therefore, the listener will
     * be identified by a unique key, returned by this function.  You should
     * remember this key to remove the listener if necessary.
     *
     * @param listener  The listener to add
     *
     * @return the key for the listener
     */
    Uint32 addTypeListener(Listener listener);

    /**
     * Removes a type listener from this text field.
     *
     * This listener is invoked when the text changes.
     *
     * Listeners must be identified by the key returned by the {@link #addTypeListener}
     * method. If this text field does not have a listener for the given key,
     * this method will fail.
     *
     * @param key  The key of the listener to remove
     *
     * @return true if the listener was succesfully removed
     */
    bool removeTypeListener(Uint32 key);

    /**
     * Clears all type listeners for this text field.
     *
     * These listeners are invoked when the text changes. This method
     * does not require you to remember the keys assigned to the
     * individual listeners.
     *
     * @return true if the listener was succesfully removed
     */
    void clearTypeListeners();
    
    /**
     * Returns true if this text field has an exit listener
     *
     * This listener is invoked when the field loses focus.
     *
     * @return true if this text field has an exit listener
     */
    bool hasExitListener() const { return !_exitListeners.empty(); }
      
    /**
     * Returns the exit listener for the given key
     *
     * This listener is invoked when the field loses focus.
     *
     * If there is no listener for the given key, it returns nullptr.
     *
     * @param key   The identifier for the listener
     *
     * @return the listener for the given key
     */
    const Listener getExitListener(Uint32 key) const;

    /**
     * Returns all exit listeners for this text field
     *
     * These listeners are invoked when the field loses focus.
     *
     * @return the exits listeners for this text field
     */
    const std::vector<Listener> getExitListeners() const;

    /**
     * Returns a key for a exit listener after adding it to this field.
     *
     * This listener is invoked when the field loses focus.
     *
     * C++ cannot hash functions types.  Therefore, the listener will
     * be identified by a unique key, returned by this function.  You should
     * remember this key to remove the listener if necessary.
     *
     * @param listener  The listener to add
     *
     * @return the key for the listener
     */
    Uint32 addExitListener(Listener listener);

    /**
     * Removes a listener from this text fieldexi.
     *
     * This listener is invoked when the field loses focus.
     *
     * Listeners must be identified by the key returned by the {@link #addExitListener}
     * method. If this text field does not have a listener for the given key,
     * this method will fail.
     *
     * @param key  The key of the listener to remove
     *
     * @return true if the listener was succesfully removed
     */
    bool removeExitListener(Uint32 key);

    /**
     * Clears all listeners for this text field.
     *
     * These listener are invoked when the field loses focus. This
     * method does not require you to remember the keys assigned to
     * the individual listeners.
     *
     * @return true if the listener was succesfully removed
     */
    void clearExitListeners();
    
    
#pragma mark -
#pragma mark Editing
    /**
     * Sets the text for this label.
     *
     * The string must be in either ASCII or UTF8 format. No other string
     * encodings are supported.  As all ASCII strings are also UTF8, you can
     * this effectively means that the text must be UTF8.
     *
     * If the font is missing glyphs in this string, the characters in the text
     * may be different than those displayed. Furthermore, if this label has no
     * font, then the text will not display at all.
     *
     * Changing this value will regenerate the render data, and is potentially
     * expensive, particularly if the font is using a fallback atlas.
     *
     * Reseting this value will naturally put the cursor at the end of the text.
     *
     * @param text      The text for this label.
     * @param resize    Whether to resize the label to fit the new text.
     */
    virtual void setText(const std::string& text, bool resize=false) override;
    
    /**
     * Activates this text field to enable editing.
     *
     * This method attaches a listener to either the {@link Mouse} or
     * {@link Touchscreen} inputs to monitor when the text field is pressed
     * and/or released. The text field will favor the mouse, but will use the
     * touch screen if no mouse input is active. If neither input is active,
     * this method will fail.
     *
     * It will also attach a listener to {@link TextInput} to provide access
     * to a (possibly virtual) keyboard and collect user typing.  Finally, it
     * attaches a liatener to {@link Keyboard} to monitor special keys such
     * as Alt, Meta, and the arrow keys.
     *
     * Notice that activating a text field and requesting focus is separate.
     * The field will request focus if selected by a touch or press, but it
     * cannot be editted until it has focus.
     *
     * @return true if the text field was successfully activated
     */
    bool activate();

   /**
    * Deactivates this text field, ignoring any future input.
    *
    * This method removes its internal listener from either the {@link Mouse}
    * or {@link Touchscreen}, and from {@link Keyboard} and {@link TextInput}.
    *
    * When deactivated, the text field will no longer change its text on its own.
    * However, the user can still change manually with the {@link setText()}
    * method. Futhermore, the appropriate type listener will be called when the
    * text changes. However, any attempts to manually acquire focus will fail.
    *
    * @param dispose    Whether this request is the result of a dispose action
    *
    * @return true if the text field was successfully deactivated
    */
    bool deactivate(bool dispose=false);

    /**
     * Returns true if this text field has been activated.
     *
     * @return true if this text field has been activated.
     */
    bool isActive() const { return _active; }
    
    /**
     * Requests text input focus for this text field.
     *
     * When a text field is activated, it does not immediately have focus.
     * A text field without focus cannot be editted.  By either clicking
     * on the field or calling thus function, you can acquire focus and
     * edit the field.
     *
     * This method will fail if the text field is not active.
     *
     * @return true if successfully requested focus.
     */
 	bool requestFocus();

    /**
     * Releases text input focus for this text field.
     *
     * When the focus is released, the label can no longer be editting.
     * Typically this means that the user has input the final value,
     * which is why the exit listener (if any exists) is called.
     *
     * In addition to calling this method manually, a user can release focus
     * either by pressing RETURN or clicking somewhere outside of the field.
     *
     * @return true if successfully released focus.
     */
 	bool releaseFocus();

    /**
     * Returns true if this text field has focus.
     *
     * @return true if this text field has focus.
     */
    bool hasFocus() const { return _focused; }
    
#pragma mark -
#pragma mark Cursor Managment
    /**
     * Returns the cursor rectangle.
     *
     * The cursor is a rectangle indicating the editing position of the text
     * field. It has width {@link #getCursorWidth}, and its height is the
     * line height. It is centered on the insertion position.
     *
     * By default, this cursor will be shown (blinking) when the text field
     * has focus. However, it is possible to disable the cursor by calling
     * {@link #setCursorVisible}. That will hide the cursor even when editing
     * is active. You can then use this rectangle to overlay your own custom
     * cursor image.
     *
     * @return the cursor rectangle.
     */
    const Rect& getCursor() const { return _cursor; }

    /**
     * Returns true if the cursor is visible (when active).
     *
     * By default, the cursor will be shown (blinking) when the text field
     * has focus. Hiding the cursor allows you to replace it with your own
     * custom cursor image.  The text field will still track the cursor
     * rectangle; it just will not show it.
     *
     * @return true if the cursor is visible (when active).
     */
    bool isCursorVisible() const { return _showCursor; }

    /**
     * Sets whether the cursor is visible (when active).
     *
     * By default, the cursor will be shown (blinking) when the text field
     * has focus. Hiding the cursor allows you to replace it with your own
     * custom cursor image.  The text field will still track the cursor
     * rectangle; it just will not show it.
     *
     * @param visible   Whether the cursor is visible (when active).
     */
    void setCursorVisible(bool visible) { _showCursor = visible; }
    
    /**
     * Returns the cursor width.
     *
     * The cursor is always a simple rectangle, though this rectangle can
     * be accessed by {@link #getCursor} to draw a custom cursor image.
     * While the cursor always has the line height as its height, this
     * value controls the width. The cursor is always centered on the
     * insertion position.
     *
     * @return the cursor width.
     */
    float getCursorWidth() const { return _cursorWidth; }

    /**
     * Sets the cursor width.
     *
     * The cursor is always a simple rectangle, though this rectangle can
     * be accessed by {@link #getCursor} to draw a custom cursor image.
     * While the cursor always has the line height as its height, this
     * value controls the width. The cursor is always centered on the
     * insertion position.
     *
     * @param width     The cursor width.
     */
    void setCursorWidth(float width) { _cursorWidth = width; }

    /**
     * Returns the cursor color
     *
     * If the cursor is visible, then it will be drawn (when active) as a
     * solid rectangle with this color.  By default, this value is black.
     *
     * @return the cursor color
     */
    Color4 getCursorColor() const { return _cursorColor; }
    
    /**
     * Sets the cursor color
     *
     * If the cursor is visible, then it will be drawn (when active) as a
     * solid rectangle with this color.  By default, this value is black.
     *
     * @param color     The cursor color
     */
    void setCursorColor(const Color4 color) { _cursorColor = color; }
    
    
#pragma mark -
#pragma mark Rendering
    /**
     * Draws this text field via the given SpriteBatch.
     *
     * This method only worries about drawing the current text field.  It does not
     * attempt to render the children.
     *
     * This method provides the correct transformation matrix and tint color.
     * You do not need to worry about whether the node uses relative color.
     * This method is called by render() and these values are guaranteed to be
     * correct.  In addition, this method does not need to check for visibility,
     * as it is guaranteed to only be called when the node is visible.
     *
     * This method overrides the one from {@link Label}. It adds the drawing of
     * a blinking cursor that indicates the edit position.
     *
     * @param batch     The SpriteBatch to draw with.
     * @param transform The global transformation matrix.
     * @param tint      The tint to blend with the Node color.
     */
    virtual void draw(const std::shared_ptr<SpriteBatch>& batch, const Affine2& transform, Color4 tint) override;

    /**
     * Allocates the render data necessary to render this node.
     */
    virtual void generateRenderData() override;
    
protected:
#pragma mark -
#pragma mark Internal Helpers
    /**
     * Updates the text with the given input data.
     *
     * This method is a callback for the {@link TextInput} device.
     *
     * @param event		The text input event to be handled.
     * @param focus		Whether the text field currently has text input focus.
     */
    void updateInput(const TextInputEvent& event, bool focus);

	/**
	 * Updates the state of any special keys.
	 *
	 * This method is a callback for the {@link Keyboard} device.
	 *
	 * @param event     The key event to be handled.
	 * @param focus     Whether the text field currently has keyboard focus
	 * @param down      Whether the key is pressed down (as opposed to released)
	 */
    void updateKey(const KeyEvent& event, bool focus, bool down);

    /**
     * Responds to a touch or press, changing the field focus.
     *
     * If the press is outside text field, focus will be released. If the
     * press is within the bounds, it will request focus (if not currently
     * in focus) and move the cursor to the position pressed.
     *
     * @param pos   The screen coordinate where the event happened.
     * @param focus	Whether the text field currently has keyboard focus
     */
    void updatePress(Vec2 pos, bool focus);

    /**
     * Updates the cursor position.
     *
     * This method is called whenever either the text changes or the cursor
     * moves. Notice that this must be updated even if the cursor is not
     * visible.
     */
    void updateCursor();

    /**
     * Moves the cursor one word forward or backward.
     *
     * If there is any space between the cursor and the word in the correct
     * direction, it will move adjacent to the word.  Otherwise, it will
     * skip over the word.
     *
     * @param forward   Whether to move the cursor forward.
     *
     * @return the index of the new cursor position
     */
    size_t skipWord(bool forward) const;

	/**
	 * Deletes one character before the current cursor.
	 *
	 * If alt is pressed, the method will delete an entire word. If meta
	 * is pressed, it will delete the entire field. If the deletion key
	 * is held down, this method will be called multiple times in a row
	 * after a short delay, lasting until the key is released.
	 */
    void deleteChar();
    
    /**
     * Moves the cursor one position to the left.
     *
     * If the label is multiline, this will move the cursor from the beginning
     * of line to the end of the next. Nothing will happen if the cursor is
     * at the start of the text.
     */
    void leftArrow();
    
    /**
     * Moves the cursor one position to the right.
     *
     * If the label is multiline, this will move the cursor from the end of a
     * line to the beginning of the next. Nothing will happen if the cursor is
     * at the end of the text.
     */
    void rightArrow();
    
    /**
     * Moves the cursor one line up.
     *
     * The cursor is moved "visually". That is, it is moved to the edit position
     * that is closest horizontally to the original position.  In the case of
     * monospaced fonts, this insures that the cursor maintains the same number
     * of characters from the start of the line.  However, this is not the case
     * for proportional fonts.
     */
    void upArrow();

    /**
     * Moves the cursor one line down.
     *
     * The cursor is moved "visually". That is, it is moved to the edit position
     * that is closest horizontally to the original position.  In the case of
     * monospaced fonts, this insures that the cursor maintains the same number
     * of characters from the start of the line.  However, this is not the case
     * for proportional fonts.
     */
    void downArrow();

    /**
     * Inserts the given unicode character into the text
     *
     * This method will force a recomputation of the layout.
     *
     * @param unicode   The unicode character
     * @param pos       The position to insert the character
     */
    size_t insertChar(Uint32 unicode, size_t pos);
    
    /**
     * Inserts a newline character into the text
     *
     * This method will force a recomputation of the layout.
     *
     * @param pos       The position to insert the newline
     */
    size_t breakLine(size_t pos);

    /**
     * Returns the index for the given row and x-coordinate
     *
     * The method is used to place the cursor position from either
     * a mouse/touch click, or an up/down arrow. The value offset
     * is adjusted to be center of the cursor, preventing us from
     * having to call {@link #updateCursor}.
     *
     * In determining the cursor position, this method find the
     * nearest character to offset. It moves the cursor to either
     * the left or right of this character depending on which side
     * of the character centerline this offset sits.
     *
     * @param row       The text row
     * @param offset    The x-coordinate to place the cursor
     *
     * @return the index for the given row and x-coordinate
     */
    size_t getCharIndex(size_t row, float& offset) const;

    /**
     * Returns the text row for the current cursor position.
     *
     * @return the text row for the current cursor position.
     */
    size_t getCharRow() const;
    
    /**
     * Invokes the appropriate listeners for this text field.
     *
     * If exit is true, it invokes the exit listeners.  Otherwise it
     * invokes the type listeners.
     *
     * @param exit  Whether to invoke the exit listeners
     */
    void invokeListeners(bool exit);

    

};
	}
}

#endif /* __CU_TEXT_FIELD_H__ */
