//
//  CUShader.cpp
//  Cornell University Game Library (CUGL)
//
//  This module provides a general purpose shader class for GLSL shaders.
//  It supports compilations and has diagnostic tools for errors. The
//  shader is general enough that it should not need to be subclassed.
//  However, to use a shader, it must be attached to a VertexBuffer.
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

#include <cugl/util/CUDebug.h>
#include <cugl/util/CUStrings.h>
#include <cugl/render/CUShader.h>
#include <cugl/render/CUTexture.h>

using namespace cugl;

/**
 * Returns a pre-processed copy of a GLSL program
 *
 * The stringification macro does not preserve newlines and does not handle
 * #ifdef directives well.  The function gets around these problems.  Newlines
 * in a shader are marked by :: (double colon). In addition ifdef/endif directives
 * do not include the # before processing.
 *
 * @param code  The unprocessed GLSL program
 *
 * @return a pre-processed copy of a GLSL program
 */
std::string preprocess_shader(const std::string code) {
    std::string source = cugl::strtool::replaceall(code, "::", "\n");
    return source;
}

#pragma mark -
#pragma mark Compilation
/**
 * Compiles this shader from the given vertex and fragment shader sources.
 *
 * If compilation fails, it will display error messages on the log.
 *
 * @return true if compilation was successful.
 */
bool Shader::compile() {
    CUAssertLog(!_vertSource.empty(), "Vertex shader source is not defined");
    CUAssertLog(!_fragSource.empty(), "Fragment shader source is not defined");
    CUAssertLog(!_program,   "This shader is already compiled");
    
    _program = glCreateProgram();
    if (!_program) {
        CULogError("Unable to allocate shader program");
        return false;
    }
    
    //Create vertex shader and compile it
    _vertShader = glCreateShader( GL_VERTEX_SHADER );
    const char* source = _vertSource.c_str();
    glShaderSource( _vertShader, 1, &source, nullptr );
    glCompileShader( _vertShader );
    
    // Validate and quit if failed
    if (!validateShader(_vertShader, "vertex")) {
        dispose();
        return false;
    }

    //Create fragment shader and compile it
    _fragShader = glCreateShader( GL_FRAGMENT_SHADER );
    source = _fragSource.c_str();
    glShaderSource( _fragShader, 1, &source, nullptr );
    glCompileShader( _fragShader );
    
    // Validate and quit if failed
    if (!validateShader(_fragShader, "fragment")) {
        dispose();
        return false;
    }
    
    // Now kiss
    glAttachShader( _program, _vertShader );
    glAttachShader( _program, _fragShader );
    glLinkProgram( _program );
    
    //Check for errors
    GLint programSuccess = GL_TRUE;
    glGetProgramiv( _program, GL_LINK_STATUS, &programSuccess );
    if( programSuccess != GL_TRUE ) {
        CULogError( "Unable to link program %d.\n", _program );
        logProgramError(_program);
        dispose();
        return false;
    }
    
    return true;
}

/**
 * Deletes the OpenGL shader and resets all attributes.
 *
 * You must reinitialize the shader to use it.
 */
void Shader::dispose() {
    glUseProgram(NULL);
    if (_fragShader) { glDeleteShader(_fragShader); _fragShader = 0;}
    if (_vertShader) { glDeleteShader(_vertShader); _vertShader = 0;}
    if (_program) { glDeleteShader(_program); _program = 0;}
    _vertSource.clear();
    _fragSource.clear();

    _attribtypes.clear();
    _attribnames.clear();
    _attribsizes.clear();
    _uniformtypes.clear();
    _uniformnames.clear();
    _uniformsizes.clear();
    _uniblocknames.clear();
    _uniblocksizes.clear();
    _uniblockfields.clear();
}

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
bool Shader::validateShader(GLuint shader, const std::string type) {
    CUAssertLog( glIsShader( shader ), "Shader %d is not a valid shader", shader);
    GLint shaderCompiled = GL_FALSE;
    glGetShaderiv( shader, GL_COMPILE_STATUS, &shaderCompiled );
    if( shaderCompiled != GL_TRUE ) {
        CULogError( "Unable to compile %s shader %d.\n", type.c_str(), shader );
        logShaderError(shader);
        return false;
    }
    return true;
}

/**
 * Displays the shader compilation errors to the log.
 *
 * If there were no errors, this method will do nothing.
 *
 * @param shader    The shader to test
 */
void Shader::logShaderError(GLuint shader) {
    CUAssertLog( glIsShader( shader ), "Shader %d is not a valid shader", shader);
    //Make sure name is shader

    //Shader log length
    int infoLogLength = 0;
    int maxLength = infoLogLength;
        
    //Get info string length
    glGetShaderiv( shader, GL_INFO_LOG_LENGTH, &maxLength );
        
    //Allocate string
    char* infoLog = new char[ maxLength ];
        
    //Get info log
    glGetShaderInfoLog( shader, maxLength, &infoLogLength, infoLog );
    if( infoLogLength > 0 ) {
        // Print Log
        CULog( "%s\n", infoLog );
    }
        
    //Deallocate string
    delete[] infoLog;
}

/**
 * Displays the program linker errors to the log.
 *
 * If there were no errors, this method will do nothing.
 *
 * @param shader    The program to test
 */
void Shader::logProgramError( GLuint program ) {
    CUAssertLog( glIsProgram( program ), "Program %d is not a valid program", program);

    //Program log length
    int infoLogLength = 0;
    int maxLength = infoLogLength;
        
    //Get info string length
    glGetProgramiv( program, GL_INFO_LOG_LENGTH, &maxLength );
        
    //Allocate string
    char* infoLog = new char[ maxLength ];
        
    //Get info log
    glGetProgramInfoLog( program, maxLength, &infoLogLength, infoLog );
    if( infoLogLength > 0 ) {
        // Print Log
        CULogError( "%s\n", infoLog );
    }
    
    //Deallocate string
    delete[] infoLog;
}

/**
 * Querys all of the shader attributes and caches them for fast look-ups
 */
