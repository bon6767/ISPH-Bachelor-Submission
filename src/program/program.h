#pragma once
#include "../visualisation/visualisation.h"
#include "../scenes/scenes.h"
#include "../scenes/simConfig.h"
#include "../scenes/renderConfig.h"

using namespace sim;
using namespace vis;
using namespace simcfg;
using namespace viscfg;
using namespace scenes;

namespace program {
struct AppConfig {
	SimulationConfig simConfig;
	Scene scene;
	RenderConfig renderConfig;
};

class Program
{
public:
	Program(AppConfig cfg);

	void RunSimulation();
	void AnalysePerformance();
	void TimeStepAnalysis();
	void GammaAnalysis();
	void ConstIterVarTimeStep();
	void AnalyseMomentum();


	void AnalyseJacobi();
	void AnalyseHeavyBall();
	void AnalyseNesterov();
	void JacobiFixed();

	void RunAnalysis(SimulationConfig simCfg, Scene scene, double bufferTime,
		double runTime, double cflThreshold,bool earlyStopping,
		std::string logPathPrefix);

		
	bool logging = false;
	bool printlog = false;
	double simulationTime = 10.;
	bool pause = false;
private:
	Visualisation visualisation;
	SimulationConfig cfg;
	Scene scene;

	bool simulateSingleFrame = false;

	// logging
	std::vector<sim::StepLog> stepLogs;
	int logCount = 0;

	void HandleInput(Simulation& sim);
	void LogData(StepLog log);
	void StoreLog(std::string prefix, SimulationConfig simCfg, size_t boundaryParticles = 0,
	              size_t fluidParticles = 0);
	void PrintLog(StepLog log);
};
}