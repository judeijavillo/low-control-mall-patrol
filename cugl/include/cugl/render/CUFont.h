//
//  CUFont.h
//  Cornell University Game Library (CUGL)
//
//  This module provides a robust font asset with atlas support.  As with
//  most font atlas set-ups, a font may spread its characters over multiple
//  atlases. That is to prevent the textures from getting too large.
//
//  This module makes heavy use of the cross-platform UTF8 utilities by
//  Nemanja Trifunovic ( https://github.com/nemtrif/utfcpp ).
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
//  Version: 3/1/21

#ifndef __CU_FONT_H__
#define __CU_FONT_H__

#include <string>
#include <unordered_map>
#include <vector>
#include <cugl/math/CUSize.h>
#include <cugl/math/CURect.h>
#include <cugl/math/CUColor4.h>
#include <cugl/render/CUTexture.h>
#include <cugl/render/CUSpriteVertex.h>
#include <cugl/render/CUMesh.h>
#include <cugl/render/CUGlyphRun.h>
#include <SDL/SDL_ttf.h>

namespace cugl {

/**
 * This class represents a true type font at a fixed size.
 *
 * It is possible to change many of the font settings after loading.  However,
 * the size is fixed and cannot be changed.  If you want a different size of
 * the same font, you must load it as a new asset.
 *
 * This font can be used to generate {@link GlyphRun} sequences for rendering
 * text to the screen. This is typically done via the {@link scene2::Label} class,
 * though the a glyph run contains enough information to be rendered directly
 * with a {@link SpriteBatch}.
 *
 * Rendering ASCII text is easy.  For unicode support however, you need to 
 * encode you text properly. the only unicode encoding that we support is UTF8.  
 * For the reason why, see
 *
 *      http://utf8everywhere.org/#how
 *
 * This font can also support atlases.  This is a texture with all of the
 * selected characters prerendered.  This texture is then used to render the
 * font on screen.  This is a potentially fast way of rendering fonts, as
 * you can simply represent the text as a quad mesh with a single texture.
 *
 * However, afont atlas textures can be huge if all glyphs are included.
 * For example, if we include all the unicode characters in Times New Roman at
 * 48 point font, the resulting atlas texture is 2048x4096, which is too much
 * for mobile devices. As a result, fonts may spread their glyphs across
 * multiple altases to keep the texture size small.
 *
 * In addition, only ASCII characters are included in a font atlas by default.
 * To get unicode characters outside of the ASCII range, you must specify
 * them when you build the atlas.
 */
class Font {
#pragma mark Attribute Classes
public:
    /**
     * This class is a simple struct to store glyph metric data
     *
     * A glyph metric stores the bounding box of a glyph, pluse the spacing
     * information around it. The bouding box is offset from an origin, and
     * the advance the distance to the next glyph origin.  For more information,
     * see https://www.libsdl.org/projects/SDL_ttf/docs/SDL_ttf.html#SEC38
     */
    class Metrics {
    public:
        /** The minimum x-offset of the glyph from the origin (left side) */
        int minx;
        /** The maximum x-offset of the glyph from the origin (right side) */
        int maxx;
        /** The minimum y-offset of the glyph from the origin (bottom edge) */
        int miny;
        /** The maximum y-offset of the glyph from the origin (top edge) */
        int maxy;
        /** The distance from the origin of this glyph to the next */
        int advance;
    };

    /**
     * This enum represents the possible font styles.
     *
     * Generally, these styles would be encoded in the font face, but they
     * are provided to allow the user some flexibility with any font.
     *
     * With the exception of normal style (which is an absent of any style), all
     * of the styles may be combined.  So it is possible to have a bold, italic,
     * underline font with strikethrough. To combine styles, simply treat the
     * Style value as a bitmask, and combine them with bitwise operations.
     */
    enum class Style : int {
        /** The default style provided by this face */
        NORMAL      = TTF_STYLE_NORMAL,
        /** An adhoc created bold style */
        BOLD        = TTF_STYLE_BOLD,
        /** An adhoc created italics style */
        ITALIC      = TTF_STYLE_ITALIC,
        /** An adhoc created underline style */
        UNDERLINE   = TTF_STYLE_UNDERLINE,
        /** An adhoc created strike-through style */
        STRIKE      = TTF_STYLE_STRIKETHROUGH
    };
    
    /**
     * This enum represents the hints for rasterization.
     *
     * Hinting is used to align the font to a rasterized grid. At low screen
     * resolutions, hinting is critical for producing clear, legible text
     * (particularly if you are not supporting antialiasing).
     */
    enum class Hinting : int {
        /**
         * This corresponds to the default hinting algorithm, optimized for
         * standard gray-level rendering
         */
        NORMAL  = TTF_HINTING_NORMAL,
        /**
         * This is a lighter hinting algorithm for non-monochrome modes. Many
         * generated glyphs are more fuzzy but better resemble its original
         * shape. This is a bit like rendering on Mac OS X.
         */
        LIGHT   = TTF_HINTING_LIGHT,
        /**
         * This is a strong hinting algorithm that should only be used for
         * monochrome output. The result is probably unpleasant if the glyph
         * is rendered in non-monochrome modes.
         */
        MONO    = TTF_HINTING_MONO,
        /**
         * In this case, no hinting is used so the font may become very blurry
         * or messy at smaller sizes.
         */
        NONE    = TTF_HINTING_NONE
    };
    
#pragma mark -
#pragma mark Font Atlas
protected:
    /**
     * This class represents a single font atlas.
     *
     * A font atlas is a collection of pre-rendered glyphs, together with a directory
     * of the bounds for each glyph.  This directory information makes it very easy
     * to quickly construct a textured quad mesh for a series of glyphys.
     *
     * An font may have more than one atlas, particulary if the font size is large
     * and there are a large number of supported glyphs. In that case, the atlases
     * typically support a disjoint set of glyphs. However, we do not enforce this.
     */
    class Atlas {
    private:
        /** Weak reference to our parent */
        Font* _parent;
        /** This atlast size */
        Size _size;
        /** A (temporary) SDL surface for computing the atlas textures */
        SDL_Surface* _surface;
        
        /**
         * Lays out the glyphs in reasonably efficient packing.
         *
         * This method computes both the size of the atlas and the placement of the
         * individual glyphs. This method will consume glyphs from the provided
         * glyphset as it assigns them a position. So if it successfully adds all
         * glyphs, the value glyphset will be emptied.
         */
        void layout(std::deque<Uint32>& glyphset);

        /**
         * Allocates a blank surface of the given size.
         *
         * This method is necessary because SDL surface allocation is quite
         * involved when you want proper alpha support.
         *
         * @return a blank surface of the given size.
         */
        static SDL_Surface* allocSurface(int width, int height);

    public:
        /** The texture (may be null if not materialized) */
        std::shared_ptr<Texture> texture;
        /** The location of each glyph in the atlas texture. This includes padding. */
        std::unordered_map<Uint32,Rect> glyphmap;

        /**
         * Creates an uninitialized atlas with no parent font.
         *
         * NEVER USE A CONSTRUCTOR WITH NEW. If you want to allocate an object on
         * the heap, use one of the static constructors instead.
         */
        Atlas();
        
        /**
         * Deletes this atlas, diposing of all its resources.
         */
        ~Atlas() { dispose(); }
        
        /**
         * Deletes the atlas resources and resets all attributes.
         *
         * This will delete the parent font as well. You must reinitialize the
         * atlas to use it.
         */
        void dispose();
        
        /**
         * Initializes an atlas for the given font and glyphset
         *
         * This initializer will perform the layout computation, but it will
         * not create any textures or SDL surfaces. It will consume glyphs
         * from the provided glyphset as it adds them to the atlas. So if it
         * successfully adds all glyphs, the value glyphset will be emptied.
         *
         * It is possible for the atlas to reject some glyphs. This is
         * typically because the resulting texture size would exceed the
         * maximum allowable texture size. In that case, the remaining
         * elements in glyphset are glyphs that must be processed by another
         * atlas.
         *
         * If this atlas cannot process any of the elements in glyphset
         * (because they are unsupported), then this method returns false.
         *
         * @param parent    The parent font of this atlas
         * @param glyphset  The glyphs to add to this atlas
         *
         * @return true if the atlas was successfully initialized
         */
        bool init(Font* parent, std::deque<Uint32>& glyphset);
        
        /**
         * Returns a newly allocated atlas for the given font and glyphset
         *
         * This allocator will perform the layout computation, but it will
         * not create any textures or SDL surfaces. It will consume glyphs
         * from the provided glyphset as it adds them to the atlas. So if it
         * successfully adds all glyphs, the value glyphset will be emptied.
         *
         * It is possible for the atlas to reject some glyphs. This is
         * typically because the resulting texture size would exceed the
         * maximum allowable texture size. In that case, the remaining
         * elements in glyphset are glyphs that must be processed by another
         * atlas.
         *
         * If this atlas cannot process any of the elements in glyphset
         * (because they are unsupported), then this method returns nullptr.
         *
         * @param parent    The parent font of this atlas
         * @param glyphset  The glyphs to add to this atlas
         *
         * @return a newly allocated atlas for the given font and glyphset
         */
        static std::shared_ptr<Atlas> alloc(Font* parent, std::deque<Uint32>& glyphset) {
            std::shared_ptr<Atlas> result = std::make_shared<Atlas>();
            return (result->init(parent,glyphset) ? result : nullptr);
        }
        
        /**
         * Returns true if this font has a glyph for the given (UNICODE) character.
         *
         * The Unicode representation uses the endianness native to the platform.
         * Therefore, this value should not be serialized.  Use UTF8 to represent
         * unicode in a platform-independent manner.
         *
         * Note that control characters (like newline) never have glyphs. However,
         * spaces do.
         *
         * @param a     The Unicode character to check.
         *
         * @return true if this font has a glyph for the given (UNICODE) character.
         */
        bool hasGlyph(Uint32 a) const;
        
        /**
         * Returns true if this atlas has all of the given unicode glyphs
         *
         * The Unicode representation uses the endianness native to the platform.
         * Therefore, this value should not be serialized.  Use UTF8 to represent
         * unicode in a platform-independent manner.
         *
         * Note that control characters (like newline) never have glyphs. However,
         * spaces do.
         *
         * @param glyphs    The Unicode characters to check.
         *
         * @return true if this atlas has all of the given unicode glyphs
         */
        bool hasGlyphs(const std::vector<Uint32>& glyphs) const;
        