void Shader::cacheAttributes() {
    GLint count;

    GLint size;     // size of the variable
    GLenum type;    // type of the variable (float, vec3 or mat4, etc)

    const GLsizei bufSize = 16; // maximum name length
    GLchar name[bufSize];       // variable name in GLSL
    GLsizei length;             // name length
    
    glGetProgramiv(_program, GL_ACTIVE_ATTRIBUTES, &count);
    for (GLuint ii = 0; ii < count; ii++) {
        glGetActiveAttrib(_program, ii, bufSize, &length, &size, &type, name);
        GLenum error = glGetError();
        if (!error) {
            std::string key(name);
            _attribtypes[key] = type;
            _attribsizes[key] = size;
            _attribnames[ii] = key;
        }
    }
}

/**
 * Querys all of the shader uniforms and caches them for fast look-ups
 *
 * This includes uniform buffer blocks as well.
 */
void Shader::cacheUniforms() {
    GLint count;

    GLint size;     // size of the variable
    GLenum type;    // type of the variable (float, vec3 or mat4, etc)

    const GLsizei bufSize = 16; // maximum name length
    GLchar name[bufSize];       // variable name in GLSL
    GLsizei length;             // name length
    
    glGetProgramiv(_program, GL_ACTIVE_UNIFORMS, &count);
    for (GLuint ii = 0; ii < count; ii++) {
        glGetActiveUniform(_program, ii, bufSize, &length, &size, &type, name);
        GLenum error = glGetError();
        if (!error) {
            std::string key(name);
            _uniformtypes[key] = type;
            _uniformsizes[key] = size;
            _uniformnames[ii]  = key;
        }
    }
    
    glGetProgramiv(_program, GL_ACTIVE_UNIFORM_BLOCKS, &count);
    for (GLuint ii = 0; ii < count; ii++) {
        glGetActiveUniformBlockName(_program, ii, bufSize, &length, name);
        GLenum error = glGetError();
        if (!error) {
            std::string key(name);
            glGetActiveUniformBlockiv(_program, ii, GL_UNIFORM_BLOCK_DATA_SIZE, &size);
            _uniblocksizes[key] = size;
            _uniblocknames[ii]  = key;
            
            // Link the block to uniforms
            glGetActiveUniformBlockiv(_program, ii, GL_UNIFORM_BLOCK_ACTIVE_UNIFORMS, &size);
            int* ans = new int[size];
            glGetActiveUniformBlockiv(_program, ii, GL_UNIFORM_BLOCK_ACTIVE_UNIFORM_INDICES, ans);
            for(int jj = 0; jj < size; jj++) {
                _uniblockfields[ans[jj]] = ii;
            }
            delete[] ans;
        }
    }
}


#pragma mark -
#pragma mark Binding
/**
 * Initializes this shader with the given vertex and fragment source.
 *
 * The shader will compile the vertex and fragment sources and link
 * them together. When compilation is complete, the shader will not be
 * bound.  However, any shader that was actively bound during compilation
 * also be unbound as well.
 *
 * @param vsource   The source string for the vertex shader.
 * @param fsource   The source string for the fragment shader.
 *
 * @return true if initialization was successful.
 */
bool Shader::init(const std::string vsource, const std::string fsource) {
    _vertSource = vsource;
    _fragSource = fsource;
    if (!compile()) {
        return false;
    }
    
    cacheAttributes();
    cacheUniforms();
    bind();
    return true;
}

/**
 * Binds this shader, making it active.
 *
 * Once bound, any OpenGL calls will then be sent to this shader. This
 * call is reentrant, and may safely be called on an active shader.
 */
void Shader::bind() {
    CUAssertLog(_program, "Shader has not been initialized.");
    glUseProgram( _program );
}

/**
 * Unbinds this shader, making it no longer active.
 *
 * Once unbound, OpenGL calls will no longer be sent to this shader.
 */
void Shader::unbind() {
    CUAssertLog(_program, "Shader has not been initialized.");
    if (isBound()) {
        glUseProgram( NULL );
    }
}

/**
 * Returns true if this shader is currently bound.
 *
 * Any OpenGL calls will be sent to this shader only if it is bound.
 *
 * @return true if this shader is currently bound.
 */
bool Shader::isBound() const {
    GLint prog;
    glGetIntegerv(GL_CURRENT_PROGRAM,&prog);
    return prog == _program;
}
 

#pragma mark -
#pragma mark Attribute Properties
/**
 * Returns a vector of all attribute variables in this shader
 *
 * @return a vector of all attribute variables in this shader
 */
std::vector<std::string> Shader::getAttributes() const {
    std::vector<std::string> result;
    for(auto it = _attribnames.begin(); it != _attribnames.end(); ++it) {
        result.push_back(it->second);
    }
    return result;
}

/**
 * Returns the program offset of the given attribute
 *
 * If name is not a valid attribute, this method returns -1.
 *
 * @param name  The attribute variable name
 *
 * @return the program offset of the given attribute
 */
GLint Shader::getAttributeLocation(const std::string name) const {
    return glGetAttribLocation(_program,name.c_str());
}

/**
 * Returns the size (in bytes) of the given attribute
 *
 * @param name  The attribute variable name
 *
 * @return the size (in bytes) of the given attribute
 */
GLint Shader::getAttributeSize(const std::string name) const {
    auto search = _attribsizes.find(name);
    if (search == _attribsizes.end()) {
        return -1;
    }
    return search->second;
}

/**
 * Returns the type of the given attribute
 *
 * If name is not a valid attribute, this method returns GL_FALSE.
 *
 * @param name  The attribute variable name
 *
 * @return the type of the given attribute
 */
GLenum Shader::getAttributeType(const std::string name) const  {
    auto search = _attribtypes.find(name);
    if (search == _attribtypes.end()) {
        return GL_FALSE;
    }
    return search->second;
}

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
GLint Shader::getOutputLocation(const std::string name) const {
    return glGetFragDataLocation(_program, name.c_str());
}


#pragma mark -
#pragma mark Uniform Properties
/**
 * Returns a vector of all uniform variables in this shader
 *
 * @return a vector of all uniform variables in this shader
 */
std::vector<std::string> Shader::getUniforms() const {
    std::vector<std::string> result;
    for(auto it = _uniformnames.begin(); it != _uniformnames.end(); ++it) {
        result.push_back(it->second);
    }
    return result;
}

/**
 * Returns the program offset of the given uniform
 *
 * If name is not a valid uniform, this method returns -1.
 *
 * @param name  The uniform variable name
 *
 * @return the program offset of the given uniform
 */
GLint Shader::getUniformLocation(const std::string name) const {
    return glGetUniformLocation(_program,name.c_str());
}

