//
//  CUVertexBuffer.cpp
//  Cornell University Game Library (CUGL)
//
//  This module provides a vertex buffer for drawing to OpenGL. A
//  vertex buffer receives vertices and uniforms, and passes them
//  to a shader.  A vertex buffer must be attached to a shader to
//  be used.  However, a vertex buffer can swap shaders at any
//  time, which is why this class is separated out.
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
#include <cugl/render/CUVertexBuffer.h>
#include <cugl/render/CUShader.h>
#include <cugl/render/CUTexture.h>

using namespace cugl;

#pragma mark Constructors
/**
 * Creates an uninitialized vertex buffer.
 *
 * You must initialize the vertex buffer to allocate buffer memory.
 */
VertexBuffer::VertexBuffer() :
_vertArray(0),
_vertBuffer(0),
_indxBuffer(0),
_stride(0) {
    _shader = nullptr;
}

/**
 * Deletes this vertex buffer, disposing all resources.
 */
VertexBuffer::~VertexBuffer() {
    dispose();
}

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
bool VertexBuffer::init(GLsizei stride) {
    _stride = stride;
    glGenVertexArrays (1, &_vertArray);
    if (!_vertArray) {
        GLenum error = glGetError();
        CULogError("Could not create vertex array. %s", gl_error_name(error).c_str());
        return false;
    }

    // Generate the buffers
    glGenBuffers(1, &_vertBuffer);
    if (!_vertBuffer) {
        GLenum error = glGetError();
        glDeleteVertexArrays(1,&_vertArray);
        CULogError("Could not create vertex buffer. %s", gl_error_name(error).c_str());
        return false;
    }
    
    glGenBuffers(1, &_indxBuffer);
    if (!_indxBuffer) {
        GLenum error = glGetError();
        CULogError("Could not create index buffer. %s", gl_error_name(error).c_str());
        glDeleteVertexArrays(1,&_vertArray);
        glDeleteBuffers(1,&_vertBuffer);
        return false;
    }
    
    return true;
}

/**
 * Deletes the vertex buffer, freeing all resources.
 *
 * You must reinitialize the vertex buffer to use it.
 */
void VertexBuffer::dispose() {
    if (!_vertBuffer) {
        return;
    }
    _enabled.clear();
    _attributes.clear();
    glDeleteBuffers(1,&_indxBuffer);
    glDeleteBuffers(1,&_vertBuffer);
    glDeleteVertexArrays(1,&_vertArray);
    _indxBuffer = 0;
    _vertBuffer = 0;
    _vertArray  = 0;
    _shader = nullptr;
    _stride = 0;
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
void VertexBuffer::bind() {
    CUAssertLog(_vertBuffer, "VertexBuffer has not be initialized.");
    glBindVertexArray(_vertArray);
	glBindBuffer( GL_ARRAY_BUFFER, _vertBuffer );
	glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, _indxBuffer );
    if (_shader != nullptr) {
        _shader->bind();
    }
}

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
void VertexBuffer::unbind() {
    if (isBound()) {
        glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, 0 );
        glBindBuffer( GL_ARRAY_BUFFER, 0 );
        glBindVertexArray(0);
    }
}

/**
 * Attaches the given shader to this vertex buffer.
 *
 * This method will link all enabled attributes in this vertex buffer
 * (warning about any attributes that are missing from the shader).
 * It will also immediately bind both the vertex buffer and the
 * shader, making them ready to use.
 *
 * @param shader    The shader to detach
 */
void VertexBuffer::attach(const std::shared_ptr<Shader>& shader) {
    CUAssertLog(shader, "Attempting to attach a null shader");
    if (_shader != shader) {
        _shader = shader;
        bind();
        
        // Link up attributes on the first time
        for(auto it = _attributes.begin(); it != _attributes.end(); ++it) {
            std::string name = it->first;
			GLint pos = glGetAttribLocation(_shader->getProgram(), name.c_str());
			if (pos == -1) {
				CUWarn("Active shader has no attribute %s", name.c_str());
			} else if (_enabled[name]) {
				glEnableVertexAttribArray(pos);
				glVertexAttribPointer(pos,it->second.size,it->second.type,
									  it->second.norm,_stride,
									  reinterpret_cast<void*>(it->second.offset));
			} else {
				glDisableVertexAttribArray(pos);
			}
        }

        GLenum error = glGetError();
        CUAssertLog(error == GL_NO_ERROR, "VertexBuffer: %s", gl_error_name(error).c_str());
    } else {
        bind();
    }
}

/**
 * Returns the previously active shader, after detaching it.
 *
 * This method will unbind the vertex buffer, but not the shader.
 *
 * @return  The previously active shader (or null if none)
 */
std::shared_ptr<Shader> VertexBuffer::detach() {
    std::shared_ptr<Shader> result = _shader;
    unbind();
    _shader = nullptr;
    return result;
}

/**
 * Returns true if this vertex is currently bound.
 *
 * @return true if this vertex is currently bound.
 */
