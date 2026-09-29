#pragma once
#include "../utils/vector2.h"

namespace cam
{
	struct Camera {
		Vector2d startPosition = { 0., 0. };
		double viewSize = 2.;
		bool saveFrames = false;
		std::string name = "camera";	// used for out directory

		Vector2d position = { 0., 0. };
		float zoom = 1;
	};
}