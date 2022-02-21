//
//  CUMesh.h
//  Cornell University Game Library (CUGL)
//
//  This template provides a general purpose mesh class.  It stores the
//  vertex data to be used by a {@link VertexBuffer} as well as the
//  associated indices and OpenGL drawing command.
//
//  While we generally try to avoid templates (it is hard to guarantee a lot
//  of functionality, and they are tricky to debug), this is one case in the
//  rendering pipeline where a template is absolutely necessary.  That is
//  because the actual vertex data varies from shader to shader.
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
//  Version: 2/12/20
#ifndef __CU_MESH_H__
#define __CU_MESH_H__
#include <vector>
#include <sstream>
#include <memory>
#include <unordered_map>
#include <cugl/math/cu_math.h>
//#include <cugl/math/CUPath2.h>
#include <cugl/math/CUPoly2.h>
#include <cugl/util/CUDebug.h>

#define GL_UNDEFINED -1

namespace cugl {

/**
 * This class represents an arbitrary drawing mesh.
 *
 * A mesh is a collection of vertices, together with indices and a drawing command.
 * The type of the indices and drawing command are fixed, but the vertex type is
 * templated.  This allows a mesh to be adapter to an arbitrary {@link Shader}.
 *
 * The only requirement of a mesh vertex is that it have at least one field called
 * position, and this type be one of {@link Vec2}, {@link Vec3} or {@link Vec4}.
 */
template <typename T>
class Mesh {
public:
    /** The mesh vertices, to be passed to the shader */
    std::vector<T> vertices;
    /** The mesh indices, providing a shape to the vertices */
    std::vector<GLuint> indices;
    /** The OpenGL drawing command */
    GLenum command;

    /**
     * Creates an empty mesh with no data.
     *
     * Access the attributes to add data to the mesh.
     */
    Mesh() { command = GL_UNDEFINED; }

    /**
     * Deletes the given mesh, freeing all resources.
     */
    ~Mesh() { clear(); }
    
    /**
     * Creates a copy of the given mesh.
     *
     * Both the vertices and the indices are copied.  No references to the
     * original mesh are kept.
     *
     * @param mesh  The mesh to copy
     */
    Mesh(const Mesh& mesh) { set(mesh); }

    /**
     * Creates a copy with the resource of the given polygon.
     *
     * It is unsafe to use the original mesh after this method is called.
     *
     * @param mesh  The mesh to take from
     */
    Mesh(Mesh&& mesh) :
        vertices(std::move(mesh.vertices)), indices(std::move(mesh.indices)), command(mesh.command) {}

    /**
     * Creates a mesh from the given {@link Poly2} object.
     *
     * No vertex attribute other than position is set.  Additional
     * information (such as color or texture coordinate) must be added
     * later.  The command will be GL_TRIANGLES if the polygon is solid,
     * GL_LINES if it is a path, and GL_UNDEFINED otherwise.
     *
     * @param poly  The polygon defining this mesh
     */
    Mesh(const Poly2& poly) {
        set(poly);
    }

#pragma mark -
#pragma mark Setters
    /**
     * Sets this mesh to be a copy of the given one.
     *
     * All of the contents are copied, so that this mesh does not hold any
     * references to elements of the other mesh. This method returns
     * a reference to this mesh for chaining.
     *
     * @param other  The mesh to copy
     *
     * @return This mesh for chaining
     */
    Mesh& operator=(const Mesh& other) { return set(other); }

    /**
     * Sets this mesh to be have the resources of the given one.
     *
     * It is unsafe to use the original mesh after this method is called.
     *
     * This method returns a reference to this mesh for chaining.
     *
     * @param other  The mesh to take from
     *
     * @return This mesh for chaining
     */
    Mesh& operator=(Mesh&& other) {
        vertices = std::move(other.vertices);
        indices = std::move(other.indices);
        command = other.command;
        return *this;
    }

    /**
     * Sets the mesh to match the {@link Poly2} object.
     *
     * No vertex attribute other than position is set.  Additional
     * information (such as color or texture coordinate) must be added
     * later.  The command will be GL_TRIANGLES if the polygon is solid,
     * GL_LINES if it is a path, and GL_UNDEFINED otherwise.
     *
     * This method returns a reference to this polygon for chaining.
     *
     * @param poly  The polygon defining this mesh
     *
     * @return This polygon, returned for chaining
     */
    Mesh& operator=(const Poly2& poly) { return set(poly); }
    /*
    Mesh& operator=(const Path2& path) { return set(path); }
     */
    /**
     * Sets this mesh to be a copy of the given one.
     *
     * All of the contents are copied, so that this mesh does not hold any
     * references to elements of the other mesh. This method returns
     * a reference to this mesh for chaining.
     *
     * @param other  The mesh to copy
     *
     * @return This mesh for chaining
     */
    Mesh& set(const Mesh& other) {
        vertices = other.vertices;
        indices = other.indices;
        command = other.command;
        return *this;
    }
    
