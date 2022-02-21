//
//  CUUniformBuffer.h
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

#ifndef __CU_UNIFORM_BUFFER_H__
#define __CU_UNIFORM_BUFFER_H__
#include <string>
#include <vector>
#include <unordered_map>
#include <cugl/math/cu_math.h>

namespace cugl {

/**
 * This class defines a uniform block buffer for shader optimization.
 *
 * Like {@link Texture}, shaders and uniform buffers have a many-to-many relationship. 
 * This many-to-many relationship is captured by bind points. A uniform buffer is 
 * associated with a bind point and a shader associates a binding point with a uniform 
 * struct. That uniform struct then pulls data from the uniform buffer.
 *
 * When discussing the relationship between a shader and a uniform buffer, we talk about 
 * a buffer being bound and a buffer being active. A bound buffer is one that is 
 * associated with a shader; the shader will pull from the uniform buffer to populate 
 * its uniform variables. An active buffer is one that is capable of receiving data from 
 * the CPU. A buffer must be active if the user wants to change any of the data in the 
 * buffer. These are two separate concepts and are treated separately in this class.
 *
 * Technically, a shader is associated with a uniform block, not a uniform buffer, since
 * a uniform buffer may have multiple blocks.  In the case of a uniform buffer with 
 * multiple blocks, the current block is managed by the method {@link #setBlock}.
 *
 * Uniform buffers are ideal in two use cases.  First of all, they are great for uniforms
 * that are shared across multiple shaders.  But it is also worthwhile to have a buffer
 * for a single shader if (1) that shader has a large number of uniforms and (2) those
 * uniforms change semi-frequently through out a render pass.  In that case, the 
 * uniform buffer should be allocated with enough blocks so that all of the possible
 * uniform values can be assigned at the start of the render pass, each to a different
 * block.  Once the shader starts to receive vertices, the uniforms should be managed
 * via the {@link #setBlock} method.
 */
class UniformBuffer {
private:
    /** The OpenGL uniform buffer; 0 is not allocated. */
    GLuint _dataBuffer;
    /** The number of blocks assigned to the uniform buffer. */
    GLuint _blockcount;
    /** The active uniform block for this buffer. */
    GLuint _blockpntr;
    /** The capacity of a single block in the uniform buffer. */
    GLsizei _blocksize;
    /** The alignment stride of a single block. */
    GLsizei _blockstride;
    /** The bind point associated with this buffer (default 0) */
    GLuint _bindpoint;
    /** An underlying byte buffer to manage the uniform data */
    char* _bytebuffer;
    /** The draw type for this buffer */
    GLenum _drawtype;
    /** Whether the byte buffer flushes automatically */
    bool _autoflush;
    /** Whether the byte buffer must be flushed to the graphics card */
    bool _dirty;
    /** A mapping of struct names to their std140 offsets */
    std::unordered_map<std::string, GLsizei> _offsets;
    /** The decriptive buffer name */
    std::string _name;

public:
#pragma mark Constructors

    /**
     * Creates an uninitialized uniform buffer.
     *
     * You must initialize the uniform buffer to allocate memory.
     */
    UniformBuffer();
    
    /**
     * Deletes this uniform buffer, disposing all resources.
     */
    ~UniformBuffer();
    
    /**
     * Deletes the uniform buffer, freeing all resources.
     *
     * You must reinitialize the uniform buffer to use it.
     */
    void dispose();

    /**
     * Initializes this uniform buffer to support a block of the given capacity.
     *
     * This uniform buffer will only support a single block.  The block capacity is
     * measured in bytes.  In std140 format, all scalars are 4 bytes, vectors are 8 
     * or 16 bytes, and matrices are treated as an array of 8 or 16 byte column vectors.
     *
     * @param capacity  The block capacity in bytes
     *
     * @return true if initialization was successful.
     */
    bool init(GLsizei capacity);

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
     * The drawtype is GL_STATIC_DRAW, GL_DYNAMIC_DRAW, or GL_STREAM_DRAW. If the
     * uniform values change often, the difference between GL_STATIC_DRAW and
     * GL_STREAM_DRAW is 1-2 orders of magnitude.
     *
     * @param capacity  The block capacity in bytes
     * @param blocks    The number of blocks to support
     *
     * @return true if initialization was successful.
     */
    bool init(GLsizei capacity, GLuint blocks);

    
#pragma mark -
#pragma mark Static Constructors
    /**
     * Returns a new uniform block to support a block of the given capacity.
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
     *
     * @return a new uniform block to support a block of the given capacity.
     */
    static std::shared_ptr<UniformBuffer> alloc(GLsizei capacity) {
        std::shared_ptr<UniformBuffer> result = std::make_shared<UniformBuffer>();
        return (result->init(capacity) ? result : nullptr);
    }
    
