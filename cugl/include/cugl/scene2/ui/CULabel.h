//
//  CULabel.h
//  Cornell University Game Library (CUGL)
//
//  This module provides a scene graph node that displays formatted text. It
//  is backed by a text layout object and therefore can support multi-line
//  text.
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
//  Version: 7/30/21
//
#ifndef __CU_LABEL_H__
#define __CU_LABEL_H__

#include <string>
#include <cugl/scene2/graph/CUSceneNode.h>
#include <cugl/render/CUTextAlignment.h>

namespace cugl {

/** Forward reference for now */
class Font;
class GlyphRun;
class TextLayout;
    /**
     * The classes to construct an 2-d scene graph.
     *
     * This namespace was chosen to future-proof the game engine. We will
     * eventually want to add 3-d scene graphs as well, and this namespace
     * will prevent any collisions with those scene graph nodes.
     */
    namespace scene2 {

/**
 * This class is a node the represents formatted text on rectangular background.
 *
 * By default, the content size is just large enough to render the text given.
 * If you reset the content size to larger than the what the text needs, the
 * the text is placed in the label according to the text bounds. If you reset
 * it to smaller, the text may be cut off in rendering.
 *
 * If the background color is not clear, then the label will have a colored
 * backing rectangle.  The rectangle will extended from the origin to the
 * content size in Node space.
 *
 * The text itself is formatted using a {@link TextLayout}. This text layout
 * breaks the text into multiple lines as needed, though by default, a label
 * only breaks lines at explicit newlines. The text layout aligns the lines
 * of text with each other. But it also positions the text against the
 * background rectangle as well. See {@link #getHorizontalAlignment} and
 * {@link #getVerticalAlignment} for more information.
 *
 * To display the text, you need a {@link Font}. The label assumes that the
 * font has an atlas, either through a pre-built atlas, or by setting the
 * attribute {@link Font#setAtlasFallback} to true.  If the font does not
 * have an atlas, or characters are missing from the atlas, then those
 * glyphs will not be displayed. It is generally recommended that you use
 * a prebuilt atlas, as fallback atlases introduce significant time and
 * memory overhead.
 */
class Label : public SceneNode {
#pragma mark Values
protected:
    /** The font (with or without an atlas) */
    std::shared_ptr<Font> _font;
    /** The underlying text layout (the text and font are accessed from here) */
    std::shared_ptr<TextLayout> _layout;
    /** The position to place the origin of the layout (in Node coordinates) */
    Vec2 _offset;
    /** The bottom padding offset */
    float _padbot;
    /** The left padding offset */
    float _padleft;
    /** The top padding offset */
    float _padtop;
    /** The right padding offset */
    float _padrght;

    /** Whether to shadow the text */
    bool _dropShadow;
    /** The blurring effect for the drop shadow */
    float _dropBlur;
    /** The drop shadow offset */
    Vec2  _dropOffset;
    /** The color of the text (default is BLACK) */
    Color4 _foreground;
    /** The color of the background panel (default is CLEAR) */
    Color4 _background;
    
    /** The blending equation for this texture */
    GLenum _blendEquation;
    /** The source factor for the blend function */
    GLenum _srcFactor;
    /** The destination factor for the blend function */
    GLenum _dstFactor;

    /** Whether or not the glyphs have been rendered */
    bool _rendered;
    /** The font bounds */
    Rect _bounds;
    /** The glyph runs to render */
    std::unordered_map<GLuint, std::shared_ptr<GlyphRun>> _glyphrun;

public:
#pragma mark -
#pragma mark Constructors
    /**
     * Creates an uninitialized label with no text or font information.
     *
     * You must initialize this Label before use.
     *
     * NEVER USE A CONSTRUCTOR WITH NEW. If you want to allocate a Node on the
     * heap, use one of the static constructors instead.
     */
    Label();
    
    /**
     * Deletes this label, disposing all resources
     */
    ~Label() { dispose(); }
    
    /**
     * Disposes all of the resources used by this label.
     *
     * A disposed Label can be safely reinitialized. Any children owned by this
     * node will be released.  They will be deleted if no other object owns them.
     *
     * It is unsafe to call this on a Label that is still currently inside of
     * a scene graph.
     */
    virtual void dispose() override;
    
    
    /**
     * Disables the inherited initializer.
     *
     * This initializer is disabled and should not be used.
     *
     * @return false
     */
    virtual bool initWithPosition(const Vec2 pos) override {
        CUAssertLog(false,"Position-only initializer is not supported");
        return false;
    }
    
    /**
     * Disables the inherited initializer.
     *
     * This initializer is disabled and should not be used.
     *
     * @return false
     */
    bool initWithPosition(float x, float y) {
        CUAssertLog(false,"Position-only initializer is not supported");
        return false;
    }
    
    /**
     * Initializes a label with the given size.
     *
     * The size defines the content size. The bounding box of the node is
     * (0,0,width,height) and is anchored in the bottom left corner (0,0).
     * The node is positioned at the origin in parent space.
     *
     * The label will be empty, as it has no font or text.
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
     * The label will be empty, as it has no font or text.
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
     * The label will be empty, as it has no font or text.
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
     * The label will be empty, as it has no font or text.
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
     * Initializes a label at (0,0) with the given text and font
     *
     * The label will be sized to fit the rendered text exactly. That is, the
     * height will be the maximum height of the font, and the width will be the
     * sum of the advance of the rendered characters.  That means that there
     * may be some natural spacing around the characters.
     *
     * By default, the text will formated so that the origin is on the left
     * edge of the baseline (of the top line). The text will take up a single
     * line unless there are newline characters in the string. If any glyphs
     * are missing from the font atlas, they will not be rendered.
     *
     * @param text  The text to display in the label
     * @param font  The font for this label
     *
     * @return true if initialization was successful.
     */
    bool initWithText(const std::string text, const std::shared_ptr<Font>& font);