/**
 * Returns the size (in bytes) of the given uniform
 *
 * If name is not a valid uniform, this method returns -1.
 *
 * @param name  The uniform variable name
 *
 * @return the size (in bytes) of the given uniform
 */
GLint Shader::getUniformSize(const std::string name) const {
    auto search = _uniformsizes.find(name);
    if (search == _uniformsizes.end()) {
        return -1;
    }
    return search->second;
}

/**
 * Returns the type of the given uniform
 *
 * If name is not a valid uniform, this method returns GL_FALSE.
 *
 * @param name  The uniform variable name
 *
 * @return the type of the given uniform
 */
GLenum Shader::getUniformType(const std::string name) const {
    auto search = _uniformtypes.find(name);
    if (search == _uniformtypes.end()) {
        return GL_FALSE;
    }
    return search->second;
}


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
std::vector<std::string> Shader::getSamplers() const {
    std::vector<std::string> result;
    for(auto it = _uniformtypes.begin(); it != _uniformtypes.end(); ++it) {
        if (it->second == GL_SAMPLER_2D) {
            result.push_back(it->first);
        }
    }
    return result;
}

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
GLint Shader::getSamplerLocation(const std::string name) const {
    GLint result =  glGetUniformLocation(_program,name.c_str());
    if (result != -1 && _uniformtypes.at(name) != GL_SAMPLER_2D) {
        result = -1;
    }
    return result;
}

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
void Shader::setSampler(GLint pos, GLuint bpoint) {
    setUniform1ui(pos,bpoint);
}

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
void Shader::setSampler(const std::string name, GLuint bpoint) {
    setUniform1ui(name,bpoint);
}

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
void Shader::setSampler(GLint pos, const std::shared_ptr<Texture>& texture) {
    GLuint bpoint = texture == nullptr ? 0 : texture->getBindPoint();
    setUniform1ui(pos,bpoint);
}

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
void Shader::setSampler(const std::string name, const std::shared_ptr<Texture>& texture) {
    GLuint bpoint = texture == nullptr ? 0 : texture->getBindPoint();
    setUniform1ui(name,bpoint);
}

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
GLuint Shader::getSampler(GLint pos) const {
    GLuint result = 0;
    getUniformuiv(pos,1,&result);
    return result;
}

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
GLuint Shader::getSampler(const std::string name) const {
    GLuint result = 0;
    getUniformuiv(name,1,&result);
    return result;
}
    

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
std::vector<std::string> Shader::getUniformBlocks() const {
    std::vector<std::string> result;
    for(auto it = _uniblocknames.begin(); it != _uniblocknames.end(); ++it) {
        result.push_back(it->second);
    }
    return result;
}

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
std::vector<std::string> Shader::getUniformsForBlock(GLint pos) const {
    std::vector<std::string> result;
    for(auto it = _uniblockfields.begin(); it != _uniblockfields.end(); ++it) {
        if (it->first == pos) {
            result.push_back(_uniformnames.at(it->second));
        }
    }
    return result;
}

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
std::vector<std::string> Shader::getUniformsForBlock(std::string name) const {
    std::vector<std::string> result;
    GLuint index = glGetUniformBlockIndex(_program, name.c_str());
    if (index == GL_INVALID_INDEX) {
        return result;
    }
    for(auto it = _uniblockfields.begin(); it != _uniblockfields.end(); ++it) {
        if (it->first == index) {
            result.push_back(_uniformnames.at(it->second));
        }
    }
    return result;
}

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
void Shader::setUniformBlock(GLint pos, GLuint bindpoint) {
    glUniformBlockBinding(_program, pos, bindpoint);
}

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
void Shader::setUniformBlock(const std::string name, GLuint bindpoint) {
    GLuint index = glGetUniformBlockIndex(_program, name.c_str());
    if (index != GL_INVALID_INDEX) {
        glUniformBlockBinding(_program, index, bindpoint);
    }
}

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
void Shader::setUniformBlock(GLint pos, const std::shared_ptr<UniformBuffer>& buffer) {
    GLuint bpoint = buffer == nullptr ? 0 : buffer->getBindPoint();
    
    // Do some verification
    for(auto it = _uniblockfields.begin(); it != _uniblockfields.end(); ++it) {
        if (it->second == pos) {
            const std::string name = _uniformnames.at(it->first);
            GLsizei offset = buffer->getOffset(name);
            if (offset == cugl::UniformBuffer::INVALID_OFFSET) {
                CUWarn("Uniform buffer is missing variable '%s'.",name.c_str());
            }
        }
    }

    std::vector<std::string> names = buffer->getOffsets();
    std::unordered_map<std::string, bool> check;
    for(auto it = names.begin(); it != names.end(); ++it) {
        check[*it] = false;
    }
    for(auto it = _uniblockfields.begin(); it != _uniblockfields.end(); ++it) {
        if (it->second == pos) {
            check[_uniformnames.at(it->first)] = true;
        }
    }
    for(auto it = check.begin(); it != check.end(); ++it) {
        if (!it->second) {
            CUWarn("Shader is missing variable '%s'.",it->first.c_str());
        }
    }
    
    // Now bind
    glUniformBlockBinding(_program, pos, bpoint);
}

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
void Shader::setUniformBlock(const std::string name,
                             const std::shared_ptr<UniformBuffer>& buffer) {
    GLuint index = glGetUniformBlockIndex(_program, name.c_str());
    if (index != GL_INVALID_INDEX) {
        setUniformBlock(index, buffer);
    }
}

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
GLuint Shader::getUniformBlock(GLint pos) const {
    GLint block;
    glGetActiveUniformBlockiv(_program,pos,GL_UNIFORM_BLOCK_BINDING,&block);
    return block;
}

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
GLuint Shader::getUniformBlock(const std::string name) const {
    GLuint index = glGetUniformBlockIndex(_program, name.c_str());
    if (index == GL_INVALID_INDEX) {
        return 0;
    }
    
    GLint block;
    glGetActiveUniformBlockiv(_program,index,GL_UNIFORM_BLOCK_BINDING,&block);
    return block;
}


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
void Shader::setUniformVec2(GLint pos, const Vec2 vec) {
    CUAssertLog(isBound(), "Shader is not active.");
    glUniform2f(pos,vec.x,vec.y);
}

