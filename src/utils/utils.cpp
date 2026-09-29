#include "utils.h"
#include <sstream>

static inline double pi() { return 3.14159265358979323846; }

namespace utils {

float magnitude(double x, double y) {
    return sqrt(x * x + y * y);
}

bool isInBox(double x, double y, Box b) {
    return x > b.min_x && x < b.max_x && y > b.min_y && y < b.max_y;
}

std::vector<Vector2d> line(Vector2d start, Vector2d end, double h) {
    Vector2d d = end - start;
    double distance = std::sqrt(d.x * d.x + d.y * d.y);
    int n = distance / h;
    std::vector<Vector2d> line;
    for (int i = 0; i < n; i++) {
        line.emplace_back(Vector2d{
            start.x + (i / static_cast<double>(n)) * (end.x - start.x),
            start.y + (i / static_cast<double>(n)) * (end.y - start.y)
            });
    }
    return line;
}

std::vector<Vector2d> line(Vector2d position, double width, double rotation, double h) {
    double rad = rotation * pi() / 180.;

    double r = width / 2.;
    auto start = Vector2d{
        position.x + r * cos(rad),
        position.y + r * sin(rad)
    };
    auto end = Vector2d{
    position.x - r * cos(rad),
    position.y - r * sin(rad)
    };
    return line(start, end, h);
}

std::string getTimeStamp() {
    std::time_t t = std::time(nullptr);
    std::tm* tm = std::localtime(&t);
    std::ostringstream oss;
    oss << std::put_time(tm, "%Y%m%d%H%M");
    std::string dateTimeString = oss.str();
    return dateTimeString;
}
}
