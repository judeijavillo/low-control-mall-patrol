//
//  CUShader.h
//  Cornell University Game Library (CUGL)
//
//  This module provides a general purpose shader class for GLSL shaders.
//  It supports compilations and has diagnostic tools for errors. The
//  shader is general enough that it should not need to be subclassed.
//  However, to use a shader, it must be attached to a VertexBuffer.
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
//  Version: 2/10/20

#ifndef __CU_SHADER_H__
#define __CU_SHADER_H__

#include <string>
#include <vector>
#include <unordered_map>
#include <cugl/base/CUBase.h>
#include <cugl/math/cu_math.h>
#include <cugl/render/CUTexture.h>
#include <cugl/render/CUUniformBuffer.h>

// We use raw string literals for shaders, but we need to prefix by system.
#if CU_GL_PLATFORM == CU_GL_OPENGLES
    #define SHADER(A) ("#version 300 es\n#define CUGLES 1\n"+A)
#else
    #define SHADER(A) ("#version 330\n"+A)
#endif


#pragma mark -
namespace cugl {

/**
 * This class defines a GLSL shader.
 *
 * This class compiles and links any well-defined GLSL shader.  It also has methods
 * for querying and binding the shader. The class is written to be agnostic about 
 * whether we are using OpenGL or OpenGLES.
 *
 * However, a shader cannot be used by itself.  To use a shader, it must first be 
 * attached to a {@link VertexBuffer}. When using a shader, with a vertex buffer 
 * keep in mind the "performance hit" hierarchy. From our experiments, the cost of 
 * changing data in a rendering phase going from most expensive to cheapest is as 
 * follows:
 *
 *  <ul>
 *    <li>Render target</li>
 *    <li>Shader</li>
 *    <li>Sampler</li>
 *    <li>Vertex Buffer</li>
 *    <li>Vertex Stream</li>
 *    <li>Texture</li>
 *    <li>Uniform</li>
 *    <li>Draw Call</li>
 *  </ul>
 *
 * By vertex stream we mean the act of loading data into a vertex buffer. Since the
 * cost of swapping a vertex buffer is more expensive that reloading data into it,
 * a vertex buffer should only be swapped when the format of the vertex data changes.
 *
 * Uniforms tend to be fairly cheap.  However, some uniforms are samplers and those
 * are incredibly expensive to change.  A sampler is a uniform that is bound to a
 * texture bind point.  Because of how textures are loaded, it is (much) cheaper to
 * activate a texture to an existing bind point than it is to change the bind point used
 * in the shader.
 *
 * Even the most basic uniforms are by no means cheap. The best case graphics
 * performance is when you can load the vertex buffer once and then call a single 
 * draw command for all of the vertices (the difference is an order of magnitude).  
 * Therefore, any properties that space only a few vertices (e.g. quads) should be 
 * pushed into the vertex data itself.  For example, it is faster to transform quads 
 * in the CPU than it is in the GPU.
 *
 * Because of the limitations of OpenGLES, this class only supports vertex and 
 * fragment shaders -- it does not support tesselation or geometry shaders.  
 * Furthermore, keep in mind that Apple has deprecated OpenGL. MacOS devices are 
 * stuck at OpenGL 4.1 and iOS devices are stuck at OpenGLES 3.0. So it is not 
 * safe to use any shader more recent than version 140 on desktop/laptop and 
 * version 300 on mobile.
 *
 * Another side effect of OpenGLES is that this class does not support explicit 
 * binding of multiple output locations (glBindFragDataLocation). If a shader has 
 * multiple output targets, then these must be explicitly managed inside the shader 
 * with the layout keyword.  Otherwise, the output bind points from the appropriate
 * query methods.
 */
class Shader {
#pragma mark Values
protected:
    /** The OpenGL program for this shader */
    GLuint _program;
    /** The OpenGL vertex shader for this shader */
    GLuint _vertShader;
    /** The OpenGL fragment shader for this shader */
    GLuint _fragShader;
    /** The source string for the vertex shader */
    std::string _vertSource;
    /** The source string for the fragment shader */
    std::string _fragSource;
    /** The attribute locations of this shader */
    std::unordered_map<std::string, GLenum> _attribtypes;
    /** The attribute variable names of this shader */
    std::unordered_map<GLint, std::string>  _attribnames;
    /** The attribute locations of this shader */
    std::unordered_map<std::string, GLint>  _attribsizes;
    /** The uniform locations of this shader */
    std::unordered_map<std::string, GLenum> _uniformtypes;
    /** The uniform variable names for this shader (includes samplers) */
    std::unordered_map<GLint,std::string>   _uniformnames;
    /** The uniform locations of this shader (includes samplers) */
    std::unordered_map<std::string, GLint>  _uniformsizes;
    /** The uniform block variable names for this shader */
    std::unordered_map<GLint,std::string>   _uniblocknames;
    /** The uniform block locations of this shader */
    std::unordered_map<std::string, GLint>  _uniblocksizes;
    /** Mappings of uniforms to a uniform block */
    std::unordered_map<GLint, GLint>        _uniblockfields;

    
#pragma mark -
#pragma mark Compilation
private:
    /**
     * Compiles this shader from the given vertex and fragment shader sources.
     *
     * When compilation is complete, the shader will not be bound.  However,
     * any shader that was actively bound during compilation also be unbound 
     * as well.
     *
     * If compilation fails, it will display error messages on the log.
     *
     * @return true if compilation was successful.
     */
    virtual bool compile();
    
    /**
     * Returns true if the shader was compiled properly.
     *
     * If compilation fails, it will display error messages on the log.
     *
     * @param shader    The shader to test
     * @param type      The shader type (vertex or fragment)
     *
     * @return true if the shader was compiled properly.
     */
    static bool validateShader(GLuint shader, const std::string type);
    
    /**
     * Displays the shader compilation errors to the log.
     *
     * If there were no errors, this method will do nothing.
     *
     * @param shader    The shader to test
     */
    static void logShaderError(GLuint shader);

    /**
     * Displays the program linker errors to the log.
     *
     * If there were no errors, this method will do nothing.
     *
     * @param shader    The program to test
     */
    static void logProgramError(GLuint shader);
    
    /**
     * Querys all of the shader attributes and caches them for fast look-ups
     */
    void cacheAttributes();
    
    /**
     * Querys all of the shader uniforms and caches them for fast look-ups
     *
     * This includes uniform buffer blocks as well.
     */
    void cacheUniforms();
    
    
#pragma mark -
#pragma mark Constructors
public:
    /**
     * Creates an uninitialized shader with no source.
     *
     * You must initialize the shader to add a source and compile it.
     */
    Shader() :  _program(0), _vertShader(0), _fragShader(0) {};

    /**
     * Deletes this shader, disposing all resources.
     */
    ~Shader() { dispose(); }

    /**
     * Deletes the OpenGL shader and resets all attributes.
     *
     * You must reinitialize the shader to use it.
     */
    virtual void dispose();
    
    /**
     * Initializes this shader with the given vertex and fragment source.
     *
     * The shader will compile the vertex and fragment sources and link
     * them together. When compilation is complete, the shader will be
     * bound and active.
     *
     * @param vsource   The source string for the vertex shader.
     * @param fsource   The source string for the fragment shader.
     *
     * @return true if initialization was successful.
     */
    bool init(const std::string vsource, std::string fsource);

