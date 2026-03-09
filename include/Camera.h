



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