    /**
     * Initializes a label at the position with the given text and font
     *
     * The label will be sized to fit the rendered text exactly. That is, the
     * height will be the maximum height of the font, and the width will be the
     * sum of the advance of the rendered characters.  That means that there
     * may be some natural spacing around the characters.
     *
     * By default, the text will formated so that the origin is on the left
     * edge of the baseline (of the top line). The text will take up a single
     * line unless there are newline characters in the string. If any glyphs
     * are missing from the font atlas, they will not be rendered.
     *
     * @param position  The label position
     * @param text      The text to display in the label
     * @param font      The font for this label
     *
     * @return true if initialization was successful.
     */
    bool initWithText(const Vec2 position, const std::string text, const std::shared_ptr<Font>& font);

    /**
     * Initializes a multiline label with the given dimensions
     *
     * The label will use the size specified and attempt to fit the text in
     * this region. Lines will be broken at white space locations to keep each
     * line within the size width. However, this may result in so many lines
     * that glyphs at the bottom are cut out. A label will never render text
     * outside of its content bounds.
     *
     * By default, a multiline label is aligned to the top and left. It has
     * a line spacing of 1 (single-spaced).
     *
     * The label will be placed at the origin of the parent and will be anchored
     * in the bottom left.
     *
     * @param size  The size of the label to display
     * @param text  The text to display in the label
     * @param font  The font for this label
     *
     * @return true if initialization was successful.
     */
    bool initWithTextBox(const Size size, const std::string text, const std::shared_ptr<Font>& font);
    
    /**
     * Initializes a multiline label with the given dimensions
     *
     * The label will use the size specified and attempt to fit the text in
     * this region. Lines will be broken at white space locations to keep each
     * line within the size width. However, this may result in so many lines
     * that glyphs at the bottom are cut out. A label will never render text
     * outside of its content bounds.
     *
     * By default, a multiline label is aligned to the top and left. It has
     * a line spacing of 1 (single-spaced).
     *
     * The label will use the rectangle origin to position this label in its
     * parent. It will be anchored in the bottom left.
     *
     * @param rect  The bounds of the label to display
     * @param text  The text to display in the label
     * @param font  The font for this label
     *
     * @return true if initialization was successful.
     */
    bool initWithTextBox(const Rect rect, const std::string text, const std::shared_ptr<Font>& font);

    /**
     * Initializes a node with the given JSON specificaton.
     *
     * This initializer is designed to receive the "data" object from the
     * JSON passed to {@link Scene2Loader}.  This JSON format supports all
     * of the attribute values of its parent class. In addition, it supports
     * the following additional attributes:
     *
     *     "font":       The name of a previously loaded font asset
     *     "text":       The initial label text
     *     "foreground": Either a four-element integer array (values 0..255) or a string.
     *                   Any string should be a web color or a Tkinter color name.
     *     "background": Either a four-element integer array (values 0..255) or a string.
     *                   Any string should be a web color or a Tkinter color name.
     *     "padding":    A number or a 4-element float array.
     *     "dropshadow": A boolean indicating the presence of a drop shadow
     *     "wrap":       A boolean indicating whether to break text into lines
     *     "spacing":    A float indicating the spacing factor between lines
     *     "halign":     One of 'left', 'center', 'right', 'justify', 'hard left',
     *                   'true center' and 'hard right'.
     *     "valign":     One of 'top', 'middle', 'bottom', 'hard top', 'true middle'
     *                   and 'hard bottom'.
     *
     * All attributes are optional. There are no required attributes.  However,
     * a label without a font cannot display text.
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
     * Returns a newly allocated label with the given size.
     *
     * The size defines the content size. The bounding box of the node is
     * (0,0,width,height) and is anchored in the bottom left corner (0,0).
     * The node is positioned at the origin in parent space.
     *
     * The label will empty, as it has no font or text.     
     *
     * @param size  The size of the label in parent space
     *
     * @return a newly allocated label with the given size.
     */
    static std::shared_ptr<Label> allocWithBounds(const Size size) {
        std::shared_ptr<Label> result = std::make_shared<Label>();
        return (result->initWithBounds(size) ? result : nullptr);
    }
    
    /**
     * Returns a newly allocated label with the given size.
     *
     * The size defines the content size. The bounding box of the node is
     * (0,0,width,height) and is anchored in the bottom left corner (0,0).
     * The node is positioned at the origin in parent space.
     *
     * The label will empty, as it has no font or text.     
     *
     * @param width     The width of the label in parent space
     * @param height    The height of the label in parent space
     *
     * @return a newly allocated label with the given size.
     */
    static std::shared_ptr<Label> allocWithBounds(float width, float height) {
        std::shared_ptr<Label> result = std::make_shared<Label>();
        return (result->initWithBounds(width,height) ? result : nullptr);
    }
    
    /**
     * Returns a newly allocated label with the given bounds.
     *
     * The rectangle origin is the bottom left corner of the node in parent
     * space, and corresponds to the origin of the Node space. The size
     * defines its content width and height in node space. The node anchor
     * is placed in the bottom left corner.
     *
     * The label will empty, as it has no font or text.     
     *
     * @param rect  The bounds of the label in parent space
     *
     * @return a newly allocated label with the given bounds.
     */
    static std::shared_ptr<Label> allocWithBounds(const Rect rect) {
        std::shared_ptr<Label> result = std::make_shared<Label>();
        return (result->initWithBounds(rect) ? result : nullptr);
    }
    
