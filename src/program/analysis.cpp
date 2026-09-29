#include "program.h"
#include <cmath>
#include <iostream>
#include <sstream>
#include <tuple>

namespace program {

void Program::RunAnalysis(SimulationConfig simCfg, Scene scene, double bufferTime,
    double runTime, double cflThreshold, bool earlyStopping,
    std::string logPathPrefix)
{
    Simulation sim{ simCfg, scene };
    while (sim.runtime <= bufferTime) { sim.Update(); } sim.runtime = 0; sim.ResetRuntime();
    while (sim.runtime <= runTime) {
        sim.Update();
        LogData(sim.log);
        // early stopping
        if (sim.getCFLValue() > 0.3) { sim.stable = false; }
        if (earlyStopping && !sim.stable) { std::cout << "early stopping" << std::endl; break; }
    }
    StoreLog(logPathPrefix, simCfg, sim.get_bn(), sim.get_n());
}

void Program::AnalyseMomentum()
{
    /// Beta - Omega relation
    /// On low resolution: 16k particles
    scene = scenes::WaterColumn();
    cfg = simcfg::WaterColumnConfig();
    cfg.particlesPerSquaremeter = 16384;
    cfg.dtIISPH = 0.001;
    cfg.iisphMaxIterations = 300;
    
    if (false) {
        double betas[] = { 0.0, 0.8, 0.9, 0.925, 0.95, 0.975, 1., 1.025, 1.05 };
        double omegas[] = { 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 0.9, 1.1, 1.2 };
        for (JacobiVariant variant : { HeavyBall, Nesterov }) {
            cfg.jacobiVariant = variant;
            for (double beta : betas) {
                for (double omega : omegas) {
                    cfg.iisphBeta = beta;
                    cfg.iisphOmega = omega;
                    // printlog = true;
                    // RunSimulation();
                    std::ostringstream prefix;
                    prefix << "logs/nesterov/beta_omega/"
                        << JacobiVariantStr[variant]
                        << "_b_" << beta
                        << "_o_" << omega << "_";
                    std::cout << JacobiVariantStr[variant] << "\tbeta=" << beta << "\tomega=" << omega << std::endl;
                    RunAnalysis(cfg, scene,
                        0.5,    // bufferTime<
                        1.,     // runTime
                        0.3,    // cflThreshold
                        true,   // earlyStopping
                        prefix.str()
                    );
                }
            }
        }
    }

    /// RESOLUTIONS
    {
    std::tuple<int, double> resolutions[] = {
        std::make_tuple(4096, 0.002),
        std::make_tuple(64009, 0.0005),
        std::make_tuple(256036, 0.00025) };
    double betas[] = { 0.8, 0.9, 0.95, 1.};
    double omegas[] = {0.5, 0.7, 0.9, 1.1 };
    for (auto resolution : resolutions) {
        cfg.particlesPerSquaremeter = std::get<0>(resolution);
        cfg.dtIISPH = std::get<1>(resolution);
        
        for (JacobiVariant variant : { HeavyBall, Nesterov }) {
            cfg.jacobiVariant = variant;
            for (double beta : betas) {
                for (double omega : omegas) {
                    cfg.iisphBeta = beta;
                    cfg.iisphOmega = omega;
                    // printlog = true;
                    // RunSimulation();
                    std::ostringstream prefix;
                    prefix << "logs/nesterov/beta_omega/res/"
                        <<  "n_" << std::get<0>(resolution)
                        <<   "_" << JacobiVariantStr[variant]
                        << "_b_" << beta
                        << "_o_" << omega << "_";
                    std::cout << JacobiVariantStr[variant] << "\tn=" << std::get<0>(resolution)
                              << "\tbeta = " << beta << "\tomega = " << omega << std::endl;
                    RunAnalysis(cfg, scene,
                        0.5,    // bufferTime<
                        1.,     // runTime
                        0.3,    // cflThreshold
                        true,   // earlyStopping
                        prefix.str()
                    );
                }
            }
        }
    }
    }
}


void Program::AnalysePerformance() {
    cfg = WaterColumnConfig();
    scene = WaterColumn();
    sim::Simulation sim{ cfg, scene };
    while (sim.runtime <= simulationTime) {
        sim.Update();
        LogData(sim.log);

        // early stopping
        if (!sim.stable) break;
    }
    StoreLog("logs/performance/", cfg, sim.get_bn(), sim.get_n());
}

void Program::TimeStepAnalysis() {
    scene = WaterColumn();
    cfg = simcfg::WaterColumnConfig();
    simulationTime = 5.;
    cfg.sphVariant = sim::IISPH;

    double dt_min = 0.0001;
    double ddt = 0.0001;
    double dt_max = 0.0008;

    double dt = dt_min;

    std::cout << "dt analysis ranging from " << dt_min << "s to " << dt_max << "s." << std::endl;
    // analysis loop
    while (dt < dt_max) {
        cfg.dtIISPH = dt;
        Simulation sim{ cfg, scene };

        std::cout << "now analysing dt " << dt << std::endl;

        while (sim.runtime <= simulationTime) {
            sim.Update();
            LogData(sim.log);

            // early stopping
            if (!sim.stable) return;
        }
        StoreLog("logs/timeStep/", cfg, sim.get_bn(), sim.get_n());
        dt += ddt;
    }
}

void Program::GammaAnalysis() {
    std::cout << "GAMMA ANALYSIS" << std::endl;
    scene = scenes::WaterColumn();
    cfg = simcfg::GammaAnalysisConfig();

    cfg.settleTime = 0.5;                                   // impact transient: logged, but not judged
    simulationTime = 3.;                                    // measurement window, as in the original sweep
    const double totalTime = cfg.settleTime + simulationTime;

    double gammas1[] = { 0.8, 0.4, 0.3, 0.5, 1.4, 0.7, 1.1, 1.2, 0.6, 0.9, 1.3, 1.0 };
    double gammas2[] = { 1.3, 0.5, 1.0, 0.9, 0.3, 0.8, 0.4, 0.7, 1.1, 1.4, 0.6, 1.2 };
    int n1 = (sizeof(gammas1) / sizeof(gammas1[0]));
    int n2 = (sizeof(gammas2) / sizeof(gammas2[0]));

    for (int i = 0; i < n1; i++) {
        for (int j = 0; j < n2; j++) {
            double g1 = gammas1[i];
            double g2 = gammas2[j];
            cfg.gamma1 = g1;
            cfg.gamma2 = g2;
            Simulation sim{ cfg, scene };
            std::cout << "gamma 1: " << sim.getGamma1()
                << "\tgamma 2: " << sim.getGamma2()
                << "\t(test: " << i * n2 + j << " out of " << n1*n2 << ")"
                << std::endl;
            while (sim.runtime < totalTime)
            {
                sim.Update();
                const bool measuring = sim.runtime >= cfg.settleTime;

                // set before logging: LogData stores a copy, so the aborting
                // step is the last row and it carries the reason
                if (measuring && sim.getCFLValue() > cfg.cflThreshold) {
                    sim.log.status = Unstable;
                }
                LogData(sim.log);

                // blow-up (particle outside grid, or cfl above threshold) aborts in any phase
                if (sim.log.status == Unstable) {
                    std::cout << "\tunstable at " << sim.runtime << "s"
                        << " (cfl " << sim.getCFLValue() << ")" << std::endl;
                    break;
                }
                // cap == fail, but only once the transient has passed
                if (measuring && sim.log.status == IterCap) {
                    std::cout << "\titeration cap at " << sim.runtime << "s"
                        << " (" << sim.log.DIIters << " iters)" << std::endl;
                    break;
                }
            }
            std::cout << "finished after: "
                << sim.log.realtime << "s"
                << std::endl;

            std::ostringstream prefix;
            prefix << "logs/gamma/"
                << "g1_" << sim.getGamma1()
                << "_g2_" << sim.getGamma2() << "_";

            StoreLog(prefix.str(), cfg, sim.get_bn(), sim.get_n());
        }
    }
    std::cout << "GAMMA ANALYSIS Complete\n" << std::endl;
}

void Program::ConstIterVarTimeStep() {
    simulationTime = 5.;
    scene = WaterColumn();
    cfg = WaterColumnConfig();
    cfg.useFixedIterations = true;
    cfg.fixedIterations = 10;

    double timesteps[] = { 0.0005, 0.0006, 0.0007, 0.0008, 0.0009, 0.0010, 0.0011,0.0012, 0.0013, 0.0014, 0.0015 };
    for (double dt : timesteps) {
        cfg.dtIISPH = dt;
        Simulation sim{ cfg, scene };
        std::cout << "dt: " << dt << std::endl;
        while (sim.runtime < simulationTime) {
            sim.Update();
            if (!sim.stable) {
                std::cout << "early stopping at " << sim.runtime << "s" << std::endl;
                break;
            }
            LogData(sim.log);
        }
        std::ostringstream prefix;
        prefix << "logs/const_iter_v_dt/iter_" <<sim.getFixedIterationsCount()
            << "_t_" << sim.getTimestep() << "_";
        StoreLog(prefix.str(), cfg, sim.get_bn(), sim.get_n());
    }
}

}