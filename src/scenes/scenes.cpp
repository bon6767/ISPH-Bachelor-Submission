#include <algorithm>
#include <vector>

#include "scenes.h"
#include "../simulation/simulation_types.h"

using namespace sim;

namespace scenes {
Scene SVGTest() {
	Scene s = Scene{};
	s.name = "svgBoundaryTest";

	s.svg_path = "assets/scenes/complex_boundary.svg";

	s.gridDimension = 6;
	s.boundary = { 0., 0. };
	std::vector<sim::FluidVolume> volumes{
		// sim::FluidVolume{ Vector2d{1.5, 1.5}, Vector2d{-0.98-0.25, 0.48+0.25}},
		
		// paul
		// sim::FluidVolume{ Vector2d{0.58, 0.58}, Vector2d{-0.8, -1.1}},
		// sim::FluidVolume{ Vector2d{0.58, 0.58}, Vector2d{-0.8, -0.5}}
	
		sim::FluidVolume{ Vector2d{1.,1.}, Vector2d{-0.9, 0.8}}
	};
	std::vector<Line> barriers{
		// sim::Line{Vector2f{-0.7f, 0.3f}, Vector2f{0.7f, 0.4f}}
	};
	std::vector<Box> deleteAreas{
		// hole right in the middle of the fluid
		// sim::Box{-2.5f, 0.5f, -1.5f, 1.5f} 
	};

	std::vector<sim::Inlet> inlets{
		// sim::Inlet{
		// Vector2d{-1, -1.4},	// pos
		// 0.1,					// width
		// -45.,					// rotation
		// .8,					// speed
		// 3.,					// start time
		// 15,						// end time
		// },
		// sim::Inlet{
		// Vector2d{-0.8, -0.5},	// pos
		// 0.1,					// width
		// 45.,					// rotation
		// 1.,					// speed
		// 1.,					// start time
		// 15,						// end time
		// }
	};

	s.cameras = {
		cam::Camera{{0, 0}, 5.},
	};

	s.fluidVolumes = volumes;
	s.barriers = barriers;
	s.inlets = inlets;
	// s.deleteAreas = deleteAreas;
	return s;

}

Scene BreakingDam() {
	Scene s = Scene{};

	s.gridDimension = 4;
	s.boundary = { 3.003, 2.003 };
	std::vector<sim::FluidVolume> volumes{
		sim::FluidVolume{ Vector2d{1., 1.}, Vector2d{-1., 0.5}}
	};
	std::vector<Line> barriers{
		// sim::Line{Vector2f{-0.7f, 0.3f}, Vector2f{0.7f, 0.4f}}
	};
	std::vector<Box> deleteAreas{
		// hole right in the middle of the fluid
		// sim::Box{-2.5f, 0.5f, -1.5f, 1.5f} 
	};

	s.cameras = {
		cam::Camera{{0, 0}, 3.2},
		cam::Camera{{-1, 0.}, 5.},
	};

	s.fluidVolumes = volumes;
	s.barriers = barriers;
	// s.deleteAreas = deleteAreas;
	return s;
}

Scene WaterColumn() {
	Scene s{};
	s.name = "WaterColumn";

	s.gridDimension = 1.5;
	// ---- SCENE ----
	std::vector<sim::FluidVolume> volumes{
		sim::FluidVolume{
			Vector2d{1., 1.},
			Vector2d{0., 0.1}}
	};

	std::vector<Line> barriers{
	};

	s.cameras = {
	cam::Camera{{0, 0}, 1.5},
	cam::Camera{{-1, 0.}, 5.},
	};

	s.fluidVolumes = volumes;
	s.barriers = barriers;
	s.boundary = { 1.001, 1.2 };

	return s;
}

Scene WaterColumnOfHeight(double height) {
	Scene s{};
	s.name = "WaterColumnOfHeight";

	const double boxHeight = height + 0.2;
	// the neighbour grid is a square centred on the origin, so it has to cover the taller side
	s.gridDimension = std::max(1.5, boxHeight + 0.3);
	s.boundary = { 1.001, boxHeight };

	std::vector<sim::FluidVolume> volumes{
		sim::FluidVolume{
			Vector2d{1., height},
			Vector2d{0., (boxHeight - height) / 2.}}
	};

	s.fluidVolumes = volumes;
	s.barriers = std::vector<Line>{};

	return s;
}

Scene AccelerationBox() {
	Scene s{};
	s.name = "InletOutlet";

	s.boundary = { 6.01, 4.01 };
	std::vector<sim::FluidVolume> volumes{
		sim::FluidVolume{ Vector2d{2., 2.}, Vector2d{-2., 1.}}
	};
	std::vector<Line> barriers{
		// sim::Line{Vector2f{-0.7f, 0.3f}, Vector2f{0.7f, 0.4f}}
		sim::Line{ Vector2d{2.25f, 1.25f}, Vector2d{3.03f, 1.25f}},
		sim::Line{ Vector2d{0.f, 2.f}, Vector2d{2.25f, 1.25f}}
	};
	std::vector<Box> deleteAreas{
		// hole right in the middle of the fluid
		// sim::Box{-2.5f, 0.5f, -1.5f, 1.5f} 
	};
	std::vector<sim::AccelerationArea> a_areas{
	sim::AccelerationArea{
		{-10, -10, 10, 10},
		5, 0,
		5.f, 11.f
	}
	};

	s.cameras = { cam::Camera{{0., 0.}, 7.} };

	s.fluidVolumes = volumes;
	s.barriers = barriers;
	s.accelerationAreas = a_areas;
	return s;
}

Scene DoubleBreakingDam() {
	Scene s{};

	s.gridDimension = 12.;

	// ---- SCENE ----
	std::vector<sim::FluidVolume> volumes{
		sim::FluidVolume{ Vector2d{3.f, 3.f}, Vector2d{-3.486f, 1.49f}},
		sim::FluidVolume{ Vector2d{2.f, 2.f}, Vector2d{ 4.f, 2.f}}
	};

	std::vector<sim::Line> barriers{
		// sim::Line{Vector2f{-0.7f, 0.3f}, Vector2f{0.7f, 0.4f}}
	};

	s.cameras = { cam::Camera{{0., 0.}, 12.} };

	s.fluidVolumes = volumes;
	s.barriers = barriers;
	s.boundary = { 10.f, 6.f };
	return s;
}

Scene BreakingDamAgainstSlope() {
	Scene s{};

	// ---- SCENE ----
	s.boundary = { 6.f, 4.f };

	std::vector<sim::FluidVolume> volumes{
		sim::FluidVolume{ Vector2d{2.f, 2.f}, Vector2d{-2.f, 1.f}}
	};

	std::vector<sim::Line> barriers{
		sim::Line{Vector2d{2.f, 2.f}, Vector2d{3.f, 1.f}}
	};


	s.fluidVolumes = volumes;
	s.barriers = barriers;

	return s;
}

Scene BreakingDamAcceleration() {
	Scene s{};

	s.boundary = { 6.f, 4.f };

	std::vector<sim::FluidVolume> volumes{
		sim::FluidVolume{ Vector2d{2.5f, 2.5f}, Vector2d{-1.75f, 0.75f}}
	};

	std::vector<sim::Line> barriers{
		// sim::Line{Vector2f{2.25f, 2.f}, Vector2f{2.25f, 1.25f}},
		sim::Line{ Vector2d{2.25f, 1.25f}, Vector2d{3.03f, 1.25f}},
		sim::Line{ Vector2d{0.f, 2.f}, Vector2d{2.25f, 1.25f}}
	};

	std::vector<sim::AccelerationArea> a_areas{
		sim::AccelerationArea{
			{-10, -10, 10, 10},
			5, 0,
			5.f, 15.f
		}
	};

	s.fluidVolumes = volumes;
	s.barriers = barriers;
	s.accelerationAreas = a_areas;

	return s;
}

Scene Beach() {
	Scene s{};

	s.boundary = { 30.f, 5.f };

	std::vector<sim::FluidVolume> volumes{
		sim::FluidVolume{ Vector2d{15.f, 1.5f}, Vector2d{-7.5f, 1.75f}}
	};

	std::vector<sim::Line> barriers{
		sim::Line{ Vector2d{10.f, 1.25f}, Vector2d{15.05f,1.1f}}, // horizontale
		sim::Line{ Vector2d{0.f, 2.5f}, Vector2d{10.f, 1.25f}}  // schraege
	};

	std::vector<sim::AccelerationArea> a_areas{
		// sim::AccelerationArea{
		// 	{-40.f, -40.f, 10.f, 1.75f},
		// 	0.5f, 0.f,
		// 	15.f, 60.f
		// },
		sim::AccelerationArea{
			{-40.f, -40.f, 40.f, 40.f},
			-5.f, 0.f,
			15.f, 22.f
		}
	};

	s.fluidVolumes = volumes;
	s.barriers = barriers;
	s.accelerationAreas = a_areas;

	return s;
}

Scene Ventil() {
	Scene s{};


	// ---- SCENE ----
	s.gridDimension = 41.f;
	s.boundary = { 40.f, 35.f };

	std::vector<sim::FluidVolume> volumes{
		sim::FluidVolume{ Vector2d{24.999f, 33.f}, Vector2d{-7.59f, 1.f}},
		// sim::FluidVolume{ Vector2f{2.f, 2.f}, Vector2f{ 4.f, 2.f}}
	};

	std::vector<sim::Line> barriers{
		sim::Line{Vector2d{4.99f, -15.f}, Vector2d{4.99f, 16.f}},
		sim::Line{Vector2d{4.99f, 16.5f}, Vector2d{4.99f, 18.f}},
	};

	s.cameras = { cam::Camera{{0.,0.}, 43.} };

	s.fluidVolumes = volumes;
	s.barriers = barriers;

	return s;
}

Scene Parkour() {
	Scene s{};

	s.boundary = { 12.f, 12.f };

	std::vector<sim::FluidVolume> volumes{
		sim::FluidVolume{ Vector2d{3.8f, 3.8f}, Vector2d{-4.f, -4.f}}
	};

	std::vector<sim::Line> barriers{
		sim::Line{Vector2d{-6.f, -2.f}, Vector2d{-2.5f, -1.8f}},
		sim::Line{Vector2d{-2.0f, 0.f}, Vector2d{0.f, -2.f}},
		sim::Line{Vector2d{-6.f, 0.5f}, Vector2d{1.5f, 1.f}},
		sim::Line{Vector2d{-6.f, -2.f}, Vector2d{-2.5f, -1.8f}},
		sim::Line{Vector2d{6.f, 1.f}, Vector2d{0.f,3.f}},
		sim::Line{Vector2d{-6.f, 4.f}, Vector2d{0.f, 6.f}},
		sim::Line{Vector2d{6., 3.5f}, Vector2d{3.5f, 6.f}}
	};


	s.fluidVolumes = volumes;
	s.barriers = barriers;

	return s;
}

Scene WaterDrop() {
	Scene s{};

	s.gridDimension = 13.;
	s.boundary = { 12.f, 12.f };

	std::vector<sim::FluidVolume> volumes{
		sim::FluidVolume{ Vector2d{12.f, 3.f}, Vector2d{0.f, 4.5f}},
		sim::FluidVolume{ Vector2d{2.f, 2.f}, Vector2d{-1.f, -2.f}},
		sim::FluidVolume{ Vector2d{1.5f, 2.f}, Vector2d{0.5f, -4.5f}},
		sim::FluidVolume{ Vector2d{1.5f, 1.5f}, Vector2d{-0.2f, 1.f}}
	};

	std::vector<sim::Line> barriers{


	};

	s.fluidVolumes = volumes;
	s.barriers = barriers;

	return s;
}

Scene WaterFall() {
	Scene s{};

	s.gridDimension = 15.;
	s.boundary = { 12.f, 12.f };

	std::vector<sim::FluidVolume> volumes{
		sim::FluidVolume{ Vector2d{12.f, 2.f}, Vector2d{0.f, 5.f}},
		sim::FluidVolume{ Vector2d{12.f, 3.f}, Vector2d{0.f, -4.5f}},
	};

	std::vector<sim::Line> barriers{
		sim::Line{Vector2d{-5.f, -2.948f}, Vector2d{6.1, -2.948f}},

		sim::Line{Vector2d{-6.f, -1.f}, Vector2d{4, -1.f}},
		sim::Line{Vector2d{6, 1.f}, Vector2d{-4, 1.f}},
		// sim::Line{Vector2f{-6, 3.f}, Vector2f{4.f, 3.f}},
		// sim::Line{Vector2f{-2.f, 0.f}, Vector2f{-2.f, 1.f}},
		// sim::Line{Vector2f{-2.f, 1.f}, Vector2f{0.f, 1.f}},
	};


	s.fluidVolumes = volumes;
	s.barriers = barriers;

	return s;
}

Scene Glass() {
	Scene s{};

	s.gridDimension = 10.;
	s.boundary = { 0.5f, 5.f };
	s.boundaryOffset = { 0.f, -2.f };

	std::vector<sim::FluidVolume> volumes{
		sim::FluidVolume{ Vector2d{0.2f, 0.1f}, Vector2d{0.f, 0.447f}},

		sim::FluidVolume{ Vector2d{0.03f, 4.7f}, Vector2d{0.f, -2.f}},
	};

	std::vector<sim::Line> barriers{
		sim::Line{Vector2d{-0.1f, 0.5f}, Vector2d{-0.1f, 0.2f}},
		sim::Line{Vector2d{0.1f, 0.5f}, Vector2d{0.1f, 0.2f}}
	};


	s.fluidVolumes = volumes;
	s.barriers = barriers;

	return s;
}

Scene WaterInlet() {
	Scene s{};

	s.boundary = { 1.f, 1.f };
	s.boundaryOffset = { 0.f, 0.f };

	std::vector<sim::FluidVolume> volumes{
		sim::FluidVolume{ Vector2d{0.18f, 0.1f}, Vector2d{0.f, 0.45f}},
	};

	std::vector<sim::Line> barriers{
		sim::Line{Vector2d{-0.1f, 0.5f}, Vector2d{-0.1f, 0.2f}},
		sim::Line{Vector2d{0.1f, 0.5f}, Vector2d{0.1f, 0.2f}}
	};

	std::vector<sim::Inlet> inlets{
		sim::Inlet{
		Vector2d{-0.4f, 0.f},
		0.05f,
		-90.f,
		1.7f,	// speed
		0.5f,	// start time
		2.f	// end time
		},
		sim::Inlet{
		Vector2d{0.4f, 0.f},
		0.05f,
		90.f,
		1.7f,	// speed
		2.25f,	// start time
		3.75f	// end time
		},
		sim::Inlet{
		Vector2d{0.4f, 0.f},
		0.05f,
		90.f,
		1.7f,	// speed
		6.5f,	// start time
		30.f	// end time
		}
	};

	std::vector<sim::Outlet> outlets{
		sim::Outlet{{-0.275f, 0.49f, -0.225, 0.5},
			6.f, INFINITY
		},
		sim::Outlet{{-1.025f, 0.49f, 1.025f, 0.5},
			6.f, 6.7f
		},
		sim::Outlet{{0.225f, 0.49f, 0.275, 0.5},
			6.f, INFINITY
		}
	};

	s.cameras = { cam::Camera{{0., 0.2}, 1.1 } };

	s.fluidVolumes = volumes;
	s.barriers = barriers;
	s.inlets = inlets;
	s.outlets = outlets;
	return s;
}

Scene WaterDroplet() {
	Scene s{};

	s.boundary = { 0.2f, 0.2f };
	s.boundaryOffset = { 0.f, 0.f };

	std::vector<sim::FluidVolume> volumes{
		sim::FluidVolume{ Vector2d{0.099f, 0.03f}, Vector2d{0.f, 0.085f}},
		sim::FluidVolume{ Vector2d{0.005f, 0.0125f}, Vector2d{0.f, -0.07f}},
	};

	std::vector<sim::Line> barriers{
		sim::Line{Vector2d{-0.05f, 0.1f}, Vector2d{-0.05f, 0.05f}},
		sim::Line{Vector2d{0.05f, 0.1f}, Vector2d{0.05f, 0.05f}}
	};

	// std::vector<sim::Inlet> inlets{
	// 	sim::Inlet{
	// 	sf::Vector2f{-0.4f, 0.f},
	// 	0.05f,
	// 	-90.f,
	// 	1.7f,	// speed
	// 	0.5f,	// start time
	// 	2.f	// end time
	// 	}
	// };


	s.fluidVolumes = volumes;
	s.barriers = barriers;
	// s.inlets = inlets;

	return s;
}

Scene Drop() {
	Scene s{};

	s.boundary = { 6.f, 4.f };

	std::vector<sim::FluidVolume> volumes{
		sim::FluidVolume{ Vector2d{2.f, 2.f}, Vector2d{0, 0}}
	};

	std::vector<sim::Line> barriers{
		// sim::Line{Vector2f{-0.7f, 0.3f}, Vector2f{0.7f, 0.4f}}
	};


	s.fluidVolumes = volumes;
	s.barriers = barriers;

	return s;
}

Scene ParticleBounce() {
	Scene s{};

	s.boundary = { 2.5f, 2.f };

	std::vector<sim::FluidVolume> volumes{
		sim::FluidVolume{ Vector2d{1.f, 1.f}, Vector2d{0.f, 0.5f}}
	};

	std::vector<sim::Line> barriers{
		// sim::Line{Vector2f{-1.3f, 1.15f}, Vector2f{1.4f, 1.15f}}
	};


	s.fluidVolumes = volumes;
	s.barriers = barriers;

	return s;
}

Scene FloatingSquare() {
	Scene s{};

	s.boundary = { 2.5f, 2.f };

	std::vector<sim::FluidVolume> volumes{
		sim::FluidVolume{ Vector2d{1.f, 1.f}, Vector2d{0.f, 0.f}}
	};

	std::vector<sim::Line> barriers{
		// sim::Line{Vector2f{-1.3f, 1.15f}, Vector2f{1.4f, 1.15f}}
	};


	s.fluidVolumes = volumes;
	s.barriers = barriers;

	return s;
}

Scene Brunnen() {
	Scene s{};

	s.boundary = { 5.f, 2.f };

	std::vector<sim::FluidVolume> volumes{
		// sim::FluidVolume{ Vector2f{3.5f, 0.5f}, Vector2f{-0.75f, 0.75f}}
		sim::FluidVolume{ Vector2d{5.f, 0.5f}, Vector2d{0.f, 0.75f}}
	};

	std::vector<sim::Line> barriers{
		// sim::Line{Vector2f{1.01f, 0.2f}, Vector2f{1.01f, 1.015f}}
	};

	std::vector<sim::Inlet> inlets{
	sim::Inlet{
		// sf::Vector2f{-1.95f, -0.075f-0.1f},	// start
		// sf::Vector2f{-2.05f, 0.075f-0.1f},	// end
		// 2.f,	// speed
		// 0.25f,	// start time
		// 60.f	// end time
		}
	};

	std::vector<sim::Outlet> outlets{
		// sim::Outlet{{
		// 	1.f, 0.9f,
		// 	2.5f, 1.f
		// }}
	};

	std::vector<sim::AccelerationArea> a_areas{
		sim::AccelerationArea{
			{-0.1f, 0.f, 0.1f, 0.8f},  // area
			0.f, -10.f,  // acceleration
			1.f, 15.f
		}
	};

	s.fluidVolumes = volumes;
	s.barriers = barriers;
	s.inlets = inlets;
	s.outlets = outlets;
	s.accelerationAreas = a_areas;

	return s;
}

Scene WaterStreams() {
	Scene s{};

	s.boundary = { 12.f, 12.f };

	std::vector<sim::FluidVolume> volumes{
		sim::FluidVolume{ Vector2d{12.f, 1.f}, Vector2d{0.f, 5.5f}},
	};

	std::vector<sim::Inlet> inlets{
	// sim::Inlet{
	// 	// sf::Vector2f{-3.f, -3.f},	// end
	// 	// sf::Vector2f{-4.f, -2.f},	// start
	// 	// 6.f,	// speed
	// 	// 0.5f,	// start time
	// 	// 3.f	// end time
	// 	},
	// 	sim::Inlet{
	// 	// sf::Vector2f{0.f, -2.f},	// end
	// 	// sf::Vector2f{-1.f, -3.f},	// start
	// 	// 6.f,	// speed
	// 	// 1.5f,	// start time
	// 	// 4.5f	// end time
	// 	}
	};

	s.fluidVolumes = volumes;
	s.inlets = inlets;

	return s;
}

Scene Droplet1M() {
	Scene s{};

	s.boundary = { 1.f, 1.f };

	std::vector<sim::FluidVolume> volumes{
		sim::FluidVolume{ Vector2d{1.f, 0.08f}, Vector2d{0.f, 0.46f}},
		sim::FluidVolume{ Vector2d{0.1f, 0.2f}, Vector2d{0.f, 0.f}}
	};

	std::vector<sim::Inlet> inlets{
		// sim::Inlet{
		// 	// sf::Vector2f{-3.f, -3.f},	// end
		// 	// sf::Vector2f{-4.f, -2.f},	// start
		// 	// 6.f,	// speed
		// 	// 0.5f,	// start time
		// 	// 3.f	// end time
		// 	},
		// 	sim::Inlet{
		// 	// sf::Vector2f{0.f, -2.f},	// end
		// 	// sf::Vector2f{-1.f, -3.f},	// start
		// 	// 6.f,	// speed
		// 	// 1.5f,	// start time
		// 	// 4.5f	// end time
		// 	}
	};

	s.fluidVolumes = volumes;
	s.inlets = inlets;

	return s;
}

Scene ConstantStream() {
	Scene s{};

	s.gridDimension = 13.;
	s.boundary = { 12.f, 12.f };

	std::vector<sim::FluidVolume> volumes{
		// sim::FluidVolume{ Vector2d{7.9f, 1.9f}, Vector2d{-2.f, 5.f}},
	};

	std::vector<sim::Line> barriers{
		sim::Line{Vector2d{-4.5f, -2.5f}, Vector2d{6.1, -3.f}},
		sim::Line{Vector2d{-6.f, -1.f}, Vector2d{4.f, -0.5f}},
		sim::Line{Vector2d{6.f, 1.f}, Vector2d{-4, 1.5f}},
		sim::Line{Vector2d{2.f, 6.f}, Vector2d{2.f, 3.8f}}
	};

	std::vector<sim::Outlet> outlets = {
		sim::Outlet{{2.f, 5.9f, 6.f, 6.f }}
	};

	std::vector<sim::Inlet> inlets = {
		sim::Inlet{
			Vector2d{1.f, -5.3f}, 0.8f, 85.f,	// pos, width, rot
			4.f,		//speed
			0.f, 60.f	// start, end time
		}
	};

	s.cameras = { cam::Camera{{0., 0.}, 15. }};

	s.fluidVolumes = volumes;
	s.barriers = barriers;
	s.outlets = outlets;
	s.inlets = inlets;

	return s;
}

Scene Turbulence() {
	Scene s{};

	s.boundary = { 0.998f, 0.998f };

	std::vector<sim::FluidVolume> volumes{
		sim::FluidVolume{ Vector2d{1.f, 1.f}, Vector2d{0.f, 0.f}}
	};

	std::vector<sim::AccelerationArea> a_area{
		sim::AccelerationArea{
			{-0.2, -0.05, 0.2, 0.05},
			5.f, 0.f, // acc
			0.5f, 3.f // time
		}
	};

	s.fluidVolumes = volumes;
	s.accelerationAreas = a_area;
	return s;
}

}