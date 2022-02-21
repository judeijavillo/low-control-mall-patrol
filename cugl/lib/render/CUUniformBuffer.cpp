//
//  CUUniformBuffer.cpp
//  Cornell University Game Library (CUGL)
//
//  This module provides a uniform buffer for sending batched uniforms to a shader.
//  Unlike a vertex buffer, uniform buffers are optional (you can set the uniforms
//  directly in the shader). Uniform buffers are solely a performance optimization.
//  In our tests, they only provide a win when (1) there are a large number of
//  uniforms being passed to the shader, (2) these uniforms can be loaded at the
//  start of the frame, and (3) shader updates are done through offset management.
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
//  Version: 2/29/20
#include <cugl/util/CUDebug.h>
#include <cugl/render/CUUniformBuffer.h>

using namespace cugl;


#pragma mark Constructors
/**
 * Creates an uninitialized uniform buffer.
 *
 * You must initialize the uniform buffer to allocate memory.
 */
UniformBuffer::UniformBuffer() :
_dataBuffer(0),
_blockcount(0),
_blockpntr(0),
_blocksize(0),
_blockstride(0),
_bindpoint(0),
_autoflush(false),
_dirty(false)
{
    _name = "";
    _bytebuffer = nullptr;
    _drawtype = GL_STREAM_DRAW;
}

/**
 * Deletes this uniform buffer, disposing all resources.
 */
UniformBuffer::~UniformBuffer() { dispose(); }

/**
 * Initializes this uniform buffer to support a block of the given capacity.
 *
 * This uniform buffer will only support a single block.  The block capacity is
 * measured in bytes.  In std140 format, all scalars are 4 bytes, vectors are 8
 * or 16 bytes, and matrices are treated as an array of 8 or 16 byte column vectors.
 *
 * The drawtype is GL_STATIC_DRAW, GL_DYNAMIC_DRAW, or GL_STREAM_DRAW. If the
 * uniform values change often, the difference between GL_STATIC_DRAW and
 * GL_STREAM_DRAW is 1-2 orders of magnitude.
 *
 * @param capacity  The block capacity in bytes
 * @param drawtype  One of GL_STATIC_DRAW, GL_DYNAMIC_DRAW, or GL_STREAM_DRAW
 *
 * @return true if initialization was successful.
 */
bool UniformBuffer::init(GLsizei capacity) {
    return init(capacity,1);
}

/**
 * Initializes this uniform buffer to support multiple blocks of the given capacity.
 *
 * The block capacity is measured in bytes.  In std140 format, all scalars are
 * 4 bytes, vectors are 8 or 16 bytes, and matrices are treated as an array of
 * 8 or 16 byte column vectors.
 *
 * Keep in mind that uniform buffer blocks must be aligned, and so this may take
 * significantly more memory than the number of blocks times the capacity. If the
 * graphics card cannot support that many blocks, this method will return false.
 *
 * @param capacity  The block capacity in bytes
 * @param blocks    The number of blocks to support
 *
 * @return true if initialization was successful.
 */
bool UniformBuffer::init(GLsizei capacity, GLuint blocks) {
    CUAssertLog(blocks, "Block count must be nonzero");
    _blockcount = blocks;
    _blocksize = capacity;
    
    GLint value;
    glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &value);
    while (_blockstride < _blocksize) {
        _blockstride += value;
    }
    
    // Quit if the memory request is too high
    glGetIntegerv(GL_MAX_UNIFORM_BLOCK_SIZE, &value);
    if (_blockstride > value) {
        CUAssertLog(false,"Capacity exceeds maximum value of %d bytes",value);
        _blockcount  = 0;
        _blocksize   = 0;
        _blockstride = 0;
        return false;
    }    
    
    GLenum error;
    glGenBuffers(1, &_dataBuffer);
    if (!_dataBuffer) {
        error = glGetError();
        CULogError("Could not create uniform buffer. %s", gl_error_name(error).c_str());
        return false;
    }

    _bytebuffer = (char*)malloc(_blockstride*_blockcount);
    glBindBuffer(GL_UNIFORM_BUFFER, _dataBuffer);
    glBufferData(GL_UNIFORM_BUFFER, _blockstride*_blockcount, NULL, _drawtype);
    error = glGetError();
    if (error) {
        glDeleteBuffers(1, &_dataBuffer);
        _dataBuffer = 0;
        CULogError("Could not allocate memory for uniform buffer. %s",
                   gl_error_name(error).c_str());
        return false;
    }
    
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
    return true;
}

/**
 * Deletes the uniform buffer, freeing all resources.
 *
 * You must reinitialize the uniform buffer to use it.
 */
void UniformBuffer::dispose() {
    if (_dataBuffer) {
        glDeleteBuffers(1,&_dataBuffer);
        _dataBuffer = 0;
    }
    if (_bytebuffer) {
        free(_bytebuffer);
        _bytebuffer = nullptr;
    }
    _name.clear();
    _offsets.clear();
    _blockcount  = 0;
    _blocksize   = 0;
    _blockstride = 0;
    _bindpoint = 0;
}


#pragma mark -
#pragma mark Binding
/**
 * Sets the bind point for this uniform buffer.
 *
 * Uniform buffers and shaders have a many-to-many relationship. This means
 * that connecting them requires an intermediate table. The positions in
 * this table are called bind points. A uniform buffer is associated with
 * a bind point and a shader associates a bind point with a uniform struct.
 * That uniform struct then pulls data from the active block of the uniform
 * buffer. By default this value is 0.
 *
 * The uniform buffer does not need to be active to call this method. This
 * method only sets the bind point preference and does not actually
 * {@link #bind} the buffer. However, if the buffer is bound to another bind
 * point, then it will be unbound from that point.
 *
 * @param point The bind point for for this uniform buffer.
 */
void UniformBuffer::setBindPoint(GLuint point) {
    GLint bound;
    glGetIntegeri_v(GL_UNIFORM_BUFFER_BINDING,_bindpoint,&bound);
    if (bound == _dataBuffer) {
        glBindBufferBase(GL_UNIFORM_BUFFER, _bindpoint, 0);
    }
    _bindpoint = point;
}

/**
 * Binds this uniform buffer to its bind point.
 *
 * Unlike {@link Texture}, it is possible to bind a uniform buffer to its
 * bind point without making it the active uniform buffer. An inactive buffer
 * will still stream data to the shader, though its data cannot be altered
 * without making it active.
 *
 * Binding a buffer to a bind point replaces the uniform block originally
 * there.  So this buffer can be unbound without a call to {@link #unbound}.
 * However, if another buffer is bound to a different bind point than this
 * block, it will not affect this buffer's relationship with the shader.
 *
 * For compatibility reasons with {@link Texture} we allow this method to
 * both bind and activate the uniform buffer in one call.
 *
 * This call is reentrant. If can be safely called multiple times.
 *
 * @param activate  Whether to activate this buffer in addition to binding.
 */
void UniformBuffer::bind(bool activate) {
    if (activate) {
        this->activate();
    }
    glBindBufferBase(GL_UNIFORM_BUFFER, _bindpoint, _dataBuffer);
}

