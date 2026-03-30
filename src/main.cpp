#include <SFML/Graphics.hpp>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <limits>
#include <optional>
#include <random>
#include <sstream>
#include <string>
#include <vector>

struct Vec3 {
    float x{};
    float y{};
    float z{};

    Vec3() = default;
    Vec3(float x_, float y_, float z_) : x(x_), y(y_), z(z_) {}

    Vec3 operator+(const Vec3& other) const { return {x + other.x, y + other.y, z + other.z}; }
    Vec3 operator-(const Vec3& other) const { return {x - other.x, y - other.y, z - other.z}; }
    Vec3 operator*(float scalar) const { return {x * scalar, y * scalar, z * scalar}; }

    Vec3& operator+=(const Vec3& other) {
        x += other.x;
        y += other.y;
        z += other.z;
        return *this;
    }

    float length() const { return std::sqrt(x * x + y * y + z * z); }

    static float distance(const Vec3& a, const Vec3& b) {
        return (a - b).length();
    }
};

std::ostream& operator<<(std::ostream& os, const Vec3& v) {
    os << "(" << v.x << ", " << v.y << ", " << v.z << ")";
    return os;
}

struct SpeciesConfig {
    std::string name;
    float speed;
    float eatRadius;
    int maxWithoutFood;
    int reproductionCooldown;
    int matureAge;
    int maxAge;
    int boredomLimit;
    int overcrowdThreshold;
    float perceptionRadius;
    float initialEnergy;
    float reproductionEnergy;
    float childEnergy;
    bool sexualReproduction;
};

class Entity {
public:
    Entity(std::uint64_t id_, Vec3 position_, const SpeciesConfig* config_)
        : id(id_), position(position_), config(config_), energy(config_->initialEnergy) {}

    virtual ~Entity() = default;

    virtual void step() {
        ++age;
        ++sinceLastMeal;
        ++sinceLastReproduction;
        ++lonelyTicks;
        energy -= 0.1f;
    }

    [[nodiscard]] bool deadFromNaturalCauses() const {
        return age > config->maxAge || sinceLastMeal > config->maxWithoutFood || energy <= 0.0f;
    }

    [[nodiscard]] bool deadFromLoneliness() const { return lonelyTicks > config->boredomLimit; }

    std::uint64_t id;
    Vec3 position;
    const SpeciesConfig* config;
    int age{0};
    int sinceLastMeal{0};
    int sinceLastReproduction{0};
    int lonelyTicks{0};
    float energy{0.0f};
};

class Plant final : public Entity {
public:
    Plant(std::uint64_t id_, Vec3 position_, const SpeciesConfig* config_)
        : Entity(id_, position_, config_) {}

    void step() override {
        Entity::step();
        energy += 0.05f;
    }
};

class Animal : public Entity {
public:
    Animal(std::uint64_t id_, Vec3 position_, const SpeciesConfig* config_)
        : Entity(id_, position_, config_) {}

    void moveToward(const Vec3& target) {
        Vec3 dir = target - position;
        const float len = dir.length();
        if (len > 0.0001f) {
            dir = dir * (1.0f / len);
            position += dir * config->speed;
        }
    }

    void randomMove(std::mt19937& rng) {
        std::uniform_real_distribution<float> d(-1.0f, 1.0f);
        Vec3 dir{d(rng), d(rng), d(rng)};
        const float len = dir.length();
        if (len > 0.0001f) {
            position += dir * (config->speed / len);
        }
    }
};

class Herbivore final : public Animal {
public:
    Herbivore(std::uint64_t id_, Vec3 position_, const SpeciesConfig* config_)
        : Animal(id_, position_, config_) {}
};

class Predator final : public Animal {
public:
    Predator(std::uint64_t id_, Vec3 position_, const SpeciesConfig* config_)
        : Animal(id_, position_, config_) {}
};

class World {
public:
    World()
        : rng(std::random_device{}()),
          foodCfg{"Food", 0.0f, 2.0f, 240, 120, 30, 4000, 400, 10, 20.0f, 10.0f, 18.0f, 6.0f, false},
          herbCfg{"Herbivore", 1.2f, 2.4f, 120, 110, 30, 2800, 240, 12, 42.0f, 22.0f, 35.0f, 13.0f, true},
          predCfg{"Predator", 1.45f, 2.5f, 180, 180, 45, 3000, 300, 8, 48.0f, 30.0f, 52.0f, 20.0f, true} {
        seedPopulation();
    }

