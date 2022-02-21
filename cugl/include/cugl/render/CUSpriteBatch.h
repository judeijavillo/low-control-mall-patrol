//
//  CUSpriteBatch.h
//  Cornell University Game Library (CUGL)
//
//  This module provides one-stop shopping for basic 2d graphics.  Despite the
//  name, it is also capable of drawing solid shapes, as well as wireframes.
//  It also has support for color gradients and (rotational) scissor masks.
//
//  While it is possible to swap out the shader for this class, the shader is
//  very peculiar in how it uses uniforms.  You should study SpriteShader.frag
//  and SpriteShader.vert before making any shader changes to this class.
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
#ifndef __CU_SPRITE_BATCH_H__
#define __CU_SPRITE_BATCH_H__

#include <SDL/SDL.h>
#include <vector>
#include "CUSpriteVertex.h"
#include "CUMesh.h"
#include <cugl/math/CUMathBase.h>
#include <cugl/math/CUMat4.h>
#include <cugl/math/CUColor4.h>

// Default memory sizes
#define DEFAULT_CAPACITY  8192
#undef CLIP_MASK

namespace cugl {

/** Forward references */
class VertexBuffer;
class UniformBuffer;
class TextLayout;
class Shader;
class Affine2;
class Texture;
class Gradient;
class Scissor;
class Font;
class Rect;
class Poly2;
class Path2;

#pragma mark -
#pragma mark Stencil Effects
/**
 * An enum to support stenciling effects.
 *
 * A {@link SpriteBatch} can support many types of stencil effects. Classic
 * stencil effects including clipping (limiting drawing to a specific region)
 * or masking (prohibiting drawing to a specific region). The stencil effects
 * supported are designed with {@link scene2::CanvasNode} in mind as the primary
 * use case.
 *
 * In particular, stencil effects are designed to support simple constructive
 * area geometry operations. You can union, intersect, or subtract stencil
 * regions to produce the relevant effects. However, this is only used for
 * drawing and does not actually construct the associated geometries.
 *
 * To support the CAG operations, the sprite batch stencil buffer has two
 * areas: low and high. Operations can be applied to one or both of these
 * regions. All binary operations are operations between these two regions.
 * For example, {@link #CLIP_MASK} will restrict all drawing to the stencil
 * region defined in the low buffer, while also prohibiting any drawing to
 * the stencil region in the high buffer. This has the visible effect of
 * "subtracting" the high buffer from the low buffer.
 *
 * The CAG operations are only supported at the binary level, as we only have
 * two halves of the stencil buffer. However, using non-drawing effects like
 * {@link #CLIP_WIPE} or {@link #CLIP_CARVE}, it is possible to produce more
 * interesting nested expressions.
 *
 * Note that when using split-buffer operations, only one of the operations
 * will modify the stencil buffer. That is why there no effects such as
 * `FILL_WIPE` or `CLAMP_STAMP`.
 */
enum StencilEffect {
    /**
     * Differs the to the existing OpenGL stencil settings. (DEFAULT)
     *
     * This effect neither enables nor disables the stencil buffer. Instead
     * it uses the existing OpenGL settings.  This is the effect that you
     * should use when you need to manipulate the stencil buffer directly.
     */
    NATIVE = 0,

    /**
     * Disables any stencil effects.
     *
     * This effect directs a {@link SpriteBatch} to ignore the stencil buffer
     * (both halves) when drawing.  However, it does not clear the contents
     * of the stencil buffer.  To clear the stencil buffer, you will need to
     * call {@link SpriteBatch#clearStencil}.
     */
    NONE = 1,
    
    /**
     * Restrict all drawing to the unified stencil region.
     *
     * In order for this effect to do anything, you must have created a
     * stencil region with {@link #STAMP} or one of its variants. This
     * effect will process the drawing commands normally, but restrict all
     * drawing to the stencil region. This can be used to quickly draw
     * non-convex shapes by making a stencil and drawing a rectangle over
     * the stencil.
     *
     * This effect is the same as {@link #CLIP_JOIN} in that it respects
     * the union of the two halves of the stencil buffer.
     */
    CLIP = 2,

    /**
     * Prohibits all drawing to the unified stencil region.
     *
     * In order for this effect to do anything, you must have created a
     * stencil region with {@link #STAMP} or one of its variants. This
     * effect will process the drawing commands normally, but reject any
     * attempts to draw to the stencil region. This can be used to quickly
     * draw shape borders on top of a solid shape.
     *
     * This effect is the same as {@link #MASK_JOIN} in that it respects
     * the union of the two halves of the stencil buffer.
     */
    MASK = 3,

    /**
     * Restrict all drawing to the unified stencil region.
     *
     * In order for this effect to do anything, you must have created a
     * stencil region with {@link #STAMP} or one of its variants. This
     * effect will process the drawing commands normally, but restrict all
     * drawing to the stencil region. This can be used to quickly draw
     * non-convex shapes by making a stencil and drawing a rectangle over
     * the stencil.
     *
     * This effect is different from {@link #CLIP} in that it will zero
     * out the pixels it draws in the stencil buffer, effectively removing
     * them from the stencil region. In many applications, this is a fast
     * way to clear the stencil buffer once it is no longer needed.
     *
     * This effect is the same as {@link #FILL_JOIN} in that it respects
     * the union of the two halves of the stencil buffer.
     */
    FILL = 4,
    
    /**
     * Erases from the unified stencil region.
     *
     * This effect will not draw anything to the screen. Instead, it will
     * only draw to the stencil buffer directly. Any pixel drawn will be
     * zeroed in the buffer, removing it from the stencil region. The
     * effect {@link #FILL} is a combination of this and {@link #CLIP}.
     * Again, this is a potential optimization for clearing the stencil
     * buffer. However, on most tiled-based GPUs, it is probably faster
     * to simply clear the whole buffer.
     */
    WIPE = 5,
    
    /**
     * Adds a stencil region the unified buffer
     *
     * This effect will not have any immediate visible effects. Instead 
     * it creates a stencil region for the effects such as {@link #CLIP}, 
     * {@link #MASK}, and the like.
     *
     * The shapes are drawn to the stencil buffer using a nonzero fill
     * rule. This has the advantage that (unlike an even-odd fill rule)
     * stamps are additive and can be drawn on top of each other. However,
     * it has the disadvantage that it requires both halves of the stencil
     * buffer to store the stamp (which part of the stamp is in which half
     * is undefined).
     *
     * While this effect implements a nonzero fill rule faithfully, there
     * are technical limitations. The size of the stencil buffer means
     * that more than 256 overlapping polygons of the same orientation
     * will cause unpredictable effects. If this is a problem, use an
     * even odd fill rule instead like {@link #STAMP_NONE} (which has 
     * no such limitations).
     */
    STAMP = 6,

    /**
     * Adds a stencil region the lower buffer
     *
     * This effect will not have any immediate visible effects. Instead
     * it creates a stencil region for the effects such as {@link #CLIP},
     * {@link #MASK}, and the like.
     *
     * Like {@link #STAMP}, shapes are drawn to the stencil buffer instead
     * of the screen. But unlike stamp, this effect is always additive. It
     * ignores path orientation, and does not support holes. This allows
     * the effect to implement a nonzero fill rule while using only half
     * of the buffer. This effect is equivalent to {@link #CARVE_NONE} in
     * that it uses only the lower half.
     *
     * The primary application of this effect is to create stencils from
     * extruded paths so that overlapping sections are not drawn twice
     * (which has negative effects on alpha blending).
     */
    CARVE = 7,

    /**
     * Limits drawing so that each pixel is updated once.
     *
     * This effect is a variation of {@link #CARVE} that also draws as it
     * writes to the stencil buffer.  This guarantees that each pixel is
     * updated exactly once. This is used by extruded paths so that
     * overlapping sections are not drawn twice (which has negative
     * effects on alpha blending).
     *
     * This effect is equivalent to {@link #CLAMP_NONE} in that it uses
     * only the lower half.
     */
    CLAMP = 8,

    /**
     * Applies {@link #CLIP} using the upper stencil buffer only.
     *
     * As with {@link #CLIP}, this effect restricts drawing to the stencil
     * region. However, this effect only uses the stencil region present
     * in the upper stencil buffer.
     *
     * This effect is designed to be used with stencil regions created by
     * {@link NONE_STAMP}. While it can be used by a stencil region created
     * by {@link STAMP}, the lower stencil buffer is ignored, and hence the
     * results are unpredictable.
     */
    NONE_CLIP = 9,

    /**
     * Applies {@link #MASK} using the upper stencil buffer only.
     *
     * As with {@link #MASK}, this effect prohibits drawing to the stencil
     * region. However, this effect only uses the stencil region present
     * in the upper stencil buffer.
     *
     * This effect is designed to be used with stencil regions created by
     * {@link NONE_STAMP}. While it can be used by a stencil region created
     * by {@link STAMP}, the lower stencil buffer is ignored, and hence the
     * results are unpredictable.
     */
    NONE_MASK = 10,

    /**
     * Applies {@link #FILL} using the upper stencil buffer only.
     *
     * As with {@link #FILL}, this effect limits drawing to the stencil
     * region. However, this effect only uses the stencil region present
     * in the upper stencil buffer.  It also only zeroes out the upper
     * stencil buffer.
     *
     * This effect is designed to be used with stencil regions created by
     * {@link NONE_STAMP}. While it can be used by a stencil region created
     * by {@link STAMP}, the lower stencil buffer is ignored, and hence the
     * results are unpredictable.
     */
    NONE_FILL = 11,

    /**
     * Applies {@link #WIPE} using the upper stencil buffer only.
     *
     * As with {@link #WIPE}, this effect zeroes out the stencil region,
     * erasing parts of it. However, its effects are limited to the upper
     * stencil region.
     *
     * This effect is designed to be used with stencil regions created by
     * {@link NONE_STAMP}. While it can be used by a stencil region created
     * by {@link STAMP}, the lower stencil buffer is ignored, and hence the
     * results are unpredictable.
     */
    NONE_WIPE = 12,
    
    /**
     * Adds a stencil region to the upper buffer
     *
     * This effect will not have any immediate visible effect on the screen
     * screen. Instead, it creates a stencil region for the effects such as
     * {@link #CLIP}, {@link #MASK}, and the like.
     *
     * Unlike {@link #STAMP}, the region created is limited to the upper
     * half of the stencil buffer. That is because the shapes are drawn
     * to the buffer with an even-odd fill rule (which does not require
     * the full stencil buffer to implement). This has the disadvantage
     * that stamps drawn on top of each other have an "erasing" effect.
     * However, it has the advantage that the this stamp supports a wider
     * array of effects than the simple stamp effect.
     *
     * Use {@link #NONE_CLAMP} if you have an simple stencil with no
     * holes that you wish to write to the upper half of the buffer.
     */
    NONE_STAMP = 13,
    
    /**
     * Adds a stencil region to the upper buffer
     *
     * This value will not have any immediate visible effect on the
     * screen. Instead, it creates a stencil region for the effects
     * such as {@link #CLIP}, {@link #MASK}, and the like.

     * Like {@link #STAMP}, shapes are drawn to the stencil buffer instead
     * of the screen. But unlike stamp, this effect is always additive. It
     * ignores path orientation, and does not support holes. This allows
     * the effect to implement a nonzero fill rule while using only the
     * upper half of the buffer.
     *
     * The primary application of this effect is to create stencils from
     * extruded paths so that overlapping sections are not drawn twice
     * (which has negative effects on alpha blending).
     */
    NONE_CARVE = 14,
    
    /**
     * Uses the upper buffer to limit each pixel to single update.
     *
     * This effect is a variation of {@link #NONE_CARVE} that also draws
     * as it writes to the upper stencil buffer.  This guarantees that
     * each pixel is updated exactly once. This is used by extruded paths
     * so that overlapping sections are not drawn twice (which has negative
     * effects on alpha blending).
     */
    NONE_CLAMP = 15,
    
    /**
     * Restrict all drawing to the unified stencil region.
     *
     * This effect is the same as {@link #CLIP} in that it respects the 
     * union of the two halves of the stencil buffer.
     */
    CLIP_JOIN = 16,
    
    /**
     * Restrict all drawing to the intersecting stencil region.
     *
     * This effect is the same as {@link #CLIP}, except that it limits
     * drawing to the intersection of the stencil regions in the two
     * halves of the stencil buffer. If a unified stencil region was
     * created by {@link #STAMP}, then the results of this effect are
     * unpredictable.
     */
    CLIP_MEET = 17,
    
    /**
     * Applies {@link #CLIP} using the lower stencil buffer only.
     *
     * As with {@link #CLIP}, this effect restricts drawing to the stencil
     * region. However, this effect only uses the stencil region present
     * in the lower stencil buffer.
     *
     * This effect is designed to be used with stencil regions created by
     * {@link STAMP_NONE}. While it can be used by a stencil region created
     * by {@link STAMP}, the upper stencil buffer is ignored, and hence the
     * results are unpredictable.
     */
    CLIP_NONE = 18,

    /**
     * Applies a lower buffer {@link #CLIP} with an upper {@link #MASK}.
     *
     * This command restricts drawing to the stencil region in the lower
     * buffer while prohibiting any drawing to the stencil region in the
     * upper buffer. If this effect is applied to a unified stencil region
     * created by {@link #STAMP}, then the results are unpredictable.
     */
    CLIP_MASK = 19,
    
    /**
     * Applies a lower buffer {@link #CLIP} with an upper {@link #FILL}.
     *
     * This command restricts drawing to the stencil region in the unified
     * stencil region of the two buffers. However, it only zeroes pixels in
     * the stencil region of the upper buffer; the lower buffer is untouched.
     * If this effect is applied to a unified stencil region created by
     * {@link #STAMP}, then the results are unpredictable.
     */
    CLIP_FILL = 20,