    /**
     * Returns a new shader with the given vertex and fragment source.
     *
     * The shader will compile the vertex and fragment sources and link
     * them together. When compilation is complete, the shader will be
     * bound and active.
     *
     * @param vsource   The source string for the vertex shader.
     * @param fsource   The source string for the fragment shader.
     *
     * @return a new shader with the given vertex and fragment source.
     */
    static std::shared_ptr<Shader> alloc(const std::string vsource, std::string fsource) {
        std::shared_ptr<Shader> result = std::make_shared<Shader>();
        return (result->init(vsource, fsource) ? result : nullptr);
    }


#pragma mark -
#pragma mark Binding
    /**
     * Binds this shader, making it active.
     *
     * Once bound, any OpenGL calls will then be sent to this shader. This
     * call is reentrant, and may safely be called on an active shader.
     */
    void bind();
    
    /**
     * Unbinds this shader, making it no longer active.
     *
     * Once unbound, OpenGL calls will no longer be sent to this shader.
     */
    void unbind();
    
    /**
     * Returns true if this shader has been compiled and is ready for use.
     *
     * @return true if this shader has been compiled and is ready for use.
     */
    bool isReady() const { return _program != 0; }

    /**
     * Returns true if this shader is currently bound.
     *
     * Any OpenGL calls will be sent to this shader only if it is bound.
     *
     * @return true if this shader is currently bound.
     */
    bool isBound() const;

    
#pragma mark -
#pragma mark Source
    /**
     * Returns the source string for the vertex shader.
     *
     * The string is empty if not defined.
     *
     * @return the source string for the vertex shader.
     */
    const std::string getVertSource() const {
        return _vertSource;
    }
    
    /**
     * Returns the source string for the fragment shader.
     *
     * The string is empty if not defined.
     *
     * @return the source string for the fragment shader.
     */
    const std::string getFragSource() const {
        return _fragSource;
    }
    
    /**
     * Returns the OpenGL program associated with this shader.
     *
     * This method will return 0 if the program is not initialized.
     *
     * @return the OpenGL program associated with this shader.
     */
    GLuint getProgram() const { return _program; }


#pragma mark -
#pragma mark Attribute Properties
    /**
     * Returns a vector of all attribute variables in this shader
     *
     * @return a vector of all attribute variables in this shader
     */
    std::vector<std::string> getAttributes() const;
    
    /**
     * Returns the program offset of the given attribute
     *
     * If name is not a valid attribute, this method returns -1.
     *
     * @param name  The attribute variable name
     *
     * @return the program offset of the given attribute
     */
    GLint getAttributeLocation(const std::string name) const;
    
    /**
     * Returns the size (in bytes) of the given attribute
     *
     * If name is not a valid attribute, this method returns -1.
     *
     * @param name  The attribute variable name
     *
     * @return the size (in bytes) of the given attribute
     */
    GLint getAttributeSize(const std::string name) const;

    /**
     * Returns the type of the given attribute
     *
     * If name is not a valid attribute, this method returns GL_FALSE.
     *
     * @param name  The attribute variable name
     *
     * @return the type of the given attribute
     */
    GLenum getAttributeType(const std::string name) const;

    /**
     * Returns the program offset of the given output variable.
     *
     * An output variable is a variable in a fragment shader that writes to
     * a texture.  All shaders have at least one output variable.  However,
     * shaders can have more than one output variable, particularly when
     * used in conjunction with a {@link RenderTarget}.  This method is
     * helpful for getting the number and names of these variables for
     * setting up a RenderTarget.
     *
     * To explicit set the program offset of an output variable, the shader
     * should use the layout keyword in GLSL. Because of compatibility
     * issues with Apple products (iOS, MacOS), it is not possible to get
     * much information about output variables other than their location.
     *
     * If name is not a valid output variable, this method returns -1.
     *
     * @param name  The output variable name
     *
     * @return the program offset of the given output variable.
     */
    GLint getOutputLocation(const std::string name) const;

    
#pragma mark -
#pragma mark Uniform Properties
    /**
     * Returns a vector of all uniform variables in this shader
     *
     * @return a vector of all uniform variables in this shader
     */
    std::vector<std::string> getUniforms() const;
    
    /**
     * Returns the program offset of the given uniform
     *
     * If name is not a valid uniform, this method returns -1.
     *
     * @param name  The uniform variable name
     *
     * @return the program offset of the given uniform
     */
    GLint getUniformLocation(const std::string name) const;

    /**
     * Returns the size (in bytes) of the given uniform
     *
     * If name is not a valid uniform, this method returns -1.
     *
     * @param name  The uniform variable name
     *
     * @return the size (in bytes) of the given uniform
     */
    GLint getUniformSize(const std::string name) const;

    /**
     * Returns the type of the given uniform
     *
     * If name is not a valid uniform, this method returns GL_FALSE.
     *
     * @param name  The uniform variable name
     *
     * @return the type of the given uniform
     */
    GLenum getUniformType(const std::string name) const;

    
#pragma mark -
#pragma mark Sampler Properties
    /**
     * Returns a vector of all samplers used by this shader
     *
     * A sampler is a variable attached to a texture.  All samplers are also
     * uniforms.  Therefore this vector is a subset of the names returned
     * by {@link getUniforms}.
     *
     * @return a vector of all samplers used by this shader
     */
    std::vector<std::string> getSamplers() const;
    
    /**
     * Returns the program offset of the given sampler variable
     *
     * A sampler is a variable attached to a texture.  All samplers are also
     * uniforms.  Therefore this method has the same effect as the method
     * {@link getUniformLocation}.
     *
     * If name is not a valid sampler, this method returns -1.
     *
     * @param name  The sampler variable name
     *
     * @return the program offset of the given sampler variable
     */
    GLint getSamplerLocation(const std::string name) const;

    /**
     * Sets the given sampler variable to a texture bindpoint.
     *
     * A sampler is a variable attached to a texture.  All samplers are also
     * uniforms.  Therefore this method has the same effect as the method
     * {@link setUniform1ui}.
     *
     * This method will only succeed if the shader is actively bound.
     *
     * Note that changing this value is causes a significant performance
     * hit to a shader.  Whenever possible, it is better to keep the bindpoint
     * the same while binding a different texture to the same point.
     *
     * @param pos      The location of the sampler in the shader
     * @param bpoint   The bindpoint for the sampler
     */
    void setSampler(GLint pos, GLuint bpoint);
    
    /**
     * Sets the given sampler variable to a texture bindpoint.
     *
     * A sampler is a variable attached to a texture.  All samplers are also
     * uniforms.  Therefore this method has the same effect as the method
     * {@link setUniform1ui}.
     *
     * This method will only succeed if the shader is actively bound.
     *
     * Note that changing this value is causes a significant performance
     * hit to a shader.  Whenever possible, it is better to keep the bindpoint
     * the same while binding a different texture to the same point.
     *
     * @param name      The name of the sampler variable
     * @param bpoint   The bindpoint for the sampler
     */
    void setSampler(const std::string name, GLuint bpoint);

    /**
     * Sets the given sampler variable to the bindpoint of the given texture.
     *
     * A sampler is a variable attached to a texture.  All samplers are also
     * uniforms.  Therefore this method has the same effect as the method
     * {@link setUniform1ui}.
     *
     * This method will only succeed if the shader is actively bound.
     *
     * @param pos      The location of the sampler in the shader
     * @param texture   The texture to initialize the bindpoint
     */
    void setSampler(GLint pos, const std::shared_ptr<Texture>& texture);

    /**
     * Sets the given sampler variable to the bindpoint of the given texture.
     *
     * A sampler is a variable attached to a texture.  All samplers are also
     * uniforms.  Therefore this method has the same effect as the method
     * {@link setUniform1ui}.
     *
     * This method will bind the sampler to the current bindpoint of the
     * texture.  The shader will not be aware if the texture changes its
     * bindpoint in the future.
     *
     * This method will only succeed if the shader is actively bound.
     *
     * @param name      The name of the sampler variable
     * @param texture   The texture to initialize the bindpoint
     */
    void setSampler(const std::string name, const std::shared_ptr<Texture>& texture);
    