    /**
     * Sets the mesh to match the {@link Poly2} object.
     *
     * No vertex attribute other than position is set.  Additional
     * information (such as color or texture coordinate) must be added
     * later.  The command will be GL_TRIANGLES if the polygon is solid,
     * GL_LINES if it is a path, and GL_UNDEFINED otherwise.
     *
     * This method returns a reference to this polygon for chaining.
     *
     * @param poly  The polygon defining this mesh
     *
     * @return This polygon, returned for chaining
     */
    Mesh& set(const Poly2& poly) {
        for(auto it = poly.vertices.begin(); it != poly.vertices.end(); ++it) {
            T vertex;
            vertex.position = *it;
            vertex.color = Color4::WHITE.getPacked();
            vertices.push_back(vertex);
        }
        for(auto it = poly.indices.begin(); it != poly.indices.end(); ++it) {
            indices.push_back(*it);
        }
        command = GL_TRIANGLES;
        return *this;
    }
     
    /*
    Mesh& set(const Path2& path) {
        vertices.reserve(path.size());
        for(auto it = path.vertices().begin(); it != path.vertices().end(); ++it) {
            T vertex;
            vertex.position = *it;
            vertex.color = Color4::WHITE.getPacked();
            vertices.push_back(vertex);
        }
        indices.reserve(2*path.size());
        for(size_t ii = 0; ii < path.size()-1; ii++) {
            indices.push_back(ii  );
            indices.push_back(ii+1);
        }
        if (path.isClosed()) {
            indices.push_back(path.size()-1);
            indices.push_back(0);
        }
        command = GL_LINES;
        return *this;
    }
    */
    
    /**
     * Sets the mesh to have the given vertices
     *
     * The resulting mesh has no indices.  The command will be
     * reset to GL_UNDEFINED.
     *
     * This method returns a reference to this mesh for chaining.
     *
     * @param verts The vector of vertices (as Vec2) in this mesh
     *
     * @return This mesh for chaining
     */
    Mesh& set(const std::vector<T>& verts) {
        for(auto it = verts.begin(); it != verts.end(); ++it) {
            T vertex;
            vertex.position = *it;
            vertices.push_back(vertex);
        }
        indices.clear();
        command = GL_UNDEFINED;
        return *this;
    }
    
    /**
     * Sets a mesh to have the given vertices and indices.
     *
     * This method will assign a command according to the multiplicity of the
     * indices. If the number of indices n is divisible by 3, it will be
     * GL_TRIANGLES. Otherwise, if it is even, it will be GL_LINES.  All
     * other values will be undefined and the user must manually set the type.
     *
     * This method returns a reference to this mesh for chaining.
     *
     * @param verts The vector of vertices (as floats) in this mesh
     * @param indx  The vector of indices for the rendering
     *
     * @return This mesh for chaining
     */
    Mesh& set(const std::vector<T>& verts, const std::vector<GLuint>& indx) {
        for(auto it = verts.begin(); it != verts.end(); ++it) {
            T vertex;
            vertex.position = *it;
            vertices.push_back(vertex);
        }
        indices = indx;
        command = GL_UNDEFINED;
        if (indx.size() % 3 == 0) {
            command = GL_TRIANGLES;
        } else if (indx.size() % 2 == 0) {
            command = GL_LINES;
        }
        return *this;
    }
    
    
    /**
     * Clears the contents of this mesh and sets the command to GL_LINES
     *
     * @return This mesh for chaining
     */
    Mesh& clear() {
        vertices.clear();
        indices.clear();
        command = GL_LINES;
        return *this;
    }
    
    
#pragma mark -
#pragma mark Operators
    /**
     * Transforms all of the vertices of this mesh.
     *
     * Because we allow meshes to be of arbitrary dimension, the
     * only guaranteed safe transforms are {@link Mat4} objects.
     *
     * @param transform The transform matrix
     *
     * @return This mesh with the vertices transformed
     */
    Mesh& operator*=(const Mat4& transform) {
        for(auto it = vertices.begin(); it != vertices.end(); ++it) {
            it->position *= transform;
        }
        return *this;
    }

    /**
     * Returns a new mesh by transforming the vertices of this one.
     *
     * Because we allow meshes to be of arbitrary dimension, the
     * only guaranteed safe transforms are {@link Mat4} objects.
     *
     * Note: This method does not modify the mesh.
     *
     * @param transform The transform matrix
     *
     * @return The transformed mesh
     */
    Mesh operator*(const Mat4& transform) const {
        return Mesh(*this) *= transform;
    }

