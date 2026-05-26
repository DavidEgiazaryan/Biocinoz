#include "Renderer.h"
#include "World.h"

#include <string>

namespace {
constexpr int kMaxIterations = 5000;

bool hasArgument(int argc, char** argv, const std::string& expected) {
    for (int i = 1; i < argc; ++i) {
        if (expected == argv[i]) {
            return true;
        }
    }
    return false;
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