    /**
     * Returns the texture bindpoint associated with the given sampler variable.
     *
     * The shader does not track the actual texture associated with this bindpoint,
     * only the bindpoint itself.  It is up to the software developer to keep track of
     * what texture is currently at that bindpoint.
     *
     * @param pos      The location of the sampler in the shader
     *
     * @return the texture bindpoint associated with the given sampler variable.
     */
    GLuint getSampler(GLint pos) const;

    /**
     * Returns the texture bindpoint associated with the given sampler variable.
     *
     * The shader does not track the actual texture associated with this bindpoint,
     * only the bindpoint itself.  It is up to the software developer to keep track of
     * what texture is currently at that bindpoint.
     *
     * @param name      The name of the sampler variable
     *
     * @return the texture bindpoint associated with the given sampler variable.
     */
    GLuint getSampler(const std::string name) const;
    
    
#pragma mark -
#pragma mark Uniform Blocks
    /**
     * Returns a vector of all uniform blocks used by this shader
     *
     * A uniform block is a variable attached to a uniform buffer.  It is not the
     * same as a normal uniform and cannot be treated as such.  In this case
     * the uniform values are set in the {@link UniformBuffer} object and
     * not the shader.
     *
     * @return a vector of all uniform blocks used by this shader
     */
    std::vector<std::string> getUniformBlocks() const;

    /**
     * Returns a vector of all uniforms for the given block.
     *
     * A uniform block is a variable attached to a uniform buffer.  It is not the
     * same as a normal uniform and cannot be treated as such.  In this case
     * the uniform values are set in the {@link UniformBuffer} object and
     * not the shader.
     *
     * This method allows us to verify that at {@link UniformBuffer} object
     * properly matches this shader.
     *
     * @param pos   The location of the uniform block in the shader
     *
     * @return a vector of all uniform blocks used by this shader
     */
    std::vector<std::string> getUniformsForBlock(GLint pos) const;

    /**
     * Returns a vector of all uniforms for the given block.
     *
     * A uniform block is a variable attached to a uniform buffer.  It is not the
     * same as a normal uniform and cannot be treated as such.  In this case
     * the uniform values are set in the {@link UniformBuffer} object and
     * not the shader.
     *
     * This method allows us to verify that at {@link UniformBuffer} object
     * properly matches this shader.
     *
     * @param name      The name of the uniform block in the shader
     *
     * @return a vector of all uniform blocks used by this shader
     */
    std::vector<std::string> getUniformsForBlock(std::string name) const;

    /**
     * Sets the given uniform block variable to a uniform buffer bindpoint.
     *
     * A uniform block is a variable attached to a uniform buffer.  It is not the
     * same as a normal uniform and cannot be treated as such.  In this case
     * the uniform values are set in the {@link UniformBuffer} object and
     * not the shader.
     *
     * This method will only succeed if the shader is actively bound.
     *
     * @param pos      The location of the uniform block in the shader
     * @param bpoint   The bindpoint for the uniform block
     */
    void setUniformBlock(GLint pos, GLuint bpoint);

    /**
     * Sets the given uniform block variable to a uniform buffer bindpoint.
     *
     * A uniform block is a variable attached to a uniform buffer.  It is not the
     * same as a normal uniform and cannot be treated as such.  In this case
     * the uniform values are set in the {@link UniformBuffer} object and
     * not the shader.
     *
     * This method will only succeed if the shader is actively bound.
     *
     * @param name      The name of the uniform block in the shader
     * @param bpoint   The bindpoint for the uniform block
     */
    void setUniformBlock(const std::string name, GLuint bpoint);

    /**
     * Sets the given uniform block variable to the bindpoint of the given uniform buffer.
     *
     * A uniform block is a variable attached to a uniform buffer.  It is not the
     * same as a normal uniform and cannot be treated as such.  In this case
     * the uniform values are set in the {@link UniformBuffer} object and
     * not the shader.
     *
     * This method will bind the uniform buffer to the current bindpoint of the
     * block object.  The shader will not be aware if the buffer object changes
     * its bindpoint in the future. However, it will verify whether the buffer 
     * object has uniform variables matching this shader.
     *
     * This method will only succeed if the shader is actively bound.
     *
     * @param pos       The location of the uniform block in the shader
     * @param buffer    The buffer to bind to this uniform block
     */
    void setUniformBlock(GLint pos, const std::shared_ptr<UniformBuffer>& buffer);

    /**
     * Sets the given uniform block variable to the bindpoint of the given uniform buffer.
     *
     * A uniform block is a variable attached to a uniform buffer.  It is not the
     * same as a normal uniform and cannot be treated as such.  In this case
     * the uniform values are set in the {@link UniformBuffer} object and
     * not the shader.
     *
     * This method will bind the uniform buffer to the current bindpoint of the
     * block object.  The shader will not be aware if the buffer object changes
     * its bindpoint in the future. However, it will verify whether the buffer 
     * object has uniform variables matching this shader.
     *
     * This method will only succeed if the shader is actively bound.
     *
     * @param name      The name of the uniform block in the shader
     * @param buffer    The buffer to bind to this uniform block
     */
    void setUniformBlock(const std::string name,
                         const std::shared_ptr<UniformBuffer>& buffer);

    /**
     * Returns the buffer bindpoint associated with the given uniform block.
     *
     * The shader does not track the actual uniform buffer associated with this
     * bindpoint, only the bindpoint itself.  It is up to the software developer to
     * keep track of what uniform buffer is currently at that bindpoint.
     *
     * @param pos      The location of the uniform block in the shader
     *
     * @return the buffer bindpoint associated with the given uniform block.
     */
    GLuint getUniformBlock(GLint pos) const;

    /**
     * Returns the buffer bindpoint associated with the given uniform block.
     *
     * The shader does not track the actual uniform buffer associated with this
     * bindpoint, only the bindpoint itself.  It is up to the software developer to
     * keep track of what uniform buffer is currently at that bindpoint.
     *
     * @param name      The name of the uniform block in the shader
     *
     * @return the buffer bindpoint associated with the given uniform block.
     */
    GLuint getUniformBlock(const std::string name) const;

    
#pragma mark -
#pragma mark CUGL Uniforms
    /**
     * Sets the given uniform to a vector value.
     *
     * This method will only succeed if the shader is actively bound.
     *
     * @param pos   The location of the uniform in the shader
     * @param vec   The value for the uniform
     */
    void setUniformVec2(GLint pos, const Vec2 vec);
    
    /**
     * Sets the given uniform to a vector value.
     *
     * This method will only succeed if the shader is actively bound.
     *
     * @param name  The name of the uniform
     * @param vec   The value for the uniform
     */
    void setUniformVec2(const std::string name, const Vec2 vec);

    /**
     * Returns true if it can access the given uniform as a vector.
     *
     * This method will only succeed if the shader is actively bound. It assumes
     * that the shader variable is a vec2 (or larger).
     *
     * @param pos   The location of the uniform in the shader
     * @param vec   The vector to store the result
     *
     * @return true if it can access the given uniform as a vector.
     */
    bool getUniformVec2(GLint pos, Vec2& vec) const;

    /**
     * Returns true if it can access the given uniform as a vector.
     *
     * This method will only succeed if the shader is actively bound. It assumes
     * that the shader variable is a vec2 (or larger).
     *
     * @param name  The name of the uniform
     * @param vec   The vector to store the result
     *
     * @return true if it can access the given uniform as a vector.
     */
    bool getUniformVec2(const std::string name, Vec2& vec) const;
    
    /**
     * Sets the given uniform to a vector value.
     *
     * This method will only succeed if the shader is actively bound.
     *
     * @param pos   The location of the uniform in the shader
     * @param vec   The value for the uniform
     */
    void setUniformVec3(GLint pos, const Vec3 vec);