bool VertexBuffer::isBound() const {
    GLint vao;
    glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &vao);
    return vao == _vertArray;
}


#pragma mark -
#pragma mark Vertex Processing
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
void VertexBuffer::loadVertexData(const void * data, GLsizei size, GLenum usage) {
    //CUAssertLog(isBound(), "Vertex buffer is not bound"); // Problems on android emulator for now
    glBufferData( GL_ARRAY_BUFFER, _stride * size, data, usage );
    
    GLenum error = glGetError();
    CUAssertLog(error == GL_NO_ERROR, "VertexBuffer: %s", gl_error_name(error).c_str());
}

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
void VertexBuffer::loadIndexData(const void * data, GLsizei size, GLenum usage) {
    //CUAssertLog(isBound(), "Vertex buffer is not bound"); // Problems on android emulator for now
    glBufferData( GL_ELEMENT_ARRAY_BUFFER, size * sizeof(GLuint), data, usage );
    GLenum error = glGetError();
    CUAssertLog(error == GL_NO_ERROR, "VertexBuffer: %s", gl_error_name(error).c_str());
}

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
void VertexBuffer::draw(GLenum mode, GLsizei count, GLsizei offset) {
    //CUAssertLog(isBound(), "Vertex buffer is not bound"); // Problems on android emulator for now
    glDrawElements(mode, count, GL_UNSIGNED_INT, (void*)(offset * sizeof(GLuint)));
}

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
void VertexBuffer::drawInstanced(GLenum mode, GLsizei count, GLsizei instance, GLsizei offset) {
    //CUAssertLog(isBound(), "Vertex buffer is not bound"); // Problems on android emulator for now
    glDrawElementsInstanced(mode, count, GL_UNSIGNED_INT, (void*)(offset * sizeof(GLuint)), instance);
}


#pragma mark -
#pragma mark Attributes
/**
 * Defines the (periodic) position for the given attribute in this vertex buffer.
 *
 * Since a vertex buffer is often an interleaving of multiple attributes, the
 * associated shader needs to know how to map data to the shader
 * attributes.  That is the purpose of this method.
 *
 * However, this method can be called even when a shader is not attached
 * to this buffer.  The values will be cached and applied when a shader is
 * attached.  This allows a vertex buffer to swap (compatible) shaders
 * with little additional code.
 *
 *@param name   The name of the attribute
 *@param size   The number of components per vertex
 *@param type   The data type per component
 *@param norm   Whether the data values are normalized (floating point only)
 *@param offset The offset of the first component in the buffer
 */
void VertexBuffer::setupAttribute(const std::string name, GLint size, GLenum type,
                                  GLboolean norm, GLsizei offset) {
    AttribData data;
    data.size = size;
    data.norm = norm;
    data.type = type;
    data.offset = offset;
    _attributes[name] = data;
    _enabled[name] = true;
    
    if (_shader != nullptr) {
        _shader->bind();
        GLint pos = glGetAttribLocation(_shader->getProgram(), name.c_str());
        if (pos == -1) {
            CUWarn("Active shader has no attribute %s", name.c_str());
        } else {
            glEnableVertexAttribArray(pos);
            glVertexAttribPointer(pos,data.size,data.type,data.norm,_stride,
                                  reinterpret_cast<void*>(data.offset));
        }
        
        GLenum error = glGetError();
        CUAssertLog(error == GL_NO_ERROR, "VertexBuffer: %s", gl_error_name(error).c_str());
    }
}

/**
 * Enables the given attribute
 *
 * Attributes are immediately enabled once they are set-up.  This method
 * is only needed if the attribute was previously disabled.  It will have
 * no effect if the active shader does not support this attribute.
 *
 * @param name  The attribute to enable.
 */
void VertexBuffer::enableAttribute(const std::string name) {
	CUAssertLog(_enabled.find(name) != _enabled.end(),
                "Vertex buffer has no attribute %s", name.c_str());
    CUAssertLog(isBound(), "Vertex buffer is not bound.");
	if (!_enabled[name]) {
		_enabled[name] = true;
		if (_shader != nullptr) {
			GLint locale = _shader->getUniformLocation(name);
			glEnableVertexAttribArray(locale);
		}
	}
}

/**
 * Disables the given attribute
 * 
 * Attributes are immediately enabled once they are set-up.  This method
 * allows you to temporarily turn off an attribute.  If that attribute is
 * required by the shader, it will use the default value for the type instead.
 *
 * @param name  The attribute to disable.
 */
void VertexBuffer::disableAttribute(const std::string name) {
	CUAssertLog(_enabled.find(name) != _enabled.end(),
                "Vertex buffer has no attribute %s", name.c_str());
    CUAssertLog(isBound(), "Vertex buffer is not bound.");
	if (_enabled[name]) {
		_enabled[name] = false;
		if (_shader != nullptr) {
			GLint locale = _shader->getUniformLocation(name);
			glDisableVertexAttribArray(locale);
		}
	}    
}