/**
 * Unbinds this uniform buffer disassociating it from its bind point.
 *
 * This call will have no affect on the active buffer (e.g. which buffer is
 * receiving data from the program). This method simply removes this buffer
 * from its bind point.
 *
 * Once unbound, the bind point for this buffer will no longer send data
 * to the appropriate uniform(s) in the shader. In that case the shader will
 * use default values according to the variable types.
 *
 * This call is reentrant.  If can be safely called multiple times.
 */
void UniformBuffer::unbind() {
    GLint bound;
    glGetIntegeri_v(GL_UNIFORM_BUFFER_BINDING,_bindpoint,&bound);
    if (bound == _dataBuffer) {
        glBindBufferBase(GL_UNIFORM_BUFFER, _bindpoint, 0);
    }
}

/**
 * Activates this uniform block so that if can receive data.
 *
 * This method makes this uniform block the active uniform buffer. This means
 * that changes made to the data in this uniform buffer will be pushed to the
 * graphics card. If there were are any pending changes to the uniform buffer
 * (made when it was not active), they will be pushed immediately when this
 * method is called.
 *
 * This method does not bind the uniform block to a bind point. That must be
 * done with a call to {@link #bind}.
 *
 * This call is reentrant.  If can be safely called multiple times.
 */
void UniformBuffer::activate() {
    glBindBuffer(GL_UNIFORM_BUFFER, _dataBuffer);
    if (_autoflush && _dirty) {
        glBufferData(GL_UNIFORM_BUFFER,_blockstride*_blockcount,_bytebuffer,_drawtype);
        _dirty = false;
    }
}

/**
 * Deactivates this uniform block, making it no longer active.
 *
 * This method will not unbind the buffer from its bind point (assuming it is
 * bound to one). It simply means that it is no longer the active uniform buffer
 * and cannot receive new data. Data sent to this buffer will be cached and sent
 * to the graphics card once the buffer is reactivated.  However, the shader will
 * use the current graphics card data until that happens.
 *
 * This call is reentrant.  If can be safely called multiple times.
 */
void UniformBuffer::deactivate() {
#if CU_PLATFORM == CU_PLATFORM_ANDROID
 	// There are problems with this query on emulator
 	glBindBuffer(GL_UNIFORM_BUFFER, 0);
#else
	GLint bound;
    glGetIntegerv(GL_UNIFORM_BUFFER_BINDING,&bound);
    if (bound == _dataBuffer) {
        glBindBuffer(GL_UNIFORM_BUFFER, 0);
    }
#endif
}

/**
 * Returns true if this uniform buffer is currently bound.
 *
 * A uniform buffer is bound if it is attached to a bind point. That means that
 * the shader will pull its data for that bind point from this buffer. A uniform
 * block can be bound without being active.
 *
 * @return true if this uniform block is currently bound.
 */
bool UniformBuffer::isBound() const {
    GLint bound;
    glGetIntegeri_v(GL_UNIFORM_BUFFER_BINDING,_bindpoint,&bound);
    return bound == _dataBuffer;
}
    
/**
 * Returns true if this uniform buffer is currently active.
 *
 * An active uniform block is the one that pushes changes in data directly to
 * the graphics card. If the buffer is not active, then many of the setter
 * methods in this class will cache changes but delay applying them until the
 * buffer is reactivated.
 *
 * Unlike {@link Texture}, it is possible for a uniform buffer to be active but
 * not bound.
 *
 * @return true if this uniform block is currently active.
 */
bool UniformBuffer::isActive() const {
    GLint bound;
    glGetIntegerv(GL_UNIFORM_BUFFER_BINDING,&bound);
    return bound == _dataBuffer;
}

/**
 * Sets the active uniform block in this buffer.
 *
 * The active uniform block is the block from which the shader will pull
 * uniform values. This value can only be altered if this buffer is bound
 * (though it need not be active).
 *
 * @param block The active uniform block in this buffer.
 */
void UniformBuffer::setBlock(GLuint block) {
    CUAssertLog(isBound(), "Buffer is not bound.");
    if (_blockpntr != block) {
        _blockpntr = block;
        glBindBufferRange(GL_UNIFORM_BUFFER,_bindpoint,_dataBuffer,
                          block*_blockstride,_blocksize);
    }
}

/**
 * Flushes any changes in the backing byte buffer to the graphics card.
 *
 * This method is only necessary if the user has accessed the backing byte
 * buffer directly via {@link #getData} and needs to push these changes to the
 * graphics card.  Calling this method will not affect the active uniform
 * buffer.
 */
void UniformBuffer::flush() {
    // CUAssertLog(isActive(), "Buffer is not active."); // Problems on android emulator for now
    glBufferData(GL_UNIFORM_BUFFER,_blockstride*_blockcount,_bytebuffer,_drawtype);
    _dirty = false;
}



#pragma mark -
#pragma mark Data Offsets
/**
 * Defines the byte offset of the given buffer variable.
 *
 * It is not necessary to call this method to use the uniform buffer. It is
 * always possible to pass data to the uniform block by specifying the byte
 * offset.  The shader uses byte offsets to pull data from the uniform buffer
 * and assign it to the appropriate struct variable.
 *
 * However, this method makes use of the uniform buffer easier to follow. It
 * explicitly assigns a variable name to a byte offset. This variable name
 * can now be used in place of the byte offset with passing data to this
 * uniform block.
 *
 * Use of this method does not require the uniform buffer to be bound or
 * even active.
 *
 * @param name      The variable name to use for this offset
 * @param offset    The buffer offset in bytes
 */
void UniformBuffer::setOffset(const std::string name, GLsizei offset) {
    _offsets[name] = offset;
}

/**
 * Returns the byte offset for the given name.
 *
 * This method requires that name be previously associated with an offset
 * via {@link #setOffset}. If it has not been associated with an offset,
 * then this method will return {@link #INVALID_OFFSET} instead.
 *
 * @param name      The variable name to query for an offset
 *
 * @return the byte offset of the given struct variable.
 */
GLsizei UniformBuffer::getOffset(const std::string name) const {
    auto elt = _offsets.find(name);
    if (elt == _offsets.end()) {
        return INVALID_OFFSET;
    }
    return elt->second;
}

/**
 * Returns the offsets defined for this buffer
 *
 * The vector returned will include the name of every variable set by
 * the method {@link #setOffset}.
 *
 * @return the offsets defined for this buffer
 */
std::vector<std::string> UniformBuffer::getOffsets() const {
    std::vector<std::string> result;
    for(auto it = _offsets.begin(); it != _offsets.end(); ++it) {
        result.push_back(it->first);
    }
    return result;
}

/** The byte position of an invalid offset */
GLsizei UniformBuffer::INVALID_OFFSET = ((GLsizei)-1);