    /**
     * Returns a new uniform buffer to support multiple blocks of the given capacity.
     *
     * The block capacity is measured in bytes.  In std140 format, all scalars are 
     * 4 bytes, vectors are 8 or 16 bytes, and matrices are treated as an array of 
     * 8 or 16 byte column vectors.
     *
     * Keep in mind that uniform buffer blocks must be aligned, and so this may take
     * significantly more memory than the number of blocks times the capacity. If the
     * graphics card cannot support that many blocks, this method will return nullptr.
     *
     * The drawtype is GL_STATIC_DRAW, GL_DYNAMIC_DRAW, or GL_STREAM_DRAW. If the
     * uniform values change often, the difference between GL_STATIC_DRAW and
     * GL_STREAM_DRAW is 1-2 orders of magnitude.
     *
     * @param capacity  The block capacity in bytes
     * @param blocks    The number of blocks to support
     *
     * @return a new uniform buffer to support multiple blocks of the given capacity.
     */
    static std::shared_ptr<UniformBuffer> alloc(GLsizei capacity, GLuint blocks) {
        std::shared_ptr<UniformBuffer> result = std::make_shared<UniformBuffer>();
        return (result->init(capacity,blocks) ? result : nullptr);
    }

#pragma mark -
#pragma mark Attributes
    /**
     * Sets the name of this uniform buffer.
     *
     * A name is a user-defined way of identifying a buffer. It is typically the
     * appropriate shader variable name, but this is not necessary for it to
     * function properly.
     *
     * @param name  The name of this uniform block.
     */
    void setName(std::string name) { _name = name; }
    
    /**
     * Returns the name of this uniform buffer.
     *
     * A name is a user-defined way of identifying a buffer.  It is typically the
     * appropriate shader variable name, but this is not necessary for it to
     * function properly.
     *
     * @return the name of this uniform block.
     */
    const std::string& getName() const { return _name; }
    
    /**
     * Returns the draw type for this buffer
     *
     * The drawtype is GL_STATIC_DRAW, GL_DYNAMIC_DRAW, or GL_STREAM_DRAW. If the
     * uniform values change often, the difference between GL_STATIC_DRAW and
     * GL_STREAM_DRAW is 1-2 orders of magnitude.
     *
     * By default, the draw type is GL_STREAM_DRAW.
     *
     * @return the draw type for this buffer
     */
     GLenum getDrawType() const { return _drawtype; }

    /**
     * Sets the draw type for this buffer
     *
     * The drawtype is GL_STATIC_DRAW, GL_DYNAMIC_DRAW, or GL_STREAM_DRAW. If the
     * uniform values change often, the difference between GL_STATIC_DRAW and
     * GL_STREAM_DRAW is 1-2 orders of magnitude.
     *
     * By default, the draw type is GL_STREAM_DRAW.
     *
     * @param type   The draw type for this buffer
     */
     void setDrawType(GLenum type) { _drawtype = type; }
     
    /**
     * Returns true if this uniform buffer has been properly initialized.
     *
     * @return true if this uniform buffer has been properly initialized.
     */
    bool isReady() const { return _dataBuffer != 0; }
    
    /**
     * Returns the number of blocks supported by this buffer.
     *
     * A uniform buffer can support multiple uniform blocks at once.  The
     * active block is identify by the method {@link #getBlock}.
     *
     * @return the number of blocks supported by this buffer.
     */
    GLuint getBlockCount() const { return _blockcount; }

    /**
     * Returns the capacity of a single block in this uniform buffer.
     *
     * The block size is the amount of data necessary to populate the
     * uniforms for a single block. It is measured in bytes.
     *
     * @return the capacity of a single block in this uniform buffer.
     */
    GLsizei getBlockSize() const { return _blocksize; }
    
    /**
     * Returns the stride of a single block in this uniform buffer.
     *
     * The stride measures the alignment (in bytes) of a block.  It is at
     * least as large as the block capacity, but may be more.
     *
     * @return the stride of a single block in this uniform buffer.
     */
    GLsizei getBlockStride() const { return _blockstride; }
    
    /**
     * Returns the backing byte-buffer for the uniform buffer
     *
     * The byte buffer is used to store pending changes while the buffer is active
     * (and is kept synchronized when it is active).  This method allows direct access
     * for bulk writes the buffer.  However, the graphics card will not be aware of
     * any of these changes until {@link #flush()} is called.
     *
     * The buffer returned will have a capacity of (block count) x (block stride).
     *
     * @return the backing byte-buffer for the uniform buffer
     */
    char* getData() { return _bytebuffer; }

