#include "Predator.h"

Predator::Predator(std::uint64_t id, Vec3 position, const SpeciesConfig* config)
    : Animal(id, position, config) {}