    /**
     * Appends the given mesh to this one.
     *
     * The vertices of other are appended to the end of this mesh.
     * The indices are reindex to account for this shift.
     *
     * This method will fail to append to the mesh if other does not
     * share the same command as this mesh.
     *
     * @param other The mesh to append
     *
     * @return This mesh with the new vertices
     */
    Mesh& operator+=(const Mesh& other) {
        if (other.command != command) {
            return *this;
        }
        GLuint size = (GLuint)vertices.size();
        for(auto it = other.vertices.begin(); it != other.vertices.end(); ++it) {
            vertices.push_back(*it);
        }
        for(auto it = other.indices.begin(); it != other.indices.end(); ++it) {
            indices.push_back(*it+size);
        }
        return *this;
    }

    /**
     * Return the concatenation of this mesh and other.
     *
     * The vertices of other are appended to the end of this mesh.
     * The indices are reindex to account for this shift.
     *
     * This method will fail to append to the mesh if other does not
     * share the same command as this mesh.
     *
     * Note: This method does not modify the mesh.
     *
     * @param other The mesh to concatenate
     *
     * @return the concatenation of this mesh and other.
     */
    Mesh operator+(const Mesh& other) const {
        return Mesh(*this) += other;
    }
    
#pragma mark -
#pragma mark Slicing
    /**
     * Returns true is this mesh is sliceable.
     *
     * The only sliceable mesh types are GL_LINES and GL_TRIANGLES.  That
     * is because the mesh is represented in regular, decomposable chunks.
     * This method not only checks that the command is correct, but that
     * the index size is correct as well.
     *
     * @return true is this mesh is sliceable.
     */
    bool isSliceable() const {
        if (command == GL_LINES) {
            return indices.size() % 2 == 0;
        } else if (command == GL_TRIANGLES) {
            return indices.size() % 3 == 0;
        }
        return false;
    }

    /**
     * Returns the slice of this mesh between start and end.
     *
     * The sliced mesh will use the indices from start to end (not including
     * end). It will include the vertices referenced by those indices, and
     * only those vertices. The command will remain the same.
     *
     * The only sliceable mesh types are GL_LINES and GL_TRIANGLES.  That
     * is because the mesh is represented in regular, decomposable chunks.
     * Any attempt to slice another mesh type will fail.
     *
     * @param start The start index
     * @param end   The end index
     *
     * @return the slice of this mesh between start and end.
     */
    Mesh slice(size_t start, size_t end) const  {
        CUAssertLog(command == GL_LINES || command == GL_TRIANGLES, "Mesh is not a sliceable type");
        
        int divider = command == GL_TRIANGLES ? 3 : 2;
        CUAssertLog(start % divider == 3 && start < indices.size(), "Start position %d is not a valid slice point", start);
        CUAssertLog(end % divider == 3 && end <= indices.size(), "End position %d is not a valid slice point", end);

        std::unordered_map<GLuint,GLuint> visited;
        std::vector<T> verts;
        std::vector<GLuint> indx;
        for(size_t ii = start; ii < end; ii++) {
            auto find = visited.find(indices[ii]);
            if (find != visited.end()) {
                indx.push_back(find->second);
            } else {
                visited[indices[ii]] = (GLuint)verts.size();
                verts.push_back(vertices[indices[ii]]);
            }
        }
        
        Mesh result(verts,indx);
        result.command = command;
        return result;
    }

    /**
     * Returns the slice of this mesh from the start index to the end.
     *
     * The sliced mesh will use the indices from start to the end. It will
     * include the vertices referenced by those indices, and only those
     * vertices.  The command will remain the same.
     *
     * The only sliceable mesh types are GL_LINES and GL_TRIANGLES. That
     * is because the mesh is represented in regular, decomposable chunks.
     * Any attempt to slice another mesh type will fail.
     *
     * @param start The start index
     *
     * @return the slice of this mesh between start and end.
     */
    Mesh sliceFrom(size_t start) const  {
        return slice(start,indices.size());
    }

    /**
     * Returns the slice of this mesh from the begining to end.
     *
     * The sliced mesh will use the indices up to  (but not including) end.
     * It will include the vertices referenced by those indices, and only
     * those vertices. The command will remain the same.
     *
     * The only sliceable mesh types are GL_LINES and GL_TRIANGLES.  That
     * is because the mesh is represented in regular, decomposable chunks.
     * Any attempt to slice another mesh type will fail.
     *
     * @param end   The end index
     *
     * @return the slice of this mesh between start and end.
     */
    Mesh sliceTo(size_t end) const  {
        return slice(0,end);
    }

};

}

#endif /* __CU_MESH_H__ */
