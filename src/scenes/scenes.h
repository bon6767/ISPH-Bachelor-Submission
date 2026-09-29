#pragma once
#include <vector>
#include "../simulation/simulation_types.h"
#include "../scenes/camera.h"

namespace scenes {
struct Scene {
	std::string name;

	std::vector<sim::FluidVolume> fluidVolumes = {};
	
	std::string svg_path;

	Vector2d boundary = { 6.f, 4.f };
	Vector2d boundaryOffset = { 0.f, 0.f };
	
	double gridDimension = 7.f;		// or size basically domain size

	// Scene Elements
	std::vector<cam::Camera> cameras = { cam::Camera{} };
	std::vector<sim::Line> barriers = {};
	std::vector<sim::Outlet> outlets = {};
	std::vector<sim::Inlet> inlets = {};
	std::vector<sim::AccelerationArea> accelerationAreas = {};
};

Scene SVGTest();
Scene BreakingDam();
Scene WaterColumn();
Scene WaterColumnLowRes();
Scene WaterColumnOfHeight(double height);	// same width and resolution, height as parameter
Scene AccelerationBox();
Scene DoubleBreakingDam();
Scene Brunnen();
Scene WaterFall();
Scene Glass();
Scene ConstantStream();
Scene Ventil();
Scene WaterDrop();
Scene WaterInlet();
Scene FloatingSquare();
}