    /**
     * Applies a lower buffer {@link #CLIP} with an upper {@link #WIPE}.
     *
     * As with {@link #WIPE}, this command does not do any drawing on screen.
     * Instead, it zeroes out the upper stencil buffer. However, it is clipped
     * by the stencil region in the lower buffer, so that it does not zero out
     * any pixel outside this region. Hence this is a way to erase the lower
     * buffer stencil region from the upper buffer stencil region.
     */
    CLIP_WIPE = 21,

    /**
     * Applies a lower buffer {@link #CLIP} with an upper {@link #STAMP}.
     *
     * As with {@link #NONE_CLAMP}, this writes a shape to the upper stencil
     * buffer using an even-odd fill rule. This means that adding a shape on
     * top of existing shape has an erasing effect. However, it also restricts
     * its operation to the stencil region in the lower stencil buffer. Note
     * that if a pixel is clipped while drawing, it will not be added the
     * stencil region in the upper buffer.
     */
    CLIP_STAMP = 22,

    /**
     * Applies a lower buffer {@link #CLIP} with an upper {@link #CARVE}.
     *
     * As with {@link #NONE_CARVE}, this writes an additive shape to the
     * upper stencil buffer. However, it also restricts its operation to
     * the stencil region in the lower stencil buffer. Note that if a pixel
     * is clipped while drawing, it will not be added the stencil region in
     * the upper buffer. Hence this is a way to copy the lower buffer stencil
     * region into the upper buffer.
     */
    CLIP_CARVE = 23,

    /**
     * Applies a lower buffer {@link #CLIP} with an upper {@link #CLAMP}.
     *
     * As with {@link #NONE_CLAMP}, this draws a nonoverlapping shape using
     * the upper stencil buffer. However, it also restricts its operation to
     * the stencil region in the lower stencil buffer. Note that if a pixel
     * is clipped while drawing, it will not be added the stencil region in
     * the upper buffer.
     */
    CLIP_CLAMP = 24,

    /**
     * Prohibits all drawing to the unified stencil region.
     *
     * This effect is the same as {@link #MASK} in that it respects the
     * union of the two halves of the stencil buffer.
     */
    MASK_JOIN = 25,
    
    /**
     * Prohibits all drawing to the intersecting stencil region.
     *
     * This effect is the same as {@link #MASK}, except that it limits
     * drawing to the intersection of the stencil regions in the two
     * halves of the stencil buffer. If a unified stencil region was
     * created by {@link #STAMP}, then the results of this effect are
     * unpredictable.
     */
    MASK_MEET = 26,
    
    /**
     * Applies {@link #MASK} using the lower stencil buffer only.
     *
     * As with {@link #MASK}, this effect prohibits drawing to the stencil
     * region. However, this effect only uses the stencil region present
     * in the lower stencil buffer.
     *
     * This effect is designed to be used with stencil regions created by
     * {@link #STAMP_NONE}. While it can be used by a stencil region created
     * by {@link #STAMP}, the upper stencil buffer is ignored, and hence
     * the results are unpredictable.
     */
    MASK_NONE = 27,

    /**
     * Applies a lower buffer {@link #MASK} with an upper {@link #CLIP}.
     *
     * This command restricts drawing to the stencil region in the upper
     * buffer while prohibiting any drawing to the stencil region in the
     * lower buffer. If this effect is applied to a unified stencil region
     * created by {@link #STAMP}, then the results are unpredictable.
     */
    MASK_CLIP = 28,
    
    /**
     * Applies a lower buffer {@link #MASK} with an upper {@link #FILL}.
     *
     * This command restricts drawing to the stencil region in the upper
     * buffer while prohibiting any drawing to the stencil region in the
     * lower buffer. However, it only zeroes the stencil region in the
     * upper buffer; the lower buffer is untouched. In addition, it will
     * only zero those pixels that were drawn.
     *
     * If this effect is applied to a unified stencil region created by
     * {@link #STAMP}, then the results are unpredictable.
     */
    MASK_FILL = 29,

    /**
     * Applies a lower buffer {@link #MASK} with an upper {@link #WIPE}.
     *
     * As with {@link #WIPE}, this command does not do any drawing on screen.
     * Instead, it zeroes out the upper stencil buffer. However, it is masked
     * by the stencil region in the lower buffer, so that it does not zero out
     * any pixel inside this region.
     */
    MASK_WIPE = 30,

    /**
     * Applies a lower buffer {@link #MASK} with an upper {@link #STAMP}.
     *
     * As with {@link #NONE_STAMP}, this writes a shape to the upper stencil
     * buffer using an even-odd fill rule. This means that adding a shape on
     * top of existing shape has an erasing effect. However, it also masks
     * its operation by the stencil region in the lower stencil buffer. Note
     * that if a pixel is masked while drawing, it will not be added the
     * stencil region in the upper buffer.
     */
    MASK_STAMP = 31,

    /**
     * Applies a lower buffer {@link #MASK} with an upper {@link #CARVE}.
     *
     * As with {@link #NONE_CARVE}, this writes an additive shape to the
     * upper stencil buffer. However, it also prohibits any drawing to the
     * stencil region in the lower stencil buffer. Note that if a pixel is
     * masked while drawing, it will not be added the stencil region in
     * the upper buffer.
     */
    MASK_CARVE = 32,

    /**
     * Applies a lower buffer {@link #MASK} with an upper {@link #CLAMP}.
     *
     * As with {@link #NONE_CLAMP}, this draws a nonoverlapping shape using
     * the upper stencil buffer. However, it also prohibits any drawing to
     * the stencil region in the lower stencil buffer. Note that if a pixel
     * is masked while drawing, it will not be added the stencil region in
     * the upper buffer.
     */
    MASK_CLAMP = 33,
    
    /**
     * Restrict all drawing to the unified stencil region.
     *
     * This effect is the same as {@link #FILL} in that it respects the
     * union of the two halves of the stencil buffer.
     */
    FILL_JOIN = 34,
    
    /**
     * Restrict all drawing to the intersecting stencil region.
     *
     * This effect is the same as {@link #FILL}, except that it limits
     * drawing to the intersection of the stencil regions in the two
     * halves of the stencil buffer.
     *
     * When zeroing out pixels, this operation zeroes out both halves
     * of the stencil buffer. If a unified stencil region was created
     * by {@link #STAMP}, the results of this effect are unpredictable.
     */
    FILL_MEET = 35,
    
    /**
     * Applies {@link #FILL} using the lower stencil buffer only.
     *
     * As with {@link #FILL}, this effect restricts drawing to the stencil
     * region. However, this effect only uses the stencil region present
     * in the lower stencil buffer. It also only zeroes the stencil region
     * in this lower buffer.
     *
     * This effect is designed to be used with stencil regions created by
     * {@link STAMP_NONE}. While it can be used by a stencil region created
     * by {@link STAMP}, the upper stencil buffer is ignored, and hence the
     * results are unpredictable.
     */
    FILL_NONE = 36,

    /**
     * Applies a lower buffer {@link #FILL} with an upper {@link #MASK}.
     *
     * This command restricts drawing to the stencil region in the lower
     * buffer while prohibiting any drawing to the stencil region in the
     * upper buffer.
     *
     * When zeroing out the stencil region, this part of the effect is only
     * applied to the lower buffer. If this effect is applied to a unified
     * stencil region created by {@link #STAMP}, then the results are
     * unpredictable.
     */
    FILL_MASK = 37,
    
    /**
     * Applies a lower buffer {@link #FILL} with an upper {@link #CLIP}.
     *
     * This command restricts drawing to the stencil region in the unified
     * stencil region of the two buffers. However, it only zeroes pixels in
     * the stencil region of the lower buffer; the lower buffer is untouched.
     * If this effect is applied to a unified stencil region created by
     * {@link #STAMP}, then the results are unpredictable.
     */
    FILL_CLIP = 38,
    
    /**
     * Applies {@link #WIPE} using the lower stencil buffer only.
     *
     * As with {@link #WIPE}, this effect zeroes out the stencil region,
     * erasing parts of it. However, its effects are limited to the lower
     * stencil region.
     *
     * This effect is designed to be used with stencil regions created by
     * {@link STAMP_NONE}. While it can be used by a stencil region created
     * by {@link STAMP}, the upper stencil buffer is ignored, and hence the
     * results are unpredictable.
     */
    WIPE_NONE = 39,

    /**
     * Applies a lower buffer {@link #WIPE} with an upper {@link #MASK}.
     *
     * This command erases from the stencil region in the lower buffer.
     * However, it limits its erasing to locations that are not masked by
     * the stencil region in the upper buffer. If this effect is applied
     * to a unified stencil region created by {@link #STAMP}, the results
     * are unpredictable.
     */
    WIPE_MASK = 40,
    
    /**
     * Applies a lower buffer {@link #WIPE} with an upper {@link #CLIP}.
     *
     * This command erases from the stencil region in the lower buffer.
     * However, it limits its erasing to locations that are contained in
     * the stencil region in the upper buffer. If this effect is applied
     * to a unified stencil region created by {@link #STAMP}, the results
     * are unpredictable.
     */
    WIPE_CLIP = 41,
    
    /**
     * Adds a stencil region to the lower buffer
     *
     * This effect will not have any immediate visible effect on the screen
     * screen. Instead, it creates a stencil region for the effects such as
     * {@link #CLIP}, {@link #MASK}, and the like.
     *
     * Unlike {@link #STAMP}, the region created is limited to the lower
     * half of the stencil buffer. That is because the shapes are drawn
     * to the buffer with an even-odd fill rule (which does not require
     * the full stencil buffer to implement). This has the disadvantage
     * that stamps drawn on top of each other have an "erasing" effect.
     * However, it has the advantage that the this stamp supports a wider
     * array of effects than the simple stamp effect.
     */
    STAMP_NONE = 42,

    /**
     * Applies a lower buffer {@link #STAMP} with an upper {@link #CLIP}.
     *
     * As with {@link #STAMP_NONE}, this writes a shape to the lower stencil
     * buffer using an even-odd fill rule. This means that adding a shape on
     * top of existing shape has an erasing effect. However, it also restricts
     * its operation to the stencil region in the upper stencil buffer. Note
     * that if a pixel is clipped while drawing, it will not be added the
     * stencil region in the lower buffer.
     */
    STAMP_CLIP = 43,

    /**
     * Applies a lower buffer {@link #STAMP} with an upper {@link #MASK}.
     *
     * As with {@link #STAMP_NONE}, this writes a shape to the lower stencil
     * buffer using an even-odd fill rule. This means that adding a shape on
     * top of existing shape has an erasing effect. However, it also masks
     * its operation by the stencil region in the upper stencil buffer. Note
     * that if a pixel is masked while drawing, it will not be added the
     * stencil region in the lower buffer.
     */
    STAMP_MASK = 44,
    
    /**
     * Adds a stencil region to both the lower and the upper buffer
     *
     * This effect will not have any immediate visible effect on the screen
     * screen. Instead, it creates a stencil region for the effects such as
     * {@link #CLIP}, {@link #MASK}, and the like.
     *
     * Unlike {@link #STAMP}, the region is create twice and put in both the
     * upper and the lower stencil buffer. That is because the shapes are drawn
     * to the buffer with an even-odd fill rule (which does not require
     * the full stencil buffer to implement). This has the disadvantage
     * that stamps drawn on top of each other have an "erasing" effect.
     * However, it has the advantage that the this stamp supports a wider
     * array of effects than the simple stamp effect.
     *
     * The use of both buffers to provide a greater degree of flexibility.
     */
    STAMP_BOTH = 45,

    /**
     * Adds a stencil region to the lower buffer
     *
     * This effect is equivalent to {@link #CARVE}, since it only uses
     * half of the stencil buffer.
     */
    CARVE_NONE = 46,
    
    /**
     * Applies a lower buffer {@link #CARVE} with an upper {@link #CLIP}.
     *
     * As with {@link #CARVE_NONE}, this writes an additive shape to the
     * lower stencil buffer. However, it also restricts its operation to
     * the stencil region in the upper stencil buffer. Note that if a pixel
     * is clipped while drawing, it will not be added the stencil region in
     * the lower buffer. Hence this is a way to copy the upper buffer stencil
     * region into the lower buffer.
     */
    CARVE_CLIP = 47,
  
    /**
     * Applies a lower buffer {@link #CARVE} with an upper {@link #MASK}.
     *
     * As with {@link #CARVE_NONE}, this writes an additive shape to the
     * lower stencil buffer. However, it also prohibits any drawing to the
     * stencil region in the upper stencil buffer. Note that if a pixel is
     * masked while drawing, it will not be added the stencil region in
     * the lower buffer.
     */
    CARVE_MASK = 48,
    
    /**
     * Adds a stencil region to both the lower and upper buffer
     *
     * This effect is similar to {@link #CARVE}, except that it uses both
     * buffers.  This is to give a wider degree of flexibility.
     */
    CARVE_BOTH = 49,
    
    /**
     * Uses the lower buffer to limit each pixel to single update.
     *
     * This effect is equivalent to {@link #CLAMP}, since it only uses
     * half of the stencil buffer.
     */
    CLAMP_NONE = 50,
    
    /**
     * Applies a lower buffer {@link #CLAMP} with an upper {@link #CLIP}.
     *
     * As with {@link #CLAMP_NONE}, this draws a nonoverlapping shape using
     * the lower stencil buffer. However, it also restricts its operation to
     * the stencil region in the upper stencil buffer. Note that if a pixel
     * is clipped while drawing, it will not be added the stencil region in
     * the lower buffer.
     */
    CLAMP_CLIP = 51,
  
