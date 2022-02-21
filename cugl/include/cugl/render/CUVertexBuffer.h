//
//  CUVertexBuffer.h
//  Cornell University Game Library (CUGL)
//
//  This module provides a vertex buffer for drawing to OpenGL. A
//  vertex buffer receives vertices and uniforms, and passes them
//  to a shader.  A vertex buffer must be attached to a shader to
//  be used.  However, a vertex buffer can swap shaders at any
//  time, which is why this class is separated out.
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
//  Version: 2/25/20

#ifndef __CU_VERTEX_BUFFER_H__
#define __CU_VERTEX_BUFFER_H__

#include <string>
#include <unordered_map>
#include <cugl/math/CUMathBase.h>
#include <cugl/math/CUMat4.h>


namespace cugl {

// Forward reference to the associated shader
class Shader;

/**
 * This class defines a vertex buffer for drawing with a shader.
 *
 * What we are calling a vertex buffer is technically a vertex array plus its
 * associated buffers in OpenGL. A vertex buffer receives vertices and passes
 * them to a shader. A vertex buffer must be attached to a shader to be used.
 * However, a vertex buffer can swap shaders at any time, which is why this
 * class is separated out.
 *
 * Unlike {@link Texture} and {@link UniformBuffer}, a vertex buffer does
 * not have a true many one relationship with a {@link Shader} object.
 * A vertex buffer can only be connected to one shader at a time and vice
 * versa.  So we model this as a direct connection.  As vertex buffers push
 * data to a shader, the dependency requires that a shader be linked to
 * a vertex buffer object.
 *
 * This class tries to remain loosely coupled with its shader.  If the vertex
 * buffer has attributes lacking in the shader, they will be ignored. If it is missing
 * attributes that the shader expects, the shader will use the default value
 * for the type.
 */
class VertexBuffer {
private:
    /**
     * A data type for keeping track of attribute data.
     *
     * This class is necessary since we are allowing the vertex
     * buffer to specify attributes before hooking it up to the shader.
     * This type is used to initialize the attribute hooks as soon as
     * the shader is attached.
     */
    class AttribData {
    public:
        /** The attribute size */
        GLint size;
        /** The attribute type (as specified in OpenGL) */
        GLenum type;
        /** Whether the attribute is normalized (floating points only) */
        GLboolean norm;
        /** The offset of the attribute in the vertex buffer */
        GLsizeiptr offset;
    };
    
    /** The data stride of this buffer (0 if there is only one attribute) */
    GLsizei _stride;

    /** The array buffer for drawing a the shape */
    GLuint _vertArray;
    /** The vertex buffer for drawing a shape */
    GLuint _vertBuffer;
    /** The index buffer for drawing a shape */
    GLuint _indxBuffer;
    
    /** The shader currently attached to this vertex buffer */
    std::shared_ptr<Shader> _shader;
    
    /** The enabled attributes */
    std::unordered_map<std::string, bool> _enabled;
    /** The settings for each attribute */
    std::unordered_map<std::string, AttribData> _attributes;
    
public:
#pragma mark Constructors
    /**
     * Creates an uninitialized vertex buffer.
     *
     * You must initialize the vertex buffer to allocate buffer memory.
     */
    VertexBuffer();
    
    /**
     * Deletes this vertex buffer, disposing all resources.
     */
    ~VertexBuffer();
    
    /**
     * Deletes the vertex buffer, freeing all resources.
     *
     * You must reinitialize the vertex buffer to use it.
     */
    void dispose();

    /**
     * Initializes this vertex buffer to support the given stride.
     *
     * The stride is the size of a single piece of vertex data.  The vertex buffer
     * needs this value to set attribute locations.  Since changing this value
     * fundamentally changes the type of data that can be sent to this vertex
     * buffer, it is set at buffer creation and cannot be changed.
     *
     * It is possible for the stride to be 0, but only if the shader consists
     * of a single attribute.  Using stride 0 is not recommended.
     *
     * @param stride   The size of a single piece of vertex data.
     *
     * @return true if initialization was successful.
     */
    bool init(GLsizei stride);
    
    /**
     * Returns a new vertex buffer to support the given stride.
     *
     * The stride is the size of a single piece of vertex data.  The vertex buffer
     * needs this value to set attribute locations.  Since changing this value
     * fundamentally changes the type of data that can be sent to this vertex
     * buffer, it is set at buffer creation and cannot be changed.
     *
     * It is possible for the stride to be 0, but only if the shader consists
     * of a single attribute.  Using stride 0 is not recommended.
     *
     * @param stride   The size of a single piece of vertex data.
     *
     * @return a new vertex buffer to support the given stride.
     */
    static std::shared_ptr<VertexBuffer> alloc(GLsizei stride) {
        std::shared_ptr<VertexBuffer> result = std::make_shared<VertexBuffer>();
        return (result->init(stride) ? result : nullptr);
    }


#pragma mark -
#pragma mark Binding
    /**
     * Binds this vertex buffer, making it active.
     *
     * If this vertex buffer has an attached shader, this will bind the shader
     * as well. Once bound, all vertex data and uniforms will be sent to
     * the associated shader.
     *
     * A vertex buffer can be bound without being attached to a shader.  However,
     * if it is actively attached to a shader, this method will bind that shader 
     * as well.
     */
    void bind();
    
