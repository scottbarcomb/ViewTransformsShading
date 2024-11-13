#ifndef MESH_H
#define MESH_H

#define GLEW_STATIC
#include <GL/glew.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/string_cast.hpp>

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <vector>

class Mesh
{
public:
    // the vertex array object, vertex buffor object, and element buffer object
    unsigned int VAO, VBO, EBO;
    // the storage container for the vertices
    std::vector<glm::vec3> vertices;
    // the storage container for the normals
    std::vector<glm::vec3> normals;
    // the storage container for our faces (contains the triangle vertex indices)
    std::vector<glm::ivec3> faces;
     // the storage container for our triangles
    std::vector<glm::vec3> triangles; // contains triangle EBO vertices plus their normal vectors
    // the container of transformed triangles to be returned (for use with CPU transformations)
    std::vector<glm::vec3> transformedTriangles;

    // keep track of largest vertex for initial scaling on load-up
    glm::vec3 largestVertex;

    // constructor reads .obj file and builds the VBO and VAO
    Mesh(const char* objPath);
    void load(); // initializes the buffers by binding them and doing other OpenGL stuff
    void render(); // draws the arrays
    void unload(); // unbinds and deletes objects
    void applyTransform(glm::mat4 transform); // for use with CPU transformations
private:
    // triangulation method used to split non-triangular object faces into triangles
    void triangulate(std::vector<int> vertIndices, std::vector<int> normIndices, std::vector<glm::ivec3>& faces);
};

Mesh::Mesh(const char* objPath)
{
    // 1. retrieve the raw data from the obj file
    std::ifstream objFile;
    // open file
    objFile.open(objPath);
    std::string line;
    // initialize largest vertex
    largestVertex = glm::vec3(0.0);

    while (std::getline(objFile, line))
    {
        // check v for vertices
        if (line.substr(0,2) == "v ")
        {
            std::istringstream v(line.substr(2));
            glm::vec3 vert;
            double x, y, z;
            v >> x, v >> y, v >> z;
            vert = glm::vec3(x, y, z);
            vertices.push_back(vert);

            // check for largest vertex
            if (glm::length(vert) > glm::length(largestVertex))
                largestVertex = vert;
        }

        // check vn for normals
        else if (line.substr(0, 2) == "vn")
        {
            std::istringstream vn(line.substr(2));
            glm::vec3 norm;
            double x, y, z;
            vn >> x, vn >> y, vn >> z;
            norm = glm::vec3(x, y, z);
            normals.push_back(norm);
        }

        // check f for faces
        else if (line.substr(0,2) == "f ")
        {
            std::istringstream f(line.substr(2));
            std::string v;
            std::string::size_type pos;
            std::vector<int> faceIndices;
            std::vector<int> normIndices;

            while (f >> v)
            {
                pos = v.find('/');
                int idx = stoi(v.substr(0, pos));
                idx--;
                faceIndices.push_back(idx);

                std::string::size_type oldPos = pos;
                pos = v.find(' ', oldPos);
                oldPos += 2;
                idx = stoi(v.substr(oldPos, pos));
                idx--;
                normIndices.push_back(idx);
            }

            triangulate(faceIndices, normIndices, faces);
        }
    }

    // 2. set up mesh data
    // add each vertex and corresponding normal to the triangles storage container

    for (unsigned int i = 0; i < faces.size() / 2; i++)
    {
        int idx = i * 2;
        glm::ivec3 triangle = faces.at(idx);
        glm::ivec3 normal = faces.at(idx+1);
        triangles.push_back(vertices.at(triangle.x));
        triangles.push_back(normals.at(normal.x));
        triangles.push_back(vertices.at(triangle.y));
        triangles.push_back(normals.at(normal.y));
        triangles.push_back(vertices.at(triangle.z));
        triangles.push_back(normals.at(normal.z));
    }

    // initialize the transformed triangles container (not used when GPU is doing the work)
    transformedTriangles = triangles;
}

void Mesh::load()
{
    // 3. set up vertex buffers
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    //glGenBuffers(1, &EBO);
    // bind the Vertex Array Object first, then bind and set vertex buffer(s), and then configure vertex attributes(s).
    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    //glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    
    glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3) * triangles.size(), &triangles[0].x, GL_STATIC_DRAW);
    //glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(glm::ivec3) * faces.size(), &faces[0].x, GL_STATIC_DRAW);

    // position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // normal attribute
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // bind the buffers
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    //glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);

    // bind the vertex array
    glBindVertexArray(0);
}

void Mesh::render()
{
    glBindVertexArray(VAO); // seeing as we only have a single VAO there's no need to bind it every time, but we'll do so to keep things a bit more organized
    glDrawArrays(GL_TRIANGLES, 0, triangles.size() * 3);
    glBindVertexArray(0); // unbind our VA no need to unbind it every time 
}

void Mesh::unload()
{
    // de-allocate all resources once they've outlived their purpose:
    // --------------------------------------------------------------
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
}

void Mesh::applyTransform(glm::mat4 transform)
{    
    // for each unique vertex in the array
    for (unsigned int i = 0; i < triangles.size() / 2; i++)
    {
        // grab vertex and normalize it to 1
        glm::vec4 vertex = glm::vec4(triangles.at(i*2), 1.0);

        // apply transformation to vertex
        glm::vec4 newVertex = transform * vertex;

        // update the transformation container
        transformedTriangles.at(i*2) = newVertex;
    }
    // reload VBO and VAO for re-rendering
    load();
}

void Mesh::triangulate(std::vector<int> vertIndices, std::vector<int> normIndices, std::vector<glm::ivec3>& faces)
{
    glm::ivec3 face;
    glm::ivec3 prevFace;
    glm::ivec3 norm;
    glm::ivec3 prevNorm;
    int count = 0;

    // 1. Set up initial triangle with normals
    int v1, v2, v3, n1, n2, n3;
    v1 = vertIndices.at(0);
    v2 = vertIndices.at(1);
    v3 = vertIndices.at(2);
    n1 = normIndices.at(0);
    n2 = normIndices.at(1);
    n3 = normIndices.at(2);
    face = glm::ivec3(v1, v2, v3);
    norm = glm::ivec3(n1, n2, n3);
    faces.push_back(face);
    faces.push_back(norm);
    prevFace = face;
    prevNorm = norm;

    // 2. Go through every new triangle and trace triangles
    // there are n-2 triangles in a polygon with n vertices
    int numTriangles = vertIndices.size() - 2;
    // we already found one triangle
    numTriangles--;
    for (unsigned int i = 0; i < numTriangles; i++)
    {
        // index 1 = previous index 3
        // index 2 = previous index 1
        v1 = prevFace.z;
        v2 = prevFace.x;

        n1 = prevNorm.z;
        n2 = prevNorm.x;

        // index 3 = vertex n - count when i is even
        // index 3 = vertex 4 + count when i is odd
        // count starts at zero
        int idx;
        if (i % 2) // if i is odd
        {
            idx = 4 + count - 1;
            count++;
        }
        else // i is even
        {
            idx = vertIndices.size() - count - 1;
        }
        v3 = vertIndices.at(idx);
        n3 = normIndices.at(idx);
        
        // Create face and add to faces vector
        face = glm::ivec3(v1, v2, v3);
        norm = glm::ivec3(n1, n2, n3);
        faces.push_back(face);
        faces.push_back(norm);
        prevFace = face;
        prevNorm = norm;
    }
}

#endif