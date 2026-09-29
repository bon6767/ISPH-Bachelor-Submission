#include "program.h"
#include <iostream>
#include <sstream>

using Clock = std::chrono::steady_clock;

namespace program {

Program::Program(AppConfig app) :
    cfg(app.simConfig),
    scene(app.scene),
    visualisation(Visualisation{ app.renderConfig, app.scene }) {
}

void Program::RunSimulation() {
    sim::Simulation sim{ cfg, scene };
    visualisation.initialiseRendering(sim);
    std::cout << "n=" << sim.get_n() << std::endl;

    /// Two clocks, because "how long did it take" has two answers here and only one of them is
    /// the solver's. `solve` accumulates Update() alone; `active` adds logging and rendering but
    /// still only while the run is actually stepping. Neither starts before the loop: the loop
    /// spins in HandleInput while paused, and with pause defaulting to true a single clock would
    /// be counting however long it took someone to press space.
    double solveMs = 0.;
    double activeMs = 0.;
    while (sim.runtime <= simulationTime) {
        HandleInput(sim);
        if (pause && !simulateSingleFrame) continue;    // when paused and user controlled single simulation steps
        simulateSingleFrame = false;

        const auto stepStart = Clock::now();
        sim.Update();
        const auto solveEnd = Clock::now();

        if (logging) LogData(sim.log);
        if (printlog) PrintLog(sim.log);

        // early stopping

        visualisation.renderSimulation(sim);

        const auto stepEnd = Clock::now();
        solveMs += std::chrono::duration<double, std::milli>(solveEnd - stepStart).count();
        activeMs += std::chrono::duration<double, std::milli>(stepEnd - stepStart).count();
    }
    std::cout << "finished " << sim.runtime << "s of simulation in "
              << solveMs / 1000. << "s of solver time"
              << "   (" << activeMs / 1000. << "s including logging and rendering)" << std::endl;
    std::ostringstream pathPrefix;
    pathPrefix << "logs/runs/" << utils::getTimeStamp() << "_";
    if (logging) StoreLog(pathPrefix.str(), cfg, sim.get_bn(), sim.get_n());
}

void Program::HandleInput(Simulation& sim) {
    auto* window = &visualisation.getWindow();
    // check all events triggered since last iteration
    while (const std::optional event = window->pollEvent()) {
        // close event
        if (event->is<sf::Event::Closed>()) {
            window->close();
        }
        if (const auto* keyPressed = event->getIf<sf::Event::KeyPressed>())
        {
            if (keyPressed->code == sf::Keyboard::Key::R)
            {
                sim.reverseTime();
            }
            if (keyPressed->code == sf::Keyboard::Key::Space)
            {
                pause = !pause;
            }
            if (keyPressed->code == sf::Keyboard::Key::E)
            {
                simulateSingleFrame = true;
            }
            if (keyPressed->code >= sf::Keyboard::Key::Num0 && keyPressed->code <= sf::Keyboard::Key::Num9)
            {
                int camSelect = (int)keyPressed->code-27;
                std::cout << "selected Camera " << camSelect << std::endl;
                visualisation.activeCamera = camSelect;
                if (pause) visualisation.renderSimulation(sim);
            }
            float camMoveSpeed = 0.3;
            if (keyPressed->code == sf::Keyboard::Key::A) {
                visualisation.MoveCamera(camMoveSpeed, 0.);
                if (pause) visualisation.renderSimulation(sim);
            }
            if (keyPressed->code == sf::Keyboard::Key::D) {
                visualisation.MoveCamera(-camMoveSpeed, 0.);
                if (pause) visualisation.renderSimulation(sim);

            }
            if (keyPressed->code == sf::Keyboard::Key::W) {
                visualisation.MoveCamera(0., camMoveSpeed);
                if (pause) visualisation.renderSimulation(sim);

            }
            if (keyPressed->code == sf::Keyboard::Key::S) {
                visualisation.MoveCamera(0., -camMoveSpeed);
                if (pause) visualisation.renderSimulation(sim);

            }
            if (keyPressed->code == sf::Keyboard::Key::X) {
                visualisation.ResetCamera();
                if (pause) visualisation.renderSimulation(sim);
            }
        }
        if (const auto* scroll = event->getIf<sf::Event::MouseWheelScrolled>())
        {
            visualisation.Zoom(0.1 * scroll->delta);
            // visualisation.marchingThreshold += 0.1 * scroll->delta;
            // std::cout << "treshold=" << visualisation.marchingThreshold << std::endl;
            if (pause) visualisation.renderSimulation(sim);
        }
    }
}

void Program::LogData(StepLog log) {
    stepLogs.push_back(log);
}

void Program::PrintLog(StepLog log) {
    std::cout
        << std::setprecision(2)
        << "runtime: " << log.realtime
        << "\tprogress: " << log.runtime << "/" << simulationTime << "s"
        << "\tDI err: " << log.DIErr * 100. << "%"
        << "\tVD err: " << log.VDErr * 100. << "%"
        << "\tDI iters : " << log.DIIters
        << "\tVD iters : " << log.VDIters
        << "\tcfl: " << log.cfl
        //<< "\tsolve t: " << log.solveTime
        //<< "\tnbr t: " << log.nbrTime
        //<< "\tstable: " << log.stable
        << std::endl; 
}

void Program::StoreLog(std::string prefix, SimulationConfig simCfg, size_t boundaryParticles,
                       size_t fluidParticles) {                   // call once, after the run
    std::ostringstream path;
    path << prefix
        << "log_" << ++logCount
        << ".csv";

    utils::ensureParentDir(path.str());
    std::ofstream file(path.str());
    file << std::setprecision(9)
         << "# " << SimulationConfigStr(simCfg, boundaryParticles, fluidParticles) << "\n"
         << "realtime,runtime,DIIters,VDiters,DIErr,VDErr,"
            "solveTime,nbrTime,kernelTime,setupTime,densityDeviation,velocityDivergence,"
            "stable,cfl,status\n";
    for (const auto& l : stepLogs)
        file << l.realtime << "," << l.runtime << ","
             << l.DIIters << "," << l.VDIters  << ","
             << l.DIErr << "," << l.VDErr  << ","
             << l.solveTime << "," << l.nbrTime << ","
             << l.kernelTime << "," << l.setupTime << ","
             << l.densityDeviation << "," << l.velocityDivergence << ","
             << l.stable << "," << l.cfl << ","
             << static_cast<int>(l.status) << "\n";   // SimulationStatus: 0 ok, 1 unstable, 2 iterCap
    stepLogs.clear();
}

}