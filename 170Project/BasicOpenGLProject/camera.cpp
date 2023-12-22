#include "camera.h"
#include <iostream>

Camera::Camera()
{
    position = glm::vec3(0.0, 50.0, 0.0f);
    front = glm::vec3(0.0, 0.0, -1.0f);
    up = glm::vec3(0.0, 1.0, 0.0f);
	worldUp = up;
    right = glm::cross(front, up);
    yaw = -90.0f;
    pitch = 0.0f;
	update();
}

glm::mat4 Camera::getViewMatrix() {
	return glm::lookAt(position, position + front, up);
}

void Camera::update() {
	glm::vec3 direction;
	direction.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
	direction.y = sin(glm::radians(pitch));
	direction.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));

	front = glm::normalize(direction);
	right = glm::normalize(glm::cross(front, worldUp));
	up = glm::normalize(glm::cross(right, front));
}

void Camera::processMouseMovement(float xOffset, float yOffset)
{

	float sens = 0.1f;
	xOffset *= sens;
	yOffset *= sens;

	yaw += xOffset;
	pitch += yOffset;

	update();
	
}

void Camera::setForward(glm::vec3 front)
{
	this->forward = position + front;
}
