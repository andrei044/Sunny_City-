#include "Camera.hpp"

namespace gps {

    //Camera constructor
    Camera::Camera(glm::vec3 cameraPosition, glm::vec3 cameraTarget, glm::vec3 worldUp) {
        //TODO
        this->cameraPosition = cameraPosition;
        this->cameraTarget = cameraTarget;
        this->cameraUpDirection = worldUp;
        this->cameraFrontDirection = glm::normalize(-this->cameraPosition + this->cameraTarget);
        this->cameraRightDirection = glm::normalize(glm::cross(worldUp,this->cameraFrontDirection));
    }

    //return the view matrix, using the glm::lookAt() function
    glm::mat4 Camera::getViewMatrix() {
        return glm::lookAt(this->cameraPosition, this->cameraTarget, this->cameraUpDirection);
    }

    //update the camera internal parameters following a camera move event
    void Camera::move(MOVE_DIRECTION direction, float speed) {
        if (direction == MOVE_FORWARD) {
            this->cameraPosition += speed * this->cameraFrontDirection;
            this->cameraTarget += speed * this->cameraFrontDirection;
        }
        if (direction == MOVE_BACKWARD) {
            this->cameraPosition -= speed * this->cameraFrontDirection;
            this->cameraTarget -= speed * this->cameraFrontDirection;
        }
        if (direction == MOVE_LEFT) {
            this->cameraPosition -= glm::normalize(glm::cross(this->cameraFrontDirection, this->cameraUpDirection)) * speed;
            this->cameraTarget -= glm::normalize(glm::cross(this->cameraFrontDirection, this->cameraUpDirection)) * speed;
        }
        if (direction == MOVE_RIGHT) {
            this->cameraPosition += glm::normalize(glm::cross(this->cameraFrontDirection, this->cameraUpDirection)) * speed;
            this->cameraTarget += glm::normalize(glm::cross(this->cameraFrontDirection, this->cameraUpDirection)) * speed;
        }
    }

    //update the camera internal parameters following a camera rotate event
    //yaw - camera rotation around the y axis
    //pitch - camera rotation around the x axis
    void Camera::rotate(float pitch, float yaw) {
        glm::vec3 direction;
        direction.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
        direction.y = sin(glm::radians(pitch));
        direction.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
        this->cameraFrontDirection = glm::normalize(direction);
        this->cameraRightDirection = glm::normalize(glm::cross(this->cameraUpDirection, this->cameraFrontDirection));
        this->cameraTarget = this->cameraPosition + this->cameraFrontDirection;
    }

    glm::vec3 Camera::getPosition() {
        return this->cameraPosition;
    }
}
