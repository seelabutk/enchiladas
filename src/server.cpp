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

#include <dirent.h>
#include <iostream>
#include <cstdlib>
#include <map>
#include <tuple>

using namespace Net;

int main(int argc, const char **argv)
{
    if (argc != 3)
    {
        std::cerr << "Usage: " << argv[0] << " <configuration directory> " << "<port>";
        std::cerr << std::endl;
        return 1;
    }

    std::string config_dir = argv[1];
    DIR *directory = opendir(config_dir.c_str());
    struct dirent *dirp;


    // Share the camera 
    pbnj::Camera *camera;
    std::map<std::string, std::tuple<pbnj::Configuration*, pbnj::Volume*,pbnj::Camera*, pbnj::Renderer*>> volume_map;

    pbnj::pbnjInit(&argc, argv);
    while ((dirp = readdir(directory)) != NULL)
    {
        std::string filename(dirp->d_name);
        std::string::size_type index = filename.rfind(".");
        if (index == std::string::npos)
        {
            continue;
        }
        std::string extension = filename.substr(index);
        if (extension.compare(".json") == 0)
        {
            std::tuple<pbnj::Configuration*, pbnj::Volume*, pbnj::Camera*, pbnj::Renderer*> pbnj_container; 
            pbnj::Configuration *config = new pbnj::Configuration(config_dir + "/" + filename);
            pbnj::Volume *volume = new pbnj::Volume(config->dataFilename, 
                    config->dataVariable, config->dataXDim, config->dataYDim, 
                    config->dataZDim, true);
            pbnj::Camera *camera;
            camera = new pbnj::Camera(config->imageWidth, config->imageHeight);
            pbnj::Renderer *renderer;
            renderer = new pbnj::Renderer();

            volume_map[filename.substr(0, index)] = std::make_tuple(config, volume, camera, renderer);
        }
    }

    for (auto it = volume_map.begin(); it != volume_map.end(); it++)
    {
        pbnj::Configuration *config = std::get<0>(it->second);
        pbnj::Volume *volume = std::get<1>(it->second);
        pbnj::Camera *camera = std::get<2>(it->second);
        pbnj::Renderer *renderer = std::get<3>(it->second);


        volume->setColorMap(config->colorMap);
        volume->setOpacityMap(config->opacityMap);
        volume->attenuateOpacity(config->opacityAttenuation);

        camera->setPosition(config->cameraX, config->cameraY, config->cameraZ);

        renderer->setBackgroundColor(config->bgColor);
        renderer->setVolume(volume);
        renderer->setCamera(camera);
    }

    Net::Port port(9080);
    port = std::stol(argv[2]);

    Net::Address addr(Net::Ipv4::any(), port);

    ench::EnchiladaServer eserver(addr, volume_map);
    eserver.init(1);
    eserver.start();
    eserver.shutdown();

    return 0;
}

