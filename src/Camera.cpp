#include "Camera.h"
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include <cmath>

// -------------------------------------------------------------------------- //
//  Construction                                                               //
// -------------------------------------------------------------------------- //

Camera::Camera()
{
    updateVectors();
    recalculateView();
    recalculateProjection();
}

// -------------------------------------------------------------------------- //
//  Mode                                                                       //
// -------------------------------------------------------------------------- //

void Camera::setMode(Mode mode)
{
    m_mode = mode;
    recalculateProjection();
}

// -------------------------------------------------------------------------- //
//  Projection setup                                                           //
// -------------------------------------------------------------------------- //

void Camera::setPerspective(float fovDegrees, float aspect,
                            float nearPlane, float farPlane)
{
    m_fov    = fovDegrees;
    m_aspect = aspect;
    m_near   = nearPlane;
    m_far    = farPlane;
    recalculateProjection();
}

void Camera::setOrthoSize(float width, float height)
{
    m_orthoWidth  = width;
    m_orthoHeight = height;
    recalculateProjection();
}

void Camera::setAspect(float aspect)
{
    m_aspect = aspect;
    recalculateProjection();
}

// -------------------------------------------------------------------------- //
//  View (look-at)                                                             //
// -------------------------------------------------------------------------- //

void Camera::setPosition(const glm::vec3& pos)
{
    m_position = pos;
    recalculateView();
}

void Camera::setTarget(const glm::vec3& target)
{
    m_front = glm::normalize(target - m_position);
    m_right = glm::normalize(glm::cross(m_front, m_up));

    // Derive yaw/pitch from the new front vector so FPS controls stay in sync
    m_pitch = glm::degrees(std::asin(m_front.y));
    m_yaw   = glm::degrees(std::atan2(m_front.z, m_front.x));

    recalculateView();
}

void Camera::setUp(const glm::vec3& up)
{
    m_up = up;
    recalculateView();
}

// -------------------------------------------------------------------------- //
//  FPS controls                                                               //
// -------------------------------------------------------------------------- //

void Camera::processMouse(float xDelta, float yDelta, float sensitivity)
{
    m_yaw   += xDelta * sensitivity;
    m_pitch -= yDelta * sensitivity;   // subtract: moving mouse up should pitch up

    if (m_constrainPitch)
        m_pitch = std::clamp(m_pitch, -89.f, 89.f);

    updateVectors();
    recalculateView();
}

void Camera::processKeyboard(MoveDir direction, float dt, float speed)
{
    float velocity = speed * dt;

    switch (direction)
    {
        case MoveDir::Forward:  m_position += m_front * velocity;  break;
        case MoveDir::Backward: m_position -= m_front * velocity;  break;
        case MoveDir::Left:     m_position -= m_right * velocity;  break;
        case MoveDir::Right:    m_position += m_right * velocity;  break;
        case MoveDir::Up:       m_position += m_up    * velocity;  break;
        case MoveDir::Down:     m_position -= m_up    * velocity;  break;
    }

    recalculateView();
}

// -------------------------------------------------------------------------- //
//  Private helpers                                                            //
// -------------------------------------------------------------------------- //

void Camera::updateVectors()
{
    float yawRad   = glm::radians(m_yaw);
    float pitchRad = glm::radians(m_pitch);

    glm::vec3 front;
    front.x = std::cos(yawRad) * std::cos(pitchRad);
    front.y = std::sin(pitchRad);
    front.z = std::sin(yawRad) * std::cos(pitchRad);

    m_front = glm::normalize(front);
    m_right = glm::normalize(glm::cross(m_front, glm::vec3(0.f, 1.f, 0.f)));
    m_up    = glm::normalize(glm::cross(m_right, m_front));
}

void Camera::recalculateView()
{
    m_view = glm::lookAt(m_position, m_position + m_front, m_up);
}

void Camera::recalculateProjection()
{
    if (m_mode == Mode::Perspective)
    {
        m_projection = glm::perspective(
            glm::radians(m_fov), m_aspect, m_near, m_far);
    }
    else
    {
        // Origin top-left, +Y down — matches screen/SDL coordinate conventions
        m_projection = glm::ortho(
            0.f, m_orthoWidth,
            m_orthoHeight, 0.f,
            -1.f, 1.f);
    }
}
