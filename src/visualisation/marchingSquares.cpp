#include "visualisation.h"
#include <iostream>

namespace vis
{
void Visualisation::marchingSquares(const Simulation& sim) {
	//int insideCells = 0;
	int end = marchingGridDimension * marchingGridDimension - 1;
	int mid = end / 2 + marchingGridDimension / 2;
	const int cellsPerAxis =
		static_cast<int>(marchingGridDimension) - 1;

	std::vector<sf::Vertex> marchingVertices(marchingGridDimension * marchingGridDimension * 9);
// #pragma omp parallel for
	for (int i = 0; i < cellsPerAxis; i++) {
		for (int j = 0; j < cellsPerAxis; j++) {
			// i is the row index, j is the column index
			size_t cellIndexA = i * marchingGridDimension + j;
			size_t cellIndexB = i * marchingGridDimension + j+1;
			size_t cellIndexC = (i+1) * marchingGridDimension + j;
			size_t cellIndexD = (i+1) * marchingGridDimension + j+1;

			size_t offset = cellIndexA * 9; // 9=verticespercell

			/// UPDATE GRID
			{
			// only compute the corners that havent been computed yet:
			if (i == 0 && j == 0) {	// compute all 4 corners
				marchingGridPointUpdate(sim, cellIndexA);
				marchingGridPointUpdate(sim, cellIndexB);
				marchingGridPointUpdate(sim, cellIndexC);
				marchingGridPointUpdate(sim, cellIndexD);
			}
			if (i == 0 && j > 0) {
				marchingGridPointUpdate(sim, cellIndexB);
				marchingGridPointUpdate(sim, cellIndexD);
			}
			if (i > 0 && j == 0) {
				marchingGridPointUpdate(sim, cellIndexC);
				marchingGridPointUpdate(sim, cellIndexD);
			}
			if (i > 0 && j > 0) {
				marchingGridPointUpdate(sim, cellIndexD);
			}
			}
			// If cell is completely inside
			bool inA = marchingDensities[cellIndexA] >= marchingThreshold;
			bool inB = marchingDensities[cellIndexB] >= marchingThreshold;
			bool inC = marchingDensities[cellIndexC] >= marchingThreshold;
			bool inD = marchingDensities[cellIndexD] >= marchingThreshold;
			bool inside  =  inA &&  inB &&  inC &&  inD;
			bool outside = !inA && !inB && !inC && !inD;

			// if (inside) { insideCells++; }

			// if (cellIndexA == mid) {
			// 	std::cout << "in? " << inside << "," << lastInside[cellIndexA]
			// 		<< "\tout? " << outside << "," << lastOutside[cellIndexA] << std::endl;
			// }
			// if (lastInside[cellIndexA] == inside) continue;		// cell is still entirely inside
			// if (lastOutside[cellIndexA] == outside) continue;	// cell is still entirely outisde
			// 
			// lastInside[cellIndexA] = inside;
			// lastOutside[cellIndexA] = outside;

			sf::Vertex unused = { sf::Vector2f(0.f, 0.f ), sf::Color::Transparent };
			std::array<sf::Vertex, 9> cellVertices;	// all at same position = nothing to render
			cellVertices.fill(unused);

			sf::Vector2f posA = worldToScreen(marchingGrid[cellIndexA], activeCamera);
			sf::Vector2f posB = worldToScreen(marchingGrid[cellIndexB], activeCamera);
			sf::Vector2f posC = worldToScreen(marchingGrid[cellIndexC], activeCamera);
			sf::Vector2f posD = worldToScreen(marchingGrid[cellIndexD], activeCamera);

			sf::Color	 colA = ComputeColorGradient(cfg.velocityGradient, marchingSpeeds[cellIndexA]);
			sf::Color	 colB = ComputeColorGradient(cfg.velocityGradient, marchingSpeeds[cellIndexB]);
			sf::Color	 colC = ComputeColorGradient(cfg.velocityGradient, marchingSpeeds[cellIndexC]);
			sf::Color	 colD = ComputeColorGradient(cfg.velocityGradient, marchingSpeeds[cellIndexD]);

			sf::Vertex A = sf::Vertex{ posA, colA };	sf::Vertex B = sf::Vertex{ posB, colB };
			sf::Vertex C = sf::Vertex{ posC, colC };	sf::Vertex D = sf::Vertex{ posD, colD };


			// if cell is now entirely outside but wasn't before
			if (outside) 
			{
				cellVertices.fill(unused);
				
				// marchingMesh.update(
				// 	cellVertices.data(),
				// 	cellVertices.size(),
				// 	static_cast<unsigned int>(offset)
				// );
				for (std::size_t k = 0; k < 9; ++k)
					marchingVertices[offset + k] = cellVertices[k];
				continue;
			}

			// if cell is now entirely inside but wasn't before
			if (inside) {
			 	// triangle 1
			 	cellVertices[0] = A;
			 	cellVertices[1] = B;
			 	cellVertices[2] = D;
			 	
			 	// triangle 2
			 	cellVertices[3] = A;
			 	cellVertices[4] = D;
			 	cellVertices[5] = C;
			 	
			 	// marchingMesh.update(
			 	// 	cellVertices.data(),
			 	// 	cellVertices.size(),
			 	// 	static_cast<unsigned int>(offset)
			 	// );
				for (std::size_t k = 0; k < 9; ++k)
					marchingVertices[offset + k] = cellVertices[k];
			 	continue;
			}


			int configuration = inA * 8 + inB * 4 + inC * 2 + inD;

			float tE = (marchingThreshold   - marchingDensities[cellIndexA])
				/ (marchingDensities[cellIndexB] - marchingDensities[cellIndexA]);

			float tF = (marchingThreshold   - marchingDensities[cellIndexB])
				/ (marchingDensities[cellIndexD] - marchingDensities[cellIndexB]);

			float tG = (marchingThreshold   - marchingDensities[cellIndexA])
				/ (marchingDensities[cellIndexC] - marchingDensities[cellIndexA]);

			float tH = (marchingThreshold   - marchingDensities[cellIndexC])
				/ (marchingDensities[cellIndexD] - marchingDensities[cellIndexC]);

			sf::Vector2f posE = posA + tE * (posB - posA);
			sf::Vector2f posF = posB + tF * (posD - posB);
			sf::Vector2f posG = posA + tG * (posC - posA);
			sf::Vector2f posH = posC + tH * (posD - posC);

			sf::Color	 colE = ComputeColorGradient(ColorGradient{ colA, colB , 0, 1. }, tE);
			sf::Color	 colF = ComputeColorGradient(ColorGradient{ colB, colD , 0, 1. }, tF);
			sf::Color	 colG = ComputeColorGradient(ColorGradient{ colA, colC , 0, 1. }, tG);
			sf::Color	 colH = ComputeColorGradient(ColorGradient{ colC, colD , 0, 1. }, tH);

			sf::Vertex E = sf::Vertex{ posE, colE };	sf::Vertex F = sf::Vertex{ posF, colF };
			sf::Vertex G = sf::Vertex{ posG, colG };	sf::Vertex H = sf::Vertex{ posH, colH };


			switch (configuration)
			{
			default:
				std::cout << "unknown marching configuration" << std::endl;
				break;
			case 1:
				cellVertices[0] = D;
				cellVertices[1] = H;
				cellVertices[2] = F;
				break;
			case 2:
				cellVertices[0] = C;
				cellVertices[1] = G;
				cellVertices[2] = H;
				break;
			case 3:
				cellVertices[0] = D;
				cellVertices[1] = C;
				cellVertices[2] = F;

				cellVertices[3] = C;
				cellVertices[4] = G;
				cellVertices[5] = F;
				break;
			case 4:
				cellVertices[0] = B;
				cellVertices[1] = F;
				cellVertices[2] = E;
				break;
			case 5:
				cellVertices[0] = B;
				cellVertices[1] = D;
				cellVertices[2] = E;

				cellVertices[3] = D;
				cellVertices[4] = H;
				cellVertices[5] = E;
				break;
			case 6:
				cellVertices[0] = B;
				cellVertices[1] = F;
				cellVertices[2] = E;

				cellVertices[3] = C;
				cellVertices[4] = G;
				cellVertices[5] = H;
				break;
			case 7:
				cellVertices[0] = D;
				cellVertices[1] = C;
				cellVertices[2] = G;

				cellVertices[3] = D;
				cellVertices[4] = G;
				cellVertices[5] = E;

				cellVertices[6] = D;
				cellVertices[7] = E;
				cellVertices[8] = B;
				break;
			case 8:
				cellVertices[0] = A;
				cellVertices[1] = E;
				cellVertices[2] = G;
				break;
			case 9:
				cellVertices[0] = A;
				cellVertices[1] = E;
				cellVertices[2] = G;

				cellVertices[3] = D;
				cellVertices[4] = H;
				cellVertices[5] = F;
				break;
			case 10:
				cellVertices[0] = A;
				cellVertices[1] = E;
				cellVertices[2] = H;

				cellVertices[3] = A;
				cellVertices[4] = H;
				cellVertices[5] = C;
				break;
			case 11:
				cellVertices[0] = A;
				cellVertices[1] = E;
				cellVertices[2] = F;

				cellVertices[3] = A;
				cellVertices[4] = F;
				cellVertices[5] = D;

				cellVertices[6] = A;
				cellVertices[7] = D;
				cellVertices[8] = C;
				break;
			case 12:
				cellVertices[0] = A;
				cellVertices[1] = B;
				cellVertices[2] = F;

				cellVertices[3] = A;
				cellVertices[4] = F;
				cellVertices[5] = G;
				break;
			case 13:
				cellVertices[0] = B;
				cellVertices[1] = D;
				cellVertices[2] = H;

				cellVertices[3] = B;
				cellVertices[4] = H;
				cellVertices[5] = G;

				cellVertices[6] = B;
				cellVertices[7] = G;
				cellVertices[8] = A;
				break;
			case 14:
				cellVertices[0] = A;
				cellVertices[1] = B;
				cellVertices[2] = F;

				cellVertices[3] = A;
				cellVertices[4] = F;
				cellVertices[5] = H;

				cellVertices[6] = A;
				cellVertices[7] = H;
				cellVertices[8] = C;
				break;
			}
			
			for (std::size_t k = 0; k < 9; ++k)
				marchingVertices[offset + k] = cellVertices[k];
			//marchingMesh.update(
			//	cellVertices.data(),
			//	cellVertices.size(),
			//	static_cast<unsigned int>(offset)
			//);
		}
	}
	marchingMesh.update(marchingVertices.data());
	// std::cout << "sample density at 0=" << marchingDensities[0] << std::endl;
	// std::cout << "sample density at end=" << marchingDensities[end] << std::endl;
	// std::cout << "sample density at mid=" << marchingDensities[mid] << std::endl;
	// std::cout << "cells inside="<<insideCells << std::endl;
}

// void Visualisation marchingUpdateMeshAtCell()

Vector2d Visualisation::marchingGridPointToPosition(int i, int j) {
	double domainHalf = marchingDomainSize / 2.f;
	return Vector2d{
		-domainHalf + i * marchingCellSize,
		-domainHalf + j * marchingCellSize
	};
}

void Visualisation::marchingGridPointUpdate(const Simulation& sim, int cellIndex) {
	FieldSample res = sim.sampleField(marchingGrid[cellIndex]);
	marchingDensities[cellIndex] = res.density;
	marchingSpeeds[cellIndex] = res.speed();
}

void Visualisation::buildMarchingGridPositions() {
	std::cout << "init (" << marchingGridDimension * marchingGridDimension << ")" << std::endl;
	marchingDensities	= std::vector<double>		(marchingGridDimension * marchingGridDimension);
	marchingSpeeds		= std::vector<double>		(marchingGridDimension * marchingGridDimension);
	marchingGrid		= std::vector<Vector2d>		(marchingGridDimension * marchingGridDimension);
	// lastInside			= std::vector<bool>			(marchingGridDimension * marchingGridDimension);
	// lastOutside			= std::vector<bool>			(marchingGridDimension * marchingGridDimension, true);

	for (int i = 0; i < marchingGridDimension; i++) {
		for (int j = 0; j < marchingGridDimension; j++) {
			// i is the row index, j is the column index
			size_t cellIndex = i * marchingGridDimension + j;
			//std::cout << "at cell=" << cellIndex << std::endl;
			Vector2d pos = marchingGridPointToPosition(i, j);
			marchingGrid[cellIndex] = pos;
		}
	}
	
	// int end = marchingGridDimension * marchingGridDimension-1;
	// std::cout << "sample pos at 0: (" << marchingGrid[0].x << "," << marchingGrid[0].y << ")" << std::endl;
	// std::cout << "sample pos at end: (" << marchingGrid[end].x << "," << marchingGrid[end].y << ")" << std::endl;
	// std::cout << "sample pos at mid: (" << marchingGrid[end/2+ marchingGridDimension / 2].x << "," << marchingGrid[end/2+marchingGridDimension/2].y << ")" << std::endl;
	// std::cout << "init done" << std::endl;
}

}