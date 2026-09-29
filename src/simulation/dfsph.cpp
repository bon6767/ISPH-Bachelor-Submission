#include "simulation.h"

namespace sim {
void Simulation::UpdateDFSPH() {
    /// STEP 1: NON PRESSURE VELOCITY STEP
#pragma omp parallel for
    for (int i = 0; i < n; i++) {
        // calculate non pressure accelerations
        nonp_a_x[i] = gravity.x;
        nonp_a_y[i] = gravity.y;
        ComputeViscosity(i);
        ComputeSurfaceTension(i);
        AccelerationAreas(i); // debug note this was before viscosity before

        // apply nonpressure accelerations
        adv_vel_x[i] = vel_x[i] + nonp_a_x[i] * stepSize;
        adv_vel_y[i] = vel_y[i] + nonp_a_y[i] * stepSize;
    }

    /// STEP 2: SOLVE DI
    DFSPHSolveSourceTerm(Density, false, 5, 0.0001);
#pragma omp parallel for
    for (int i = 0; i < n; i++) {
        // update position
        pos_x[i] += adv_vel_x[i] * stepSize;
        pos_y[i] += adv_vel_y[i] * stepSize;
    }

    /// STEP 3: COMPUTE NEIGHBOURS
    ComputeNeighbours();
    ComputeKernel();

    /// STEP 4: UPDATE DENSITIES
#pragma omp parallel for
    for (int i = 0; i < n; i++) {
        ComputeDensitiesIISPH(i);
    }

    /// STEP 5: SOLVE VD
    DFSPHSolveSourceTerm(VelocityDiv, false, 3, 0.00001);

    /// STEP 6: UPDATE VELOCITIES
    for (int i = 0; i < n; i++) {
        vel_x[i] = adv_vel_x[i];
        vel_y[i] = adv_vel_y[i];
    }

}

void Simulation::DFSPHSolveSourceTerm(ISPHSourceTerm sourceTerm, bool useFixedIters, int iterations, double eta) {
    /// SOURCE TERM,    DIAGONAL,   PRESSURE INIT
#pragma omp parallel for
    for (int i = 0; i < n; i++) {
        ComputeSourceTermIISPH(i, sourceTerm);
        ComputeAiiIISPH(i);
        // if (sourceTerm == VelocityDiv && density[i] < restDensity) { aii[i] = 0.; }

        /// INITIALISING pressure for the solver
        if (aii[i] != 0) {
            if (jacobiVariant == Nesterov) {
                pressure[i] = std::max(iisphOmega * (source[i] / aii[i]), 0.);
                pressure_tmp[i] = pressure[i] * (1 + iisphBeta);
                Ap_prev[i] = 0.;
            }
            else {
                // normal jacobi or heavy ball
                pressure_prev[i] = 0;
                pressure[i] = std::max(iisphOmega * (source[i] / aii[i]), 0.);
            }
        }
        else {
            pressure_tmp[i] = 0;
            pressure_prev[i] = 0;
            pressure[i] = 0;
            Ap_prev[i] = 0;
        }
    }

    // initialising iterations, while condition bool and solve timer.
    int iter = 0;
    bool whileCondition = true;
    // Time the iteration loop only
    // steady_clock, not high_resolution_clock, which is a non-monotonic system_clock alias on libstdc++.
    const auto solveStart = Clock::now();
    /// ITERATIONS
    while (whileCondition) { //repeat until 0.1% pressure error
        sourceError = 0.f;
        iter++;

#pragma omp parallel for
        for (int i = 0; i < n; i++) {
            ComputePressureAccelerationIISPH(i);
        }

        double err = 0.;
#pragma omp parallel for reduction(+:err)
        for (int i = 0; i < n; i++)
        {
            ComputeApIISPH(i);
            if (sourceTerm == Density) {
                err += std::max(Ap[i] - source[i], 0.);
            }
            if (sourceTerm == VelocityDiv) {
                // err += std::abs(Ap[i] - source[i]);
                err += std::max(Ap[i] - source[i], 0.);
                // err += Ap[i] - source[i];
            }
            ComputePressureIISPH(i);
        }
        sourceError = err / (static_cast<double>(n) * restDensity); // so its relative

        whileCondition =
            !useFixedIters
            && ((sourceError > eta && iter < iisphMaxIterations)
                || iter < iisphMinIterations)
            || useFixedIters
            && iter < iterations;
    }

    /// Logging
    const double solveTime =
        std::chrono::duration<double, std::milli>(
            Clock::now() - solveStart).count();
    if (sourceTerm == Density) {
        log.DIIters = iter;
        log.DIErr = sourceError;
    }
    else {
        log.VDIters = iter;
        log.VDErr = sourceError;
    }
        log.solveTime += solveTime;


    /// Update Velocity
#pragma omp parallel for
    for (int i = 0; i < n; i++) {
        adv_vel_x[i] += stepSize * pa_x[i];
        adv_vel_y[i] += stepSize * pa_y[i];
    }
}

}