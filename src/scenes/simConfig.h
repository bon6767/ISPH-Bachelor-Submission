#pragma once
#include <vector>
#include <cmath>
#include "../simulation/simulation_types.h"

namespace simcfg {
struct SimulationConfig {
	// Resolution
	int particlesPerSquaremeter = 16;                        // amount of particles representing the fluid (will be rounded down to nearest square)
	int resStage = 3;

	// SPH Variant
	sim::SPHVariant sphVariant = sim::IISPH;
	sim::JacobiVariant jacobiVariant = sim::Nesterov;
	sim::ISPHSourceTerm sourceTerm = sim::Density;

	// consider only compressed particles for VD source term (to fix the sprayed particles
	// not reenterin
	bool vdOnlyCompressed = true;

	// SESPH
	double dtSESPH = 1 / 120.f;			// simulation stepsize in seconds SESPH
	double stiffness = 1000.f;                          // stiffness constant
	
	// IISPH
	double dtIISPH = 1 / 120.f;			// simulation stepsize in seconds IISPH
	double iisphOmega = 0.5f;                      // jacobi relaxation
	double iisphEta = 0.001f;                      // stop condition avg density error
	int iisphMaxIterations = 100;
	int iisphMinIterations = 3;
	double iisphBeta = 0.9;

	// boundary handling
	double boundaryViscosity = 0.0001f;
	double gamma1 = 1.2;
	double gamma2 = 0.7;
	double bSize = 0.5;
	int boundaryLayers = 1;	// layers of boundary particles, bh apart

	// other
	double viscosity = 0.0001f;
	double surfaceTension = 0.1;
	double fluidDensity = 1.3f;                       // in kg/m^2
	Vector2d gravity = { 0.f, 9.f };                 // gravity constant real value ~9.81


	// configurations
	bool useFixedIterations = false;
	int fixedIterations = 3;
	bool useVariableTimeStep = false;
	double cfl_lambda = 0.5;

	// analysis rules (logged, so a run is self-describing)
	double settleTime = 0.;    // time to settle before measuring
	double cflThreshold = 0.3;   // dfl threshold for labelling unstable


	double dt() const {
		double dtFactor = pow(2, resStage);
		if (sphVariant == sim::SESPH) {
			return dtSESPH / dtFactor;
		}
		return dtIISPH/dtFactor;
	};

	int particlesPerSqm() const {
		int resFactor = pow(4, resStage);
		return particlesPerSquaremeter * resFactor;
	};

	double h() const {
		int root = std::floor(sqrt(particlesPerSqm()));
		return 1. / root;
	}
};

SimulationConfig BreakingDamConfig();
SimulationConfig WaterColumnConfig();
SimulationConfig GlassConfig();
SimulationConfig GasConfig();
SimulationConfig GammaAnalysisConfig();
SimulationConfig SolverAnalysisConfig();
SimulationConfig DamProfileConfig();
SimulationConfig InletSesphConfig();	// water inlet comparison: SESPH side
SimulationConfig InletIisphConfig();	// water inlet comparison: IISPH side, same fluid


std::string SimulationConfigStr(SimulationConfig cfg, size_t boundaryParticles = 0,
                                size_t fluidParticles = 0);
}