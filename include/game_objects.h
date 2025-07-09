#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <string>

struct GameBall {
        glm::vec3 position;
        glm::vec3 velocity;
        float radius;
        bool  active;
        std::string object_name;
        int   object_id;
        int texture_unit_index;
        int   shader_object_id;
        glm::quat orientation;
        glm::vec3 angular_velocity;
    };


// Estrutura para definir um segmento de reta da tabela
struct BoundingSegment {
    glm::vec3 p1; // Ponto inicial do segmento
    glm::vec3 p2; // Ponto final do segmento
};


// Estrutura para representar uma caçapa no jogo
struct Pocket {
    glm::vec3 position; // Posição do centro da caçapa no mundo virtual
    float radius;       // Raio da caçapa (para detecção de colisão)
    // Você pode adicionar um ID ou nome aqui se quiser renderizá-las depois.
};
