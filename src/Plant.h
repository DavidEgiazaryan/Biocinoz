#pragma once

#include "Entity.h"

class Plant final : public Entity {
public:
    Plant(std::uint64_t id, Vec3 position, const SpeciesConfig* config);

    void step() override;
};