        /**
         * Returns true if this atlas has all of the given glyphs
         *
         * We assume that the string represents the glyphs in a UTF8 encoding.
         *
         * Note that control characters (like newline) never have glyphs. However,
         * spaces do.
         *
         * @param glyphs    The UTF8 glyphs to check.
         *
         * @return true if this atlas has all of the given glyphs
         */
        bool hasGlyphs(const std::string& glyphs) const;

        /**
         * Creates a single quad to render this character and stores it in mesh
         *
         * This method will append the vertices to the provided mesh and update
         * the indices to include these new vertices. Once the quad is generated,
         * the offset will be adjusted to contain the next place to render a
         * character. This method will not generate anything if the character is
         * not supported by this atlas.
         *
         * The quad is adjusted so that all of the vertices fit in the provided
         * rectangle.  This may mean that no quad is generated at all.
         *
         * @param thechar   The character to convert to render data
         * @param offset    The (unkerned) starting position of the quad
         * @param rect      The bounding box for the quad
         * @param mesh      The mesh to store the vertices
         */
        bool getQuad(Uint32 thechar, Vec2& offset, Mesh<SpriteVertex2>& mesh, const Rect rect) const;

        /**
         * Creates a single quad to render this character and stores it in mesh
         *
         * This method will append the vertices to the provided mesh and update
         * the indices to include these new vertices. Once the quad is generated,
         * the offset will be adjusted to contain the next place to render a
         * character. This method will not generate anything if the character is
         * not supported by this atlas.
         *
         * @param thechar   The character to convert to render data
         * @param offset    The (unkerned) starting position of the quad
         * @param mesh      The mesh to store the vertices
         */
        void getQuad(Uint32 thechar, Vec2& offset, Mesh<SpriteVertex2>& mesh) const;

        /**
         * Builds the texture data for this given atlas.
         *
         * This method does not generate the OpenGL texture, but does all other
         * work in creates the atlas. In particular it creates the image buffer
         * so that texture creation is just one OpenGL call. This creation will
         * happen once {@link materialize()} is called. As a result, it is safe
         * to call this method outside of the main thread.
         *
         * @return true if atlas creation was successful
         */
        bool build();
        
        /**
         * Creates the OpenGL texture for this atlas.
         *
         * This method must be called on the main thread. It is only safe to call
         * this method after a succesful call to {@link #build()}.
         *
         * @return true if texture creation was successful.
         */
        bool materialize();
    };

#pragma mark -
#pragma mark Font Class
    /** The name of this font (typically the family name if known) */
    std::string _name;
    /** The name of this font style */
    std::string _stylename;
    
    /** The underlying SDL data */
    TTF_Font* _data;

    // Cached settings
    /** The point size of this font. */
    int _fontSize;
    /** The (maximum) height of this font. It is the sum of ascent and descent. */
    int _fontHeight;
    /** The maximum distance from the baseline to the glyph bottom (always negative) */
    int _fontDescent;
    /** The maximum distance from the baseline to the glyph top (always positive) */
    int _fontAscent;
    /** The recommended line skip for this font */
    int _fontLineSkip;
    /** Whether this is a fixed width font */
    bool _fixedWidth;
    /** Whether to use kerning when rendering */
    bool _useKerning;
    
    // Render settings
    /** The font face style */
    Style _style;
    /** The rasterization hints */
    Hinting _hints;
    
    // Altas support
    /** The cached metrics for each font glyph. This does not include padding. */
    std::unordered_map<Uint32, Metrics> _glyphsize;
    /** The kerning for each pair of characters */
    std::unordered_map<Uint32, std::unordered_map<Uint32, Uint32> > _kernmap;
    /** The individual atlases for this font */
    std::vector<std::shared_ptr<Atlas>> _atlases;
    /** The number of pixels to pad around each edge of a glyph.  Necessary to support font blurs. */
    Uint32 _atlasPadding;
    /** The atlas storing any particular character */
    std::unordered_map<Uint32, size_t> _atlasmap;

    // GlyphRun generation
    /** Whether to generate an impromptu atlas for missing glyphs */
    bool _fallback;
    /** The maximum number of pixels to reduce the advance when shrinking a line */
    int _shrinkLimit;
    /** The maximum number of pixels to grow the advance when stretching a line */
    int _stretchLimit;

    
public:
#pragma mark Constructors
    /**
     * Creates a degenerate font with no data.
     *
     * NEVER USE A CONSTRUCTOR WITH NEW. If you want to allocate an object on
     * the heap, use one of the static constructors instead.
     */
    Font();
    
    /**
     * Deletes this font, disposing of all resources.
     */
    ~Font() { dispose(); }
    
    /**
     * Deletes the font resources and resets all attributes.
     *
     * This will delete the original font information in addition to any
     * generating atlases. You must reinitialize the font to use it.
     */
    virtual void dispose();
    
    /**
     * Initializes a font of the given size from the file.
     *
     * The font size is fixed on initialization.  It cannot be changed without
     * disposing of the entire font.  However, all other attributes may be
     * changed.
     *
     * @param file  The file with the font asset
     * @param size  The font size in points
     *
     * @return true if initialization is successful.
     */
    bool init(const std::string file, Uint32 size);

    /**
     * Returns a newly allocated font of the given size from the file.
     *
     * The font size is fixed on creation.  It cannot be changed without
     * creating a new font asset.  However, all other attributes may be
     * changed.
     *
     * @param file  The file with the font asset
     * @param size  The font size in points
     *
     * @return a newly allocated font of the given size from the file.
     */
    static std::shared_ptr<Font> alloc(const std::string file, Uint32 size) {
        std::shared_ptr<Font> result = std::make_shared<Font>();
        return (result->init(file,size) ? result : nullptr);
    }

#pragma mark -
#pragma mark Attributes
    /**
     * Returns the family name of this font.
     *
     * This method may return an empty string, meaning the information is not 
     * available.
     *
     * @return the family name of this font.
     */
    const std::string getName() const { return _name; }

    /**
     * Returns the point size of this font.
     *
     * The point size does not convey any metric information about this font.
     * However, it is important for scaling the font to other sizes.
     *
     * @return the point size of this font.
     */
    int getPointSize() const { return _fontSize; }
    
    /**
     * Returns the style name of this font.
     *
     * This method may return an empty string, meaning the information is not
     * available.
     *
     * @return the style name of this font.
     */
    const std::string getStyleName() const { return _stylename; }
    
    /**
     * Returns the maximum height of this font.
     *
     * This is the sum of the ascent and the negative descent.  Any box that
     * is this many characters high can sucessfully hold a glyph from this 
     * font.
     *
     * @return the maximum height of this font.
     */
    int getHeight() const { return _fontHeight; }
    
    /**
     * Returns the maximum distance from the baseline to the bottom of a glyph.
     *
     * This value will always be negative.  You should add this value to the 
     * y position to shift the baseline down to the rendering origin.
     *
     * @return the maximum distance from the baseline to the bottom of a glyph.
     */
    int getDescent() const { return _fontDescent; }
    
    /**
     * Returns the maximum distance from the baseline to the top of a glyph.
     *
     * This value will always be positive.
     *
     * @return the maximum distance from the baseline to the top of a glyph.
     */
    int getAscent() const { return _fontAscent; }
    
    /**
     * Returns the recommended lineskip of this font.
     *
     * The line skip is the recommended height of a line of text. It is often
     * larger than the font height.
     *
     * @return the recommended lineskip of this font.
     */
    int getLineSkip() const { return _fontLineSkip; }
    
    /**
     * Returns true if the font is a fixed width font.
     *
     * Fixed width fonts are monospace, meaning every character that exists in
     * the font is the same width. In this case you can assume that a rendered
     * string's width is going to be the result of a simple calculation:
     *
     *      glyph_width * string_length
     *
     * @return true if the font is a fixed width font.
     */
    bool isFixedWidth() const { return _fixedWidth; }

    /**
     * Returns true if this font has a glyph for the given (UNICODE) character.
     *
     * The Unicode representation uses the endianness native to the platform.
     * Therefore, this value should not be serialized. Use UTF8 to represent
     * unicode in a platform-independent manner.
     *
     * This method is not an indication of whether or not there is a font
     * atlas for this font. It is simply an indication whether or not this
     * glyphs are present in the font source. Note that control characters
     * (like newline) never have glyphs. However, spaces do.
     *
     * @param a     The Unicode character to check.
     *
     * @return true if this font has a glyph for the given (UNICODE) character.
     */
    bool hasGlyph(Uint32 a) const;
    
    /**
     * Returns true if this font can successfuly render the given glyphs.
     *
     * This method is not an indication of whether or not there is a font
     * atlas for this font. It is simply an indication whether or not these
     * glyphs are present in the font source. Note that control characters
     * (like newline) never have glyphs. However, spaces do.
     *
     * This method will return false if just one glyph is missing. The glyph
     * identifiers may either be in UTF8 or ASCII; the method will handle
     * conversion automatically.
     *
     * @param glyphs  The glyph identifiers to check.
     *
     * @return true if this font can successfuly render the given glyphs.
     */
    bool hasGlyphs(const std::string glyphs) const;
    
    /**
     * Returns true if this font can successfuly render the given glyphs.
     *
     * This method is not an indication of whether or not there is a font
     * atlas for this font. It is simply an indication whether or not these
     * glyphs are present in the font source. Note that control characters
     * (like newline) never have glyphs. However, spaces do.
     *
     * This method will return false if just one glyph is missing. The glyph
     * identifiers should be the UNICODE values. The Unicode representation
     * uses the endianness native to the platform. Therefore, these values
     * should not be serialized. You should use UTF8 to represent unicode
     * in a platform-independent manner.
     *
     * @param glyphs  The glyph identifiers to check.
     *
     * @return true if this font can successfuly render the given glyphs.
     */
    bool hasGlyphs(const std::vector<Uint32>& glyphs) const {
        for(auto it = glyphs.begin(); it != glyphs.end(); ++it) {
            if (!hasGlyph(*it)) { return false; }
        }
        return true;
    }

#pragma mark -
#pragma mark Settings
    /**
     * Returns true if this font atlas uses kerning when rendering.
     *
     * Without kerning, each character is guaranteed to take up its entire
     * advance when rendered. This may make spacing look awkard. This
     * value is true by default.
     *
     * Note that kerning is different from tracking (see {@link getGlyphs}).
     * Tracking is used to dynamically shrink or stretch the text to fit a
     * given region, while kerning is used at all times.
     *
     * @return true if this font atlas uses kerning when rendering.
     */
    bool usesKerning() const { return _useKerning; }
    