    /**
     * Sets the given uniform to a vector value.
     *
     * This method will only succeed if the shader is actively bound.
     *
     * @param name  The name of the uniform
     * @param vec   The value for the uniform
     */
    void setUniformVec3(const std::string name, const Vec3 vec);

    /**
     * Returns true if it can access the given uniform as a vector.
     *
     * This method will only succeed if the shader is actively bound. It assumes
     * that the shader variable is a vec3 (or larger).
     *
     * @param pos   The location of the uniform in the shader
     * @param vec   The vector to store the result
     *
     * @return true if it can access the given uniform as a vector.
     */
    bool getUniformVec3(GLint pos, Vec3& vec) const;

    /**
     * Returns true if it can access the given uniform as a vector.
     *
     * This method will only succeed if the shader is actively bound. It assumes
     * that the shader variable is a vec3 (or larger).
     *
     * @param name  The name of the uniform
     * @param vec   The vector to store the result
     *
     * @return true if it can access the given uniform as a vector.
     */
    bool getUniformVec3(const std::string name, Vec3& vec) const;

    /**
     * Sets the given uniform to a vector value.
     *
     * This method will only succeed if the shader is actively bound.
     *
     * @param pos   The location of the uniform in the shader
     * @param vec   The value for the uniform
     */
    void setUniformVec4(GLint pos, const Vec4 vec);

    /**
     * Sets the given uniform to a vector value.
     *
     * This method will only succeed if the shader is actively bound.
     *
     * @param name  The name of the uniform
     * @param vec   The value for the uniform
     */
    void setUniformVec4(const std::string name, const Vec4 vec);

    /**
     * Returns true if it can access the given uniform as a vector.
     *
     * This method will only succeed if the shader is actively bound. It assumes
     * that the shader variable is a vec4 (or larger).
     *
     * @param pos   The location of the uniform in the shader
     * @param vec   The vector to store the result
     *
     * @return true if it can access the given uniform as a vector.
     */
    bool getUniformVec4(GLint pos, Vec4& vec) const;

    /**
     * Returns true if it can access the given uniform as a vector.
     *
     * This method will only succeed if the shader is actively bound. It assumes
     * that the shader variable is a vec4 (or larger).
     *
     * @param name  The name of the uniform
     * @param vec   The vector to store the result
     *
     * @return true if it can access the given uniform as a vector.
     */
    bool getUniformVec4(const std::string name, Vec4& vec) const;

    /**
     * Sets the given uniform to a color value.
     *
     * This method will only succeed if the shader is actively bound.
     *
     * @param pos   The location of the uniform in the shader
     * @param color The value for the uniform
     */
    void setUniformColor4(GLint pos, const Color4 color);

    /**
     * Sets the given uniform to a color value.
     *
     * This method will only succeed if the shader is actively bound.
     *
     * @param name  The name of the uniform
     * @param color The value for the uniform
     */
    void setUniformColor4(const std::string name, const Color4 color);

    /**
     * Returns true if it can access the given uniform as a color.
     *
     * This method will only succeed if the shader is actively bound. It assumes
     * that the shader variable is a vec4 (or larger).
     *
     * @param pos   The location of the uniform in the shader
     * @param color The object to store the result
     *
     * @return true if it can access the given uniform as a color.
     */
    bool getUniformColor4(GLint pos, Color4& color) const;

    /**
     * Returns true if it can access the given uniform as a color.
     *
     * This method will only succeed if the shader is actively bound. It assumes
     * that the shader variable is a vec4 (or larger).
     *
     * @param name  The name of the uniform
     * @param color The object to store the result
     *
     * @return true if it can access the given uniform as a color.
     */
    bool getUniformColor4(const std::string name, Color4& color) const;

    /**
     * Sets the given uniform to a color value.
     *
     * This method will only succeed if the shader is actively bound.
     *
     * @param pos   The location of the uniform in the shader
     * @param color The value for the uniform
     */
    void setUniformColor4f(GLint pos, const Color4f color);

    /**
     * Sets the given uniform to a color value.
     *
     * This method will only succeed if the shader is actively bound.
     *
     * @param name  The name of the uniform
     * @param color The value for the uniform
     */
    void setUniformColor4f(const std::string name, const Color4f color);

    /**
     * Returns true if it can access the given uniform as a color.
     *
     * This method will only succeed if the shader is actively bound. It assumes
     * that the shader variable is a vec4 (or larger).
     *
     * @param pos   The location of the uniform in the shader
     * @param color The object to store the result
     *
     * @return true if it can access the given uniform as a color.
     */
    bool getUniformColor4f(GLint pos, Color4f& color) const;

    /**
     * Returns true if it can access the given uniform as a color.
     *
     * This method will only succeed if the shader is actively bound. It assumes
     * that the shader variable is a vec4 (or larger).
     *
     * @param name  The name of the uniform
     * @param color The object to store the result
     *
     * @return true if it can access the given uniform as a color.
     */
    bool getUniformColor4f(const std::string name, Color4f& color) const;

    /**
     * Sets the given uniform to a matrix value.
     *
     * This method will only succeed if the shader is actively bound.
     *
     * @param pos   The location of the uniform in the shader
     * @param mat   The value for the uniform
     */
    void setUniformMat4(GLint pos, const Mat4& mat);
    
    /**
     * Sets the given uniform to a matrix value
     *
     * This method will only succeed if the shader is actively bound.
     * It will silently fail (with no error) if name is does not refer
     * to  a valid uniform.
     *
     * @param name  The name of the uniform
     * @param mat   The value for the uniform
     */
    void setUniformMat4(const std::string name, const Mat4& mat);

    /**
     * Returns true if it can access the given uniform as a matrix.
     *
     * This method will only succeed if the shader is actively bound. It assumes
     * that the shader variable is a mat4 (or larger).
     *
     * @param pos   The location of the uniform in the shader
     * @param mat   The object to store the result
     *
     * @return true if it can access the given uniform as a matrix.
     */
    bool getUniformMat4(GLint pos, Mat4& mat) const;

    /**
     * Returns true if it can access the given uniform as a matrix.
     *
     * This method will only succeed if the shader is actively bound. It assumes
     * that the shader variable is a mat4 (or larger).
     *
     * @param name  The name of the uniform
     * @param mat   The object to store the result
     *
     * @return true if it can access the given uniform as a matrix.
     */
    bool getUniformMat4(const std::string name, Mat4& mat) const;

    /**
     * Sets the given uniform to an affine transform.
     *
     * Affine transforms are passed to a shader as a 3x3 matrix on
     * homogenous coordinates.
     *
     * This method will only succeed if the shader is actively bound.
     *
     * @param pos   The location of the uniform in the shader
     * @param mat   The value for the uniform
     */
    void setUniformAffine2(GLint pos, const Affine2& mat);

    /**
     * Sets the given uniform to an affine transform.
     *
     * Affine transforms are passed to a shader as a 3x3 matrix on
     * homogenous coordinates.
     *
     * This method will only succeed if the shader is actively bound.
     *
     * @param name  The name of the uniform
     * @param mat   The value for the uniform
     */
    void setUniformAffine2(const std::string name, const Affine2& mat);

    /**
     * Returns true if it can access the given uniform as an affine transform.
     *
     * Affine transforms are read from a shader as a 3x3 matrix on homogenous 
     * coordinates. This method will only succeed if the shader is actively bound. 
     * It assumes that the shader variable is a mat3 (or larger).
     *
     * @param pos   The location of the uniform in the shader
     * @param mat   The object to store the result
     *
     * @return true if it can access the given uniform as an affine transform.
     */
    bool getUniformAffine2(GLint pos, Affine2& mat) const;

