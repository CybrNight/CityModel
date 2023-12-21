#include "transform.h"
#include <float.h>
#include <iostream>
#include "model.h"

Transform::Transform() {
    center = glm::vec3(0, 0, 0);
    position = glm::vec3(0, 0, 0);
    modelMatrix = glm::mat4(1);
    rotationMatrix = glm::mat4(1);
    translateMatrix = glm::mat4(1);
    scaleMatrix = glm::mat4(1);
    _scale = glm::vec3(1);
}

Transform::Transform(Model model) : Transform() {
    for (auto i = model.meshes.begin(); i != model.meshes.end(); i++) {
        meshes.push_back(*i);
    }
    calculateCenter();
}

void Transform::translate(float x, float y, float z) {
    translate(glm::vec3(x, y, z));
}

void Transform::setPosition(float x, float y, float z)
{
    setPosition(glm::vec3(x, y, z));
}

void Transform::setPosition(glm::vec3 position)
{
    translateMatrix = glm::mat4(1);
    this->position = glm::vec3(0, 0, 0);
    translate(position);
}


void Transform::scale(float x, float y, float z)
{

    scale(glm::vec3(x, y, z));
}

void Transform::scale(glm::vec3 scale) {
    _scale = scale/300.0f;
    translate(-position);
    scaleMatrix = glm::scale(scaleMatrix, _scale);
    translate(position);
}

void Transform::scale(float s) {
    scale(s, s, s);
}

void Transform::rotate(float degrees, glm::vec3 axis) {
    translate(-position);
    rotationMatrix = glm::rotate(rotationMatrix, glm::radians(degrees), axis);
    translate(position);
}


void Transform::translate(glm::vec3 position) {
    this->position += position/300.0f;
    translateMatrix = glm::translate(translateMatrix, position);
}

void Transform::addMesh(Mesh* mesh) {
    meshes.push_back(mesh);

    modelMatrix = glm::mat4(1.0f);
    //modelMatrix = glm::translate(modelMatrix, mesh->center);
}

void Transform::calculateCenter() {
    glm::vec3 minPositions = glm::vec3(0, 0, 0);
    glm::vec3 maxPositions = glm::vec3(0, 0, 0);
    
    //Iterate over all meshes in model
    for (int i = 0; i < meshes.size(); i++) {
        Mesh mesh = *meshes[i];

        //Iterate over all vertices in the mesh
        for (auto vert = mesh.vertices.begin(); vert != mesh.vertices.end(); vert++) {
            glm::vec3 pos = vert->position;

            //Get smallest x value in model
            if (pos.x < minPositions.x) {
                minPositions.x = pos.x;
            }

            if (pos.y < minPositions.y) {
                minPositions.y = pos.y;
            }

            if (pos.z < minPositions.z) {
                minPositions.z = pos.z;
            }

            //Get highest x value in model
            if (pos.x > maxPositions.x) {
                maxPositions.x = pos.x;
            }

            if (pos.y > maxPositions.y) {
                maxPositions.y = pos.y;
            }

            if (pos.z > maxPositions.z) {
                maxPositions.z = pos.z;
            }
        }
    }

    center = ((maxPositions - minPositions) / 2.0f) + minPositions;
    center /= 300;
    std::cout << center.x << std::endl;
    setPosition(center);
}

glm::mat4 Transform::getModelMatrix() {
    return translateMatrix * rotationMatrix * scaleMatrix;
}

void Transform::setModelMatrix(glm::mat4 matrix){

    modelMatrix = matrix;
}

void Transform::loadIdentity() {
    modelMatrix = glm::mat4(1.0f);
}

void Transform::draw(ShaderProgram shader){
    glm::vec3 pos = position - oldPos;

    for (int i = 0; i < meshes.size(); i++) {
        meshes[i]->draw(shader, getModelMatrix());
    }
}


float Transform::getSurface(glm::vec3 a, glm::vec3 b, glm::vec3 c){
    float x1 = a.x - b.x;
    float x2 = a.y - b.y;
    float x3 = a.z - b.z;
    float y1 = a.x - c.x;
    float y2 = a.y - c.y;
    float y3 = a.z - c.z;

    return sqrt(
        pow(x2 * y3 - x3 * y2, 2) +
        pow(x3 * y1 - x1 * y3, 2) +
        pow(x1 * y2 - x2 * y1, 2)
    ) / 2.0f;
}

bool Transform::intersects(Transform transform) {
    glm::vec3 otherPos = transform.position;  
    glm::vec3 otherMax = glm::vec3(otherPos.x+transform._scale.x/2, otherPos.y + transform._scale.y / 2, otherPos.z + transform._scale.z / 2);
    glm::vec3 otherMin = glm::vec3(otherPos.x - transform._scale.x / 2, otherPos.y - transform._scale.y / 2, otherPos.z - transform._scale.z / 2);


    glm::vec3 max = glm::vec3(position.x + _scale.x / 2, position.y + _scale.y / 2, position.z + _scale.z / 2);
    glm::vec3 min = glm::vec3(position.x - _scale.x / 2, position.y - _scale.y / 2, position.z - _scale.z / 2);

    if (max.x > otherMin.x && min.x < otherMax.x) {
        if (max.y > otherMin.y && min.y < otherMax.y) {
            if (max.z > otherMin.z && min.z < otherMax.z) {
                return true;
            }
        }
    }

    return false;
}