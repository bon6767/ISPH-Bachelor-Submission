#include "simConfig.h"
#include <sstream>

// injected by CMake (target_compile_definitions); "unknown" when the build does not set it
#ifndef GIT_HASH
#define GIT_HASH "unknown"
#endif

using namespace sim;

namespace simcfg {
SimulationConfig WaterColumnConfig()
{
	SimulationConfig cfg{};

	// Resolution
	cfg.particlesPerSquaremeter = 16;
	cfg.resStage = 5;

	// SESPH
	cfg.dtSESPH = 0.0002;
	cfg.stiffness = 5000.;

	// IISPH
	cfg.dtIISPH = 0.001;
	cfg.iisphOmega = 0.5;	// jacobi relaxation
	cfg.iisphBeta = 1.;
	cfg.iisphEta = 0.001;	// stop condition avg density error
	cfg.iisphMaxIterations = 100;
	cfg.useVariableTimeStep = false;

	// boundary handling
	cfg.gamma1 = 1.1;
	cfg.gamma2 = 0.7;
	cfg.boundaryViscosity = 0.00001;

	// other
	cfg.viscosity = 0.005;
	cfg.surfaceTension = 0.0001;
	cfg.fluidDensity = 999.9;
	cfg.gravity = { 0., 9. };

	return cfg;
}

SimulationConfig GammaAnalysisConfig() {
	SimulationConfig cfg{};
	cfg.sphVariant = IISPH;
	cfg.jacobiVariant = RelaxedJacobi;

	cfg.iisphOmega = 0.5;	// jacobi relaxation
	// cfg.iisphBeta = 0.85;	// momentum weight
	
	// Resolution
	cfg.particlesPerSquaremeter = 16;
	cfg.resStage = 4;

	// IISPH
	cfg.dtIISPH = 0.0128;	// dt() = 8e-4, dt/h = 0.051: the regime of the original sweep
	cfg.iisphEta = 0.001;	// stop condition avg density error
	cfg.iisphMaxIterations = 300;	// cap == fail, ~3-5x the expected healthy count at this dt
	cfg.cflThreshold = 0.5;	// marks a run failed in the measurement window

	// boundary handling
	cfg.gamma1 = 1.1;
	cfg.gamma2 = 0.5;
	cfg.boundaryViscosity = 0.0001;

	// other
	cfg.viscosity = 0.0001;
	cfg.surfaceTension = 0.05;
	cfg.fluidDensity = 999.9;
	cfg.gravity = { 0., 9. };

	return cfg;
}

// The IISPH solver analyses (jacobi omega scan, fixed-budget sweeps) share one configuration
SimulationConfig SolverAnalysisConfig() {
	SimulationConfig cfg{};
	cfg.sphVariant = IISPH;
	cfg.jacobiVariant = RelaxedJacobi;
	cfg.iisphOmega = 0.5;
	cfg.iisphBeta = 0.9;

	// 16384 particles at dt = 0.001 -> dt/h = 0.128, the regime of the momentum runs
	cfg.particlesPerSquaremeter = 16;
	cfg.resStage = 5;
	cfg.dtIISPH = 0.032;

	cfg.iisphEta = 0.001;
	cfg.iisphMaxIterations = 300;	// cap == fail: 4x the ~75 iterations a healthy jacobi needs

	// the cell the gamma sweep picked, which is also what the momentum runs used
	cfg.gamma1 = 1.1;
	cfg.gamma2 = 0.6;
	cfg.boundaryViscosity = 0.00001;

	cfg.viscosity = 0.00001;
	cfg.surfaceTension = 0.05;
	cfg.fluidDensity = 1.3;
	cfg.gravity = { 0., 9. };

	cfg.settleTime = 0.5;
	cfg.cflThreshold = 0.5;

	return cfg;
}

/// The dam iteration-profile run. Shared by the logged sweep and the interactive run, so the
/// frames grabbed on screen belong to the same simulation as the curve in the figure - one
/// function rather than two copies that quietly drift apart.
SimulationConfig DamProfileConfig() {
	SimulationConfig cfg = SolverAnalysisConfig();
	cfg.jacobiVariant = RelaxedJacobi;
	cfg.iisphOmega = 0.5;
	cfg.surfaceTension = 1.;
	cfg.viscosity = 0.001;
	cfg.dtIISPH = 0.0004 * 32.;
	cfg.settleTime = 0.;					// no settle: the release transient is the subject

	return cfg;
}

/// The water inlet comparison. One scene, one fluid, two pressure models - everything below is
/// shared, and the two wrappers differ only in which solver runs, so any difference in the
/// result belongs to the solver rather than to the setup.
///
/// Built on GlassConfig because that is what the scene was authored against, with the fluid
/// swapped for the light one the rest of the analysis uses: at rho_0 = 999.9 a stiffness of 5000
/// cannot hold a column up at all, since the compression it needs goes as rho_0^2 g H / k.
static SimulationConfig InletComparisonBase() {
	SimulationConfig cfg = GlassConfig();

	/// One stage finer than the first comparison, which halves h and with it both timesteps -
	/// dt() divides by 2^resStage, so neither dtSESPH nor dtIISPH needs touching.
	///
	/// The expectation is that this separates them. SESPH's limit is the speed of sound, so its
	/// step halves while each step costs four times as much: eight times the work. IISPH's step
	/// is set by the flow's CFL, which also halves it, but its iteration count goes as dt^2 / h
	/// and therefore halves too - four times the work. A factor of two should open up, where at
	/// 16384 the two were within 2% of each other.
	cfg.resStage = 7;
	cfg.fluidDensity = 1.3;
	cfg.bSize = 0.5;

	cfg.stiffness = 5000.;
	/// dt = 9.77e-6 at resStage 7, which is 1.24x SESPH's stability criterion of
	/// 0.25 h / sqrt(k/rho_0) = 7.88e-6.
	///
	/// The margin has had to shrink twice, and each time a run showed it. 5x the criterion blew
	/// up at t = 27.4s; 2.5x survived 30s at 65536 particles but failed at t = 3.65s once the
	/// resolution went one stage finer - not on CFL, which was 0.17 there, but with a particle
	/// leaving the grid. So the criterion is not merely conservative: how far above it a run can
	/// sit depends on the resolution, and at this one there is almost no room left.
	cfg.dtSESPH = 0.00125;			// dt = 9.77e-6
	cfg.dtIISPH = 0.0005 * 32.;		// dt = 5e-4

	// hand-tuned until SESPH held this scene: a jet hitting a glass is harder on it than a
	// settling column, and it needs the damping. IISPH does not, but takes it anyway - a
	// comparison against a fluid only one of the two can handle would not be one.
	cfg.viscosity = 0.007;
	cfg.surfaceTension = 0.1;
	cfg.gamma1 = 1.1;
	cfg.gamma2 = 0.7;

	return cfg;
}

SimulationConfig InletSesphConfig() {
	SimulationConfig cfg = InletComparisonBase();
	cfg.sphVariant = SESPH;
	return cfg;
}

SimulationConfig InletIisphConfig() {
	SimulationConfig cfg = InletComparisonBase();
	cfg.sphVariant = IISPH;
	/// Relaxed jacobi at the relaxation the omega scan picked, so this comparison and the water
	/// column one are the same solver. GlassConfig leaves jacobiVariant at its default, which is
	/// Nesterov - fine for looking at, but not the baseline the thesis compares against.
	/// Heavy ball rather than relaxed jacobi: the point of the thesis, and 5-7x fewer iterations
	/// in the dam at matched accuracy. beta 0.8 / omega 1.1 is the pick the water column made at
	/// 64009 particles - the dam found the higher-resolution row transferred better than the
	/// same-resolution one, and this scene is finer than either.
	cfg.jacobiVariant = HeavyBall;
	cfg.iisphBeta = 0.8;
	cfg.iisphOmega = 1.1;
	cfg.iisphMaxIterations = 300;
	/// Without this the comparison measures the floor rather than the method. Heavy ball should
	/// reach eta here in two to three iterations, and a floor of 3 would hide any improvement
	/// below that - exactly what happened in the dam sweep, where it read 3.0 at every timestep.
	cfg.iisphMinIterations = 1;
	/// The mean density error SESPH reached in this scene, measured rather than chosen. eta
	/// bounds the same quantity SESPH logs to DIErr, so setting it to SESPH's result is what
	/// makes the two comparable - at GlassConfig's 1e-4 IISPH was solving to three times the
	/// accuracy and paying for it.
	cfg.iisphEta = 0.000274;
	/// Momentum changes where the best timestep is, not just how many iterations it needs.
	/// Relaxed jacobi needs N ~ dt^2, so (T/dt)(c0 + N c_iter) has an interior minimum - at
	/// 65536 that was dt = 2.4e-4 with 12 iterations. Heavy ball needs N ~ dt, and then the
	/// solve term is T a c_iter, a constant: the cost falls monotonically with dt and the only
	/// thing stopping it is the CFL condition. So this is no longer chosen at a cost optimum but
	/// pushed as close to the flow's limit as is prudent.
	///
	/// 1.25e-4 was the first attempt, scaled from the 0.361 that 2.125e-4 gave at 65536 on the
	/// assumption that CFL follows dt. It does not, quite: the run peaked at 0.691, because a
	/// finer grid resolves the jets better and they reach higher speeds. Mean CFL was only 0.145
	/// and p95 0.228, so the breach was one impact rather than a sustained overrun, but 0.5 is
	/// the criterion applied everywhere else in the thesis and a peak over it is a peak over it.
	///
	/// 8e-5 scales that measured 0.691 down to about 0.44. Momentum absorbs part of the cost:
	/// heavy ball needs N ~ dt, so a smaller step also needs fewer iterations per step.
	cfg.dtIISPH = 0.00008 * 128.;	// dt = 8e-5 at resStage 7
	return cfg;
}

SimulationConfig BreakingDamConfig()
{
	SimulationConfig cfg{};
	
	// Resolution
	cfg.particlesPerSquaremeter = 16;
	cfg.resStage = 5;
	
	// SESPH
	cfg.dtSESPH = 0.0005;
	cfg.stiffness = 1000.;
	
	// IISPH
	cfg.jacobiVariant = HeavyBall;
	cfg.dtIISPH = 0.008;
	cfg.iisphOmega = 0.55;	// jacobi relaxation
	cfg.iisphBeta = 0.93;
	cfg.iisphEta = 0.0001;	// stop condition avg density error
	cfg.iisphMaxIterations = 100;
	
	// boundary handling
	cfg.gamma1 = 1.1;
	cfg.gamma2 = 0.5;
	cfg.boundaryViscosity = 0.005;

	// other
	cfg.viscosity = 0.05;
	cfg.surfaceTension = 0.2;
	cfg.fluidDensity = 999.9;
	cfg.gravity = { 0., 9. };

	return cfg;
}

SimulationConfig GlassConfig()
{
	SimulationConfig cfg{};

	// Resolution
	cfg.particlesPerSquaremeter = 16;
	cfg.resStage = 5;

	// SESPH
	cfg.dtSESPH = 0.0005;
	cfg.stiffness = 1000.;

	// IISPH
	cfg.dtIISPH = 0.02;
	cfg.iisphOmega = 0.5;	// jacobi relaxation
	cfg.iisphEta = 0.0001;	// stop condition avg density error
	cfg.iisphMaxIterations = 100;
	cfg.iisphBeta = 0.9;

	// boundary handling
	cfg.gamma1 = 1.1;
	cfg.gamma2 = 0.5;
	cfg.boundaryViscosity = 0.003;

	// other
	cfg.viscosity = 0.005;
	cfg.surfaceTension = 0.1;
	cfg.fluidDensity = 999.9;
	cfg.gravity = { 0., 9. };

	return cfg;
}

SimulationConfig GasConfig()
{
	SimulationConfig cfg{};

	// Resolution
	cfg.particlesPerSquaremeter = 2048 * 2;

	cfg.sphVariant = SESPH;

	// SESPH
	cfg.dtSESPH = 0.0001;
	cfg.stiffness = 50;

	// boundary handling
	cfg.gamma1 = 10.;
	cfg.gamma2 = 10.;
	cfg.boundaryViscosity = 0.00001;

	// other
	cfg.viscosity = 0.00001;
	cfg.surfaceTension = 0.05;
	cfg.fluidDensity = 0.01;
	cfg.gravity = { 0., 9. };

	return cfg;
}


std::string SimulationConfigStr(SimulationConfig cfg, size_t boundaryParticles,
	size_t fluidParticles) {

	std::ostringstream cfgString;
	cfgString << "{"
		<< "\"sphVariant\":\"" << sim::SPHVariantStr[cfg.sphVariant] << "\", "
		<< "\"jacobiVariant\":\"" << sim::JacobiVariantStr[cfg.jacobiVariant] << "\", "
		// which residual the solver drives to zero - two runs that differ only in this are
		// otherwise indistinguishable in the header
		<< "\"sourceTerm\":\"" << sim::ISPHSourceTermStr[cfg.sourceTerm] << "\", "
		<< "\"vdOnlyCompressed\":" << cfg.vdOnlyCompressed << ", "
		<< "\"iiSPHOmega\":" << cfg.iisphOmega << ", "
		<< "\"iisphBeta\":" << cfg.iisphBeta << ", "
		<< "\"dt\":" << cfg.dt() << ", "
		<< "\"stiffness\":" << cfg.stiffness << ", "
		<< "\"iisphEta\":" << cfg.iisphEta << ", "
		<< "\"iisphMaxIterations\":" << cfg.iisphMaxIterations << ", "
		<< "\"useFixedIterations\":" << cfg.useFixedIterations << ", "
		<< "\"fixedIterations\":" << cfg.fixedIterations << ", "
		<< "\"particlesPerSqm\":" << cfg.particlesPerSqm() << ", "
		<< "\"fluidDensity\":" << cfg.fluidDensity << ", "
		<< "\"viscosity\":" << cfg.viscosity << ", "
		<< "\"boundaryViscosity\":" << cfg.boundaryViscosity << ", "
		<< "\"surfaceTension\":" << cfg.surfaceTension << ", "
		<< "\"gamma1\":" << cfg.gamma1 << ", "
		<< "\"gamma2\":" << cfg.gamma2 << ", "
		<< "\"settleTime\":" << cfg.settleTime << ", "
		<< "\"cflThreshold\":" << cfg.cflThreshold << ", "
		<< "\"boundaryLayers\":" << cfg.boundaryLayers << ", "
		<< "\"boundaryParticles\":" << boundaryParticles << ", "
		// the scene's size in particles: cfg fixes the resolution, not how much fluid there is
		<< "\"fluidParticles\":" << fluidParticles << ", "
		<< "\"commit\":\"" << GIT_HASH << "\""
		<< "}";
	return cfgString.str();
}

}