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

#include <SFML/Graphics.hpp>

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
        ++age;
        ++sinceLastReproduction;
        energy += 0.08f;
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
          foodCfg{"Food", 0.0f, 2.0f, 240, 85, 18, 4000, 400, 18, 20.0f, 10.0f, 15.0f, 4.0f, false},
          herbCfg{"Herbivore", 1.45f, 2.8f, 220, 55, 18, 2800, 800, 20, 60.0f, 30.0f, 27.0f, 8.0f, true},
          predCfg{"Predator", 1.18f, 2.1f, 340, 340, 60, 3000, 1500, 7, 48.0f, 30.0f, 72.0f, 24.0f, true} {
        seedPopulation();
    }

    void tick() {
        if (!isAlive()) {
            return;
        }

        ++iteration;
        spawnFoodTick();
        stepPlants();
        stepHerbivores();
        stepPredators();
        reproducePlants();
        reproduceAnimals(herbivores, herbCfg, 520);
        reproduceAnimals(predators, predCfg, 36);
        killOvercrowded(plants, foodCfg);
        killOvercrowded(herbivores, herbCfg);
        killOvercrowded(predators, predCfg);
        removeDead();
        stabilizeYoungBiocenosis();
    }

    void simulate(int maxIterations) {
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

    [[nodiscard]] bool alive() const { return isAlive(); }
    [[nodiscard]] int currentIteration() const { return iteration; }
    [[nodiscard]] float width() const { return worldX; }
    [[nodiscard]] float height() const { return worldY; }
    [[nodiscard]] float depth() const { return worldZ; }
    [[nodiscard]] const std::vector<Plant>& food() const { return plants; }
    [[nodiscard]] const std::vector<Herbivore>& herd() const { return herbivores; }
    [[nodiscard]] const std::vector<Predator>& hunters() const { return predators; }
    [[nodiscard]] std::string status() const { return statusLine(); }

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

    void seedPopulation() {
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
        if (plants.size() >= 1200) {
            return;
        }

        const int baseSpawn = plants.size() < 850 ? 4 : 1;
        const int underTargetBonus = plants.size() < 360 ? 8 : 0;
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
        if (plants.size() >= 1200) {
            return;
        }

        std::vector<Plant> newborns;
        for (auto& p : plants) {
            if (p.age > p.config->matureAge && p.sinceLastReproduction > p.config->reproductionCooldown && p.energy > p.config->reproductionEnergy) {
                p.sinceLastReproduction = 0;
                p.energy -= p.config->childEnergy;
                newborns.emplace_back(nextId++, mutateAround(p.position), &foodCfg);
                if (plants.size() + newborns.size() >= 1200) {
                    break;
                }
            }
        }
        appendAll(plants, newborns);
    }

    template <typename T>
    void reproduceAnimals(std::vector<T>& population, const SpeciesConfig& cfg, std::size_t populationCap) {
        if (population.size() >= populationCap) {
            return;
        }

        std::vector<T> newborns;

        for (auto& e : population) {
            if (e.age <= cfg.matureAge || e.sinceLastReproduction <= cfg.reproductionCooldown || e.energy <= cfg.reproductionEnergy) {
                continue;
            }

            bool canReproduce = true;
            if (cfg.sexualReproduction) {
                canReproduce = hasSameSpeciesNearby(e, population, cfg.perceptionRadius * 0.4f);
            }

            if (canReproduce) {
                e.sinceLastReproduction = 0;
                e.energy -= cfg.childEnergy;
                newborns.emplace_back(nextId++, mutateAround(e.position), &cfg);
                if (population.size() + newborns.size() >= populationCap) {
                    break;
                }
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
        if (hasSameSpeciesNearby(who, population, who.config->perceptionRadius * 0.45f)) {
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

    void stabilizeYoungBiocenosis() {
        if (iteration > minStableIterations) {
            return;
        }

        replenishPopulation(plants, foodCfg, 260, 380);
        replenishPopulation(herbivores, herbCfg, 36, 56);
        replenishPopulation(predators, predCfg, 6, 10);
    }

    template <typename T>
    void replenishPopulation(std::vector<T>& population, const SpeciesConfig& cfg, std::size_t minimum, std::size_t target) {
        if (population.size() >= minimum) {
            return;
        }

        while (population.size() < target) {
            population.emplace_back(nextId++, randomPosition(), &cfg);
        }
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

namespace {
constexpr int kMaxIterations = 5000;
constexpr unsigned int kWindowWidth = 1120;
constexpr unsigned int kWindowHeight = 760;
constexpr float kDefaultSecondsPerTick = 0.10f;
constexpr float kMinSecondsPerTick = 0.025f;
constexpr float kMaxSecondsPerTick = 0.45f;

struct RenderParticle {
    Vec3 position;
    sf::Color color;
    float radius;
};

bool hasArgument(int argc, char** argv, const std::string& expected) {
    for (int i = 1; i < argc; ++i) {
        if (expected == argv[i]) {
            return true;
        }
    }
    return false;
}

sf::FloatRect simulationRect(const sf::RenderWindow& window) {
    const sf::Vector2u size = window.getSize();
    return {
        48.0f,
        66.0f,
        static_cast<float>(size.x) - 96.0f,
        static_cast<float>(size.y) - 118.0f
    };
}

sf::Vector2f projectPosition(const Vec3& position, const World& world, const sf::FloatRect& rect) {
    const float x = rect.left + (position.x / world.width()) * rect.width;
    const float y = rect.top + (position.y / world.height()) * rect.height;
    return {x, y};
}

sf::Color colorWithDepth(sf::Color color, float depthRatio) {
    const auto channel = [depthRatio](sf::Uint8 value) {
        const float mixed = static_cast<float>(value) * (0.58f + depthRatio * 0.42f) + depthRatio * 32.0f;
        return static_cast<sf::Uint8>(std::clamp(mixed, 0.0f, 255.0f));
    };

    const float alpha = 120.0f + depthRatio * 120.0f;
    return {
        channel(color.r),
        channel(color.g),
        channel(color.b),
        static_cast<sf::Uint8>(std::clamp(alpha, 0.0f, 255.0f))
    };
}

void drawGrid(sf::RenderWindow& window, const sf::FloatRect& rect) {
    sf::RectangleShape area({rect.width, rect.height});
    area.setPosition(rect.left, rect.top);
    area.setFillColor(sf::Color(17, 24, 28));
    area.setOutlineColor(sf::Color(82, 105, 111));
    area.setOutlineThickness(1.0f);
    window.draw(area);

    const sf::Color gridColor(54, 70, 72, 110);
    constexpr int gridLines = 10;
    for (int i = 1; i < gridLines; ++i) {
        const float t = static_cast<float>(i) / static_cast<float>(gridLines);
        const float x = rect.left + rect.width * t;
        const float y = rect.top + rect.height * t;

        sf::Vertex vertical[] = {
            sf::Vertex({x, rect.top}, gridColor),
            sf::Vertex({x, rect.top + rect.height}, gridColor)
        };
        sf::Vertex horizontal[] = {
            sf::Vertex({rect.left, y}, gridColor),
            sf::Vertex({rect.left + rect.width, y}, gridColor)
        };

        window.draw(vertical, 2, sf::Lines);
        window.draw(horizontal, 2, sf::Lines);
    }
}

template <typename T>
void appendParticles(std::vector<RenderParticle>& particles, const std::vector<T>& population, sf::Color color, float radius) {
    for (const auto& entity : population) {
        particles.push_back({entity.position, color, radius});
    }
}

void drawParticles(sf::RenderWindow& window, const World& world, const sf::FloatRect& rect) {
    std::vector<RenderParticle> particles;
    particles.reserve(world.food().size() + world.herd().size() + world.hunters().size());

    appendParticles(particles, world.food(), sf::Color(90, 210, 110), 2.4f);
    appendParticles(particles, world.herd(), sf::Color(236, 194, 82), 4.1f);
    appendParticles(particles, world.hunters(), sf::Color(234, 82, 87), 5.3f);

    std::sort(particles.begin(), particles.end(), [](const RenderParticle& lhs, const RenderParticle& rhs) {
        return lhs.position.z < rhs.position.z;
    });

    for (const auto& particle : particles) {
        const float depthRatio = std::clamp(particle.position.z / world.depth(), 0.0f, 1.0f);
        const float radius = particle.radius * (0.68f + depthRatio * 0.82f);
        sf::CircleShape shape(radius, 18);
        shape.setOrigin(radius, radius);
        shape.setPosition(projectPosition(particle.position, world, rect));
        shape.setFillColor(colorWithDepth(particle.color, depthRatio));
        shape.setOutlineColor(sf::Color(8, 12, 14, 120));
        shape.setOutlineThickness(1.0f);
        window.draw(shape);
    }
}

void drawPopulationBars(sf::RenderWindow& window, const World& world) {
    const sf::Vector2u size = window.getSize();
    const float width = static_cast<float>(size.x) - 96.0f;
    const float left = 48.0f;
    const float top = 26.0f;
    const float gap = 8.0f;
    const float barHeight = 8.0f;

    const auto drawBar = [&](float y, std::size_t count, float expectedMax, sf::Color color) {
        sf::RectangleShape track({width, barHeight});
        track.setPosition(left, y);
        track.setFillColor(sf::Color(35, 43, 45));
        window.draw(track);

        const float fillWidth = width * std::clamp(static_cast<float>(count) / expectedMax, 0.0f, 1.0f);
        sf::RectangleShape fill({fillWidth, barHeight});
        fill.setPosition(left, y);
        fill.setFillColor(color);
        window.draw(fill);
    };

    drawBar(top, world.food().size(), 1200.0f, sf::Color(90, 210, 110));
    drawBar(top + barHeight + gap, world.herd().size(), 520.0f, sf::Color(236, 194, 82));
    drawBar(top + (barHeight + gap) * 2.0f, world.hunters().size(), 36.0f, sf::Color(234, 82, 87));
}

void updateWindowTitle(sf::RenderWindow& window, const World& world, float secondsPerTick, bool paused, bool finished) {
    std::ostringstream title;
    title << "Biocinoz | " << world.status() << " | tick "
          << static_cast<int>(secondsPerTick * 1000.0f) << " ms";
    if (paused) {
        title << " | paused";
    }
    if (finished) {
        title << " | finished";
    }
    window.setTitle(title.str());
}

void reportFinalStatus(const World& world) {
    if (!world.alive()) {
        std::cout << "System collapsed at iteration " << world.currentIteration() << "\n";
    }
    std::cout << "Final status: " << world.status() << "\n";
    if (world.currentIteration() >= 1000 && world.alive()) {
        std::cout << "Stable biocenosis reached 1000+ iterations.\n";
    }
}

void runGraphics(World& world, int maxIterations) {
    sf::RenderWindow window(sf::VideoMode(kWindowWidth, kWindowHeight), "Biocinoz");
    window.setFramerateLimit(60);

    float secondsPerTick = kDefaultSecondsPerTick;
    sf::Clock tickClock;
    bool paused = false;
    bool finalStatusPrinted = false;

    while (window.isOpen()) {
        sf::Event event{};
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                window.close();
            } else if (event.type == sf::Event::KeyPressed) {
                if (event.key.code == sf::Keyboard::Escape) {
                    window.close();
                } else if (event.key.code == sf::Keyboard::Space) {
                    paused = !paused;
                } else if (event.key.code == sf::Keyboard::Up) {
                    secondsPerTick = std::max(secondsPerTick * 0.75f, kMinSecondsPerTick);
                } else if (event.key.code == sf::Keyboard::Down) {
                    secondsPerTick = std::min(secondsPerTick * 1.25f, kMaxSecondsPerTick);
                }
            }
        }

        const bool finished = world.currentIteration() >= maxIterations || !world.alive();
        if (!paused && !finished && tickClock.getElapsedTime().asSeconds() >= secondsPerTick) {
            world.tick();
            tickClock.restart();
        }

        const bool nowFinished = world.currentIteration() >= maxIterations || !world.alive();
        if (nowFinished && !finalStatusPrinted) {
            reportFinalStatus(world);
            finalStatusPrinted = true;
        }

        updateWindowTitle(window, world, secondsPerTick, paused, nowFinished);

        window.clear(sf::Color(11, 13, 15));
        const sf::FloatRect rect = simulationRect(window);
        drawGrid(window, rect);
        drawParticles(window, world, rect);
        drawPopulationBars(window, world);
        window.display();
    }

    if (!finalStatusPrinted) {
        reportFinalStatus(world);
    }
}
}

int main(int argc, char** argv) {
    World world;
    if (hasArgument(argc, argv, "--headless")) {
        world.simulate(kMaxIterations);
        return 0;
    }

    runGraphics(world, kMaxIterations);
    return 0;
}