/**
 * Sets the given uniform to a vector value.
 *
 * This method will only succeed if the shader is actively bound.
 *
 * @param name  The name of the uniform
 * @param vec   The value for the uniform
 */
void Shader::setUniformVec2(const std::string name, const Vec2 vec) {
    CUAssertLog(isBound(), "Shader is not active.");
    GLint locale = getUniformLocation(name.c_str());
    if (locale >= 0) glUniform2f(locale,vec.x,vec.y);
}

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
bool Shader::getUniformVec2(GLint pos, Vec2& vec) const {
    float* data = reinterpret_cast<float*>(&vec);
    return getUniformfv(pos, 2, data);
}

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
bool Shader::getUniformVec2(const std::string name, Vec2& vec) const {
    float* data = reinterpret_cast<float*>(&vec);
    return getUniformfv(name, 2, data);
}

/**
 * Sets the given uniform to a vector value.
 *
 * This method will only succeed if the shader is actively bound.
 *
 * @param pos   The location of the uniform in the shader
 * @param vec   The value for the uniform
 */
void Shader::setUniformVec3(GLint pos, const Vec3 vec) {
    CUAssertLog(isBound(), "Shader is not active.");
    glUniform3f(pos,vec.x,vec.y,vec.z);
}

/**
 * Sets the given uniform to a vector value.
 *
 * This method will only succeed if the shader is actively bound.
 *
 * @param name  The name of the uniform
 * @param vec   The value for the uniform
 */
void Shader::setUniformVec3(const std::string name, const Vec3 vec) {
    CUAssertLog(isBound(), "Shader is not active.");
    GLint locale = getUniformLocation(name.c_str());
    if (locale >= 0) glUniform3f(locale,vec.x,vec.y,vec.z);
}

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
bool Shader::getUniformVec3(GLint pos, Vec3& vec) const {
    float* data = reinterpret_cast<float*>(&vec);
    return getUniformfv(pos, 3, data);
}

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
bool Shader::getUniformVec3(const std::string name, Vec3& vec) const {
    float* data = reinterpret_cast<float*>(&vec);
    return getUniformfv(name, 3, data);
}

/**
 * Sets the given uniform to a vector value.
 *
 * This method will only succeed if the shader is actively bound.
 *
 * @param pos   The location of the uniform in the shader
 * @param vec   The value for the uniform
 */
void Shader::setUniformVec4(GLint pos, const Vec4 vec) {
    CUAssertLog(isBound(), "Shader is not active.");
    glUniform4f(pos,vec.x,vec.y,vec.z,vec.w);
}

/**
 * Sets the given uniform to a vector value.
 *
 * This method will only succeed if the shader is actively bound.
 *
 * @param name  The name of the uniform
 * @param vec   The value for the uniform
 */
void Shader::setUniformVec4(const std::string name, const Vec4 vec) {
    CUAssertLog(isBound(), "Shader is not active.");
    GLint locale = getUniformLocation(name.c_str());
    if (locale >= 0) glUniform4f(locale,vec.x,vec.y,vec.z,vec.w);
}

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
bool Shader::getUniformVec4(GLint pos, Vec4& vec) const {
    float* data = reinterpret_cast<float*>(&vec);
    return getUniformfv(pos, 4, data);
}

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
bool Shader::getUniformVec4(const std::string name, Vec4& vec) const {
    float* data = reinterpret_cast<float*>(&vec);
    return getUniformfv(name, 4, data);
}

/**
 * Sets the given uniform to a color value.
 *
 * This method will only succeed if the shader is actively bound.
 *
 * @param pos       The location of the uniform in the shader
 * @param color   The value for the uniform
 */
void Shader::setUniformColor4(GLint pos, const Color4 color) {
    setUniformVec4(pos, (Vec4)color);
}

/**
 * Sets the given uniform to a color value.
 *
 * This method will only succeed if the shader is actively bound.
 *
 * @param name      The name of the uniform
 * @param color   The value for the uniform
 */
void Shader::setUniformColor4(const std::string name, const Color4 color) {
    setUniformVec4(name, (Vec4)color);
}

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
bool Shader::getUniformColor4(GLint pos, Color4& color) const {
    float data[4];
    if (getUniformfv(pos, 4, data)) {
        color.set(data);
        return true;
    }
    return false;
}

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
bool Shader::getUniformColor4(const std::string name, Color4& color) const {
    float data[4];
    if (getUniformfv(name, 4, data)) {
        color.set(data);
        return true;
    }
    return false;
}

/**
 * Sets the given uniform to a color value.
 *
 * This method will only succeed if the shader is actively bound.
 *
 * @param pos       The location of the uniform in the shader
 * @param color   The value for the uniform
 */
void Shader::setUniformColor4f(GLint pos, const Color4f color) {
    setUniformVec4(pos, (Vec4)color);
}
/**
 * Sets the given uniform to a color value.
 *
 * This method will only succeed if the shader is actively bound.
 *
 * @param name      The name of the uniform
 * @param color   The value for the uniform
 */
void Shader::setUniformColor4f(const std::string name, const Color4f color) {
    setUniformVec4(name, (Vec4)color);
}

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
bool Shader::getUniformColor4f(GLint pos, Color4f& color) const {
    float* data = reinterpret_cast<float*>(&color);
    return (getUniformfv(pos, 4, data));
}

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
bool Shader::getUniformColor4f(const std::string name, Color4f& color) const {
    float* data = reinterpret_cast<float*>(&color);
    return (getUniformfv(name, 4, data));
}

/**
 * Sets the given uniform to a matrix value.
 *
 * This method will only succeed if the shader is actively bound.
 *
 * @param pos   The location of the uniform in the shader
 * @param mat   The value for the uniform
 */
