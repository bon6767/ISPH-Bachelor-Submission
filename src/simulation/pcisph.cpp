#include "simulation.h"
#include <iostream>


namespace sim {
// Densities:
// Compute Densities function used in the final version (PCISPH)
void Simulation::ComputeDensitiesPCISPH(int i) {
    const auto& nbrs = getNeighbours(i);
    const auto& kernel = kernelCache[i];

    density[i] = 0.f;
    double vx_i = vel_x[i], vy_i = vel_y[i]; // velocity of particle i

    for (int k = 0; k < nbrs.size(); k++) {
        int j = nbrs[k];
        double mass_j = (j >= n) ? b_mass : mass;

        density[i] += kernel[k].W * mass_j; // current density

        // density change
        double vx_j = (j >= n) ? 0 : vel_x[j];   // velocity of particle j
        double vy_j = (j >= n) ? 0 : vel_y[j];   // velocity of particle j
        // float mass_j = (j >= n) ? mass : mass;  // this line will make sense / has to be..
        // .. adjusted, when boundary particles have a different mass.

        double vx_ij = vx_i - vx_j, vy_ij = vy_i - vy_j; // velocity difference
        double densitychange = vx_ij * kernel[k].gx + vy_ij * kernel[k].gy;
        density[i] += mass_j * densitychange * stepSize;  // density change
    }
}

// pressure computation with the new "dynamic" stiffness
void Simulation::ComputePressurePCISPH(int i) {
    // pressure[i] = K * (density[i] - restDensity);
    pressure[i] = std::max(K * (density[i] - restDensity), 0.);
    // LMAO
    // pressure[i] = std::max(pressure[i] + K * (density[i] - restDensity), 0.f);
}

// ------------------- Update PCISPH --------------
void Simulation::UpdatePCISPH() {
    // non pressure accelerations, gravity and viscosity
#pragma omp parallel for
    for (int i = 0; i < n; i++) {
        // calculate non pressure accelerations
        nonp_a_x[i] = gravity.x;
        nonp_a_y[i] = gravity.y;

        AccelerationAreas(i);

        ComputeViscosity(i);
        nonp_a_x[i] += visc_a_x[i];
        nonp_a_y[i] += visc_a_y[i];

        ComputeSurfaceTension(i);
    }

#pragma omp parallel for
    for (int i = 0; i < n; i++) {
        // apply nonpressure accelerations
        vel_x[i] += nonp_a_x[i] * stepSize;
        vel_y[i] += nonp_a_y[i] * stepSize;
    }


    // calc density error
    float densityError = 1;
    int iter = 0;
    while (densityError > 0.002479f && iter < 100) { //repeat until 0.1% pressure error
        iter++;
#pragma omp parallel for
        for (int i = 0; i < n; i++) {
            ComputeDensitiesPCISPH(i);
            ComputePressurePCISPH(i);
        }

#pragma omp parallel for
        for (int i = 0; i < n; i++) {
            ComputePressureAccelerationSESPH(i);
            vel_x[i] += pa_x[i] * stepSize;
            vel_y[i] += pa_y[i] * stepSize;
        }

        float avgDensity = getAvgDensity();
        densityError = std::abs((avgDensity - restDensity) / restDensity);
    }

    std::cout << "density error after " << iter << " iteration = " << densityError * 100.f << "%" << std::endl;

#pragma omp parallel for
    for (int i = 0; i < n; i++) {
        // update position
        pos_x[i] += vel_x[i] * stepSize;
        pos_y[i] += vel_y[i] * stepSize;
    }
}
}