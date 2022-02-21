//
//  CUTextLayout.h
//  Cornell University Game Library (CUGL)
//
//  This module is used to solve two problems. First, it allows us to finally
//  support multi-line text, which was missing in earlier versions of CUGL
//  (and has been one of the most requested features). More importantly, it
//  allows us to decouple text formatting from the Label class. Because of
//  this class we can draw text directly to a sprite batch without having to
//  use the scene graph API.
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
//  Version: 7/29/21
//
#ifndef __CU_TEXT_LAYOUT_H__
#define __CU_TEXT_LAYOUT_H__

#include <cugl/math/CURect.h>
#include <cugl/render/CUTextAlignment.h>
#include <unordered_map>
#include <vector>
#include <string>

namespace cugl {

/** Forward declarations to reduced includes */
class Font;
class GlyphRun;

namespace scene2 {
    class Label;
    class TextField;
}

/**
 * This class manages the layout of (potentially) multiple lines of text.
 *
 * The purpose of this class is to decouple text layout from the {@link scene2::Label}
 * scene graph class, so we can draw text directly to a {@link SpriteBatch}.
 * Given a string, it potentially breaks the string up into multiple lines and
 * allows you to control the relative alignment of each line.
 *
 * In addition, a text layout object has an implicit coordinate system with
 * an origin. This origin is defined by the {@link #getHorizontalAlignment}
 * and {@link #getVerticalAlignment} options. This origin is used to place
 * the text when it is rendered with a sprite batch.
 *
 * Changing any of the layout attributes will obviously invalidate the text
 * layout. For performance reasons, we do not automatically recompute the
 * layout in that case. Instead, the user must call {@link #layout} to
 * arrange the text.
 *
 * By default, the text layout will only break lines at newline characters in
 * the string. However, you can perform more agressive line breaking with the
 * optional {@link #getWidth} attribute.  When this attribute is positive, the
 * text layout will break lines so that each line does not exceed this width.
 *
 * Lines will be broken at the last white space character found before exceeding
 * the width. If there is no such whitespace character, it will break the line
 * before the first character exceeding the width. While this class does not
 * support more sophisticated line breaking like hyphenation, the end result is
 * good enough for most in-game multi-line text support.
 *
 * When formatting multiline text, whitespace at the beginning and end of each
 * line will be "swallowed", causing it to be ignored for purposes of alignment.
 * The exception is at the beginning and end of a paragraph.  Whitespace there
 * will be preserved.  A paragraph is defined as any piece of text separated by
 * a newline. So the first part of the string before a newline is a paragraph,
 * and each substring after a newline is also a paragraph.
 *
 * Finally, it is possible to disable all line breaking in a text layout
 * (including newlines). Simply set the width to a negative value.
 */
class TextLayout {
private:
#pragma mark Rows
    /**
     * This inner class represents a single line of text.
     *
     * These objects are generated whenever either the text or the font changes.
     * This class refers back to the original string via indices.
     */
    class Row {
    public:
        /** Position of the start of the (sub)string */
        size_t begin;
        /** Position of the end of the (sub)string */
        size_t end;
        /** Whether this row is the start of a paragraph */
        bool paragraph;
        /** The natural bounds of this line (including font ascent and descent) */
        Rect exterior;
        /** The tight bounds of this line, ignoring font-specific padding */
        Rect interior;
        
        /**
         * Creates a new (empty) row with default values.
         */
        Row();
        
        /**
         * Deletes the contents of this row
         */
        ~Row();
    };

    /** The rows of this text layout.  May be empty if no layout is performed. */
    std::vector<Row> _rows;
    /** The text stored in this text layout */
    std::string _text;
    /** The font laying out this text */
    std::shared_ptr<Font> _font;
    
