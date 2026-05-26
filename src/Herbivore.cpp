#include "Herbivore.h"

Herbivore::Herbivore(std::uint64_t id, Vec3 position, const SpeciesConfig* config)
    : Animal(id, position, config) {}