    /**
     * Sets whether this font atlas uses kerning when rendering.
     *
     * Without kerning, each character is guaranteed to take up its entire
     * advance when rendered. This may make spacing look awkard. This
     * value is true by default.
     *
     * Note that kerning is different from tracking (see {@link getGlyphs}).
     * Tracking is used to dynamically shrink or stretch the text to fit a
     * given region, while kerning is used at all times.
     *
     * Reseting this value will clear any existing atlas collection.
     *
     * @param kerning   Whether this font atlas uses kerning when rendering.
     */
    void setKerning(bool kerning);

    /**
     * Returns the style for this font.
     *
     * With the exception of normal style (which is an absent of any style), all
     * of the styles may be combined.  So it is possible to have a bold, italic,
     * underline font with strikethrough. To combine styles, simply treat the
     * Style value as a bitmask, and combine them with bitwise operations.
     *
     * @return the style for this font.
     */
    Style getStyle() const { return _style; }
    
    /**
     * Sets the style for this font.
     *
     * Changing this value will delete any atlas that is present.  The atlas
     * must be regenerated.
     *
     * With the exception of normal style (which is an absent of any style), all
     * of the styles may be combined.  So it is possible to have a bold, italic,
     * underline font with strikethrough. To combine styles, simply treat the
     * Style value as a bitmask, and combine them with bitwise operations.
     *
     * Reseting this value will clear any existing atlas collection.
     *
     * @param style The style for this font.
     */
    void setStyle(Style style);
    
    /** 
     * Returns the rasterization hints
     *
     * Hinting is used to align the font to a rasterized grid. At low screen
     * resolutions, hinting is critical for producing clear, legible text
     * (particularly if you are not supporting antialiasing).
     *
     * @return the rasterization hints
     */
    Hinting getHinting() const { return _hints; }
    
    /**
     * Sets the rasterization hints
     *
     * Changing this value will delete any atlas that is present.  The atlas
     * must be regenerated.
     *
     * Hinting is used to align the font to a rasterized grid. At low screen
     * resolutions, hinting is critical for producing clear, legible text
     * (particularly if you are not supporting antialiasing).
     *
     * Reseting this value will clear any existing atlas collection.
     *
     * @param hinting   The rasterization hints
     */
    void setHinting(Hinting hinting);

    /**
     * Returns the atlas padding
     *
     * The atlas padding the guaranteed padding between glyphs in the textures
     * for the atlas collection. By default, glyphs are no more than a pixel
     * apart from each other, to minimize texture size. This value represents
     * the individual pixels to add along all four borders of the glyph.
     *
     * However, this prevents font blur effects, as the individual glyphs will
     * blur into each other. If you plan to render a font with a non-zero value
     * to {@link SpriteBatch#setBlur}, then you must add padding equal to
     * or exceeding the radius.
     *
     * @return the atlas padding
     */
    Uint32 getPadding() const { return _atlasPadding; }

    /**
     * Sets the atlas padding
     *
     * The atlas padding the guaranteed padding between glyphs in the textures
     * for the atlas collection. By default, glyphs are no more than a pixel
     * apart from each other, to minimize texture size. This value represents
     * the individual pixels to add along all four borders of the glyph.
     *
     * However, this prevents font blur effects, as the individual glyphs will
     * blur into each other. If you plan to render a font with a non-zero value
     * to {@link SpriteBatch#setBlur}, then you must add padding equal to
     * or exceeding the radius.
     *
     * Reseting this value will clear any existing atlas collection.
     *
     * @param padding   The additional atlas padding
     */
    void setPadding(Uint32 padding);
    
    /**
     * Sets whether to generate a fallback atlas for glyph runs.
     *
     * When creating a set of glyphs run it is possible for some of the
     * glyphs to be supported by the font, but missing from the all of
     * the atlases. This is particularly true for unicode characters
     * beyond the ascii range. By default, the glyph run set will simply
     * omit this glyphs.
     *
     * However, if this value is set to true, the glyph run methods like
     * {@link #getGlyphs} will generate a one-time atlas for the missing
     * characters. This atlas will **not** be stored for future use. In
     * addition, forcing this creation means that the glyph generation
     * methods are no longer safe to be used outside of the main thread
     * (this is not an issue if this attribute is false).
     *
     * @param fallback  Whether to generate a fallback atlas for glyph runs.
     */
    void setAtlasFallback(bool fallback) { _fallback = fallback; }
    
    /**
     * Returns true if this font generates a fallback atlas for glyph runs.
     *
     * When creating a set of glyphs run it is possible for some of the
     * glyphs to be supported by the font, but missing from the all of
     * the atlases. This is particularly true for unicode characters
     * beyond the ascii range. By default, the glyph run set will simply
     * omit this glyphs.
     *
     * However, if this value is set to true, the glyph run methods like
     * {@link #getGlyphs} will generate a one-time atlas for the missing
     * characters. This atlas will **not** be stored for future use. In
     * addition, forcing this creation means that the glyph generation
     * methods are no longer safe to be used outside of the main thread
     * (this is not an issue if this attribute is false).
     *
     * @return true if this font generates a fallback atlas for glyph runs.
     */
    bool hasAtlasFallback() const { return _fallback; }
    
    /**
     * Sets the limit for shrinking the advance during tracking
     *
     * A font can provided limited tracking support to shrink or grow the
     * space between characters (in order to fit a glyph run to a given
     * width). This value is the maximum number of units that tracking
     * will ever reduce the advance between two characters. This limit
     * is applied uniformly to all characters, including spaces.
     *
     * By default this value is 0, disabling all (negative) tracking.
     *
     * @param limit     The limit for shrinking the advance during tracking
     */
    void setShrinkLimit(Uint32 limit) { _shrinkLimit = limit; }
    
    /**
     * Returns the limit for shrinking the advance during tracking
     *
     * A font can provided limited tracking support to shrink or grow the
     * space between characters (in order to fit a glyph run to a given
     * width). This value is the maximum number of units that tracking
     * will ever reduce the advance between two characters. This limit
     * is applied uniformly to all characters, including spaces.
     *
     * By default this value is 0, disabling all (negative) tracking.
     *
     * @return the limit for shrinking the advance during tracking
     */
    Uint32 getShrinkLimit() const { return _shrinkLimit; }

    /**
     * Sets the limit for stretching the advance during tracking
     *
     * A font can provided limited tracking support to shrink or grow the
     * space between characters (in order to fit a glyph run to a given
     * width). This value is the maximum number of units that tracking
     * will ever grow the advance between two (non-space) characters.
     *
     * By default this value is 0. That means that any positive tracking
     * will be applied to spaces only. In that case, the result would be
     * equivalent to old-school justification, which stretches a line
     * by only resizing whitespace.
     *
     * @param limit     The limit for stretching the advance during tracking
     */
    void setStretchLimit(Uint32 limit) { _stretchLimit = limit; }
    
    /**
     * Returns the limit for stretching the advance during tracking
     *
     * A font can provided limited tracking support to shrink or grow the
     * space between characters (in order to fit a glyph run to a given
     * width). This value is the maximum number of units that tracking
     * will ever grow the advance between two (non-space) characters.
     *
     * By default this value is 0. That means that any positive tracking
     * will be applied to spaces only. In that case, the result would be
     * equivalent to old-school justification, which stretches a line
     * by only resizing whitespace.
     *
     * @return the limit for stretching the advance during tracking
     */
    Uint32 getStretchLimit() const { return _stretchLimit; }
    
#pragma mark -
#pragma mark Measurements
    /**
     * Returns the glyph metrics for the given (Unicode) character.
     *
     * See {@link Metrics} for an explanation of the data provided by this
     * method. This method will fail if the glyph is not in this font. In
     * particular, control characters (e.g. newlines) will fail while spaces
     * will not.
     *
     * The Unicode representation uses the endianness native to the platform.
     * Therefore, this value should not be serialized.  Use UTF8 to represent
     * unicode in a platform-independent manner. See the function
     * {@link strtool::getCodePoints} for how to get a unicode codepoint from
     * a UTF8 string.
     *
     * @param thechar   The Unicode character to measure.
     *
     * @return the glyph metrics for the given (Unicode) character.
     */
    const Metrics getMetrics(Uint32 thechar) const;
    
    /**
     * Returns the kerning adjustment between the two (Unicode) characters.
     *
     * This value is the amount of overlap (in pixels) between any two adjacent
     * character glyphs rendered by this font.  If the value is 0, there is no
     * kerning for this pair.
     *
     * The Unicode representation uses the endianness native to the platform.
     * Therefore, this value should not be serialized.  Use UTF8 to represent
     * unicode in a platform-independent manner. See the function
     * {@link strtool::getCodePoints} for how to get a unicode codepoint from
     * a UTF8 string.
     *
     * @param a     The first Unicode character in the pair
     * @param b     The second Unicode character in the pair
     *
     * @return the kerning adjustment between the two (Unicode) characters.
     */
    unsigned int getKerning(Uint32 a, Uint32 b) const;
    
    /**
     * Returns the size (in pixels) necessary to render this string.
     *
     * This size is a conservative estimate to render the string. The height
     * is guaranteed to be the maximum height of the font, regardless of the
     * text measured.  In addition, the measurement will include the full
     * advance of the both the first and last characters.  This means that
     * there may be some font-specific padding around these characters.
     *
     * The y-origin of this rectangle is guaranteed to be {@link #getDescent}.
     * That is because glyphs will use the baseline as the origin when
     * rendering the text.
     *
     * This measurement does not actually render the string. This method will
     * not fail if it includes glyphs that are not present in the font, but it
     * will drop them when measuring the size.
     *
     * The string may either be in UTF8 or ASCII; the method will handle
     * conversion automatically.
     *
     * @param text  The string to measure
     *
     * @return the size (in pixels) necessary to render this string.
     */
    Size getSize(const std::string text) const;
    
