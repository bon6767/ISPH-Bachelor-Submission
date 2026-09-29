#pragma once

template <typename T>
struct Vector2 {
    T x{}, y{};   // default-initialised to 0

    Vector2& operator+=(const Vector2& r) { x += r.x; y += r.y; return *this; }
    Vector2& operator-=(const Vector2& r) { x -= r.x; y -= r.y; return *this; }
    Vector2& operator*=(T s) { x *= s;   y *= s;   return *this; }
    Vector2& operator/=(T s) { x /= s;   y /= s;   return *this; }
    bool operator==(const Vector2& r) { return x == r.x && y == r.y; }
    bool operator!=(const Vector2& r) { return x != r.x || y != r.y; }
};

template <typename T> Vector2<T> operator+(Vector2<T> a, const Vector2<T>& b) { return a += b; }
template <typename T> Vector2<T> operator-(Vector2<T> a, const Vector2<T>& b) { return a -= b; }
template <typename T> Vector2<T> operator-(const Vector2<T>& a) { return { -a.x, -a.y }; }
template <typename T> Vector2<T> operator*(Vector2<T> v, T s) { return v *= s; }
template <typename T> Vector2<T> operator*(T s, Vector2<T> v) { return v *= s; }
template <typename T> Vector2<T> operator/(T s, Vector2<T> v) { return v /= s; }

template<typename T>
T dotProduct(const Vector2<T>& v, const Vector2<T>& w) {
    return v.x * w.x + v.y * w.y;
}

template<typename T>
T crossProduct(const Vector2<T>& a, const Vector2<T>& b) {
    return a.x * b.y - a.y * b.x;
}

template<typename T>
T magnitude(const Vector2<T>& a) {
    return  sqrt(a.x * a.x + a.y * a.y);
}

template<typename T>
Vector2<T> normalize(const Vector2<T>& a) {
    auto mag = magnitude(a);
    if (mag == 0) return Vector2<T>{0, 0};
    return  a * (1. / mag);
}



using Vector2d = Vector2<double>;
// using Vector2f = Vector2<float>;