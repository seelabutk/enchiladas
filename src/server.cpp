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
#include <csignal>

using namespace Pistache;

static volatile int doShutdown = 0;

void sigintHandler(int sig) {
  doShutdown = 1;
}

void waitForShutdown(ench::EnchiladaServer *ench) {
  std::signal(SIGINT, sigintHandler);

  while (!doShutdown) {
    sleep(1);
  }

  ench->shutdown();
}

int main(int argc, const char **argv)
{
    /*
     * Check the input parameters
     */
    if (argc != 3 && argc != 4)
    {
        std::cerr << "Usage: " 
            << argv[0] 
            << " <configuration directory>" 
            << " <port>"
            << " [application directory]";

        std::cerr << std::endl;
        return 1;
    }

    std::string app_dir = ".";
    if (argc == 4)
    {
        app_dir = argv[3];
    }

    // Variables for parsing the directory files
    std::string config_dir = argv[1];
    DIR *directory;
    if ((directory = opendir(config_dir.c_str())) == NULL)
    {
        std::cerr<<"Could not open configuration directory"<<std::endl;
        exit(-1);
    }
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

            // Let's keep a renderer per volume to support time series for now
            pbnj::Renderer **renderer; 
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
                renderer = new pbnj::Renderer*[1];
                renderer[0] = new pbnj::Renderer();
                renderer[0]->setVolume(dataset.volume);
                renderer[0]->setBackgroundColor(config->bgColor);
                renderer[0]->setCamera(camera);
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
                dataset.timeseries->setColorMap(config->colorMap);
                dataset.timeseries->setOpacityMap(config->opacityMap);
                dataset.timeseries->setOpacityAttenuation(config->opacityAttenuation);
                dataset.timeseries->setMemoryMapping(true);
                dataset.timeseries->setMaxMemory(30);

                renderer = new pbnj::Renderer*[dataset.timeseries->getLength()];
                for (int i = 0; i < dataset.timeseries->getLength(); i++)
                {
                    renderer[i] = new pbnj::Renderer();
                    renderer[i]->setVolume(dataset.timeseries->getVolume(i));
                    renderer[i]->setBackgroundColor(config->bgColor);
                    renderer[i]->setCamera(camera);
                }
            }
            else
            {
                std::cerr<<"Cannot open this type of PBNJ file: "<<filename;
                continue;
            }

            camera->setPosition(config->cameraX, config->cameraY, config->cameraZ);
            volume_map[filename.substr(0, index)] = std::make_tuple(config, 
                    dataset, camera, renderer);
        }
    }

    Pistache::Port port(9080);
    port = std::stol(argv[2]);

    Pistache::Address addr(Pistache::Ipv4::any(), port);

    ench::EnchiladaServer eserver(addr, volume_map);
    eserver.init(app_dir, 1);

    eserver.start();

    std::thread waitForShutdownThread(waitForShutdown, &eserver);
    waitForShutdownThread.join();

    return 0;
}

