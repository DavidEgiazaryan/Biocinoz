#include "World.h"

#include <algorithm>
#include <functional>
#include <iostream>
#include <limits>
#include <optional>
#include <sstream>

namespace {
constexpr std::size_t kPlantCap = 1200;
constexpr std::size_t kHerbivoreCap = 520;
constexpr std::size_t kPredatorCap = 36;

template <typename T>
std::optional<std::size_t> nearestWithinRadius(const Vec3& from, const std::vector<T>& candidates, float radius) {
    float best = std::numeric_limits<float>::max();
    std::optional<std::size_t> result;
    for (std::size_t i = 0; i < candidates.size(); ++i) {
        const float d = Vec3::distance(from, candidates[i].getPosition());
        if (d < radius && d < best) {
            best = d;
            result = i;
        }
    }
    return result;
}

template <typename T>
bool hasSameSpeciesNearby(const T& who, const std::vector<T>& population, float radius) {
    for (const auto& another : population) {
        if (another.getId() == who.getId()) {
            continue;
        }
        if (Vec3::distance(who.getPosition(), another.getPosition()) <= radius) {
            return true;
        }
    }
    return false;
}

template <typename T>
void updateLoneliness(T& who, const std::vector<T>& population) {
    if (hasSameSpeciesNearby(who, population, who.species().perceptionRadius * 0.45f)) {
        who.resetLoneliness();
    }
}

template <typename T>
void removeMarked(std::vector<T>& values, const std::vector<bool>& removeFlags) {
    std::vector<T> filtered;
    filtered.reserve(values.size());
    for (std::size_t i = 0; i < values.size(); ++i) {
        if (!removeFlags[i]) {
            filtered.push_back(values[i]);
        }
    }
    values.swap(filtered);
}

template <typename T>
void appendAll(std::vector<T>& dst, const std::vector<T>& src) {
    dst.insert(dst.end(), src.begin(), src.end());
}

template <typename T, typename Pred>
void eraseIf(std::vector<T>& values, Pred pred) {
    values.erase(std::remove_if(values.begin(), values.end(), pred), values.end());
}

template <typename T>
void killOvercrowded(std::vector<T>& population, const SpeciesConfig& cfg) {
    if (population.empty()) {
        return;
    }

    std::vector<bool> dead(population.size(), false);
    for (std::size_t i = 0; i < population.size(); ++i) {
        int around = 0;
        for (std::size_t j = 0; j < population.size(); ++j) {
            if (i == j) {
                continue;
            }
            if (Vec3::distance(population[i].getPosition(), population[j].getPosition()) < 7.0f) {
                ++around;
            }
        }
        if (around > cfg.overcrowdThreshold) {
            dead[i] = true;
        }
    }
    removeMarked(population, dead);
}

template <typename T>
void reproduceAnimals(
    std::vector<T>& population,
    const SpeciesConfig& cfg,
    std::size_t populationCap,
    std::uint64_t& nextId,
    const std::function<Vec3(const Vec3&)>& mutateAround
) {
    if (population.size() >= populationCap) {
        return;
    }

    std::vector<T> newborns;

    for (auto& e : population) {
        if (e.getAge() <= cfg.matureAge ||
            e.getSinceLastReproduction() <= cfg.reproductionCooldown ||
            e.getEnergy() <= cfg.reproductionEnergy) {
            continue;
        }

        bool canReproduce = true;
        if (cfg.sexualReproduction) {
            canReproduce = hasSameSpeciesNearby(e, population, cfg.perceptionRadius * 0.4f);
        }

        if (canReproduce) {
            e.resetSinceLastReproduction();
            e.reduceEnergy(cfg.childEnergy);
            newborns.emplace_back(nextId++, mutateAround(e.getPosition()), &cfg);
            if (population.size() + newborns.size() >= populationCap) {
                break;
            }
        }
    }

    appendAll(population, newborns);
}
}

