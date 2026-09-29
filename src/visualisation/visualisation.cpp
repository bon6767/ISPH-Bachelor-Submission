#include "visualisation.h"
#include "visualisation_types.h"
#include <iostream>
#include <sstream>

using namespace viscfg;
using Clock = std::chrono::steady_clock;


namespace vis {

Visualisation::Visualisation(RenderConfig renderConfig, Scene scene) :
    cfg(renderConfig),
    scene(scene),
    renderBoundary(renderConfig.renderBoundary),
    renderInterval(renderConfig.renderInterval),
    visualiseQuantity(renderConfig.visualiseProperty),
    frameInterval(1),
    visualiseCells(renderConfig.visualiseCells),
    visualiseNeighbours(renderConfig.visualiseNeighbours),
    activeCamera(0),
    cameras(scene.cameras),
    debugParticle(renderConfig.debugParticle),
    marchingThreshold(renderConfig.marchingThreshold)
{
}

void Visualisation::initialiseRendering(const Simulation& sim) {
    if (renderInterval <= 0) {
        std::cout << "(renderInterval <= 0 --> Headless simulation; no visualisation instance" << std::endl;
        return;
    }
    if (cameras.size() == 0) { std::cout << "no cameras. no rendering." << std::endl; return; }

    //window
    window = sf::RenderWindow(sf::VideoMode({ cfg.screenWidth, cfg.screenHeight }), scene.name,
        sf::State::Windowed);

    if (!font.openFromFile("assets/CascadiaMono.ttf")) {
        std::cout << "warning: could not load font 'assets/CascadiaMono.ttf'; text will not be rendered" << std::endl;
    }

    if (renderInterval <= 0) return;
    frameInterval = 1 / cfg.outFPS / sim.getTimestep();
    std::cout << "save interval " << frameInterval << std::endl;

    // particleTexture.loadFromFile("../../../../assets/circle.png");
    particleTexture.loadFromFile("assets/circle.png");
    particleTexture.setSmooth(true);    // makes it look round :)

    for (cam::Camera& camera : cameras) {
        camera.position = camera.startPosition;
    }
    
    /// Marching Squares
    marchingDomainSize = scene.gridDimension * cfg.marchingGridCutOff;
    marchingCellSize = sim.get_h();
    marchingGridDimension = ceil(marchingDomainSize / marchingCellSize);
    buildMarchingGridPositions();
    std::cout << "dimension=" << marchingGridDimension*marchingGridDimension
        << " domainSize=" << marchingDomainSize << std::endl;
    marchingMesh = sf::VertexBuffer(sf::PrimitiveType::Triangles,
        sf::VertexBuffer::Usage::Dynamic);
    marchingMesh.create(marchingGridDimension * marchingGridDimension * 9);     // 9 is the num of vertices per cell


    createVertexBuffer(sim, winTrisVB);
    createVertexBuffer(sim, vidTrisVB);
    sceneObjRecs = sceneObjects(sim);
    renderSimulation(sim);

    std::cout << "initialised visualisation instance" << std::endl;
    std::cout << "render interval: " << renderInterval << std::endl;
}

void Visualisation::createVertexBuffer(const Simulation& sim, sf::VertexBuffer& buffer) {
    float worldToScreenScale = WorldToScreenScale(activeCamera);
    int fn = sim.get_n();
    int bn = sim.get_bn();
    int n = renderBoundary ? fn + bn : fn;
    // vertex buffer
    vertices.resize((n) * 6);
    buffer = sf::VertexBuffer(sf::PrimitiveType::Triangles, sf::VertexBuffer::Usage::Stream);
    buffer.create(vertices.size());

    // for saving images, create render texture
    if (cfg.saveToImage) rt = sf::RenderTexture({ window.getSize().x, window.getSize().y });

    sf::Color c = sf::Color{ 0, 0, 0, 0 };

    // add texCoords
    for (int i = 0; i < n; i++) {

        int v = i * 6;

        // triangle 1: tl, tr, bl
        vertices[v + 0].texCoords = { 0.f, 0.f };   vertices[v + 0].color = c;
        vertices[v + 1].texCoords = { 64.f, 0.f };  vertices[v + 1].color = c;
        vertices[v + 2].texCoords = { 0.f, 64.f };  vertices[v + 2].color = c;

        // triangle 2: tr, br, bl
        vertices[v + 3].texCoords = { 64.f, 0.f };  vertices[v + 3].color = c;
        vertices[v + 4].texCoords = { 64.f, 64.f }; vertices[v + 4].color = c;
        vertices[v + 5].texCoords = { 0.f, 64.f };  vertices[v + 5].color = c;
    }

    /// boundary iteration
    for (int i = fn; i < n; i++) {
        auto& pos_x = sim.getPos_x();
        auto& pos_y = sim.getPos_y();

        Vector2f pos = worldToScreen(Vector2f(pos_x[i], pos_y[i]), activeCamera);
        const float r = sim.getParticleRadius()
            * cfg.particleScale * worldToScreenScale * sim.get_bSize();

        sf::Color c = { 255, 255, 255, 255 };

        // relative corner positions
        const Vector2f tl = { pos.x - r, pos.y + r }; // top left
        const Vector2f tr = { pos.x + r, pos.y + r }; // top right
        const Vector2f bl = { pos.x - r, pos.y - r }; // bottom left
        const Vector2f br = { pos.x + r, pos.y - r }; // bottom right

        const int v = i * 6;

        // triangle 1: tl, tr, bl
        vertices[v + 0].position = tl;  vertices[v + 0].color = c;
        vertices[v + 1].position = tr;  vertices[v + 1].color = c;
        vertices[v + 2].position = bl;  vertices[v + 2].color = c;

        // triangle 2: tr, br, bl
        vertices[v + 3].position = tr;  vertices[v + 3].color = c;
        vertices[v + 4].position = br;  vertices[v + 4].color = c;
        vertices[v + 5].position = bl;  vertices[v + 5].color = c;
    }
    buffer.update(vertices.data());
}

void Visualisation::renderSimulation(const Simulation& sim) {
    bool renderWindow = (renderInterval > 0                 // we want to render AND
        && sim.getSteps() % renderInterval == 0);           // we should render window
    bool renderVideo = (cfg.saveToImage                     // we want to render video AND
        && sim.runtime - lastSaveTime >= 1. / cfg.outFPS);  // we should render video
    bool renderFrame = (renderWindow || renderVideo) && cameras.size() > 0;

    if (!renderFrame) return;
    
    if (activeCamera >= cameras.size()) activeCamera = 0;
    
    bool cameraChange = activeCamera != lastActiveCam || lastZoom != cameras[activeCamera].zoom || lastCamPos != cameras[activeCamera].position;
    if (cameraChange) {
        if(cfg.winRenderType == Particles) createVertexBuffer(sim, winTrisVB);
        if(cfg.vidRenderType == Particles) createVertexBuffer(sim, vidTrisVB);
        createVertexBuffer(sim, vidTrisVB); /// TODO GET RID OF THIS AGAIN, JUST FOR A TEST
        if(cfg.winRenderType == MarchingSquares
         ||cfg.vidRenderType == MarchingSquares) sceneObjRecs = sceneObjects(sim);

        lastActiveCam = activeCamera; lastZoom = cameras[activeCamera].zoom; lastCamPos = cameras[activeCamera].position;
    }

    if (renderWindow) {
        window.clear(cfg.backgroundColor);
        if (cfg.winRenderType == Particles) {
            renderParticles(sim, winTrisVB);
            window.draw(winTrisVB, &particleTexture);
        }
        else {
            marchingSquares(sim);
            window.draw(winTrisVB, &particleTexture);
            window.draw(marchingMesh);
            for (auto r : sceneObjRecs) {
                window.draw(r);
            }
        }
        window.display();
    }

    /// This is kind of überflüssig right now but i want to do this for all render cameras soon instead of just this,
    /// so i want to have it separated already. (win and vid rendering)
    /// should ofc also have separate marchingTris but whatever
    if (renderVideo) {
        rt.clear(cfg.backgroundColor);
        if (cfg.vidRenderType == Particles) {
            std::cout << "YEA";
            renderParticles(sim, vidTrisVB);
            rt.draw(vidTrisVB, &particleTexture);
        }
        else {
            marchingSquares(sim);
            rt.draw(marchingMesh);
            rt.draw(vidTrisVB, &particleTexture);
            for (auto r : sceneObjRecs) {
                rt.draw(r);
            }
        }
        if (cfg.renderTextOnOutput) DrawTimeOnOutput(sim);
        saveVideoFrame(sim);
    }
}

void Visualisation::saveVideoFrame(const Simulation& sim) {
    lastSaveTime = sim.runtime;
    std::cout << "save image " << saveFrame << std::endl;

    rt.display();

    sf::Image img = rt.getTexture().copyToImage();
    std::ostringstream filename;
    filename << "frames/" << std::setw(6) << std::setfill('0') << saveFrame++ << ".tga";
    utils::ensureParentDir(filename.str());
    img.saveToFile(filename.str());

}

void Visualisation::renderParticles(const Simulation& sim, sf::VertexBuffer& buffer)
{
    float worldToScreenScale = WorldToScreenScale(activeCamera);
    //define the positions of the triangle's points
    size_t n = sim.get_n();
    size_t bn = sim.get_bn();
    // if particle count changed, redo buffer.
    if (vertices.size() != (n + bn) * 6) {
        createVertexBuffer(sim, buffer);
    }

    auto& pos_x = sim.getPos_x();
    auto& pos_y = sim.getPos_y();

    const float r = sim.getParticleRadius() *
        cfg.particleScale * worldToScreenScale;

    //define the positions of the triangle's points
#pragma omp parallel for
    for (size_t i = 0; i < n; i++) {
        Vector2f pos = worldToScreen(Vector2f(pos_x[i], pos_y[i]), activeCamera);

        // relative corner positions
        const Vector2f tl = { pos.x - r, pos.y + r }; // top left
        const Vector2f tr = { pos.x + r, pos.y + r }; // top right
        const Vector2f bl = { pos.x - r, pos.y - r }; // bottom left
        const Vector2f br = { pos.x + r, pos.y - r }; // bottom right

        const int v = i * 6;

        auto c = ComputeParticleColor(sim, i);
        // triangle 1: tl, tr, bl
        vertices[v + 0].position = tl;  vertices[v + 0].color = c;
        vertices[v + 1].position = tr;  vertices[v + 1].color = c;
        vertices[v + 2].position = bl;  vertices[v + 2].color = c;

        // triangle 2: tr, br, bl
        vertices[v + 3].position = tr;  vertices[v + 3].color = c;
        vertices[v + 4].position = br;  vertices[v + 4].color = c;
        vertices[v + 5].position = bl;  vertices[v + 5].color = c;
    }

    // for applying colors by the Debug function
    // it only does something if a valid value has been set for particleToDebugNeighbours
    if (visualiseCells) VisualiseCells(sim);
    //debug::debugVisualiseProperty(&simulation, this);
    if (visualiseNeighbours)PaintNeighbours(sim);

    buffer.update(vertices.data());
    if (cfg.renderText) renderTexts(sim);
}

void Visualisation::renderTexts(const Simulation& sim) {
    // set the string to display
    std::ostringstream timeString;
    std::ostringstream cflString;
    std::ostringstream timeStepString;
    timeString << std::setprecision(3) << "t=" << std::fixed << sim.runtime << "s";
    cflString << std::setprecision(3) << std::fixed << "          cfl=" << sim.getCFLValue();
    timeStepString << "                     dt=1/" << 1.f / sim.getTimestep() << "s";

    DrawString(timeString.str(), 48, Vector2f{ 0.05f * cfg.screenWidth, cfg.screenHeight - 0.065f * cfg.screenHeight });
    DrawString(cflString.str(), 48, Vector2f{ 0.05f * cfg.screenWidth, cfg.screenHeight - 0.065f * cfg.screenHeight });
    DrawString(timeStepString.str(), 48, Vector2f{ 0.05f * cfg.screenWidth, cfg.screenHeight - 0.065f * cfg.screenHeight });
}

Vector2f Visualisation::worldToScreen(Vector2f coord, int cameraId) {
    Vector2f offset = Vector2f(
        cfg.screenWidth,
        cfg.screenHeight
    ) / 2.f;
    Vector2f camOffset = Vector2f(cameras[activeCamera].position.x, cameras[activeCamera].position.y) * WorldToScreenScale(cameraId);
    return (coord * WorldToScreenScale(cameraId)) + offset - camOffset;
}

Vector2f Visualisation::worldToScreen(Vector2d coord, int cameraId) {
    Vector2f c = { (float)coord.x, (float)coord.y };
    Vector2f offset = Vector2f(
        cfg.screenWidth,
        cfg.screenHeight
    ) / 2.f;
    Vector2f camOffset = Vector2f(cameras[activeCamera].position.x, cameras[activeCamera].position.y) * WorldToScreenScale(cameraId);
    return (c * WorldToScreenScale(cameraId)) + offset - camOffset;
}

float Visualisation::WorldToScreenScale(int cameraId)
{
    return (cfg.screenHeight / cameras[cameraId].viewSize) * cameras[cameraId].zoom;
}

sf::Color Visualisation::ComputeParticleColor(const Simulation& sim, int i)
{
    if (visualiseQuantity == Density) {
        float density = sim.getDensity(i);
        return ComputeColorGradient(cfg.densityGradient, density);
    }

    if (visualiseQuantity == Velocity) {
        float velocity = sim.getParticleSpeed(i);
        return ComputeColorGradient(cfg.velocityGradient, velocity);
    }
    if (visualiseQuantity == Pressure) {
        float pressure = sim.getPressure(i);
        return ComputeColorGradient(cfg.pressureGradient, pressure);
    }
    return cfg.basecolor;
}

sf::Color Visualisation::ComputeColorGradient(ColorGradient gradient, double t) {
    t = ((t - gradient.minVal) / (gradient.maxVal - gradient.minVal));  // maps x to (0, 1)
    t = std::clamp(t, 0., 1.);
    auto color = sf::Color(
        gradient.minColor.r + (gradient.maxColor.r - gradient.minColor.r) * t,
        gradient.minColor.g + (gradient.maxColor.g - gradient.minColor.g) * t,
        gradient.minColor.b + (gradient.maxColor.b - gradient.minColor.b) * t,
        gradient.minColor.a + (gradient.maxColor.a - gradient.minColor.a) * t);
    return color;
}

std::vector<sf::RectangleShape> Visualisation::sceneObjects(const Simulation& sim)
{
    auto scale = WorldToScreenScale(activeCamera);
    float thickness = sim.get_h() * scale;
    sf::Color bColor{ 233, 231, 251, 255 };
    std::vector<sf::RectangleShape> recs{};

    // boundary
    Vector2f b{ (float)scene.boundary.x * scale, (float)scene.boundary.y * scale };
    Vector2f o = worldToScreen(scene.boundaryOffset, activeCamera);
    sf::RectangleShape r;
    r.setPosition(o - b / 2.f);
    r.setSize({ (float)b.x, (float)b.y });
    r.setOutlineThickness(thickness);
    r.setFillColor({ 0,0,0,0 });
    r.setOutlineColor(bColor);
    recs.push_back(r);

    for (auto b : scene.barriers) {
        Vector2f start = worldToScreen(b.start, activeCamera);
        Vector2f end = worldToScreen(b.end, activeCamera);
        auto diff = end - start;
        float length = utils::magnitude(diff.x, diff.y);    // this is right
        float dx = 0.f;
        float dy = 0.f;
        Vector2f pos = {};
        if (start.x > end.x) {
            dx = end.x - start.x;
            dy = end.y - start.y;
            pos = start;
        }
        else {
            dx = end.x - start.x;
            dy = end.y - start.y;
            pos = start;
        }
        float angle = std::atan2(dy, dx);

        sf::RectangleShape rec;
        rec.setPosition({(float)pos.x, (float)pos.y});
        rec.setRotation(sf::radians(angle));
        rec.setSize({ length, 0 });
        rec.setOutlineThickness(thickness/2.);
        rec.setOutlineColor(bColor);
        recs.push_back(rec);
    }

    return recs;
}

void Visualisation::DrawString(std::string string, int fontSize, Vector2f pos) {
    sf::Text text(font);
    text.setCharacterSize(fontSize); // in pixels, not points!
    text.setString(string);
    text.setPosition(pos);
    window.draw(text);
}

/// The saved frames get the time and nothing else: they are figure material, and the cfl and
/// timestep belong in the caption rather than burned into the image.
/// Drawn from the video branch rather than from renderTexts, because the particles are drawn
/// after that call - text written there ends up behind the fluid.
void Visualisation::DrawTimeOnOutput(const Simulation& sim) {
    std::ostringstream timeString;
    timeString << std::setprecision(3) << "t=" << std::fixed << sim.runtime << "s";

    sf::Text text(font);
    text.setCharacterSize(48);
    text.setString(timeString.str());
    text.setPosition({ 0.05f * cfg.screenWidth, cfg.screenHeight - 0.065f * cfg.screenHeight });
    rt.draw(text);
}

void Visualisation::colorParticle(int i, sf::Color c) {
    int v = i * 6;

    setVertexColor(v + 0, c);
    setVertexColor(v + 1, c);
    setVertexColor(v + 2, c);
    setVertexColor(v + 3, c);
    setVertexColor(v + 4, c);
    setVertexColor(v + 5, c);
}

void Visualisation::VisualiseCells(const Simulation& sim) {
    for (int i = 0; i < sim.get_n(); i++) {
        int cell = sim.getParticleToCell(i);
        int colorValue = cell % 2 == 0 ? 255 : 140;
        // int colorValue = (cell % 200)+55;
        auto color = sf::Color(0, colorValue, colorValue, 255);
        colorParticle(i, color);
    }
}

void Visualisation::PaintNeighbours(const Simulation& sim) {
    if (debugParticle < 0 || debugParticle >= sim.get_n()) return;
    auto& nbrs = sim.getNeighbours(debugParticle);
    if (nbrs.empty()) return;

    // old debug to make sure neighbourcounts are right
    // size_t numNeighbours = nbrs.size();
    // std::cout << "num nbrs: " << numNeighbours << std::endl;

    auto c1 = sf::Color{ 255, 0, 0, 255 };
    auto c2 = sf::Color{ 255, 200, 0, 255 };

    for (auto i : nbrs) {
        auto c = c1;
        if (i == debugParticle) c = c2;
        colorParticle(i, c);
    }
}

void Visualisation::Zoom(float delta) {
    if (cameras[activeCamera].zoom + delta < 0.1f) {
        cameras[activeCamera].zoom = 0.1; 
    }else if (cameras[activeCamera].zoom + delta > 30.f) {
        cameras[activeCamera].zoom = 30.f;
    }else {
        cameras[activeCamera].zoom += delta;
    }
}

void Visualisation::MoveCamera(float x, float y) {
    float scale = (cameras[activeCamera].zoom != 0) ? cameras[activeCamera].zoom : 1;
    cameras[activeCamera].position.x -= x / scale;
    cameras[activeCamera].position.y -= y / scale;
}

void Visualisation::ResetCamera() {
    cameras[activeCamera].position = cameras[activeCamera].startPosition;
    cameras[activeCamera].zoom = 1.f;
}

}