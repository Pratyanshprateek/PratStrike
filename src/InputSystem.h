#pragma once

#include <algorithm>

#include <SDL.h>

#include "Components.h"
#include "ECS.h"
#include "Math.h"

class InputSystem
{
public:
    void update(Registry& reg, std::uint32_t playerEntityId, SDL_Renderer* renderer, Vector2 cameraPos, float dt)
    {
        (void)renderer;

        if (!reg.isAlive(playerEntityId))
        {
            return;
        }

        const Uint8* keys = SDL_GetKeyboardState(nullptr);
        TransformComponent& tf = reg.getComponent<TransformComponent>(playerEntityId);
        VelocityComponent& vel = reg.getComponent<VelocityComponent>(playerEntityId);
        PlayerStateComponent& ps = reg.getComponent<PlayerStateComponent>(playerEntityId);

        ps.wantsToFire = false;

        const float moveX = (keys[SDL_SCANCODE_D] != 0U || keys[SDL_SCANCODE_RIGHT] != 0U ? 1.0f : 0.0f) -
                            (keys[SDL_SCANCODE_A] != 0U || keys[SDL_SCANCODE_LEFT] != 0U ? 1.0f : 0.0f);
        const float moveY = (keys[SDL_SCANCODE_S] != 0U || keys[SDL_SCANCODE_DOWN] != 0U ? 1.0f : 0.0f) -
                            (keys[SDL_SCANCODE_W] != 0U || keys[SDL_SCANCODE_UP] != 0U ? 1.0f : 0.0f);
        const Vector2 moveInput(moveX, moveY);
        const Vector2 moveDirection = moveInput.lengthSq() > 0.0f ? moveInput.normalized() : Vector2();

        if (ps.dashCooldown > 0.0f)
        {
            ps.dashCooldown = std::max(0.0f, ps.dashCooldown - dt);
        }

        if (keys[SDL_SCANCODE_LSHIFT] != 0U && ps.dashCooldown <= 0.0f && !ps.isDashing)
        {
            ps.isDashing = true;
            ps.dashTimer = ps.dashDuration;
            ps.dashCooldown = ps.dashCooldownMax;
            ps.dashDirection = moveDirection.lengthSq() > 0.0f ? moveDirection : Vector2(0.0f, -1.0f).rotated(tf.rotation);
        }

        if (ps.isDashing)
        {
            ps.dashTimer -= dt;
            vel.velocity = ps.dashDirection * 600.0f;
            vel.speed = 600.0f;

            if (ps.dashTimer <= 0.0f)
            {
                ps.dashTimer = 0.0f;
                ps.isDashing = false;
            }
        }
        else if (moveDirection.lengthSq() > 0.0f)
        {
            vel.velocity = moveDirection * vel.maxSpeed;
            vel.speed = vel.maxSpeed;
        }
        else
        {
            vel.velocity = Vector2();
            vel.speed = 0.0f;
        }

        ps.fireCooldown = std::max(0.0f, ps.fireCooldown - dt);

        int mouseX = 0;
        int mouseY = 0;
        const Uint32 mouseButtons = SDL_GetMouseState(&mouseX, &mouseY);
        if (((mouseButtons & SDL_BUTTON(SDL_BUTTON_LEFT)) != 0U || keys[SDL_SCANCODE_SPACE] != 0U) && ps.fireCooldown <= 0.0f)
        {
            ps.fireCooldown = ps.fireRate;
            ps.wantsToFire = true;
        }

        const Vector2 mouseWorldPos(static_cast<float>(mouseX) + cameraPos.x, static_cast<float>(mouseY) + cameraPos.y);
        const Vector2 aimDirection = mouseWorldPos - tf.position;
        if (aimDirection.length() > 5.0f)
        {
            tf.rotation = aimDirection.angle();
        }
        else if (moveDirection.lengthSq() > 0.0f)
        {
            tf.rotation = moveDirection.angle();
        }
    }
};