    /**
     * The width of this text layout.
     *
     * Set this value to 0 to force line breaks only at newlines. Set this to a
     * negative value to disable line breaking completely.
     */
    float  _breakline;
    /**
     * The line spacing of this layout.
     *
     * This has the standard typography interpretation. 1 is single space, while
     * 2 is double space.
     */
    float  _spacing;

    /** The bounds of this text layout */
    Rect _bounds;
    /** The horizontal alignment of the text in this layout */
    HorizontalAlign _halign;
    /** The vertical alignment of the text layout */
    VerticalAlign _valign;
    
public:
#pragma mark -
#pragma mark Constructors
    /**
     * Creates a degenerate text layout with no data.
     *
     * NEVER USE A CONSTRUCTOR WITH NEW. If you want to allocate an object on
     * the heap, use one of the static constructors instead.
     */
	TextLayout();

    /**
     * Deletes this text layout, disposing of all resources.
     */
	~TextLayout() { dispose(); }
    
    /**
     * Deletes the layout resources and resets all attributes.
     *
     * You must reinitialize the text layout to use it.
     */
    void dispose();

    /**
     * Initializes an empty text layout
     *
     * You will need to add both text and a font, as well as call the
     * method {@link #layout} to properly use this object.
     *
     * This layout will have a size of 0, which menas that this method
     * will only break lines at newlines. This guarantees multi-line
     * text support without taking control away from the programmer.
     *
     * @return true if initialization is successful.
     */
    bool init();
    
    /**
     * Initializes an empty text layout with the given width.
     *
     * You will need to add both text and a font, as well as call the
     * method {@link #layout} to properly use this object.
     *
     * A width of 0 will guarantee that breaks only happen at newlines,
     * while a negative width means that line breaks never happen at all.
     * A postive width will force the layout manager to break up lines
     * so that no individual line exceeds that width.
     *
     * @param width The width for each line
     *
     * @return true if initialization is successful.
     */
    bool initWithWidth(float width);

    /**
     * Initializes a text layout with the given text and font.
     *
     * This layout will have a size of 0, which menas that this method
     * will only break lines at newlines. This guarantees multi-line
     * text support without taking control away from the programmer.
     *
     * Note that this method does not actually arrange the text. You
     * must call {@link #layout} to complete the layout.  This gives
     * you time to change the horizontal or vertical alignment before
     * performing the layout.
     *
     * @param text  The text to layout
     * @param font  The font to arrange the text
     *
     * @return true if initialization is successful.
     */
    bool initWithText(const std::string text, const std::shared_ptr<Font>& font);

    /**
     * Initializes a text layout with the given text, font, and width.
     *
     * A width of 0 will guarantee that breaks only happen at newlines,
     * while a negative width means that line breaks never happen at all.
     * A postive width will force the layout manager to break up lines
     * so that no individual line exceeds that width.
     *
     * Note that this method does not actually arrange the text. You
     * must call {@link #layout} to complete the layout.  This gives
     * you time to change the horizontal or vertical alignment before
     * performing the layout.
     *
     * @param text  The text to layout
     * @param font  The font to arrange the text
     * @param width The width for each line
     *
     * @return true if initialization is successful.
     */
    bool initWithTextWidth(const std::string text, const std::shared_ptr<Font>& font, float width);

    /**
     * Returns a newly allocated empty text layout
     *
     * You will need to add both text and a font, as well as call the
     * method {@link #layout} to properly use this object.
     *
     * This layout will have a size of 0, which menas that this method
     * will only break lines at newlines. This guarantees multi-line
     * text support without taking control away from the programmer.
     *
     * @return a newly allocated empty text layout
     */
    static std::shared_ptr<TextLayout> alloc() {
        std::shared_ptr<TextLayout> result = std::make_shared<TextLayout>();
        return (result->init() ? result : nullptr);
    }

