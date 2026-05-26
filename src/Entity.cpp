#include "Entity.h"

#include <algorithm>

Entity::Entity(std::uint64_t id_, Vec3 position_, const SpeciesConfig* config_)
    : id(id_), position(position_), config(config_), energy(config_->initialEnergy) {}

void Entity::step() {
    ++age;
    ++sinceLastMeal;
    ++sinceLastReproduction;
    ++lonelyTicks;
    energy -= 0.1f;
}

bool Entity::deadFromNaturalCauses() const {
    return age > config->maxAge || sinceLastMeal > config->maxWithoutFood || energy <= 0.0f;
}

bool Entity::deadFromLoneliness() const {
    return lonelyTicks > config->boredomLimit;
}

std::uint64_t Entity::getId() const {
    return id;
}

const Vec3& Entity::getPosition() const {
    return position;
}

Vec3& Entity::getPosition() {
    return position;
}

void Entity::setPosition(const Vec3& newPosition) {
    position = newPosition;
}

const SpeciesConfig& Entity::species() const {
    return *config;
}

int Entity::getAge() const {
    return age;
}

int Entity::getSinceLastReproduction() const {
    return sinceLastReproduction;
}

float Entity::getEnergy() const {
    return energy;
}

void Entity::addEnergy(float value) {
    energy += value;
}

void Entity::reduceEnergy(float value) {
    energy -= value;
}

void Entity::clampEnergyMax(float maxEnergy) {
    energy = std::min(energy, maxEnergy);
}

void Entity::resetSinceLastMeal() {
    sinceLastMeal = 0;
}

void Entity::resetSinceLastReproduction() {
    sinceLastReproduction = 0;
}

void Entity::resetLoneliness() {
    lonelyTicks = 0;
}

void Entity::incrementAge() {
    ++age;
}

void Entity::incrementSinceLastMeal() {
    ++sinceLastMeal;
}

void Entity::incrementSinceLastReproduction() {
    ++sinceLastReproduction;
}

void Entity::incrementLonelyTicks() {
    ++lonelyTicks;
}
