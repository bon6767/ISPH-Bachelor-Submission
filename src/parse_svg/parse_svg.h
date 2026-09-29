#pragma once
#include "../simulation/simulation_types.h"
#include "../scenes/simConfig.h"
#include <vector>

#include "../../include/nanosvg/nanosvg.h"


namespace svg
{
	sim::SceneParticles parseSVG(std::string path, simcfg::SimulationConfig cfg);
	std::vector<Vector2d> flattenPath(NSVGpath* path, double threshold, Vector2d offset);
	void flattenSegment(std::vector<Vector2d>& out, Vector2d A, Vector2d B, Vector2d C, Vector2d D, double threshold, int level = 0);
	void sampleStroke(NSVGshape* shape, simcfg::SimulationConfig cfg, std::vector<Vector2d>& particles, Vector2d offset);
	void sampleArea(NSVGshape* shape, simcfg::SimulationConfig cfg, std::vector<Vector2d>& particles, std::vector<Vector2d> obstacles);
	Vector2d svgToWorld(Vector2d pos, Vector2d offset);
}