void Shader::setUniformMat4(GLint pos, const Mat4& mat) {
    CUAssertLog(isBound(), "Shader is not active.");
    glUniformMatrix4fv(pos,1,false,mat.m);
}

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
void Shader::setUniformMat4(const std::string name, const Mat4& mat) {
    CUAssertLog(isBound(), "Shader is not active.");
    GLint locale = getUniformLocation(name.c_str());
    if (locale >= 0) glUniformMatrix4fv(locale,1,false,mat.m);
}

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
bool Shader::getUniformMat4(GLint pos, Mat4& mat) const {
    return getUniformfv(pos, 16, mat.m);
}

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
bool Shader::getUniformMat4(const std::string name, Mat4& mat) const {
    return getUniformfv(name, 16, mat.m);
}

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
void Shader::setUniformAffine2(GLint pos, const Affine2& mat) {
    CUAssertLog(isBound(), "Shader is not active.");
    float data[9];
    mat.get3x3(data);
    glUniformMatrix3fv(pos,1,false,data);
}

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
void Shader::setUniformAffine2(const std::string name, const Affine2& mat) {
    CUAssertLog(isBound(), "Shader is not active.");
    GLint locale = getUniformLocation(name.c_str());
    if (locale >= 0) {
        float data[9];
        mat.get3x3(data);
        glUniformMatrix3fv(locale,1,false,data);
    }
}

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
bool Shader::getUniformAffine2(GLint pos, Affine2& mat) const {
    float data[9];
    if (getUniformfv(pos, 9, data)) {
        mat.set(data, 3);
        return true;
    }
    return false;
}

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
bool Shader::getUniformAffine2(const std::string name, Affine2& mat) const {
    float data[9];
    if (getUniformfv(name, 9, data)) {
        mat.set(data, 3);
        return true;
    }
    return false;
}

/**
 * Sets the given uniform to a quaternion.
 *
 * This method will only succeed if the shader is actively bound.
 *
 * @param pos   The location of the uniform in the shader
 * @param mat   The value for the uniform
 */
void Shader::setUniformQuaternion(GLint pos, const Quaternion& quat) {
    setUniformVec4(pos, (Vec4)quat);
}

/**
 * Sets the given uniform to a quaternion.
 *
 * This method will only succeed if the shader is actively bound.
 *
 * @param name  The name of the uniform
 * @param mat   The value for the uniform
 */
void Shader::setUniformQuaternion(const std::string name, const Quaternion& quat) {
    setUniformVec4(name, (Vec4)quat);
}

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
bool Shader::getUniformQuaternion(GLint pos, Quaternion& quat) const {
    float* data = reinterpret_cast<float*>(&quat);
    return getUniformfv(pos, 4, data);
}

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
bool Shader::getUniformQuaternion(const std::string name, Quaternion& quat) const {
    float* data = reinterpret_cast<float*>(&quat);
    return getUniformfv(name, 4, data);
}


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
void Shader::setUniform1f(GLint pos, GLfloat v0) {
	CUAssertLog(isBound(), "Shader is not active.");
	glUniform1f(pos, v0);
}

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
void Shader::setUniform1f(const std::string name, GLfloat v0) {
	CUAssertLog(isBound(), "Shader is not active.");
	GLint locale = getUniformLocation(name.c_str());
	if (locale >= 0) glUniform1f(locale, v0);
}

/**
 * Sets the given uniform to a pair of float values.
 *
 * This method will only succeed if the shader is actively bound.
 *
 * @param pos   The location of the uniform in the shader
 * @param v0    The first value for the uniform
 * @param v1    The second value for the uniform
 */
void Shader::setUniform2f(GLint pos, GLfloat v0, GLfloat v1) {
	CUAssertLog(isBound(), "Shader is not active.");
	glUniform2f(pos, v0, v1);
}

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
void Shader::setUniform2f(const std::string name, GLfloat v0, GLfloat v1) {
	CUAssertLog(isBound(), "Shader is not active.");
	GLint locale = getUniformLocation(name.c_str());
	if (locale >= 0) glUniform2f(locale, v0, v1);
}

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
void Shader::setUniform3f(GLint pos, GLfloat v0, GLfloat v1, GLfloat v2) {
	CUAssertLog(isBound(), "Shader is not active.");
	glUniform3f(pos, v0, v1, v2);
}

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
void Shader::setUniform3f(const std::string name, GLfloat v0, GLfloat v1, GLfloat v2) {
	CUAssertLog(isBound(), "Shader is not active.");
	GLint locale = getUniformLocation(name.c_str());
	if (locale >= 0) glUniform3f(locale, v0, v1, v2);
}

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
void Shader::setUniform4f(GLint pos, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3) {
	CUAssertLog(isBound(), "Shader is not active.");
	glUniform4f(pos, v0, v1, v2, v3);
}

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
void Shader::setUniform4f(const std::string name, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3) {
	CUAssertLog(isBound(), "Shader is not active.");
	GLint locale = getUniformLocation(name.c_str());
	if (locale >= 0) glUniform4f(locale, v0, v1, v2, v3);
}

/**
 * Sets the given uniform to a single int value.
 *
 * This method will only succeed if the shader is actively bound.
 *
 * @param pos   The location of the uniform in the shader
 * @param v0    The value for the uniform
 */
void Shader::setUniform1i(GLint pos, GLint v0) {
	CUAssertLog(isBound(), "Shader is not active.");
	glUniform1i(pos, v0);
}

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
void Shader::setUniform1i(const std::string name, GLint v0) {
	CUAssertLog(isBound(), "Shader is not active.");
	GLint locale = getUniformLocation(name.c_str());
	if (locale >= 0) glUniform1i(locale, v0);
}

/**
 * Sets the given uniform to a pair of int values.
 *
 * This method will only succeed if the shader is actively bound.
 *
 * @param pos   The location of the uniform in the shader
 * @param v0    The first value for the uniform
 * @param v1    The second value for the uniform
 */
void Shader::setUniform2i(GLint pos, GLint v0, GLint v1) {
	CUAssertLog(isBound(), "Shader is not active.");
	glUniform2i(pos, v0, v1);
}

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
void Shader::setUniform2i(const std::string name, GLint v0, GLint v1) {
	CUAssertLog(isBound(), "Shader is not active.");
	GLint locale = getUniformLocation(name.c_str());
	if (locale >= 0) glUniform2i(locale, v0, v1);
}

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
void Shader::setUniform3i(GLint pos, GLint v0, GLint v1, GLint v2) {
	CUAssertLog(isBound(), "Shader is not active.");
	glUniform3i(pos, v0, v1, v2);
}

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
void Shader::setUniform3i(const std::string name, GLint v0, GLint v1, GLint v2) {
	CUAssertLog(isBound(), "Shader is not active.");
	GLint locale = getUniformLocation(name.c_str());
	if (locale >= 0) glUniform3i(locale, v0, v1, v2);
}

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
void Shader::setUniform4i(GLint pos, GLint v0, GLint v1, GLint v2, GLint v3) {
	CUAssertLog(isBound(), "Shader is not active.");
	glUniform4i(pos, v0, v1, v2, v3);
}

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
void Shader::setUniform4i(const std::string name, GLint v0, GLint v1, GLint v2, GLint v3) {
	CUAssertLog(isBound(), "Shader is not active.");
	GLint locale = getUniformLocation(name.c_str());
	if (locale >= 0) glUniform4i(locale, v0, v1, v2, v3);
}