World::World()
    : rng(std::random_device{}()),
      foodCfg{"Food", 0.0f, 2.0f, 240, 85, 18, 4000, 400, 18, 20.0f, 10.0f, 15.0f, 4.0f, false},
      herbCfg{"Herbivore", 1.45f, 2.8f, 220, 55, 18, 2800, 800, 20, 60.0f, 30.0f, 27.0f, 8.0f, true},
      predCfg{"Predator", 1.18f, 2.1f, 340, 340, 60, 3000, 1500, 7, 48.0f, 30.0f, 72.0f, 24.0f, true} {
    seedPopulation();
}

void World::tick() {
    if (!isAlive()) {
        return;
    }

    ++iteration;
    spawnFoodTick();
    stepPlants();
    stepHerbivores();
    stepPredators();
    reproducePlants();
    reproduceAnimals(herbivores, herbCfg, kHerbivoreCap, nextId, [this](const Vec3& origin) {
        std::uniform_real_distribution<float> delta(-6.0f, 6.0f);
        Vec3 p = origin + Vec3{delta(rng), delta(rng), delta(rng)};
        clampPosition(p);
        return p;
    });
    reproduceAnimals(predators, predCfg, kPredatorCap, nextId, [this](const Vec3& origin) {
        std::uniform_real_distribution<float> delta(-6.0f, 6.0f);
        Vec3 p = origin + Vec3{delta(rng), delta(rng), delta(rng)};
        clampPosition(p);
        return p;
    });
    killOvercrowded(plants, foodCfg);
    killOvercrowded(herbivores, herbCfg);
    killOvercrowded(predators, predCfg);
    removeDead();
    stabilizeYoungBiocenosis();
}

void World::simulate(int maxIterations) {
    while (iteration < maxIterations && isAlive()) {
        tick();

        if (!isAlive()) {
            std::cout << "System collapsed at iteration " << iteration << "\n";
            break;
        }
        if (iteration % 100 == 0) {
            std::cout << statusLine() << "\n";
        }
    }

    std::cout << "Final status: " << statusLine() << "\n";
    if (iteration >= 1000 && isAlive()) {
        std::cout << "Stable biocenosis reached 1000+ iterations.\n";
    }
}

bool World::alive() const {
    return isAlive();
}

int World::currentIteration() const {
    return iteration;
}

float World::width() const {
    return worldX;
}

float World::height() const {
    return worldY;
}

float World::depth() const {
    return worldZ;
}

const std::vector<Plant>& World::food() const {
    return plants;
}

const std::vector<Herbivore>& World::herd() const {
    return herbivores;
}

const std::vector<Predator>& World::hunters() const {
    return predators;
}

std::string World::status() const {
    return statusLine();
}

void World::seedPopulation() {
    for (int i = 0; i < 320; ++i) {
        plants.emplace_back(nextId++, randomPosition(), &foodCfg);
    }
    for (int i = 0; i < 72; ++i) {
        herbivores.emplace_back(nextId++, randomPosition(), &herbCfg);
    }
    for (int i = 0; i < 10; ++i) {
        predators.emplace_back(nextId++, randomPosition(), &predCfg);
    }
}

Vec3 World::randomPosition() {
    std::uniform_real_distribution<float> dx(0.0f, worldX);
    std::uniform_real_distribution<float> dy(0.0f, worldY);
    std::uniform_real_distribution<float> dz(0.0f, worldZ);
    return {dx(rng), dy(rng), dz(rng)};
}

void World::clampPosition(Vec3& p) const {
    p.x = std::clamp(p.x, 0.0f, worldX);
    p.y = std::clamp(p.y, 0.0f, worldY);
    p.z = std::clamp(p.z, 0.0f, worldZ);
}

void World::spawnFoodTick() {
    if (plants.size() >= kPlantCap) {
        return;
    }

    const int baseSpawn = plants.size() < 850 ? 4 : 1;
    const int underTargetBonus = plants.size() < 360 ? 8 : 0;
    for (int i = 0; i < baseSpawn + underTargetBonus; ++i) {
        plants.emplace_back(nextId++, randomPosition(), &foodCfg);
    }
}

