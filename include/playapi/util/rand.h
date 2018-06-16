#pragma once

#include <random>

namespace playapi {

class rand {

private:

    static bool initialized;

    static std::mt19937 rng;

public:

    static void initialize();

    template <typename T>
    static T next_int(T min, T max) {
        if (!initialized)
            initialize();
        std::uniform_int_distribution<T> dist(min, max);
        return dist(rng);
    }

};

}