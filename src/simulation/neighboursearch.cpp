#include "simulation.h"
#include <iostream>

namespace sim {
// --- Neighbour Computation ---
// ---     Index Sort        ---
void Simulation::ComputeNeighbours() {
    const auto neighbourStart = Clock::now();

    // -- grid attributes --
    float x_min = -gridDimension / 2.f;  // left border of grid in world coordinates
    float y_min = -gridDimension / 2.f;  // top border of grid (in sfml negative y is up)
    float cellSize = 2 * h;            // edge size of a single cell in the grid
    int K = std::ceil(gridDimension / cellSize);    // amount of cells in x-(and y)-direction to achieve gridDimension)

    // prepare vectors
    cells.assign(K * K + 1, 0); // fills cells with K*K+1 many 0s (+1 because index sort needs it)
    indices.resize(n + bn);
    particleToCell.resize(n + bn);
    int offset = sqrt(cells.size() - 1);  // offset to get to previous row

    // loop over all particles
    #pragma omp parallel for
    for (size_t i = 0; i < n + bn; i++)
    {
        // compute cell index of particle i
        int k = floor((pos_x[i] - x_min) / cellSize);
        int l = floor((pos_y[i] - y_min) / cellSize);
        int c = k + l * K;  // cell index of particle i

        // check if in grid
        if (c < 0 || c >= cells.size() - 1)
        {
            if (stable) {
                std::cerr << "ERROR: particle outside grid" << std::endl;
                stable = false;
            }
            continue;
        }
        particleToCell[i] = c;  // store which cell particle i belongs to
        #pragma omp atomic
        cells[c] += 1;          // increment counter for cell particle i is in
    }
    if (!stable) return;

    // accumulate counters in cells
    // (maybe don't parallelise because it references to itself)
    // #pragma omp parallel for
    for (int i = 1; i < cells.size(); i++) {
        cells[i] += cells[i - 1];
    }

    // associate particle i with cell j and decrement counter in cells
    // don't parallelise because of --cells[c] being called on same c in different threads ..
    /// TODO: mb look into how this can be parallelised anyways
    // #pragma omp parallel for
    for (size_t i = 0; i < n + bn; i++)
    {
        int c = particleToCell[i];
        // using pre-decrement: index will get cells[c]-1 and cells[c] will be decremented
        int index = --cells[c]; // index of 
        indices[index] = i;     // <- puts pokarticle index at position in indices in order of cell
    }

    /// building cache / nbr list
    // clear neigbour map
    neighboursmap.resize(n);
    for (auto& vec : neighboursmap) {
        vec.clear();
        vec.reserve(32);
    }
    #pragma omp parallel for
    for (int p = 0; p < n; p++) {
        auto& nbrs = neighboursmap[p];
        nbrs.clear();
        int c = particleToCell[p];

        // we want to check the cells around the particles own cell
        // cells.size() is a square + 1, so this will always be an int.
        for (int i = -1; i <= 1; i++) {
            for (int j = -1; j <= 1; j++) {
                int cellIndex = c + i * offset + j;
                // check if index is in bounds:
                if (cellIndex < 0 || cellIndex >= cells.size() - 1) continue;

                int start = cells[cellIndex];
                int end = cells[cellIndex + 1];

                for (int k = start; k < end; k++) {
                    int other = indices[k];
                    if (isNeighbour(p, other)) nbrs.emplace_back(other);
                }
            }
        }
    }
    const double nbrTime =
        std::chrono::duration<double, std::milli>(
            Clock::now() - neighbourStart
        ).count();
    log.nbrTime = nbrTime;
    return;
}

const std::vector<int>& Simulation::getNeighbours(int particleId) const {
    return neighboursmap[particleId];
}

const std::vector<int> Simulation::getNeighboursAtPosition(Vector2d pos) const{
    // -- grid attributes --
    float x_min = -gridDimension / 2.f;  // left border of grid in world coordinates
    float y_min = -gridDimension / 2.f;  // top border of grid (in sfml negative y is up)
    float cellSize = 2 * h;            // edge size of a single cell in the grid
    int K = std::ceil(gridDimension / cellSize);    // amount of cells in x-(and y)-direction to achieve gridDimension)

    int offset = sqrt(cells.size() - 1);  // offset to get to previous row


    // compute cell index of particle i
    int k = floor((pos.x - x_min) / cellSize);
    int l = floor((pos.y - y_min) / cellSize);
    int c = k + l * K;  // cell index of particle i
    if (c <= 0 || c >= cells.size() - 1)
    {
        std::cerr << "ERROR: position outside grid" << std::endl;
        return {};
    }
    std::vector<int> nbrs;

    // we want to check the cells around the particles own cell
    // cells.size() is a square + 1, so this will always be an int.
    for (int i = -1; i <= 1; i++) {
        for (int j = -1; j <= 1; j++) {
            int cellIndex = c + i * offset + j;
            // check if index is in bounds:
            if (cellIndex <= 0 || cellIndex >= cells.size() - 1) {
                std::cerr << "ERROR: position outside grid" << std::endl;
                continue;
            }

            int start = cells[cellIndex];
            int end = cells[cellIndex + 1];

            for (int k = start; k < end; k++) {
                int other = indices[k];
                if (isNeighbour(pos.x, pos.y, other)) nbrs.emplace_back(other);
            }
        }
    }
    return nbrs;
}

bool Simulation::isNeighbour(int p1, int p2) {
    const double r = 2.f * h;
    const double r2 = r * r;
    // check if in they are in distance:
    // satz des kerouac
    double a = pos_x[p2] - pos_x[p1];
    double b = pos_y[p2] - pos_y[p1];
    double c = a * a + b * b;

    // -- distance check --
    return c < r2;
}

bool Simulation::isNeighbour(double x, double y, int p) const{
    const double r = 2.f * h;
    const double r2 = r * r;
    // check if in they are in distance:
    // satz des kerouac
    double a = x - pos_x[p];
    double b = y - pos_y[p];
    double c = a * a + b * b;

    // -- distance check --
    return c < r2;
}

}