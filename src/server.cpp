// ENCH headers
#include "enchiladas.h"
#include "EnchiladaServer.h"

// PBNJ headers
#include "pbnj.h"
#include "Camera.h"
#include "Configuration.h"
#include "Renderer.h"
#include "TransferFunction.h"
#include "Volume.h"

#include <iostream>
#include <cstdlib>

using namespace Net;

int main(int argc, const char **argv)
{
    if (argc != 3)
    {
        std::cerr << "Usage: " << argv[0] << " <config_file.json>" << "<port>";
        std::cerr << std::endl;
        return 1;
    }

    pbnj::Renderer *renderer;
    pbnj::Configuration *config;
    pbnj::Camera *camera;

    config = new pbnj::Configuration(argv[1]);

    pbnj::pbnjInit(&argc, argv);

    pbnj::Volume *volume = new pbnj::Volume(config->dataFilename, config->dataVariable,
            config->dataXDim, config->dataYDim, config->dataZDim, true);

    volume->setColorMap(config->colorMap);
    volume->setOpacityMap(config->opacityMap);
    volume->attenuateOpacity(config->opacityAttenuation);

    camera = new pbnj::Camera(config->imageWidth, 
            config->imageHeight);
    camera->setPosition(config->cameraX, config->cameraY, config->cameraZ);

    renderer = new pbnj::Renderer();
    renderer->setVolume(volume);
    renderer->setCamera(camera);

    Net::Port port(9080);
    port = std::stol(argv[2]);

    Net::Address addr(Net::Ipv4::any(), port);

    ench::EnchiladaServer eserver(addr, renderer, config, camera);
    eserver.init(1);
    eserver.start();
    eserver.shutdown();

    return 0;
}