    /**
     * Returns a newly allocated label with the given bounds.
     *
     * The rectangle origin is the bottom left corner of the node in parent
     * space, and corresponds to the origin of the Node space. The size
     * defines its content width and height in node space. The node anchor
     * is placed in the bottom left corner.
     *
     * The label will empty, as it has no font or text.     
     *
     * @param x         The x-coordinate of the label origin in parent space
     * @param y         The y-coordinate of the label origin in parent space
     * @param width     The width of the label in parent space
     * @param height    The height of the label in parent space
     *
     * @return a newly allocated label with the given bounds.
     */
    static std::shared_ptr<Label> allocWithBounds(float x, float y, float width, float height) {
        std::shared_ptr<Label> result = std::make_shared<Label>();
        return (result->initWithBounds(x,y,width,height) ? result : nullptr);
    }

    /**
     * Returns a newly allocated label with the given text and font
     *
     * The label will be sized to fit the rendered text exactly. That is, the
     * height will be the maximum height of the font, and the width will be the
     * sum of the advance of the rendered characters.  That means that there
     * may be some natural spacing around the characters.
     *
     * By default, the text will formated so that the origin is on the left
     * edge of the baseline (of the top line). The text will take up a single
     * line unless there are newline characters in the string. If any glyphs
     * are missing from the font atlas, they will not be rendered.
     *
     * @param text  The text to display in the label
     * @param font  The font for this label
     *
     * @return a newly allocated label with the given text and font atlas
     */
    static std::shared_ptr<Label> allocWithText(const std::string text, const std::shared_ptr<Font>& font) {
        std::shared_ptr<Label> result = std::make_shared<Label>();
        return (result->initWithText(text,font) ? result : nullptr);
    }

    /**
     * Returns a newly allocated label with the given text and font
     *
     * The label will be sized to fit the rendered text exactly. That is, the
     * height will be the maximum height of the font, and the width will be the
     * sum of the advance of the rendered characters.  That means that there
     * may be some natural spacing around the characters.
     *
     * By default, the text will formated so that the origin is on the left
     * edge of the baseline (of the top line). The text will take up a single
     * line unless there are newline characters in the string. If any glyphs
     * are missing from the font atlas, they will not be rendered.
     *
     * @param position  The label position
     * @param text      The text to display in the label
     * @param font      The font for this label
     *
     * @return a newly allocated label with the given text and font atlas
     */
    static std::shared_ptr<Label> allocWithText(const Vec2 position, const std::string text, const std::shared_ptr<Font>& font) {
        std::shared_ptr<Label> result = std::make_shared<Label>();
        return (result->initWithText(position,text,font) ? result : nullptr);
    }

    /**
     * Returns a newly allocated multiline label with the given dimensions
     *
     * The label will use the size specified and attempt to fit the text in
     * this region. Lines will be broken at white space locations to keep each
     * line within the size width. However, this may result in so many lines
     * that glyphs at the bottom are cut out. A label will never render text
     * outside of its content bounds.
     *
     * By default, a multiline label is aligned to the top and left. It has
     * a line spacing of 1 (single-spaced).
     *
     * The label will be placed at the origin of the parent and will be anchored
     * in the bottom left.
     *
     * @param size  The size of the label to display
     * @param text  The text to display in the label
     * @param font  The font for this label
     *
     * @return a newly allocated multiline label with the given dimensions
     */
    static std::shared_ptr<Label> allocWithTextBox(const Size size, const std::string text,
                                                   const std::shared_ptr<Font>& font) {
        std::shared_ptr<Label> result = std::make_shared<Label>();
        return (result->initWithTextBox(size,text,font) ? result : nullptr);
    }
    
    /**
     * Returns a newly allocated multiline label with the given dimensions
     *
     * The label will use the size specified and attempt to fit the text in
     * this region. Lines will be broken at white space locations to keep each
     * line within the size width. However, this may result in so many lines
     * that glyphs at the bottom are cut out. A label will never render text
     * outside of its content bounds.
     *
     * By default, a multiline label is aligned to the top and left. It has
     * a line spacing of 1 (single-spaced).
     *
     * The label will use the rectangle origin to position this label in its
     * parent. It will be anchored in the bottom left.
     *
     * @param rect  The bounds of the label to display
     * @param text  The text to display in the label
     * @param font  The font for this label
     *
     * @return a newly allocated multiline label with the given dimensions
     */
    static std::shared_ptr<Label> allocWithTextBox(const Rect rect, const std::string text,
                                                   const std::shared_ptr<Font>& font) {
        std::shared_ptr<Label> result = std::make_shared<Label>();
        return (result->initWithTextBox(rect,text,font) ? result : nullptr);
    }