    /**
     * Applies a lower buffer {@link #CLAMP} with an upper {@link #MASK}.
     *
     * As with {@link #CLAMP_NONE}, this draws a nonoverlapping shape using
     * the lower stencil buffer. However, it also prohibits any drawing to
     * the stencil region in the upper stencil buffer. Note that if a pixel
     * is masked while drawing, it will not be added the stencil region in
     * the lower buffer.
     */
    CLAMP_MASK = 52
    
};

#pragma mark -
#pragma mark SpriteBatch
/**
 * This class is a sprite batch for drawing 2d graphics.
 *
 * A sprite batch gathers together sprites and draws them as a single mesh
 * whenever possible. However this sprite batch is different from a classic sprite
 * batch (from XNA or LibGDX) in that it provides a complete 2d graphics pipeline
 * supporting both solid shapes and outlines, with texture, gradient, and scissor
 * mask support.
 *
 * This sprite batch is capable of drawing with an active texture. In that case,
 * the shape will be drawn with a solid color. If no color has been specified, the
 * default color is white. Outlines use the same texturing rules that solids do.
 * There is also support for a simple, limited radius blur effect on textures.
 *
 * Color gradient support is provided by the {@link Gradient} class. All gradients
 * will be tinted by the current color (so the color should be reset to white
 * before using a gradient).
 *
 * Scissor masks are supported by the {@link Scissor} class. This is useful for
 * constraining shapes to an internal window. A scissor mask must be a transformed
 * rectangle; it cannot mask with arbitrary polygons.
 *
 * Drawing only occurs when the methods {@link #flush} or {@link #end} are 
 * called. Because loading vertices into a {@link VertexBuffer} is an expensive 
 * operation, this sprite batch attempts to minimize this as much as possible.
 * Even texture switches are batched.  However, it is still true that using a
 * single texture atlas can significantly improve drawing speed.
 *
 * A review of this class shows that there are a lot of redundant drawing methods.
 * The scene graphs only use the {@link Mesh} methods. This goal has been to make 
 * this class more accessible to students familiar with classic sprite batches 
 * found in LibGDX or XNA.
 *
 * It is possible to swap out the shader for this class with another one.  Any
 * shader for this class should support {@link SpriteVertex2} as its vertex data.
 * If you need additional vertex information, such as normals, you should create
 * a new class.  It should also have a uniform for the perspective matrix,
 * texture, and drawing type (type 0). Support for gradients and scissors occur
 * via a uniform block that is provides the data in the order scissor, and then
 * gradient.  See SpriteShader.frag for more information.
 *
 * This is an extremely heavy-weight class. There is rarely any need to have more
 * than one of these at a time. If you want to implement your own shader effects,
 * it is better to construct your own custom pipeline with {@link Shader} and
 * {@link VertexBuffer}.
 */
class SpriteBatch {
#pragma mark Values
private:
    
    /**
     * A class storing the drawing context for the associate shader.
     *
     * Because we want to minimize the number of times we load vertices
     * to the vertex buffer, all uniforms are recorded and delayed until the
     * final graphics call.  We include blending attributes as part of the
     * context, since they have similar performance characteristics to
     * other uniforms
     */
    class Context;

    /** Whether this sprite batch has been initialized yet */
    bool _initialized;
    /** Whether this sprite batch is currently active */
    bool _active;
    
    /** The shader for this sprite batch */
    std::shared_ptr<Shader> _shader;
    /** The vertex buffer for this sprite batch */
    std::shared_ptr<VertexBuffer>  _vertbuff;
    /** The vertex buffer for this sprite batch */
    std::shared_ptr<UniformBuffer> _unifbuff;
    
    /** The sprite batch vertex mesh */
    SpriteVertex2* _vertData;
    /** The vertex capacity of the mesh */
    unsigned int _vertMax;
    /** The number of vertices in the current mesh */
    unsigned int _vertSize;

    /** The indices for the vertex mesh */
    GLuint*  _indxData;
    /** The index capacity of the mesh */
    unsigned int _indxMax;
    /** The number of indices in the current mesh */
    unsigned int _indxSize;
    
    /** The active drawing context */
    Context* _context;
    /** Whether the current context has been used. */
    bool _inflight;
    /** The drawing context history */
    std::vector<Context*> _history;
    
    /** The active color */
    Color4 _color;
    
    /** The active gradient */
    std::shared_ptr<Gradient> _gradient;
    /** The active scissor mask */
    std::shared_ptr<Scissor>  _scissor;

    // Monitoring values
    /** The number of vertices drawn in this pass (so far) */
    unsigned int _vertTotal;
    /** The number of OpenGL calls in this pass (so far) */
    unsigned int _callTotal;
    

#pragma mark -
#pragma mark Constructors
public:
    /**
     * Creates a degenerate sprite batch with no buffers.
     *
     * You must initialize the buffer before using it.
     */
    SpriteBatch();
    
    /**
     * Deletes the sprite batch, disposing all resources
     */
    ~SpriteBatch() { dispose(); }

    /**
     * Deletes the vertex buffers and resets all attributes.
     *
     * You must reinitialize the sprite batch to use it.
     */
    void dispose();
    
    /** 
     * Initializes a sprite batch with the default vertex capacity.
     *
     * The default vertex capacity is 8192 vertices and 8192*3 = 24576 indices.
     * If the mesh exceeds these values, the sprite batch will flush before
     * before continuing to draw. Similarly uniform buffer is initialized with
     * 512 buffer positions. This means that the uniform buffer is comparable
     * in memory size to the vertices, but only allows 512 gradient or scissor
     * mask context switches before the sprite batch must flush. If you wish to
     * increase (or decrease) the capacity, use the alternate initializer.
     *
     * The sprite batch begins with no active texture, and the color white.
     * The perspective matrix is the identity.
     *
     * @return true if initialization was successful.
     */
    bool init();
    
    /**
     * Initializes a sprite batch with the default vertex capacity and given shader
     *
     * The default vertex capacity is 8192 vertices and 8192*3 = 24576 indices.
     * If the mesh exceeds these values, the sprite batch will flush before
     * before continuing to draw. Similarly uniform buffer is initialized with
     * 512 buffer positions. This means that the uniform buffer is comparable
     * in memory size to the vertices, but only allows 512 gradient or scissor
     * mask context switches before the sprite batch must flush. If you wish to
     * increase (or decrease) the capacity, use the alternate initializer.
     *
     * The sprite batch begins with no active texture, and the color white.
     * The perspective matrix is the identity.
     *
     * See the class description for the properties of a valid shader.
     *
     * @param shader The shader to use for this spritebatch
     *
     * @return true if initialization was successful.
     */
    bool init(const std::shared_ptr<Shader>& shader) {
        return init(DEFAULT_CAPACITY,shader);
    }
    
    /**
     * Initializes a sprite batch with the given vertex capacity.
     *
     * The index capacity will be 3 times the vertex capacity. The maximum
     * number of possible indices is the maximum size_t, so the vertex size
     * must be a third that.  In addition, the sprite batch will allocate
     * 1/16 of the vertex capacity for uniform blocks (for gradients and
     * scissor masks). This means that the uniform buffer is comparable
     * in memory size to the vertices while still allowing a reasonably
     * high rate of change for quads and regularly shaped sprites.
     *
     * If the mesh exceeds the capacity, the sprite batch will flush before
     * before continuing to draw. You should tune your system to have the 
     * appropriate capacity.  To small a capacity will cause the system to
     * thrash. However, too large a capacity could stall on memory transfers.
     *
     * The sprite batch begins with no active texture, and the color white.
     * The perspective matrix is the identity.
     *
     * @param capacity The vertex capacity of this spritebatch
     *
     * @return true if initialization was successful.
     */
    bool init(unsigned int capacity);
    
    /**
     * Initializes a sprite batch with the given vertex capacity and shader
     *
     * The index capacity will be 3 times the vertex capacity. The maximum
     * number of possible indices is the maximum size_t, so the vertex size
     * must be a third that.  In addition, the sprite batch will allocate
     * 1/16 of the vertex capacity for uniform blocks (for gradients and
     * scissor masks). This means that the uniform buffer is comparable
     * in memory size to the vertices while still allowing a reasonably
     * high rate of change for quads and regularly shaped sprites.
     *
     * If the mesh exceeds the capacity, the sprite batch will flush before
     * before continuing to draw. You should tune your system to have the
     * appropriate capacity.  To small a capacity will cause the system to
     * thrash. However, too large a capacity could stall on memory transfers.
     *
     * The sprite batch begins with no active texture, and the color white.
     * The perspective matrix is the identity.
     *
     * See the class description for the properties of a valid shader.
     *
     * @param capacity The vertex capacity of this spritebatch
     * @param shader The shader to use for this spritebatch
     *
     * @return true if initialization was successful.
     */
    bool init(unsigned int capacity, const std::shared_ptr<Shader>& shader);
    
    
#pragma mark -
#pragma mark Static Constructors
    /**
     * Returns a new sprite batch with the default vertex capacity.
     *
     * The default vertex capacity is 8192 vertices and 8192*3 = 24576 indices.
     * If the mesh exceeds these values, the sprite batch will flush before
     * before continuing to draw. Similarly uniform buffer is initialized with
     * 512 buffer positions. This means that the uniform buffer is comparable
     * in memory size to the vertices, but only allows 512 gradient or scissor
     * mask context switches before the sprite batch must flush. If you wish to
     * increase (or decrease) the capacity, use the alternate allocator.
     *
     * The sprite batch begins with no active texture, and the color white.
     * The perspective matrix is the identity.
     *
     * @return a new sprite batch with the default vertex capacity.
     */
    static std::shared_ptr<SpriteBatch> alloc() {
        std::shared_ptr<SpriteBatch> result = std::make_shared<SpriteBatch>();
        return (result->init() ? result : nullptr);
    }

    /**
     * Returns a new sprite batch with the default vertex capacity and given shader
     *
     * The default vertex capacity is 8192 vertices and 8192*3 = 24576 indices.
     * If the mesh exceeds these values, the sprite batch will flush before
     * before continuing to draw. Similarly uniform buffer is initialized with
     * 512 buffer positions. This means that the uniform buffer is comparable
     * in memory size to the vertices, but only allows 512 gradient or scissor
     * mask context switches before the sprite batch must flush. If you wish to
     * increase (or decrease) the capacity, use the alternate allocator.
     *
     * The sprite batch begins with no active texture, and the color white.
     * The perspective matrix is the identity.
     *
     * See the class description for the properties of a valid shader.
     *
     * @param shader The shader to use for this spritebatch
     *
     * @return a new sprite batch with the default vertex capacity and given shader
     */
    static std::shared_ptr<SpriteBatch> alloc(const std::shared_ptr<Shader>& shader) {
        std::shared_ptr<SpriteBatch> result = std::make_shared<SpriteBatch>();
        return (result->init(shader) ? result : nullptr);
    }
    
    /**
     * Returns a new sprite batch with the given vertex capacity.
     *
     * The index capacity will be 3 times the vertex capacity. The maximum
     * number of possible indices is the maximum size_t, so the vertex size
     * must be a third that.  In addition, the sprite batch will allocate
     * 1/16 of the vertex capacity for uniform blocks (for gradients and
     * scissor masks). This means that the uniform buffer is comparable
     * in memory size to the vertices while still allowing a reasonably
     * high rate of change for quads and regularly shaped sprites.
     *
     * If the mesh exceeds the capacity, the sprite batch will flush before
     * before continuing to draw. You should tune your system to have the
     * appropriate capacity.  To small a capacity will cause the system to
     * thrash. However, too large a capacity could stall on memory transfers.
     *
     * The sprite batch begins with the default blank texture, and color
     * white. The perspective matrix is the identity.
     *
     * @param capacity The vertex capacity of this spritebatch
     *
     * @return a new sprite batch with the given vertex capacity.
     */
    static std::shared_ptr<SpriteBatch> alloc(unsigned int capacity) {
        std::shared_ptr<SpriteBatch> result = std::make_shared<SpriteBatch>();
        return (result->init(capacity) ? result : nullptr);
    }
    
    /**
     * Returns a new sprite batch with the given vertex capacity and shader
     *
     * The index capacity will be 3 times the vertex capacity. The maximum
     * number of possible indices is the maximum size_t, so the vertex size
     * must be a third that.  In addition, the sprite batch will allocate
     * 1/16 of the vertex capacity for uniform blocks (for gradients and
     * scissor masks). This means that the uniform buffer is comparable
     * in memory size to the vertices while still allowing a reasonably
     * high rate of change for quads and regularly shaped sprites.
     *
     * If the mesh exceeds the capacity, the sprite batch will flush before
     * before continuing to draw. You should tune your system to have the
     * appropriate capacity.  To small a capacity will cause the system to
     * thrash. However, too large a capacity could stall on memory transfers.
     *
     * The sprite batch begins with the default blank texture, and color
     * white. The perspective matrix is the identity.
     *
     * See the class description for the properties of a valid shader.
     *
     * @param capacity The vertex capacity of this spritebatch
     * @param shader The shader to use for this spritebatch
     *
     * @return a new sprite batch with the given vertex capacity and shader
     */
    static std::shared_ptr<SpriteBatch> alloc(unsigned int capacity, const std::shared_ptr<Shader>& shader) {
        std::shared_ptr<SpriteBatch> result = std::make_shared<SpriteBatch>();
        return (result->init(capacity,shader) ? result : nullptr);
    }

#pragma mark -
#pragma mark Attributes
    /**
     * Returns true if this sprite batch has been initialized and is ready for use.
     *
     * @return true if this sprite batch has been initialized and is ready for use.
     */
    bool isReady() const { return _initialized; }
    
    /**
     * Returns whether this sprite batch is actively drawing.
     *
     * A sprite batch is in use if begin() has been called without the
     * requisite end() to flush the pipeline.
     *
     * @return whether this sprite batch is actively drawing.
     */
    bool isDrawing() const { return _active; }

    /**
     * Returns the number of vertices drawn in the latest pass (so far).
     *
     * This value will be reset to 0 whenever begin() is called.
     *
     * @return the number of vertices drawn in the latest pass (so far).
     */
    unsigned int getVerticesDrawn() const { return _vertTotal; }

    /**
     * Returns the number of OpenGL calls in the latest pass (so far).
     *
     * This value will be reset to 0 whenever begin() is called.
     *
     * @return the number of OpenGL calls in the latest pass (so far).
     */
    unsigned int getCallsMade() const { return _callTotal; }

    /**
     * Sets the shader for this sprite batch
     *
     * This value may NOT be changed during a drawing pass. See the
     * class description for the properties of a valid shader.
     *
     * @param shader The active color for this sprite batch
     */
    void setShader(const std::shared_ptr<Shader>& shader);
    
