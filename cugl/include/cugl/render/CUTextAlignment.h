//
//  CUTextAlignment.h
//  Cornell University Game Library (CUGL)
//
//  Originally these two enums were part of the Label class. Now that text
//  layout is factored out of this class, these enums are put in their own
//  separate file so that they can be accessed by both TextLayout and Label.
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
#ifndef __CU_TEXT_ALIGNMENT_H__
#define __CU_TEXT_ALIGNMENT_H__

/**
 * @label HorizontalAlign
 *
 * This enumeration represents the horizontal alignment of each line of text.
 *
 * The horizontal alignment has two meanings. First, it is the relationship
 * of the relative alignment of multiple lines. In addition, it defines the
 * x-coordinate origin of the text layout. The later is relevant even when
 * the text layout is a single line.
 */
enum class HorizontalAlign : int {
    /**
     * Anchors the each line on the natural left side (DEFAULT)
     *
     * The x-coordinate origin will be the left side of each line of text,
     * and all lines of text will be aligned at this point. If the initial
     * character of a line of text has any natural spacing on the left-hand
     * side (e.g the character width is less than the advance), this will
     * be included.
     */
    LEFT = 0,
    /**
     * Anchors each line of text in the natural center.
     *
     * The x-coordinate origin with the be the center of each line of text,
     * and lines of text will all be centered at this point. Centering will
     * include any natural spacing (e.g the character width is less than
     * the advance) around the first and last character.
     */
    CENTER = 1,
    /**
     * Anchors each line of text on the natural right side
     *
     * The x-coordinate origin will be the right side of each line of text,
     * and all lines of text will be aligned at this point. If the final
     * character of a line of text has any natural spacing on the right-hand
     * side (e.g the character width is less than the advance), this will
     * be included.
     */
    RIGHT = 2,
    /**
     * Justifies each line of text.
     *
     * Justification means that each line of text is flush on both the left
     * and right sides (including any natural spacing around the first and
     * last character of each line). This will involve introducing additional
     * white space, as well as spaces between characters if the attribute
     * {@link Font#getStretchLimit} is non-zero. When this option is chosen,
     * the x-coordinate origin is on the natural left side.
     */
    JUSTIFY = 3,
    /**
     * Anchors the each line on the visible left side
     *
     * The x-coordinate origin will be the left side of each line of text,
     * and all lines of text will be aligned at this point. If the initial
     * character of a line of text has any natural spacing on the left-hand
     * side (e.g the character width is less than the advance), this will
     * be ignored. Hence the left edge of the first character of each line
     * will visibly align.
     */
    HARD_LEFT = 4,
    /**
     * Anchors each line of text in the visible center.
     *
     * The x-coordinate origin with the be the center of each line of text,
     * and lines of text will all be centered at this point. Centering will
     * ignore any natural spacing (e.g the character width is less than
     * the advance) around the first and last character. Hence the text will
     * be measured from the left edge of the first character to the right
     * edge of the second.
     */
    TRUE_CENTER = 5,
    /**
     * Anchors the each line on the visible right side
     *
     * The x-coordinate origin will be the right side of each line of text,
     * and all lines of text will be aligned at this point. If the final
     * character of a line of text has any natural spacing on the right-hand
     * side (e.g the character width is less than the advance), this will
     * be ignored. Hence the right edge of the last character of each line
     * will visibly align.
     */
    HARD_RIGHT = 6
};

/**
 * @label VerticalAlign
 *
 * This enumeration represents the vertical alignment of each line of text.
 *
 * The vertical defines the y-coordinate origin of this text layout. In the
 * case of multiple lines, the alignment is (often) with respect to the
 * entire block of text, not just the first line.
 */
enum class VerticalAlign {
    /**
     * Anchors the block according to the text baseline (DEFAULT)
     *
     * In the case of a multi-line block, the y-origin is the baseline of
     * the first (top) line, assuming that text reads top-to-bottom.
     */
    BASELINE = 0,
    /**
     * Anchors the block at the bottom, as defined by the font descent
     *
     * The descent is the amount space below the baseline that the font
     * reserves for its characters. This space will be included even if
     * no character in the block is below the baseline.
     *
     * In the case of a multi-line block, the y-origin is the bottom of
     * the last line.
     */
    BOTTOM = 1,
    /**
     * Anchors the block at the natural center.
     *
     * This option uses the font height when computing the center. This
     * means including the descent below the bottom line and the ascent
     * above the top line, even if no characters use that space.
     *
     * In the case of a multi-line block, the y-origin is computed as
     * the center of the ascent of the first line to the descent of the
     * last line.
     */
    MIDDLE = 2,
    /**
     * Anchors the block at the top, as defined by the font ascent
     *
     * The ascent is the amount space above the baseline that the font
     * reserves for its characters. This space will be included even if
     * no character in the block uses the full ascent.
     *
     * In the case of a multi-line block, the y-origin is the top of the
     * first line.
     */
    TOP = 3,
    /**
     * Anchors the block at the visible bottom
     *
     * This option ignores the font descent when computing the y-origin.
     * To the y-coordinate origin is placed so that the it touches the
     * character with the greatest extent below the baseline. If no
     * characters are below the baseline, it uses the baseline as the
     * bottom.
     *
     * In the case of a multi-line block, the y-origin is the bottom of
     * the last line.
     */
    HARD_BOTTOM = 4,
    /**
     * Anchors the block at the visible middle
     *
     * This option ignores the font height when computing the y-origin. It
     * measures the center from the top of the highest character to the
     * bottom of the character with greatest extent below the baseline. If
     * no characters are below the baseline, it uses the baseline as the
     * bottom.
     *
     * In the case of a multi-line block, the y-origin is centered between
     * the highest character of the first line, and the visible bottom of
     * the last line.
     */
    TRUE_MIDDLE= 5,
    /**
     * Anchors the block at the visible top
     *
     * This option ignores the font ascent when computing the y-origin.
     * To the y-coordinate origin is placed so that the it touches the
     * the top of the highest character above the baseline.
     *
     * In the case of a multi-line block, the y-origin is the top of
     * the first line.
     */
    HARD_TOP = 6,
  };


#endif /* __CU_TEXT_ALIGNMENT_H__ */