    void simulate(int maxIterations, bool render) {
        sf::RenderWindow window;
        if (render) {
            window.create(sf::VideoMode(1280, 800), "3D Biocenosis (projection)");
            window.setFramerateLimit(60);
        }

        for (iteration = 1; iteration <= maxIterations; ++iteration) {
            if (render) {
                sf::Event event{};
                while (window.pollEvent(event)) {
                    if (event.type == sf::Event::Closed) {
                        window.close();
                        render = false;
                    }
                }
            }

            spawnFoodTick();
            stepPlants();
            stepHerbivores();
            stepPredators();
            reproducePlants();
            reproduceAnimals(herbivores, herbCfg);
            reproduceAnimals(predators, predCfg);
            killOvercrowded(plants, foodCfg);
            killOvercrowded(herbivores, herbCfg);
            killOvercrowded(predators, predCfg);
            removeDead();

            if (!isAlive()) {
                std::cout << "System collapsed at iteration " << iteration << "\n";
                break;
            }

            if (render && window.isOpen()) {
                draw(window);
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

    void seedPopulation() {
        for (int i = 0; i < 260; ++i) {
            plants.emplace_back(nextId++, randomPosition(), &foodCfg);
        }
        for (int i = 0; i < 42; ++i) {
            herbivores.emplace_back(nextId++, randomPosition(), &herbCfg);
        }
        for (int i = 0; i < 14; ++i) {
            predators.emplace_back(nextId++, randomPosition(), &predCfg);
        }
    }

    Vec3 randomPosition() {
        std::uniform_real_distribution<float> dx(0.0f, worldX);
        std::uniform_real_distribution<float> dy(0.0f, worldY);
        std::uniform_real_distribution<float> dz(0.0f, worldZ);
        return {dx(rng), dy(rng), dz(rng)};
    }

    void clampPosition(Vec3& p) const {
        p.x = std::clamp(p.x, 0.0f, worldX);
        p.y = std::clamp(p.y, 0.0f, worldY);
        p.z = std::clamp(p.z, 0.0f, worldZ);
    }

    template <typename T>
    std::optional<std::size_t> nearestWithinRadius(const Vec3& from, const std::vector<T>& candidates, float radius) {
        float best = std::numeric_limits<float>::max();
        std::optional<std::size_t> result;
        for (std::size_t i = 0; i < candidates.size(); ++i) {
            const float d = Vec3::distance(from, candidates[i].position);
            if (d < radius && d < best) {
                best = d;
                result = i;
            }
        }
        return result;
    }

    void spawnFoodTick() {
        const int baseSpawn = 8;
        const int underTargetBonus = plants.size() < 320 ? 6 : 0;
        for (int i = 0; i < baseSpawn + underTargetBonus; ++i) {
            plants.emplace_back(nextId++, randomPosition(), &foodCfg);
        }
    }

    void stepPlants() {
        for (auto& p : plants) {
            p.step();
            if (p.energy > 24.0f) {
                p.energy = 24.0f;
            }
        }
    }

    void stepHerbivores() {
        std::vector<bool> eaten(plants.size(), false);

        for (auto& h : herbivores) {
            h.step();
            if (const auto target = nearestWithinRadius(h.position, plants, h.config->perceptionRadius); target) {
                h.moveToward(plants[*target].position);
            } else {
                h.randomMove(rng);
            }
            clampPosition(h.position);

            if (const auto prey = nearestWithinRadius(h.position, plants, h.config->eatRadius); prey) {
                eaten[*prey] = true;
                h.sinceLastMeal = 0;
                h.energy += 9.0f;
            }

            updateLoneliness(h, herbivores);
        }

        removeMarked(plants, eaten);
    }

    void stepPredators() {
        std::vector<bool> hunted(herbivores.size(), false);

        for (auto& p : predators) {
            p.step();
            if (const auto target = nearestWithinRadius(p.position, herbivores, p.config->perceptionRadius); target) {
                p.moveToward(herbivores[*target].position);
            } else {
                p.randomMove(rng);
            }
            clampPosition(p.position);

            if (const auto prey = nearestWithinRadius(p.position, herbivores, p.config->eatRadius); prey) {
                hunted[*prey] = true;
                p.sinceLastMeal = 0;
                p.energy += 15.0f;
            }

            updateLoneliness(p, predators);
        }

        removeMarked(herbivores, hunted);
    }

    void reproducePlants() {
        std::vector<Plant> newborns;
        for (auto& p : plants) {
            if (p.age > p.config->matureAge && p.sinceLastReproduction > p.config->reproductionCooldown && p.energy > p.config->reproductionEnergy) {
                p.sinceLastReproduction = 0;
                p.energy -= p.config->childEnergy;
                newborns.emplace_back(nextId++, mutateAround(p.position), &foodCfg);
            }
        }
        appendAll(plants, newborns);
    }

    template <typename T>
    void reproduceAnimals(std::vector<T>& population, const SpeciesConfig& cfg) {
        std::vector<T> newborns;

        for (auto& e : population) {
            if (e.age <= cfg.matureAge || e.sinceLastReproduction <= cfg.reproductionCooldown || e.energy <= cfg.reproductionEnergy) {
                continue;
            }

            bool canReproduce = true;
            if (cfg.sexualReproduction) {
                canReproduce = hasSameSpeciesNearby(e, population, 12.0f);
            }

            if (canReproduce) {
                e.sinceLastReproduction = 0;
                e.energy -= cfg.childEnergy;
                newborns.emplace_back(nextId++, mutateAround(e.position), &cfg);
            }
        }

        appendAll(population, newborns);
    }

    template <typename T>
    bool hasSameSpeciesNearby(const T& who, const std::vector<T>& population, float radius) {
        for (const auto& another : population) {
            if (another.id == who.id) {
                continue;
            }
            if (Vec3::distance(who.position, another.position) <= radius) {
                return true;
            }
        }
        return false;
    }

    template <typename T>
    void updateLoneliness(T& who, const std::vector<T>& population) {
        if (hasSameSpeciesNearby(who, population, 14.0f)) {
            who.lonelyTicks = 0;
        }
    }

    Vec3 mutateAround(const Vec3& origin) {
        std::uniform_real_distribution<float> delta(-6.0f, 6.0f);
        Vec3 p = origin + Vec3{delta(rng), delta(rng), delta(rng)};
        clampPosition(p);
        return p;
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
                if (Vec3::distance(population[i].position, population[j].position) < 7.0f) {
                    ++around;
                }
            }
            if (around > cfg.overcrowdThreshold) {
                dead[i] = true;
            }
        }
        removeMarked(population, dead);
    }

    void removeDead() {
        eraseIf(plants, [](const Plant& p) { return p.deadFromNaturalCauses() || p.deadFromLoneliness(); });
        eraseIf(herbivores, [](const Herbivore& h) { return h.deadFromNaturalCauses() || h.deadFromLoneliness(); });
        eraseIf(predators, [](const Predator& p) { return p.deadFromNaturalCauses() || p.deadFromLoneliness(); });
    }

    bool isAlive() const {
        return !plants.empty() && !herbivores.empty() && !predators.empty();
    }

    std::string statusLine() const {
        std::ostringstream oss;
        oss << "it=" << iteration
            << " food=" << plants.size()
            << " herb=" << herbivores.size()
            << " pred=" << predators.size();
        return oss.str();
    }

    void draw(sf::RenderWindow& window) {
        window.clear(sf::Color(16, 21, 30));

        auto drawPoint = [&](const Vec3& p, sf::Color color, float baseRadius) {
            const float depthScale = 0.5f + 0.7f * (p.z / worldZ);
            sf::CircleShape c(baseRadius * depthScale);
            c.setFillColor(color);
            c.setPosition(p.x * 5.0f + 30.0f, p.y * 4.5f + 30.0f);
            window.draw(c);
        };

        for (const auto& p : plants) {
            drawPoint(p.position, sf::Color(40, 200, 90), 2.0f);
        }
        for (const auto& h : herbivores) {
            drawPoint(h.position, sf::Color(235, 205, 52), 3.0f);
        }
        for (const auto& p : predators) {
            drawPoint(p.position, sf::Color(220, 80, 80), 3.8f);
        }

        window.display();
    }

    template <typename T, typename Pred>
    static void eraseIf(std::vector<T>& v, Pred pred) {
        v.erase(std::remove_if(v.begin(), v.end(), pred), v.end());
    }

    template <typename T>
    static void appendAll(std::vector<T>& dst, const std::vector<T>& src) {
        dst.insert(dst.end(), src.begin(), src.end());
    }

    template <typename T>
    static void removeMarked(std::vector<T>& values, const std::vector<bool>& removeFlags) {
        std::vector<T> filtered;
        filtered.reserve(values.size());
        for (std::size_t i = 0; i < values.size(); ++i) {
            if (!removeFlags[i]) {
                filtered.push_back(values[i]);
            }
        }
        values.swap(filtered);
    }
};

int main(int argc, char** argv) {
    bool render = true;
    if (argc > 1 && std::string(argv[1]) == "--headless") {
        render = false;
    }

    World world;
    world.simulate(5000, render);
    return 0;
}