    /**
     * Returns the shader for this sprite batch
     *
     * This value may NOT be changed during a drawing pass. See the
     * class description for the properties of a valid shader.
     *
     * @return the shader for this sprite batch
     */
    const std::shared_ptr<Shader>& getShader() const { return _shader; }
    
    /**
     * Sets the active color of this sprite batch 
     *
     * All subsequent shapes and outlines drawn by this sprite batch will be
     * tinted by this color.  This color is white by default.
     *
     * @param color The active color for this sprite batch
     */
    void setColor(const Color4 color);

    /**
     * Returns the active color of this sprite batch
     *
     * All subsequent shapes and outlines drawn by this sprite batch will be
     * tinted by this color.  This color is white by default.
     *
     * @return the active color of this sprite batch
     */
    const Color4 getColor() const { return _color; }

    /**
     * Sets the active perspective matrix of this sprite batch
     *
     * The perspective matrix is the combined modelview-projection from the
     * camera. By default, this is the identity matrix.
     *
     * @param perspective   The active perspective matrix for this sprite batch
     */
    void setPerspective(const Mat4& perspective);
    
    /**
     * Returns the active perspective matrix of this sprite batch
     *
     * The perspective matrix is the combined modelview-projection from the
     * camera.  By default, this is the identity matrix.
     *
     * @return the active perspective matrix of this sprite batch
     */
    const Mat4& getPerspective() const;
    
    /**
     * Sets the active texture of this sprite batch
     *
     * All subsequent shapes and outlines drawn by this sprite batch will use
     * this texture.  If the value is nullptr, all shapes and outlines will be
     * draw with a solid color instead.  This value is nullptr by default.
     *
     * @param texture The active texture for this sprite batch
     */
    void setTexture(const std::shared_ptr<Texture>& texture);

    /**
     * Returns the active texture of this sprite batch
     *
     * All subsequent shapes and outlines drawn by this sprite batch will use
     * this texture.  If the value is nullptr, all shapes and outlines will be
     * drawn with a solid color instead.  This value is nullptr by default.
     *
     * @return the active texture of this sprite batch
     */
    const std::shared_ptr<Texture>& getTexture() const;

    /**
     * Sets the active gradient of this sprite batch
     *
     * Gradients may be used in the place of (and together with) colors.
     * Gradients are like applied textures, and use the gradient coordinates
     * in {@link SpriteVertex2} as their texture coordinates.
     *
     * If this value is nullptr, then no gradient is active. In that case,
     * the color vertex attribute will be interpretted as normal (e.g. a
     * traditional color vector).  This value is nullptr by default.
     *
     * All gradients are tinted by the active color. Unless you explicitly
     * want this tinting, you should set the active color to white before
     * drawing with an active gradient.
     *
     * This method acquires a copy of the gradient. Changes to the original
     * gradient after calling this method have no effect.
     *
     * @param gradient   The active gradient for this sprite batch
     */
    void setGradient(const std::shared_ptr<Gradient>& gradient);
    
    /**
     * Returns the active gradient of this sprite batch
     *
     * Gradients may be used in the place of (and together with) colors.
     * Gradients are like applied textures, and use the gradient coordinates
     * in {@link SpriteVertex2} as their texture coordinates.
     *
     * If this value is nullptr, then no gradient is active. In that case,
     * the color vertex attribute will be interpretted as normal (e.g. a
     * traditional color vector).  This value is nullptr by default.
     *
     * All gradients are tinted by the active color. Unless you explicitly
     * want this tinting, you should set the active color to white before
     * drawing with an active gradient.
     *
     * This method returns a copy of the internal gradient. Changes to this
     * object have no effect on the sprite batch.
     *
     * @return The active gradient for this sprite batch
     */
    std::shared_ptr<Gradient> getGradient() const;
    
    /**
     * Sets the active scissor mask of this sprite batch
     *
     * Scissor masks may be combined with all types of drawing (colors,
     * textures, and gradients).  They are specified in the same coordinate
     * system as {@link getPerspective}. 
     *
     * If this value is nullptr, then no scissor mask is active. This value
     * is nullptr by default.
     *
     * This method acquires a copy of the scissor. Changes to the original
     * scissor mask after calling this method have no effect.
     *
     * @param scissor   The active scissor mask for this sprite batch
     */
    void setScissor(const std::shared_ptr<Scissor>& scissor);
     
    /**
     * Returns the active scissor mask of this sprite batch
     *
     * Scissor masks may be combined with all types of drawing (colors,
     * textures, and gradients).  They are specified in the same coordinate
     * system as {@link getPerspective}. 
     *
     * If this value is nullptr, then no scissor mask is active. This value
     * is nullptr by default.
     *
     * This method returns a copy of the internal scissor. Changes to this
     * object have no effect on the sprite batch.
     *
     * @return The active scissor mask for this sprite batch
     */
    std::shared_ptr<Scissor> getScissor() const;
    
    /**
     * Sets the blending function for the source color
     *
     * The enums are the standard ones supported by OpenGL.  See
     *
     *      https://www.opengl.org/sdk/docs/man/html/glBlendFunc.xhtml
     *
     * However, this setter does not do any error checking to verify that
     * the enums are valid.
     *
     * By default this value is GL_SRC_ALPHA, as sprite batches do not
     * use premultiplied alpha.
     *
     * @param func  Specifies how the source color is blended
     */
    void setSrcBlendFunc(GLenum func) {
        setSrcBlendFunc(func,func);
    }
    
    /**
     * Sets the blending functions for the source color
     *
     * The enums are the standard ones supported by OpenGL.  See
     *
     *      https://www.opengl.org/sdk/docs/man/html/glBlendFunc.xhtml
     *
     * This version of the function allows you to specify different
     * blending fuctions for the RGB and alpha components of the source
     * color. This setter does not do any error checking to verify that
     * the enums are valid.
     *
     * By default both values are GL_SRC_ALPHA, as sprite batches do not
     * use premultiplied alpha.
     *
     * @param rgb       The blend function for the source RGB components
     * @param alpha     The blend function for the source alpha component
     */
    void setSrcBlendFunc(GLenum rgb, GLenum alpha);
    
    /** 
     * Returns the source blending function for the RGB components
     *
     * By default this value is GL_SRC_ALPHA, as sprite batches do not
     * use premultiplied alpha. For other options, see
     *
     *      https://www.opengl.org/sdk/docs/man/html/glBlendFunc.xhtml
     *
     * @return the source blending function for the RGB components
     */
    GLenum getSrcBlendRGB() const;

    /**
     * Returns the source blending function for the alpha component
     *
     * By default this value is GL_SRC_ALPHA, as sprite batches do not
     * use premultiplied alpha. For other options, see
     *
     *      https://www.opengl.org/sdk/docs/man/html/glBlendFunc.xhtml
     *
     * @return the source blending function for the alpha component
     */
    GLenum getSrcBlendAlpha() const;


    /**
     * Sets the blending function for the destination color
     *
     * The enums are the standard ones supported by OpenGL.  See
     *
     *      https://www.opengl.org/sdk/docs/man/html/glBlendFunc.xhtml
     *
     * However, this setter does not do any error checking to verify that
     * the enums are valid.
     *
     * By default this value is GL_ONE_MINUS_SRC_ALPHA, as sprite batches
     * do not use premultiplied alpha.
     *
     * @param func  Specifies how the source color is blended
     */
    void setDstBlendFunc(GLenum func) {
        setDstBlendFunc(func, func);
    }
    
    /**
     * Sets the blending functions for the destination color
     *
     * The enums are the standard ones supported by OpenGL.  See
     *
     *      https://www.opengl.org/sdk/docs/man/html/glBlendFunc.xhtml
     *
     * This version of the function allows you to specify different
     * blending fuctions for the RGB and alpha components of the
     * destiniation color. This setter does not do any error checking
     * to verify that the enums are valid. 
     *
     * By default both values are GL_ONE_MINUS_SRC_ALPHA, as sprite
     * batches do not use premultiplied alpha.
     *
     * @param rgb       The blend function for the source RGB components
     * @param alpha     The blend function for the source alpha component
     */
    void setDstBlendFunc(GLenum rgb, GLenum alpha);
    
    /**
     * Returns the destination blending function for the RGB components
     *
     * By default this value is GL_ONE_MINUS_SRC_ALPHA, as sprite batches 
     * do not use premultiplied alpha. For other options, see
     *
     *      https://www.opengl.org/sdk/docs/man/html/glBlendFunc.xhtml
     *
     * @return the destination blending function for the RGB components
     */
    GLenum getDstBlendRGB() const;

    /**
     * Returns the destination blending function for the alpha component
     *
     * By default this value is GL_ONE_MINUS_SRC_ALPHA, as sprite batches 
     * do not use premultiplied alpha. For other options, see
     *
     *      https://www.opengl.org/sdk/docs/man/html/glBlendFunc.xhtml
     *
     * @return the destination blending function for the alpha component
     */
    GLenum getDstBlendAlpha() const;
    
    /**
     * Sets the blending equation for this sprite batch
     *
     * The enum must be a standard ones supported by OpenGL.  See
     *
     *      https://www.opengl.org/sdk/docs/man/html/glBlendEquation.xhtml
     *
     * However, this setter does not do any error checking to verify that
     * the input is valid.  By default, the equation is GL_FUNC_ADD.
     *
     * @param equation  Specifies how source and destination colors are combined
     */
    void setBlendEquation(GLenum equation);

    /**
     * Returns the blending equation for this sprite batch
     *
     * By default this value is GL_FUNC_ADD. For other options, see
     *
     *      https://www.opengl.org/sdk/docs/man/html/glBlendEquation.xhtml
     *
     * @return the blending equation for this sprite batch
     */
    GLenum getBlendEquation() const;
    
    /**
     * Sets the current depth of this sprite batch.
     *
     * The depth value is appended to all 2d shapes drawn by this sprite 
     * batch.If this value is non-zero then depth testing is turned
     * on. However, the exact depth function is up to you and should
     * be set outside of this sprite batch.
     *
     * @param depth The current depth of this sprite batch.
     */
    void setDepth(float depth);
    
    /**
     * Returns the current depth of this sprite batch.
     *
     * The depth value is appended to all 2d shapes drawn by this sprite
     * batch.If this value is non-zero then depth testing is turned
     * on. However, the exact depth function is up to you and should
     * be set outside of this sprite batch.
     *
     * @return the current depth of this sprite batch.
     */
    float getDepth() const;

    /**
     * Sets the blur radius in pixels (0 if there is no blurring).
     *
     * This sprite batch supports a simple Gaussian blur. The blur
     * samples at 5 points along each axis. The values are essentially
     * at the radius, half-radius, and center. Because of the limited
     * sampling, large radii will start to produce a pixellation effect.
     * But it can produce acceptable blur effects with little cost to
     * performance. It is especially ideal for font-blur effects on
     * font atlases.
     *
     * When applying a blur to a {@link GlyphRun}, make sure that the
     * source {@link Font} has {@link Font#setPadding} set to at least
     * the blur radius. Otherwise, the blur will bleed into other glyphs.
     *
     * Setting this value to 0 will disable texture blurring.  This
     * value is 0 by default.
     *
     * @param radius    The blur radius in pixels
     */
    void setBlur(GLfloat radius);

    /**
     * Returns the blur radius in pixels (0 if there is no blurring).
     *
     * This sprite batch supports a simple Gaussian blur. The blur
     * samples at 5 points along each axis. The values are essentially
     * at the radius, half-radius, and center. Because of the limited
     * sampling, large radii will start to produce a pixellation effect.
     * But it can produce acceptable blur effects with little cost to
     * performance. It is especially ideal for font-blur effects on
     * font atlases.
     *
     * When applying a blur to a {@link GlyphRun}, make sure that the
     * source {@link Font} has {@link Font#setPadding} set to at least
     * the blur radius. Otherwise, the blur will bleed into other glyphs.
     *
     * Setting this value to 0 will disable texture blurring.  This
     * value is 0 by default.
     *
     * @return the blur radius in pixels (0 if there is no blurring).
     */
    GLfloat getBlur() const;

    /**
     * Sets the current stencil effect
     *
     * Stencil effects can be used to restrict the drawing region and
     * are generally used to speed up the processing of non-convex
     * shapes. See {@link StencilEffect} for the list of supported
     * effects, as well as a discussion of how the two halves of the
     * stencil buffer work.
     *
     * This value should be set to {@link StencilEffect#NATIVE} (the
     * default) if you wish to directly manipulate the OpenGL stencil.
     * This is sometimes necessary for more complex effects.
     *
     * @param effect    The current stencil effect
     */
    void setStencilEffect(StencilEffect effect);

    /**
     * Returns the current stencil effect
     *
     * Stencil effects can be used to restrict the drawing region and
     * are generally used to speed up the processing of non-convex
     * shapes. See {@link StencilEffect} for the list of supported
     * effects, as well as a discussion of how the two halves of the
     * stencil buffer work.
     *
     * This value should be set to {@link StencilEffect#NATIVE} (the
     * default) if you wish to directly manipulate the OpenGL stencil.
     * This is sometimes necessary for more complex effects.
     *
     * @return the current stencil effect
     */
    StencilEffect getStencilEffect() const;
    
    /**
     * Clears the stencil buffer.
     *
     * This method clears both halves of the stencil buffer: both upper
     * and lower. See {@link StencilEffect} for a discussion of how the
     * two halves of the stencil buffer work.
     */
    void clearStencil();
    
    /**
     * Clears half of the stencil buffer.
     *
     * This method clears only one of the two halves of the stencil
     * buffer. See {@link StencilEffect} for a discussion of how the
     * two halves of the stencil buffer work.
     *
     * @param lower     Whether to clear the lower stencil buffer
     */
    void clearHalfStencil(bool lower);
    

#pragma mark -
#pragma mark Rendering
    /**
     * Starts drawing with the current perspective matrix.
     *
     * This call will disable depth buffer writing. It enables blending and
     * texturing. You must call either {@link #flush} or {@link #end} to
     * complete drawing.
     *
     * Calling this method will reset the vertex and OpenGL call counters to 0.
     */
    void begin();
    