    /**
     * Returns the size (in pixels) necessary to render this string.
     *
     * This size is a conservative estimate to render the string.  The height
     * is guaranteed to be the maximum height of the font, regardless of the
     * text measured.  In addition, the measurement will include the full
     * advance of the both the first and last characters.  This means that
     * there may be some font-specific padding around these characters.
     *
     * The y-origin of this rectangle is guaranteed to be {@link #getDescent}.
     * That is because glyphs will use the baseline as the origin when
     * rendering the text.
     *
     * This measurement does not actually render the string. This method will
     * not fail if it includes glyphs that are not present in the font, but it
     * will drop them when measuring the size.
     *
     * The C-style string substr need not be null-terminated. Instead,
     * the termination is indicated by the parameter end. This provides
     * efficient substring processing. The string may either be in UTF8
     * or ASCII; the method will handle conversion automatically.
     *
     * @param substr    The start of the string to measure
     * @param end       The end of the string to measure
     *
     * @return the size (in pixels) necessary to render this string.
     */
    Size getSize(const char* substr, const char* end) const;
    
    /**
     * Returns the pixel offset of the glyphs inside a rendered string.
     *
     * The result of {@link getSize()} is very conservative.  Even if no 
     * character uses the maximum height, it provides the full height of 
     * the font.  Furthermore, if the last character does not use the full 
     * advance, there will be padding after that character.
     *
     * The rectangle returned by this method provide the internal bounds of
     * the rendered text.  The value is in "text space".  If a string is
     * rendered at position (0,0), this is the bounding box for all of the
     * glyphs that are actually rendered.  It is the tightest bounding box
     * that can fit all of the generated glyph.  You can use this rectangle
     * to eliminate any font-specific spacing that may have been placed
     * around the glyphs.
     *
     * For example, suppose the string is "ah".  In many fonts, these two
     * glyphs would not dip below the baseline. Therefore, the y value of
     * the returned rectangle would be at the font baseline (which is always
     * 0), indicating that it is safe to start rendering there.
     *
     * This measurement does not actually render the string. This method will
     * not fail if it includes glyphs that are not present in the font, but it
     * will drop them when measuring the size.
     *
     * The string may either be in UTF8 or ASCII; the method will handle
     * conversion automatically.
     *
     * @param text  The string to measure
     *
     * @return the size of the quad sequence generated for this string.
     */
    Rect getInternalBounds(const std::string text) const;
    
    /**
     * Returns the pixel offset of the glyphs inside a rendered string.
     *
     * The result of {@link getSize()} is very conservative.  Even if no
     * character uses the maximum height, it provides the full height of
     * the font.  Furthermore, if the last character does not use the full
     * advance, there will be padding after that character.
     *
     * The rectangle returned by this method provide the internal bounds of
     * the rendered text.  The value is in "text space".  If a string is
     * rendered at position (0,0), this is the bounding box for all of the
     * glyphs that are actually rendered.  It is the tightest bounding box
     * that can fit all of the generated glyph.  You can use this rectangle
     * to eliminate any font-specific spacing that may have been placed
     * around the glyphs.
     *
     * For example, suppose the string is "ah".  In many fonts, these two
     * glyphs would not dip below the baseline.  Therefore, the y value of
     * the returned rectangle would be at the font baseline (which is always
     * 0), indicating that it is safe to start rendering there.
     *
     * This measurement does not actually render the string. This method will
     * not fail if it includes glyphs that are not present in the font, but it
     * will drop them when measuring the size.
     *
     * The C-style string substr need not be null-terminated. Instead,
     * the termination is indicated by the parameter end. This provides
     * efficient substring processing. The string may either be in UTF8
     * or ASCII; the method will handle conversion automatically.
     *
     * @param substr    The start of the string to measure
     * @param end       The end of the string to measure
     *
     * @return the size of the quad sequence generated for this string.
     */
    Rect getInternalBounds(const char* substr, const char* end) const;

    /**
     * Returns the tracking adjustments to fit the text in the given width
     *
     * Unlike kerning, tracking is used to dynamically adjust the spaces between
     * characters. The purpose is to fix the text to the given width exactly (or
     * as close as possible).  Usually this means shrinking the space when the
     * text is larger than the width.  But in the case of justification, it may
     * also be used to increase the space. The number of tracking measurements
     * is one less than the number of characters.
     *
     * All tracking is measured in integer offsets. That is because text looks
     * more uniform when glyph positions are at integral values (otherwise the
     * texture may shimmer on movement). Whenever possible, the algorithm will
     * try to track the text to within 1 unit of the width (under, not over).
     * In the case of shrinking, this may not be possible if the shrink limit
     * is too low.
     *
     * Tracking adjustments will be uniform between non-space characters. If
     * any non-uniform adjustments need to be made, they will be made around
     * white-space.
     *
     * The string may either be in UTF8 or ASCII; the method will handle
     * conversion automatically.
     *
     * @param text      The text to measure
     * @param width     The line width
     *
     * @return the tracking adjustments to fit the text in the given width
     */
    std::vector<int> getTracking(const std::string text, float width);

    /**
     * Returns the tracking adjustments to fit the text in the given width
     *
     * Unlike kerning, tracking is used to dynamically adjust the spaces between
     * characters. The purpose is to fix the text to the given width exactly (or
     * as close as possible).  Usually this means shrinking the space when the
     * text is larger than the width.  But in the case of justification, it may
     * also be used to increase the space. The number of tracking measurements
     * is one less than the number of characters.
     *
     * All tracking is measured in integer offsets. That is because text looks
     * more uniform when glyph positions are at integral values (otherwise the
     * texture may shimmer on movement). Whenever possible, the algorithm will
     * try to track the text to within 1 unit of the width (under, not over).
     * In the case of shrinking, this may not be possible if the shrink limit
     * is too low.
     *
     * Tracking adjustments will be uniform between non-space characters. If
     * any non-uniform adjustments need to be made, they will be made around
     * white-space.
     *
     * The C-style string substr need not be null-terminated. Instead, the
     * termination is indicated by the parameter end. This provides efficient
     * substring processing. The string may either be in UTF8 or ASCII; the
     * method will handle conversion automatically.
     *
     * @param substr    The start of the string to measure
     * @param end       The end of the string to measure
     * @param width     The line width
     *
     * @return the tracking adjustments to fit the text in the given width
     */
    std::vector<int> getTracking(const char* substr, const char* end, float width);
    
#pragma mark -
#pragma mark Atlas Support
    /**
     * Deletes the current collections of atlases
     *
     * Until a new font atlas is created, any attempt to use this font
     * will result in adhoc atlases (e.g. one-off atlases associated
     * with a single glyph run).
     */
    void clearAtlases();

    /**
     * Creates an atlas collection for the ASCII characters in this font.
     *
     * Only the ASCII characters are added to the atlases, even if the font has
     * support for more characters. You should use a character set method
     * if you want Unicode characters supported.
     *
     * If there is already an active atlas collection, this method will preserve
     * those atlas textures. Instead, it will only generate atlases for the
     * characters that are not currently supported.
     *
     * The character atlas textures are generated immediately, so the method
     * {@link getAtlases()} may be called with no delay.
     *
     * WARNING: This method is not thread safe.  It generates OpenGL textures, 
     * which means that it may only be called in the main thread.
     *
     * @return true if the atlases were successfully created.
     */
    bool buildAtlases();

    /**
     * Creates an atlas collection for the given character set.
     *
     * The atlases generated contains characters in the provided character set,
     * and will omit all other chacters. This includes ASCII characters that may
     * be missing from the character set. The character set string must either
     * be in ASCII or UTF8 encoding. It will handle both automatically, but no
     * other encoding (e.g. Latin1) is accepted.
     *
     * If there is already an active atlas collection, this method will preserve
     * those atlas textures. Instead, it will only generate atlases for the
     * characters that are not currently supported.
     *
     * The character atlas textures are generated immediately, so the method
     * {@link getAtlases()} may be called with no delay.
     *
     * WARNING: This method is not thread safe.  It generates OpenGL textures, 
     * which means that it may only be called in the main thread.
     *
     * @param charset   The set of characters in the atlas
     *
     * @return true if the atlases were successfully created.
     */
    bool buildAtlases(const std::string charset);
    
    /**
     * Creates an atlas collection for the given character set.
     *
     * The atlases generated contains characters in the provided character set,
     * and will omit all other chacters. This includes ASCII characters that may
     * be missing from the character set. The character set provided must be
     * a collection of UNICODE encodings. The Unicode representation uses the 
     * endianness native to the platform. Therefore, this value should not be 
     * serialized. Use UTF8 to represent unicode in a platform-independent manner.
     *
     * If there is already an active atlas collection, this method will preserve
     * those atlas textures. Instead, it will only generate atlases for the
     * characters that are not currently supported.
     *
     * The character atlas textures are generated immediately, so the method
     * {@link getAtlases()} may be called with no delay.
     *
     * WARNING: This method is not thread safe.  It generates OpenGL textures, 
     * which means that it may only be called in the main thread.
     *
     * @param charset   The set of characters in the atlas
     *
     * @return true if the atlases were successfully created.
     */
    bool buildAtlases(const std::vector<Uint32>& charset);

    /**
     * Creates an atlas collection for the ASCII characters in this font.
     *
     * Only the ASCII characters are added to the atlases, even if the font has
     * support for more characters. You should use a character set method
     * if you want Unicode characters supported.
     *
     * If there is already an active atlas collection, this method will preserve
     * those atlas textures. Instead, it will only generate atlases for the
     * characters that are not currently supported.
     *
     * This method does not generate any OpenGL textures, but does all other
     * work in creates the atlases. In particular it creates the image buffers
     * so that texture creation is just one OpenGL call. This creation will
     * happen the first time that {@link #storeAtlases()} is called.
     *
     * As a result, this method is thread safe. It may be called in any
     * thread, including threads other than the main one.
     *
     * @return true if the atlases were successfully created.
     */
    bool buildAtlasesAsync();

    /**
     * Creates an atlas collection for the given character set.
     *
     * The atlas only contains characters in the provided character set, and
     * will omit all other chacters.  This includes ASCII characters that may
     * be missing from the character set. The character set string must either
     * be in ASCII or UTF8 encoding. It will handle both automatically, but no
     * other encoding (e.g. Latin1) is accepted.
     *
     * If there is already an active atlas collection, this method will preserve
     * those atlas textures. Instead, it will only generate atlases for the
     * characters that are not currently supported.
     *
     * This method does not generate any OpenGL textures, but does all other
     * work in creates the atlases. In particular it creates the image buffers
     * so that texture creation is just one OpenGL call. This creation will
     * happen the first time that {@link #storeAtlases()} is called.
     *
     * As a result, this method is thread safe. It may be called in any
     * thread, including threads other than the main one.
     *
     * @param charset   The set of characters in the atlas
     *
     * @return true if the atlases were successfully created.
     */
    bool buildAtlasesAsync(const std::string charset);

