#include <stdio.h>
#include <string.h>
#include <math.h>
#include <iostream>
#define NANOSVG_IMPLEMENTATION	// Expands implementation
#include "../../include/nanosvg/nanosvg.h"

#include "parse_svg.h"

constexpr double MM_TO_M = 0.001f;
constexpr double SPLIT_THRESHOLD = 0.5;
namespace svg
{
	sim::SceneParticles parseSVG(std::string path, simcfg::SimulationConfig cfg) {
		/// Load
        if(path.empty()) return sim::SceneParticles(); // no path given

		NSVGimage* image = nsvgParseFromFile(path.c_str(), "mm", 96.0f);
		if (!image) { std::cout << "Failed to load " << path << std::endl; }

        // svg center is at top left corner. so shift everything half image
        Vector2d offset = Vector2d{ (image->width/96.0)*25.4/2., (image->height/96.0)*25.4/2.};
        std::cout << "offset=(" << offset.x << "," << offset.y << ")" << std::endl;

        /// SAMPLE
        std::vector<Vector2d> fluid;
        std::vector<Vector2d> obst;

        // First, sample all lines
        for (NSVGshape* shape = image->shapes;
            shape != nullptr;
            shape = shape->next)
        {
            const bool hasFill =
                shape->fill.type != NSVG_PAINT_NONE;

            const bool hasStroke =
                shape->stroke.type != NSVG_PAINT_NONE;

            if (hasStroke)
            {   // is obstacle
                sampleStroke(shape, cfg, obst, offset);
            }
        }

        // Then, sample all obstacles
        for (NSVGshape* shape = image->shapes;
            shape != nullptr;
            shape = shape->next)
        {
            const bool hasFill =
                shape->fill.type != NSVG_PAINT_NONE;

            const bool hasStroke =
                shape->stroke.type != NSVG_PAINT_NONE;

            if (hasFill)
            {   // is fluid
                sampleArea(shape, cfg, fluid, obst);
            }
        }
        nsvgDelete(image);

        /// Create State to pass to sim.
        sim::SceneParticles state;
        for (auto f : fluid) {
            state.f.push_back(f);
        }

        for (auto b : obst) {
            state.b.push_back(b);
        }

        return state;
	}

    void sampleStroke(NSVGshape* shape, simcfg::SimulationConfig cfg, std::vector<Vector2d>& particles, Vector2d offset)
    { 
        /// Flatten bezier to polylines -> sample along polylines
        const double eps = SPLIT_THRESHOLD * cfg.h() * cfg.bSize;   // sampleStroke
        std::vector<Vector2d> polyLine = flattenPath(shape->paths, eps * eps, offset);

        if (polyLine.size() < 2) { if (!polyLine.empty()) particles.push_back(polyLine[0]); return; }

        double stepSize = cfg.h() * cfg.bSize;
        particles.push_back(polyLine[0]);   // place very first point of stroke
        for (int i = 0; i < polyLine.size()-1; i++) { // Iterating through lines (could be a long straight wall)
            // for each new segment first place the next point so that it lies on the segment, but is exactly stepSize away
            Vector2d p = polyLine[i]; Vector2d q = polyLine[i+1];
            
            Vector2d m = p - particles.back();
            Vector2d dir = normalize(q-p);
            double segLength = magnitude(q - p);
            if (segLength == 0.0)
                continue;
            
            double b = dotProduct(m, dir);
            double c = dotProduct(m, m) - stepSize * stepSize;
            double disc = b * b - c;

            if (disc < 0.0) continue;
            double moveDistance = -b + std::sqrt(disc);
            if (moveDistance < 0.0 || moveDistance > segLength)
                continue;
            p = p + dir * moveDistance;
            particles.push_back(p);


            // now from that position on, go along the line.
            // dir = normalize(q - p);
            double remainingLength = segLength-moveDistance;

            Vector2d step = dir * stepSize;
            
            int numSteps = static_cast<int>(remainingLength / stepSize);
            for (int k = 1; k <= numSteps; k++) { // stepping through line
                Vector2d new_p = p + static_cast<double>(k) * step;
                // Vector2d pos = particles[k - 1] + step;
                particles.push_back(new_p);
            }
        }

    }
    void sampleArea(NSVGshape* shape, simcfg::SimulationConfig cfg, std::vector<Vector2d>& particles, std::vector<Vector2d> obstacles)
    { 
        
    }

    std::vector<Vector2d> flattenPath(NSVGpath* path, double threshold, Vector2d offset) {
        /// Turns the path that consists of bezier curve segments into polylines
        std::vector<Vector2d> polyLine{};
        polyLine.push_back(svgToWorld({ path->pts[0], path->pts[1] }, offset));  // Add A
        for (int i = 0; i < path->npts - 1; i += 3) {
            const float* p = &path->pts[i * 2];
            Vector2d A = svgToWorld({ p[0], p[1] }, offset);
            Vector2d B = svgToWorld({ p[2], p[3] }, offset);
            Vector2d C = svgToWorld({ p[4], p[5] }, offset);
            Vector2d D = svgToWorld({ p[6], p[7] }, offset);

            std::vector<Vector2d> seg;
            flattenSegment(seg, A, B, C, D, threshold);
            polyLine.insert(polyLine.end(), seg.begin(), seg.end());
        }
        return polyLine;
    }

    void flattenSegment(std::vector<Vector2d>& out, Vector2d A, Vector2d B, Vector2d C, Vector2d D, double threshold, int level) {
        /// Recursively turns a bezier segment into polylines.
        if (level > 10) { out.push_back(D); return; }

        // Test condition whether line can be returned
        // distances from control points to chord
        double dB = std::abs(crossProduct(B - D, D - A));
        double dC = std::abs(crossProduct(C - D, D - A));
        double d = dB + dC;

        Vector2d chord = D - A;
        double chordLen2 = chord.x * chord.x + chord.y * chord.y;
        // case that can't converge
        if (chordLen2 == 0.0 && d == 0.0) { out.push_back(D); return; }
        
        bool straight_enough = d * d < threshold * chordLen2;
        if (straight_enough) {
            out.push_back(D);
            return;
        } // else
        // split bezier curve:
        Vector2d E{ (A + B) * 0.5 };    Vector2d F{ (B + C) * 0.5 };
        Vector2d G{ (C + D) * 0.5 };    Vector2d H{ (E + F) * 0.5 };
        Vector2d J{ (F + G) * 0.5 };    Vector2d K{ (H + J) * 0.5 };

        flattenSegment(out, A, E, H, K, threshold, level + 1);
        flattenSegment(out, K, J, G, D, threshold, level + 1);
    }

    Vector2d svgToWorld(Vector2d pos, Vector2d offset) {
        return Vector2d{
            (pos.x - offset.x) * MM_TO_M,
            (pos.y - offset.y) * MM_TO_M 
        };
    }

}