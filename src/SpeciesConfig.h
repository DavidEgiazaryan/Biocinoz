#pragma once

#include <string>

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