    /**
     * Returns a newly allocated empty text layout with the given width.
     *
     * You will need to add both text and a font, as well as call the
     * method {@link #layout} to properly use this object.
     *
     * A width of 0 will guarantee that breaks only happen at newlines,
     * while a negative width means that line breaks never happen at all.
     * A postive width will force the layout manager to break up lines
     * so that no individual line exceeds that width.
     *
     * @param width The width for each line
     *
     * @return a newly allocated empty text layout with the given width.
     */
    static std::shared_ptr<TextLayout> allocWithWidth(float width) {
        std::shared_ptr<TextLayout> result = std::make_shared<TextLayout>();
        return (result->initWithWidth(width) ? result : nullptr);
    }
    
    /**
     * Returns a newly allocated text layout with the given text and font
     *
     * This layout will have a size of 0, which menas that this method
     * will only break lines at newlines. This guarantees multi-line
     * text support without taking control away from the programmer.
     *
     * Note that this method does not actually arrange the text. You
     * must call {@link #layout} to complete the layout.  This gives
     * you time to change the horizontal or vertical alignment before
     * performing the layout.
     *
     * @param text  The text to layout
     * @param font  The font to arrange the text
     *
     * @return a newly allocated text layout with the given text and font
     */
    static std::shared_ptr<TextLayout> allocWithText(const std::string text, const std::shared_ptr<Font>& font) {
        std::shared_ptr<TextLayout> result = std::make_shared<TextLayout>();
        return (result->initWithText(text,font) ? result : nullptr);
    }

    /**
     * Returns a newly allocated text layout with the given text, font, and width.
     *
     * A width of 0 will guarantee that breaks only happen at newlines,
     * while a negative width means that line breaks never happen at all.
     * A postive width will force the layout manager to break up lines
     * so that no individual line exceeds that width.
     *
     * Note that this method does not actually arrange the text. You
     * must call {@link #layout} to complete the layout.  This gives
     * you time to change the horizontal or vertical alignment before
     * performing the layout.
     *
     * @param text  The text to layout
     * @param font  The font to arrange the text
     * @param width The width for each line
     *
     * @return a newly allocated text layout with the given text, font, and width.
     */
    static std::shared_ptr<TextLayout> allocWithTextWidth(const std::string text, const std::shared_ptr<Font>& font, float width) {
        std::shared_ptr<TextLayout> result = std::make_shared<TextLayout>();
        return (result->initWithTextWidth(text,font,width) ? result : nullptr);
    }
 
    
#pragma mark -
#pragma mark Attributes
    /**
     * Returns the text associated with this layout.
     *
     * Changing this value will {@link #invalidate} the layout.
     *
     * @return the text associated with this layout.
     */
    const std::string& getText() const { return _text; }
    
    /**
     * Sets the text associated with this layout.
     *
     * Changing this value will {@link #invalidate} the layout.
     *
     * @param text  The text associated with this layout.
     */
    void setText(const std::string text);

    /**
     * Returns the font associated with this layout.
     *
     * Changing this value will {@link #invalidate} the layout.
     *
     * @return the font associated with this layout.
     */
    const std::shared_ptr<Font>& getFont() const { return _font; }
    
    /**
     * Sets the font associated with this layout.
     *
     * Changing this value will {@link #invalidate} the layout.
     *
     * @param font  The font associated with this layout.
     */
    void setFont(const std::shared_ptr<Font>& font);
    
    /**
     * Returns the line width of this layout.
     *
     * This value will determine how the layout breaks up lines to arrange
     * text. A width of 0 will guarantee that breaks only happen at newlines,
     * while a negative width means that line breaks never happen at all. A
     * postive width will force the text layout to break up lines so that no
     * individual line exceeds that width.
     *
     * Changing this value will {@link #invalidate} the layout.
     *
     * @return the line width of this layout.
     */
    float getWidth() const { return _breakline; }
    