void World::stepPlants() {
    for (auto& p : plants) {
        p.step();
        p.clampEnergyMax(24.0f);
    }
}

void World::stepHerbivores() {
    std::vector<bool> eaten(plants.size(), false);

    for (auto& h : herbivores) {
        h.step();
        if (const auto target = nearestWithinRadius(h.getPosition(), plants, h.species().perceptionRadius); target) {
            h.moveToward(plants[*target].getPosition());
        } else {
            h.randomMove(rng);
        }
        clampPosition(h.getPosition());

        if (const auto prey = nearestWithinRadius(h.getPosition(), plants, h.species().eatRadius); prey) {
            eaten[*prey] = true;
            h.resetSinceLastMeal();
            h.addEnergy(9.0f);
        }

        updateLoneliness(h, herbivores);
    }

    removeMarked(plants, eaten);
}

void World::stepPredators() {
    std::vector<bool> hunted(herbivores.size(), false);

    for (auto& p : predators) {
        p.step();
        if (const auto target = nearestWithinRadius(p.getPosition(), herbivores, p.species().perceptionRadius); target) {
            p.moveToward(herbivores[*target].getPosition());
        } else {
            p.randomMove(rng);
        }
        clampPosition(p.getPosition());

        if (const auto prey = nearestWithinRadius(p.getPosition(), herbivores, p.species().eatRadius); prey) {
            hunted[*prey] = true;
            p.resetSinceLastMeal();
            p.addEnergy(15.0f);
        }

        updateLoneliness(p, predators);
    }

    removeMarked(herbivores, hunted);
}

void World::reproducePlants() {
    if (plants.size() >= kPlantCap) {
        return;
    }

    std::vector<Plant> newborns;
    for (auto& p : plants) {
        if (p.getAge() > p.species().matureAge &&
            p.getSinceLastReproduction() > p.species().reproductionCooldown &&
            p.getEnergy() > p.species().reproductionEnergy) {
            p.resetSinceLastReproduction();
            p.reduceEnergy(p.species().childEnergy);

            std::uniform_real_distribution<float> delta(-6.0f, 6.0f);
            Vec3 position = p.getPosition() + Vec3{delta(rng), delta(rng), delta(rng)};
            clampPosition(position);
            newborns.emplace_back(nextId++, position, &foodCfg);

            if (plants.size() + newborns.size() >= kPlantCap) {
                break;
            }
        }
    }
    appendAll(plants, newborns);
}

void World::removeDead() {
    eraseIf(plants, [](const Plant& p) { return p.deadFromNaturalCauses() || p.deadFromLoneliness(); });
    eraseIf(herbivores, [](const Herbivore& h) { return h.deadFromNaturalCauses() || h.deadFromLoneliness(); });
    eraseIf(predators, [](const Predator& p) { return p.deadFromNaturalCauses() || p.deadFromLoneliness(); });
}

void World::stabilizeYoungBiocenosis() {
    if (iteration > minStableIterations) {
        return;
    }

    replenishPopulation(plants, foodCfg, 260, 380);
    replenishPopulation(herbivores, herbCfg, 36, 56);
    replenishPopulation(predators, predCfg, 6, 10);
}

template <typename T>
void World::replenishPopulation(
    std::vector<T>& population,
    const SpeciesConfig& cfg,
    std::size_t minimum,
    std::size_t target
) {
    if (population.size() >= minimum) {
        return;
    }

    while (population.size() < target) {
        population.emplace_back(nextId++, randomPosition(), &cfg);
    }
}

bool World::isAlive() const {
    return !plants.empty() && !herbivores.empty() && !predators.empty();
}

std::string World::statusLine() const {
    std::ostringstream oss;
    oss << "it=" << iteration
        << " food=" << plants.size()
        << " herb=" << herbivores.size()
        << " pred=" << predators.size();
    return oss.str();
}