    /**
     * Returns a newly allocated node with the given JSON specificaton.
     *
     * This initializer is designed to receive the "data" object from the
     * JSON passed to {@link Scene2Loader}.  This JSON format supports all
     * of the attribute values of its parent class.  In addition, it supports
     * the following additional attributes:
     *
     *     "font":       The name of a previously loaded font asset
     *     "text":       The initial label text
     *     "foreground": Either a four-element integer array (values 0..255) or a string.
     *                   Any string should be a web color or a Tkinter color name.
     *     "background": Either a four-element integer array (values 0..255) or a string.
     *                   Any string should be a web color or a Tkinter color name.
     *     "padding":    A number or a 4-element float array.
     *     "dropshadow": A boolean indicating the presence of a drop shadow
     *     "wrap":       A boolean indicating whether to break text into lines
     *     "spacing":    A float indicating the spacing factor between lines
     *     "halign":     One of 'left', 'center', 'right', 'justify', 'hard left',
     *                   'true center' and 'hard right'.
     *     "valign":     One of 'top', 'middle', 'bottom', 'hard top', 'true middle'
     *                   and 'hard bottom'.
     *
     * All attributes are optional. There are no required attributes.  However,
     * a label without a font cannot display text.
     *
     * @param loader    The scene loader passing this JSON file
     * @param data      The JSON object specifying the node
     *
     * @return a newly allocated node with the given JSON specificaton.
     */
    static std::shared_ptr<SceneNode> allocWithData(const Scene2Loader* loader,
                                                    const std::shared_ptr<JsonValue>& data) {
        std::shared_ptr<Label> result = std::make_shared<Label>();
        if (!result->initWithData(loader,data)) { result = nullptr; }
        return std::dynamic_pointer_cast<SceneNode>(result);
    }
    
#pragma mark -
#pragma mark Text Attributes
    /**
     * Returns the text for this label.
     *
     * The string will be in either ASCII or UTF8 format. No other string
     * encodings are supported.  As all ASCII strings are also UTF8, you can
     * this effectively means that the text must be UTF8.
     *
     * If the font is missing glyphs in this string, the characters in the text 
     * may be different than those displayed. Furthermore, if this label has no
     * font, then the text will not display at all.
     *
     * @return the text for this label.
     */
    const std::string& getText() const;
    
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
     * @param text      The text for this label.
     * @param resize    Whether to resize the label to fit the new text.
     */
    virtual void setText(const std::string& text, bool resize=false);
    
    /**
     * Returns the font to use for this label
     *
     * @return the font to use for this label
     */
    std::shared_ptr<Font> getFont() const { return _font; }
    
    /**
     * Sets the font to use this label
     *
     * Changing this value will regenerate the render data, and is potentially
     * expensive, particularly if the font does not have an atlas.
     *
     * @param font      The font to use for this label
     * @param resize    Whether to resize this label to fit the new font
     */
    void setFont(const std::shared_ptr<Font>& font, bool resize=false);
    
    /**
     * Returns the horizontal alignment of the text.
     *
     * Horizontal alignment serves two purposes in a label. First it is the
     * alignment of multiple lines of text to each other, as specified in
     * {@link TextLayout}. But it also represents the relationship between
     * the text and the background region of this node. In particular,
     * setting this value has the following effects:
     *
     * LEFT, HARD_LEFT, and JUSTIFY all place the left edge of the text
     * layout against the left edge of the label, after applying padding.
     * Similarly, RIGHT places the right edge of the text layout against
     * the right edge of the label, after applying padding.
     *
     * Padding is unusual for the CENTER and TRUE_CENTER alignments. The
     * center of the label is determined from the edges after applying
     * padding. So if the left and right padding are not equal, then
     * the text will not be placed at the true center. Even if they are
     * equal, the left and right padding reduce the width of the label,
     * which can affect word wrap.
     *
     * This value is LEFT by default.
     *
     * @return the horizontal alignment of the text.
     */
    HorizontalAlign getHorizontalAlignment() const;

    /**
     * Sets the horizontal alignment of the text.
     *
     * Horizontal alignment serves two purposes in a label. First it is the
     * alignment of multiple lines of text to each other, as specified in
     * {@link TextLayout}. But it also represents the relationship between
     * the text and the background region of this node. In particular,
     * setting this value has the following effects:
     *
     * LEFT, HARD_LEFT, and JUSTIFY all place the left edge of the text
     * layout against the left edge of the label, after applying padding.
     * Note that the meaning of the "left edge" differs between LEFT and
     * HARD_LEFT in {@link TextLayout}. Similarly, RIGHT and HARD_RIGHT
     * place the right edge of the text layout against the right edge of
     * the label, after applying padding.
     *
     * Padding is unusual for the CENTER and TRUE_CENTER alignments. The
     * center of the label is determined from the edges after applying
     * padding. So if the left and right padding are not equal, then
     * the text will not be placed at the true center. Even if they are
     * equal, the left and right padding reduce the width of the label,
     * which can affect word wrap.
     *
     * This value is LEFT by default.
     *
     * @param halign    The horizontal alignment of the text.
     */
    void setHorizontalAlignment(HorizontalAlign halign);

    /**
     * Returns the vertical alignment of the text.
     *
     * Vertical alignment is used to place the formatted text in against
     * the background rectangle. The options are interpretted as follows:
     *
     * TOP and HARD_TOP place the top edge of the text layout against the
     * top edge of the label, after applying padding. Note that the meaning
     * of "top edge" differs between TOP and HARD_TOP in {@link TextLayout}.
     * Similarly, BOTTOM and HARD_BOTTOM place the bottom edge of the text
     * layout against the bottom edge of the label, after applying padding.
     *
     * Padding is unusual for the MIDDLE and TRUE_MIDDLE alignments. The
     * middle of the label is determined from the edges after applying
     * padding. So if the top and bottom padding are not equal, then the
     * text will not be placed at the true middle.
     *
     * Finally, for BASELINE, this will place the baseline of the *bottom*
     * line (not the top, as the case with {@link TextLayout}) line at the
     * bottom edge of the label, after adjusting for padding.
     *
     * This value is TOP by default.
     *
     * @return the vertical alignment of the text.
     */
    VerticalAlign getVerticalAlignment() const;
    
