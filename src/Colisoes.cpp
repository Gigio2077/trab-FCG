// Arquivo: Colisoes.cpp

#include "Colisoes.h"
#include "game_objects.h"

#include <glm/glm.hpp>
#include <glm/gtx/norm.hpp>
#include <iostream>
#include <algorithm> // Para glm::clamp

// Constantes físicas e da mesa (copiadas da main.cpp para tornar Colisoes.cpp autossuficiente)
const float BALL_Y_AXIS = -0.2667f;
const float BALL_VIRTUAL_RADIUS = 0.02625f;
const float TABLE_WIDTH = 1.0415f;
const float TABLE_DEPTH = 2.2845f;
const float TABLE_HALF_WIDTH = TABLE_WIDTH / 2;
const float TABLE_HALF_DEPTH = TABLE_DEPTH / 2;
const float GRID_CELL_SIZE = BALL_VIRTUAL_RADIUS * 4.0f;
const int GRID_COLS = static_cast<int>(TABLE_WIDTH / GRID_CELL_SIZE) + 1;
const int GRID_ROWS = static_cast<int>(TABLE_DEPTH / GRID_CELL_SIZE) + 1;
const float FELT_SURFACE_Y_ACTUAL = BALL_Y_AXIS - BALL_VIRTUAL_RADIUS;
const float GRAVITY = 9.8f;
const float RESTITUTION_COEFF = 0.8f;
const float BALL_FRICTION_FACTOR = 0.99f;
const float VELOCITY_STOP_THRESHOLD = 0.01f;
const float FIXED_PHYSICS_DELTA_TIME = 1.0f / 120.0f;


void updateSpatialGrid(std::vector<GameBall>& balls, std::vector<std::vector<std::vector<size_t>>>& spatialGrid) {
    spatialGrid.assign(GRID_COLS, std::vector<std::vector<size_t>>(GRID_ROWS));
    for (size_t i = 0; i < balls.size(); ++i) {
        if (!balls[i].active) continue;
        int col = static_cast<int>((balls[i].position.x + TABLE_HALF_WIDTH) / GRID_CELL_SIZE);
        int row = static_cast<int>((balls[i].position.z + TABLE_HALF_DEPTH) / GRID_CELL_SIZE);
        col = glm::clamp(col, 0, GRID_COLS - 1);
        row = glm::clamp(row, 0, GRID_ROWS - 1);
        spatialGrid[col][row].push_back(i);
    }
}


