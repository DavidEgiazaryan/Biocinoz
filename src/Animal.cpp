#include "Animal.h"

Animal::Animal(std::uint64_t id, Vec3 position, const SpeciesConfig* config)
    : Entity(id, position, config) {}

void Animal::moveToward(const Vec3& target) {
    Vec3 dir = target - getPosition();
    const float len = dir.length();
    if (len > 0.0001f) {
        dir = dir * (1.0f / len);
        getPosition() += dir * species().speed;
    }
}

void Animal::randomMove(std::mt19937& rng) {
    std::uniform_real_distribution<float> d(-1.0f, 1.0f);
    Vec3 dir{d(rng), d(rng), d(rng)};
    const float len = dir.length();
    if (len > 0.0001f) {
        getPosition() += dir * (species().speed / len);
    }
}
