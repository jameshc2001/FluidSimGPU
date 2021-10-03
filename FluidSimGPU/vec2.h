#pragma once

#include <cmath>

class Vec2 {
	public:
		float x;
		float y;

		Vec2() : x(0), y(0) {}
		Vec2(float _x, float _y) : x(_x), y(_y) {}

		/*float x() const { return x; }
		float y() const { return y; }*/

		float lengthSquared() const { return x * x + y * y; }
		float length() const { return sqrt(lengthSquared()); }

		Vec2 normalised() const {
			float l = length();
			if (l > 0) return Vec2(x / l, y / l);
			return Vec2(0, 0);
		}

		Vec2& operator+=(const Vec2& v) {
			x += v.x;
			y += v.y;
			return *this;
		}

		Vec2& operator-=(const Vec2& v) {
			x -= v.x;
			y -= v.y;
			return *this;
		}

		Vec2& operator*=(const float s) {
			x *= s;
			y *= s;
			return *this;
		}

		Vec2& operator/=(const float s) {
			x /= s;
			y /= s;
			return *this;
		}

		Vec2 operator-() const { return Vec2(-x, -y); }

		float dot(const Vec2& v) {
			return x * v.x + y * v.y;
		}
};

inline float cross(const Vec2& v1, const Vec2& v2) {
	return v1.x * v2.y - v1.y * v2.x;
}

inline Vec2 operator+(const Vec2& v1, const Vec2& v2) {
	return Vec2(v1.x + v2.x, v1.y + v2.y);
}

inline Vec2 operator-(const Vec2& v1, const Vec2& v2) {
	return Vec2(v1.x - v2.x, v1.y - v2.y);
}

inline Vec2 operator*(const Vec2& v, const float s) {
	return Vec2(v.x * s, v.y * s);
}

inline Vec2 operator*(const float s, const Vec2& v) {
	return v * s;
}

inline Vec2 operator/(const Vec2& v, const float s) {
	return v * (1.0f / s);
}