void SimularColisoes(
    float deltaTime,
    std::vector<GameBall>& balls,
    std::vector<BoundingSegment>& tableSegments,
    std::vector<BoundingSegment>& pocketSegments,
    std::vector<Pocket>& pockets,
    std::vector<std::vector<std::vector<size_t>>>& spatialGrid,
    bool& cueBallPositioningMode
) {
    static float physics_accumulator = 0.0f;
    physics_accumulator += deltaTime;

    while (physics_accumulator >= FIXED_PHYSICS_DELTA_TIME)
    {
        updateSpatialGrid(balls, spatialGrid); // Atualiza o grid antes de usar
        for (size_t i = 0; i < balls.size(); ++i)
        {
            GameBall& ball_A = balls[i];
            if (!ball_A.active) continue;

            ball_A.velocity.y -= GRAVITY * FIXED_PHYSICS_DELTA_TIME;
            ball_A.position += ball_A.velocity * FIXED_PHYSICS_DELTA_TIME;
            ball_A.velocity.x *= BALL_FRICTION_FACTOR;
            ball_A.velocity.z *= BALL_FRICTION_FACTOR;

            if (glm::length(glm::vec2(ball_A.velocity.x, ball_A.velocity.z)) < VELOCITY_STOP_THRESHOLD)
            {
                ball_A.velocity.x = 0.0f;
                ball_A.velocity.z = 0.0f;
            }

            glm::vec3 linear_velocity_xz = glm::vec3(ball_A.velocity.x, 0.0f, ball_A.velocity.z);
            float linear_speed_xz = glm::length(linear_velocity_xz);
            if (linear_speed_xz > VELOCITY_STOP_THRESHOLD)
            {
                glm::vec3 surface_normal = glm::vec3(0.0f, 1.0f, 0.0f);
                ball_A.angular_velocity = glm::cross(surface_normal, linear_velocity_xz) / ball_A.radius;
                glm::quat frame_rotation = glm::angleAxis(glm::length(ball_A.angular_velocity) * FIXED_PHYSICS_DELTA_TIME,
                                                          glm::normalize(ball_A.angular_velocity));
                ball_A.orientation = glm::normalize(frame_rotation * ball_A.orientation);
            }
            else
            {
                ball_A.angular_velocity = glm::vec3(0.0f);
            }

            if (ball_A.position.y - ball_A.radius < FELT_SURFACE_Y_ACTUAL)
            {
                ball_A.position.y = FELT_SURFACE_Y_ACTUAL + ball_A.radius;
                ball_A.velocity.y *= -RESTITUTION_COEFF;
                if (glm::abs(ball_A.velocity.y) < VELOCITY_STOP_THRESHOLD)
                {
                    ball_A.velocity.y = 0.0f;
                }
            }

            int col_A = static_cast<int>((ball_A.position.x + TABLE_HALF_WIDTH) / GRID_CELL_SIZE);
            int row_A = static_cast<int>((ball_A.position.z + TABLE_HALF_DEPTH) / GRID_CELL_SIZE);
            col_A = glm::clamp(col_A, 0, GRID_COLS - 1);
            row_A = glm::clamp(row_A, 0, GRID_ROWS - 1);

            for (int dc = -1; dc <= 1; ++dc)
            for (int dr = -1; dr <= 1; ++dr)
            {
                int c = col_A + dc, r = row_A + dr;
                if (c >= 0 && c < GRID_COLS && r >= 0 && r < GRID_ROWS)
                for (size_t j_idx : spatialGrid[c][r])
                {
                    if (j_idx <= i) continue;
                    GameBall& ball_B = balls[j_idx];
                    if (!ball_B.active) continue;

                    glm::vec3 d = ball_A.position - ball_B.position;
                    float dist = glm::length(d);
                    float sum_r = ball_A.radius + ball_B.radius;
                    if (dist < sum_r)
                    {
                        glm::vec3 n = glm::normalize(d);
                        float penetration = sum_r - dist;
                        ball_A.position += n * (penetration / 2.0f);
                        ball_B.position -= n * (penetration / 2.0f);

                        glm::vec3 rel_vel = ball_A.velocity - ball_B.velocity;
                        float proj = glm::dot(rel_vel, n);
                        if (proj > 0) continue;
                        glm::vec3 impulse = (-(1.0f + RESTITUTION_COEFF) * proj / 2.0f) * n;
                        ball_A.velocity += impulse;
                        ball_B.velocity -= impulse;
                    }
                }
            }
            for (const auto& seg : pocketSegments)
            {
                glm::vec2 s = glm::vec2(seg.p2.x - seg.p1.x, seg.p2.z - seg.p1.z);
                glm::vec2 b = glm::vec2(ball_A.position.x - seg.p1.x, ball_A.position.z - seg.p1.z);
                float t = glm::clamp(glm::dot(b, s) / glm::dot(s, s), 0.0f, 1.0f);
                glm::vec2 cp = glm::vec2(seg.p1.x, seg.p1.z) + t * s;
                glm::vec2 n = glm::vec2(ball_A.position.x, ball_A.position.z) - cp;
                float d = glm::length(n);
                if (d < ball_A.radius)
                {
                    glm::vec2 dir = glm::normalize(n);
                    float pen = ball_A.radius - d;
                    ball_A.position.x += dir.x * pen;
                    ball_A.position.z += dir.y * pen;
                    glm::vec2 v(ball_A.velocity.x, ball_A.velocity.z);
                    float dot = glm::dot(v, dir);
                    if (dot < 0)
                    {
                        glm::vec2 rv = v - 2.0f * dot * dir;
                        rv *= RESTITUTION_COEFF;
                        ball_A.velocity.x = rv.x;
                        ball_A.velocity.z = rv.y;
                    }
                }
            }

            for (const auto& seg : tableSegments)
            {
                glm::vec2 s = glm::vec2(seg.p2.x - seg.p1.x, seg.p2.z - seg.p1.z);
                glm::vec2 b = glm::vec2(ball_A.position.x - seg.p1.x, ball_A.position.z - seg.p1.z);
                float t = glm::clamp(glm::dot(b, s) / glm::dot(s, s), 0.0f, 1.0f);
                glm::vec2 cp = glm::vec2(seg.p1.x, seg.p1.z) + t * s;
                glm::vec2 n = glm::vec2(ball_A.position.x, ball_A.position.z) - cp;
                float d = glm::length(n);
                if (d < ball_A.radius)
                {
                    glm::vec2 dir = glm::normalize(n);
                    float pen = ball_A.radius - d;
                    ball_A.position.x += dir.x * pen;
                    ball_A.position.z += dir.y * pen;
                    glm::vec2 v(ball_A.velocity.x, ball_A.velocity.z);
                    float dot = glm::dot(v, dir);
                    if (dot < 0)
                    {
                        glm::vec2 rv = v - 2.0f * dot * dir;
                        rv *= RESTITUTION_COEFF;
                        ball_A.velocity.x = rv.x;
                        ball_A.velocity.z = rv.y;
                    }
                }
            }

            for (const auto& pocket : pockets)
            {
                float dist = glm::length(ball_A.position - pocket.position);
                if (dist <= (ball_A.radius + pocket.radius))
                {
                    if (ball_A.texture_unit_index == 0)
                    {
                        cueBallPositioningMode = true;
                        ball_A.position = glm::vec3(-0.0020f, BALL_Y_AXIS, 0.5680f);
                        ball_A.velocity = glm::vec3(0.0f);
                        std::cout << "DEBUG: Bola branca encacapada!\n";
                    }
                    else
                    {
                        ball_A.active = false;
                        ball_A.position = glm::vec3(1000.0f);
                        ball_A.velocity = glm::vec3(0.0f);
                        std::cout << "DEBUG: Bola encacapada! Pos: ("
                                  << ball_A.position.x << ", "
                                  << ball_A.position.y << ", "
                                  << ball_A.position.z << ")\n";
                    }
                    break;
                }
            }
        }

        physics_accumulator -= FIXED_PHYSICS_DELTA_TIME;
    }
}