    /**
     * Returns true if this uniform buffer supports autoflushing.
     *
     * Active, auto-flushed buffers write data directly to the graphics card as soon
     * as it is provided to the buffer.  A buffer than does not auto-flush will not
     * write to the graphics card until {@link #flush} is called, even if it is active.
     *
     * By default, autoflushing is turned off.
     *
     * @return true if this uniform buffer supports autoflushing.
     */
    bool getAutoflush() const { return _autoflush; }
    
    /**
     * Sets whether this uniform buffer supports autoflushing.
     *
     * Active, auto-flushed buffers write data directly to the graphics card as soon
     * as it is provided to the buffer.  A buffer than does not auto-flush will not
     * write to the graphics card until {@link #flush} is called, even if it is active.
     *
     * By default, autoflushing is turned off.
     *
     * @param style Whether this uniform buffer supports autoflushing.
     */
    void setAutoflush(bool style) { _autoflush = style; }


#pragma mark -
#pragma mark Binding
    /**
     * Returns the OpenGL buffer for this uniform buffer.
     *
     * The buffer is a value assigned by OpenGL when the uniform buffer was allocated. 
     * This method will return 0 if the block is not initialized. This method is 
     * provided to allow the user direct access to the buffer for maximum flexibility.
     *
     * @return the OpenGL buffer for this unform block.
     */
    GLuint getBuffer() const { return _dataBuffer; }
    
    /**
     * Returns the bind point for this uniform buffer.
     *
     * Uniform buffers and shaders have a many-to-many relationship. This means
     * that connecting them requires an intermediate table. The positions in
     * this table are called bind points. A uniform buffer is associated with
     * a bind point and a shader associates a bind point with a uniform struct.
     * That uniform struct then pulls data from the active block of the uniform 
     * buffer. By default this value is 0.
     *
     * @return the bind point for for this uniform block.
     */
    GLuint getBindPoint() const { return _bindpoint; }
    
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
    void setBindPoint(GLuint point);
    
    /**
     * Binds this uniform buffer to its bind point.
     *
     * Unlike {@link Texture}, it is possible to bind a uniform buffer to its
     * bind point without making it the active uniform buffer. An inactive buffer
     * will still stream data to the shader, though its data cannot be altered 
     * without making it active.
     *
     * Binding a buffer to a bind point replaces the uniform block originally
     * there.  So this buffer can be unbound without a call to {@link #unbind}.
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
    void bind(bool activate=true);
    
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
    void unbind();
    
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
    void activate();
    
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
    void deactivate();
    
    /**
     * Returns true if this uniform buffer is currently bound.
     *
     * A uniform buffer is bound if it is attached to a bind point. That means that 
     * the shader will pull its data for that bind point from this buffer. A uniform 
     * block can be bound without being active.
     *
     * @return true if this uniform block is currently bound.
     */
    bool isBound() const;
    
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
    bool isActive() const;
    
    /**
     * Returns the active uniform block in this buffer.
     *
     * The active uniform block is the block from which the shader will pull
     * uniform values.  This value can be altered even if the buffer is not active
     * (or even bound)
     *
     * @return the active uniform block in this buffer.
     */
     GLuint getBlock() const { return _blockpntr; }

    /**
     * Sets the active uniform block in this buffer.
     *
     * The active uniform block is the block from which the shader will pull
     * uniform values. This value can only be altered if this buffer is bound
     * (though it need not be active).
     *
     * @param block The active uniform block in this buffer.
     */
     void setBlock(GLuint block);

    /**
     * Flushes any changes in the backing byte buffer to the graphics card.
     *
     * This method must be called if any changes have been made to the buffer data and 
     * auto-flush is not turned on (which is the default).  Even if auto-flush is 
     * turned on, it must be called if the user has accessed the backing byte buffer
     * directly via {@link #getData}.
     *
     * This method requires the byte buffer to be active.
     */
    void flush();
#pragma mark -
#pragma mark Data Offsets
    
    /** The byte position of an invalid offset */
    static GLsizei INVALID_OFFSET;

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
    void setOffset(const std::string name, GLsizei offset);
    
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
    GLsizei getOffset(const std::string name) const;

    /**
     * Returns the offsets defined for this buffer
     *
     * The vector returned will include the name of every variable set by
     * the method {@link #setOffset}.
     *
     * @return the offsets defined for this buffer
     */
    std::vector<std::string> getOffsets() const;


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
    void setUniformVec2(GLint block, GLsizei offset, const Vec2 vec);
    
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
    void setUniformVec2(GLint block, const std::string name, const Vec2 vec);

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
    bool getUniformVec2(GLuint block, GLsizei offset, Vec2& vec) const;

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
    bool getUniformVec2(GLuint block, const std::string name, Vec2& vec) const;
    
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
    void setUniformVec3(GLint block, GLsizei offset, const Vec3 vec);

