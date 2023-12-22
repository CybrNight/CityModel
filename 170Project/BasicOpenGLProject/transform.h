#pragma once
#include "mesh.h"
#include "glm/glm.hpp"
#include "model.h"

class Transform
{
public:
    glm::vec3 position;
    glm::vec3 oldPos;

    glm::vec3 _scale;
    glm::vec3 center;

    Transform();
    Transform(Mesh* mesh, glm::vec3 position);
    Transform(Model model);
    void translate(glm::vec3 location);
    void translate(float x, float y, float z);
    void setPosition(glm::vec3 location);
    void setPosition(float x, float y, float z);
    void scale(float x, float y, float z);
    void scale(glm::vec3 scale);
    void scale(float s);
    void rotate(float degrees, glm::vec3 axis);
    void addMesh(Mesh* mesh);
    glm::mat4 getModelMatrix();
    void setModelMatrix(glm::mat4 matrix);
    void loadIdentity();
    void draw(ShaderProgram shader);
    void calculateCenter();
    float getSurface(glm::vec3 a, glm::vec3 b, glm::vec3 c);
    bool intersects(Transform transform);

    ~Transform() {

    }

private:
    std::vector<Mesh*> meshes;
    glm::mat4 modelMatrix;
    glm::mat4 translateMatrix;
    glm::mat4 rotationMatrix;
    glm::mat4 scaleMatrix;
};