    /**
     * Sets the line width of this layout.
     *
     * This value will determine how the layout breaks up lines to arrange
     * text. A width of 0 will guarantee that breaks only happen at newlines,
     * while a negative width means that line breaks never happen at all. A
     * postive width will force the text layout to break up lines so that no
     * individual line exceeds that width.
     *
     * Changing this value will {@link #invalidate} the layout.
     *
     * @param width     The line width of this layout.
     */
    void setWidth(float width);

    /**
     * Returns the line spacing of this layout.
     *
     * This value is multiplied by the font size to determine the space
     * between lines in the layout. So a value of 1 is single-spaced text,
     * while a value of 2 is double spaced. The value should be positive.
     *
     * Changing this value will {@link #invalidate} the layout.
     *
     * @return the line spacing of this layout.
     */
    float getSpacing() const { return _spacing; }
    
    /**
     * Sets the line spacing of this layout.
     *
     * This value is multiplied by the font size to determine the space
     * between lines in the layout. So a value of 1 is single-spaced text,
     * while a value of 2 is double spaced. The value should be positive.
     *
     * Changing this value will {@link #invalidate} the layout.
     *
     * @param spacing   The line spacing of this layout.
     */
    void setSpacing(float spacing);
    
    /**
     * Returns the horizontal alignment of the text.
     *
     * The horizontal alignment has two meanings. First, it is the relationship
     * of the relative alignment of multiple lines. In addition, it defines the
     * x-coordinate origin of the text layout. The later is relevant even when
     * the text layout is a single line.
     *
     * Changing this value will {@link #invalidate} the layout.
     *
     * @return the horizontal alignment of the text.
     */
    HorizontalAlign getHorizontalAlignment() const { return _halign; }

    /**
     * Sets the horizontal alignment of the text.
     *
     * The horizontal alignment has two meanings. First, it is the relationship
     * of the relative alignment of multiple lines. In addition, it defines the
     * x-coordinate origin of the text layout. The later is relevant even when
     * the text layout is a single line.
     *
     * Changing this value will {@link #invalidate} the layout.
     *
     * @param halign    The horizontal alignment of the text.
     */
    void setHorizontalAlignment(HorizontalAlign halign);

    /**
     * Returns the vertical alignment of the text.
     *
     * The vertical alignment defines the y-coordinate origin of this text layout.
     * In the case of multiple lines, the alignment is (often) with respect to the
     * entire block of text, not just the first line.
     *
     * Changing this value will {@link #invalidate} the layout.
     *
     * @return the vertical alignment of the text.
     */
    VerticalAlign getVerticalAlignment() const { return _valign; }
    
    /**
     * Sets the vertical alignment of the text.
     *
     * The vertical alignment defines the y-coordinate origin of this text layout.
     * In the case of multiple lines, the alignment is (often) with respect to the
     * entire block of text, not just the first line.
     *
     * Changing this value will {@link #invalidate} the layout.
     *
     * @param valign    The vertical alignment of the text.
     */
    void setVerticalAlignment(VerticalAlign valign);
    
#pragma mark -
#pragma mark Layout Processing
    /**
     * Arranges the text according to the given font and settings
     *
     * Changing any of the layout attributes will obviously invalidate the text
     * layout. For performance reasons, we do not automatically recompute the
     * layout in that case. Instead, the user must call {@link #layout} to
     * arrange the text.
     */
    void layout();

    /**
     * Invalidates the text layout
     *
     * This deletes all rows (so the line count is 0). You will need to call
     * {@link #layout} to reperform the layout.
     */
    void invalidate();
    
    /**
     * Returns true if the layout has been successful.
     *
     * This method will return false unless {@link #layout} has been called.
     *
     * @return true if the layout has been successful.
     */
    bool validated() const {
        return _rows.size() > 0;
    }
    
    /**
     * Returns the bounds of this text layout
     *
     * This rectangle is in the coordinate space whose origin is defined by the
     * horizontal and vertical alignment. This rectangle has zero size if the
     * {@link #layout} method has not been called or if the layout has been
     * invalidated.
     *
     * @return the bounds of this text layout
     */
    const Rect& getBounds() const { return _bounds; }
    