    /**
     * Creates an atlas collection for the given character set.
     *
     * The atlases generated contains characters in the provided character set,
     * and will omit all other chacters. This includes ASCII characters that may
     * be missing from the character set. The character set provided must be
     * a collection of UNICODE encodings. The Unicode representation uses the 
     * endianness native to the platform. Therefore, this value should not be 
     * serialized. Use UTF8 to represent unicode in a platform-independent manner.
     *
     * If there is already an active atlas collection, this method will preserve
     * those atlas textures. Instead, it will only generate atlases for the
     * characters that are not currently supported.
     *
     * This method does not generate any OpenGL textures, but does all other
     * work in creates the atlases. In particular it creates the image buffers
     * so that texture creation is just one OpenGL call. This creation will
     * happen the first time that {@link #storeAtlases()} is called.
     *
     * As a result, this method is thread safe. It may be called in any
     * thread, including threads other than the main one.
     *
     * @param charset   The set of characters in the atlas
     *
     * @return true if the atlases were successfully created.
     */
    bool buildAtlasesAsync(const std::vector<Uint32>& charset);

    /**
     * Creates an OpenGL texture for each atlas in the collection.
     *
     * This method should be called to finalize the work of {@link #buildAtlasesAsync}.
     * This method must be called on the main thread.
     */
    bool storeAtlases();

    /**
     * Returns the OpenGL textures for the associated atlas collection.
     *
     * When combined with a quad sequence generated by the associate atlas,
     * each texture can be used to draw a font in a {@link SpriteBatch}. If
     * there is no atlas collection, this method returns an empty vector.
     *
     * @return the OpenGL textures for the associated atlas collection.
     */
    const std::vector<std::shared_ptr<Texture>> getAtlases();

    /**
     * Returns true if the given unicode character has atlas support.
     *
     * If this method is true, then {@link #getGlyphs()} is guaranteed
     * to succeed and be thread safe, whenever the text is consists of
     * the provide character.
     *
     * The characters provided should be represented by a UNICODE value.
     * The Unicode representation uses the endianness native to the platform. 
     * Therefore, this value should not be serialized. Use UTF8 to represent 
     * unicode in a platform-independent manner.     
     *
     * @param thechar   The character to test (as UNICODE)
     *
     * @return true if the given characters have atlas support.
     */
    bool hasAtlas(Uint32 thechar) const {
        return _atlasmap.find(thechar) != _atlasmap.end();
    }


    /**
     * Returns true if the given characters have atlas support.
     *
     * If this method is true, then {@link #getGlyphs()} is guaranteed
     * to succeed and be thread safe, whenever the text is consists of
     * provided characters.
     *
     * The characters provided should be in UTF8 or ASCII format.
     *
     * @param charset   The characters to test (as UTF8 or ASCII)
     *
     * @return true if the given characters have atlas support.
     */
    bool hasAtlases(const std::string charset) const;
    
    /**
     * Returns true if the given characters have atlas support.
     *
     * If this method is true, then {@link #getGlyphs()} is guaranteed
     * to succeed and be thread safe, whenever the text is consists of
     * provided characters.
     *
     * The characters provided should be represented by UNICODE values.
     * The Unicode representation uses the endianness native to the platform. 
     * Therefore, this value should not be serialized. Use UTF8 to represent 
     * unicode in a platform-independent manner.     
     *
     * @param charset   The characters to test (as UNICODE)
     *
     * @return true if the given characters have atlas support.
     */
    bool hasAtlases(const std::vector<Uint32>& charset) const;

    
#pragma mark -
#pragma mark Glyph Generation
    /**
     * Returns a set of glyph runs to render the given string
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
     * {@link #getAtlases} allows you to identify the atlas for each run.
     *
     * If a character in the string is not represented by a glyph in
     * the atlas collection, then it will be skipped unless the value
     * {@link #setAtlasFallback} is set to true. In that case, this method
     * will generate a one-time atlas for these characters and return its
     * texture as part of the set. However, this atlas will not be stored
     * for future use. In addition, forcing this creation of a fallback
     * atlas makes this method no longer safe to be used outside of the
     * main thread (this is not an issue if {@link #hasAtlasFallback} is
     * false). Note that control characters (e.g. newlines) have no glyphs.
     *
     * The origin value determines the position of the bottom of the text
     * baseline. You should use the methods {@link #getDescent} and
     * {@link #getAscent} to place either the bottom or top of the text,
     * respectively.
     *
     * The string may either be in UTF8 or ASCII; the method will handle
     * conversion automatically.
     *
     * @param text      The string for glyph generation
     * @param origin    The position of the first character
     *
     * @return a set of glyph runs to render the given string
     */
    std::unordered_map<GLuint, std::shared_ptr<GlyphRun>> getGlyphs(const std::string text, const Vec2 origin);

    /**
     * Returns a set of glyph runs to render the given (sub)string
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
     * {@link #getAtlases} allows you to identify the atlas for each run.
     *
     * If a character in the string is not represented by a glyph in
     * the atlas collection, then it will be skipped unless the value
     * {@link #setAtlasFallback} is set to true. In that case, this method
     * will generate a one-time atlas for these characters and return its
     * texture as part of the set. However, this atlas will not be stored
     * for future use. In addition, forcing this creation of a fallback
     * atlas makes this method no longer safe to be used outside of the
     * main thread (this is not an issue if {@link #hasAtlasFallback} is
     * false). Note that control characters (e.g. newlines) have no glyphs.
     *
     * The origin value determines the position of the bottom of the text
     * baseline. You should use the methods {@link #getDescent} and
     * {@link #getAscent} to place either the bottom or top of the text,
     * respectively.
     *
     * The C-style string substr need not be null-terminated. Instead,
     * the termination is indicated by the parameter end. This provides
     * efficient substring processing. The string may either be in UTF8
     * or ASCII; the method will handle conversion automatically.
     *
     * @param substr    The start of the string for glyph generation
     * @param end       The end of the string for glyph generation
     * @param origin    The position of the first character
     *
     * @return a set of glyph runs to render the given string
     */
    std::unordered_map<GLuint, std::shared_ptr<GlyphRun>> getGlyphs(const char* substr, const char* end, const Vec2 origin);
    
    /**
     * Returns a set of glyph runs to render the given string
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
     * {@link #getAtlases} allows you to identify the atlas for each run.
     *
     * If a character in the string is not represented by a glyph in
     * the atlas collection, then it will be skipped unless the value
     * {@link #setAtlasFallback} is set to true. In that case, this method
     * will generate a one-time atlas for these characters and return its
     * texture as part of the set. However, this atlas will not be stored
     * for future use. In addition, forcing this creation of a fallback
     * atlas makes this method no longer safe to be used outside of the
     * main thread (this is not an issue if {@link #hasAtlasFallback} is
     * false). Note that control characters (e.g. newlines) have no glyphs.
     *
     * The origin value determines the position of the bottom of the text
     * baseline. You should use the methods {@link #getDescent} and
     * {@link #getAscent} to place either the bottom or top of the text,
     * respectively.
     *
     * The quad sequence is adjusted so that all of the vertices fit in the
     * provided rectangle. The primary use-case for this is to guarantee
     * that glyphs do no spill outside of a window. This may mean that some
     * of the glyphs will be truncrated or even omitted.
     *
     * The tracking width is used to justify text in multi-line formats. If
     * track is positive, the spacing between the glyphs will be adjusted to
     * fit that width exactly. Once again, this may result in glyphs being
     * truncated if either the track width is greater then the rectangle width,
     * or if the font shrink limit is insufficent.
     *
     * The string may either be in UTF8 or ASCII; the method will handle
     * conversion automatically.
     *
     * @param text      The string for glyph generation
     * @param origin    The position of the first character
     * @param rect      The bounding box for the quads
     * @param track     The tracking width (if positive)
     *
     * @return a set of glyph runs to render the given string
     */
    std::unordered_map<GLuint, std::shared_ptr<GlyphRun>> getGlyphs(const std::string text, const Vec2 origin,
                                                                    const Rect rect, float track=0);

    /**
     * Returns a set of glyph runs to render the given string
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
     * {@link #getAtlases} allows you to identify the atlas for each run.
     *
     * If a character in the string is not represented by a glyph in
     * the atlas collection, then it will be skipped unless the value
     * {@link #setAtlasFallback} is set to true. In that case, this method
     * will generate a one-time atlas for these characters and return its
     * texture as part of the set. However, this atlas will not be stored
     * for future use. In addition, forcing this creation of a fallback
     * atlas makes this method no longer safe to be used outside of the
     * main thread (this is not an issue if {@link #hasAtlasFallback} is
     * false). Note that control characters (e.g. newlines) have no glyphs.
     *
     * The origin value determines the position of the bottom of the text
     * baseline. You should use the methods {@link #getDescent} and
     * {@link #getAscent} to place either the bottom or top of the text,
     * respectively.
     *
     * The quad sequence is adjusted so that all of the vertices fit in the
     * provided rectangle. The primary use-case for this is to guarantee
     * that glyphs do no spill outside of a window. This may mean that some
     * of the glyphs will be truncrated or even omitted.
     *
     * The tracking width is used to justify text in multi-line formats. If
     * track is positive, the spacing between the glyphs will be adjusted to
     * fit that width exactly. Once again, this may result in glyphs being
     * truncated if either the track width is greater then the rectangle width,
     * or if the font shrink limit is insufficent.
     *
     * The C-style string substr need not be null-terminated. Instead,
     * the termination is indicated by the parameter end. This provides
     * efficient substring processing. The string may either be in UTF8
     * or ASCII; the method will handle conversion automatically.
     *
     * @param substr    The start of the string for glyph generation
     * @param end       The end of the string for glyph generation
     * @param origin    The position of the first character
     * @param rect      The bounding box for the quads
     * @param track     The tracking width (if positive)
     *
     * @return a set of glyph runs to render the given string
     */
    std::unordered_map<GLuint, std::shared_ptr<GlyphRun>> getGlyphs(const char* substr, const char* end, const Vec2 origin,
                                                                    const Rect rect, float track=0);
    
