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
#include "TimeSeries.h"

#include <dirent.h>
#include <iostream>
#include <cstdlib>
#include <map>
#include <tuple>

using namespace Net;

int main(int argc, const char **argv)
{
    /*
     * Check the input parameters
     */
    if (argc != 3)
    {
        std::cerr << "Usage: " 
            << argv[0] 
            << " <configuration directory> " 
            << "<port>";

        std::cerr << std::endl;
        return 1;
    }

    // Variables for parsing the directory files
    std::string config_dir = argv[1];
    DIR *directory = opendir(config_dir.c_str());
    struct dirent *dirp;

    /*
     * A volume hash table that keeps PBNJ objects of a dataset
     * in one place 
     */
    std::map<std::string, ench::pbnj_container> volume_map;

    // Must call pbnjInit before using it
    pbnj::pbnjInit(&argc, argv);

    while ((dirp = readdir(directory)) != NULL)
    {
        std::string filename(dirp->d_name);
        std::string::size_type index = filename.rfind(".");
        // if the filename doesn't have an extension
        if (index == std::string::npos) 
        {
            continue;
        }
        std::string extension = filename.substr(index);
        if (extension.compare(".json") == 0)
        {
            pbnj::Configuration *config = new pbnj::Configuration(config_dir + "/" + filename);
            pbnj::Camera *camera = new pbnj::Camera(
                    config->imageWidth, 
                    config->imageHeight);
            pbnj::Renderer *renderer = new pbnj::Renderer();
            pbnj::CONFSTATE single_multi = config->getConfigState();
            ench::Dataset dataset;

            /*
             * If we have a single volume at hand
             */
            if (single_multi == pbnj::CONFSTATE::SINGLE_NOVAR 
                    || single_multi == pbnj::CONFSTATE::SINGLE_VAR)
            {
                dataset.volume = new pbnj::Volume(
                        config->dataFilename, 
                        config->dataVariable, 
                        config->dataXDim, 
                        config->dataYDim, 
                        config->dataZDim, true);

                dataset.volume->setColorMap(config->colorMap);
                dataset.volume->setOpacityMap(config->opacityMap);
                dataset.volume->attenuateOpacity(config->opacityAttenuation);
                renderer->setVolume(dataset.volume);
            }
            /*
             * If we have a time series
             */
            else if (single_multi == pbnj::CONFSTATE::MULTI_VAR 
                    || single_multi == pbnj::CONFSTATE::MULTI_NOVAR)
            {
                dataset.timeseries = new pbnj::TimeSeries(
                        config->globbedFilenames, 
                        config->dataVariable, 
                        config->dataXDim,
                        config->dataYDim,
                        config->dataZDim);
                renderer->setVolume(dataset.timeseries->getVolume(0));

            }
            else
            {
                std::cerr<<"Cannot open this type of PBNJ file: "<<filename;
                continue;
            }

            camera->setPosition(config->cameraX, config->cameraY, config->cameraZ);

            renderer->setBackgroundColor(config->bgColor);
            renderer->setCamera(camera);

            volume_map[filename.substr(0, index)] = std::make_tuple(config, 
                    dataset, camera, renderer);
        }
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