    /**
     * Returns true if it can access the given uniform as an affine transform.
     *
     * Affine transforms are read from a shader as a 3x3 matrix on homogenous 
     * coordinates. This method will only succeed if the shader is actively bound. 
     * It assumes that the shader variable is a mat3 (or larger).
     *
     * @param name  The name of the uniform
     * @param mat   The object to store the result
     *
     * @return true if it can access the given uniform as an affine transform.
     */
    bool getUniformAffine2(const std::string name, Affine2& mat) const;

    /**
     * Sets the given uniform to a quaternion.
     *
     * This method will only succeed if the shader is actively bound.
     *
     * @param pos   The location of the uniform in the shader
     * @param quat   The value for the uniform
     */
    void setUniformQuaternion(GLint pos, const Quaternion& quat);

    /**
     * Sets the given uniform to a quaternion.
     *
     * This method will only succeed if the shader is actively bound.
     *
     * @param name  The name of the uniform
     * @param quat  The value for the uniform
     */
    void setUniformQuaternion(const std::string name, const Quaternion& quat);

    /**
     * Returns true if it can access the given uniform as a quaternion.
     *
     * This method will only succeed if the shader is actively bound. It assumes
     * that the shader variable is a vec4 (or larger).
     *
     * @param pos   The location of the uniform in the shader
     * @param quat  The quaternion to store the result
     *
     * @return true if it can access the given uniform as a quaternion.
     */
    bool getUniformQuaternion(GLint pos, Quaternion& quat) const;

    /**
     * Returns true if it can access the given uniform as a quaternion.
     *
     * This method will only succeed if the shader is actively bound. It assumes
     * that the shader variable is a vec4 (or larger).
     *
     * @param name  The name of the uniform
     * @param quat  The quaternion to store the result
     *
     * @return true if it can access the given uniform as a quaternion.
     */
    bool getUniformQuaternion(const std::string name, Quaternion& quat) const;


#pragma mark -
#pragma mark Legacy Uniforms
    /**
     * Sets the given uniform to a single float value.
     *
     * This method will only succeed if the shader is actively bound.
     *
     * @param pos   The location of the uniform in the shader
     * @param v0    The value for the uniform
     */
    void setUniform1f(GLint pos, GLfloat v0);

    /**
     * Sets the given uniform to a single float value.
     *
     * This method will only succeed if the shader is actively bound.
     * It will silently fail (with no error) if name is does not refer
     * to  a valid uniform.
     *
     * @param name  The name of the uniform
     * @param v0    The value for the uniform
     */
    void setUniform1f(const std::string name, GLfloat v0);

    /**
     * Sets the given uniform to a pair of float values.
     *
     * This method will only succeed if the shader is actively bound.
     *
     * @param pos   The location of the uniform in the shader
     * @param v0    The first value for the uniform
     * @param v1    The second value for the uniform
     */
    void setUniform2f(GLint pos, GLfloat v0, GLfloat v1);
    
    /**
     * Sets the given uniform to a pair of float values.
     *
     * This method will only succeed if the shader is actively bound.
     * It will silently fail (with no error) if name is does not refer
     * to  a valid uniform.
     *
     * @param name  The name of the uniform
     * @param v0    The first value for the uniform
     * @param v1    The second value for the uniform
     */
    void setUniform2f(const std::string name, GLfloat v0, GLfloat v1);

    /**
     * Sets the given uniform to a trio of float values.
     *
     * This method will only succeed if the shader is actively bound.
     *
     * @param pos   The location of the uniform in the shader
     * @param v0    The first value for the uniform
     * @param v1    The second value for the uniform
     * @param v2    The third value for the uniform
     */
    void setUniform3f(GLint pos, GLfloat v0, GLfloat v1, GLfloat v2);

    /**
     * Sets the given uniform to a trio of float values.
     *
     * This method will only succeed if the shader is actively bound.
     * It will silently fail (with no error) if name is does not refer
     * to  a valid uniform.
     *
     * @param name  The name of the uniform
     * @param v0    The first value for the uniform
     * @param v1    The second value for the uniform
     * @param v2    The third value for the uniform
     */
    void setUniform3f(const std::string name, GLfloat v0, GLfloat v1, GLfloat v2);

    /**
     * Sets the given uniform to a quartet of float values.
     *
     * This method will only succeed if the shader is actively bound.
     *
     * @param pos   The location of the uniform in the shader
     * @param v0    The first value for the uniform
     * @param v1    The second value for the uniform
     * @param v2    The third value for the uniform
     * @param v3    The fourth value for the uniform
     */
    void setUniform4f(GLint pos, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3);

    /**
     * Sets the given uniform to a quartet of float values.
     *
     * This method will only succeed if the shader is actively bound.
     * It will silently fail (with no error) if name is does not refer
     * to  a valid uniform.
     *
     * @param name  The name of the uniform
     * @param v0    The first value for the uniform
     * @param v1    The second value for the uniform
     * @param v2    The third value for the uniform
     * @param v3    The fourth value for the uniform
     */
    void setUniform4f(const std::string name, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3);

    /**
     * Sets the given uniform to a single int value.
     *
     * This method will only succeed if the shader is actively bound.
     *
     * @param pos   The location of the uniform in the shader
     * @param v0    The value for the uniform
     */
    void setUniform1i(GLint pos, GLint v0);

    /**
     * Sets the given uniform to a single int value.
     *
     * This method will only succeed if the shader is actively bound.
     * It will silently fail (with no error) if name is does not refer
     * to  a valid uniform.
     *
     * @param name  The name of the uniform
     * @param v0    The value for the uniform
     */
    void setUniform1i(const std::string name, GLint v0);

    /**
     * Sets the given uniform to a pair of int values.
     *
     * This method will only succeed if the shader is actively bound.
     *
     * @param pos   The location of the uniform in the shader
     * @param v0    The first value for the uniform
     * @param v1    The second value for the uniform
     */
    void setUniform2i(GLint pos, GLint v0, GLint v1);

    /**
     * Sets the given uniform to a pair of int values.
     *
     * This method will only succeed if the shader is actively bound.
     * It will silently fail (with no error) if name is does not refer
     * to  a valid uniform.
     *
     * @param name  The name of the uniform
     * @param v0    The first value for the uniform
     * @param v1    The second value for the uniform
     */
    void setUniform2i(const std::string name, GLint v0, GLint v1);

    /**
     * Sets the given uniform to a trio of int values.
     *
     * This method will only succeed if the shader is actively bound.
     *
     * @param pos   The location of the uniform in the shader
     * @param v0    The first value for the uniform
     * @param v1    The second value for the uniform
     * @param v2    The third value for the uniform
     */
    void setUniform3i(GLint pos, GLint v0, GLint v1, GLint v2);
    
    /**
     * Sets the given uniform to a trio of int values.
     *
     * This method will only succeed if the shader is actively bound.
     * It will silently fail (with no error) if name is does not refer
     * to  a valid uniform.
     *
     * @param name  The name of the uniform
     * @param v0    The first value for the uniform
     * @param v1    The second value for the uniform
     * @param v2    The third value for the uniform
     */
    void setUniform3i(const std::string name, GLint v0, GLint v1, GLint v2);

    /**
     * Sets the given uniform to a quartet of int values.
     *
     * This method will only succeed if the shader is actively bound.
     *
     * @param pos   The location of the uniform in the shader
     * @param v0    The first value for the uniform
     * @param v1    The second value for the uniform
     * @param v2    The third value for the uniform
     * @param v3    The fourth value for the uniform
     */
    void setUniform4i(GLint pos, GLint v0, GLint v1, GLint v2, GLint v3);
    