#pragma mark -
#pragma mark CUGL Uniforms
/**
 * Sets the given uniform variable to a vector value.
 *
 * This method will write the vector as 2*sizeof(float) bytes to the appropriate
 * buffer location (and the buffer must have the appropriate capacity).
 *
 * Values set by this method will not be sent to the graphics card until the 
 * buffer is flushed. However, if the buffer is active and auto-flush is turned
 * on, it will be written immediately.
 *
 * If block is -1, it sets this value in every block in this uniform buffer.
 * This is a potentially expensive operation if the block is active.  For
 * mass changes, it is better to deactivate the buffer, and have them apply
 * once the buffer is reactivated.
 *
 * @param block     The block in this uniform buffer to access
 * @param offset    The offset within the block
 * @param vec       The value for the uniform
 */
void UniformBuffer::setUniformVec2(GLint block, GLsizei offset, const Vec2 vec) {
    const float* data = reinterpret_cast<const float*>(&vec);
    setUniformfv(block,offset,2,data);
}

/**
 * Sets the given uniform variable to a vector value.
 *
 * This method requires that the uniform name be previously bound to a byte
 * offset with the call {@link #setOffset}. This method will write the vector
 * as 2*sizeof(float) bytes to the appropriate buffer location (and the buffer
 * must have the appropriate capacity).
 *
 * Values set by this method will not be sent to the graphics card until the 
 * buffer is flushed. However, if the buffer is active and auto-flush is turned
 * on, it will be written immediately.
 *
 * If block is -1, it sets this value in every block in this uniform buffer.
 * This is a potentially expensive operation if the block is active.  For
 * mass changes, it is better to deactivate the buffer, and have them apply
 * once the buffer is reactivated.
 *
 * @param block The block in this uniform buffer to access
 * @param name  The name of the uniform variable
 * @param vec   The value for the uniform
 */
void UniformBuffer::setUniformVec2(GLint block, const std::string name, const Vec2 vec) {
    const float* data = reinterpret_cast<const float*>(&vec);
    setUniformfv(block,name,2,data);
}

/**
 * Returns true if it can access the given uniform variable as a vector.
 *
 * This method will read the vector as 2*sizeof(float) bytes to the appropriate
 * buffer location (and the buffer must have the appropriate capacity).
 *
 * The buffer does not have to be active to call this method.  If it is not
 * active and there are pending changes to this uniform variable, this method
 * will read those changes and not the current value in the graphics card.
 *
 * @param block     The block in this uniform buffer to access
 * @param offset    The offset within the block
 * @param vec       The vector to store the result
 *
 * @return true if it can access the given uniform variable as a vector.
 */
bool UniformBuffer::getUniformVec2(GLuint block, GLsizei offset, Vec2& vec) const {
    float* data = reinterpret_cast<float*>(&vec);
    return getUniformfv(block,offset,2,data);
}

/**
 * Returns true if it can access the given uniform variable as a vector.
 *
 * This method requires that the uniform name be previously bound to a byte
 * offset with the call {@link #setOffset}. This method will read the vector
 * as 2*sizeof(float) bytes to the appropriate buffer location (and the buffer
 * must have the appropriate capacity).
 *
 * The buffer does not have to be active to call this method.  If it is not
 * active and there are pending changes to this uniform variable, this method
 * will read those changes and not the current value in the graphics card.
 *
 * @param block The block in this uniform buffer to access
 * @param name  The name of the uniform variable
 * @param vec   The vector to store the result
 *
 * @return true if it can access the given uniform variable as a vector.
 */
bool UniformBuffer::getUniformVec2(GLuint block, const std::string name, Vec2& vec) const {
    float* data = reinterpret_cast<float*>(&vec);
    return getUniformfv(block,name,2,data);
}

/**
 * Sets the given uniform variable to a vector value.
 *
 * This method will write the vector as 3*sizeof(float) bytes to the appropriate
 * buffer location (and the buffer must have the appropriate capacity).
 *
 * Values set by this method will not be sent to the graphics card until the 
 * buffer is flushed. However, if the buffer is active and auto-flush is turned
 * on, it will be written immediately.
 *
 * If block is -1, it sets this value in every block in this uniform buffer.
 * This is a potentially expensive operation if the block is active.  For
 * mass changes, it is better to deactivate the buffer, and have them apply
 * once the buffer is reactivated.
 *
 * @param block     The block in this uniform buffer to access
 * @param offset    The offset within the block
 * @param vec       The value for the uniform
 */
void UniformBuffer::setUniformVec3(GLint block, GLsizei offset, const Vec3 vec) {
    const float* data = reinterpret_cast<const float*>(&vec);
    setUniformfv(block,offset,3,data);
}

/**
 * Sets the given uniform variable to a vector value.
 *
 * This method requires that the uniform name be previously bound to a byte
 * offset with the call {@link #setOffset}. This method will write the vector
 * as 3*sizeof(float) bytes to the appropriate buffer location (and the buffer
 * must have the appropriate capacity).
 *
 * Values set by this method will not be sent to the graphics card until the 
 * buffer is flushed. However, if the buffer is active and auto-flush is turned
 * on, it will be written immediately.
 *
 * If block is -1, it sets this value in every block in this uniform buffer.
 * This is a potentially expensive operation if the block is active.  For
 * mass changes, it is better to deactivate the buffer, and have them apply
 * once the buffer is reactivated.
 *
 * @param block The block in this uniform buffer to access
 * @param name  The name of the uniform variable
 * @param vec   The value for the uniform
 */
void UniformBuffer::setUniformVec3(GLint block, const std::string name, const Vec3 vec) {
    const float* data = reinterpret_cast<const float*>(&vec);
    setUniformfv(block,name,3,data);
}

/**
 * Returns true if it can access the given uniform variable as a vector.
 *
 * This method will read the vector as 3*sizeof(float) bytes to the appropriate
 * buffer location (and the buffer must have the appropriate capacity).
 *
 * The buffer does not have to be active to call this method.  If it is not
 * active and there are pending changes to this uniform variable, this method
 * will read those changes and not the current value in the graphics card.
 *
 * @param block     The block in this uniform buffer to access
 * @param offset    The offset within the block
 * @param vec       The vector to store the result
 *
 * @return true if it can access the given uniform variable as a vector.
 */
bool UniformBuffer::getUniformVec3(GLuint block, GLsizei offset, Vec3& vec) const {
    float* data = reinterpret_cast<float*>(&vec);
    return getUniformfv(block,offset,3,data);
}

/**
 * Returns true if it can access the given uniform variable as a vector.
 *
 * This method requires that the uniform name be previously bound to a byte
 * offset with the call {@link #setOffset}. This method will read the vector
 * as 3*sizeof(float) bytes to the appropriate buffer location (and the buffer
 * must have the appropriate capacity).
 *
 * The buffer does not have to be active to call this method.  If it is not
 * active and there are pending changes to this uniform variable, this method
 * will read those changes and not the current value in the graphics card.
 *
 * @param block The block in this uniform buffer to access
 * @param name  The name of the uniform variable
 * @param vec   The vector to store the result
 *
 * @return true if it can access the given uniform variable as a vector.
 */
bool UniformBuffer::getUniformVec3(GLuint block, const std::string name, Vec3& vec) const {
    float* data = reinterpret_cast<float*>(&vec);
    return getUniformfv(block,name,3,data);
}

