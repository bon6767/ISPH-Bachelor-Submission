#pragma once
#include <string>
#include <vector>
#include "../utils/vector2.h"

namespace sim {

struct SceneParticles{
    std::vector<Vector2d> f;		// particle positions, including boundary
    std::vector<Vector2d> b;		// particle velocities
};

struct FluidVolume {
	Vector2d size;
    Vector2d offset;
};

struct KernelResult { double W; double gx, gy; };

struct FieldSample {
    double density; double vel_x; double vel_y;
    double speed() const {
        return sqrt(
            vel_x * vel_x +
            vel_y * vel_y
        );
    }
};

enum SimulationStatus { OK, Unstable, IterCap };
const std::string SimulationStatusStr[] = { "ok", "unstable", "iterCap" };
struct StepLog {
    double realtime;        double runtime;
    int DIIters;            int VDIters;
    double DIErr;           double VDErr;
    // splitting the different time measurements.
    // solve time - just while loop of solver (2 nbr passes each); nbr time - constructoin; 
    // kernel time - building kernel (1 nbr pass); setup time - density, v*, source, aii, (6 nbr passes)
    double solveTime = 0.;  double nbrTime;
    double kernelTime = 0.; double setupTime = 0.;
    // mean |rho_i - rho_0| / rho_0 of the actual fluid, measured before the step solves.
    // Separate from DIErr/VDErr on purpose: those are each formulation's own solver residual,
    // and the velocity-divergence one says nothing about how far the density has drifted -
    // this is the same physical quantity whichever source term is in use.
    double densityDeviation = 0.;
    // The velocity-divergence counterpart, in 1/s. Filled by the analysis loop from
    // Simulation::GetVelocityDivergence(), not by the step - it stays 0 in an ordinary run.
    double velocityDivergence = 0.;
    bool stable;            double cfl = 0.;
    SimulationStatus status = OK;
};

enum SPHVariant { SESPH, PCISPH, IISPH, DFSPH };
const std::string SPHVariantStr[] = { "SESPH", "PCISPH", "IISPH" };
enum JacobiVariant { RelaxedJacobi, HeavyBall, Nesterov };
const std::string JacobiVariantStr[] = { "RelaxedJacobi", "HeavyBall", "Nesterov" };
enum ISPHSourceTerm { Density, VelocityDiv };
const std::string ISPHSourceTermStr[] = { "Density", "VelocityDiv" };

/// Scene Elements
struct Inlet {
    Vector2d position;
    double width;
    double rotation;

    double speed;

    float startTime = 0.f;
    float endTime = 60.f;

    float timeSinceEmission = 0.f;
};

struct Box {
    double min_x;
    double min_y;
    double max_x;
    double max_y;
};


struct Line {
    Vector2d start;
    Vector2d end;
};

struct AccelerationArea {
    Box area;
    double a_x;
    double a_y;
    float startTime = 0.f;
    float endTime = 60.f;
};

struct Outlet {
    Box area;
    float startTime = 0.f;
    float endTime = 60.f;
};
}