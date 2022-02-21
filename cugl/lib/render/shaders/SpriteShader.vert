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

// Positions
in vec4 aPosition;
out vec2 outPosition;

// Colors
in  vec4 aColor;
out vec4 outColor;

// Texture coordinates
in  vec2 aTexCoord;
out vec2 outTexCoord;

// Gradient coordinates
in  vec2 aGradCoord;
out vec2 outGradCoord;

// Matrices
uniform mat4 uPerspective;

// Depth value (this is a 2d pipeline)
uniform float uDepth;

// Transform and pass through                                                   
void main(void) {
    gl_Position = uPerspective*vec4(aPosition.xy,0,1);
    outPosition = aPosition.xy; // Need untransformed for scissor
    outColor = aColor;
    outTexCoord = aTexCoord;
    outGradCoord = aGradCoord;
}

/////////// SHADER END //////////)"