/**
 * Sets the given uniform variable to a vector value.
 *
 * This method will write the vector as 4*sizeof(float) bytes to the appropriate
 * buffer location (and the buffer must have the appropriate capacity).
 *
 * Values set by this method will not be sent to the graphics card until the 
 * buffer is flushed. However, if the buffer is active and auto-flush is turned
 * on, it will be written immediately.
 *
 * If block is -1, it sets this value in every block in this uniform buffer.
 * This is a potentially expensive operation if the block is active.  For
 * mass changes, it is better to deactivate the buffer, and have them apply
 * once the buffer is reactivated.
 *
 * @param block     The block in this uniform buffer to access
 * @param offset    The offset within the block
 * @param vec       The value for the uniform
 */
void UniformBuffer::setUniformVec4(GLint block, GLsizei offset, const Vec4 vec) {
    const float* data = reinterpret_cast<const float*>(&vec);
    setUniformfv(block,offset,4,data);
}

/**
 * Sets the given uniform variable to a vector value.
 *
 * This method requires that the uniform name be previously bound to a byte
 * offset with the call {@link #setOffset}. This method will write the vector
 * as 4*sizeof(float) bytes to the appropriate buffer location (and the buffer
 * must have the appropriate capacity).
 *
 * Values set by this method will not be sent to the graphics card until the 
 * buffer is flushed. However, if the buffer is active and auto-flush is turned
 * on, it will be written immediately.
 *
 * If block is -1, it sets this value in every block in this uniform buffer.
 * This is a potentially expensive operation if the block is active.  For
 * mass changes, it is better to deactivate the buffer, and have them apply
 * once the buffer is reactivated.
 *
 * @param block The block in this uniform buffer to access
 * @param name  The name of the uniform variable
 * @param vec   The value for the uniform
 */
void UniformBuffer::setUniformVec4(GLint block, const std::string name, const Vec4 vec) {
    const float* data = reinterpret_cast<const float*>(&vec);
    setUniformfv(block,name,4,data);
}


/**
 * Returns true if it can access the given uniform variable as a vector.
 *
 * This method will read the vector as 4*sizeof(float) bytes to the appropriate
 * buffer location (and the buffer must have the appropriate capacity).
 *
 * The buffer does not have to be active to call this method.  If it is not
 * active and there are pending changes to this uniform variable, this method
 * will read those changes and not the current value in the graphics card.
 *
 * @param block     The block in this uniform buffer to access
 * @param offset    The offset within the block
 * @param vec       The vector to store the result
 *
 * @return true if it can access the given uniform variable as a vector.
 */
bool UniformBuffer::getUniformVec4(GLuint block, GLsizei offset, Vec4& vec) const {
    float* data = reinterpret_cast<float*>(&vec);
    return getUniformfv(block,offset,4,data);
}

/**
 * Returns true if it can access the given uniform variable as a vector.
 *
 * This method requires that the uniform name be previously bound to a byte
 * offset with the call {@link #setOffset}. This method will read the vector
 * as 4*sizeof(float) bytes to the appropriate buffer location (and the buffer
 * must have the appropriate capacity).
 *
 * The buffer does not have to be active to call this method.  If it is not
 * active and there are pending changes to this uniform variable, this method
 * will read those changes and not the current value in the graphics card.
 *
 * @param block The block in this uniform buffer to access
 * @param name  The name of the uniform variable
 * @param vec   The vector to store the result
 *
 * @return true if it can access the given uniform variable as a vector.
 */
bool UniformBuffer::getUniformVec4(GLuint block, const std::string name, Vec4& vec) const {
    float* data = reinterpret_cast<float*>(&vec);
    return getUniformfv(block,name,4,data);
}

/**
 * Sets the given uniform variable to a color value.
 *
 * This method will write the color as 4*sizeof(float) bytes to the appropriate
 * buffer location (and the buffer must have the appropriate capacity).
 *
 * Values set by this method will not be sent to the graphics card until the 
 * buffer is flushed. However, if the buffer is active and auto-flush is turned
 * on, it will be written immediately.
 *
 * If block is -1, it sets this value in every block in this uniform buffer.
 * This is a potentially expensive operation if the block is active.  For
 * mass changes, it is better to deactivate the buffer, and have them apply
 * once the buffer is reactivated.
 *
 * @param block     The block in this uniform buffer to access
 * @param offset    The offset within the block
 * @param color     The value for the uniform
 */
void UniformBuffer::setUniformColor4(GLint block, GLsizei offset, const Color4 color) {
    const Color4f fcolor = (Color4f)color;
    setUniformColor4f(block, offset, fcolor);
}

/**
 * Sets the given uniform variable to a color value.
 *
 * This method requires that the uniform name be previously bound to a byte
 * offset with the call {@link #setOffset}. This method will write the color
 * as 4*sizeof(float) bytes to the appropriate buffer location (and the buffer
 * must have the appropriate capacity).
 *
 * Values set by this method will not be sent to the graphics card until the 
 * buffer is flushed. However, if the buffer is active and auto-flush is turned
 * on, it will be written immediately.
 *
 * If block is -1, it sets this value in every block in this uniform buffer.
 * This is a potentially expensive operation if the block is active.  For
 * mass changes, it is better to deactivate the buffer, and have them apply
 * once the buffer is reactivated.
 *
 * @param block The block in this uniform buffer to access
 * @param name  The name of the uniform variable
 * @param color The value for the uniform
 */
void UniformBuffer::setUniformColor4(GLint block, const std::string name, const Color4 color) {
    const Color4f fcolor = (Color4f)color;
    setUniformColor4f(block, name, fcolor);
}

/**
 * Returns true if it can access the given uniform variable as a color.
 *
 * This method will read the color as 4*sizeof(float) bytes to the appropriate
 * buffer location (and the buffer must have the appropriate capacity).
 *
 * The buffer does not have to be active to call this method.  If it is not
 * active and there are pending changes to this uniform variable, this method
 * will read those changes and not the current value in the graphics card.
 *
 * @param block     The block in this uniform buffer to access
 * @param offset    The offset within the block
 * @param color     The color to store the result
 *
 * @return true if it can access the given uniform variable as a color.
 */
bool UniformBuffer::getUniformColor4(GLuint block, GLsizei offset, Color4& color) const {
    float data[4];
    if (getUniformfv(block,offset,4,data)) {
        color.set(data);
        return true;
    }
    return false;
}

/**
 * Returns true if it can access the given uniform variable as a color.
 *
 * This method requires that the uniform name be previously bound to a byte
 * offset with the call {@link #setOffset}. This method will read the color
 * as 4*sizeof(float) bytes to the appropriate buffer location (and the buffer
 * must have the appropriate capacity).
 *
 * The buffer does not have to be active to call this method.  If it is not
 * active and there are pending changes to this uniform variable, this method
 * will read those changes and not the current value in the graphics card.
 *
 * @param block The block in this uniform buffer to access
 * @param name  The name of the uniform variable
 * @param color The color to store the result
 *
 * @return true if it can access the given uniform variable as a vector.
 */
