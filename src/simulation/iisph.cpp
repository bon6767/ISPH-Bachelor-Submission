#include "simulation.h"
#include <cmath>
#include <iostream> /// can delete after debugging

namespace sim {

void Simulation::UpdateIISPH() {
    // measure the time of setup before solving (6 iterationf over neighbours)
    const auto setupStart = Clock::now();

    /// STEP 1: DENSITIES

    // introduced this term to compare DI with VD
    // reduction, not a bare parallel for: every thread accumulates into densityDeviation, so
    // without it the increments race and the sum silently comes out low and non-deterministic
    double densityDeviation = 0.;
#pragma omp parallel for reduction(+:densityDeviation)
    for (int i = 0; i < n; i++) {
        ComputeDensitiesIISPH(i);
        densityDeviation += std::abs(density[i] - restDensity);
    }
    log.densityDeviation = densityDeviation / (static_cast<double>(n) * restDensity);

    /// STEP 2: NON PRESSURE VELOCITY STEP
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

    /// STEP 3: SOURCE TERM,    DIAGONAL,   PRESSURE INIT
#pragma omp parallel for
    for (int i = 0; i < n; i++) {
        ComputeSourceTermIISPH(i, sourceTerm);
        ComputeAiiIISPH(i);

        // Fix particles not remerging.
        if (sourceTerm == VelocityDiv && density[i] < restDensity) { aii[i] = 0.; }

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


    const double setupTime =
        std::chrono::duration<double, std::milli>(
            Clock::now() - setupStart).count();

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
            && ((sourceError > iisphEta && iter < iisphMaxIterations)
                || iter < iisphMinIterations)
            || useFixedIters
            && iter < fixedIters;
    }

    /// Logging
    const double solveTime =
        std::chrono::duration<double, std::milli>(
            Clock::now() - solveStart).count();
    int DIIters; int VDIters;
    double DIErr; double VDErr;
    if (sourceTerm == Density) { DIIters = iter; VDIters = 0; DIErr = sourceError; VDErr = 0.; }
    else { DIIters = 0; VDIters = iter; DIErr = 0.; VDErr = sourceError; }
    log.runtime = runtime;
    log.DIIters = DIIters;    log.VDIters = VDIters;
    log.DIErr = DIErr;        log.VDErr = VDErr;
    log.solveTime = solveTime;
    log.setupTime = setupTime;
    log.stable = stable;
    log.status = !stable ? Unstable
        : (iter >= iisphMaxIterations ? IterCap : OK);


#pragma omp parallel for
    for (int i = 0; i < n; i++) {
        vel_x[i] = adv_vel_x[i] + stepSize * pa_x[i];
        vel_y[i] = adv_vel_y[i] + stepSize * pa_y[i];
        // update position
        pos_x[i] += vel_x[i] * stepSize;
        pos_y[i] += vel_y[i] * stepSize;
    }
}

/// ###########################################
/// ############## [ I I S P H ] ##############
/// ###########################################
/// ########## [ F U N C T I O N S ] ##########
/// ###########################################
void Simulation::ComputeDensitiesIISPH(int i) {
    //  $\rho_i=\sum_j m_j W_{ij}+\sum_b m_bW_ib$
    const auto& nbrs = getNeighbours(i);
    const auto& kernel = kernelCache[i];

    density[i] = 0.;

    for (int k = 0; k < nbrs.size(); k++) {
        int j = nbrs[k];
        double mass_j = (j >= n) ? gamma1 * b_mass : mass;
        density[i] += kernel[k].W * mass_j; // current density
    }
}

void Simulation::ComputeSourceTermIISPH(int i, ISPHSourceTerm sourceTerm) {
    // s_i=\rho^0_i - \rho_i
    // - \Delta t \sum_j m_j (\v^*_i-\v^*_j)\nabla W_{ij}
    // - \sum_b m_b (\v^*_i-\v^*_b(t+\Delta t))\nabla W_{ib}
    const auto& nbrs = getNeighbours(i);
    const auto& kernel = kernelCache[i];

    double s = 0.;
    if (sourceTerm == Density) { s = restDensity - density[i]; }

    for (int k = 0; k < nbrs.size(); k++) {
        int j = nbrs[k];

        double vx_ij = adv_vel_x[i]; // init the velocity diff x
        vx_ij -= (j >= n) ? 0 : adv_vel_x[j]; // subtract if not a boundary
        double vy_ij = adv_vel_y[i]; // init the velocity diff y
        vy_ij -= (j >= n) ? 0 : adv_vel_y[j]; // subtract if not a boundary

        double mass_j = (j >= n) ? b_mass : mass;
        double densitychange = vx_ij * kernel[k].gx + vy_ij * kernel[k].gy;

        s -= stepSize * mass_j * densitychange;
    }
    source[i] = s;
}

void Simulation::ComputeAiiIISPH(int i) {
    const auto& nbrs = getNeighbours(i);
    const auto& kernel = kernelCache[i];

    // rho0^2
    double rho2 = restDensity * restDensity;
    // Delta t^2
    double dt2 = stepSize * stepSize;

    // compute c
    // \c_i = -\sum_j m_j / \rho_0^2 \nabla W_{ij}
    // - 2\gamma\sum_b m_b/ \rho_0^2}\nabla W_{ib}
    double cx = 0.;
    double cy = 0.;

    for (int k = 0; k < nbrs.size(); k++) {
        int j = nbrs[k];
        double mass_j = (j >= n) ? b_mass : mass;

        double factor = mass_j / rho2;
        if (j >= n) {
            factor *= 2 * gamma2;
        }
        // if neighbour, apply factor gamma

        cx -= factor * kernel[k].gx;
        cy -= factor * kernel[k].gy;
    }

    // compute aii
    // \A_{ii}=
    //  \Delta t^2\sum_j m_j \c_i\cdot\nabla W_{ij}
    // +\Delta t^2\sum_j m_j (m_i/\rho_0^2\cdot\nabla W_{ij})\cdot\nabla W_{ij}
    // +\Delta t^2\sum_b m_b \c_i\cdot\nabla W_{ib}
    // boundary or not is the same here (?)

    double a = 0.;

    for (int k = 0; k < nbrs.size(); k++) {
        int j = nbrs[k];
        double mass_j = (j >= n) ? b_mass : mass;


        // first part
        a += dt2 * mass_j * (cx * kernel[k].gx + cy * kernel[k].gy);

        // second part
        // note that its W_ji = -W_ij !!!
        double termx = -mass_j / rho2 * kernel[k].gx;
        double termy = -mass_j / rho2 * kernel[k].gy;
        double cross = termx * kernel[k].gx + termy * kernel[k].gy;
        a += (j >= n) ? 0 : dt2 * mass_j * cross;

        // third part is already included in first part
    }

    aii[i] = a;
}

void Simulation::ComputePressureAccelerationIISPH(int i) {
    // (\a_i^p)^l =
    // -\sum_j m_j (p^l_i/(\rho^0)^2 + p^l_j/(\rho^0)^2) \cdot\nabla W_{ij}
    // -\gamma\sum_b m_b * 2 * p^l_i\(\rho^0)^2 \cdot\nabla W_{ij}

    const auto& nbrs = getNeighbours(i);
    const auto& kernel = kernelCache[i];

    // rho0^2
    double rho2 = restDensity * restDensity;

    double ax = 0.;
    double ay = 0.;
    for (int k = 0; k < nbrs.size(); k++) {
        int j = nbrs[k];
        double mass_j = (j >= n) ? b_mass : mass;


        double p_factor = pressure[i] / rho2;
        if (j >= n) { p_factor *= 2 * gamma2; } // is boundary
        else        { p_factor += pressure[j] / rho2; }

        ax -= mass_j * p_factor * kernel[k].gx;
        ay -= mass_j * p_factor * kernel[k].gy;
    }
    pa_x[i] = ax;
    pa_y[i] = ay;
}

void Simulation::ComputeApIISPH(int i) {
    const auto& nbrs = getNeighbours(i);
    const auto& kernel = kernelCache[i];

    // Delta t^2
    double dt2 = stepSize * stepSize;

    double a = 0.;
    
    for (int k = 0; k < nbrs.size(); k++) {
        int j = nbrs[k];
        double mass_j = (j >= n) ? b_mass : mass;

        double dax = pa_x[i];  // acceleration diff
        double day = pa_y[i];  // acceleration diff

        dax -= (j >= n) ? 0 : pa_x[j]; // if not boundary, subtract other pa
        day -= (j >= n) ? 0 : pa_y[j]; // if not boundary, subtract other pa

        dax *= dt2 * mass_j;
        day *= dt2 * mass_j;
        a += dax * kernel[k].gx + day * kernel[k].gy;
    }
    Ap[i] = a;
}

void Simulation::ComputePressureIISPH(int i) {
    if (aii[i] == 0)  return;

    // iterations 2 and on:
    switch (jacobiVariant)
    {
    case sim::RelaxedJacobi:
        pressure[i] = std::max(
            pressure[i] + iisphOmega * (source[i] - Ap[i]) / aii[i], 0.);
        break;
    case sim::HeavyBall:
    {
        double p_old = pressure[i];
        pressure[i] = std::max(
            pressure[i] + (iisphOmega * (source[i] - Ap[i]) / aii[i]) // jacobi
            + (iisphBeta * (pressure[i] - pressure_prev[i])), 0.); // heavy ball

        // update previous
        pressure_prev[i] = p_old;
        break;
    }
    case sim::Nesterov:
    {
        double Ap_tmp = (1 + iisphBeta) * Ap[i] - iisphBeta * Ap_prev[i];
        // nesterov lookahead factor = p1+beta*(p1-0)
        double p_new = std::max(
            pressure_tmp[i] + iisphOmega * (source[i] - Ap_tmp) / aii[i], 0.);

        pressure_tmp[i] = p_new + iisphBeta * (p_new - pressure[i]);
        pressure[i] = p_new;
        Ap_prev[i] = Ap[i];
        break;
    }
    default:
        break;
    }
}
}