    /**
     * Stores the glyph runs to render the given string in the given map
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
     * {@link #getAtlases} allows you to identify the atlas for each run.
     * If the map is non-empty, the glyph run data will be appended to the
     * relevant existing glyph run (if possible).
     *
     * If a character in the string is not represented by a glyph in
     * the atlas collection, then it will be skipped unless the value
     * {@link #setAtlasFallback} is set to true. In that case, this method
     * will generate a one-time atlas for these characters and return its
     * texture as part of the set. However, this atlas will not be stored
     * for future use. In addition, forcing this creation of a fallback
     * atlas makes this method no longer safe to be used outside of the
     * main thread (this is not an issue if {@link #hasAtlasFallback} is
     * false). Note that control characters (e.g. newlines) have no glyphs.
     *
     * The origin value determines the position of the bottom of the text
     * baseline. You should use the methods {@link #getDescent} and
     * {@link #getAscent} to place either the bottom or top of the text,
     * respectively.
     *
     * The C-style string substr need not be null-terminated. Instead,
     * the termination is indicated by the parameter end. This provides
     * efficient substring processing. The string may either be in UTF8
     * or ASCII; the method will handle conversion automatically.
     *
     * @param runs      The map to store the glyph runs
     * @param text      The string for glyph generation
     * @param origin    The position of the first character
     *
     * @return the number of glyphs successfully processed
     */
    size_t getGlyphs(std::unordered_map<GLuint, std::shared_ptr<GlyphRun>>& runs, const std::string text, const Vec2 origin);
    
    /**
     * Stores the glyph runs to render the given string in the given map
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
     * {@link #getAtlases} allows you to identify the atlas for each run.
     * If the map is non-empty, the glyph run data will be appended to the
     * relevant existing glyph run (if possible).
     *
     * If a character in the string is not represented by a glyph in
     * the atlas collection, then it will be skipped unless the value
     * {@link #setAtlasFallback} is set to true. In that case, this method
     * will generate a one-time atlas for these characters and return its
     * texture as part of the set. However, this atlas will not be stored
     * for future use. In addition, forcing this creation of a fallback
     * atlas makes this method no longer safe to be used outside of the
     * main thread (this is not an issue if {@link #hasAtlasFallback} is
     * false). Note that control characters (e.g. newlines) have no glyphs.
     *
     * The origin value determines the position of the bottom of the text
     * baseline. You should use the methods {@link #getDescent} and
     * {@link #getAscent} to place either the bottom or top of the text,
     * respectively.
     *
     * The string may either be in UTF8 or ASCII; the method will handle
     * conversion automatically.
     *
     * @param runs      The map to store the glyph runs
     * @param substr    The start of the string for glyph generation
     * @param end       The end of the string for glyph generation
     * @param origin    The position of the first character
     *
     * @return the number of glyphs successfully processed
     */
    size_t getGlyphs(std::unordered_map<GLuint, std::shared_ptr<GlyphRun>>& runs, const char* substr, const char* end, const Vec2 origin);

    /**
     * Stores the glyph runs to render the given string in the given map
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
     * {@link #getAtlases} allows you to identify the atlas for each run.
     * If the map is non-empty, the glyph run data will be appended to the
     * relevant existing glyph run (if possible).
     *
     * If a character in the string is not represented by a glyph in
     * the atlas collection, then it will be skipped unless the value
     * {@link #setAtlasFallback} is set to true. In that case, this method
     * will generate a one-time atlas for these characters and return its
     * texture as part of the set. However, this atlas will not be stored
     * for future use. In addition, forcing this creation of a fallback
     * atlas makes this method no longer safe to be used outside of the
     * main thread (this is not an issue if {@link #hasAtlasFallback} is
     * false). Note that control characters (e.g. newlines) have no glyphs.
     *
     * The origin value determines the position of the bottom of the text
     * baseline. You should use the methods {@link #getDescent} and
     * {@link #getAscent} to place either the bottom or top of the text,
     * respectively.
     *
     * The quad sequence is adjusted so that all of the vertices fit in the
     * provided rectangle. The primary use-case for this is to guarantee
     * that glyphs do no spill outside of a window. This may mean that some
     * of the glyphs will be truncrated or even omitted.
     *
     * The tracking width is used to justify text in multi-line formats. If
     * track is positive, the spacing between the glyphs will be adjusted to
     * fit that width exactly. Once again, this may result in glyphs being
     * truncated if either the track width is greater then the rectangle width,
     * or if the font shrink limit is insufficent.
     *
     * The string may either be in UTF8 or ASCII; the method will handle
     * conversion automatically.
     *
     * @param runs      The map to store the glyph runs
     * @param substr    The start of the string for glyph generation
     * @param end       The end of the string for glyph generation
     * @param origin    The position of the first character
     * @param rect      The bounding box for the quads
     * @param track     The tracking width (if positive)
     *
     * @return the number of glyphs successfully processed
     */
    size_t getGlyphs(std::unordered_map<GLuint, std::shared_ptr<GlyphRun>>& runs, const char* substr, const char* end,
                     const Vec2 origin, const Rect rect, float track=0);

    /**
     * Stores the glyph runs to render the given string in the given map
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
     * {@link #getAtlases} allows you to identify the atlas for each run.
     * If the map is non-empty, the glyph run data will be appended to the
     * relevant existing glyph run (if possible).
     *
     * If a character in the string is not represented by a glyph in
     * the atlas collection, then it will be skipped unless the value
     * {@link #setAtlasFallback} is set to true. In that case, this method
     * will generate a one-time atlas for these characters and return its
     * texture as part of the set. However, this atlas will not be stored
     * for future use. In addition, forcing this creation of a fallback
     * atlas makes this method no longer safe to be used outside of the
     * main thread (this is not an issue if {@link #hasAtlasFallback} is
     * false). Note that control characters (e.g. newlines) have no glyphs.
     *
     * The origin value determines the position of the bottom of the text
     * baseline. You should use the methods {@link #getDescent} and
     * {@link #getAscent} to place either the bottom or top of the text,
     * respectively.
     *
     * The quad sequence is adjusted so that all of the vertices fit in the
     * provided rectangle. The primary use-case for this is to guarantee
     * that glyphs do no spill outside of a window. This may mean that some
     * of the glyphs will be truncrated or even omitted.
     *
     * The tracking width is used to justify text in multi-line formats. If
     * track is positive, the spacing between the glyphs will be adjusted to
     * fit that width exactly. Once again, this may result in glyphs being
     * truncated if either the track width is greater then the rectangle width,
     * or if the font shrink limit is insufficent.
     *
     * The C-style string substr need not be null-terminated. Instead,
     * the termination is indicated by the parameter end. This provides
     * efficient substring processing. The string may either be in UTF8
     * or ASCII; the method will handle conversion automatically.
     *
     * @param runs      The map to store the glyph runs
     * @param text      The string for glyph generation
     * @param origin    The position of the first character
     * @param rect      The bounding box for the quads
     * @param track     The tracking width (if positive)
     *
     * @return the number of glyphs successfully processed
     */
    size_t getGlyphs(std::unordered_map<GLuint, std::shared_ptr<GlyphRun>>& runs, const std::string text,
                     const Vec2 origin, const Rect rect, float track=0);
    
    /**
     * Returns a single glyph run quad to render this character.
     *
     * The glyph run will consist of a single quad and the texture to render
     * a quad. If the character is not represented by a glyph in the atlas
     * collection, the glyph run will be empty unless {@link #setAtlasFallback}
     * is set to true. In that case, this method will generate a one-time
     * atlas for this character and return its texture as part of the glyph
     * run. However, this atlas will not be stored for future use. In addition,
     * forcing this creation of a fallback atlas makes this method no longer
     * safe to be used outside of the main thread (this is not an issue if
     * {@link #hasAtlasFallback} is false). Note that control characters (e.g.
     * newlines) have no glyphs.
     *
     * This method will append the vertices to the provided mesh and update
     * the indices to include these new vertices. In addition, it will return
     * the texture that should be used with these vertices. Finally, once the
     * glyph is generated, the offset will be adjusted to contain the next
     * place to render a character.
     *
     * The offset determines the position of the bottom of the text baseline.
     * You should use the methods {@link #getDescent} and {@link #getAscent}
     * to place either the bottom or top of the text, respectively.
     *
     * The character should be represented by a UNICODE value. For ASCII
     * characters, this agrees with the ASCII code.
     *
     * @param thechar   The character to convert to render data
     * @param offset    The (unkerned) starting position of the quad
     *
     * @return a single glyph run quad to render this character.
     */
    std::shared_ptr<GlyphRun> getGlyph(Uint32 thechar, Vec2& offset);
    
    /**
     * Returns a single glyph run quad to render this character.
     *
     * The glyph run will consist of a single quad and the texture to render
     * a quad. If the character is not represented by a glyph in the atlas
     * collection, the glyph run will be empty unless {@link #setAtlasFallback}
     * is set to true. In that case, this method will generate a one-time
     * atlas for this character and return its texture as part of the glyph
     * run. However, this atlas will not be stored for future use. In addition,
     * forcing this creation of a fallback atlas makes this method no longer
     * safe to be used outside of the main thread (this is not an issue if
     * {@link #hasAtlasFallback} is false). Note that control characters (e.g.
     * newlines) have no glyphs.
     *
     * This method will append the vertices to the provided mesh and update
     * the indices to include these new vertices. In addition, it will return
     * the texture that should be used with these vertices. Finally, once the
     * glyph is generated, the offset will be adjusted to contain the next
     * place to render a character.
     *
     * The offset determines the position of the bottom of the text baseline.
     * You should use the methods {@link #getDescent} and {@link #getAscent}
     * to place either the bottom or top of the text, respectively.
     *
     * The quad is adjusted so that all of the vertices fit in the provided
     * rectangle.  This may mean that no quad is generated at all.
     *
     * The character should be represented by a UNICODE value. For ASCII
     * characters, this agrees with the ASCII code.
     *
     * @param thechar   The character to convert to render data
     * @param offset    The (unkerned) starting position of the quad
     * @param rect      The bounding box for the quad
     *
     * @return a single glyph run quad to render this character.
     */
    std::shared_ptr<GlyphRun> getGlyph(Uint32 thechar, Vec2& offset, const Rect rect);
    
#pragma mark -
#pragma mark Glyph Debugging
    /**
     * Returns a (line) mesh of the quad outlines for the text glyphs.
     *
     * This method is useful for debugging. When this mesh is drawn together
     * with a glyph run sequence, it shows the bounding box for each glyph.
     * However, these bounding boxes are determined by the glyph metrics,
     * and do not take into account atlas padding. So they do not represent
     * potential overlaps when the padding is non-zero.
     *
     * If a character in the string is not represented by a glyph in
     * the atlas collection, then it will be skipped unless the value
     * {@link #setAtlasFallback} is set to true. In that case, this method
     * will generate a one-time atlas for these characters and return its
     * texture as part of the set. However, this atlas will not be stored
     * for future use. In addition, forcing this creation of a fallback
     * atlas makes this method no longer safe to be used outside of the
     * main thread (this is not an issue if {@link #hasAtlasFallback} is
     * false). Note that control characters (e.g. newlines) have no glyphs.
     *
     * The origin value determines the position of the bottom of the text
     * baseline. You should use the methods {@link #getDescent} and
     * {@link #getAscent} to place either the bottom or top of the text,
     * respectively.
     *
     * The string may either be in UTF8 or ASCII; the method will handle
     * conversion automatically.
     *
     * @param text      The string for glyph generation
     * @param origin    The position of the first character
     *
     * @return a (line) mesh of the quad outlines for the text glyphs
     */
    Mesh<SpriteVertex2> getGlyphBoxes(const std::string text, const Vec2 origin);
    
