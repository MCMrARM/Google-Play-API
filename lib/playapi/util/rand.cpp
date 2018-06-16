#include <playapi/util/rand.h>

using namespace playapi;

bool rand::initialized = false;
std::mt19937 rand::rng;

void rand::initialize() {
    rng = std::mt19937(std::random_device{}());
    rng.discard(700000);
    initialized = true;
}