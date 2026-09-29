#include "program.h"
#include <iostream>
#include <sstream>
#include <tuple>

namespace program {
/*
std::tuple<int, double> resolutions[] = {
    std::make_tuple(4096, 0.002),
    std::make_tuple(16384, 0.001),
    std::make_tuple(64009, 0.0005),
    // std::make_tuple(256036, 0.00025)
};

double cfl_threshold = 0.5f;
double runtime = 1.;
double buffertime = 0.5;

void Program::AnalyseJacobi() {
    cfg = scenes::WaterColumn();
    cfg.simConfig.jacobiVariant = RelaxedJacobi;
    cfg.simConfig.iisphMaxIterations = 300;
    
    /// Normal Jacobi runs for Reference
    double omega = 0.5;
    cfg.simConfig.iisphOmega= omega;
    for (auto res : resolutions) {
        cfg.simConfig.particlesPerSquaremeter = std::get<0>(res);
        cfg.simConfig.dtIISPH = std::get<1>(res);

        std::ostringstream prefix;
        prefix << "logs/solver/jacobi/"
            << "n_" << std::get<0>(res)
            << "_" << JacobiVariantStr[RelaxedJacobi]
            << "_";
        std::cout << JacobiVariantStr[RelaxedJacobi] << "\tn=" << std::get<0>(res)
            << std::endl;
        RunAnalysis(cfg.simConfig,
            buffertime,    // bufferTime<
            runtime,     // runTime
            cfl_threshold,    // cflThreshold
            true,   // earlyStopping
            prefix.str()
        );
    }
}

void Program::AnalyseHeavyBall() {
    cfg = scenes::WaterColumn();
    cfg.simConfig.jacobiVariant = HeavyBall;
    cfg.simConfig.iisphMaxIterations = 300;

    double betas[] = { 0.6, 0.7, 0.8, 0.9, 0.95 };
    double omegas[] = { 0.7, 0.9, 1.1, 1.3, 1.5 };
    for (auto& res : resolutions) {
        cfg.simConfig.particlesPerSquaremeter = std::get<0>(res);
        cfg.simConfig.dtIISPH = std::get<1>(res);
        for (double beta : betas) {
            for (double omega : omegas) {
                cfg.simConfig.iisphBeta = beta;
                cfg.simConfig.iisphOmega = omega;
                // printlog = true;
                // RunSimulation();
                std::ostringstream prefix;
                prefix << "logs/solver/heavy_ball/"
                    << "n_" << std::get<0>(res)
                    << "_" << JacobiVariantStr[HeavyBall]
                    << "_b_" << beta
                    << "_o_" << omega << "_";
                std::cout << JacobiVariantStr[HeavyBall] << "\tn=" << std::get<0>(res)
                    << "\tbeta = " << beta << "\tomega = " << omega << std::endl;
                RunAnalysis(cfg.simConfig,
                    buffertime,    // bufferTime<
                    runtime,     // runTime
                    cfl_threshold,    // cflThreshold
                    true,   // earlyStopping
                    prefix.str()
                );
            }
        }
    }
}

void Program::AnalyseNesterov() {
    cfg = scenes::WaterColumn();
    cfg.simConfig.jacobiVariant = Nesterov;
    cfg.simConfig.iisphMaxIterations = 300;

    double betas[] = { 0.7, 0.8, 0.85, 0.9, 0.95 };
    double omegas[] = { 0.3, 0.4, 0.5, 0.6, 0.7 };
    for (auto& res : resolutions) {
        cfg.simConfig.particlesPerSquaremeter = std::get<0>(res);
        cfg.simConfig.dtIISPH = std::get<1>(res);
        for (double beta : betas) {
            for (double omega : omegas) {
                cfg.simConfig.iisphBeta = beta;
                cfg.simConfig.iisphOmega = omega;
                // printlog = true;
                // RunSimulation();
                std::ostringstream prefix;
                prefix << "logs/solver/heavy_ball/"
                    << "n_" << std::get<0>(res)
                    << "_" << JacobiVariantStr[Nesterov]
                    << "_b_" << beta
                    << "_o_" << omega << "_";
                std::cout << JacobiVariantStr[Nesterov] << "\tn=" << std::get<0>(res)
                    << "\tbeta = " << beta << "\tomega = " << omega << std::endl;
                RunAnalysis(cfg.simConfig,
                    buffertime,    // bufferTime<
                    runtime,     // runTime
                    cfl_threshold,    // cflThreshold
                    true,   // earlyStopping
                    prefix.str()
                );
            }
        }
    }
}

void Program::JacobiFixed() {
    cfg = scenes::WaterColumn();
    cfg.simConfig.iisphMaxIterations = 300;
    cfg.simConfig.useFixedIterations = true;

    int iterations[] = { 6, 12, 16, 20, 24 };
    /// JACOBI
    if(false)
    {
        cfg.simConfig.jacobiVariant = RelaxedJacobi;
        cfg.simConfig.iisphOmega = 0.5;
        for (auto& res : resolutions) {
            cfg.simConfig.particlesPerSquaremeter = std::get<0>(res);
            cfg.simConfig.dtIISPH = std::get<1>(res);

            for (int iter : iterations) {
                cfg.simConfig.fixedIterations = iter;

                std::ostringstream prefix;
                prefix << "logs/solver/fixed/"
                    << "n_" << std::get<0>(res)
                    << "_" << JacobiVariantStr[RelaxedJacobi]
                    << "_iter_" << iter << "_";

                std::cout << JacobiVariantStr[RelaxedJacobi]
                    << "\tn=" << std::get<0>(res) << "\titer=" << iter
                    << std::endl;
                RunAnalysis(cfg.simConfig,
                    buffertime,    // bufferTime<
                    runtime,     // runTime
                    cfl_threshold,    // cflThreshold
                    true,   // earlyStopping
                    prefix.str()
                );
            }
        }
    }
    /// Heavy Ball
    if(false)
    {
        std::tuple<int, double, double, double> resuls[] = {
            std::make_tuple(4096, 0.002, 0.8, 1.5),
            std::make_tuple(16384, 0.001, 0.7, 1.3),
            std::make_tuple(64009, 0.0005, 0.8, 1.1),
        };
        cfg.simConfig.jacobiVariant = HeavyBall;
        for (auto& res : resuls) {
            int n = std::get<0>(res);
            double dt = std::get<1>(res);
            double beta = std::get<2>(res);
            double omega = std::get<3>(res);
            cfg.simConfig.iisphBeta = beta;
            cfg.simConfig.iisphOmega = omega;
            cfg.simConfig.particlesPerSquaremeter = n;
            cfg.simConfig.dtIISPH = dt;

            for (int iter : iterations) {
                cfg.simConfig.fixedIterations = iter;

                std::ostringstream prefix;
                prefix << "logs/solver/fixed/"
                    << "n_" << n
                    << "_" << JacobiVariantStr[HeavyBall]
                    << "_b_" << beta
                    << "_o_" << omega
                    << "_iter_" << iter << "_";
                std::cout << JacobiVariantStr[HeavyBall]
                    << "\tn=" << n << "\titer=" << iter
                    << std::endl;
                RunAnalysis(cfg.simConfig,
                    buffertime,    // bufferTime<
                    runtime,     // runTime
                    cfl_threshold,    // cflThreshold
                    true,   // earlyStopping
                    prefix.str()
                );
            }
        }
    }

    /// Nesterov
    if(true)
    {
        std::tuple<int, double, double, double> resuls[] = {
            std::make_tuple(4096, 0.002, 0.95, 0.6),
            std::make_tuple(16384, 0.001, 0.95, 0.6),
            std::make_tuple(64009, 0.0005, 0.9, 0.5),
        };
        cfg.simConfig.jacobiVariant = Nesterov;
        for (auto& res : resuls) {
            int n = std::get<0>(res);
            double dt = std::get<1>(res);
            double beta = std::get<2>(res);
            double omega = std::get<3>(res);
            cfg.simConfig.iisphBeta = beta;
            cfg.simConfig.iisphOmega = omega;
            cfg.simConfig.particlesPerSquaremeter = n;
            cfg.simConfig.dtIISPH = dt;
            std::ostringstream prefix;

            for (int iter : iterations) {
                cfg.simConfig.fixedIterations = iter;
                
                std::ostringstream prefix;
                prefix << "logs/solver/fixed/"
                    << "n_" << n
                    << "_" << JacobiVariantStr[Nesterov]
                    << "_b_" << beta
                    << "_o_" << omega
                    << "_iter_" << iter << "_";
                std::cout << JacobiVariantStr[Nesterov]
                    << "\tn=" << n << "\titer=" << iter
                    << std::endl;
                RunAnalysis(cfg.simConfig,
                    buffertime,    // bufferTime<
                    runtime,     // runTime
                    cfl_threshold,    // cflThreshold
                    true,   // earlyStopping
                    prefix.str()
                );
            }
        }
    }
}

*/
}