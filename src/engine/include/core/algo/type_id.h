#pragma once

// From https://gpfault.net/posts/mapping-types-to-values.txt.html

#include <atomic>

extern std::atomic_int TypeIdCounter;

template <typename T>
int getTypeId() {
    static int id = ++TypeIdCounter;
    return id;
}
