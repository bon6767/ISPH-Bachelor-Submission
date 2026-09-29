#pragma once
#include "../visualisation/visualisation_types.h"

namespace viscfg {
struct RenderConfig {
	unsigned int screenWidth = 1200u;
	unsigned int screenHeight = 1200u;
	unsigned int fpsLimit = 0;
	unsigned int renderInterval = 1;
	bool renderText = true;
	bool renderTextOnOutput = false;
	bool saveToImage = false;
	double outFPS = 20.;

	vis::RenderType winRenderType = vis::Particles;
	vis::RenderType vidRenderType = vis::MarchingSquares;

	double marchingGridCutOff = 0.95;
	double marchingThreshold = 800.;

	vis::VisualiseQuantity visualiseProperty = vis::Velocity;
	vis::ColorGradient velocityGradient = {
	{ 66, 135, 245 }, { 250, 250, 250 }, 0.5, 3.5 };
	vis::ColorGradient velocityGradientTransparent = {
{ 66, 135, 245, 150 }, { 250, 250, 250, 150 }, 0.5, 3.5 };
	vis::ColorGradient densityGradient = {
	{ 178, 255, 255, 255 }, { 235, 64, 52,255 }, 1.298, 1.3 };
	vis::ColorGradient pressureGradient = {
		{ 178, 255, 255, 255 }, { 235, 64, 52,255 }, 0, 100.};
	sf::Color basecolor = sf::Color{ 178, 255, 255, 255 };
	sf::Color backgroundColor = sf::Color{ 40, 44, 52, 255 };

	float viewSize = 11.f; // -> screenHeight = 10m
	float particleScale = 1.f;                      // scale factor only for the visualisation
	bool renderBoundary = true;

	// debuggy stuff
	bool visualiseCells = false;
	bool visualiseNeighbours = false;
	int debugParticle = -1;
};

RenderConfig StandardRenderConfig();
RenderConfig SmallRenderConfig();

}