    /**
     * Sets the given uniform to a quartet of int values.
     *
     * This method will only succeed if the shader is actively bound.
     * It will silently fail (with no error) if name is does not refer
     * to  a valid uniform.
     *
     * @param name  The name of the uniform
     * @param v0    The first value for the uniform
     * @param v1    The second value for the uniform
     * @param v2    The third value for the uniform
     * @param v3    The fourth value for the uniform
     */
    void setUniform4i(const std::string name, GLint v0, GLint v1, GLint v2, GLint v3);

    /**
     * Sets the given uniform to a single unsigned value.
     *
     * This method will only succeed if the shader is actively bound.
     *
     * @param pos   The location of the uniform in the shader
     * @param v0    The value for the uniform
     */
    void setUniform1ui(GLint pos, GLuint v0);

    /**
     * Sets the given uniform to a single unsigned value.
     *
     * This method will only succeed if the shader is actively bound.
     * It will silently fail (with no error) if name is does not refer
     * to  a valid uniform.
     *
     * @param name  The name of the uniform
     * @param v0    The value for the uniform
     */
    void setUniform1ui(const std::string name, GLuint v0);

    /**
     * Sets the given uniform to a pair of unsigned values.
     *
     * This method will only succeed if the shader is actively bound.
     *
     * @param pos   The location of the uniform in the shader
     * @param v0    The first value for the uniform
     * @param v1    The second value for the uniform
     */
    void setUniform2ui(GLint pos, GLuint v0, GLuint v1);
    
    /**
     * Sets the given uniform to a pair of unsigned values.
     *
     * This method will only succeed if the shader is actively bound.
     * It will silently fail (with no error) if name is does not refer
     * to  a valid uniform.
     *
     * @param name  The name of the uniform
     * @param v0    The first value for the uniform
     * @param v1    The second value for the uniform
     */
    void setUniform2ui(const std::string name, GLuint v0, GLuint v1);

    /**
     * Sets the given uniform to a trio of unsigned values.
     *
     * This method will only succeed if the shader is actively bound.
     *
     * @param pos   The location of the uniform in the shader
     * @param v0    The first value for the uniform
     * @param v1    The second value for the uniform
     * @param v2    The third value for the uniform
     */
    void setUniform3ui(GLint pos, GLuint v0, GLuint v1, GLuint v2);

    /**
     * Sets the given uniform to a trio of unsigned values.
     *
     * This method will only succeed if the shader is actively bound.
     * It will silently fail (with no error) if name is does not refer
     * to  a valid uniform.
     *
     * @param name  The name of the uniform
     * @param v0    The first value for the uniform
     * @param v1    The second value for the uniform
     * @param v2    The third value for the uniform
     */
    void setUniform3ui(const std::string name, GLuint v0, GLuint v1, GLuint v2);

    /**
     * Sets the given uniform to a quartet of unsigned values.
     *
     * This method will only succeed if the shader is actively bound.
     *
     * @param pos   The location of the uniform in the shader
     * @param v0    The first value for the uniform
     * @param v1    The second value for the uniform
     * @param v2    The third value for the uniform
     * @param v3    The fourth value for the uniform
     */
    void setUniform4ui(GLint pos, GLuint v0, GLuint v1, GLuint v2, GLuint v3);

    /**
     * Sets the given uniform to a quartet of unsigned values.
     *
     * This method will only succeed if the shader is actively bound.
     * It will silently fail (with no error) if name is does not refer
     * to  a valid uniform.
     *
     * @param name  The name of the uniform
     * @param v0    The first value for the uniform
     * @param v1    The second value for the uniform
     * @param v2    The third value for the uniform
     * @param v3    The fourth value for the uniform
     */
    void setUniform4ui(const std::string name, GLuint v0, GLuint v1, GLuint v2, GLuint v3);

    /**
     * Sets the given uniform to an array of 1-element floats.
     *
     * This method will only succeed if the shader is actively bound.
     *
     * @param pos   The location of the uniform in the shader
     * @param count The number of elements in the array
     * @param value The array of floats
     */
    void setUniform1fv(GLint pos, GLsizei count, const GLfloat *value);

    /**
     * Sets the given uniform to an array of 1-element floats.
     *
     * This method will only succeed if the shader is actively bound.
     * It will silently fail (with no error) if name is does not refer
     * to  a valid uniform.
     *
     * @param name  The name of the uniform
     * @param count The number of elements in the array
     * @param value The array of floats
     */
    void setUniform1fv(const std::string name, GLsizei count, const GLfloat *value);

    /**
     * Sets the given uniform to an array of 2-element floats.
     *
     * This method will only succeed if the shader is actively bound.
     *
     * @param pos   The location of the uniform in the shader
     * @param count The number of elements in the array
     * @param value The array of floats
     */
    void setUniform2fv(GLint pos, GLsizei count, const GLfloat *value);

    /**
     * Sets the given uniform to an array of 2-element floats.
     *
     * This method will only succeed if the shader is actively bound.
     * It will silently fail (with no error) if name is does not refer
     * to  a valid uniform.
     *
     * @param name  The name of the uniform
     * @param count The number of elements in the array
     * @param value The array of floats
     */
    void setUniform2fv(const std::string name, GLsizei count, const GLfloat *value);

    /**
     * Sets the given uniform to an array of 3-element floats.
     *
     * This method will only succeed if the shader is actively bound.
     *
     * @param pos   The location of the uniform in the shader
     * @param count The number of elements in the array
     * @param value The array of floats
     */
    void setUniform3fv(GLint pos, GLsizei count, const GLfloat *value);

    /**
     * Sets the given uniform to an array of 3-element floats.
     *
     * This method will only succeed if the shader is actively bound.
     * It will silently fail (with no error) if name is does not refer
     * to  a valid uniform.
     *
     * @param name  The name of the uniform
     * @param count The number of elements in the array
     * @param value The array of floats
     */
    void setUniform3fv(const std::string name, GLsizei count, const GLfloat *value);

    /**
     * Sets the given uniform to an array of 4-element floats.
     *
     * This method will only succeed if the shader is actively bound.
     *
     * @param pos   The location of the uniform in the shader
     * @param count The number of elements in the array
     * @param value The array of floats
     */
    void setUniform4fv(GLint pos, GLsizei count, const GLfloat *value);

    /**
     * Sets the given uniform to an array of 4-element floats.
     *
     * This method will only succeed if the shader is actively bound.
     * It will silently fail (with no error) if name is does not refer
     * to  a valid uniform.
     *
     * @param name  The name of the uniform
     * @param count The number of elements in the array
     * @param value The array of floats
     */
    void setUniform4fv(const std::string name, GLsizei count, const GLfloat *value);

    /**
     * Sets the given uniform to an array of 1-element ints.
     *
     * This method will only succeed if the shader is actively bound.
     *
     * @param pos   The location of the uniform in the shader
     * @param count The number of elements in the array
     * @param value The array of ints
     */
    void setUniform1iv(GLint pos, GLsizei count, const GLint *value);
    
    /**
     * Sets the given uniform to an array of 1-element ints.
     *
     * This method will only succeed if the shader is actively bound.
     * It will silently fail (with no error) if name is does not refer
     * to  a valid uniform.
     *
     * @param name  The name of the uniform
     * @param count The number of elements in the array
     * @param value The array of ints
     */
    void setUniform1iv(const std::string name, GLsizei count, const GLint *value);

    /**
     * Sets the given uniform to an array of 2-element ints.
     *
     * This method will only succeed if the shader is actively bound.
     *
     * @param pos   The location of the uniform in the shader
     * @param count The number of elements in the array
     * @param value The array of ints
     */
    void setUniform2iv(GLint pos, GLsizei count, const GLint *value);

