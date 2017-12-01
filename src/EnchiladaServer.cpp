#include "EnchiladaServer.h"
#include "utils.h"

#include <iostream>
#include <fstream>
#include <cstdlib>
#include <string>
#include <map>
#include <memory>
#include <stdexcept>
#include <stdio.h>
#include <unistd.h>

#include "pistache/http.h"
#include "pistache/router.h"
#include "pistache/endpoint.h"
#include "pistache/client.h"

#include "pbnj.h"
#include "Camera.h"
#include "Configuration.h"
#include "Renderer.h"
#include "TransferFunction.h"
#include "TimeSeries.h"

namespace ench {

using namespace Pistache;

EnchiladaServer::EnchiladaServer(Pistache::Address addr, std::map<std::string, 
        ench::pbnj_container> vm):  
    httpEndpoint(std::make_shared<Pistache::Http::Endpoint>(addr)), volume_map(vm)
{
}

void EnchiladaServer::init(std::string app_dir, size_t threads)
{
    auto opts = Pistache::Http::Endpoint::options()
        .threads(threads)
        .flags(Pistache::Tcp::Options::InstallSignalHandler);

    this->httpEndpoint->init(opts);
    this->app_dir = app_dir;

    setupRoutes();
}

void EnchiladaServer::start()
{
    std::cout << "Listening..." << std::endl;
    this->httpEndpoint->setHandler(router.handler());
    this->httpEndpoint->serveThreaded();
}

void EnchiladaServer::shutdown()
{
    std::cout<<"Shutting down."<<std::endl;
    this->httpEndpoint->shutdown();
}

void EnchiladaServer::setupRoutes()
{
    using namespace Pistache::Rest;
    // serving html files
    Routes::Get(router, "/:filename?", Routes::bind(&EnchiladaServer::handleRoot, this));
    // serving static js files
    Routes::Get(router, "/js/:filename", 
            Routes::bind(&EnchiladaServer::handleJS, this));
    // serving static css files
    Routes::Get(router, "/css/:filename",
            Routes::bind(&EnchiladaServer::handleCSS, this));
    // serving renders
    Routes::Get(router, "/image/:dataset/:x/:y/:z/:upx/:upy/:upz/:vx/:vy/:vz/:lowquality/:options?",
            Routes::bind(&EnchiladaServer::handleImage, this));
    // routing to plugins
    Routes::Get(router, "/extern/:plugin/:args?", 
            Routes::bind(&EnchiladaServer::handleExternalCommand, this));

}

void EnchiladaServer::serveFile(Pistache::Http::ResponseWriter &response,
        std::string filename)
{
    try
    {
        Http::serveFile(response, filename.c_str());
    }
    catch(...)
    {
        std::string app_path = this->app_dir + "/" + filename;
        Http::serveFile(response, app_path.c_str());
    }
}


void EnchiladaServer::handleRoot(const Rest::Request &request,
        Pistache::Http::ResponseWriter response)
{
    std::string filename = "index.html"; // default root
    if (request.hasParam(":filename"))
    {
        filename = request.param(":filename").as<std::string>();
    }
    serveFile(response, filename.c_str());
}

void EnchiladaServer::handleJS(const Rest::Request &request, 
        Pistache::Http::ResponseWriter response)
{
    auto filename = request.param(":filename").as<std::string>();
    filename = "js/" + filename;
    serveFile(response, filename.c_str());
}

void EnchiladaServer::handleCSS(const Rest::Request &request, 
        Pistache::Http::ResponseWriter response)
{
    auto filename = request.param(":filename").as<std::string>();
    filename = "css/" + filename;
    serveFile(response, filename.c_str());
}

void EnchiladaServer::handleExternalCommand(const Rest::Request &request, 
        Pistache::Http::ResponseWriter response)
{
    std::string program = request.param(":plugin").as<std::string>();
    std::string args = "";

    if (request.hasParam(":args"))
    {
        args = request.param(":args").as<std::string>(); 
    }

    std::string command = this->app_dir + "/plugins/" + program + " " + args;
    std::string json_results = exec(command.c_str());
    response.send(Http::Code::Ok, json_results);
}

void EnchiladaServer::handleImage(const Rest::Request &request,
        Pistache::Http::ResponseWriter response)
{

    int camera_x = 0;
    int camera_y = 0;
    int camera_z = 0;

    float up_x = 0;
    float up_y = 1;
    float up_z = 0;

    float view_x = 0;
    float view_y = 0;
    float view_z = 1;

    int lowquality = 0;
    std::string dataset = "";

    if (request.hasParam(":dataset"))
    {
        dataset = request.param(":dataset").as<std::string>();

        camera_x = request.param(":x").as<std::int32_t>(); 
        camera_y = request.param(":y").as<std::int32_t>(); 
        camera_z = request.param(":z").as<std::int32_t>(); 

        up_x = request.param(":upx").as<float>();
        up_y = request.param(":upy").as<float>();
        up_z = request.param(":upz").as<float>();

        view_x = request.param(":vx").as<float>();
        view_y = request.param(":vy").as<float>();
        view_z = request.param(":vz").as<float>();

        lowquality = request.param(":lowquality").as<int>();
    }

    // Check if this dataset exists in the loaded datasets
    if (volume_map.count(dataset) == 0)
    {
        response.send(Http::Code::Not_Found, "Image does not exist");
        return;
    }

    pbnj::Configuration *config = std::get<0>(volume_map[dataset]);
    ench::Dataset udataset = std::get<1>(volume_map[dataset]);
    pbnj::Camera *camera = std::get<2>(volume_map[dataset]);
    pbnj::Renderer **renderer = std::get<3>(volume_map[dataset]);
    
    std::vector<unsigned char> png;

    int renderer_index = 0; // Equal to a valid timestep
    bool onlysave = false;
    bool do_isosurface = false;
    std::string filename = "";
    std::string save_filename;
    std::vector<std::string> filters; // If any image filters are specified, we'll put them here
    std::vector<float> isovalues; // In case isosurfacing is supported
    pbnj::Volume *temp_volume; // Either a normal volume or a timeseries one 

    // set a default volume based on the dataset type
    // it'll be either a timeseries volume from timestep 0
    // or the only single volume that exists
    pbnj::CONFSTATE single_multi = config->getConfigState();
    if (single_multi == pbnj::CONFSTATE::SINGLE_NOVAR 
            || single_multi == pbnj::CONFSTATE::SINGLE_VAR)
    {
        temp_volume = udataset.volume; //by default
    }
    else
    {
        temp_volume = udataset.timeseries->getVolume(0); // by default
    }

    // parse the request's extra options
    if (request.hasParam(":options"))
    {

        std::string options_line = request.param(":options").as<std::string>();
        std::vector<std::string> options;
        const char *options_chars = options_line.c_str();
        do
        {
            const char *begin = options_chars;
            while(*options_chars != ',' && *options_chars)
                options_chars++;
            options.push_back(std::string(begin, options_chars));

        } while(0 != *options_chars++);

        for (auto it = options.begin(); it != options.end(); it++)
        {
            if (*it == "isosurface")
            {
                it++; // Get the isovalues
                std::string isovalues_str = *it;
                
                // parse the isovalues
                const char *isovalues_char = isovalues_str.c_str();
                do {
                    const char* iso_begin = isovalues_char;
                    while(*isovalues_char != '-' && *isovalues_char)
                        isovalues_char++;
                    isovalues.push_back(std::stof(std::string(iso_begin, isovalues_char)));
                } while(0 != *isovalues_char++);

                do_isosurface = true;
            }

            if (*it == "colormap")
            {
                it++; // Get the value of the colormap
                // search the pbnj colormaps
                std::map<std::string, std::vector<float>>::iterator cmap_it;
                for (cmap_it = pbnj::colormaps.begin(); cmap_it != pbnj::colormaps.end(); cmap_it++)
                {
                    if (*it == cmap_it->first)
                    {
                        udataset.volume->setColorMap(pbnj::colormaps[*it]);
                        break;
                    }
                }
                // if the colormap wasn't found, it will default to grayscale
            }

            if (*it == "hq")
            {
                it++;
                if (*it == "true")
                {
                    std::cout<<"Woah, easter egg, rendering an 8192x8192 with 8 samples. "<<std::endl;
                    renderer[0]->cameraWidth = camera->imageWidth = 8192;
                    renderer[0]->cameraHeight = camera->imageHeight = 8192;
                    renderer[0]->setSamples(8);
                }
            }

            if (*it == "timestep")
            {
                it++;
                int timestep = std::stoi(*it);
                if (timestep >= 0 && timestep < udataset.timeseries->getLength())
                {
                    renderer_index = timestep;
                    temp_volume = udataset.timeseries->getVolume(renderer_index);
                }
                else
                {
                    std::cerr<<"Invalid timestep: "<<timestep<<std::endl;
                }
            }

            if (*it == "onlysave")
            {
                it++;
                onlysave = true;
                save_filename = *it;
            }

            if (*it == "filename")
            {
                it++;
                filename = *it;
                renderer_index = udataset.timeseries->getVolumeIndex(filename);
                temp_volume = udataset.timeseries->getVolume(renderer_index);
                if (renderer_index == -1)
                {
                    response.send(Http::Code::Not_Found, "Image does not exist");
                    return;
                }        
            }

            if (*it == "filters")
            {
                it++;
                std::string filter_names = *it;
                
                // parse the filter names and apply them one by one
                const char *filters_chars = filter_names.c_str();
                do {
                    const char* filter_begin = filters_chars;
                    while(*filters_chars != '-' && *filters_chars)
                        filters_chars++;
                    filters.push_back(std::string(filter_begin, filters_chars));
                } while(0 != *filters_chars++);

            }
        }
    }

    if (lowquality == 1)
    {
        renderer[renderer_index]->cameraWidth = camera->imageWidth = 64;
        renderer[renderer_index]->cameraHeight = camera->imageHeight = 64;
    }
    else
    {
        renderer[renderer_index]->cameraWidth = camera->imageWidth = std::min(config->imageWidth, lowquality);
        renderer[renderer_index]->cameraHeight = camera->imageHeight = std::min(config->imageHeight, lowquality);
    }

    camera->setPosition(camera_x, camera_y, camera_z);
    camera->setUpVector(up_x, up_y, up_z);
    camera->setView(view_x, view_y, view_z);

    if (onlysave)
    {
        // no filters are supported for the onlysave option at the moment
        std::cout<<"Saving to "<<save_filename<<std::endl;
        renderer[renderer_index]->renderImage("/app/data/" + save_filename + ".png");
        response.send(Http::Code::Ok, "saved");
    }
    else
    {
        if (do_isosurface)
        {
            renderer[renderer_index]->setIsosurface(temp_volume, isovalues);
        }
        else
        {
            renderer[renderer_index]->setVolume(temp_volume);
        }

        renderer[renderer_index]->renderToPNGObject(png);
        std::string png_data(png.begin(), png.end());

        /*
         * HTTP-based filter implementation
         *
        std::vector<Async::Promise<Http::Response>> responses;
		Http::Client client;
		auto options = Http::Client::options()
			.threads(10)
			.maxConnectionsPerHost(20);
		client.init(options);

        // apply the filters if any 
        for (auto it = filters.begin(); it != filters.end(); it++)
        {
            std::string filter = *it;

            auto request_builder = client.post("http://accona.eecs.utk.edu:8050/" + filter);
            auto resp = request_builder.body(png_data).send();
            std::cout<<"Request sent"<<std::endl;
            resp.then([&](Http::Response filter_response) {
                auto mime = Http::Mime::MediaType::fromString("image/png");
                response.send(Http::Code::Ok, filter_response.body(), mime);
                std::cout<<"Got response"<<std::endl;
            }, Async::IgnoreException);

			responses.push_back(std::move(resp));
        }
        auto sync = Async::whenAll(responses.begin(), responses.end());
        Async::Barrier<std::vector<Http::Response>> barrier(sync);
        barrier.wait_for(std::chrono::seconds(5));
        client.shutdown();
        */


        for (auto it = filters.begin(); it != filters.end(); it++)
        {
            std::string filter = *it;
            filter = this->app_dir + "/plugins/" + filter;
            std::string filtered_data;
            png_data = exec_filter(filter.c_str(), png_data);
        }

        auto mime = Http::Mime::MediaType::fromString("image/png");
        response.send(Http::Code::Ok, png_data, mime);

    }

}


}
