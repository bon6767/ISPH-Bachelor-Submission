#include <windows.h>

#ifdef _OPENMP
#include <omp.h>
#endif

#include "program/program.h"

using namespace program;

int main() {
    // std::cout << __cplusplus << "\n";
#ifdef _OPENMP
    // std::cout << "OpenMP " << _OPENMP << ", max threads: " << omp_get_max_threads() << "\n";
#else
    std::cout << "OpenMP NOT enabled!\n";
#endif
    // Call once at startup (or every few minutes)
    SetThreadExecutionState(ES_CONTINUOUS | ES_SYSTEM_REQUIRED);
    auto scene = scenes::BreakingDam();
    auto cfg = BreakingDamConfig();

    cfg.sphVariant = DFSPH;
    cfg.dtSESPH = 0.005;
    cfg.stiffness = 5000;
    cfg.fluidDensity = 1.3;
    
    cfg.bSize = 0.5;

    // auto scene = scenes::WaterColumn();
    // auto cfg = WaterColumnConfig();   // exactly what the sweep runs
    // cfg.jacobiVariant = RelaxedJacobi;
    cfg.resStage = 4;
    // cfg.iisphOmega = 0.5;
    cfg.dtIISPH = 0.0004*32;
    // cfg.sourceTerm = VelocityDiv;

    cfg.viscosity = 0.007;
    cfg.surfaceTension = 0.1;
    cfg.gamma1 = 1.1;
    cfg.gamma2 = 0.7;
    // cfg.iisphEta = 0.0000001;
    // cfg.boundaryLayers = 2;

    auto rcfg = StandardRenderConfig();
    rcfg.marchingThreshold = 0.6 * 1.3;
    rcfg.winRenderType = vis::Particles;
    rcfg.vidRenderType = vis::MarchingSquares;
    rcfg.saveToImage = false;             
                                         //    between the SESPH and IISPH runs or they mix
    rcfg.renderText = true;
    rcfg.renderTextOnOutput = false;     // burn t=... into the frames; cfl and dt stay on screen
    rcfg.renderInterval = 1;    // for the rendering window, not the video
    rcfg.outFPS = 24;           // for the video

    AppConfig app{ cfg, scene, rcfg };
    Program program{ app };
    program.logging = false;             // logs/runs/<timestamp>_log_1.csv
    program.printlog = true;
    program.pause = true;
    program.simulationTime = INFINITY;        

    program.RunSimulation();
   }
   