    /**
     * Returns a (line) mesh of the quad outlines for the text glyphs.
     *
     * This method is useful for debugging. When this mesh is drawn together
     * with a glyph run sequence, it shows the bounding box for each glyph.
     * However, these bounding boxes are determined by the glyph metrics,
     * and do not take into account atlas padding. So they do not represent
     * potential overlaps when the padding is non-zero.
     *
     * If a character in the string is not represented by a glyph in
     * the atlas collection, then it will be skipped unless the value
     * {@link #setAtlasFallback} is set to true. In that case, this method
     * will generate a one-time atlas for these characters and return its
     * texture as part of the set. However, this atlas will not be stored
     * for future use. In addition, forcing this creation of a fallback
     * atlas makes this method no longer safe to be used outside of the
     * main thread (this is not an issue if {@link #hasAtlasFallback} is
     * false). Note that control characters (e.g. newlines) have no glyphs.
     *
     * The origin value determines the position of the bottom of the text
     * baseline. You should use the methods {@link #getDescent} and
     * {@link #getAscent} to place either the bottom or top of the text,
     * respectively.
     *
     * The origin value determines the position of the bottom of the text
     * baseline. You should use the methods {@link #getDescent} and
     * {@link #getAscent} to place either the bottom or top of the text,
     * respectively.
     *
     * The C-style string substr need not be null-terminated. Instead,
     * the termination is indicated by the parameter end. This provides
     * efficient substring processing. The string may either be in UTF8
     * or ASCII; the method will handle conversion automatically.
     *
     * @param substr    The start of the string for glyph generation
     * @param end       The end of the string for glyph generation
     * @param origin    The position of the first character
     *
     * @return a (line) mesh of the quad outlines for the text glyphs
     */
    Mesh<SpriteVertex2> getGlyphBoxes(const char* substr, const char* end, const Vec2 origin);

    /**
     * Returns a (line) mesh of the quad outlines for the text glyphs.
     *
     * This method is useful for debugging. When this mesh is drawn together
     * with a glyph run sequence, it shows the bounding box for each glyph.
     * However, these bounding boxes are determined by the glyph metrics,
     * and do not take into account atlas padding. So they do not represent
     * potential overlaps when the padding is non-zero.
     *
     * If a character in the string is not represented by a glyph in
     * the atlas collection, then it will be skipped unless the value
     * {@link #setAtlasFallback} is set to true. In that case, this method
     * will generate a one-time atlas for these characters and return its
     * texture as part of the set. However, this atlas will not be stored
     * for future use. In addition, forcing this creation of a fallback
     * atlas makes this method no longer safe to be used outside of the
     * main thread (this is not an issue if {@link #hasAtlasFallback} is
     * false). Note that control characters (e.g. newlines) have no glyphs.
     *
     * The origin value determines the position of the bottom of the text
     * baseline. You should use the methods {@link #getDescent} and
     * {@link #getAscent} to place either the bottom or top of the text,
     * respectively.
     *
     * The mesh sequence is adjusted so that all of the vertices fit in the
     * provided rectangle. The primary use-case for this is to guarantee
     * that glyph boxes do no spill outside of a window. This may mean that
     * some of the boxes will be truncrated or even omitted.
     *
     * The tracking width is used to justify text in multi-line formats. If
     * track is positive, the spacing between the boxes will be adjusted to
     * fit that width exactly. Once again, this may result in boxes being
     * truncated if either the track width is greater then the rectangle
     * width, or if the font shrink limit is insufficent.
     *
     * The string may either be in UTF8 or ASCII; the method will handle
     * conversion automatically.
     *
     * @param text      The string to convert to render data.
     * @param origin    The position of the first character
     * @param rect      The bounding box for the mesh
     * @param track     The tracking width (if positive)
     *
     * @return a (line) mesh of the quad outlines for the text glyphs
     */
    Mesh<SpriteVertex2> getGlyphBoxes(const std::string text, const Vec2 origin,
                                      const Rect rect, float track=0);
    
    /**
     * Returns a (line) mesh of the quad outlines for the text glyphs.
     *
     * This method is useful for debugging. When this mesh is drawn together
     * with a glyph run sequence, it shows the bounding box for each glyph.
     * However, these bounding boxes are determined by the glyph metrics,
     * and do not take into account atlas padding. So they do not represent
     * potential overlaps when the padding is non-zero.
     *
     * If a character in the string is not represented by a glyph in
     * the atlas collection, then it will be skipped unless the value
     * {@link #setAtlasFallback} is set to true. In that case, this method
     * will generate a one-time atlas for these characters and return its
     * texture as part of the set. However, this atlas will not be stored
     * for future use. In addition, forcing this creation of a fallback
     * atlas makes this method no longer safe to be used outside of the
     * main thread (this is not an issue if {@link #hasAtlasFallback} is
     * false). Note that control characters (e.g. newlines) have no glyphs.
     *
     * The origin value determines the position of the bottom of the text
     * baseline. You should use the methods {@link #getDescent} and
     * {@link #getAscent} to place either the bottom or top of the text,
     * respectively.
     *
     * The mesh sequence is adjusted so that all of the vertices fit in the
     * provided rectangle. The primary use-case for this is to guarantee
     * that glyph boxes do no spill outside of a window. This may mean that
     * some of the boxes will be truncrated or even omitted.
     *
     * The tracking width is used to justify text in multi-line formats. If
     * track is positive, the spacing between the boxes will be adjusted to
     * fit that width exactly. Once again, this may result in boxes being
     * truncated if either the track width is greater then the rectangle
     * width, or if the font shrink limit is insufficent.
     *
     * The C-style string substr need not be null-terminated. Instead,
     * the termination is indicated by the parameter end. This provides
     * efficient substring processing. The string may either be in UTF8
     * or ASCII; the method will handle conversion automatically.
     *
     * @param substr    The start of the string for glyph generation
     * @param end       The end of the string for glyph generation
     * @param origin    The position of the first character
     * @param rect      The bounding box for the mesh
     * @param track     The tracking width (if positive)
     *
     * @return a (line) mesh of the quad outlines for the text glyphs
     */
    Mesh<SpriteVertex2> getGlyphBoxes(const char* substr, const char* end, const Vec2 origin,
                                      const Rect rect, float track=0);
    /**
     * Stores the quad outlines for the text glyphs in the given mesh.
     *
     * This method is useful for debugging. When this mesh is drawn together
     * with a glyph run sequence, it shows the bounding box for each glyph.
     * However, these bounding boxes are determined by the glyph metrics,
     * and do not take into account atlas padding. So they do not represent
     * potential overlaps when the padding is non-zero.
     *
     * If a character in the string is not represented by a glyph in
     * the atlas collection, then it will be skipped unless the value
     * {@link #setAtlasFallback} is set to true. In that case, this method
     * will generate a one-time atlas for these characters and return its
     * texture as part of the set. However, this atlas will not be stored
     * for future use. In addition, forcing this creation of a fallback
     * atlas makes this method no longer safe to be used outside of the
     * main thread (this is not an issue if {@link #hasAtlasFallback} is
     * false). Note that control characters (e.g. newlines) have no glyphs.
     *
     * The origin value determines the position of the bottom of the text
     * baseline. You should use the methods {@link #getDescent} and
     * {@link #getAscent} to place either the bottom or top of the text,
     * respectively.
     *
     * The string may either be in UTF8 or ASCII; the method will handle
     * conversion automatically.
     *
     * @param mesh      The mesh to store the new quads
     * @param text      The string for glyph generation
     * @param origin    The position of the first character
     *
     * @return the number of quads generated
     */
    size_t getGlyphBoxes(Mesh<SpriteVertex2>& mesh, const std::string text, const Vec2 origin);
    
    /**
     * Stores the quad outlines for the text glyphs in the given mesh.
     *
     * This method is useful for debugging. When this mesh is drawn together
     * with a glyph run sequence, it shows the bounding box for each glyph.
     * However, these bounding boxes are determined by the glyph metrics,
     * and do not take into account atlas padding. So they do not represent
     * potential overlaps when the padding is non-zero.
     *
     * If a character in the string is not represented by a glyph in
     * the atlas collection, then it will be skipped unless the value
     * {@link #setAtlasFallback} is set to true. In that case, this method
     * will generate a one-time atlas for these characters and return its
     * texture as part of the set. However, this atlas will not be stored
     * for future use. In addition, forcing this creation of a fallback
     * atlas makes this method no longer safe to be used outside of the
     * main thread (this is not an issue if {@link #hasAtlasFallback} is
     * false). Note that control characters (e.g. newlines) have no glyphs.
     *
     * The origin value determines the position of the bottom of the text
     * baseline. You should use the methods {@link #getDescent} and
     * {@link #getAscent} to place either the bottom or top of the text,
     * respectively.
     *
     * The C-style string substr need not be null-terminated. Instead,
     * the termination is indicated by the parameter end. This provides
     * efficient substring processing. The string may either be in UTF8
     * or ASCII; the method will handle conversion automatically.
     *
     * @param mesh      The mesh to store the new quads
     * @param substr    The start of the string for glyph generation
     * @param end       The end of the string for glyph generation
     * @param origin    The position of the first character
     *
     * @return the number of quads generated
     */
    size_t getGlyphBoxes(Mesh<SpriteVertex2>& mesh, const char* substr, const char* end, const Vec2 origin);

