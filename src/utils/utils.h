#pragma once
#include <fstream>
#include <filesystem>
#include <mutex>
#include "../simulation/simulation_types.h"
#include "vector2.h"

using namespace sim;

namespace utils {
float magnitude(double x, double y);
bool isInBox(double x, double y, Box b);

std::vector<Vector2d> line(Vector2d start, Vector2d end, double h);
std::vector<Vector2d> line(Vector2d position, double width, double rotation, double);

std::string getTimeStamp();

// creates the directory a file is about to be written to, so the exe works
// in a fresh folder without the output structure being prepared beforehand
inline void ensureParentDir(const std::string& path) {
    std::filesystem::path p(path);
    if (p.has_parent_path())
        std::filesystem::create_directories(p.parent_path());
}
}