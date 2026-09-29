/* This file contains alternative algorithms and additional 
* algorithms necessary for the simulation.cpp
* the ones in use by default will be in .cpp
*/

#include "simulation.h"
#include <random>
#include <iostream>
#include "../scenes/scenes.h"
#include "../parse_svg/parse_svg.h"
using namespace scenes;

namespace sim {
static inline float pi() { return 3.141593; }

// --- Init ---
void Simulation::SetUpScene(SimulationConfig cfg) {
    // clear vectors
    pos_x.clear(); pos_y.clear();
    vel_x.clear(); vel_y.clear();
    density.clear();
    volumeSize = 0;
    n = 0;

    SceneParticles svg_particles = svg::parseSVG(scene.svg_path, cfg);

    for (auto f : svg_particles.f) {
        CreateParticle(f.x, f.y, 0., 0.);
    }

    // generate fluid bodies
    for (auto& volume : scene.fluidVolumes) {
        fillVolumeWithParticles(volume);
        // calculate total volume size
        volumeSize += volume.size.x * volume.size.y;
    }
    n = pos_x.size();
    // std::cout << "generated " << n << "particles." << std::endl;

    for (auto b : svg_particles.b) {
        pos_x.emplace_back(b.x);
        pos_y.emplace_back(b.y);
    }

    GenerateBoundary();
    GenerateSceneObjects();

    bn = pos_x.size() - n;
    std::cout << "N=" << pos_x.size() << "\tn="<< n << "\tbn" << bn << std::endl;
}

void Simulation::fillVolumeWithParticles(FluidVolume volume)
{
    Vector2d volumeOffset = volume.offset;
    Vector2d size = volume.size;
        
    int nx = static_cast<int>(std::floor(size.x / h));  // number of particles in x dir
    int ny = static_cast<int>(std::floor(size.y / h));  // number of particles in y dir

    double middleX = (nx - 1) * 0.5f;
    double middleY = (ny - 1) * 0.5f;

    std::mt19937 rng(12345);
    std::uniform_real_distribution<double> dist(-1.0f, 1.0f);

    for (int j = 0; j < ny; ++j) {
        double y = (j - middleY) * h + volumeOffset.y;
        for (int i = 0; i < nx; ++i) {
            double x = (i - middleX) * h + volumeOffset.x;
            
            double jx = dist(rng); // random float in [-1, 1]
            double jy = dist(rng);

            // x += jx * 0.005f * h;
            // y += jy * 0.005f * h;
            CreateParticle(x, y, 0., 0.);
        }
    }
}

void Simulation::GenerateBoundary() {
    auto boundary = scene.boundary;
    if (boundary == Vector2d{ 0., 0. }) return;
    auto offset = scene.boundaryOffset;
    double bh = h * bSize;

    // One rectangle of particles per layer, each one bh further out. boundaryLayers is config,
    // so the count lands in the log header - the July 2026 version of this function had the
    // second layer hard-coded and then hard-deleted, which left months of logs ambiguous about
    // how many layers produced them.
    std::vector<Vector2d> bParticles;
    for (int layer = 0; layer < boundaryLayers; layer++) {
        double x = (boundary.x + h) / 2. + layer * bh;
        double y = (boundary.y + h) / 2. + layer * bh;

        // top, right, bottom, left
        for (auto& side : {
                utils::line(Vector2d{ -x, -y } + offset, Vector2d{  x, -y } + offset, bh),
                utils::line(Vector2d{  x, -y } + offset, Vector2d{  x,  y } + offset, bh),
                utils::line(Vector2d{  x,  y } + offset, Vector2d{ -x,  y } + offset, bh),
                utils::line(Vector2d{ -x,  y } + offset, Vector2d{ -x, -y } + offset, bh) }) {
            bParticles.insert(bParticles.end(), side.begin(), side.end());
        }
    }

    for (auto& b : bParticles) {
        pos_x.emplace_back(b.x);
        pos_y.emplace_back(b.y);
    }

    // bn = bParticles.size();
    bParticles.clear(); // no longer needed
}

void Simulation::GenerateSceneObjects() {
    for (auto& o : scene.barriers) {
        auto line = utils::line(o.start, o.end, h*bSize);
        for (auto& p : line) {
            pos_x.emplace_back(p.x);
            pos_y.emplace_back(p.y);
        }
        // bn += line.size();
    }
}

// --- adding and deleting particles ---
void Simulation::DeleteParticle(size_t i) {
    if (i >= n) return; // don't delete boundary particles.

    const size_t last = n - 1; // last fluid particle before boundary
    forEachParticleArray([&](std::vector<double>& v) {
        v[i]    = v[last];
        v[last] = v.back();
        v.pop_back();
    });
    // now particle i doesnt exist anymore, 'last' is in its position,
    // and 'back' (last boundary) is in 'last' original positoin.
    n--;
}

void Simulation::DeleteParticles() {
    if (scene.outlets.size() == 0) return;
    for (size_t i = 0; i < n;) {
        bool deleted = false;
        for (Outlet& o : scene.outlets) {
            if (runtime < o.startTime || runtime > o.endTime) continue;
            if (utils::isInBox(pos_x[i], pos_y[i], o.area)) {
                DeleteParticle(i);
                deleted = true;
                break; // slot i holds a different particle now, retest outlets from scratch
            }
        }
        if (!deleted) i++;
    }
}

void Simulation::CreateParticle(double x, double y, double vx, double vy) {
    forEachParticleArray([&](std::vector<double>& v) {
        v.emplace_back(n < v.size() ? v[n] : 0.);  // first boundary -> back
        v[n] = 0.;                                 // new fluid slot starts clean
        });
    pos_x[n] = x;   pos_y[n] = y;
    vel_x[n] = vx;  vel_y[n] = vy;
    density[n] = restDensity;
    n++;
}

void Simulation::InletEmitParticles() {
    for (Inlet& in : scene.inlets) {
        if (runtime < in.startTime) continue;
        if (runtime > in.endTime) continue;

        float spawnInterval = h / in.speed;

        if (in.timeSinceEmission == 0.f) {

            // compute normal for emission direction
            double rad = in.rotation * pi() / 180.;
            Vector2d linedir{
                cos(rad),
                sin(rad)
            };
            Vector2d normal{ -linedir.y, linedir.x };
            Vector2d vel = in.speed * normal;

            auto emitted = utils::line(in.position, in.width, in.rotation, h);
            for (Vector2d p : emitted) {
                CreateParticle(p.x, p.y, vel.x, vel.y);
            }
        }

        in.timeSinceEmission += stepSize;
        if (in.timeSinceEmission >= spawnInterval) {
            in.timeSinceEmission = 0.f;
        }
    }
}

void Simulation::AccelerationAreas(int i) {
    for (AccelerationArea a : scene.accelerationAreas) {
        if (runtime < a.startTime || runtime > a.endTime) continue;
        double x = pos_x[i];
        double y = pos_y[i];
        if (!utils::isInBox(x, y, a.area)) continue;

        nonp_a_x[i] += a.a_x;
        nonp_a_y[i] += a.a_y;
    }
}

void testKernelIntegral(sim::Simulation* sim) {
    int rowSize = sqrt(sim->get_n());  // offset to get to next row
    int mid = rowSize / 2;
    int id = mid * rowSize + mid;
    id = (id > sim->get_n()) ? 0 : id;

    auto nbrs = sim->getNeighbours(id);
    float integral = 0.f;
    for (int n : nbrs) {
        integral += sim->GetKernel(id, n).W * sim->getVolume() / sim->get_n();
    }

    std::cout << "integral at p " << id << ": " << integral << std::endl;
}

void testKernelGradient(sim::Simulation* sim) {
    // choosing a random particle not at an edge.
    int rowSize = sqrt(sim->get_n());  // offset to get to next row
    int mid = rowSize / 2;
    int id = (mid)*rowSize + mid;
    id = id > sim->get_n() ? 0 : id;

    std::cout << "particle " << id << " (" << sim->getPos_x()[id] << ", " << sim->getPos_y()[id] << ")" << std::endl;

    auto nbrs = sim->getNeighbours(id);
    double sx = 0.0, sy = 0.0, c = 0.0;
    Vector2d integral{ 0., 0. };
    for (int n : nbrs) {
        auto gx = sim->GetKernel(id, n).gx;
        auto gy = sim->GetKernel(id, n).gy;
        double y = gx - c;
        double t = sx + y;
        c = (t - sx) - y;
        sx = t;
        // sx += g.x;
        sy += gy;
        std::cout << "n: " << n << " ("
            << sim->getPos_x()[n] << ", " << sim->getPos_y()[n]
            << ")\t kernel deriative : (" << gx << ", " << gy << ")" << std::endl;
        // integral += g;
    }

    std::cout << "integral of dKernel at p " << id <<
        ": (" << sx << "," << sy << ")" << std::endl;
}

void testKernelConsistency(sim::Simulation* sim) {
    // choosing a random particle not at an edge.
    int rowSize = sqrt(sim->get_n());  // offset to get to next row
    int mid = rowSize / 2;
    int i = (mid)*rowSize + mid;
    i = (i > sim->get_n()) ? 0 : i;

    std::cout << "checking at particle :" << i << std::endl;

    // --- calculate condition ---
    double particleVolume = (sim->getVolume() / sim->get_n());
    double m_xx = 0.0, m_xy = 0.0, m_yx = 0.0, m_yy = 0.0;

    double xi = sim->getPos_x()[i];
    double yi = sim->getPos_y()[i];
    auto nbrs = sim->getNeighbours(i);
    for (int j : nbrs) {
        double xj = sim->getPos_x()[j];
        double yj = sim->getPos_y()[j];

        auto d = Vector2d{ xi - xj, yi - yj };   // difference in position
        auto kgx = sim->GetKernel(i, j).gx;    // derivative of kernel
        auto kgy = sim->GetKernel(i, j).gy;    // derivative of kernel

        m_xx += d.x * kgx * particleVolume;
        m_xy += d.x * kgy * particleVolume;
        m_yx += d.y * kgx * particleVolume;
        m_yy += d.y * kgy * particleVolume;
    }

    // --- compare to expected value ---
    std::cout << "[" << m_xx << ", " << m_xy << ",\n"
        << m_yx << ", " << m_yy << "]" << " (" << nbrs.size() << "neighbours)" << std::endl;
}

const FieldSample Simulation::sampleField(Vector2d pos) const {
    FieldSample res{};
    auto nbrs = getNeighboursAtPosition(pos);

    for (int k = 0; k < nbrs.size(); k++) {
        int j = nbrs[k];
        if (j >= n) continue;
        auto kernel = KernelAtPos(pos.x, pos.y, j);
        double mass_j = (j >= n) ? gamma1 * b_mass : mass;
        res.density += kernel.W * mass_j; // current density
        res.vel_x += vel_x[j] * mass_j / density[j] * kernel.W;
        res.vel_y += vel_y[j] * mass_j / density[j] * kernel.W;
    }
    return res;
}

/// Mean |D rho_i / Dt| / rho_0 of the current velocity field, in 1/s.
///
/// What the velocity-divergence source term drives to zero, measured whichever source term is
/// actually in use - so each formulation can be judged on the other's criterion and not only on
/// its own. The density-invariance counterpart is StepLog::densityDeviation.
///
/// Deliberately not part of the step: it is a whole extra pass over the neighbours, and folding
/// it into UpdateIISPH would put it in every timing measurement for the sake of one analysis.
/// Call it from the analysis loop after Update() instead, where the cost does not matter.
///
/// Called there, it reads the velocities the step produced against the kernel cache built from
/// the positions at the start of that step - which is the pairing the solver itself worked with,
/// so it measures the divergence the solve was actually trying to remove.
double Simulation::GetVelocityDivergence() const {
    double total = 0.;

#pragma omp parallel for reduction(+:total)
    for (int i = 0; i < n; i++) {
        const auto& nbrs = getNeighbours(i);
        const auto& kernel = kernelCache[i];

        // D rho_i / Dt = sum_j m_j (v_i - v_j) . grad W_ij, the same continuity expression
        // ComputeSourceTermIISPH builds its source from, boundary neighbours contributing v = 0
        double divergence = 0.;
        for (int k = 0; k < nbrs.size(); k++) {
            int j = nbrs[k];
            double mass_j = (j >= n) ? b_mass : mass;

            double vx_ij = vel_x[i] - ((j >= n) ? 0. : vel_x[j]);
            double vy_ij = vel_y[i] - ((j >= n) ? 0. : vel_y[j]);

            divergence += mass_j * (vx_ij * kernel[k].gx + vy_ij * kernel[k].gy);
        }
        // absolute per particle: compression here and expansion there must not cancel, which is
        // the mistake the velocity-divergence stopping criterion used to make
        total += std::abs(divergence);
    }

    return total / (static_cast<double>(n) * restDensity);
}
}