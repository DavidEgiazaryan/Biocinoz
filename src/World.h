#pragma once

#include "Herbivore.h"
#include "Plant.h"
#include "Predator.h"
#include "SpeciesConfig.h"
#include "Vec3.h"

#include <cstdint>
#include <random>
#include <string>
#include <vector>

class World {
public:
    World();

    void tick();
    void simulate(int maxIterations);

    [[nodiscard]] bool alive() const;
    [[nodiscard]] int currentIteration() const;
    [[nodiscard]] float width() const;
    [[nodiscard]] float height() const;
    [[nodiscard]] float depth() const;
    [[nodiscard]] const std::vector<Plant>& food() const;
    [[nodiscard]] const std::vector<Herbivore>& herd() const;
    [[nodiscard]] const std::vector<Predator>& hunters() const;
    [[nodiscard]] std::string status() const;

private:
    std::mt19937 rng;
    std::uint64_t nextId{1};
    int iteration{0};

    const float worldX = 220.0f;
    const float worldY = 150.0f;
    const float worldZ = 100.0f;

    SpeciesConfig foodCfg;
    SpeciesConfig herbCfg;
    SpeciesConfig predCfg;

    std::vector<Plant> plants;
    std::vector<Herbivore> herbivores;
    std::vector<Predator> predators;

    static constexpr int minStableIterations = 1000;

    void seedPopulation();
    Vec3 randomPosition();
    void clampPosition(Vec3& p) const;
    void spawnFoodTick();
    void stepPlants();
    void stepHerbivores();
    void stepPredators();
    void reproducePlants();
    void removeDead();
    void stabilizeYoungBiocenosis();
    bool isAlive() const;
    std::string statusLine() const;

    template <typename T>
    void replenishPopulation(std::vector<T>& population, const SpeciesConfig& cfg, std::size_t minimum, std::size_t target);
};