/**
 * Sets the given uniform to a single unsigned value.
 *
 * This method will only succeed if the shader is actively bound.
 *
 * @param pos   The location of the uniform in the shader
 * @param v0    The value for the uniform
 */
void Shader::setUniform1ui(GLint pos, GLuint v0) {
	CUAssertLog(isBound(), "Shader is not active.");
	glUniform1ui(pos, v0);
}

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
void Shader::setUniform1ui(const std::string name, GLuint v0) {
	CUAssertLog(isBound(), "Shader is not active.");
	GLint locale = getUniformLocation(name.c_str());
	if (locale >= 0) glUniform1ui(locale, v0);
}

/**
 * Sets the given uniform to a pair of unsigned values.
 *
 * This method will only succeed if the shader is actively bound.
 *
 * @param pos   The location of the uniform in the shader
 * @param v0    The first value for the uniform
 * @param v1    The second value for the uniform
 */
void Shader::setUniform2ui(GLint pos, GLuint v0, GLuint v1) {
	CUAssertLog(isBound(), "Shader is not active.");
	glUniform2ui(pos, v0, v1);
}

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
void Shader::setUniform2ui(const std::string name, GLuint v0, GLuint v1) {
	CUAssertLog(isBound(), "Shader is not active.");
	GLint locale = getUniformLocation(name.c_str());
	if (locale >= 0) glUniform2ui(locale, v0, v1);
}

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
void Shader::setUniform3ui(GLint pos, GLuint v0, GLuint v1, GLuint v2) {
	CUAssertLog(isBound(), "Shader is not active.");
	glUniform3ui(pos, v0, v1, v2);
}

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
void Shader::setUniform3ui(const std::string name, GLuint v0, GLuint v1, GLuint v2) {
	CUAssertLog(isBound(), "Shader is not active.");
	GLint locale = getUniformLocation(name.c_str());
	if (locale >= 0) glUniform3ui(locale, v0, v1, v2);
}

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
void Shader::setUniform4ui(GLint pos, GLuint v0, GLuint v1, GLuint v2, GLuint v3) {
	CUAssertLog(isBound(), "Shader is not active.");
	glUniform4ui(pos, v0, v1, v2, v3);
}

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
void Shader::setUniform4ui(const std::string name, GLuint v0, GLuint v1, GLuint v2, GLuint v3) {
	CUAssertLog(isBound(), "Shader is not active.");
	GLint locale = getUniformLocation(name.c_str());
	if (locale >= 0) glUniform4ui(locale, v0, v1, v2, v3);
}

/**
 * Sets the given uniform to an array of 1-element floats.
 *
 * This method will only succeed if the shader is actively bound.
 *
 * @param pos   The location of the uniform in the shader
 * @param count The number of elements in the array
 * @param value The array of floats
 */
void Shader::setUniform1fv(GLint pos, GLsizei count, const GLfloat *value) {
	CUAssertLog(isBound(), "Shader is not active.");
	glUniform1fv(pos, count, value);
}

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
void Shader::setUniform1fv(const std::string name, GLsizei count, const GLfloat *value) {
	CUAssertLog(isBound(), "Shader is not active.");
	GLint locale = getUniformLocation(name.c_str());
	if (locale >= 0) glUniform1fv(locale, count, value);
}

/**
 * Sets the given uniform to an array of 2-element floats.
 *
 * This method will only succeed if the shader is actively bound.
 *
 * @param pos   The location of the uniform in the shader
 * @param count The number of elements in the array
 * @param value The array of floats
 */
void Shader::setUniform2fv(GLint pos, GLsizei count, const GLfloat *value) {
	CUAssertLog(isBound(), "Shader is not active.");
	glUniform2fv(pos, count, value);
}

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
void Shader::setUniform2fv(const std::string name, GLsizei count, const GLfloat *value) {
	CUAssertLog(isBound(), "Shader is not active.");
	GLint locale = getUniformLocation(name.c_str());
	if (locale >= 0) glUniform2fv(locale, count, value);
}

/**
 * Sets the given uniform to an array of 3-element floats.
 *
 * This method will only succeed if the shader is actively bound.
 *
 * @param pos   The location of the uniform in the shader
 * @param count The number of elements in the array
 * @param value The array of floats
 */
void Shader::setUniform3fv(GLint pos, GLsizei count, const GLfloat *value) {
	CUAssertLog(isBound(), "Shader is not active.");
	glUniform3fv(pos, count, value);
}

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
void Shader::setUniform3fv(const std::string name, GLsizei count, const GLfloat *value) {
	CUAssertLog(isBound(), "Shader is not active.");
	GLint locale = getUniformLocation(name.c_str());
	if (locale >= 0) glUniform3fv(locale, count, value);
}

/**
 * Sets the given uniform to an array of 4-element floats.
 *
 * This method will only succeed if the shader is actively bound.
 *
 * @param pos   The location of the uniform in the shader
 * @param count The number of elements in the array
 * @param value The array of floats
 */
void Shader::setUniform4fv(GLint pos, GLsizei count, const GLfloat *value) {
	CUAssertLog(isBound(), "Shader is not active.");
	glUniform4fv(pos, count, value);
}

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
void Shader::setUniform4fv(const std::string name, GLsizei count, const GLfloat *value) {
	CUAssertLog(isBound(), "Shader is not active.");
	GLint locale = getUniformLocation(name.c_str());
	if (locale >= 0) glUniform4fv(locale, count, value);
}

/**
 * Sets the given uniform to an array of 1-element ints.
 *
 * This method will only succeed if the shader is actively bound.
 *
 * @param pos   The location of the uniform in the shader
 * @param count The number of elements in the array
 * @param value The array of ints
 */