    /**
     * Returns the tightest bounds of the text layout
     *
     * This rectangle is in the coordinate space whose origin is defined by
     * the horizontal and vertical alignment. This rectangle has zero size if
     * the {@link #layout} method has not been called or if the layout has been
     * invalidated.
     *
     * Unlike {@link #getBounds}, this rectangle sits tight against the text,
     * ignoring any natural spacing such as the ascent or descent. However, it
     * does not include any tracking that the layout may apply.
     *
     * @return the tightest bounds of the text layout
     */
    const Rect getTightBounds() const;
    
    /**
     * Returns the true bounds of the text layout, including tracking
     *
     * This rectangle is in the coordinate space whose origin is defined by
     * the horizontal and vertical alignment. This rectangle has zero size if
     * the {@link #layout} method has not been called or if the layout has been
     * invalidated.
     *
     * The method is similar to {@link #getTightBounds}, except that it also
     * includes any tracking that is applied to the layout.
     *
     * @return the true bounds of the text layout, including tracking
     */
    const Rect getTrueBounds() const;
    
    /**
     * Returns the number of lines in this text layout
     *
     * This value will be zero if {@link #layout} has not been called or if the
     * layout has been invalidated.
     *
     * @return the number of lines in this text layout
     */
    size_t getLineCount() const { return _rows.size(); }

    /**
     * Returns the text for the given line.
     *
     * Note that line breaking will omit any white space on the ends. Hence
     * adding the text for each line together may not produce the original
     * text.
     *
     * @param line	The line number
     *
     * @return the text for the given line.
     */
    std::string getLine(size_t line) const;
    
#pragma mark -
#pragma mark Glyph Querying
    /**
     * Returns the glyph bounds of the character at the given index
     *
     * The rectangle will be in the coordinate system of this text layout. In
     * addition to the size, it will accurately reflect the position of the
     * character in the layout, including any possible tracking.
     *
     * The index represents a position in the layout text string. The index
     * must be the first byte of a valid UTF8 character. If it is a successive
     * byte (and hence undecodable as a unicode character), this method will
     * return the empty rectangle.
     *
     * @param index The position in the layout text
     *
     * @return the glyph bounds of the character at the given index
     */
    Rect getGlyphBounds(size_t index) const;

    /**
     * Returns the index of the character whose glyph is located at p
     *
     * If the point p is not on top of a glyph, this method will return the
     * size of the text. Use {@link #getNearestIndex} for cases in which the
     * point is out of bounds.
     *
     * The point p is assumed to be in the coordinate system of this layout.
     * This method will never return the index of white space "swallowed" at
     * the end of multiline text, even when this point is beyond the edges
     * of the text.
     *
     * @param p     The position in layout space
     *
     * @return the index of the character whose glyph is located at p
     */
    size_t getGlyphIndex(Vec2 p) const { return getGlyphIndex(p.x,p.y); }

    /**
     * Returns the index of the character whose glyph is located at (x,y)
     *
     * If the point (x,y) is not on top of a glyph, this method will return
     * the size of the text. Use {@link #getNearestIndex} for cases in which
     * the point is out of bounds.
     *
     * The point (x,y) is assumed to be in the coordinate system of this
     * layout. This method will never return the index of white space
     * "swallowed" at the end of multiline text, even when this point is
     * beyond the edges of the text.
     *
     * @param x     The x-coordinate in layout space
     * @param y     The y-coordinate in layout space
     *
     * @return the index of the character whose glyph is located at (x,y)
     */
    size_t getGlyphIndex(float x, float y) const;
    
    /**
     * Returns the index of the character whose glyph is nearest p
     *
     * The point p is assumed to be in the coordinate system of this layout.
     * This method will never return the index of white space "swallowed" at
     * the end of multiline text, even when this point is beyond the edges
     * of the text.
     *
     * @param p     The position in layout space
     *
     * @return the index of the character whose glyph is nearest p
     */
    size_t getNearestIndex(Vec2 p) const { return getNearestIndex(p.x,p.y); }