    /**
     * Starts drawing with the given perspective matrix.
     *
     * This call will disable depth buffer writing. It enables blending and
     * texturing. You must call either {@link #flush} or {@link #end} to
     * complete drawing.
     *
     * Calling this method will reset the vertex and OpenGL call counters to 0.
     *
     * @param perspective   The perspective matrix to draw with.
     */
    void begin(const Mat4& perspective) {
        setPerspective(perspective); begin();
    }
    
    /**
     * Completes the drawing pass for this sprite batch, flushing the buffer.
     *
     * This method enables depth writes and disables blending and texturing. It
     * must always be called after a call to {@link #begin}.
     */
    void end();
    
    /**
     * Flushes the current mesh without completing the drawing pass.
     *
     * This method is called whenever you change any attribute other than color
     * mid-pass. It prevents the attribute change from retoactively affecting
     * previuosly drawn shapes.
     *
     * If you play to apply any OpenGL functionality not directly supported by
     * this sprite batch (e.g stencils), you MUST call this method first before
     * applying your effects.  In addition, you should call this again before
     * restoring the OpenGL state.
     */
    void flush();

    
#pragma mark -
#pragma mark Solid Shapes
    /**
     * Draws the given rectangle filled with the current color and texture.
     *
     * The texture will fill the entire rectangle with texture coordinate 
     * (0,1) at the bottom left corner identified by rect,origin. To draw only
     * part of a texture, use a subtexture to fill the rectangle with the
     * region [minS,maxS]x[min,maxT]. Alternatively, you can use a {@link Poly2}
     * for more fine-tuned control.
     *
     * If depth testing is on, all vertices will use the current sprite
     * batch depth.
     *
     * @param rect      The rectangle to draw
     */
    void fill(Rect rect);
    
    /**
     * Draws the given rectangle filled with the current color and texture.
     *
     * The texture will fill the entire rectangle with texture coordinate
     * (0,1) at the bottom left corner identified by rect,origin. To draw only
     * part of a texture, use a subtexture to fill the rectangle with the
     * region [minS,maxS]x[min,maxT]. Alternatively, you can use a {@link Poly2}
     * for more fine-tuned control.
     *
     * If depth testing is on, all vertices will use the current sprite
     * batch depth.
     *
     * @param rect      The rectangle to draw
     * @param offset    The rectangle offset
     */
    void fill(const Rect rect, const Vec2 offset);
    
    /**
     * Draws the given rectangle filled with the current color and texture.
     *
     * The rectangle will be scaled first, then rotated, and finally offset
     * by the given position. Rotation is measured in radians and is counter
     * clockwise from the x-axis.  Rotation will be about the provided origin, 
     * which is specified relative to the origin of the rectangle (not world 
     * coordinates).  So to spin about the center, the origin should be width/2, 
     * height/2 of the rectangle.
     *
     * The texture will fill the entire rectangle before being transformed. 
     * Texture coordinate (0,1) will at the bottom left corner identified by 
     * rect,origin. To draw only part of a texture, use a subtexture to fill 
     * the rectangle with the region [minS,maxS]x[min,maxT]. Alternatively,
     * you can use a {@link Poly2} for more fine-tuned control.
     *
     * If depth testing is on, all vertices will use the current sprite
     * batch depth.
     *
     * @param rect      The rectangle to draw
     * @param origin    The rotational offset in the rectangle
     * @param scale     The amount to scale the rectangle
     * @param angle     The amount to rotate the rectangle
     * @param offset    The rectangle offset in world coordinates
     */
    void fill(const Rect rect, const Vec2 origin, const Vec2 scale,
              float angle, const Vec2 offset);
    
    /**
     * Draws the given rectangle filled with the current color and texture.
     *
     * The rectangle will transformed by the given matrix. The transform will 
     * be applied assuming the given origin, which is specified relative to 
     * the origin of the rectangle (not world coordinates).  So to apply the 
     * transform to the center of the rectangle, the origin should be width/2, 
     * height/2 of the rectangle.
     *
     * The texture will fill the entire rectangle with texture coordinate
     * (0,1) at the bottom left corner identified by rect,origin. To draw only
     * part of a texture, use a subtexture to fill the rectangle with the
     * region [minS,maxS]x[min,maxT]. Alternatively, you can use a {@link Poly2}
     * for more fine-tuned control.
     *
     * If depth testing is on, all vertices will use the current sprite
     * batch depth.
     *
     * @param rect      The rectangle to draw
     * @param origin    The rotational offset in the rectangle
     * @param transform The coordinate transform
     */
    void fill(const Rect rect, const Vec2 origin, const Affine2& transform);
    
    /**
     * Draws the given polygon filled with the current color and texture.
     *
     * The polygon tesselation will be determined by the indices in poly. If
     * the polygon has not been triangulated (by one of the triangulation 
     * factories {@link EarclipTriangulator} or {@link DelaunayTriangulator},
     * it may not draw properly.
     *
     * The vertex coordinates will be determined by polygon vertex position.
     * A horizontal position x has texture coordinate x/texture.width. A 
     * vertical coordinate has texture coordinate 1-y/texture.height. As a 
     * result, a rectangular polygon that has the same dimensions as the
     * texture is the same as simply drawing the texture.
     *
     * One way to think of the polygon is as a "cookie cutter".  Treat the 
     * polygon coordinates as pixel coordinates in the texture file, and use
     * that to determine how the texture fills the polygon. This may make the
     * polygon larger than you like in order to get the appropriate texturing.
     * You should use one of the transform methods to fix this.
     *
     * If depth testing is on, all vertices will use the current sprite
     * batch depth.
     *
     * @param poly      The polygon to draw
     */
    void fill(const Poly2& poly);
    
    /**
     * Draws the given polygon filled with the current color and texture.
     *
     * The polygon will be offset by the given position.
     *
     * The polygon tesselation will be determined by the indices in poly. If
     * the polygon has not been triangulated (by one of the triangulation
     * factories {@link EarclipTriangulator} or {@link DelaunayTriangulator},
     * it may not draw properly.
     *
     * The vertex coordinates will be determined by polygon vertex position.
     * A horizontal position x has texture coordinate x/texture.width. A
     * vertical coordinate has texture coordinate 1-y/texture.height. As a
     * result, a rectangular polygon that has the same dimensions as the
     * texture is the same as simply drawing the texture.
     *
     * One way to think of the polygon is as a "cookie cutter".  Treat the
     * polygon coordinates as pixel coordinates in the texture file, and use
     * that to determine how the texture fills the polygon. This may make the
     * polygon larger than you like in order to get the appropriate texturing.
     * You should use one of the transform methods to fix this.
     *
     * If depth testing is on, all vertices will use the current sprite
     * batch depth.
     *
     * @param poly      The polygon to draw
     * @param offset    The polygon offset
     */
    void fill(const Poly2& poly, const Vec2 offset);
    
    /**
     * Draws the given polygon filled with the current color and texture.
     *
     * The polygon will be scaled first, then rotated, and finally offset
     * by the given position. Rotation is measured in radians and is counter
     * clockwise from the x-axis.  Rotation will be about the provided origin,
     * which is specified relative to the origin of the polygon (not world 
     * coordinates). Hence this origin is essentially the pixel coordinate 
     * of the texture (see below) to assign as the rotational center.
     *
     * The polygon tesselation will be determined by the indices in poly. If
     * the polygon has not been triangulated (by one of the triangulation
     * factories {@link EarclipTriangulator} or {@link DelaunayTriangulator},
     * it may not draw properly.
     *
     * The vertex coordinates will be determined by polygon vertex position.
     * A horizontal position x has texture coordinate x/texture.width. A
     * vertical coordinate has texture coordinate 1-y/texture.height. As a
     * result, a rectangular polygon that has the same dimensions as the
     * texture is the same as simply drawing the texture.
     *
     * One way to think of the polygon is as a "cookie cutter".  Treat the
     * polygon coordinates as pixel coordinates in the texture file, and use
     * that to determine how the texture fills the polygon. This may make the
     * polygon larger than you like in order to get the appropriate texturing.
     * You should use one of the transform methods to fix this.
     *
     * If depth testing is on, all vertices will use the current sprite
     * batch depth.
     *
     * @param poly      The polygon to draw
     * @param origin    The image origin in pixel space
     * @param scale     The amount to scale the polygon
     * @param angle     The amount to rotate the polygon
     * @param offset    The polygon offset in world coordinates
     */
    void fill(const Poly2& poly, const Vec2 origin, const Vec2 scale,
              float angle, const Vec2 offset);
    
    /**
     * Draws the given polygon filled with the current color and texture.
     *
     * The polygon will transformed by the given matrix. The transform will
     * be applied assuming the given origin, which is specified relative to
     * the origin of the polygon (not world coordinates). Hence this origin
     * is essentially the pixel coordinate of the texture (see below) to
     * assign as the origin of this transform.
     *
     * The polygon tesselation will be determined by the indices in poly. If
     * the polygon has not been triangulated (by one of the triangulation
     * factories {@link EarclipTriangulator} or {@link DelaunayTriangulator},
     * it may not draw properly.
     *
     * The vertex coordinates will be determined by polygon vertex position.
     * A horizontal position x has texture coordinate x/texture.width. A
     * vertical coordinate has texture coordinate 1-y/texture.height. As a
     * result, a rectangular polygon that has the same dimensions as the
     * texture is the same as simply drawing the texture.
     *
     * One way to think of the polygon is as a "cookie cutter".  Treat the
     * polygon coordinates as pixel coordinates in the texture file, and use
     * that to determine how the texture fills the polygon. This may make the
     * polygon larger than you like in order to get the appropriate texturing.
     * You should use one of the transform methods to fix this.
     *
     * If depth testing is on, all vertices will use the current sprite
     * batch depth.
     *
     * @param poly      The polygon to draw
     * @param origin    The image origin in pixel space
     * @param transform The coordinate transform
     */
    void fill(const Poly2& poly, const Vec2 origin, const Affine2& transform);

#pragma mark -
#pragma mark Outlines
    /**
     * Outlines the given rectangle with the current color and texture.
     *
     * The drawing will be a wireframe of a rectangle.  The wireframe will
     * be textured with Texture coordinate (0,1) at the bottom left corner 
     * identified by rect,origin. The remaining edges will correspond to the
     * edges of the texture. To draw only part of a texture, use a subtexture
     * to outline the edges with [minS,maxS]x[min,maxT]. Alternatively, you
     * can use a {@link Poly2} for more fine-tuned control.
     *
     * If depth testing is on, all vertices will use the current sprite
     * batch depth.
     *
     * @param rect      The rectangle to outline
     */
    void outline(const Rect rect);

    /**
     * Outlines the given rectangle with the current color and texture.
     *
     * The drawing will be a wireframe of a rectangle.  The wireframe will
     * be textured with Texture coordinate (0,1) at the bottom left corner
     * identified by rect,origin. The remaining edges will correspond to the
     * edges of the texture. To draw only part of a texture, use a subtexture
     * to outline the edges with [minS,maxS]x[min,maxT]. Alternatively, you
     * can use a {@link Poly2} for more fine-tuned control.
     *
     * If depth testing is on, all vertices will use the current sprite
     * batch depth.
     *
     * @param rect      The rectangle to outline
     * @param offset    The rectangle offset
     */
    void outline(const Rect rect, const Vec2 offset);

    /**
     * Outlines the given rectangle with the current color and texture.
     *
     * The rectangle will be scaled first, then rotated, and finally offset
     * by the given position. Rotation is measured in radians and is counter
     * clockwise from the x-axis.  Rotation will be about the provided origin, 
     * which is specified relative to the origin of the rectangle (not world 
     * coordinates).  So to spin about the center, the origin should be width/2, 
     * height/2 of the rectangle.
     *
     * The drawing will be a wireframe of a rectangle.  The wireframe will
     * be textured with Texture coordinate (0,1) at the bottom left corner
     * identified by rect,origin. The remaining edges will correspond to the
     * edges of the texture. To draw only part of a texture, use a subtexture
     * to outline the edges with [minS,maxS]x[min,maxT]. Alternatively, you
     * can use a {@link Poly2} for more fine-tuned control.
     *
     * If depth testing is on, all vertices will use the current sprite
     * batch depth.
     *
     * @param rect      The rectangle to outline
     * @param origin    The rotational offset in the rectangle
     * @param scale     The amount to scale the rectangle
     * @param angle     The amount to rotate the rectangle
     * @param offset    The rectangle offset in world coordinates
     */
    void outline(const Rect rect, const Vec2 origin, const Vec2 scale,
                 float angle, const Vec2 offset);
    
    /**
     * Outlines the given rectangle with the current color and texture.
     *
     * The rectangle will transformed by the given matrix. The transform will 
     * be applied assuming the given origin, which is specified relative to 
     * the origin of the rectangle (not world coordinates).  So to apply the 
     * transform to the center of the rectangle, the origin should be width/2, 
     * height/2 of the rectangle.
     *
     * The drawing will be a wireframe of a rectangle.  The wireframe will
     * be textured with Texture coordinate (0,1) at the bottom left corner
     * identified by rect,origin. The remaining edges will correspond to the
     * edges of the texture. To draw only part of a texture, use a subtexture
     * to outline the edges with [minS,maxS]x[min,maxT]. Alternatively, you
     * can use a {@link Poly2} for more fine-tuned control.
     *
     * If depth testing is on, all vertices will use the current sprite
     * batch depth.
     *
     * @param rect      The rectangle to outline
     * @param origin    The rotational offset in the rectangle
     * @param transform The coordinate transform
     */
    void outline(const Rect rect, const Vec2 origin, const Affine2& transform);
    
