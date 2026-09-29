#include <iostream>

#include "simulation.h"
#include "../utils/utils.h"

using Vector2d = Vector2<double>;
using time_point = std::chrono::steady_clock::time_point;
using Clock = std::chrono::steady_clock;

namespace sim {
// Stuff not in class
static inline double pi() { return 3.14159265358979323846; }
/*---------------------------------------------------*/
// SPH2D Simulation class
Simulation::Simulation(SimulationConfig cfg, Scene scene) :
    restDensity(cfg.fluidDensity),
    viscosity(cfg.viscosity),
    boundaryViscosity(cfg.boundaryViscosity),
    surfaceTension(cfg.surfaceTension),
    stiffness(cfg.stiffness),
    gravity(cfg.gravity),
    alpha(5. / (14. * pi() * cfg.h()*cfg.h())),
    scene(scene),
    gridDimension(scene.gridDimension),
    useFixedIters(cfg.useFixedIterations),
    fixedIters(cfg.fixedIterations),
    //iisph
    sourceTerm(cfg.sourceTerm),
    vdOnlyCompressed(cfg.vdOnlyCompressed),
    iisphOmega(cfg.iisphOmega),
    gamma1(cfg.gamma1),
    gamma2(cfg.gamma2),
    bSize(cfg.bSize),
    boundaryLayers(cfg.boundaryLayers),
    iisphEta(cfg.iisphEta),
    iisphMaxIterations(cfg.iisphMaxIterations),
    iisphMinIterations(cfg.iisphMinIterations),
    sphVariant(cfg.sphVariant),
    jacobiVariant(cfg.jacobiVariant),
    iisphBeta(cfg.iisphBeta),
    useVariableTimeStep(cfg.useVariableTimeStep),
    cfl_lambda(cfg.cfl_lambda),
    stepSize(cfg.dt()),
    maxStepSize(cfg.dt())
{
    // calculate h
    h = cfg.h();
    std::cout << "h=" << h << std::endl;

    // bn calculated in SetUpScene
    SetUpScene(cfg);
    ComputeNeighbours();

    mass = restDensity * h*h;
    b_mass = mass * bSize;

    // getting the center particle of the first volume
    if (scene.fluidVolumes.size()) {
        // compute the kernel gradient constant for PCISPH
        // AND compute kernel adjustment constant
        double integral = 0.;
        auto volDimensions = scene.fluidVolumes[0].size;
        int nx = std::floor(volDimensions.x / h), ny = std::floor(volDimensions.y / h);
        int cx = nx / 2, cy = ny / 2;
        int centerParticle = cx + cy * nx;

        kernelCorrection = 1.;
        auto& nbrs = getNeighbours(centerParticle);
        for (int j : nbrs) {
            // for kernel adjustment constant
            integral += GetKernel(centerParticle, j).W * volumeSize / n;
        }
        // kernel adjustment constant
        kernelCorrection = 1 / integral;

        Vector2d kernelGrad = { 0, 0 };
        double squaredKernelGrad = 0;
        for (int j : nbrs) {
            Vector2d W =
            { GetKernel(centerParticle, j).gx,
              GetKernel(centerParticle, j).gy };
            kernelGrad += W;
            squaredKernelGrad += dotProduct(W, W);
        }

        // pcisph kernel gradient constant
        double kernelGradDot = dotProduct(kernelGrad, kernelGrad);
        K = (restDensity * restDensity) / (2.0 * (stepSize * stepSize) * mass * mass * (kernelGradDot + squaredKernelGrad));
    }
    else {
        kernelCorrection = 1;
        if (sphVariant == PCISPH) { 
            std::cerr << "can't compute K" << std::endl; 
        }
    }

    ComputeKernel();
    for (int i = 0; i < n; i++) {
        ComputeDensitiesSESPH(i);
    }

    bool printInitLog = false;
    if (printInitLog) {
        std::cout << "Calculated K = " << K << std::endl;
        std::cout << "created simulation with "
            << n
            << " fluid particles, and " << bn << " boundary particles\n"
            << "h = " << h
            << "amount of cells " << gridSize() << std::endl;
    }
    simulationStart = Clock::now();
}

void Simulation::Update() {
    log = StepLog{};

    // first things first. delete particles
    DeleteParticles();
    InletEmitParticles();

    if (useVariableTimeStep) {
        auto vmax = getMaxParticleSpeed();
        vmax = (vmax == 0) ? 1 : vmax;
        stepSize = (cfl_lambda * h) / vmax;
        if (stepSize > maxStepSize) { stepSize = maxStepSize; }
    }
    switch (sphVariant) {
    case SESPH:
        ComputeNeighbours();
        ComputeKernel();

        UpdateSESPH();
        break;
    case PCISPH:
        ComputeNeighbours();
        ComputeKernel();

        UpdatePCISPH();
        break;
    case IISPH:
        ComputeNeighbours();
        {   
            // measure kernel constructiion cause it also loops the nbrs, so why not
            const auto kernelStart = Clock::now();
            ComputeKernel();
            log.kernelTime =
                std::chrono::duration<double, std::milli>(Clock::now() - kernelStart).count();
        }
        UpdateIISPH();
        break;
    case DFSPH:
        // Neighbours and Kernel handled in DFSPH
        UpdateDFSPH();
        break;
    }

    runtime += stepSize;
    steps++;

    /// logging
    const double realtime =
        std::chrono::duration<double>(
            Clock::now() - simulationStart
        ).count();
    log.realtime = realtime;
    // after the velocity update, so it reflects the state this step just produced
    log.cfl = getCFLValue();

}

/// Shared Physics Function
void Simulation::ComputeViscosity(int i) {
    visc_a_x[i] = 0;
    visc_a_y[i] = 0;
        
    double x_i = pos_x[i], y_i = pos_y[i];   // position of particle i
    double vx_i = vel_x[i], vy_i = vel_y[i]; // velocity of particle i

    const auto& nbrs = getNeighbours(i);
    const auto& kernel = kernelCache[i];

    for (int k = 0; k < nbrs.size(); k++) {
        int j = nbrs[k];
        double mass_j = (j >= n) ? b_mass : mass;

        double x_j = pos_x[j], y_j = pos_y[j];   // position of particle j
        double vx_j = (j >= n) ? 0 : vel_x[j];   // velocity of particle j
        double vy_j = (j >= n) ? 0 : vel_y[j];   // velocity of particle j

        double nbr_visc = (j >= n) ? boundaryViscosity : viscosity;
        double density_j = (j >= n) ? restDensity : density[j];
        double v = mass_j / density_j;     // neighbour particle volume


        double x_ij = x_i - x_j, y_ij = y_i - y_j; // position difference
        double vx_ij = vx_i - vx_j, vy_ij = vy_i - vy_j; // velocity difference

        double numerator = (vx_ij * x_ij) + (vy_ij * y_ij);
        double denominator = (x_ij * x_ij) + (y_ij * y_ij) + 0.01 * h * h;

        visc_a_x[i] += 2 * nbr_visc * v * (numerator / denominator) * kernel[k].gx;
        visc_a_y[i] += 2 * nbr_visc * v * (numerator / denominator) * kernel[k].gy;
    }

    // apply visc_a to nonpressure acceleration
    nonp_a_x[i] += visc_a_x[i];
    nonp_a_y[i] += visc_a_y[i];
}

void Simulation::ComputeSurfaceTension(int i) {
    const auto& nbrs = getNeighbours(i);
    const auto& kernel = kernelCache[i];

    for (int k = 0; k < nbrs.size(); k++) {
        int j = nbrs[k];

        if (j >= n) continue; // skip boundary particles
        auto force = -surfaceTension * kernel[k].W;
        double diff_x = pos_x[i] - pos_x[j];
        double diff_y = pos_y[i] - pos_y[j];
        nonp_a_x[i] += force * diff_x;
        nonp_a_y[i] += force * diff_y;
    }
}

}