    /**
     * Sets the given uniform to an array of 2-element ints.
     *
     * This method will only succeed if the shader is actively bound.
     * It will silently fail (with no error) if name is does not refer
     * to  a valid uniform.
     *
     * @param name  The name of the uniform
     * @param count The number of elements in the array
     * @param value The array of ints
     */
    void setUniform2iv(const std::string name, GLsizei count, const GLint *value);

    
    /**
     * Sets the given uniform to an array of 3-element ints.
     *
     * @param pos   The location of the uniform in the shader
     * @param count The number of elements in the array
     * @param value The array of ints
     */
    void setUniform3iv(GLint pos, GLsizei count, const GLint *value);

    /**
     * Sets the given uniform to an array of 3-element ints.
     *
     * This method will only succeed if the shader is actively bound.
     * It will silently fail (with no error) if name is does not refer
     * to  a valid uniform.
     *
     * @param name  The name of the uniform
     * @param count The number of elements in the array
     * @param value The array of ints
     */
    void setUniform3iv(const std::string name, GLsizei count, const GLint *value);

    /**
     * Sets the given uniform to an array of 4-element ints.
     *
     * This method will only succeed if the shader is actively bound.
     *
     * @param pos   The location of the uniform in the shader
     * @param count The number of elements in the array
     * @param value The array of ints
     */
    void setUniform4iv(GLint pos, GLsizei count, const GLint *value);

    /**
     * Sets the given uniform to an array of 4-element ints.
     *
     * This method will only succeed if the shader is actively bound.
     * It will silently fail (with no error) if name is does not refer
     * to  a valid uniform.
     *
     * @param name  The name of the uniform
     * @param count The number of elements in the array
     * @param value The array of ints
     */
    void setUniform4iv(const std::string name, GLsizei count, const GLint *value);

    /**
     * Sets the given uniform to an array of 1-element unsigned ints.
     *
     * This method will only succeed if the shader is actively bound.
     *
     * @param pos   The location of the uniform in the shader
     * @param count The number of elements in the array
     * @param value The array of unsigned ints
     */
    void setUniform1uiv(GLint pos, GLsizei count, const GLuint *value);
    
    /**
     * Sets the given uniform to an array of 1-element unsigned ints.
     *
     * This method will only succeed if the shader is actively bound.
     * It will silently fail (with no error) if name is does not refer
     * to  a valid uniform.
     *
     * @param name  The name of the uniform
     * @param count The number of elements in the array
     * @param value The array of unsigned ints
     */
    void setUniform1uiv(const std::string name, GLsizei count, const GLuint *value);

    /**
     * Sets the given uniform to an array of 2-element unsigned ints.
     *
     * This method will only succeed if the shader is actively bound.
     *
     * @param pos   The location of the uniform in the shader
     * @param count The number of elements in the array
     * @param value The array of unsigned ints
     */
    void setUniform2uiv(GLint pos, GLsizei count, const GLuint *value);
    
    /**
     * Sets the given uniform to an array of 2-element unsigned ints.
     *
     * This method will only succeed if the shader is actively bound.
     * It will silently fail (with no error) if name is does not refer
     * to  a valid uniform.
     *
     * @param name  The name of the uniform
     * @param count The number of elements in the array
     * @param value The array of unsigned ints
     */
    void setUniform2uiv(const std::string name, GLsizei count, const GLuint *value);

    /**
     * Sets the given uniform to an array of 3-element unsigned ints.
     *
     * This method will only succeed if the shader is actively bound.
     *
     * @param pos   The location of the uniform in the shader
     * @param count The number of elements in the array
     * @param value The array of unsigned ints
     */
    void setUniform3uiv(GLint pos, GLsizei count, const GLuint *value);

    /**
     * Sets the given uniform to an array of 3-element unsigned ints.
     *
     * This method will only succeed if the shader is actively bound.
     * It will silently fail (with no error) if name is does not refer
     * to  a valid uniform.
     *
     * @param name  The name of the uniform
     * @param count The number of elements in the array
     * @param value The array of unsigned ints
     */
    void setUniform3uiv(const std::string name, GLsizei count, const GLuint *value);

    /**
     * Sets the given uniform to an array of 4-element unsigned ints.
     *
     * This method will only succeed if the shader is actively bound.
     *
     * @param pos   The location of the uniform in the shader
     * @param count The number of elements in the array
     * @param value The array of unsigned ints
     */
    void setUniform4uiv(GLint pos, GLsizei count, const GLuint *value);

    /**
     * Sets the given uniform to an array of 4-element unsigned ints.
     *
     * This method will only succeed if the shader is actively bound.
     * It will silently fail (with no error) if name is does not refer
     * to  a valid uniform.
     *
     * @param name  The name of the uniform
     * @param count The number of elements in the array
     * @param value The array of unsigned ints
     */
    void setUniform4uiv(const std::string name, GLsizei count, const GLuint *value);
    
    /**
     * Sets the given uniform to an array 2x2 matrices.
     *
     * This method will only succeed if the shader is actively bound.
     *
     * @param pos   The location of the uniform in the shader
     * @param count The number of elements in the array
     * @param value The array of matrices
     * @param tpose Whether to transpose the matrices
     */
    void setUniformMatrix2fv(GLint pos, GLsizei count, const GLfloat *value, GLboolean tpose=false);

    /**
     * Sets the given uniform to an array 2x2 matrices.
     *
     * This method will only succeed if the shader is actively bound.
     * It will silently fail (with no error) if name is does not refer
     * to  a valid uniform.
     *
     * @param name  The name of the uniform
     * @param count The number of elements in the array
     * @param value The array of matrices
     * @param tpose Whether to transpose the matrices
     */
    void setUniformMatrix2fv(const std::string name, GLsizei count, const GLfloat *value, GLboolean tpose=false);

    /**
     * Sets the given uniform to an array 3x3 matrices.
     *
     * This method will only succeed if the shader is actively bound.
     *
     * @param pos   The location of the uniform in the shader
     * @param count The number of elements in the array
     * @param value The array of matrices
     * @param tpose Whether to transpose the matrices
     */
    void setUniformMatrix3fv(GLint pos, GLsizei count, const GLfloat *value, GLboolean tpose=false);

    /**
     * Sets the given uniform to an array 3x3 matrices.
     *
     * This method will only succeed if the shader is actively bound.
     * It will silently fail (with no error) if name is does not refer
     * to  a valid uniform.
     *
     * @param name  The name of the uniform
     * @param count The number of elements in the array
     * @param value The array of matrices
     * @param tpose Whether to transpose the matrices
     */
    void setUniformMatrix3fv(const std::string name, GLsizei count, const GLfloat *value, GLboolean tpose=false);

    /**
     * Sets the given uniform to an array 4x4 matrices.
     *
     * This method will only succeed if the shader is actively bound.
     *
     * @param pos   The location of the uniform in the shader
     * @param count The number of elements in the array
     * @param value The array of matrices
     * @param tpose Whether to transpose the matrices
     */
    void setUniformMatrix4fv(GLint pos, GLsizei count, const GLfloat *value, GLboolean tpose=false);

    /**
     * Sets the given uniform to an array 4x4 matrices.
     *
     * This method will only succeed if the shader is actively bound.
     * It will silently fail (with no error) if name is does not refer
     * to  a valid uniform.
     *
     * @param name  The name of the uniform
     * @param count The number of elements in the array
     * @param value The array of matrices
     * @param tpose Whether to transpose the matrices
     */
    void setUniformMatrix4fv(const std::string name, GLsizei count, const GLfloat *value, GLboolean tpose=false);

    /**
     * Sets the given uniform to an array 2x3 matrices.
     *
     * This method will only succeed if the shader is actively bound.
     *
     * @param pos   The location of the uniform in the shader
     * @param count The number of elements in the array
     * @param value The array of matrices
     * @param tpose Whether to transpose the matrices
     */
    void setUniformMatrix2x3fv(GLint pos, GLsizei count, const GLfloat *value, GLboolean tpose=false);