bool UniformBuffer::getUniformColor4(GLuint block, const std::string name, Color4& color) const {
    float data[4];
    if (getUniformfv(block,name,4,data)) {
        color.set(data);
        return true;
    }
    return false;
}

/**
 * Sets the given uniform variable to a color value.
 *
 * This method will write the color as 4*sizeof(float) bytes to the appropriate
 * buffer location (and the buffer must have the appropriate capacity).
 *
 * Values set by this method will not be sent to the graphics card until the 
 * buffer is flushed. However, if the buffer is active and auto-flush is turned
 * on, it will be written immediately.
 *
 * If block is -1, it sets this value in every block in this uniform buffer.
 * This is a potentially expensive operation if the block is active.  For
 * mass changes, it is better to deactivate the buffer, and have them apply
 * once the buffer is reactivated.
 *
 * @param block     The block in this uniform buffer to access
 * @param offset    The offset within the block
 * @param color     The value for the uniform
 */
void UniformBuffer::setUniformColor4f(GLint block, GLsizei offset, const Color4f color) {
    const float* data = reinterpret_cast<const float*>(&color);
    setUniformfv(block,offset,4,data);
}

/**
 * Sets the given uniform variable to a color value.
 *
 * This method requires that the uniform name be previously bound to a byte
 * offset with the call {@link #setOffset}. This method will write the color
 * as 4*sizeof(float) bytes to the appropriate buffer location (and the buffer
 * must have the appropriate capacity).
 *
 * Values set by this method will not be sent to the graphics card until the 
 * buffer is flushed. However, if the buffer is active and auto-flush is turned
 * on, it will be written immediately.
 *
 * If block is -1, it sets this value in every block in this uniform buffer.
 * This is a potentially expensive operation if the block is active.  For
 * mass changes, it is better to deactivate the buffer, and have them apply
 * once the buffer is reactivated.
 *
 * @param block The block in this uniform buffer to access
 * @param name  The name of the uniform variable
 * @param color The value for the uniform
 */
void UniformBuffer::setUniformColor4f(GLint block, const std::string name, const Color4f color) {
    const float* data = reinterpret_cast<const float*>(&color);
    setUniformfv(block,name,4,data);
}

/**
 * Returns true if it can access the given uniform variable as a color.
 *
 * This method will read the color as 4*sizeof(float) bytes to the appropriate
 * buffer location (and the buffer must have the appropriate capacity).
 *
 * The buffer does not have to be active to call this method.  If it is not
 * active and there are pending changes to this uniform variable, this method
 * will read those changes and not the current value in the graphics card.
 *
 * @param block     The block in this uniform buffer to access
 * @param offset    The offset within the block
 * @param color     The color to store the result
 *
 * @return true if it can access the given uniform variable as a color.
 */
bool UniformBuffer::getUniformColor4f(GLuint block, GLsizei offset, Color4f& color) const {
    float* data = reinterpret_cast<float*>(&color);
    return getUniformfv(block,offset,4,data);
}

/**
 * Returns true if it can access the given uniform variable as a color.
 *
 * This method requires that the uniform name be previously bound to a byte
 * offset with the call {@link #setOffset}. This method will read the color
 * as 4*sizeof(float) bytes to the appropriate buffer location (and the buffer
 * must have the appropriate capacity).
 *
 * The buffer does not have to be active to call this method.  If it is not
 * active and there are pending changes to this uniform variable, this method
 * will read those changes and not the current value in the graphics card.
 *
 * @param block The block in this uniform buffer to access
 * @param name  The name of the uniform variable
 * @param color The color to store the result
 *
 * @return true if it can access the given uniform variable as a color.
 */
bool UniformBuffer::getUniformColor4f(GLuint block, const std::string name, Color4f& color) const {
    float* data = reinterpret_cast<float*>(&color);
    return getUniformfv(block,name,4,data);
}

/**
 * Sets the given uniform variable to a matrix value.
 *
 * This method will write the matrix as 16*sizeof(float) bytes to the appropriate
 * buffer location (and the buffer must have the appropriate capacity).
 *
 * Values set by this method will not be sent to the graphics card until the 
 * buffer is flushed. However, if the buffer is active and auto-flush is turned
 * on, it will be written immediately.
 *
 * If block is -1, it sets this value in every block in this uniform buffer.
 * This is a potentially expensive operation if the block is active.  For
 * mass changes, it is better to deactivate the buffer, and have them apply
 * once the buffer is reactivated.
 *
 * @param block     The block in this uniform buffer to access
 * @param offset    The offset within the block
 * @param mat       The value for the uniform
 */
void UniformBuffer::setUniformMat4(GLint block, GLsizei offset, const Mat4& mat) {
    setUniformfv(block, offset, 16, mat.m);
}

/**
 * Sets the given uniform variable to a matrix value.
 *
 * This method requires that the uniform name be previously bound to a byte
 * offset with the call {@link #setOffset}. This method will write the matrix
 * as 16*sizeof(float) bytes to the appropriate buffer location (and the buffer
 * must have the appropriate capacity).
 *
 * Values set by this method will not be sent to the graphics card until the 
 * buffer is flushed. However, if the buffer is active and auto-flush is turned
 * on, it will be written immediately.
 *
 * If block is -1, it sets this value in every block in this uniform buffer.
 * This is a potentially expensive operation if the block is active.  For
 * mass changes, it is better to deactivate the buffer, and have them apply
 * once the buffer is reactivated.
 *
 * @param block The block in this uniform buffer to access
 * @param name  The name of the uniform variable
 * @param mat   The value for the uniform
 */
void UniformBuffer::setUniformMat4(GLint block, const std::string name, const Mat4& mat) {
    setUniformfv(block, name, 16, mat.m);
}

/**
 * Returns true if it can access the given uniform variable as a matrix.
 *
 * This method will read the matrix as 16*sizeof(float) bytes to the appropriate
 * buffer location (and the buffer must have the appropriate capacity).
 *
 * The buffer does not have to be active to call this method.  If it is not
 * active and there are pending changes to this uniform variable, this method
 * will read those changes and not the current value in the graphics card.
 *
 * @param block     The block in this uniform buffer to access
 * @param offset    The offset within the block
 * @param mat       The matrix to store the result
 *
 * @return true if it can access the given uniform variable as a matrix.
 */
bool UniformBuffer::getUniformMat4(GLuint block, GLsizei offset, Mat4& mat) const {
    return (getUniformfv(block, offset, 16, mat.m));
}

/**
 * Returns true if it can access the given uniform variable as a matrix.
 *
 * This method requires that the uniform name be previously bound to a byte
 * offset with the call {@link #setOffset}. This method will read the matrix
 * as 16*sizeof(float) bytes to the appropriate buffer location (and the buffer
 * must have the appropriate capacity).
 *
 * The buffer does not have to be active to call this method.  If it is not
 * active and there are pending changes to this uniform variable, this method
 * will read those changes and not the current value in the graphics card.
 *
 * @param block The block in this uniform buffer to access
 * @param name  The name of the uniform variable
 * @param mat   The matrix to store the result
 *
 * @return true if it can access the given uniform variable as a matrix.
 */
