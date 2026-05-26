#include "Renderer.h"

#include "RenderParticle.h"
#include "World.h"

#include <SFML/Graphics.hpp>

#include <algorithm>
#include <cstddef>
#include <iostream>
#include <sstream>
#include <vector>

namespace {
constexpr unsigned int kWindowWidth = 1120;
constexpr unsigned int kWindowHeight = 760;
constexpr float kDefaultSecondsPerTick = 0.10f;
constexpr float kMinSecondsPerTick = 0.025f;
constexpr float kMaxSecondsPerTick = 0.45f;

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
        particles.push_back({entity.getPosition(), color, radius});
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
