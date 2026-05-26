#pragma once

#include "Entity.h"

#include <random>

class Animal : public Entity {
public:
    Animal(std::uint64_t id, Vec3 position, const SpeciesConfig* config);

    void moveToward(const Vec3& target);
    void randomMove(std::mt19937& rng);
};
