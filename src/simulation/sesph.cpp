#include "simulation.h"
#include <cmath>


namespace sim{
// standard densities computation used for standard SPH
void Simulation::ComputeDensitiesSESPH(int i) {
    density[i] = 0.;

    auto& nbrs = getNeighbours(i);
    const auto& kernel = kernelCache[i];

    for (int k = 0; k < nbrs.size(); k++) {
        int j = nbrs[k];

        double mass_j = (j >= n) ? gamma1 * b_mass : mass;
        density[i] += kernel[k].W * mass_j; // current density
    }
}

// standard pressure computation used before PCISPH
void Simulation::ComputePressureSESPH(int i) {
    pressure[i] = std::max(stiffness * ((density[i] / restDensity) - 1), 0.);
    // gas equation
    // pressure[i] = stiffness * std::pow(density[i], 1.);
}

void Simulation::ComputePressureAccelerationSESPH(int i) {
    pa_x[i] = 0;
    pa_y[i] = 0;

    const auto& nbrs = getNeighbours(i);
    const auto& kernel = kernelCache[i];

    for (int k = 0; k < nbrs.size(); k++) {
        int j = nbrs[k];
        double mass_j = (j >= n) ? b_mass : mass;

        auto pressure_j = (j >= n) ? pressure[i] * gamma2 : pressure[j];     // if j is a boundary particle (j>=n), mirror pressure
        auto density_j = (j >= n) ? density[i]: density[j];       // if j is a boundary particle (j>=n), use density of i
        auto force = (density[i] == 0) ? 0 : pressure[i] / (density[i] * density[i]);
        force += (density_j == 0) ? 0 : pressure_j / (density_j * density_j);
        force *= mass_j;
        // auto acceleration = kernelgradient * static_cast<float>(force);
        pa_x[i] -= kernel[k].gx * force;
        pa_y[i] -= kernel[k].gy * force;
    }
}


/// ###########################################
/// ############## [ S E S P H ] ##############
/// ###########################################
void Simulation::UpdateSESPH() {
#pragma omp parallel for
    for (int i = 0; i < n; i++) {
        ComputeDensitiesSESPH(i);
        ComputePressureSESPH(i);
    }
    
    /// Two errors, logged for two different comparisons.
    ///
    /// DIErr is the one that matches what IISPH's eta bounds. Its sourceError sums
    /// max(Ap_i - source_i, 0), and expanding those gives Ap_i - source_i = rho_i^pred - rho_0,
    /// so eta bounds mean(max(rho^pred - rho_0, 0)) / rho_0 - the mean positive relative density
    /// error, clamped. getAvgDensity clamps each particle to max(rho_i, rho_0) before averaging,
    /// so subtracting rho_0 from that mean gives mean(max(rho_i - rho_0, 0)) / rho_0: the same
    /// quantity, over the same fluid particles. IISPH's is predicted for the coming state and
    /// this one is measured from the current one, which is a one-step offset that a mean over a
    /// window absorbs.
    ///
    /// densityDeviation is the unclamped mean |rho - rho_0| / rho_0, matching what UpdateIISPH
    /// records - the physical error rather than the solver's criterion. Both are kept because
    /// the clamped form hides under-density, which is the half that matters at a free surface.
    double avgDensity = getAvgDensity();
    double densityError = std::abs((avgDensity - restDensity) / restDensity);

    double densityDeviation = 0.;
#pragma omp parallel for reduction(+:densityDeviation)
    for (int i = 0; i < n; i++) {
        densityDeviation += std::abs(density[i] - restDensity);
    }

    log.runtime = runtime;
    log.DIIters = 1;            // not iterative: one pressure evaluation per step
    log.DIErr = densityError;
    log.densityDeviation = densityDeviation / (static_cast<double>(n) * restDensity);
    log.stable = stable;
    /// Without this the status stays OK whatever happens and RunLoggedCell's abort never fires -
    /// it reads log.status, which only UpdateIISPH was setting. There is no iteration cap to
    /// report here, so a blow-up is the only failure SESPH has.
    log.status = stable ? OK : Unstable;

    #pragma omp parallel for
    for (int i = 0; i < n; i++) {
        // non pressure
        nonp_a_x[i] = gravity.x;
        nonp_a_y[i] = gravity.y;
        ComputeViscosity(i);
        ComputeSurfaceTension(i);

        // pressure
        ComputePressureAccelerationSESPH(i);
    }

    #pragma omp parallel for
    for (int i = 0; i < n; i++) {
        // non pressure accelerations
        vel_x[i] += nonp_a_x[i] * stepSize;
        vel_y[i] += nonp_a_y[i] * stepSize;

        // pressure accelerations;
        vel_x[i] += pa_x[i] * stepSize;
        vel_y[i] += pa_y[i] * stepSize;

        // update position
        pos_x[i] += vel_x[i] * stepSize;
        pos_y[i] += vel_y[i] * stepSize;
    }
}

}