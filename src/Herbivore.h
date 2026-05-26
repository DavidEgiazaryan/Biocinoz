#pragma once

#include "Animal.h"

class Herbivore final : public Animal {
public:
    Herbivore(std::uint64_t id, Vec3 position, const SpeciesConfig* config);
};
