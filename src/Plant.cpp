#include "Plant.h"

Plant::Plant(std::uint64_t id, Vec3 position, const SpeciesConfig* config)
    : Entity(id, position, config) {}

void Plant::step() {
    incrementAge();
    incrementSinceLastReproduction();
    addEnergy(0.08f);
}