    /**
     * Sets the vertical alignment of the text.
     *
     * Vertical alignment is used to place the formatted text in against
     * the background rectangle. The options are interpretted as follows:
     *
     * TOP and HARD_TOP place the top edge of the text layout against the
     * top edge of the label, after applying padding. Note that the meaning
     * of "top edge" differs between TOP and HARD_TOP in {@link TextLayout}.
     * Similarly, BOTTOM and HARD_BOTTOM place the bottom edge of the text
     * layout against the bottom edge of the label, after applying padding.
     *
     * Padding is unusual for the MIDDLE and TRUE_MIDDLE alignments. The
     * middle of the label is determined from the edges after applying
     * padding. So if the top and bottom padding are not equal, then the
     * text will not be placed at the true middle.
     *
     * Finally, for BASELINE, this will place the baseline of the *bottom*
     * line (not the top, as the case with {@link TextLayout}) line at the
     * bottom edge of the label, after adjusting for padding.
     *
     * This value is TOP by default.
     *
     * @param valign    The horizontal alignment of the text.
     */
    void setVerticalAlignment(VerticalAlign valign);
    
    /**
     * Returns true if this label will wrap text to fit.
     *
     * By default, label text will be displayed on one line (unless it has
     * newline characters). This could cause the text to exceed the bounds
     * of the content size and be cut off. If this option is true, the label
     * will break up lines at white space locations (or mid-word if there
     * are no white space locations) so that each line can fit in the width
     * of the content region. This could still cause text to be cut off if
     * the height of the content region is not large enough.
     *
     * Not that even if this value is false, a label will still break lines
     * at newline characters. If you do not want the label to break up a
     * string with newlines, you should remove the newline characters from
     * the string (as these are not renderable anyway).
     *
     * @return true if this label will wrap text to fit.
     */
    bool getWrap() const;

    /**
     * Sets whether this label will wrap text to fit.
     *
     * By default, label text will be displayed on one line (unless it has
     * newline characters). This could cause the text to exceed the bounds
     * of the content size and be cut off. If this option is true, the label
     * will break up lines at white space locations (or mid-word if there
     * are no white space locations) so that each line can fit in the width
     * of the content region. This could still cause text to be cut off if
     * the height of the content region is not large enough.
     *
     * Not that even if this value is false, a label will still break lines
     * at newline characters. If you do not want the label to break up a
     * string with newlines, you should remove the newline characters from
     * the string (as these are not renderable anyway).
     *
     * @param wrap  Whether this label will wrap text to fit.
     */
    void setWrap(bool wrap);

    /**
     * Returns the line spacing of this label.
     *
     * This value is multiplied by the font size to determine the space
     * between lines in the label. So a value of 1 is single-spaced text,
     * while a value of 2 is double spaced. The value should be positive.
     *
     * @return the line spacing of this label.
     */
    float getSpacing() const;

    /**
     * Sets the line spacing of this label.
     *
     * This value is multiplied by the font size to determine the space
     * between lines in the label. So a value of 1 is single-spaced text,
     * while a value of 2 is double spaced. The value should be positive.
     *
     * @param spacing   The line spacing of this label.
     */
    void setSpacing(float spacing);

#pragma mark -
#pragma mark Label Sizing
    /**
     * Sets the padding of the label.
     *
     * Padding can be added to the bottom, left, top, and right edges of the
     * label. This padding detracts from the area in which text can appear,
     * and can influence the word wrap. We call the area of the label in
     * which the text can appear the *interior* of the label.
     *
     * Padding can also have effect on the various text alignments. See both
     * {@link #setHorizontalAlignment} and {@link #setVerticalAlignment} for
     * the affect of padding on alignment.
     *
     * @param left      The left edge padding of the label
     * @param bottom    The bottom edge padding of the label
     * @param right     The right edge padding of the label
     * @param top       The top edge padding of the label
     */
    void setPadding(float left, float bottom, float right, float top);
    
    /**
     * Sets the padding of the label.
     *
     * Padding can be added to the bottom, left, top, and right edges of the
     * label. This padding detracts from the area in which text can appear,
     * and can influence the word wrap. We call the area of the label in
     * which the text can appear the *interior* of the label. In this method,
     * the padding is applied to uniformly to all sides.
     *
     * Padding can also have effect on the various text alignments. See both
     * {@link #setHorizontalAlignment} and {@link #setVerticalAlignment} for
     * the affect of padding on alignment.
     *
     * @param pad   The uniform padding for each edge
     */
    void setPadding(float pad) {
        setPadding(pad,pad,pad,pad);
    }
    
    /**
     * Returns the left-edge padding of the label.
     *
     * Adding left padding to a label reduces the text width of the label,
     * which can affect word wrap (if enabled). In addition, the padding
     * can affect the position of the text. If the text has alignment LEFT
     * or JUSTIFY, the label will shift the text right by this amount. On
     * the other hand, it will have no effect on RIGHT aligned text (other
     * than reducing the line width).
     *
     * For CENTER aligned text, the label center is computed after applying
     * padding to the edges.  So the left and right padding should be equal
     * to ensure that the text is in the true center of the label.
     *
     * @return the left-edge padding of the label.
     */
    float getPaddingLeft() const { return _padleft; }
    
    /**
     * Sets the left-edge padding of the label.
     *
     * Adding left padding to a label reduces the text width of the label,
     * which can affect word wrap (if enabled). In addition, the padding
     * can affect the position of the text. If the text has alignment LEFT
     * or JUSTIFY, the label will shift the text right by this amount. On
     * the other hand, it will have no effect on RIGHT aligned text (other
     * than reducing the line width).
     *
     * For CENTER aligned text, the label center is computed after applying
     * padding to the edges.  So the left and right padding should be equal
     * to ensure that the text is in the true center of the label.
     *
     * @param left  The left-edge padding of the label.
     */
    void setPaddingLeft(float left) { setPadding(left,_padbot,_padrght,_padtop); }
    