    /**
     * Sets the given uniform variable to a vector value.
     *
     * This method requires that the uniform name be previously bound to a byte
     * offset with the call {@link #setOffset}. This method will write the vector 
     * as 3*sizeof(float) bytes to the appropriate buffer location (and the buffer 
     * must have the appropriate capacity).
     *
     * The buffer does not need to be active to call this method. However, changes
     * made while the buffer is inactive will be cached and not applied until the
     * buffer is reactivated.
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
    void setUniformVec3(GLint block, const std::string name, const Vec3 vec);

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
    bool getUniformVec3(GLuint block, GLsizei offset, Vec3& vec) const;

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
    bool getUniformVec3(GLuint block, const std::string name, Vec3& vec) const;

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
    void setUniformVec4(GLint block, GLsizei offset, const Vec4 vec);

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
    void setUniformVec4(GLint block, const std::string name, const Vec4 vec);

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
    bool getUniformVec4(GLuint block, GLsizei offset, Vec4& vec) const;

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
    bool getUniformVec4(GLuint block, const std::string name, Vec4& vec) const;

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
    void setUniformColor4(GLint block, GLsizei offset, const Color4 color);

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
    void setUniformColor4(GLint block, const std::string name, const Color4 color);

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
    bool getUniformColor4(GLuint block, GLsizei offset, Color4& color) const;

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
    bool getUniformColor4(GLuint block, const std::string name, Color4& color) const;

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
    void setUniformColor4f(GLint block, GLsizei offset, const Color4f color);

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
    void setUniformColor4f(GLint block, const std::string name, const Color4f color);

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
    bool getUniformColor4f(GLuint block, GLsizei offset, Color4f& color) const;

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
    bool getUniformColor4f(GLuint block, const std::string name, Color4f& color) const;

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
    void setUniformMat4(GLint block, GLsizei offset, const Mat4& mat);
    
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
    void setUniformMat4(GLint block, const std::string name, const Mat4& mat);

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
    bool getUniformMat4(GLuint block, GLsizei offset, Mat4& mat) const;

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
    bool getUniformMat4(GLuint block, const std::string name, Mat4& mat) const;

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
    void setUniformAffine2(GLint block, GLsizei offset, const Affine2& mat);

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
    void setUniformAffine2(GLint block, const std::string name, const Affine2& mat);

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
    bool getUniformAffine2(GLuint block, GLsizei offset, Affine2& mat) const;

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
     * @param name  	The name of the uniform variable
     * @param mat       The matrix to store the result
     *
     * @return true if it can access the given uniform variable as an affine transform.
     */
    bool getUniformAffine2(GLuint block, const std::string name, Affine2& mat) const;

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
    void setUniformQuaternion(GLint block, GLsizei offset, const Quaternion& quat);

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
    void setUniformQuaternion(GLint block, const std::string name, const Quaternion& quat);

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
    bool getUniformQuaternion(GLuint block, GLsizei offset, Quaternion& quat) const;

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
    bool getUniformQuaternion(GLuint block, const std::string name, Quaternion& quat) const;


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
    void setUniformfv(GLint block, GLsizei offset, GLsizei size, const GLfloat *values);

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
    void setUniformfv(GLint block, const std::string name, GLsizei size, const GLfloat *values);
    
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
    bool getUniformfv(GLuint block, GLsizei offset, GLsizei size, GLfloat *values) const;

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
    bool getUniformfv(GLuint block, const std::string name, GLsizei size, GLfloat *values) const;

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
    void setUniformiv(GLint block, GLsizei offset, GLsizei size, const GLint *values);
    
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
    void setUniformiv(GLint block, const std::string name, GLsizei size, const GLint *values);
    
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
    bool getUniformiv(GLuint block, GLsizei offset, GLsizei size, GLint *values) const;
    
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
    bool getUniformiv(GLuint block, const std::string name, GLsizei size, GLint *values) const;

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
    void setUniformuiv(GLint block, GLsizei offset, GLsizei size, const GLuint *values);

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
    void setUniformuiv(GLint block, const std::string name, GLsizei size, const GLuint *values);
    
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
    bool getUniformuiv(GLuint block, GLsizei offset, GLsizei size, GLuint *values) const;

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
    bool getUniformuiv(GLuint block, const std::string name, GLsizei size, GLuint *values) const;
};

}

#endif /* __CU_UNIFORM_BUFFER_H__ */