    /**
     * Returns the index of the character whose glyph is nearest (x,y)
     *
     * The point (x,y) is assumed to be in the coordinate system of this
     * layout. This method will never return the index of white space
     * "swallowed" at the end of multiline text, even when this point is
     * beyond the edges of the text.
     *
     * @param x     The x-coordinate in layout space
     * @param y     The y-coordinate in layout space
     *
     * @return the index of the character whose glyph is nearest (x,y)
     */
    size_t getNearestIndex(float x, float y) const;

    
#pragma mark -
#pragma mark Glyph Generation
    /**
     * Returns a set of glyph runs to render the text layout
     *
     * Each glyph run will consist of a quad mesh and a texture to render
     * those quads. Rendering all of the glyph runs together will render
     * the entire string.  Generally the quads are non-overlapping, so
     * any blending mode is supported. However, if the atlas padding is
     * non-zero (to support font blur), the quads will overlap at the
     * padding intervals.  Therefore, we recommend alpha blending when
     * you render a string.
     *
     * The keys for the glyph runs are the {@link Texture#getBuffer} values
     * for the appropriate atlas texture. This, combined with the method
     * {@link Font#getAtlases} allows you to identify the atlas for each run.
     *
     * The origin of the glyph runs will agree with that of the text layout.
     * This method will return the empty map if {@link #layout} has not been
     * called or the layout has been invalidated.
     *
     * @return a set of glyph runs to render the text layout
     */
    std::unordered_map<GLuint,std::shared_ptr<GlyphRun>> getGlyphs() const;
    
    /**
     * Returns a set of glyph runs to render the text layout
     *
     * Each glyph run will consist of a quad mesh and a texture to render
     * those quads. Rendering all of the glyph runs together will render
     * the entire string.  Generally the quads are non-overlapping, so
     * any blending mode is supported. However, if the atlas padding is
     * non-zero (to support font blur), the quads will overlap at the
     * padding intervals.  Therefore, we recommend alpha blending when
     * you render a string.
     *
     * The keys for the glyph runs are the {@link Texture#getBuffer} values
     * for the appropriate atlas texture. This, combined with the method
     * {@link Font#getAtlases} allows you to identify the atlas for each run.
     *
     * The quad sequence is adjusted so that all of the vertices fit in the
     * provided rectangle. The primary use-case for this is to guarantee
     * that glyphs do not spill outside of a window. This may mean that some
     * of the glyphs will be truncrated or even omitted.
     *
     * The origin of the glyph runs will agree with that of the text layout.
     * This method will return the empty map if {@link #layout} has not been
     * called or the layout has been invalidated.
     *
     * @param rect      The bounding box for the quads
     *
     * @return a set of glyph runs to render the text layout
     */
    std::unordered_map<GLuint,std::shared_ptr<GlyphRun>> getGlyphs(Rect rect) const;

    /**
     * Stores the glyph runs to render the text layout in the given map
     *
     * Each glyph run will consist of a quad mesh and a texture to render
     * those quads. Rendering all of the glyph runs together will render
     * the entire string.  Generally the quads are non-overlapping, so
     * any blending mode is supported. However, if the atlas padding is
     * non-zero (to support font blur), the quads will overlap at the
     * padding intervals.  Therefore, we recommend alpha blending when
     * you render a string.
     *
     * The keys for the glyph runs are the {@link Texture#getBuffer} values
     * for the appropriate atlas texture. This, combined with the method
     * {@link Font#getAtlases} allows you to identify the atlas for each run.
     * If the map is non-empty, the glyph run data will be appended to the
     * relevant existing glyph run (if possible).
     *
     * The origin of the glyph runs will agree with that of the text layout.
     * This method will do nothing if {@link #layout} has not been called or
     * the layout has been invalidated.
     *
     * @param runs      The map to store the glyph runs
     *
     * @return the number of glyphs successfully processed
     */
    size_t getGlyphs(std::unordered_map<GLuint,std::shared_ptr<GlyphRun>>& runs) const;
    