    /**
     * Stores the quad outlines for the text glyphs in the given mesh.
     *
     * This method is useful for debugging. When this mesh is drawn together
     * with a glyph run sequence, it shows the bounding box for each glyph.
     * However, these bounding boxes are determined by the glyph metrics,
     * and do not take into account atlas padding. So they do not represent
     * potential overlaps when the padding is non-zero.
     *
     * If a character in the string is not represented by a glyph in
     * the atlas collection, then it will be skipped unless the value
     * {@link #setAtlasFallback} is set to true. In that case, this method
     * will generate a one-time atlas for these characters and return its
     * texture as part of the set. However, this atlas will not be stored
     * for future use. In addition, forcing this creation of a fallback
     * atlas makes this method no longer safe to be used outside of the
     * main thread (this is not an issue if {@link #hasAtlasFallback} is
     * false). Note that control characters (e.g. newlines) have no glyphs.
     *
     * The origin value determines the position of the bottom of the text
     * baseline. You should use the methods {@link #getDescent} and
     * {@link #getAscent} to place either the bottom or top of the text,
     * respectively.
     *
     * The mesh sequence is adjusted so that all of the vertices fit in the
     * provided rectangle. The primary use-case for this is to guarantee
     * that glyph boxes do no spill outside of a window. This may mean that
     * some of the boxes will be truncrated or even omitted.
     *
     * The tracking width is used to justify text in multi-line formats. If
     * track is positive, the spacing between the boxes will be adjusted to
     * fit that width exactly. Once again, this may result in boxes being
     * truncated if either the track width is greater then the rectangle
     * width, or if the font shrink limit is insufficent.
     *
     * The string may either be in UTF8 or ASCII; the method will handle
     * conversion automatically.
     *
     * @param mesh      The mesh to store the new quads
     * @param text      The string to convert to render data.
     * @param origin    The position of the first character
     * @param rect      The bounding box for the mesh
     * @param track     The tracking width (if positive)
     *
     * @return the number of quads generated
     */
    size_t getGlyphBoxes(Mesh<SpriteVertex2>& mesh, const std::string text,
                         const Vec2 origin, const Rect rect, float track=0);
    
    /**
     * Stores the quad outlines for the text glyphs in the given mesh.
     *
     * This method is useful for debugging. When this mesh is drawn together
     * with a glyph run sequence, it shows the bounding box for each glyph.
     * However, these bounding boxes are determined by the glyph metrics,
     * and do not take into account atlas padding. So they do not represent
     * potential overlaps when the padding is non-zero.
     *
     * If a character in the string is not represented by a glyph in
     * the atlas collection, then it will be skipped unless the value
     * {@link #setAtlasFallback} is set to true. In that case, this method
     * will generate a one-time atlas for these characters and return its
     * texture as part of the set. However, this atlas will not be stored
     * for future use. In addition, forcing this creation of a fallback
     * atlas makes this method no longer safe to be used outside of the
     * main thread (this is not an issue if {@link #hasAtlasFallback} is
     * false). Note that control characters (e.g. newlines) have no glyphs.
     *
     * The origin value determines the position of the bottom of the text
     * baseline. You should use the methods {@link #getDescent} and
     * {@link #getAscent} to place either the bottom or top of the text,
     * respectively.
     *
     * The mesh sequence is adjusted so that all of the vertices fit in the
     * provided rectangle. The primary use-case for this is to guarantee
     * that glyph boxes do no spill outside of a window. This may mean that
     * some of the boxes will be truncrated or even omitted.
     *
     * The tracking width is used to justify text in multi-line formats. If
     * track is positive, the spacing between the boxes will be adjusted to
     * fit that width exactly. Once again, this may result in boxes being
     * truncated if either the track width is greater then the rectangle
     * width, or if the font shrink limit is insufficent.
     *
     * The C-style string substr need not be null-terminated. Instead,
     * the termination is indicated by the parameter end. This provides
     * efficient substring processing. The string may either be in UTF8
     * or ASCII; the method will handle conversion automatically.
     *
     * @param mesh      The mesh to store the new quads
     * @param substr    The start of the string for glyph generation
     * @param end       The end of the string for glyph generation
     * @param origin    The position of the first character
     * @param rect      The bounding box for the mesh
     * @param track     The tracking width (if positive)
     *
     * @return the number of quads generated
     */
    size_t getGlyphBoxes(Mesh<SpriteVertex2>& mesh, const char* substr, const char* end,
                         const Vec2 origin, const Rect rect, float track=0);
    
private:
#pragma mark -
#pragma mark Internals
    /**
     * Gathers glyph size information for the ASCII characters
     *
     * This method only gathers glyph size information if it was not
     * gathered previously (e.g. those characters are not in an existing
     * atlas). The returned deque stores only those characters that
     * were processed. This allows the atlas creation tools to determine
     * which glyphs are new, and not previously supported.
     *
     * @return the characters successfully processed
     */
    std::deque<Uint32> gatherGlyphs();

    /**
     * Gathers glyph size information for the character set
     *
     * This method only gathers glyph size information if it was not
     * gathered previously (e.g. those characters are not in an existing
     * atlas). The returned deque stores only those characters that
     * were processed. This allows the atlas creation tools to determine
     * which glyphs are new, and not previously supported.
     *
     * The character set may either be in UTF8 or ASCII; the method
     * will handle conversion automatically.
     *
     * @param charset   The characters to measure
     *
     * @return the characters successfully processed
     */
    std::deque<Uint32> gatherGlyphs(const std::string& charset);

    /**
     * Gathers glyph size information for the character set
     *
     * This method only gathers glyph size information if it was not
     * gathered previously (e.g. those characters are not in an existing
     * atlas). The returned deque stores only those characters that
     * were processed. This allows the atlas creation tools to determine
     * which glyphs are new, and not previously supported.
     *
     * The character set should be all UNICODE values.
     *
     * @param charset   The characters to measure
     *
     * @return the characters successfully processed
     */
    std::deque<Uint32> gatherGlyphs(const std::vector<Uint32>& charset);

    /**
     * Gathers the kerning information for given characters.
     *
     * These characters will not only be kerned against each other, but
     * they will also be kerned against any existing characters.
     *
     * @param glyphs    The glyphs to acquire kerning data for
     */
    void gatherKerning(const std::deque<Uint32>& glyphs);
    
    /**
     * Returns the metrics for the given character if available.
     *
     * This method returns a metric with all zeroes if no data is fount.
     *
     * @return the metrics for the given character if available.
     */
    Metrics computeMetrics(Uint32 thechar) const;

    /**
     * Returns the kerning between the two characters if available.
     *
     * The method will return -1 if there either of the two characters are
     * not supported by this font.
     *
     * @return the kerning between the two characters if available.
     */
    int computeKerning(Uint32 a, Uint32 b) const;
    
    /**
     * Creates an local atlas collection for the given character set.
     *
     * The atlases generated contains characters in the provided character set,
     * and will omit all other chacters. This includes ASCII characters that may
     * be missing from the character set. The character set string must either
     * be in ASCII or UTF8 encoding. It will handle both automatically, but no
     * other encoding (e.g. Latin1) is accepted.
     *
     * The atlas collection and its corresponding map are stored in the appropriate
     * reference variables. As a local collection, these atlases are not stored
     * they will be deleted once the shared pointers are released.  However, the
     * glyph metric and kerning information will be preserved for future use.
     *
     * WARNING: This method is not thread safe. It generates an OpenGL texture, 
     * which means that it may only be called in the main thread.
     *
     * @param charset   The set of characters in the atlas
     * @param atlas       Vector to store the atlas collection
     * @param map       Map to store the atlas directory
     *
     * @return true if the atlases were successfully created.
     */
    bool buildLocalAtlases(const std::vector<Uint32>& charset, 
                           std::vector<std::shared_ptr<Atlas>>& atlases,
                           std::unordered_map<Uint32, size_t>& map);
    
    /**
     * Creates a quad outline of this character and stores it in mesh
     *
     * This method will append the vertices to the provided mesh and update
     * the indices to include these new vertices. Once the quad is generated,
     * the offset will be adjusted to contain the next place to render a
     * character. 
     *
     * The dimensionons of the quad are determined by the metrics. Hence this
     * is a bounding box of the glyph, but it does not align with the actual
     * vertices of a rendered glyph.  That is, this quad outline is guaranteed
     * to fit within the bounds of a rendered glyph, but not necessarly to 
     * match it.
     *
     * The quad is adjusted so that all of the vertices fit in the provided
     * rectangle.  This may mean that no quad is generated at all.
     *
     * @param thechar   The character to convert to render data
     * @param offset    The (unkerned) starting position of the quad
     * @param mesh      The mesh to store the vertices
     */
    bool getOutline(Uint32 thechar, Vec2& offset, Mesh<SpriteVertex2>& mesh, const Rect rect);
    
};
    
#pragma mark -
#pragma mark Style Bit-Wise Operators
    
/**
 * Returns the int equivalent of a font style.
 *
 * @return the int equivalent of a font style.
 */
inline int operator*(Font::Style value) {
    return static_cast<int>(value);
}

/**
 * Returns the bitwise or of two font styles.
 *
 * @return the bitwise or of two font styles.
 */
inline Font::Style operator|(Font::Style lhs, Font::Style rhs) {
    return static_cast<Font::Style>((*lhs) | (*rhs));
}
    
/**
 * Returns the bitwise and of two font styles.
 *
 * @return the bitwise and of two font styles.
 */
inline Font::Style operator&(Font::Style lhs, Font::Style rhs) {
    return static_cast<Font::Style>((*lhs) & (*rhs));
}

/**
 * Returns the bitwise exclusive or of two font styles.
 *
 * @return the bitwise exclusive or of two font styles.
 */

inline Font::Style operator^(Font::Style lhs, Font::Style rhs) {
    return static_cast<Font::Style>((*lhs) ^ (*rhs));
}

/**
 * Returns the bitwise complement of a font style.
 *
 * @return the bitwise complement of a font style.
 */
inline Font::Style operator~(Font::Style lhs) {
    return static_cast<Font::Style>(~(*lhs));
}


}

#endif /* __CU_FONT_H__ */