void Shader::setUniform1iv(GLint pos, GLsizei count, const GLint *value) {
	CUAssertLog(isBound(), "Shader is not active.");
	glUniform1iv(pos, count, value);
}

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
void Shader::setUniform1iv(const std::string name, GLsizei count, const GLint *value) {
	CUAssertLog(isBound(), "Shader is not active.");
	GLint locale = getUniformLocation(name.c_str());
	if (locale >= 0) glUniform1iv(locale, count, value);
}

/**
 * Sets the given uniform to an array of 2-element ints.
 *
 * This method will only succeed if the shader is actively bound.
 *
 * @param pos   The location of the uniform in the shader
 * @param count The number of elements in the array
 * @param value The array of ints
 */
void Shader::setUniform2iv(GLint pos, GLsizei count, const GLint *value) {
	CUAssertLog(isBound(), "Shader is not active.");
	glUniform2iv(pos, count, value);
}

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
void Shader::setUniform2iv(const std::string name, GLsizei count, const GLint *value) {
	CUAssertLog(isBound(), "Shader is not active.");
	GLint locale = getUniformLocation(name.c_str());
	if (locale >= 0) glUniform2iv(locale, count, value);
}

/**
 * Sets the given uniform to an array of 3-element ints.
 *
 * This method will only succeed if the shader is actively bound.
 *
 * @param pos   The location of the uniform in the shader
 * @param count The number of elements in the array
 * @param value The array of ints
 */
void Shader::setUniform3iv(GLint pos, GLsizei count, const GLint *value) {
	CUAssertLog(isBound(), "Shader is not active.");
	glUniform3iv(pos, count, value);
}

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
void Shader::setUniform3iv(const std::string name, GLsizei count, const GLint *value) {
	CUAssertLog(isBound(), "Shader is not active.");
	GLint locale = getUniformLocation(name.c_str());
	if (locale >= 0) glUniform3iv(locale, count, value);
}

/**
 * Sets the given uniform to an array of 4-element ints.
 *
 * This method will only succeed if the shader is actively bound.
 *
 * @param pos   The location of the uniform in the shader
 * @param count The number of elements in the array
 * @param value The array of ints
 */
void Shader::setUniform4iv(GLint pos, GLsizei count, const GLint *value) {
	CUAssertLog(isBound(), "Shader is not active.");
	glUniform4iv(pos, count, value);
}

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
void Shader::setUniform4iv(const std::string name, GLsizei count, const GLint *value) {
	CUAssertLog(isBound(), "Shader is not active.");
	GLint locale = getUniformLocation(name.c_str());
	if (locale >= 0) glUniform4iv(locale, count, value);
}

/**
 * Sets the given uniform to an array of 1-element unsigned ints.
 *
 * This method will only succeed if the shader is actively bound.
 *
 * @param pos   The location of the uniform in the shader
 * @param count The number of elements in the array
 * @param value The array of unsigned ints
 */
void Shader::setUniform1uiv(GLint pos, GLsizei count, const GLuint *value) {
	CUAssertLog(isBound(), "Shader is not active.");
	glUniform1uiv(pos, count, value);
}

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
void Shader::setUniform1uiv(const std::string name, GLsizei count, const GLuint *value) {
	CUAssertLog(isBound(), "Shader is not active.");
	GLint locale = getUniformLocation(name.c_str());
	if (locale >= 0) glUniform1uiv(locale, count, value);
}

/**
 * Sets the given uniform to an array of 2-element unsigned ints.
 *
 * This method will only succeed if the shader is actively bound.
 *
 * @param pos   The location of the uniform in the shader
 * @param count The number of elements in the array
 * @param value The array of unsigned ints
 */
void Shader::setUniform2uiv(GLint pos, GLsizei count, const GLuint *value) {
	CUAssertLog(isBound(), "Shader is not active.");
	glUniform2uiv(pos, count, value);
}

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
void Shader::setUniform2uiv(const std::string name, GLsizei count, const GLuint *value) {
	CUAssertLog(isBound(), "Shader is not active.");
	GLint locale = getUniformLocation(name.c_str());
	if (locale >= 0) glUniform2uiv(locale, count, value);
}

/**
 * Sets the given uniform to an array of 3-element unsigned ints.
 *
 * This method will only succeed if the shader is actively bound.
 *
 * @param pos   The location of the uniform in the shader
 * @param count The number of elements in the array
 * @param value The array of unsigned ints
 */
void Shader::setUniform3uiv(GLint pos, GLsizei count, const GLuint *value) {
	CUAssertLog(isBound(), "Shader is not active.");
	glUniform3uiv(pos, count, value);
}

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
void Shader::setUniform3uiv(const std::string name, GLsizei count, const GLuint *value) {
	CUAssertLog(isBound(), "Shader is not active.");
	GLint locale = getUniformLocation(name.c_str());
	if (locale >= 0) glUniform3uiv(locale, count, value);
}

/**
 * Sets the given uniform to an array of 4-element unsigned ints.
 *
 * This method will only succeed if the shader is actively bound.
 *
 * @param pos   The location of the uniform in the shader
 * @param count The number of elements in the array
 * @param value The array of unsigned ints
 */