    /**
     * Stores the glyph runs to render the text layout in the given map
     *
     * Each glyph run will consist of a quad mesh and a texture to render
     * those quads. Rendering all of the glyph runs together will render
     * the entire string.  Generally the quads are non-overlapping, so
     * any blending mode is supported. However, if the atlas padding is
     * non-zero (to support font blur), the quads will overlap at the
     * padding intervals.  Therefore, we recommend alpha blending when
     * you render a string.
     *
     * The keys for the glyph runs are the {@link Texture#getBuffer} values
     * for the appropriate atlas texture. This, combined with the method
     * {@link Font#getAtlases} allows you to identify the atlas for each run.
     * If the map is non-empty, the glyph run data will be appended to the
     * relevant existing glyph run (if possible).
     *
     * The quad sequence is adjusted so that all of the vertices fit in the
     * provided rectangle. The primary use-case for this is to guarantee
     * that glyphs do not spill outside of a window. This may mean that some
     * of the glyphs will be truncrated or even omitted.
     *
     * The origin of the glyph runs will agree with that of the text layout.
     * This method will do nothing if {@link #layout} has not been called or
     * the layout has been invalidated.
     *
     * @param runs      The map to store the glyph runs
     * @param rect      The bounding box for the quads
     *
     * @return the number of glyphs successfully processed
     */
    size_t getGlyphs(std::unordered_map<GLuint,std::shared_ptr<GlyphRun>>& runs, Rect rect) const;
    
#pragma mark -
#pragma mark Layout Methods
private:
    /**
     * Breaks up the text into multiple lines.
     *
     * When breaking up lines, whitespace at the beginning and end of each
     * line will be "swallowed", causing it to be ignored for purposes of
     * alignment. The exception is at the beginning and end of a paragraph.
     * Whitespace there will be preserved. A paragraph is defined as any
     * piece of text separated by a newline. So the first part of the string
     * before a newline is a paragraph, and each substring after a newline
     * is also a paragraph.
     *
     * This algorithm in this method is heavily inspired by nanovg by Mikko
     * Mononen (memon@inside.org). However, this version includes many
     * optimizations as well as the paragraph-specific behavior (which is
     * more natural for editable text).
     *
     * This method will not be called if the width is negative.
     */
    void breakLines();
    
    /**
     * Resets the horizontal alignment.
     *
     * This method recomputes the horizontal position of each line.
     */
    void resetHorizontal();

    /**
     * Resets the vertical alignment.
     *
     * This method recomputes the vertical position of each line.
     */
    void resetVertical();

    /**
     * Recomputes the bounding box of this text layout
     */
    void computeBounds();
    
    /**
     * Recomputes the size of the given row, indicating if it is overwidth.
     *
     * This method is useful for when insertions are made into the middle
     * of text. It will not break up the row, but will indicated if the
     * row should be broken up.
     *
     * Note that this method will adjust the rectangles to fit the row, but
     * it will not apply any horizontal or vertical alignment.
     *
     * @param row   The row to recompute
     *
     * @return true if the new size is overwidth
     */
    bool resizeRow(size_t row);
    
    /**
     * Returns true if this row applies tracking.
     *
     * Tracking is applied if the text is multiline and either justified or
     * squeezed to fit within a line.
     *
     * @param row   The row to check
     *
     * @return true if this row applies tracking.
     */
    bool doesTrack(size_t row) const;

    /** Allow label access for fine-tuned control */
    friend class cugl::scene2::Label;
    friend class cugl::scene2::TextField;

};

}
#endif /* __CU_TEXT_LAYOUT_H__ */
