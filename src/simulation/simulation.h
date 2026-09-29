#pragma once
#include <vector>
#include "simulation_types.h"
#include "../scenes/simConfig.h"
#include "../scenes/scenes.h"
#include "../utils/utils.h"
#include <tuple>

using namespace simcfg;
using namespace scenes;

using time_point = std::chrono::steady_clock::time_point;
using Clock = std::chrono::steady_clock;
namespace sim {

/*--------------------------------------------------- */
// SPH2D Simulation class
class Simulation {
private:
	// --- simulation quantities ---	
	// physics quantities
	/// PARTICLE
	std::vector<double> pos_x, pos_y;		// particle positions, including boundary
	std::vector<double> vel_x, vel_y;		// particle velocities
	std::vector<double> visc_a_x, visc_a_y;	// viscosity accelerations
	std::vector<double> nonp_a_x, nonp_a_y;	// nonpressure accelerations
	std::vector<double> pa_x, pa_y;			// pressure accelerations
	// std::vector<double> mass;
	
	std::vector<double> density;			// particle densities
	std::vector<double> pressure;			// pressure at particles
	
	// --- IISPH specific quantities ---
	std::vector<double> adv_vel_x, adv_vel_y;      // intermediate velocities
	std::vector<double> source;                    // s_i = rho0 - rho_adv_i
	std::vector<double> aii;                       // diagonal value of A (A_ii)
	std::vector<double> Ap;                        // divergence of the velocity change due to pa
	
	// heavy ball and nesterov
	std::vector<double> pressure_tmp;		// lookahead factor for nesterov
	std::vector<double> pressure_prev;	// memory used for heavy ball and nesterov
	std::vector<double> Ap_prev;
	/// PARTICLE END

	auto particleArrays() {
    return std::tie(pos_x, pos_y, vel_x, vel_y, // mass,
                    visc_a_x, visc_a_y, nonp_a_x, nonp_a_y, pa_x, pa_y,
                    density, pressure,
                    adv_vel_x, adv_vel_y, source, aii, Ap,
                    pressure_tmp, pressure_prev, Ap_prev);
	}

	template <class F>
	void forEachParticleArray(F&& f) {
		std::apply([&](auto&... v) { (f(v), ...); }, particleArrays());
	};
	
	double volumeSize;						// 2D volume size in m2 (initial)
	double mass;							// particle mass;
	
	// Other IISPH specific stuff
	double sourceError = 0.;
	ISPHSourceTerm sourceTerm = Density;
	bool vdOnlyCompressed = false;		// VD: skip particles below rest density, as DFSPH does
	
	// --- PARAMETERS ---
	// Simulation Parameters
	// physical
	const double restDensity = 1.2;			// g/m2
	const double viscosity = 0.01;			// viscosity term
	double surfaceTension = 0.01;			// surface tension parameter
	const Vector2d gravity = { 0, 9.81 };	// gravity constant
	int particlesPerSquaremeter = 500;

	// config
	double stepSize = 1 / 144.0;			// simulation time step size (not in use rn)
	double cfl_lambda = 0.5;
	bool useVariableTimeStep = false;
	double maxStepSize = 1 / 144.0;

	// SESPH parameters
	const double stiffness = 100.;			// stiffness constant (factor on force)

	// IISPH parameters
	double iisphOmega = 0.5;                      // jacobi relaxation
	double iisphEta = 0.001;                      // stop condition avg density error
	int iisphMaxIterations = 100;
	int iisphMinIterations = 3;
	int fixedIters = 3;
	bool useFixedIters = false;
	double iisphBeta;

	// DFSPH parameters
	double dfsphDensityEta = 0.001;
	double dfsphVelocityEta = 0.000001;

	// --- Boundary Handling ---
	double gamma1 = 1.2;					// boundary mass scaling
	double gamma2 = 0.7;						// boundary pressure scaling
	const double boundaryViscosity = 0.03;
	double bSize = 1.;		// scale factor for boundary particles
	int boundaryLayers;
	double b_mass;

	// --- Constants ---
	// PCISPH
	double K;								// dynamic stiffness in case of SISPH

	// simulation size
	size_t n;								// amount of particles in fluid
	size_t bn = 0;							// amount of boundary particles
	double h;								// initial particle dimension

	// Kernel
	const double alpha;						// kernel variable alpha
	double kernelCorrection = 1.;
	std::vector<std::vector<KernelResult>> kernelCache;					// this stores the kernel result computed once each frame
	
	// --- neighbour search ---
	// index sort
	std::vector<int> cells;					// cells[i] = pointer to first particle in cell i in the indices vector
	std::vector<int> indices;				// contains indices of particles sorted by cell they are in
	std::vector<int> particleToCell;		// points a given particle id to its cell id
	const double gridDimension;				// edge size of the square grid in meters
	std::vector<std::vector<int>> neighboursmap;

	// --- configuration ---
	// might use constant size in some cases, like manual step?
	Scene scene;

	int steps = 0;

	// --- simulation step functions ---
	// SESPH
	void UpdateSESPH();
	void ComputeDensitiesSESPH(int i);
	void ComputePressureSESPH(int i);
	void ComputePressureAccelerationSESPH(int i);

	// PCISPH
	void UpdatePCISPH();
	void ComputeDensitiesPCISPH(int i);
	void ComputePressurePCISPH(int i);