    /**
     * Unbinds this vertex buffer, making it no longer active.
     *
     * A vertex buffer can be unbound without being attached to a shader.  Furthermore,
     * if it is actively attached to a shader, this method will NOT unbind the shader.
     * This allows for fast(er) switching between buffers of the same shader.
     *
     * Once unbound, all vertex data and uniforms will be ignored.  In addition, all
     * uniforms and samplers are potentially invalidated.  These values should be
     * set again when the vertex buffer is next bound.
     */
    void unbind();
    
    /**
     * Attaches the given shader to this vertex buffer.
     *
     * This method will link all enabled attributes in this vertex buffer
     * (warning about any attributes that are missing from the shader).
     * It will also immediately bind both the vertex buffer and the
     * shader, making them ready to use.
     *
     * @param shader    The shader to attach
     */
    void attach(const std::shared_ptr<Shader>& shader);
    
    /**
     * Returns the previously active shader, after detaching it.
     *
     * This method will unbind the vertex buffer, but not the shader.
     *
     * @return  The previously active shader (or null if none)
     */
    std::shared_ptr<Shader> detach();
    
    /**
     * Returns the shader currently attached to this vertex buffer.
     *
     * @return the shader currently attached to this vertex buffer.
     */
    std::shared_ptr<Shader> getShader() const { return _shader; };

    /**
     * Returns true if this vertex is currently bound.
     *
     * @return true if this vertex is currently bound.
     */
    bool isBound() const;

    
#pragma mark -
#pragma mark Vertex Processing
    /**
     * Returns the stride of this vertex buffer
     *
     * The data loaded is expected to have the size of the vertex buffer stride.
     * If it does not, strange things will happen.
     *
     * @return the stride of this vertex buffer
     */
     GLsizei getStride() const { return _stride; }
    
    /**
     * Loads the given vertex buffer with data.
     *
     * The data loaded is the data that will be used at the next draw command.
     * Frequent reloading of data and/or indices is to be discouraged (though it 
     * is faster than swapping to another vertex buffer). Instead, data and indices
     * should be loaded once (if possible) and draw calls should make use of the
     * offset parameter.
     *
     * The data loaded is expected to have the size of the vertex buffer stride.
     * If it does not, strange things will happen.
     *
     * The data usage is one of GL_STATIC_DRAW, GL_STREAM_DRAW, or GL_DYNAMIC_DRAW.
     * Static drawing should be reserved for vertices and/or indices that do not 
     * change (so all animation happens in uniforms). Given the high speed of
     * CPU processing, this approach should only be taken for large meshes that
     * can amortize the uniform changes.  For quads and other simple meshes, 
     * you should always choose GL_STREAM_DRAW.
     *
     * This method will only succeed if this buffer is actively bound.
     *
     * @param data  The data to load
     * @param size  The number of vertices to load
     * @param usage The type of data load
     */
    void loadVertexData(const void * data, GLsizei size, GLenum usage=GL_STREAM_DRAW);
    
    /**
     * Loads the given vertex buffer with indices.
     *
     * The indices loaded are those that will be used at the next draw command.
     * Frequent reloading of data and/or indices is to be discouraged (though it 
     * is faster than swapping to another vertex buffer). Instead, data and indices
     * should be loaded once (if possible) and draw calls should make use of the
     * offset parameter.
     *
     * The indices loaded are expected to refer to valid vertex positions. If they
     * do not, strange things will happen.
     *
     * The data usage is one of GL_STATIC_DRAW, GL_STREAM_DRAW, or GL_DYNAMIC_DRAW.
     * Static drawing should be reserved for vertices and/or indices that do not 
     * change (so all animation happens in uniforms). Given the high speed of
     * CPU processing, this approach should only be taken for large meshes that
     * can amortize the uniform changes.  For quads and other simple meshes, 
     * you should always choose GL_STREAM_DRAW and push as much computation to the
     * CPU as possible.
     *
     * This method will only succeed if this buffer is actively bound.
     *
     * @param data  The indices to load
     * @param size  The number of indices to load
     * @param usage The type of data load
     */
    void loadIndexData(const void * data, GLsizei size, GLenum usage=GL_STREAM_DRAW);
    