bool UniformBuffer::getUniformMat4(GLuint block, const std::string name, Mat4& mat) const {
    return (getUniformfv(block, name, 16, mat.m));
}

/**
 * Sets the given uniform variable to an affine transform.
 *
 * Affine transforms are passed to a uniform block as a 4x3 matrix on
 * homogenous coordinates. That is because the columns must be 4*sizeof(float)
 * bytes for alignment reasons. The buffer must have 12*sizeof(float) bytes
 * available for this write.
 *
 * Values set by this method will not be sent to the graphics card until the 
 * buffer is flushed. However, if the buffer is active and auto-flush is turned
 * on, it will be written immediately.
 *
 * If block is -1, it sets this value in every block in this uniform buffer.
 * This is a potentially expensive operation if the block is active.  For
 * mass changes, it is better to deactivate the buffer, and have them apply
 * once the buffer is reactivated.
 *
 * @param block     The block in this uniform buffer to access
 * @param offset    The offset within the block
 * @param mat       The value for the uniform
 */
void UniformBuffer::setUniformAffine2(GLint block, GLsizei offset, const Affine2& mat) {
    float data[12];
    mat.get3x4(data);
    setUniformfv(block, offset, 12, data);
}

/**
 * Sets the given uniform variable to an affine transform.
 *
 * Affine transforms are passed to a uniform block as a 4x3 matrix on
 * homogenous coordinates. That is because the columns must be 4*sizeof(float)
 * bytes for alignment reasons. The buffer must have 12*sizeof(float) bytes
 * available for this write.
 *
 * This method requires that the uniform name be previously bound to a byte 
 * offset with the call {@link #setOffset}.  Values set by this method will not 
 * be sent to the graphics card until the buffer is flushed. However, if the 
 * buffer is active and auto-flush is turned on, it will be written immediately.
 *
 * If block is -1, it sets this value in every block in this uniform buffer.
 * This is a potentially expensive operation if the block is active.  For
 * mass changes, it is better to deactivate the buffer, and have them apply
 * once the buffer is reactivated.
 *
 * @param block The block in this uniform buffer to access
 * @param name  The name of the uniform variable
 * @param mat   The value for the uniform
 */
void UniformBuffer::setUniformAffine2(GLint block, const std::string name, const Affine2& mat) {
    float data[12];
    mat.get3x4(data);
    setUniformfv(block, name, 12, data);
}

/**
 * Returns true if it can access the given uniform variable as an affine transform.
 *
 * Affine transforms are read from a uniform block as a 4x3 matrix on
 * homogenous coordinates. That is because the columns must be 4*sizeof(float)
 * bytes for alignment reasons. The buffer must have 12*sizeof(float) bytes
 * available for this read.
 *
 * The buffer does not have to be active to call this method.  If it is not
 * active and there are pending changes to this uniform variable, this method
 * will read those changes and not the current value in the graphics card.
 *
 * @param block     The block in this uniform buffer to access
 * @param offset    The offset within the block
 * @param mat       The matrix to store the result
 *
 * @return true if it can access the given uniform variable as an affine transform.
 */
bool UniformBuffer::getUniformAffine2(GLuint block, GLsizei offset, Affine2& mat) const {
    float data[12];
    if (getUniformfv(block, offset, 12, data)) {
        mat.set(data,4);
        return true;
    }
    return false;
}

/**
 * Returns true if it can access the given uniform variable as an affine transform.
 *
 * Affine transforms are read from a uniform block as a 4x3 matrix on
 * homogenous coordinates. That is because the columns must be 4*sizeof(float)
 * bytes for alignment reasons. The buffer must have 12*sizeof(float) bytes
 * available for this read.
 *
 * This method requires that the uniform name be previously bound to a byte
 * offset with the call {@link #setOffset}. The buffer does not have to be active
 * to call this method.  If it is not active and there are pending changes to
 * this uniform variable, this method will read those changes and not the current
 * value in the graphics card.
 *
 * @param block     The block in this uniform buffer to access
 * @param offset    The offset within the block
 * @param mat       The matrix to store the result
 *
 * @return true if it can access the given uniform variable as an affine transform.
 */
bool UniformBuffer::getUniformAffine2(GLuint block, const std::string name, Affine2& mat) const {
    float data[12];
    if (getUniformfv(block, name, 12, data)) {
        mat.set(data,4);
        return true;
    }
    return false;
}

/**
 * Sets the given uniform variable to a quaternion.
 *
 * This method will write the quaternion as 4*sizeof(float) bytes to the appropriate
 * buffer location (and the buffer must have the appropriate capacity).
 *
 * Values set by this method will not be sent to the graphics card until the 
 * buffer is flushed. However, if the buffer is active and auto-flush is turned
 * on, it will be written immediately.
 *
 * If block is -1, it sets this value in every block in this uniform buffer.
 * This is a potentially expensive operation if the block is active.  For
 * mass changes, it is better to deactivate the buffer, and have them apply
 * once the buffer is reactivated.
 *
 * @param block     The block in this uniform buffer to access
 * @param offset    The offset within the block
 * @param quat      The value for the uniform
 */
void UniformBuffer::setUniformQuaternion(GLint block, GLsizei offset, const Quaternion& quat) {
    const float* data = reinterpret_cast<const float*>(&quat);
    setUniformfv(block, offset, 4, data);
}


/**
 * Sets the given uniform variable to a quaternion
 *
 * This method requires that the uniform name be previously bound to a byte
 * offset with the call {@link #setOffset}. This method will write the quaternion
 * as 4*sizeof(float) bytes to the appropriate buffer location (and the buffer
 * must have the appropriate capacity).
 *
 * Values set by this method will not be sent to the graphics card until the 
 * buffer is flushed. However, if the buffer is active and auto-flush is turned
 * on, it will be written immediately.
 *
 * If block is -1, it sets this value in every block in this uniform buffer.
 * This is a potentially expensive operation if the block is active.  For
 * mass changes, it is better to deactivate the buffer, and have them apply
 * once the buffer is reactivated.
 *
 * @param block The block in this uniform buffer to access
 * @param name  The name of the uniform variable
 * @param quat  The value for the uniform
 */
void UniformBuffer::setUniformQuaternion(GLint block, const std::string name, const Quaternion& quat) {
    const float* data = reinterpret_cast<const float*>(&quat);
    setUniformfv(block, name, 4, data);
}


/**
 * Returns true if it can access the given uniform variable as a quaternion.
 *
 * This method will read the quaternion as 4*sizeof(float) bytes to the appropriate
 * buffer location (and the buffer must have the appropriate capacity).
 *
 * The buffer does not have to be active to call this method.  If it is not
 * active and there are pending changes to this uniform variable, this method
 * will read those changes and not the current value in the graphics card.
 *
 * @param block     The block in this uniform buffer to access
 * @param offset    The offset within the block
 * @param quat      The quaternion to store the result
 *
 * @return true if it can access the given uniform variable as a quaternion.
 */
