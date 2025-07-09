#pragma once
#include <vector>
#include "game_objects.h"
// Função que simula colisões e atualiza a física das bolas
void SimularColisoes(
    float deltaTime,
    std::vector<GameBall>& balls,
    std::vector<BoundingSegment>& tableSegments,
    std::vector<BoundingSegment>& pocketSegments,
    std::vector<Pocket>& pockets,
    std::vector<std::vector<std::vector<size_t>>>& spatialGrid,
    bool& cueBallPositioningMode
);