    /**
     * Draws to the active framebuffer using this vertex buffer
     *
     * Any call to this command will use the current texture and uniforms. If
     * the texture and/or uniforms need to be changed, then this draw command
     * will need to be broken up into chunks. Use the optional parameter 
     * offset to chunk up the draw calls without having to reload data.
     *
     * The drawing mode can be any of  GL_POINTS, GL_LINE_STRIP, GL_LINE_LOOP, 
     * GL_LINES, GL_TRIANGLE_STRIP, GL_TRIANGLE_FAN or GL_TRIANGLES.  These
     * are the only modes accepted by both OpenGL and OpenGLES. See the OpenGL
     * documentation for the number of indices required for each type.  In
     * practice the {@link Poly2} class is designed to support GL_POINTS, 
     * GL_LINES, and GL_TRIANGLES only.
     *
     * This method will only succeed if this buffer is actively bound.
     *
     * @param mode      The OpenGLES drawing mode
     * @param count     The number of vertices to draw
     * @param offset    The initial index to start with
     */
    void draw(GLenum mode, GLsizei count, GLsizei offset=0);
    
    /**
     * Draws to the active framebuffer using this vertex buffer
     *
     * This version of drawing supports instancing.This allows you to draw the
     * the same vertices multiple times, with slightly different uniforms each
     * time. While the use of this is limited -- there is an 8096 byte limit on
     * uniforms for more shaders -- it can speed up rendering in some special 
     * cases. See the documentation of glDrawElementsInstanced for how to properly
     * leverage instancing.
     *
     * Any call to this command will use the current texture and uniforms. If
     * the texture and/or uniforms need to be changed, then this draw command
     * will need to be broken up into chunks. Use the optional parameter 
     * offset to chunk up the draw calls without having to reload data.
     *
     * The drawing mode can be any of  GL_POINTS, GL_LINE_STRIP, GL_LINE_LOOP, 
     * GL_LINES, GL_TRIANGLE_STRIP, GL_TRIANGLE_FAN or GL_TRIANGLES.  These
     * are the only modes accepted by both OpenGL and OpenGLES. See the OpenGL
     * documentation for the number of indices required for each type.  In
     * practice the {@link Poly2} class is designed to support GL_POINTS, 
     * GL_LINES, and GL_TRIANGLES only.
     *
     * This method will only succeed if this buffer is actively bound.
     *
     * @param mode      The OpenGLES drawing mode
     * @param count     The number of vertices to draw
     * @param instances The number of instances to draw
     * @param offset    The initial index to start with
     */
    void drawInstanced(GLenum mode, GLsizei count, GLsizei instances, GLsizei offset=0);
    
    
#pragma mark -
#pragma mark Attributes
    /** 
     * Initializes an attributes, assigning is a size, type and offset.
     *
     * This method is necessary for the vertex buffer to convey data to the 
     * shader. Without it, the shader will used default values for the attribute
     * rather than data from the vertex buffer.
     *
     * It is safe to call this method even when the shader is not attached.
     * The values will be cached and will be used to link this buffer to the
     * shader when the shader is attached.  This also means that a vertex 
     * buffer can swap shaders without having to reinitialize attributes.
     * If a shader is attached, the attribute will be enabled immediately.
     *
     * If the attribute does not refer to one supported by the active 
     * shader, then it will be ignored (e.g. the effect is the same as 
     * disabling the attribute).
     *
     * The attribute type can be one of GL_BYTE, GL_UNSIGNED_BYTE, GL_SHORT, 
     * GL_UNSIGNED_SHORT, GL_INT, GL_UNSIGNED_INT, GL_HALF_FLOAT, GL_FLOAT, 
     * GL_FIXED, or GL_INT_2_10_10_10_REV.  Doubles are not supported by
     * OpenGLES.
     *
     * The attribute offset is measured in bytes from the start of the 
     * vertex data structure (for a single vertex).
     *
     * @param name      The attribute name
     * @param size      The attribute size in byte.
     * @param type      The attribute type
     * @param norm      Whether to normalize the value (floating point only)
     * @param offset    The attribute offset in the vertex data structure
     */
    void setupAttribute(const std::string name, GLint size, GLenum type,
                        GLboolean norm, GLsizei offset);
    
    
    /**
     * Enables the given attribute
     *
     * Attributes are immediately enabled once they are set-up.  This method
     * is only needed if the attribute was previously disabled.  It will have
     * no effect if the active shader does not support this attribute.
     *
     * @param name  The attribute to enable.
     */
    void enableAttribute(const std::string name);
    
    /**
     * Disables the given attribute
     *
     * Attributes are immediately enabled once they are set-up.  This method
     * allows you to temporarily turn off an attribute.  If that attribute is
     * required by the shader, it will use the default value for the type instead.
     *
     * @param name  The attribute to disable.
     */
    void disableAttribute(const std::string name);
    

};

}

#endif /* __CU_VERTEX_BUFFER_H__ */