bool UniformBuffer::getUniformQuaternion(GLuint block, GLsizei offset, Quaternion& quat) const {
    float* data = reinterpret_cast<float*>(&quat);
    return getUniformfv(block,offset,4,data);
}

/**
 * Returns true if it can access the given uniform variable as a quaternion.
 *
 * This method requires that the uniform name be previously bound to a byte
 * offset with the call {@link #setOffset}. This method will read the quaternion
 * as 4*sizeof(float) bytes to the appropriate buffer location (and the buffer
 * must have the appropriate capacity).
 *
 * The buffer does not have to be active to call this method.  If it is not
 * active and there are pending changes to this uniform variable, this method
 * will read those changes and not the current value in the graphics card.
 *
 * @param block The block in this uniform buffer to access
 * @param name  The name of the uniform variable
 * @param quat  The quaternion to store the result
 *
 * @return true if it can access the given uniform variable as a quaternion.
 */
bool UniformBuffer::getUniformQuaternion(GLuint block, const std::string name, Quaternion& quat) const {
    float* data = reinterpret_cast<float*>(&quat);
    return getUniformfv(block,name,4,data);
}


#pragma mark -
#pragma mark Legacy Uniforms
/**
 * Sets the given buffer offset to an array of float values
 *
 * Values set by this method will not be sent to the graphics card until the 
 * buffer is flushed. However, if the buffer is active and auto-flush is turned
 * on, it will be written immediately.
 *
 * If block is -1, it sets this value in every block in this uniform buffer.
 * This is a potentially expensive operation if the block is active.  For
 * mass changes, it is better to deactivate the buffer, and have them apply
 * once the buffer is reactivated.
 *
 * @param block     The block in this uniform buffer to access
 * @param offset    The offset within the block
 * @param size      The number of values to write to the buffer
 * @param values    The values to write
 */
void UniformBuffer::setUniformfv(GLint block, GLsizei offset, GLsizei size, const GLfloat *values) {
    CUAssertLog(block < (GLint)_blockcount, "Block %d is invalid.",block);
    CUAssertLog(offset < _blocksize, "Offset %d is invalid.",offset);
    if (block >= 0) {
        GLsizei position = block*_blockstride+offset;
        std::memcpy(_bytebuffer+position, values, size*sizeof(float));
        if (_autoflush && isActive()) {
            glBufferSubData(GL_UNIFORM_BUFFER, position, size*sizeof(float), values);
        } else {
            _dirty = true;
        }
    } else {
        bool active = false;
        if (_autoflush && isActive()) {
            active = true;
        } else {
            _dirty = true;
        }
        for(int block = 0; block < _blockcount; block++) {
            GLsizei position = block*_blockstride+offset;
            std::memcpy(_bytebuffer+position, values, size*sizeof(float));
            if (active) {
                glBufferSubData(GL_UNIFORM_BUFFER, position, size*sizeof(float), values);
            }
        }
    }
}

/**
 * Sets the given buffer location to an array of float values
 *
 * This method requires that the uniform name be previously bound to a byte offset 
 * with the call {@link #setOffset}. Values set by this method will not be sent to 
 * the graphics card until the buffer is flushed. However, if the buffer is active 
 * and auto-flush is turned on, it will be written immediately.
 *
 * If block is -1, it sets this value in every block in this uniform buffer.
 * This is a potentially expensive operation if the block is active.  For
 * mass changes, it is better to deactivate the buffer, and have them apply
 * once the buffer is reactivated.
 *
 * @param block     The block in this uniform buffer to access
 * @param name      The name of the uniform variable
 * @param size      The number of values to write to the buffer
 * @param values    The values to write
 */
void UniformBuffer::setUniformfv(GLint block, const std::string name, GLsizei size, const GLfloat *values) {
    GLsizei offset = getOffset(name);
    if (offset != INVALID_OFFSET) {
        setUniformfv(block,offset,size,values);
    }
}

/**
 * Returns true if it can access the given buffer offset as an array of floats
 *
 * The buffer does not have to be active to call this method.  If it is not
 * active and there are pending changes to this uniform variable, this method
 * will read those changes and not the current value in the graphics card.
 *
 * @param block     The block in this uniform buffer to access
 * @param offset    The offset within the block
 * @param size      The available size of the value array
 * @param values    The array to receive the values
 *
 * @return true if data was successfully read into values
 */
bool UniformBuffer::getUniformfv(GLuint block, GLsizei offset, GLsizei size, GLfloat *values) const {
    if (block >= _blockcount || offset > _blocksize) {
        return false;
    }
    GLsizei position = block*_blockstride+offset;
    std::memcpy(values, _bytebuffer+position, size*sizeof(float));
    return true;
}

/**
 * Returns true if it can access the given buffer location as an array of floats
 *
 * This method requires that the uniform name be previously bound to a byte offset
 * with the call {@link #setOffset}. The buffer does not have to be active to call
 * this method. If it is not active and there are pending changes to this uniform
 * variable, this method will read those changes and not the current value in the
 * graphics card.
 *
 * @param block     The block in this uniform buffer to access
 * @param name      The name of the uniform variable
 * @param size      The available size of the value array
 * @param values    The array to receive the values
 *
 * @return true if data was successfully read into values
 */
bool UniformBuffer::getUniformfv(GLuint block, const std::string name, GLsizei size, GLfloat *values) const {
    GLsizei offset = getOffset(name);
    if (offset != INVALID_OFFSET) {
        return getUniformfv(block,offset,size,values);
    }
    return false;
}

/**
 * Sets the given buffer offset to an array of integer values
 *
 * Values set by this method will not be sent to the graphics card until the 
 * buffer is flushed. However, if the buffer is active and auto-flush is turned
 * on, it will be written immediately.
 *
 * If block is -1, it sets this value in every block in this uniform buffer.
 * This is a potentially expensive operation if the block is active.  For
 * mass changes, it is better to deactivate the buffer, and have them apply
 * once the buffer is reactivated.
 *
 * @param block     The block in this uniform buffer to access
 * @param offset    The offset within the block
 * @param size      The number of values to write to the buffer
 * @param values    The values to write
 */
void UniformBuffer::setUniformiv(GLint block, GLsizei offset, GLsizei size, const GLint *values) {
    CUAssertLog(block < _blockcount, "Block %d is invalid.",block);
    CUAssertLog(offset < _blocksize, "Offset %d is invalid.",offset);
    if (block >= 0) {
        GLsizei position = block*_blockstride+offset;
        std::memcpy(_bytebuffer+position, values, size*sizeof(GLint));
        if (_autoflush && isActive()) {
            glBufferSubData(GL_UNIFORM_BUFFER, position, size*sizeof(GLint), values);
        } else {
            _dirty = true;
        }
    } else {
        bool active = false;
        if (_autoflush && isActive()) {
            active = true;
        } else {
            _dirty = true;
        }
        for(int block = 0; block < _blockcount; block++) {
            GLsizei position = block*_blockstride+offset;
            std::memcpy(_bytebuffer+position, values, size*sizeof(GLint));
            if (active) {
                glBufferSubData(GL_UNIFORM_BUFFER, position, size*sizeof(GLint), values);
            }
        }
    }
}