    /**
     * Returns the bottom-edge padding of the label.
     *
     * Adding bottom padding to a label does not effect word wrap, but it
     * can affect the text position. If the text has alignment BOTTOM or
     * BASELINE, the label will shift the text up by this amount. On the
     * other hand, it will have no effect on TOP aligned text.
     *
     * For MIDDLE aligned text, the label middle is computed after applying
     * padding to the edges.  So the top and bottom padding should be equal
     * to ensure that the text is in the true middle of the label.
     *
     * @return the bottom-edge padding of the label.
     */
    float getPaddingBottom() const { return _padbot; }
    
    /**
     * Sets the bottom-edge padding of the label.
     *
     * Adding bottom padding to a label does not effect word wrap, but it
     * can affect the text position. If the text has alignment BOTTOM or
     * BASELINE, the label will shift the text up by this amount. On the
     * other hand, it will have no effect on TOP aligned text.
     *
     * For MIDDLE aligned text, the label middle is computed after applying
     * padding to the edges.  So the top and bottom padding should be equal
     * to ensure that the text is in the true middle of the label.
     *
     * @param bot   The bottom-edge padding of the label.
     */
    void setPaddingBottom(float bot) { setPadding(_padleft,bot,_padrght,_padtop); }
    
    /**
     * Returns the right-edge padding of the label.
     *
     * Adding right padding to a label reduces the text width of the label,
     * which can affect word wrap (if enabled). In addition, the padding
     * can affect the position of the text. If the text has alignment RIGHT
     * then the label will shift the text left by this amount. On the other
     * hand, it will have no effect on LEFT or JUSTIFY aligned text (other
     * than reducing the line width).
     *
     * For CENTER aligned text, the label center is computed after applying
     * padding to the edges.  So the left and right padding should be equal
     * to ensure that the text is in the true center of the label.
     *
     * @return the right-edge padding of the label.
     */
    float getPaddingRight() const { return _padrght; }
    
    /**
     * Sets the right-edge padding of the label.
     *
     * Adding right padding to a label reduces the text width of the label,
     * which can affect word wrap (if enabled). In addition, the padding
     * can affect the position of the text. If the text has alignment RIGHT
     * then the label will shift the text left by this amount. On the other
     * hand, it will have no effect on LEFT or JUSTIFY aligned text (other
     * than reducing the line width).
     *
     * For CENTER aligned text, the label center is computed after applying
     * padding to the edges.  So the left and right padding should be equal
     * to ensure that the text is in the true center of the label.
     *
     * @param right The right-edge padding of the label.
     */
    void setPaddingRight(float right) { setPadding(_padleft,_padbot,right,_padtop); }
    
    /**
     * Returns the top-edge padding of the label.
     *
     * Adding top padding to a label does not effect word wrap, but it can
     * affect the position of the text. If the text has alignment TOP, the
     * label will shift the text down by this amount. On the other hand,
     * it will have no effect on BOTTOM or BASELINE aligned text.
     *
     * For MIDDLE aligned text, the label middle is computed after applying
     * padding to the edges.  So the top and bottom padding should be equal
     * to ensure that the text is in the true middle of the label.
     *
     * @return the top-edge padding of the label.
     */
    float getPaddingTop() const { return _padtop; }
    
    /**
     * Sets the top-edge padding of the label.
     *
     * Adding top padding to a label does not effect word wrap, but it can
     * affect the position of the text. If the text has alignment TOP, the
     * label will shift the text down by this amount. On the other hand,
     * it will have no effect on BOTTOM or BASELINE aligned text.
     *
     * For MIDDLE aligned text, the label middle is computed after applying
     * padding to the edges.  So the top and bottom padding should be equal
     * to ensure that the text is in the true middle of the label.
     *
     * @param top   The top-edge padding of the label.
     */
    void setPaddingTop(float top) { setPadding(_padleft,_padbot,_padrght,top); }
    
    /**
     * Returns the non-padded interior of this label.
     *
     * Padding can be added to the bottom, left, top, and right edges of the
     * label. This padding detracts from the area in which text can appear,
     * and can influence the word wrap. We call the area of the label in
     * which the text can appear the *interior* of the label.
     *
     * Normally the content bounds of a scene graph node has origin (0,0)
     * and size {@link SceneNode#getContentSize}. This method shifts the
     * origin and reduces the width and height to account for the padding
     * on the edges.
     *
     * @return the non-padded interior of this label.
     */
    const Rect getInterior() const;
    
    /**
     * Sets the non-padded interior of this label.
     *
     * Padding can be added to the bottom, left, top, and right edges of the
     * label. This padding detracts from the area in which text can appear,
     * and can influence the word wrap. We call the area of the label in
     * which the text can appear the *interior* of the label.
     *
     * This method is essentially an alternate way to set the padding. The
     * method will add padding so that the interior appears in a region
     * with origin (0,0) and size {@link SceneNode#getContentSize}. If the
     * interior cannot fit in this region, then the interior will be the
     * intersection. This method will never change the content size of the
     * label.
     *
     * @param rect  The non-padded interior of this label.
     */
    void setInterior(const Rect rect);
    
    /**
     * Returns the bounds of the rendered text.
     *
     * This is the bounds of the rendered text with respect to the Node space.
     * The size of the bounding box is the natural size to render the text.
     * This corresponds to {@link TextLayout#getBounds} of the underlying text
     * layout. In particular, this means there may be some natural spacing
     * around the characters.
     *
     * The origin of the bounds is determined by the padding and alignment.
     * If this rectangle extends outside the bounding box of the label (e.g.
     * the rectangle with origin (0,0) and the content size), then only the 
     * part of the rectangle inside the bounding box will be rendered.
     *
     * @return the bounds of the rendered text.
     */
    const Rect getTextBounds() const;
    
