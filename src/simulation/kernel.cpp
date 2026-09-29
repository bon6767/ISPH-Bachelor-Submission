#include "simulation.h"

namespace sim {
// --- Kernel ---
KernelResult Simulation::Kernel(int i, int j) const {
    double dx = pos_x[i] - pos_x[j];
    double dy = pos_y[i] - pos_y[j];
    double dist2 = dx * dx + dy * dy;
    double dist = std::sqrt(dist2);
    double q = dist / h;

    double t1 = std::max(1. - q, 0.);
    double t2 = std::max(2. - q, 0.);

    KernelResult r{};   // W = gx = gy = 0
    r.W = alpha * kernelCorrection * (t2 * t2 * t2 - 4. * t1 * t1 * t1);

    if (dist2 > 0. && q < 2.) {
        double scalar = -3. * t2 * t2 + 12. * t1 * t1;
        double factor = alpha * kernelCorrection * scalar / (dist * h);
        r.gx = dx * factor;
        r.gy = dy * factor;
    }
    return r;
}

KernelResult Simulation::KernelAtPos(double x, double y, int p) const {
    double dx = x - pos_x[p];
    double dy = y - pos_y[p];
    double dist2 = dx * dx + dy * dy;
    double dist = std::sqrt(dist2);
    double q = dist / h;

    double t1 = std::max(1. - q, 0.);
    double t2 = std::max(2. - q, 0.);

    KernelResult r{};   // W = gx = gy = 0
    r.W = alpha * kernelCorrection * (t2 * t2 * t2 - 4. * t1 * t1 * t1);

    if (dist2 > 0. && q < 2.) {
        double scalar = -3. * t2 * t2 + 12. * t1 * t1;
        double factor = alpha * kernelCorrection * scalar / (dist * h);
        r.gx = dx * factor;
        r.gy = dy * factor;
    }
    return r;
}

void Simulation::ComputeKernel() {
    kernelCache.resize(n);
#pragma omp parallel for
    for (int i = 0; i < n; i++) {
        const auto& nbrs = getNeighbours(i);
        kernelCache[i].resize(nbrs.size());
        for (int k = 0; k < nbrs.size(); k++) {
            kernelCache[i][k] = Kernel(i, nbrs[k]);   // slot k, particle nbrs[k]
        }
    }
}


}