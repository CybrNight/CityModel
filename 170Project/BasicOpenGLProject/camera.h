#pragma once
#include "glm/glm.hpp"
#include <glm/ext.hpp>

class Camera
{
public:
    glm::vec3 position;
    glm::vec3 front;
    glm::vec3 forward;
    glm::vec3 up;
    glm::vec3 worldUp;

    float yaw;
    float pitch;


    glm::vec3 right;
    glm::mat4 view;

    Camera();

    void update();
    void processMouseMovement(float xoFfset, float yOffset);
    void setForward(glm::vec3 forward);
    glm::mat4 getViewMatrix();
};