    /**
     * Returns the tightest bounds of the rendered text.
     *
     * This is the bounds of the rendered text, with respect to the Node space.
     * The size of the bounding box ignores any nautural spacing around the
     * characters. It also includes any tracking applied to each line. This
     * corresponds to {@link TextLayout#getTrueBounds} of the underlying text
     * layout.
     *
     * The origin of the bounds is determined by the padding and alignment.
     * If this rectangle extends outside the bounding box of the label (e.g.
     * the rectangle with origin (0,0) and the content size), then only the
     * part of the rectangle inside the bounding box will be rendered.
     *
     * @return the tightest bounds of the rendered text.
     */
    const Rect getTrueBounds() const;
    
    /**
     * Sets the untransformed size of the node.
     *
     * The content size remains the same no matter how the node is scaled or
     * rotated. All nodes must have a size, though it may be degenerate (0,0).
     *
     * Changing the size of a rectangle will not change the position of the
     * node.  However, if the anchor is not the bottom-left corner, it will
     * change the origin.  The Node will grow out from an anchor on an edge,
     * and equidistant from an anchor in the center.
     *
     * In addition, if the rendered text is cannot fit in the content size,
     * it may be cut off in rendering.
     *
     * @param size  The untransformed size of the node.
     */
    virtual void setContentSize(const Size size) override;
    
    /**
     * Sets the untransformed size of the node.
     *
     * The content size remains the same no matter how the node is scaled or
     * rotated. All nodes must have a size, though it may be degenerate (0,0).
     *
     * Changing the size of a rectangle will not change the position of the
     * node.  However, if the anchor is not the bottom-left corner, it will
     * change the origin.  The Node will grow out from an anchor on an edge,
     * and equidistant from an anchor in the center.
     *
     * In addition, if the rendered text is cannot fit in the content size,
     * it may be cut off in rendering.
     *
     * @param width     The untransformed width of the node.
     * @param height    The untransformed height of the node.
     */
    virtual void setContentSize(float width, float height) override {
        setContentSize(Size(width, height));
    }
    
#pragma mark -
#pragma mark Text Coloring
    /**
     * Returns the foreground color of this label.
     *
     * This color will be applied to the characters themselves. This color is
     * BLACK by default.
     *
     * @return the foreground color of this label.
     */
    Color4 getForeground() const { return _foreground; }

    /**
     * Sets the foreground color of this label.
     *
     * This color will be applied to the characters themselves. This color is
     * BLACK by default.
     *
     * @param color The foreground color of this label.
     */
    void setForeground(Color4 color) { _foreground = color; updateColor(); }

    /**
     * Returns the background color of this label.
     *
     * If this color is not CLEAR (the default color), then the label will have 
     * a colored backing rectangle.  The rectangle will extended from the origin 
     * to the content size in Node space.
     *
     * @return the background color of this label.
     */
    Color4 getBackground() const { return _background; }
    
    /**
     * Sets the background color of this label.
     *
     * If this color is not CLEAR (the default color), then the label will have
     * a colored backing rectangle.  The rectangle will extended from the origin
     * to the content size in Node space.
     *
     * @param color The background color of this label.
     */
    void setBackground(Color4 color);
    
    /**
     * Returns the drop shadow offset of this label
     *
     * A drop shadow is a blurred and/or slightly offset version of the label
     * text, drawn behind the original text. The color of the drop shadow is
     * always a slighlty transparent black. It is used to give a sense of
     * depth to the text.
     *
     * This property controls the offset of the drop shadow but does not
     * control the blur. You must use the property {@link #getShadowBlur} for
     * that. A typical drop shadow is offset down and to the right with minor
     * blurring.
     *
     * @return the drop shadow offset of this label
     */
    const Vec2& getDropShadow() const { return _dropOffset; }

    /**
     * Sets the drop shadow offset of this label
     *
     * A drop shadow is a blurred and/or slightly offset version of the label
     * text, drawn behind the original text. The color of the drop shadow is
     * always a slighlty transparent black. It is used to give a sense of
     * depth to the text.
     *
     * This property controls the offset of the drop shadow but does not
     * control the blur. You must use the property {@link #getShadowBlur} for
     * that. A typical drop shadow is offset down and to the right with minor
     * blurring.
     *
     * @param p     The drop shadow offset of this label
     */
    void setDropShadow(const Vec2 p) { setDropShadow(p.x,p.y); }

    /**
     * Sets the drop shadow offset of this label
     *
     * A drop shadow is a blurred and/or slightly offset version of the label
     * text, drawn behind the original text. The color of the drop shadow is
     * always a slighlty transparent black. It is used to give a sense of
     * depth to the text.
     *
     * This property controls the offset of the drop shadow but does not
     * control the blur. You must use the property {@link #getShadowBlur} for
     * that. A typical drop shadow is offset down and to the right with minor
     * blurring.
     *
     * @param x     The x-offset of the drop shadow
     * @param y     The x-offset of the drop shadow
     */
    void setDropShadow(float x, float y);

    /**
     * Returns the drop shadow blur of this label
     *
     * A drop shadow is a blurred and/or slightly offset version of the label
     * text, drawn behind the original text. The color of the drop shadow is
     * always a slighlty transparent black. It is used to give a sense of
     * depth to the text.
     *
     * This property controls the offset of the drop shadow but does not
     * control the blur. You must use the property {@link #getShadowBlur} for
     * that. A typical drop shadow is offset down and to the right with minor
     * blurring.
     *
     * When blurring a drop shadow, remember to use a font with the same
     * {@link Font#getPadding} as the blur size. This will prevent bleeding
     * across characters in the atlas.
     *
     * @return the drop shadow blur of this label
     */
    float getShadowBlur() const { return _dropBlur; }
    
