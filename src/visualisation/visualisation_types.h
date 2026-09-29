#pragma once
#include <SFML/Graphics.hpp>

namespace vis
{
struct ColorGradient {
	sf::Color minColor;
	sf::Color maxColor;
	double minVal;
	double maxVal;
};

enum VisualiseQuantity {
	Color,
	Density,
	Pressure,
	Velocity
};

enum RenderType {
	Particles,
	MarchingSquares,
	Both
};

}