void Shader::setUniform4uiv(GLint pos, GLsizei count, const GLuint *value) {
	CUAssertLog(isBound(), "Shader is not active.");
	glUniform4uiv(pos, count, value);
}

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
void Shader::setUniform4uiv(const std::string name, GLsizei count, const GLuint *value) {
	CUAssertLog(isBound(), "Shader is not active.");
	GLint locale = getUniformLocation(name.c_str());
	if (locale >= 0) glUniform4uiv(locale, count, value);
}

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
void Shader::setUniformMatrix2fv(GLint pos, GLsizei count, const GLfloat *value, GLboolean tpose) {
	CUAssertLog(isBound(), "Shader is not active.");
	glUniformMatrix2fv(pos, count, tpose, value);
}

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
void Shader::setUniformMatrix2fv(const std::string name, GLsizei count, const GLfloat *value, GLboolean tpose) {
	CUAssertLog(isBound(), "Shader is not active.");
	GLint locale = getUniformLocation(name.c_str());
	if (locale >= 0) glUniformMatrix2fv(locale, count, tpose, value);
}

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
void Shader::setUniformMatrix3fv(GLint pos, GLsizei count, const GLfloat *value, GLboolean tpose) {
	CUAssertLog(isBound(), "Shader is not active.");
	glUniformMatrix3fv(pos, count, tpose, value);
}

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
void Shader::setUniformMatrix3fv(const std::string name, GLsizei count, const GLfloat *value, GLboolean tpose) {
	CUAssertLog(isBound(), "Shader is not active.");
	GLint locale = getUniformLocation(name.c_str());
	if (locale >= 0) glUniformMatrix3fv(locale, count, tpose, value);
}

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
void Shader::setUniformMatrix4fv(GLint pos, GLsizei count, const GLfloat *value, GLboolean tpose) {
	CUAssertLog(isBound(), "Shader is not active.");
	glUniformMatrix4fv(pos, count, tpose, value);
}

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
void Shader::setUniformMatrix4fv(const std::string name, GLsizei count, const GLfloat *value, GLboolean tpose) {
	CUAssertLog(isBound(), "Shader is not active.");
	GLint locale = getUniformLocation(name.c_str());
	if (locale >= 0) glUniformMatrix4fv(locale, count, tpose, value);
}

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
void Shader::setUniformMatrix2x3fv(GLint pos, GLsizei count, const GLfloat *value, GLboolean tpose) {
	CUAssertLog(isBound(), "Shader is not active.");
	glUniformMatrix2x3fv(pos, count, tpose, value);
}

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
void Shader::setUniformMatrix2x3fv(const std::string name, GLsizei count, const GLfloat *value, GLboolean tpose) {
	CUAssertLog(isBound(), "Shader is not active.");
	GLint locale = getUniformLocation(name.c_str());
	if (locale >= 0) glUniformMatrix2x3fv(locale, count, tpose, value);
}

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
void Shader::setUniformMatrix3x2fv(GLint pos, GLsizei count, const GLfloat *value, GLboolean tpose) {
	CUAssertLog(isBound(), "Shader is not active.");
	glUniformMatrix3x2fv(pos, count, tpose, value);
}

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
void Shader::setUniformMatrix3x2fv(const std::string name, GLsizei count, const GLfloat *value, GLboolean tpose) {
	CUAssertLog(isBound(), "Shader is not active.");
	GLint locale = getUniformLocation(name.c_str());
	if (locale >= 0) glUniformMatrix3x2fv(locale, count, tpose, value);
}

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
void Shader::setUniformMatrix2x4fv(GLint pos, GLsizei count, const GLfloat *value, GLboolean tpose) {
	CUAssertLog(isBound(), "Shader is not active.");
	glUniformMatrix2x4fv(pos, count, tpose, value);
}

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
void Shader::setUniformMatrix2x4fv(const std::string name, GLsizei count, const GLfloat *value, GLboolean tpose) {
	CUAssertLog(isBound(), "Shader is not active.");
	GLint locale = getUniformLocation(name.c_str());
	if (locale >= 0) glUniformMatrix2x4fv(locale, count, tpose, value);
}

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
void Shader::setUniformMatrix4x2fv(GLint pos, GLsizei count, const GLfloat *value, GLboolean tpose) {
	CUAssertLog(isBound(), "Shader is not active.");
	glUniformMatrix4x2fv(pos, count, tpose, value);
}

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
void Shader::setUniformMatrix4x2fv(const std::string name, GLsizei count, const GLfloat *value, GLboolean tpose) {
	CUAssertLog(isBound(), "Shader is not active.");
	GLint locale = getUniformLocation(name.c_str());
	if (locale >= 0) glUniformMatrix4x2fv(locale, count, tpose, value);
}

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
void Shader::setUniformMatrix3x4fv(GLint pos, GLsizei count, const GLfloat *value, GLboolean tpose) {
	CUAssertLog(isBound(), "Shader is not active.");
	glUniformMatrix3x4fv(pos, count, tpose, value);
}

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
void Shader::setUniformMatrix3x4fv(const std::string name, GLsizei count, const GLfloat *value, GLboolean tpose) {
	CUAssertLog(isBound(), "Shader is not active.");
	GLint locale = getUniformLocation(name.c_str());
	if (locale >= 0) glUniformMatrix3x4fv(locale, count, tpose, value);
}

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
void Shader::setUniformMatrix4x3fv(GLint pos, GLsizei count, const GLfloat *value, GLboolean tpose) {
	CUAssertLog(isBound(), "Shader is not active.");
	glUniformMatrix4x3fv(pos, count, tpose, value);
}

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
void Shader::setUniformMatrix4x3fv(const std::string name, GLsizei count, const GLfloat *value, GLboolean tpose) {
	CUAssertLog(isBound(), "Shader is not active.");
	GLint locale = getUniformLocation(name.c_str());
    if (locale >= 0) glUniformMatrix4x3fv(locale, count, tpose, value);
}

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
bool Shader::getUniformfv(GLint pos, GLsizei size, GLfloat *value) const {
    CUAssertLog(isBound(), "Shader is not active.");
    glGetUniformfv(_program,pos,value);
    return !(glGetError());
}

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
bool Shader::getUniformfv(const std::string name, GLsizei size, GLfloat *value) const {
    CUAssertLog(isBound(), "Shader is not active.");
    GLint locale = getUniformLocation(name.c_str());
    if (locale >= 0) {
        glGetUniformfv(_program,locale,value);
        return !(glGetError());
    }
    return false;
}

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
bool Shader::getUniformiv(GLint pos, GLsizei size, GLint *value) const {
    CUAssertLog(isBound(), "Shader is not active.");
    glGetUniformiv(_program,pos,value);
    return !(glGetError());
}

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
bool Shader::getUniformiv(const std::string name, GLsizei size, GLint *value) const {
    CUAssertLog(isBound(), "Shader is not active.");
    GLint locale = getUniformLocation(name.c_str());
    if (locale >= 0) {
        glGetUniformiv(_program,locale,value);
        return !(glGetError());
    }
    return false;
}

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
bool Shader::getUniformuiv(GLint pos, GLsizei size, GLuint *value) const {
    CUAssertLog(isBound(), "Shader is not active.");
    glGetUniformuiv(_program,pos,value);
    return !(glGetError());
}

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
bool Shader::getUniformuiv(const std::string name, GLsizei size, GLuint *value) const {
    CUAssertLog(isBound(), "Shader is not active.");
    GLint locale = getUniformLocation(name.c_str());
    if (locale >= 0) {
        glGetUniformuiv(_program,locale,value);
        return !(glGetError());
    }
    return false;
}

