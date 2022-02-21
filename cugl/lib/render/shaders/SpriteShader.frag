R"(////////// SHADER BEGIN /////////
//  SpriteShader.vert
//  Cornell University Game Library (CUGL)
//
//  This is a full-power SpriteBatch vertex shader for both OpenGL and OpenGL ES.
//  It supports textures which can be tinted per vertex. It also supports gradients
//  (which can be used simulataneously with textures, but not with colors), as
//  well as a scissor mask.  Gradients use the color inputs as their texture
//  coordinates. Finally, there is support for very simple blur effects, which
//  are used for font labels.
//
//  This shader was inspired by nanovg by Mikko Mononen (memon@inside.org).
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
//  Version: 3/20/20
#ifdef CUGLES
// This one line is all the difference
precision highp float;  // highp required for gradient precision
#endif

// Bit vector for texturing, gradients, and scissoring
uniform int  uType;
// Blur offset for simple kernel blur
uniform vec2 uBlur;

// The texture for sampling
uniform sampler2D uTexture;

// The output color
out vec4 frag_color;

// The inputs from the vertex shader
in vec2 outPosition;
in vec4 outColor;
in vec2 outTexCoord;
in vec2 outGradCoord;

// The stroke+gradient uniform block
layout (std140) uniform uContext
{
    mat3 scMatrix;      // 48
    vec2 scExtent;      //  8
    vec2 scScale;       //  8
    mat3 gdMatrix;      // 48
    vec4 gdInner;       // 16
    vec4 gdOuter;       // 16
    vec2 gdExtent;      //  8
    float gdRadius;     //  4
    float gdFeathr;     //  4
};

/**
 * Returns an interpolation value for a box gradient
 *
 * The value returned is the mix parameter for the inner and
 * outer color.
 *
 * Adapted from nanovg by Mikko Mononen (memon@inside.org)
 *
 * pt:      The (transformed) point to test
 * ext:     The gradient extent
 * radius:  The gradient radius
 * feather: The gradient feather amount
 */
float boxgradient(vec2 pt, vec2 ext, float radius, float feather) {
    vec2 ext2 = ext - vec2(radius,radius);
    vec2 dst = abs(pt) - ext2;
    float m = min(max(dst.x,dst.y),0.0) + length(max(dst,0.0)) - radius;
    return clamp((m + feather*0.5) / feather, 0.0, 1.0);
}

/**
 * Returns an alpha value for scissoring
 *
 * A pixel with value 0 is dropped, while one with value 1 is kept.
 * The scale value sets the 0 to 1 transition (which should be quick).
 *
 * Adapted from nanovg by Mikko Mononen (memon@inside.org)
 *
 * pt:  The point to test
 */
float scissormask(vec2 pt) {
    vec2 sc = (abs((scMatrix * vec3(pt,1.0)).xy) - scExtent);
    sc = vec2(0.5,0.5) - sc * scScale;
    return clamp(sc.x,0.0,1.0) * clamp(sc.y,0.0,1.0);
}

/**
 * Returns the result of a simple kernel blur
 *
 * This function blurs the texture with a separable 256
 * element kernel. It is not meant to provide a full-feature
 * Gaussian blur. It is a high performance blur for simple
 * effects (like font blurring).
 *
 * coord: The texture coordinate to blur
 */
vec4 blursample(vec2 coord) {
    // Separable gaussian
    float factor[5] = float[]( 1.0,  4.0, 6.0, 4.0, 1.0 );
    // Sample steps
    float steps[5]  = float[]( -1.0, -0.5, 0.0, 0.5, 1.0 );

    // Sample from the texture and average
    vec4 result = vec4(0.0);
    for(int ii = 0; ii < 5; ii++) {
        vec4 row = vec4(0.0);
        for(int jj = 0; jj < 5; jj++) {
            vec2 offs = vec2(uBlur.x*steps[ii],uBlur.y*steps[jj]);
            row += texture(uTexture, coord + offs)*factor[jj];
        }
        result += row*factor[ii];
    }

    result /= vec4(256);
    return result;
}

/**
 * Performs the main fragment shading.
 */
void main(void) {
    vec4 result;
    float fType = float(uType);

    if (mod(fType, 4.0) >= 2.0) {
        // Apply a gradient color
        mat3  cmatrix = gdMatrix;
        vec2  cextent = gdExtent;
        float cfeathr = gdFeathr;
        vec2 pt = (cmatrix * vec3(outGradCoord,1.0)).xy;
        float d = boxgradient(pt,cextent,gdRadius,cfeathr);
        result = mix(gdInner,gdOuter,d)*outColor;
    } else {
        // Use a solid color
        result = outColor;
    }
    
    if (mod(fType, 2.0) == 1.0) {
        // Include texture (tinted by color and/or gradient)
        if (uType >= 8) {
            result *= blursample(outTexCoord);
        } else {
            result *= texture(uTexture, outTexCoord);
        }
    }
    
    if (mod(fType, 8.0) >= 4.0) {
        // Apply scissor mask
        result.w *= scissormask(outPosition);
    }

    frag_color = result;
}
/////////// SHADER END //////////)"