    /**
     * Sets the given uniform to an array 2x3 matrices.
     *
     * This method will only succeed if the shader is actively bound.
     * It will silently fail (with no error) if name is does not refer
     * to  a valid uniform.
     *
     * @param name  The name of the uniform
     * @param count The number of elements in the array
     * @param value The array of matrices
     * @param tpose Whether to transpose the matrices
     */
    void setUniformMatrix2x3fv(const std::string name, GLsizei count, const GLfloat *value, GLboolean tpose=false);

    
    /**
     * Sets the given uniform to an array 3x2 matrices.
     *
     * This method will only succeed if the shader is actively bound.
     *
     * @param pos   The location of the uniform in the shader
     * @param count The number of elements in the array
     * @param value The array of matrices
     * @param tpose Whether to transpose the matrices
     */
    void setUniformMatrix3x2fv(GLint pos, GLsizei count, const GLfloat *value, GLboolean tpose=false);

    /**
     * Sets the given uniform to an array 3x2 matrices.
     *
     * This method will only succeed if the shader is actively bound.
     * It will silently fail (with no error) if name is does not refer
     * to  a valid uniform.
     *
     * @param name  The name of the uniform
     * @param count The number of elements in the array
     * @param value The array of matrices
     * @param tpose Whether to transpose the matrices
     */
    void setUniformMatrix3x2fv(const std::string name, GLsizei count, const GLfloat *value, GLboolean tpose=false);
    
    /**
     * Sets the given uniform to an array 2x4 matrices.
     *
     * This method will only succeed if the shader is actively bound.
     *
     * @param pos   The location of the uniform in the shader
     * @param count The number of elements in the array
     * @param value The array of matrices
     * @param tpose Whether to transpose the matrices
     */
    void setUniformMatrix2x4fv(GLint pos, GLsizei count, const GLfloat *value, GLboolean tpose=false);

    /**
     * Sets the given uniform to an array 2x4 matrices.
     *
     * This method will only succeed if the shader is actively bound.
     * It will silently fail (with no error) if name is does not refer
     * to  a valid uniform.
     *
     * @param name  The name of the uniform
     * @param count The number of elements in the array
     * @param value The array of matrices
     * @param tpose Whether to transpose the matrices
     */
    void setUniformMatrix2x4fv(const std::string name, GLsizei count, const GLfloat *value, GLboolean tpose=false);

    /**
     * Sets the given uniform to an array 4x2 matrices.
     *
     * This method will only succeed if the shader is actively bound.
     *
     * @param pos   The location of the uniform in the shader
     * @param count The number of elements in the array
     * @param value The array of matrices
     * @param tpose Whether to transpose the matrices
     */
    void setUniformMatrix4x2fv(GLint pos, GLsizei count, const GLfloat *value, GLboolean tpose=false);

    /**
     * Sets the given uniform to an array 4x2 matrices.
     *
     * This method will only succeed if the shader is actively bound.
     * It will silently fail (with no error) if name is does not refer
     * to  a valid uniform.
     *
     * @param name  The name of the uniform
     * @param count The number of elements in the array
     * @param value The array of matrices
     * @param tpose Whether to transpose the matrices
     */
    void setUniformMatrix4x2fv(const std::string name, GLsizei count, const GLfloat *value, GLboolean tpose=false);

    /**
     * Sets the given uniform to an array 3x4 matrices.
     *
     * This method will only succeed if the shader is actively bound.
     *
     * @param pos   The location of the uniform in the shader
     * @param count The number of elements in the array
     * @param value The array of matrices
     * @param tpose Whether to transpose the matrices
     */
    void setUniformMatrix3x4fv(GLint pos, GLsizei count, const GLfloat *value, GLboolean tpose=false);

    /**
     * Sets the given uniform to an array 3x4 matrices.
     *
     * This method will only succeed if the shader is actively bound.
     * It will silently fail (with no error) if name is does not refer
     * to  a valid uniform.
     *
     * @param name  The name of the uniform
     * @param count The number of elements in the array
     * @param value The array of matrices
     * @param tpose Whether to transpose the matrices
     */
    void setUniformMatrix3x4fv(const std::string name, GLsizei count, const GLfloat *value, GLboolean tpose=false);
    
    /**
     * Sets the given uniform to an array 4x3 matrices.
     *
     * This method will only succeed if the shader is actively bound.
     *
     * @param pos   The location of the uniform in the shader
     * @param count The number of elements in the array
     * @param value The array of matrices
     * @param tpose Whether to transpose the matrices
     */
    void setUniformMatrix4x3fv(GLint pos, GLsizei count, const GLfloat *value, GLboolean tpose=false);

    /**
     * Sets the given uniform to an array 4x3 matrices.
     *
     * This method will only succeed if the shader is actively bound.
     * It will silently fail (with no error) if name is does not refer
     * to  a valid uniform.
     *
     * @param name  The name of the uniform
     * @param count The number of elements in the array
     * @param value The array of matrices
     * @param tpose Whether to transpose the matrices
     */
    void setUniformMatrix4x3fv(const std::string name, GLsizei count, const GLfloat *value, GLboolean tpose=false);

    /**
     * Gets the given uniform as an array of float values
     *
     * This method will only succeed if the shader is actively bound.
     *
     * @param pos   The location of the uniform in the shader
     * @param size  The available size of the value array
     * @param value The array to receive the values
     *
     * @return true if data was successfully read into value
     */
    bool getUniformfv(GLint pos, GLsizei size, GLfloat *value) const;

    /**
     * Gets the given uniform as an array of float values
     *
     * This method will only succeed if the shader is actively bound.
     * It will silently fail (with no error) if name is does not refer
     * to  a valid uniform.
     *
     * @param name  The name of the uniform
     * @param size  The available size of the value array
     * @param value The array to receive the values
     *
     * @return true if data was successfully read into value
     */
    bool getUniformfv(const std::string name, GLsizei size, GLfloat *value) const;

    /**
     * Gets the given uniform as an array of integer values
     *
     * This method will only succeed if the shader is actively bound.
     *
     * @param pos   The location of the uniform in the shader
     * @param size  The available size of the value array
     * @param value The array to receive the values
     *
     * @return true if data was successfully read into value
     */
    bool getUniformiv(GLint pos, GLsizei size, GLint *value) const;

    /**
     * Gets the given uniform as an array of integer values
     *
     * This method will only succeed if the shader is actively bound.
     * It will silently fail (with no error) if name is does not refer
     * to  a valid uniform.
     *
     * @param name  The name of the uniform
     * @param size  The available size of the value array
     * @param value The array to receive the values
     *
     * @return true if data was successfully read into value
     */
    bool getUniformiv(const std::string name, GLsizei size, GLint *value) const;

    /**
     * Gets the given uniform as an array of unsigned integer values
     *
     * This method will only succeed if the shader is actively bound.
     *
     * @param pos   The location of the uniform in the shader
     * @param size  The available size of the value array
     * @param value The array to receive the values
     *
     * @return true if data was successfully read into value
     */
    bool getUniformuiv(GLint pos, GLsizei size, GLuint *value) const;

    /**
     * Gets the given uniform as an array of unsigned integer values
     *
     * This method will only succeed if the shader is actively bound.
     * It will silently fail (with no error) if name is does not refer
     * to  a valid uniform.
     *
     * @param name  The name of the uniform
     * @param size  The available size of the value array
     * @param value The array to receive the values
     *
     * @return true if data was successfully read into value
     */
    bool getUniformuiv(const std::string name, GLsizei size, GLuint *value) const;
};
    
}

#endif /* __CU_SHADER_H__ */
