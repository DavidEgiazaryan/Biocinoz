#pragma once

#include "Vec3.h"

#include <SFML/Graphics.hpp>

struct RenderParticle {
    Vec3 position;
    sf::Color color;
    float radius;
};