    /**
     * Outlines the given path with the current color and texture.
     *
     * The drawing will be a wireframe of a path, but the lines are textured.
     * The vertex coordinates will be determined by path vertex position.
     * A horizontal position x has texture coordinate x/texture.width. A
     * vertical coordinate has texture coordinate 1-y/texture.height. As a
     * result, a rectangular polygon that has the same dimensions as the
     * texture is the same as simply outlining the rectangle.
     *
     * One way to think of the path is as a "cookie cutter".  Treat the
     * path coordinates as pixel coordinates in the texture file, and use
     * that to determine how the texture fills the path. This may make the
     * path larger than you like in order to get the appropriate texturing.
     * You should use one of the transform methods to fix this.
     *
     * If depth testing is on, all vertices will use the current sprite
     * batch depth.
     *
     * @param path      The path to outline
     */
    void outline(const Path2& path);
    
    /**
     * Outlines the given path with the current color and texture.
     *
     * The path will be offset by the given position.
     *
     * The drawing will be a wireframe of a path, but the lines are textured.
     * The vertex coordinates will be determined by path vertex position.
     * A horizontal position x has texture coordinate x/texture.width. A
     * vertical coordinate has texture coordinate 1-y/texture.height. As a
     * result, a rectangular polygon that has the same dimensions as the
     * texture is the same as simply outlining the rectangle.
     *
     * One way to think of the path is as a "cookie cutter".  Treat the
     * path coordinates as pixel coordinates in the texture file, and use
     * that to determine how the texture fills the path. This may make the
     * path larger than you like in order to get the appropriate texturing.
     * You should use one of the transform methods to fix this.
     *
     * If depth testing is on, all vertices will use the current sprite
     * batch depth.
     *
     * @param path      The path to outline
     * @param offset    The path offset
     */
    void outline(const Path2& path, const Vec2 offset);
    
    /**
     * Outlines the given path with the current color and texture.
     *
     * The path will be scaled first, then rotated, and finally offset
     * by the given position. Rotation is measured in radians and is counter
     * clockwise from the x-axis.  Rotation will be about the provided origin,
     * which is specified relative to the origin of the path (not world
     * coordinates). Hence this origin is essentially the pixel coordinate 
     * of the texture (see below) to assign as the rotational center.
     *
     * The drawing will be a wireframe of a path, but the lines are textured.
     * The vertex coordinates will be determined by path vertex position.
     * A horizontal position x has texture coordinate x/texture.width. A
     * vertical coordinate has texture coordinate 1-y/texture.height. As a
     * result, a rectangular polygon that has the same dimensions as the
     * texture is the same as simply outlining the rectangle.
     *
     * One way to think of the path is as a "cookie cutter".  Treat the
     * path coordinates as pixel coordinates in the texture file, and use
     * that to determine how the texture fills the path. This may make the
     * path larger than you like in order to get the appropriate texturing.
     * You should use one of the transform methods to fix this.
     *
     * If depth testing is on, all vertices will use the current sprite
     * batch depth.
     *
     * @param path      The path to outline
     * @param origin    The image origin in pixel space
     * @param scale     The amount to scale the path
     * @param angle     The amount to rotate the path
     * @param offset    The path offset in world coordinates
     */
    void outline(const Path2& path, const Vec2 origin, const Vec2 scale, float angle, const Vec2 offset);
    
    /**
     * Outlines the given path with the current color and texture.
     *
     * The path will transformed by the given matrix. The transform will
     * be applied assuming the given origin, which is specified relative to the 
     * origin of the path (not world coordinates). Hence this origin is
     * essentially the pixel coordinate of the texture (see below) to 
     * assign as the origin of this transform.
     *
     * The drawing will be a wireframe of a path, but the lines are textured.
     * The vertex coordinates will be determined by path vertex position.
     * A horizontal position x has texture coordinate x/texture.width. A
     * vertical coordinate has texture coordinate 1-y/texture.height. As a
     * result, a rectangular polygon that has the same dimensions as the
     * texture is the same as simply outlining the rectangle.
     *
     * One way to think of the path is as a "cookie cutter".  Treat the
     * path coordinates as pixel coordinates in the texture file, and use
     * that to determine how the texture fills the path. This may make the
     * path larger than you like in order to get the appropriate texturing.
     * You should use one of the transform methods to fix this.
     *
     * If depth testing is on, all vertices will use the current sprite
     * batch depth.
     *
     * @param path      The path to outline
     * @param origin    The image origin in pixel space
     * @param transform The coordinate transform
     */
    void outline(const Path2& path, const Vec2 origin, const Affine2& transform);
    
// The methods below are to make transition from the intro class easier
#pragma mark -
#pragma mark Convenience Methods
    /**
     * Draws the texture (without tint) at the given position
     *
     * This is a convenience method that calls the appropriate fill method.
     * It sets both the texture and color (removing the previous active values).
     * It then draws a rectangle of the size of the texture, with bottom left
     * corner at the given position.
     *
     * If depth testing is on, all vertices will use the current sprite
     * batch depth.
     *
     * @param texture   The new active texture
     * @param position  The bottom left corner of the texture
     */
    void draw(const std::shared_ptr<Texture>& texture, const Vec2 position);
    
    /**
     * Draws the tinted texture at the given position
     *
     * This is a convenience method that calls the appropriate fill method.
     * It sets both the texture and color (removing the previous active values).  
     * It then draws a rectangle of the size of the texture, with bottom left 
     * corner at the given position.
     *
     * If depth testing is on, all vertices will use the current sprite
     * batch depth.
     *
     * @param texture   The new active texture
     * @param color     The new active color
     * @param position  The bottom left corner of the texture
     */
    void draw(const std::shared_ptr<Texture>& texture, const Color4 color, const Vec2 position);
    
    /**
     * Draws the texture (without tint) inside the given bounds
     *
     * This is a convenience method that calls the appropriate fill method.
     * It sets both the texture and color (removing the previous active values).
     * It then draws the specified rectangle filled with the texture.
     *
     * If depth testing is on, all vertices will use the current sprite
     * batch depth.
     *
     * @param texture   The new active texture
     * @param bounds    The rectangle to texture
     */
    void draw(const std::shared_ptr<Texture>& texture, const Rect bounds);

    /**
     * Draws the tinted texture at the given position
     *
     * This is a convenience method that calls the appropriate fill method.
     * It sets both the texture and color (removing the previous active values).
     * It then draws the specified rectangle filled with the texture.
     *
     * If depth testing is on, all vertices will use the current sprite
     * batch depth.
     *
     * @param texture   The new active texture
     * @param color     The new active color
     * @param bounds    The rectangle to texture
     */
    void draw(const std::shared_ptr<Texture>& texture, const Color4 color, const Rect bounds);

    /**
     * Draws the texture (without tint) transformed by the given parameters
     *
     * This is a convenience method that calls the appropriate fill method.
     * It sets both the texture and color (removing the previous active values).
     * It then draws a texture-sized rectangle centered at the given origin, and
     * transformed by the given parameters.
     *
     * The rectangle will be scaled first, then rotated, and finally offset
     * by the given position. Rotation is measured in radians and is counter
     * clockwise from the x-axis.  Rotation will be about the provided origin,
     * which is specified in texture pixel coordinates (e.g from the bottom
     * left corner).
     *
     * If depth testing is on, all vertices will use the current sprite
     * batch depth.
     *
     * @param texture   The new active texture
     * @param origin    The image origin in pixel space
     * @param scale     The amount to scale the texture
     * @param angle     The amount to rotate the texture
     * @param offset    The texture offset in world coordinates
     */
    void draw(const std::shared_ptr<Texture>& texture, const Vec2 origin,
              const Vec2 scale, float angle, const Vec2 offset);
    
    /**
     * Draws the tinted texture transformed by the given parameters
     *
     * This is a convenience method that calls the appropriate fill method.
     * It sets both the texture and color (removing the previous active values).
     * It then draws a texture-sized rectangle centered at the given origin, and
     * transformed by the given parameters.
     *
     * The rectangle will be scaled first, then rotated, and finally offset
     * by the given position. Rotation is measured in radians and is counter
     * clockwise from the x-axis.  Rotation will be about the provided origin,
     * which is specified in texture pixel coordinates (e.g from the bottom
     * left corner).
     *
     * If depth testing is on, all vertices will use the current sprite
     * batch depth.
     *
     * @param texture   The new active texture
     * @param color     The new active color
     * @param origin    The image origin in pixel space
     * @param scale     The amount to scale the texture
     * @param angle     The amount to rotate the texture
     * @param offset    The texture offset in world coordinates
     */
    void draw(const std::shared_ptr<Texture>& texture, const Color4 color,
              const Vec2 origin, const Vec2 scale, float angle, const Vec2 offset);
    
    /**
     * Draws the texture (without tint) transformed by the given parameters
     *
     * This is a convenience method that calls the appropriate fill method.
     * It sets both the texture and color (removing the previous active values).
     * It fills the specified rectangle with the texture.
     *
     * The rectangle will be scaled first, then rotated, and finally offset
     * by the given position. Rotation is measured in radians and is counter
     * clockwise from the x-axis.  Rotation will be about the provided origin, 
     * which is specified relative to the origin of the rectangle (not world 
     * coordinates).  So to spin about the center, the origin should be width/2, 
     * height/2 of the rectangle.
     *
     * The texture will fill the entire rectangle before being transformed. 
     * Texture coordinate (0,1) will at the bottom left corner identified by 
     * rect,origin. To draw only part of a texture, use a subtexture to fill 
     * the rectangle with the region [minS,maxS]x[min,maxT]. Alternatively,
     * you can use a {@link Poly2} for more fine-tuned control.
     *
     * If depth testing is on, all vertices will use the current sprite
     * batch depth.
     *
     * @param texture   The new active texture
     * @param bounds    The rectangle to texture
     * @param origin    The image origin in pixel space
     * @param scale     The amount to scale the texture
     * @param angle     The amount to rotate the texture
     * @param offset    The rectangle offset in world coordinates
     */
    void draw(const std::shared_ptr<Texture>& texture, const Rect bounds,
              const Vec2 origin, const Vec2 scale, float angle, const Vec2 offset);

    /**
     * Draws the tinted texture transformed by the given parameters
     *
     * This is a convenience method that calls the appropriate fill method.
     * It sets both the texture and color (removing the previous active values).
     * It fills the specified rectangle with the texture.
     *
     * The rectangle will be scaled first, then rotated, and finally offset
     * by the given position. Rotation is measured in radians and is counter
     * clockwise from the x-axis.  Rotation will be about the provided origin, 
     * which is specified relative to the origin of the rectangle (not world 
     * coordinates).  So to spin about the center, the origin should be width/2, 
     * height/2 of the rectangle.
     *
     * The texture will fill the entire rectangle before being transformed. 
     * Texture coordinate (0,1) will at the bottom left corner identified by 
     * rect,origin. To draw only part of a texture, use a subtexture to fill 
     * the rectangle with the region [minS,maxS]x[min,maxT]. Alternatively,
     * you can use a {@link Poly2} for more fine-tuned control.
     *
     * If depth testing is on, all vertices will use the current sprite
     * batch depth.
     *
     * @param texture   The new active texture
     * @param color     The new active color
     * @param bounds    The rectangle to texture
     * @param origin    The image origin in pixel space
     * @param scale     The amount to scale the texture
     * @param angle     The amount to rotate the texture
     * @param offset    The rectangle offset in world coordinates
     */
    void draw(const std::shared_ptr<Texture>& texture, const Color4 color, const Rect bounds,
              const Vec2 origin, const Vec2 scale, float angle, const Vec2 offset);

    /**
     * Draws the texture (without tint) transformed by the matrix
     *
     * This is a convenience method that calls the appropriate fill method.
     * It sets both the texture and color (removing the previous active values).
     * It then draws a texture-sized rectangle centered at the given origin, and
     * transformed by the given matrix.
     *
     * The transform will be applied assuming the given image origin, which is
     * specified in texture pixel coordinates (e.g from the bottom left corner).
     *
     * If depth testing is on, all vertices will use the current sprite
     * batch depth.
     *
     * @param texture   The new active texture
     * @param origin    The image origin in pixel space
     * @param transform The coordinate transform
     */
    void draw(const std::shared_ptr<Texture>& texture, const Vec2 origin,
              const Affine2& transform);
    
    /**
     * Draws the tinted texture transformed by the matrix
     *
     * This is a convenience method that calls the appropriate fill method.
     * It sets both the texture and color (removing the previous active values).
     * It then draws a texture-sized rectangle centered at the given origin, and
     * transformed by the given matrix.
     *
     * The transform will be applied assuming the given image origin, which is
     * specified in texture pixel coordinates (e.g from the bottom left corner).
     *
     * If depth testing is on, all vertices will use the current sprite
     * batch depth.
     *
     * @param texture   The new active texture
     * @param color     The new active color
     * @param origin    The image origin in pixel space
     * @param transform The coordinate transform
     */
    void draw(const std::shared_ptr<Texture>& texture, const Color4 color,
              const Vec2 origin, const Affine2& transform);
    
    /**
     * Draws the texture (without tint) transformed by the matrix
     *
     * This is a convenience method that calls the appropriate fill method.
     * It sets both the texture and color (removing the previous active values).
     * It fills the specified rectangle with the texture, transformed by the 
     * given matrix.
     *
     * The transform will be applied assuming the given image origin, which is
     * specified in texture pixel coordinates (e.g from the bottom left corner).
     *
     * If depth testing is on, all vertices will use the current sprite
     * batch depth.
     *
     * @param texture   The new active texture
     * @param bounds    The rectangle to texture
     * @param origin    The image origin in pixel space
     * @param transform The coordinate transform
     */
    void draw(const std::shared_ptr<Texture>& texture, const Rect bounds,
              const Vec2 origin, const Affine2& transform);

    /**
     * Draws the tinted texture transformed by the matrix
     *
     * This is a convenience method that calls the appropriate fill method.
     * It sets both the texture and color (removing the previous active values).
     * It fills the specified rectangle with the texture, transformed by the 
     * given matrix.
     *
     * The transform will be applied assuming the given image origin, which is
     * specified in texture pixel coordinates (e.g from the bottom left corner).
     *
     * If depth testing is on, all vertices will use the current sprite
     * batch depth.
     *
     * @param texture   The new active texture
     * @param color     The new active color
     * @param bounds    The rectangle to texture
     * @param origin    The image origin in pixel space
     * @param transform The coordinate transform
     */
    void draw(const std::shared_ptr<Texture>& texture, const Color4 color,
              const Rect bounds, const Vec2 origin, const Affine2& transform);
    
