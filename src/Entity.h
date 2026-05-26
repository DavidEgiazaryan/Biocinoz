#pragma once

#include "SpeciesConfig.h"
#include "Vec3.h"

#include <cstdint>

class Entity {
public:
    Entity(std::uint64_t id, Vec3 position, const SpeciesConfig* config);
    virtual ~Entity() = default;

    virtual void step();

    [[nodiscard]] bool deadFromNaturalCauses() const;
    [[nodiscard]] bool deadFromLoneliness() const;

    [[nodiscard]] std::uint64_t getId() const;
    [[nodiscard]] const Vec3& getPosition() const;
    [[nodiscard]] Vec3& getPosition();
    void setPosition(const Vec3& newPosition);

    [[nodiscard]] const SpeciesConfig& species() const;
    [[nodiscard]] int getAge() const;
    [[nodiscard]] int getSinceLastReproduction() const;
    [[nodiscard]] float getEnergy() const;

    void addEnergy(float value);
    void reduceEnergy(float value);
    void clampEnergyMax(float maxEnergy);
    void resetSinceLastMeal();
    void resetSinceLastReproduction();
    void resetLoneliness();

protected:
    void incrementAge();
    void incrementSinceLastMeal();
    void incrementSinceLastReproduction();
    void incrementLonelyTicks();

private:
    std::uint64_t id;
    Vec3 position;
    const SpeciesConfig* config;
    int age{0};
    int sinceLastMeal{0};
    int sinceLastReproduction{0};
    int lonelyTicks{0};
    float energy{0.0f};
};