    /**
     * Sets the drop shadow blur of this label
     *
     * A drop shadow is a blurred and/or slightly offset version of the label
     * text, drawn behind the original text. The color of the drop shadow is
     * always a slighlty transparent black. It is used to give a sense of
     * depth to the text.
     *
     * This property controls the offset of the drop shadow but does not
     * control the blur. You must use the property {@link #getShadowBlur} for
     * that. A typical drop shadow is offset down and to the right with minor
     * blurring.
     *
     * When blurring a drop shadow, remember to use a font with the same
     * {@link Font#getPadding} as the blur size. This will prevent bleeding
     * across characters in the atlas.
     *
     * @param blur  The drop shadow blur of this label
     */
    void setShadowBlur(float blur);
    
    /**
     * Sets the blending function for this texture node.
     *
     * The enums are the standard ones supported by OpenGL.  See
     *
     *      https://www.opengl.org/sdk/docs/man/html/glBlendFunc.xhtml
     *
     * However, this setter does not do any error checking to verify that
     * the enums are valid.  By default, srcFactor is GL_SRC_ALPHA while
     * dstFactor is GL_ONE_MINUS_SRC_ALPHA. This corresponds to non-premultiplied
     * alpha blending.
     *
     * This blending factor only affects the texture of the current node.  It
     * does not affect any children of the node.
     *
     * @param srcFactor Specifies how the source blending factors are computed
     * @param dstFactor Specifies how the destination blending factors are computed.
     */
    void setBlendFunc(GLenum srcFactor, GLenum dstFactor) { _srcFactor = srcFactor; _dstFactor = dstFactor; }
    
    /**
     * Returns the source blending factor
     *
     * By default this value is GL_SRC_ALPHA. For other options, see
     *
     *      https://www.opengl.org/sdk/docs/man/html/glBlendFunc.xhtml
     *
     * This blending factor only affects the texture of the current node.  It
     * does not affect any children of the node.
     *
     * @return the source blending factor
     */
    GLenum getSourceBlendFactor() const { return _srcFactor; }
    
    /**
     * Returns the destination blending factor
     *
     * By default this value is GL_ONE_MINUS_SRC_ALPHA. For other options, see
     *
     *      https://www.opengl.org/sdk/docs/man/html/glBlendFunc.xhtml
     *
     * This blending factor only affects the texture of the current node.  It
     * does not affect any children of the node.
     *
     * @return the destination blending factor
     */
    GLenum getDestinationBlendFactor() const { return _srcFactor; }
    
    /**
     * Sets the blending equation for this textured node
     *
     * The enum must be a standard ones supported by OpenGL.  See
     *
     *      https://www.opengl.org/sdk/docs/man/html/glBlendEquation.xhtml
     *
     * However, this setter does not do any error checking to verify that
     * the input is valid.  By default, the equation is GL_FUNC_ADD.
     *
     * This blending equation only affects the texture of the current node.  It
     * does not affect any children of the node.
     *
     * @param equation  Specifies how source and destination colors are combined
     */
    void setBlendEquation(GLenum equation) { _blendEquation = equation; }
    
    /**
     * Returns the blending equation for this textured node
     *
     * By default this value is GL_FUNC_ADD. For other options, see
     *
     *      https://www.opengl.org/sdk/docs/man/html/glBlendEquation.xhtml
     *
     * This blending equation only affects the texture of the current node.  It
     * does not affect any children of the node.
     *
     * @return the blending equation for this sprite batch
     */
    GLenum getBlendEquation() const { return _blendEquation; }

#pragma mark -
#pragma mark Rendering
    /**
     * Draws this Node via the given SpriteBatch.
     *
     * This method only worries about drawing the current node.  It does not
     * attempt to render the children.
     *
     * This is the method that you should override to implement your custom
     * drawing code.  You are welcome to use any OpenGL commands that you wish.
     * You can even skip use of the SpriteBatch.  However, if you do so, you
     * must flush the SpriteBatch by calling end() at the start of the method.
     * in addition, you should remember to call begin() at the start of the
     * method.
     *
     * This method provides the correct transformation matrix and tint color.
     * You do not need to worry about whether the node uses relative color.
     * This method is called by render() and these values are guaranteed to be
     * correct.  In addition, this method does not need to check for visibility,
     * as it is guaranteed to only be called when the node is visible.
     *
     * @param batch     The SpriteBatch to draw with.
     * @param transform The global transformation matrix.
     * @param tint      The tint to blend with the Node color.
     */
    virtual void draw(const std::shared_ptr<SpriteBatch>& batch, const Affine2& transform, Color4 tint) override;
    
protected:
    /**
     * Allocates the render data necessary to render this node.
     */
    virtual void generateRenderData();
    
    /**
     * Clears the render data, releasing all vertices and indices.
     */
    void clearRenderData();
    
    /**
     * Updates the color value for any other data that needs it.
     *
     * This method is used to synchronize the background and foreground
     * colors.
     */
    void updateColor();

    /**
     * Resizes the content bounds to fit the text.
     */
    void resize();
    
    /**
     * Repositions the text inside of this label.
     *
     * This method is called whenever there is a formatting or alignment
     * change to the label.
     */
    void reanchor();

};
    }  
}
#endif /* __CU_LABEL_H__ */
