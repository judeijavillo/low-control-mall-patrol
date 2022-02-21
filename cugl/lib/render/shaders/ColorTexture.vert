R"(////////// SHADER BEGIN /////////
//  ColorTexture.vert
//  Cornell University Game Library (CUGL)
//
//  This is a primitive SpriteBatch vertex shader for both OpenGL and OpenGL ES.
//  It supports textures which can be tinted per vertex. However, it does not
//  support gradients or scissors.  It is for testing purposes only.
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
//  Version: 2/10/20

// Positions
in vec4 aPosition;

// Colors
in  vec4 aColor;
out vec4 outColor;

// Texture coordinates
in  vec2 aTexCoord;
out vec2 outTexCoord;

// Matrices
uniform mat4 uPerspective;

// Transform and pass through
void main(void) {
    gl_Position = uPerspective*aPosition;
    outColor = aColor;
    outTexCoord = aTexCoord;
}

/////////// SHADER END //////////)"