    /**
     * Draws the textured polygon (without tint) at the given position
     *
     * This is a convenience method that calls the appropriate fill method.
     * It sets both the texture and color (removing the previous active values).
     * It then draws the polygon, offset by the given value.
     *
     * The polygon tesselation will be determined by the indices in poly. If
     * the polygon has not been triangulated (by one of the triangulation
     * factories {@link EarclipTriangulator} or {@link DelaunayTriangulator},
     * it may not draw properly.
     *
     * The vertex coordinates will be determined by polygon vertex position.
     * A horizontal position x has texture coordinate x/texture.width. A
     * vertical coordinate has texture coordinate 1-y/texture.height. As a
     * result, a rectangular polygon that has the same dimensions as the
     * texture is the same as simply drawing the texture.
     *
     * One way to think of the polygon is as a "cookie cutter".  Treat the
     * polygon coordinates as pixel coordinates in the texture filed, and use
     * that to determine how the texture fills the polygon. This may make the
     * polygon larger than you like in order to get the appropriate texturing.
     * You should use one of the transform methods to fix this.
     *
     * If depth testing is on, all vertices will use the current sprite
     * batch depth.
     *
     * @param texture   The new active texture
     * @param poly      The polygon to texture
     * @param offset    The polygon offset
     */
    void draw(const std::shared_ptr<Texture>& texture, 
              const Poly2& poly, const Vec2 offset);

    /**
     * Draws the tinted, textured polygon at the given position
     *
     * This is a convenience method that calls the appropriate fill method.
     * It sets both the texture and color (removing the previous active values).
     * It then draws the polygon, offset by the given value.
     *
     * The polygon tesselation will be determined by the indices in poly. If
     * the polygon has not been triangulated (by one of the triangulation
     * factories {@link EarclipTriangulator} or {@link DelaunayTriangulator},
     * it may not draw properly.
     *
     * The vertex coordinates will be determined by polygon vertex position.
     * A horizontal position x has texture coordinate x/texture.width. A
     * vertical coordinate has texture coordinate 1-y/texture.height. As a
     * result, a rectangular polygon that has the same dimensions as the
     * texture is the same as simply drawing the texture.
     *
     * One way to think of the polygon is as a "cookie cutter".  Treat the
     * polygon coordinates as pixel coordinates in the texture filed, and use
     * that to determine how the texture fills the polygon. This may make the
     * polygon larger than you like in order to get the appropriate texturing.
     * You should use one of the transform methods to fix this.
     *
     * If depth testing is on, all vertices will use the current sprite
     * batch depth.
     *
     * @param texture   The new active texture
     * @param color     The new active color
     * @param poly      The polygon to texture
     * @param offset    The polygon offset
     */
    void draw(const std::shared_ptr<Texture>& texture, const Color4 color,
              const Poly2& poly, const Vec2 offset);
    
    /**
     * Draws the textured polygon (without tint) transformed by the given parameters
     *
     * This is a convenience method that calls the appropriate fill method.
     * It sets both the texture and color (removing the previous active values).
     * It then draws the polygon, transformed by the given parameters.
     *
     * The polygon will be scaled first, then rotated, and finally offset
     * by the given position. Rotation is measured in radians and is counter
     * clockwise from the x-axis.  Rotation will be about the provided origin,
     * which is specified relative to the origin of the polygon (not world 
     * coordinates). Hence this origin is essentially the pixel coordinate 
     * of the texture (see below) to assign as the rotational center.
     *
     * The polygon tesselation will be determined by the indices in poly. If
     * the polygon has not been triangulated (by one of the triangulation
     * factories {@link EarclipTriangulator} or {@link DelaunayTriangulator},
     * it may not draw properly.
     *
     * The vertex coordinates will be determined by polygon vertex position.
     * A horizontal position x has texture coordinate x/texture.width. A
     * vertical coordinate has texture coordinate 1-y/texture.height. As a
     * result, a rectangular polygon that has the same dimensions as the
     * texture is the same as simply drawing the texture.
     *
     * One way to think of the polygon is as a "cookie cutter".  Treat the
     * polygon coordinates as pixel coordinates in the texture filed, and use
     * that to determine how the texture fills the polygon. This may make the
     * polygon larger than you like in order to get the appropriate texturing.
     * You should use one of the transform methods to fix this.
     *
     * If depth testing is on, all vertices will use the current sprite
     * batch depth.
     *
     * @param texture   The new active texture
     * @param poly      The polygon to texture
     * @param origin    The image origin in pixel space
     * @param scale     The amount to scale the polygon
     * @param angle     The amount to rotate the polygon
     * @param offset    The polygon offset in world coordinates
     */
    void draw(const std::shared_ptr<Texture>& texture, const Poly2& poly,
              const Vec2 origin, const Vec2 scale, float angle, const Vec2 offset);

    /**
     * Draws the tinted, textured polygon transformed by the given parameters
     *
     * This is a convenience method that calls the appropriate fill method.
     * It sets both the texture and color (removing the previous active values).
     * It then draws the polygon, translated by the given parameters.
     *
     * The polygon will be scaled first, then rotated, and finally offset
     * by the given position. Rotation is measured in radians and is counter
     * clockwise from the x-axis.  Rotation will be about the provided origin,
     * which is specified relative to the origin of the polygon (not world 
     * coordinates). Hence this origin is essentially the pixel coordinate 
     * of the texture (see below) to assign as the rotational center.
     *
     * The polygon tesselation will be determined by the indices in poly. If
     * the polygon has not been triangulated (by one of the triangulation
     * factories {@link EarclipTriangulator} or {@link DelaunayTriangulator},
     * it may not draw properly.
     *
     * The vertex coordinates will be determined by polygon vertex position.
     * A horizontal position x has texture coordinate x/texture.width. A
     * vertical coordinate has texture coordinate 1-y/texture.height. As a
     * result, a rectangular polygon that has the same dimensions as the
     * texture is the same as simply drawing the texture.
     *
     * One way to think of the polygon is as a "cookie cutter".  Treat the
     * polygon coordinates as pixel coordinates in the texture filed, and use
     * that to determine how the texture fills the polygon. This may make the
     * polygon larger than you like in order to get the appropriate texturing.
     * You should use one of the transform methods to fix this.
     *
     * If depth testing is on, all vertices will use the current sprite
     * batch depth.
     *
     * @param texture   The new active texture
     * @param color     The new active color
     * @param poly      The polygon to texture
     * @param origin    The image origin in pixel space
     * @param scale     The amount to scale the polygon
     * @param angle     The amount to rotate the polygon
     * @param offset    The polygon offset in world coordinates
     */
    void draw(const std::shared_ptr<Texture>& texture, const Color4 color, const Poly2& poly,
              const Vec2 origin, const Vec2 scale, float angle, const Vec2 offset);

    /**
     * Draws the textured polygon (without tint) transformed by the given matrix
     *
     * This is a convenience method that calls the appropriate fill method.
     * It sets both the texture and color (removing the previous active values).
     * It then draws the polygon, translated by the given matrix.
     *
     * The polygon will transformed by the given matrix. The transform will
     * be applied assuming the given origin, which is specified relative to the 
     * origin of the polygon (not world coordinates). Hence this origin is 
     * essentially the pixel coordinate of the texture (see below) to 
     * assign as the origin of this transform.
     *
     * The polygon tesselation will be determined by the indices in poly. If
     * the polygon has not been triangulated (by one of the triangulation
     * factories {@link EarclipTriangulator} or {@link DelaunayTriangulator},
     * it may not draw properly.
     *
     * The vertex coordinates will be determined by polygon vertex position.
     * A horizontal position x has texture coordinate x/texture.width. A
     * vertical coordinate has texture coordinate 1-y/texture.height. As a
     * result, a rectangular polygon that has the same dimensions as the
     * texture is the same as simply drawing the texture.
     *
     * One way to think of the polygon is as a "cookie cutter".  Treat the
     * polygon coordinates as pixel coordinates in the texture filed, and use
     * that to determine how the texture fills the polygon. This may make the
     * polygon larger than you like in order to get the appropriate texturing.
     * You should use one of the transform methods to fix this.
     *
     * If depth testing is on, all vertices will use the current sprite
     * batch depth.
     *
     * @param texture   The new active texture
     * @param poly      The polygon to texture
     * @param origin    The image origin in pixel space
     * @param transform The coordinate transform
     */
    void draw(const std::shared_ptr<Texture>& texture, const Poly2& poly,
              const Vec2 origin, const Affine2& transform);
    
    /**
     * Draws the tinted, textured polygon transformed by the given matrix
     *
     * This is a convenience method that calls the appropriate fill method.
     * It sets both the texture and color (removing the previous active values).
     * It then draws the polygon, translated by the given matrix.
     *
     * The polygon will transformed by the given matrix. The transform will
     * be applied assuming the given origin, which is specified relative to the 
     * origin of the polygon (not world coordinates). Hence this origin is 
     * essentially the pixel coordinate of the texture (see below) to 
     * assign as the origin of this transform.
     *
     * The polygon tesselation will be determined by the indices in poly. If
     * the polygon has not been triangulated (by one of the triangulation
     * factories {@link EarclipTriangulator} or {@link DelaunayTriangulator},
     * it may not draw properly.
     *
     * The vertex coordinates will be determined by polygon vertex position.
     * A horizontal position x has texture coordinate x/texture.width. A
     * vertical coordinate has texture coordinate 1-y/texture.height. As a
     * result, a rectangular polygon that has the same dimensions as the
     * texture is the same as simply drawing the texture.
     *
     * One way to think of the polygon is as a "cookie cutter".  Treat the
     * polygon coordinates as pixel coordinates in the texture filed, and use
     * that to determine how the texture fills the polygon. This may make the
     * polygon larger than you like in order to get the appropriate texturing.
     * You should use one of the transform methods to fix this.
     *
     * If depth testing is on, all vertices will use the current sprite
     * batch depth.
     *
     * @param texture   The new active texture
     * @param color     The new active color
     * @param poly      The polygon to texture
     * @param origin    The image origin in pixel space
     * @param transform The coordinate transform
     */
    void draw(const std::shared_ptr<Texture>& texture, const Color4 color,
              const Poly2& poly, const Vec2 origin, const Affine2& transform);

#pragma mark -
#pragma mark Direct Mesh Drawing
    /**
     * Draws the given mesh with the current texture and/or gradient.
     *
     * This method provides more fine tuned control over texture coordinates
     * that the other fill/outline methods.  The texture no longer needs to
     * be drawn uniformly over the shape. The offset will be applied to the
     * vertex positions directly in world space. If depth testing is on, all
     * vertices will be the current depth.
     *
     * The drawing command will be determined by the mesh, and the triangulation
     * or lines determined by the mesh indices. The mesh vertices use their own
     * color values.  However, if tint is true, these values will be tinted
     * (i.e. multiplied) by the current active color.
     *
     * If depth testing is on, all vertices will use the current sprite
     * batch depth.
     *
     * @param mesh      The sprite mesh
     * @param position  The coordinate offset for the mesh
     * @param tint      Whether to tint with the active color
     */
    void drawMesh(const Mesh<SpriteVertex2>& mesh, const Vec2 position, bool tint = true);

    /**
     * Draws the given mesh with the current texture and/or gradient.
     *
     * This method provides more fine tuned control over texture coordinates
     * that the other fill/outline methods.  The texture no longer needs to be
     * drawn uniformly over the shape. The transform will be applied to the
     * vertex positions directly in world space. If depth testing is on, all
     * vertices will be the current depth.
     *
     * The drawing command will be determined by the mesh, and the triangulation
     * or lines determined by the mesh indices. The mesh vertices use their own
     * color values.  However, if tint is true, these values will be tinted
     * (i.e. multiplied) by the current active color.
     *
     * If depth testing is on, all vertices will use the current sprite
     * batch depth.
     *
     * @param mesh      The sprite mesh
     * @param transform The coordinate transform
     * @param tint      Whether to tint with the active color
     */
    void drawMesh(const Mesh<SpriteVertex2>& mesh, const Affine2& transform, bool tint = true);

    /**
     * Draws the vertices in a triangle fan with the current texture and/or gradient.
     *
     * This method provides more fine tuned control over texture coordinates
     * that the other fill/outline methods.  The texture no longer needs to
     * be drawn uniformly over the shape. The offset will be applied to the
     * vertex positions directly in world space. If depth testing is on, all
     * vertices will be the current depth.
     *
     * The drawing command will be GL_TRIANGLES, and the triangulation will
     * be a mesh anchored on the first element. This method is ideal for
     * convex polygons.
     *
     * The mesh vertices use their own color values. However, if tint is true,
     * these values will be tinted (i.e. multiplied) by the current active
     * color. If depth testing is on, all vertices will use the current sprite
     * batch depth.
     *
     * @param vertices  The vertices to draw as a triangle fan
     * @param position  The coordinate offset for the mesh
     * @param tint      Whether to tint with the active color
     *
     * @return the number of vertices added to the drawing buffer.
     */
    void drawMesh(const std::vector<SpriteVertex2>& vertices, const Vec2 position, bool tint = true) {
        drawMesh(vertices.data(),vertices.size(),position,tint);
    }
    
