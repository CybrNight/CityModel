#pragma once
#include <GL/glew.h>
#include <GL/freeglut.h>
#include <glm/glm.hpp>
#include <glm/ext.hpp>
#include <string>
#include <vector>
#include "shaderprogram.h"

struct Vertex {
    glm::vec3 position;
    glm::vec4 color;
    glm::vec3 normal;

    glm::vec2 texCoords;
};

struct Texture {
    GLuint texId;
    std::string type;
    std::string path;

    Texture(GLuint texId, std::string type) {
        this->texId = texId;
        this->type = type;
        this->path = path;
    }
    Texture() {

    }
};

class Mesh
{
public:
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    std::vector<Texture> textures;
    glm::vec3 center;

    Mesh(std::vector<Vertex> vertices, std::vector<unsigned int> indices, std::vector<Texture> textures);

    ~Mesh() {

    }

    void draw(ShaderProgram shader, glm::mat4 matrix);

private:
    GLuint VAO;
    GLuint VBO;
    GLuint EBO;

    void setupMesh();
       
};

