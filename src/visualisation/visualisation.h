#pragma once
#include <SFML/Graphics.hpp>
#include "../simulation/simulation.h"
#include "../scenes/renderConfig.h"
#include "../scenes/camera.h"

using namespace viscfg;
using namespace sim;
using sf::Vector2f;

namespace vis {

class Visualisation
{
public:
	explicit Visualisation(RenderConfig renderConfig, Scene scene);

	void initialiseRendering(const Simulation& sim);
	void renderSimulation(const Simulation& sim);							// calls function to render simulation according to renderType
	sf::RenderWindow& getWindow() { return window; }
	
	void colorParticle(int i, sf::Color c);
	void setVertexColor(int i, sf::Color c) { vertices[i].color = c; }
	bool renderBoundary;
	int renderInterval;
	int frameInterval;

	unsigned int activeCamera;
	void Zoom(float delta);
	void MoveCamera(float x, float y);
	void ResetCamera();


	double marchingThreshold;
	/// Writable up until initialiseRendering, which is what reads marchingGridCutOff and
	/// saveToImage - so a run can choose how it is drawn instead of every run sharing whatever
	/// main.cpp happened to build the Program with.
	RenderConfig& renderConfig() { return cfg; }
private:
	RenderConfig cfg;
	Scene scene;
	void createVertexBuffer(const Simulation& simulation, sf::VertexBuffer& buffer);
	void Visualisation::DrawString(std::string string, int fontSize, Vector2f pos);
	void DrawTimeOnOutput(const Simulation& sim);		// time only, onto the saved frame

	void renderParticles(const Simulation& sim, sf::VertexBuffer& buffer);
	void renderTexts(const Simulation& sim);
	void saveVideoFrame(const Simulation& sim);

	// Different Visualisations
	VisualiseQuantity visualiseQuantity;

	sf::RenderWindow window;					// sfml render window object

	// for VertexfBuffer renderType
	sf::Texture particleTexture;				// texture for circle (for VertexArray approach)
	sf::VertexBuffer winTrisVB;					// like vertexArray but on GPU, even faster
	sf::VertexBuffer vidTrisVB;					// like winTrisVB but for the video output
	std::vector<sf::Vertex> vertices;			// (for winTrisVB) documentation uses an array, but those can't be resized
	
	// Marching Squares
	double marchingDomainSize;
	double marchingCellSize;
	int marchingGridDimension;

	std::vector<double> marchingDensities;
	std::vector<double> marchingSpeeds;
	std::vector<Vector2d> marchingGrid;
	// std::vector<bool> lastInside;
	// std::vector<bool> lastOutside;
	sf::VertexBuffer marchingMesh;

	std::vector<sf::RectangleShape> sceneObjRecs;
	void marchingSquares(const Simulation& sim);
	Vector2d marchingGridPointToPosition(int i, int j);
	void marchingGridPointUpdate(const Simulation& sim, int cellIndex);
	void buildMarchingGridPositions();

	// for saving to image
	std::vector<cam::Camera> cameras;
	int lastActiveCam = 0;
	float lastZoom = 1;
	Vector2d lastCamPos = { 0.,0. };
	sf::RenderTexture rt;
	int saveFrame = 0;
	double lastSaveTime = -INFINITY;

	// for text
	sf::Font font; // loaded via openFromFile; left empty (no glyphs) if loading fails

	Vector2f worldToScreen(Vector2f coord, int cameraId);
	Vector2f worldToScreen(Vector2d coord, int cameraId);
	float WorldToScreenScale(int cameraId);
	sf::Color ComputeParticleColor(const Simulation& sim, int i);
	sf::Color ComputeColorGradient(ColorGradient gradient, double t);
	std::vector<sf::RectangleShape> sceneObjects(const Simulation& sim);

	// debuggy stuff
	bool visualiseCells = false;
	bool visualiseNeighbours = false;
	int debugParticle = -1;
	void VisualiseCells(const Simulation& sim);
	void PaintNeighbours(const Simulation& sim);
};
}