	// --- IISPH methods ---
	void UpdateIISPH();
	void ComputeDensitiesIISPH(int i);
	void ComputeSourceTermIISPH(int i, ISPHSourceTerm sourceTerm);
	void ComputeAiiIISPH(int i);
	void ComputePressureAccelerationIISPH(int i);
	void ComputeApIISPH(int i);
	void ComputePressureIISPH(int i);

	// --- DFSPH methods ---
	void UpdateDFSPH();
	void DFSPHSolveSourceTerm(ISPHSourceTerm sourceTerm, bool useFixedIters, int iterations, double eta);

	// external forces
	void ComputeViscosity(int i);
	void ComputeSurfaceTension(int i);

	// --- neighbour search ---
	void ComputeNeighbours();										// calls function to call neighbours depending on which algorithm is active
	bool isNeighbour(int p1, int p2);								// returns true if p1 and p2 are in kernel radius (< 2*h)
	bool isNeighbour(double x, double y, int p) const;

	// setup functions
	void SetUpScene(SimulationConfig cfg);					// initialises particle vectors and places them in a square defined by volumeSize
	void fillVolumeWithParticles(FluidVolume volume);
	void GenerateBoundary();
	void GenerateSceneObjects();

	// --- dynamic scenes functions ---
	void DeleteParticle(size_t i);
	void DeleteParticles();
	void CreateParticle(double x, double y, double ax, double ay);
	void InletEmitParticles();
	void AccelerationAreas(int i);


public:
	Simulation(SimulationConfig config, Scene scene);

	// --- Simulation Functions ---
	// Update
	void Update();
	void TimeNeighboursUpdate();
	// --- kernel functions ---
	KernelResult Kernel(int i, int j) const;
	KernelResult KernelAtPos(double x, double y, int p) const;
	void ComputeKernel();

	SPHVariant sphVariant = IISPH;
	JacobiVariant jacobiVariant = RelaxedJacobi;

	// --- Getters ---
	// parameter getters/setters
	float getTimestep() const { return stepSize; }
	double getGamma1() const { return gamma1; }
	double getGamma2() const { return gamma2; }
	double getJacobiBeta() const { return iisphBeta; }

	// simulation getters
	size_t get_n() const { return n; }								// returns amount of particles
	rsize_t get_bn() const { return bn; }
	double get_h() const { return h; }								// returns constant h
	double get_bSize() const { return bSize; }
	int getSteps() const { return steps; }
	const std::vector<int>& getNeighbours(int particleId) const;			// returns vector of particleIds neighbours
	const std::vector<int> getNeighboursAtPosition(Vector2d pos) const;
	KernelResult GetKernel(int i, int j) const { return Kernel(i, j); }

	// physics getters
	const std::vector<double>& getPos_x() const { return pos_x; }	// returns all particles x positions
	const std::vector<double>& getPos_y() const { return pos_y; }	// returns all particles y positions
	double getDensity(int particleId) const { return density[particleId]; }
	double getAvgDensity() {
		double avgDensity = 0;
		for (double rho : density) {
			rho = (rho >= restDensity) ? rho : restDensity;
			avgDensity += rho;
		}
		return avgDensity / static_cast<double>(density.size());
	}
	const FieldSample sampleField(Vector2d pos) const;

	std::vector<double> getDensityVector() const { return density; }
	float getCurrentVolume() {
		float volume = 0;
		for (float rho : density) {
			volume += mass / rho;
		}
		return volume;
	}
	double getPressure(int particleId) const { return pressure[particleId]; }
	double getPressureAcc(int particleId) const { return utils::magnitude(pa_x[particleId], pa_y[particleId]); }
	double getViscosityAcc(int particleId) const { return utils::magnitude(visc_a_x[particleId], visc_a_y[particleId]); }
	double getParticleSpeed(int particleId) const { return utils::magnitude(vel_x[particleId], vel_y[particleId]); }
	float getMaxParticleSpeed() const {
		float maxSpeed = 0.0f;

		const size_t n = vel_x.size();
		for (size_t i = 0; i < n; ++i) {
			float vx = vel_x[i];
			float vy = vel_y[i];
			float speed = std::sqrt(vx * vx + vy * vy);

			if (speed > maxSpeed) {
				maxSpeed = speed;
			}
		}

		return maxSpeed;
	}
	float getRestDensity() const { return restDensity; }
	float getVolume() const { return volumeSize; }
	double getParticleRadius() const { return h / 2.; }
	float getCFLValue() const {
		return stepSize * getMaxParticleSpeed() / h;
	}
	// Analysis only, not called from the step - see simulation_util.cpp
	double GetVelocityDivergence() const;
	int gridSize() const { return cells.size(); }							// returns amount of cells in grid
	int getParticleToCell(int particleId) const {							// returns cell given particle is in
		return particleToCell[particleId];
	}

	void reverseTime() { stepSize *= -1; }
	bool getFixedIterationsBool() const { return useFixedIters; }
	int getFixedIterationsCount() const { return fixedIters; }

	// -- debug --
	double runtime = 0.;
	bool timeNeighbourComputation = false;
	time_point simulationStart;
	StepLog log;
	bool stable = true;
	void ResetRuntime() { simulationStart = Clock::now(); }
};
}