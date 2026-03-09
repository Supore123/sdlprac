



#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
/**
 * Camera
 *
 * Provides view and projection matrices for both 3D (perspective) and
 * 2D/UI (orthographic) rendering.
 *
 * ── Perspective (3D) ────────────────────────────────────────────────────────
 *   Set mode to Perspective. Drive it either by:
 *     setPosition / setTarget           — look-at style (cutscenes, fixed cams)
 *     processKeyboard / processMouse    — FPS free-look style
 *
 * ── Orthographic (2D / UI) ──────────────────────────────────────────────────
 *   Set mode to Orthographic. Coordinates map 1:1 to screen pixels when
 *   you call setOrthoSize(screenWidth, screenHeight).
 *
 * ── Usage ───────────────────────────────────────────────────────────────────
 *   Camera cam;
 *   cam.setMode(Camera::Mode::Perspective);
 *   cam.setPerspective(45.f, 1280.f / 720.f, 0.1f, 1000.f);
 *   cam.setPosition({0, 2, 5});
 *   cam.setTarget ({0, 0, 0});
 *
 *   // In render:
 *   ShaderManager::setUniform(prog, "uView",       cam.getView());
 *   ShaderManager::setUniform(prog, "uProjection", cam.getProjection());
 *
 * ── FPS mode ────────────────────────────────────────────────────────────────
 *   cam.processMouse(xDelta, yDelta);        // call with InputManager delta
 *   cam.processKeyboard(dir, dt);
 */
class Camera
{
public:
    enum class Mode { Perspective, Orthographic };

    // Movement directions for processKeyboard()
    enum class MoveDir { Forward, Backward, Left, Right, Up, Down };

    // ------------------------------------------------------------------ //
    //  Construction                                                        //
    // ------------------------------------------------------------------ //

    Camera();
    // ------------------------------------------------------------------ //
    //  Mode                                                                //
    // ------------------------------------------------------------------ //

    void setMode(Mode mode);
    Mode getMode() const { return m_mode; }

    // ------------------------------------------------------------------ //
    //  Projection setup                                                    //
    // ------------------------------------------------------------------ //

    /**
     * Configure a perspective projection.
     * @param fovDegrees   Vertical field of view in degrees
     * @param aspect       width / height
     * @param nearPlane    Near clip distance (> 0)
     * @param farPlane     Far clip distance
     */
    void setPerspective(float fovDegrees, float aspect,
                        float nearPlane = 0.1f, float farPlane = 1000.f);

    /**
     * Configure an orthographic projection sized to pixel coordinates.
     * Call this whenever the window is resized.
     * Origin is top-left; +Y goes down (matches SDL/screen conventions).
     */
    void setOrthoSize(float width, float height);

    /** Update the aspect ratio after a window resize (perspective only). */
    void setAspect(float aspect);

    // ------------------------------------------------------------------ //
    //  View (look-at)                                                      //
    // ------------------------------------------------------------------ //

    void setPosition(const glm::vec3& pos);
    void setTarget  (const glm::vec3& target);
    void setUp      (const glm::vec3& up);

    const glm::vec3& getPosition() const { return m_position; }
    const glm::vec3& getFront()    const { return m_front;    }
    const glm::vec3& getRight()    const { return m_right;    }

    // ------------------------------------------------------------------ //
    //  FPS controls                                                        //
    // ------------------------------------------------------------------ //

    /**
     * Rotate camera from mouse delta (pixels).
     * @param sensitivity  Degrees-per-pixel scaling factor (default 0.1)
     */
    void processMouse(float xDelta, float yDelta,
                      float sensitivity = 0.1f);

    /**
     * Translate camera along its local axes.
     * @param speed  Units per second
     */
    void processKeyboard(MoveDir direction, float dt, float speed = 5.f);

    /** Clamp pitch to ±89° to avoid gimbal flip. Enabled by default. */
    void setConstrainPitch(bool constrain) { m_constrainPitch = constrain; }

    // ------------------------------------------------------------------ //
    //  Matrix accessors                                                    //
    // ------------------------------------------------------------------ //

    const glm::mat4& getView()       const { return m_view;       }
    const glm::mat4& getProjection() const { return m_projection; }

    /** VP = Projection * View — convenience for shaders that want one matrix. */
    glm::mat4 getViewProjection() const { return m_projection * m_view; }

    // --- helpers ----------------------------------------------------------
    float getOrthoWidth() const { return m_orthoWidth; }
    float getOrthoHeight() const { return m_orthoHeight; }
private:
    Mode m_mode = Mode::Perspective;

    // Perspective params
    float m_fov    = 45.f;
    float m_aspect = 16.f / 9.f;
    float m_near   = 0.1f;
    float m_far    = 1000.f;

    // Ortho params
    float m_orthoWidth  = 1280.f;
    float m_orthoHeight = 720.f;

    // View params
    glm::vec3 m_position = { 0.f,  0.f,  3.f };
    glm::vec3 m_front    = { 0.f,  0.f, -1.f };
    glm::vec3 m_up       = { 0.f,  1.f,  0.f };
    glm::vec3 m_right    = { 1.f,  0.f,  0.f };

    // Euler angles (degrees) for FPS mode
    float m_yaw   = -90.f;
    float m_pitch =   0.f;
    bool  m_constrainPitch = true;

    // Cached matrices
    glm::mat4 m_view       = glm::mat4(1.f);
    glm::mat4 m_projection = glm::mat4(1.f);

    void recalculateView();
    void recalculateProjection();
    void updateVectors();   // recompute front/right/up from yaw+pitch
};
