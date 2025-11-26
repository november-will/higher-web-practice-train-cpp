#pragma once

#include <algorithm>
#include <array>
#include <cassert>
#include <random>

class RandomGen {
public:
    RandomGen() = delete;
    RandomGen(const RandomGen&) = delete;
    RandomGen& operator=(const RandomGen&) = delete;

    template <class T>
    static const T& GetRandomElem(const std::vector<T>& elements) {
        assert(elements.size() > 0);
        return elements[GetInRange(0, static_cast<int>(elements.size()) - 1)];
    }

    template <class T, std::size_t N>
    static const T& GetRandomElem(const std::array<T, N>& elements) {
        assert(elements.size() > 0);
        return elements[GetInRange(0, static_cast<int>(elements.size()) - 1)];
    }

    static int GetInRange(int from, int to) {
        std::uniform_int_distribution<int> dist(from, to);
        return dist(Generator());
    }

private:
    static std::mt19937& Generator() {
        static std::mt19937 random_gen(std::random_device{}());
        return random_gen;
    }
};