/**
 * Sets the given buffer location to an array of integer values
 *
 * This method requires that the uniform name be previously bound to a byte offset 
 * with the call {@link #setOffset}. Values set by this method will not be sent to 
 * the graphics card until the buffer is flushed. However, if the buffer is active 
 * and auto-flush is turned on, it will be written immediately.
 *
 * If block is -1, it sets this value in every block in this uniform buffer.
 * This is a potentially expensive operation if the block is active.  For
 * mass changes, it is better to deactivate the buffer, and have them apply
 * once the buffer is reactivated.
 *
 * @param block     The block in this uniform buffer to access
 * @param name      The name of the uniform variable
 * @param size      The number of values to write to the buffer
 * @param values    The values to write
 */
void UniformBuffer::setUniformiv(GLint block, const std::string name, GLsizei size, const GLint *values) {
    GLsizei offset = getOffset(name);
    if (offset != INVALID_OFFSET) {
        setUniformiv(block,offset,size,values);
    }
}

/**
 * Returns true if it can access the given buffer offset as an array of integers
 *
 * The buffer does not have to be active to call this method.  If it is not
 * active and there are pending changes to this uniform variable, this method
 * will read those changes and not the current value in the graphics card.
 *
 * @param block     The block in this uniform buffer to access
 * @param offset    The offset within the block
 * @param size      The available size of the value array
 * @param values    The array to receive the values
 *
 * @return true if data was successfully read into values
 */
bool UniformBuffer::getUniformiv(GLuint block, GLsizei offset, GLsizei size, GLint *values) const {
    if (block >= _blockcount || offset > _blocksize) {
        return false;
    }
    GLsizei position = block*_blockstride+offset;
    std::memcpy(values, _bytebuffer+position, size*sizeof(GLint));
    return true;
}

/**
 * Returns true if it can access the given buffer location as an array of integers
 *
 * This method requires that the uniform name be previously bound to a byte offset
 * with the call {@link #setOffset}. The buffer does not have to be active to call
 * this method. If it is not active and there are pending changes to this uniform
 * variable, this method will read those changes and not the current value in the
 * graphics card.
 *
 * @param block     The block in this uniform buffer to access
 * @param name      The name of the uniform variable
 * @param size      The available size of the value array
 * @param values    The array to receive the values
 *
 * @return true if data was successfully read into values
 */
bool UniformBuffer::getUniformiv(GLuint block, const std::string name, GLsizei size, GLint *values) const {
    GLsizei offset = getOffset(name);
    if (offset != INVALID_OFFSET) {
        return getUniformiv(block,offset,size,values);
    }
    return false;
}

/**
 * Sets the given buffer offset to an array of unsigned integer values
 *
 * Values set by this method will not be sent to the graphics card until the 
 * buffer is flushed. However, if the buffer is active and auto-flush is turned
 * on, it will be written immediately.
 *
 * If block is -1, it sets this value in every block in this uniform buffer.
 * This is a potentially expensive operation if the block is active.  For
 * mass changes, it is better to deactivate the buffer, and have them apply
 * once the buffer is reactivated.
 *
 * @param block     The block in this uniform buffer to access
 * @param offset    The offset within the block
 * @param size      The number of values to write to the buffer
 * @param values    The values to write
 */
void UniformBuffer::setUniformuiv(GLint block, GLsizei offset, GLsizei size, const GLuint *values) {
    CUAssertLog(block < _blockcount, "Block %d is invalid.",block);
    CUAssertLog(offset < _blocksize, "Offset %d is invalid.",offset);
    if (block >= 0) {
        GLsizei position = block*_blockstride+offset;
        std::memcpy(_bytebuffer+position, values, size*sizeof(GLuint));
        if (_autoflush && isActive()) {
            glBufferSubData(GL_UNIFORM_BUFFER, position, size*sizeof(GLuint), values);
        } else {
            _dirty = true;
        }
    } else {
        bool active = false;
        if (_autoflush && isActive()) {
            active = true;
        } else {
            _dirty = true;
        }
        for(int block = 0; block < _blockcount; block++) {
            GLsizei position = block*_blockstride+offset;
            std::memcpy(_bytebuffer+position, values, size*sizeof(GLuint));
            if (active) {
                glBufferSubData(GL_UNIFORM_BUFFER, position, size*sizeof(GLuint), values);
            }
        }
    }
}

/**
 * Sets the given buffer location to an array of unsigned integer values
 *
 * This method requires that the uniform name be previously bound to a byte offset 
 * with the call {@link #setOffset}. Values set by this method will not be sent to 
 * the graphics card until the buffer is flushed. However, if the buffer is active 
 * and auto-flush is turned on, it will be written immediately.
 *
 * If block is -1, it sets this value in every block in this uniform buffer.
 * This is a potentially expensive operation if the block is active.  For
 * mass changes, it is better to deactivate the buffer, and have them apply
 * once the buffer is reactivated.
 *
 * @param block     The block in this uniform buffer to access
 * @param name      The name of the uniform variable
 * @param size      The number of values to write to the buffer
 * @param values    The values to write
 */
void UniformBuffer::setUniformuiv(GLint block, const std::string name, GLsizei size, const GLuint *values) {
    GLsizei offset = getOffset(name);
    if (offset != INVALID_OFFSET) {
        setUniformuiv(block,offset,size,values);
    }
}

/**
 * Returns true if it can access the buffer offset as an array of unsigned integers
 *
 * The buffer does not have to be active to call this method.  If it is not
 * active and there are pending changes to this uniform variable, this method
 * will read those changes and not the current value in the graphics card.
 *
 * @param block     The block in this uniform buffer to access
 * @param offset    The offset within the block
 * @param size      The available size of the value array
 * @param values    The array to receive the values
 *
 * @return true if data was successfully read into values
 */
bool UniformBuffer::getUniformuiv(GLuint block, GLsizei offset, GLsizei size, GLuint *values) const {
    if (block >= _blockcount || offset > _blocksize) {
        return false;
    }
    GLsizei position = block*_blockstride+offset;
    std::memcpy(values, _bytebuffer+position, size*sizeof(GLint));
    return true;
}

/**
 * Returns true if it can access the buffer location as an array of unsigned integers
 *
 * This method requires that the uniform name be previously bound to a byte offset
 * with the call {@link #setOffset}. The buffer does not have to be active to call
 * this method. If it is not active and there are pending changes to this uniform
 * variable, this method will read those changes and not the current value in the
 * graphics card.
 *
 * @param block     The block in this uniform buffer to access
 * @param name      The name of the uniform variable
 * @param size      The available size of the value array
 * @param values    The array to receive the values
 *
 * @return true if data was successfully read into values
 */
bool UniformBuffer::getUniformuiv(GLuint block, const std::string name, GLsizei size, GLuint *values) const {
    GLsizei offset = getOffset(name);
    if (offset != INVALID_OFFSET) {
        return getUniformuiv(block,offset,size,values);
    }
    return false;
}