    /**
     * Draws the vertices in a triangle fan with the current texture and/or gradient.
     *
     * This method provides more fine tuned control over texture coordinates
     * that the other fill/outline methods.  The texture no longer needs to be
     * drawn uniformly over the shape. The transform will be applied to the
     * vertex positions directly in world space. If depth testing is on, all
     * vertices will be the current depth.
     *
     * The drawing command will be GL_TRIANGLES, and the triangulation will
     * be a mesh anchored on the first element. This method is ideal for
     * convex polygons.
     *
     * The mesh vertices use their own color values. However, if tint is true,
     * these values will be tinted (i.e. multiplied) by the current active
     * color. If depth testing is on, all vertices will use the current sprite
     * batch depth.
     *
     * @param vertices  The vertices to draw as a triangle fan
     * @param transform The transform to apply to the vertices
     * @param tint      Whether to tint with the active color
     *
     * @return the number of vertices added to the drawing buffer.
     */
    void drawMesh(const std::vector<SpriteVertex2>& vertices, const Affine2& transform, bool tint = true) {
        drawMesh(vertices.data(),vertices.size(),transform,tint);
    }

    /**
     * Draws the vertices in a triangle fan with the current texture and/or gradient.
     *
     * This method provides more fine tuned control over texture coordinates
     * that the other fill/outline methods.  The texture no longer needs to
     * be drawn uniformly over the shape. The offset will be applied to the
     * vertex positions directly in world space. If depth testing is on, all
     * vertices will be the current depth.
     *
     * The drawing command will be GL_TRIANGLES, and the triangulation will
     * be a mesh anchored on the first element. This method is ideal for
     * convex polygons.
     *
     * The mesh vertices use their own color values. However, if tint is true,
     * these values will be tinted (i.e. multiplied) by the current active
     * color. If depth testing is on, all vertices will use the current sprite
     * batch depth.
     *
     * @param vertices  The vertices to draw as a triangle fan
     * @param size      The number of vertices in the fan
     * @param position  The coordinate offset for the mesh
     * @param tint      Whether to tint with the active color
     *
     * @return the number of vertices added to the drawing buffer.
     */
    void drawMesh(const SpriteVertex2* vertices, size_t size, const Vec2 position, bool tint = true);
    
    /**
     * Draws the vertices in a triangle fan with the current texture and/or gradient.
     *
     * This method provides more fine tuned control over texture coordinates
     * that the other fill/outline methods.  The texture no longer needs to be
     * drawn uniformly over the shape. The transform will be applied to the
     * vertex positions directly in world space. If depth testing is on, all
     * vertices will be the current depth.
     *
     * The drawing command will be GL_TRIANGLES, and the triangulation will
     * be a mesh anchored on the first element. This method is ideal for
     * convex polygons.
     *
     * The mesh vertices use their own color values. However, if tint is true,
     * these values will be tinted (i.e. multiplied) by the current active
     * color. If depth testing is on, all vertices will use the current sprite
     * batch depth.
     *
     * @param vertices  The vertices to draw as a triangle fan
     * @param size      The number of vertices in the fan
     * @param transform The transform to apply to the vertices
     * @param tint      Whether to tint with the active color
     *
     * @return the number of vertices added to the drawing buffer.
     */
    void drawMesh(const SpriteVertex2* vertices, size_t size, const Affine2& transform, bool tint = true);
    
#pragma mark -
#pragma mark Text Drawing
    /**
     * Draws the text with the specified font at the given position.
     *
     * The position specifies the location of the left edge of the baseline of
     * the rendered text. The text will be displayed on only one line. For more
     * fine tuned control of text, you should use a {@link TextLayout}.
     *
     * By default, all text is rendered with white letters. However, this can
     * be tinted by the current sprite batch color to produce any color letters
     * required.
     *
     * If depth testing is on, the font glyphs will use the current sprite
     * batch depth.
     *
     * @param text      The text to display
     * @param font      The font to render the text
     * @param position  The left edge of the text baseline
     */
    void drawText(const std::string text, const std::shared_ptr<Font>& font, const Vec2 position);
    
    /**
     * Draws the text with the specified font and transform
     *
     * The offset is measured from the left edge of the font baseline to identify
     * the origin of the rendered text. This origin is used when applying the
     * transform to the rendered text.
     *
     * By default, all text is rendered with white letters. However, this can
     * be tinted by the current sprite batch color to produce any color letters
     * required.
     *
     * If depth testing is on, the font glyphs will use the current sprite
     * batch depth.
     *
     * @param text      The text to display
     * @param font      The font to render the text
     * @param origin    The rotational origin relative to the baseline
     * @param transform The coordinate transform
     */
    void drawText(const std::string text, const std::shared_ptr<Font>& font, const Vec2 origin, const Affine2& transform);

    /**
     * Draws the text layout at the specified position
     *
     * The position specifies the location of the text layout origin. See the
     * specification of {@link TextLayout} for more information.
     *
     * By default, all text is rendered with white letters. However, this can
     * be tinted by the current sprite batch color to produce any color letters
     * required.
     *
     * If depth testing is on, the font glyphs will use the current sprite
     * batch depth.
     *
     * @param text      The text layout for the text to display
     * @param position  The left edge of the text baseline
     */
    void drawText(const std::shared_ptr<TextLayout>& text, const Vec2 position);

    /**
     * Draws the text layout with the given coordinate transform
     *
     * The transform is applied to the coordinate space of the {@link TextLayout}.
     *
     * By default, all text is rendered with white letters. However, this can
     * be tinted by the current sprite batch color to produce any color letters
     * required.
     *
     * If depth testing is on, the font glyphs will use the current sprite
     * batch depth.
     *
     * @param text      The text layout for the text to display
     * @param transform The coordinate transform
     */
    void drawText(const std::shared_ptr<TextLayout>& text, const Affine2& transform);

#pragma mark -
#pragma mark Internal Helpers
private:
    /**
     * Sets the current drawing command.
     *
     * The value must be one of GL_TRIANGLES or GL_LINES.
     *
     * @param command   The new drawing command
     */
    void setCommand(GLenum command);
    
    /**
     * Returns the current drawing command.
     *
     * The value must be one of GL_TRIANGLES or GL_LINES.
     *
     * @return the current drawing command.
     */
    GLenum getCommand() const;

    /**
     * Records the current drawing context, freezing it.
     *
     * This method must be called whenever we need to update a context that 
     * is currently in-flight. It ensures that the vertices and uniform blocks 
     * batched so far will use the correct set of uniforms.
     */
    void record();
    
    /**
     * Deletes the recorded uniforms.
     *
     * This method is called upon flushing or cleanup.
     */
    void unwind();
    
    /**
     * Sets the active uniform block to agree with the gradient and stroke.
     *
     * This method is called upon vertex preparation.
     *
     * @param context   The current uniform context
     */
    void setUniformBlock(Context* context);
    
    /**
     * Updates the shader with the current blur offsets
     *
     * Blur offsets depend upon the texture size. This method converts the
     * blur step into an offset in texture coordinates. It supports
     * non-square textures.
     *
     * If there is no active texture, the blur offset will be 0.
     *
     * @param texture   The texture to blur
     * @param step      The blur step in pixels
     */
    void blurTexture(const std::shared_ptr<Texture>& texture, GLfloat step);

    /**
     * Clears the stencil buffer specified
     *
     * @param buffer    The stencil buffer (lower, upper, both)
     */
    void clearStencilBuffer(GLenum buffer);

    /**
     * Configures the OpenGL settings to apply the given effect.
     *
     * @param effect    The stencil effect
     */
    void applyEffect(StencilEffect effect);
    
    /**
     * Returns the number of vertices added to the drawing buffer.
     *
     * This method adds the given rectangle to the vertex buffer, but does not
     * draw it yet.  You must call {@link #flush} or {@link #end} to draw the 
     * rectangle. This method will automatically flush if the maximum number
     * of vertices is reached.
     *
     * If depth testing is on, all vertices will use the current sprite
     * batch depth.
     *
     * @param rect  The rectangle to add to the buffer
     *
     * @return the number of vertices added to the drawing buffer.
     */
    unsigned int prepare(const Rect rect);
    
    /**
     * Returns the number of vertices added to the drawing buffer.
     *
     * This method adds the given rectangle to the vertex buffer, but does not
     * draw it yet.  You must call {@link #flush} or {@link #end} to draw the 
     * rectangle. This method will automatically flush if the maximum number
     * of vertices is reached.
     *
     * All vertices will be uniformly transformed by the transform matrix.
     * If depth testing is on, all vertices will use the current sprite
     * batch depth.
     *
     * @param rect  The rectangle to add to the buffer
     * @param mat   The transform to apply to the vertices
     *
     * @return the number of vertices added to the drawing buffer.
     */
    unsigned int prepare(const Rect rect, const Affine2& mat);

    /**
     * Returns the number of vertices added to the drawing buffer.
     *
     * This method adds the given polygon to the vertex buffer, but does not
     * draw it yet.  You must call {@link #flush} or {@link #end} to draw the 
     * polygon. This method will automatically flush if the maximum number
     * of vertices is reached.
     *
     * If depth testing is on, all vertices will use the current sprite
     * batch depth.
     *
     * @param poly  The polygon to add to the buffer
     *
     * @return the number of vertices added to the drawing buffer.
     */
    unsigned int prepare(const Poly2& poly);
    
    /**
     * Returns the number of vertices added to the drawing buffer.
     *
     * This method adds the given polygon to the vertex buffer, but does not
     * draw it yet.  You must call {@link #flush} or {@link #end} to draw the 
     * polygon. This method will automatically flush if the maximum number
     * of vertices is reached.
     *
     * All vertices will be uniformly offset by the given vector. If depth
     * testing is on, all vertices will use the current sprite batch depth.
     *
     * @param poly  The polygon to add to the buffer
     * @param off  	The offset to apply to the vertices
     *
     * @return the number of vertices added to the drawing buffer.
     */
    unsigned int prepare(const Poly2& poly, const Vec2 off);

    /**
     * Returns the number of vertices added to the drawing buffer.
     *
     * This method adds the given polygon to the vertex buffer, but does not
     * draw it yet.  You must call {@link #flush} or {@link #end} to draw the 
     * polygon. This method will automatically flush if the maximum number
     * of vertices is reached.
     *
     * All vertices will be uniformly transformed by the transform matrix.
     * If depth testing is on, all vertices will use the current sprite
     * batch depth.
     *
     * @param poly  The polygon to add to the buffer
     * @param mat   The transform to apply to the vertices
     *
     * @return the number of vertices added to the drawing buffer.
     */
    unsigned int prepare(const Poly2& poly, const Affine2& mat);
    
    /**
     * Returns the number of vertices added to the drawing buffer.
     *
     * This method is an alternate version of {@link #prepare} for the same
     * arguments.  It runs slower (e.g. the compiler cannot easily optimize
     * the loops) but it is guaranteed to work on any size polygon.  This
     * is important for avoiding memory corruption.
     *
     * All vertices will be uniformly transformed by the transform matrix.
     * If depth testing is on, all vertices will use the current sprite
     * batch depth.
     *
     * @param poly  The polygon to add to the buffer
     * @param mat   The transform to apply to the vertices
     *
     * @return the number of vertices added to the drawing buffer.
     */
    unsigned int chunkify(const Poly2& poly, const Affine2& mat);

    /**
     * Returns the number of vertices added to the drawing buffer.
     *
     * This method adds the given mesh (both vertices and indices) to the
     * vertex buffer, but does not draw it.  You must call {@link #flush}
     * or {@link #end} to draw the complete mesh. This method will automatically
     * flush if the maximum number of vertices (or uniform blocks) is reached.
     *
     * If depth testing is on, all vertices will use the current sprite
     * batch depth.
     *
     * @param mesh  The mesh to add to the buffer
     * @param mat   The transform to apply to the vertices
     * @param tint  Whether to tint with the active color
     *
     * @return the number of vertices added to the drawing buffer.
     */
    unsigned int prepare(const Mesh<SpriteVertex2>& mesh, const Affine2& mat, bool tint = true);

    /**
     * Returns the number of vertices added to the drawing buffer.
     *
     * This method is an alternate version of {@link #prepare} for the same
     * arguments. It runs slower (e.g. the compiler cannot easily optimize
     * the loops) but it is guaranteed to work on any size mesh. This is
     * important for avoiding memory corruption.
     *
     * If depth testing is on, all vertices will use the current sprite
     * batch depth.
     *
     * @param mesh  The mesh to add to the buffer
     * @param mat   The transform to apply to the vertices
     * @param tint  Whether to tint with the active color
     *
     * @return the number of vertices added to the drawing buffer.
     */
    unsigned int chunkify(const Mesh<SpriteVertex2>& mesh, const Affine2& mat, bool tint = true);

    /**
     * Returns the number of vertices added to the drawing buffer.
     *
     * This method adds the given vertices to the vertex buffer. In addition,
     * this method adds the requisite indices to the index buffer to draw
     * these vertices as a triangle fan (anchored on the first element).
     * This method is ideal for meshes on convex polygons.
     *
     * With that said, this method does not actually draw the triangle fan.
     * You must call {@link #flush} or {@link #end} to draw the vertices.
     * This method will automatically flush if the maximum number of vertices
     * (or uniform blocks) is reached.
     *
     * If depth testing is on, all vertices will use the current sprite
     * batch depth.
     *
     * @param vertices  The vertices to draw as a triangle fan
     * @param size      The number of vertices in the fan
     * @param mat       The transform to apply to the vertices
     * @param tint      Whether to tint with the active color
     *
     * @return the number of vertices added to the drawing buffer.
     */
    unsigned int prepare(const SpriteVertex2* vertices, size_t size, const Affine2& mat, bool tint = true);

    /**
     * Returns the number of vertices added to the drawing buffer.
     *
     * This method is an alternate version of {@link #prepare} for the same
     * arguments. It runs slower (e.g. the compiler cannot easily optimize
     * the loops) but it is guaranteed to work on any number of vertices.
     * This is important for avoiding memory corruption.
     *
     * If depth testing is on, all vertices will use the current sprite
     * batch depth.
     *
     * @param vertices  The vertices to draw as a triangle fan
     * @param size      The number of vertices in the fan
     * @param mat       The transform to apply to the vertices
     * @param tint      Whether to tint with the active color
     *
     * @return the number of vertices added to the drawing buffer.
     */
    unsigned int chunkify(const SpriteVertex2* vertices, size_t size, const Affine2& mat, bool tint = true);
    
};

}

#endif /* __CU_SPRITE_BATCH_H__ */
