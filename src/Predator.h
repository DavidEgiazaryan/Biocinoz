#pragma once

#include "Animal.h"

class Predator final : public Animal {
public:
    Predator(std::uint64_t id, Vec3 position, const SpeciesConfig* config);
};
