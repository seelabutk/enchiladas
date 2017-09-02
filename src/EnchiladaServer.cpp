#include "EnchiladaServer.h"

#include <iostream>
#include <cstdlib>
#include <string>
#include <memory>
#include <stdexcept>

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

std::string exec(const char* cmd) {
    std::array<char, 128> buffer;
    std::string result;
    std::shared_ptr<FILE> pipe(popen(cmd, "r"), pclose);
    if (!pipe) throw std::runtime_error("popen() failed!");
    while (!feof(pipe.get())) {
        if (fgets(buffer.data(), 128, pipe.get()) != NULL)
            result += buffer.data();
    }
    return result;
}

void EnchiladaServer::init(size_t threads)
{
    auto opts = Pistache::Http::Endpoint::options()
        .threads(threads)
        .flags(Pistache::Tcp::Options::InstallSignalHandler);

    this->httpEndpoint->init(opts);

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
    Routes::Get(router, "/", Routes::bind(&EnchiladaServer::handleRoot, this));
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

void EnchiladaServer::handleRoot(const Rest::Request &request,
        Pistache::Http::ResponseWriter response)
{
    Http::serveFile(response, "index.html");
}

void EnchiladaServer::handleJS(const Rest::Request &request, 
        Pistache::Http::ResponseWriter response)
{
    auto filename = request.param(":filename").as<std::string>();
    filename = "js/" + filename;
    Http::serveFile(response, filename.c_str());
}

void EnchiladaServer::handleCSS(const Rest::Request &request, 
        Pistache::Http::ResponseWriter response)
{
    auto filename = request.param(":filename").as<std::string>();
    filename = "css/" + filename;
    Http::serveFile(response, filename.c_str());
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

    std::string command = "./plugins/" + program + " " + args;
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
    std::string filename = "";
    std::string save_filename;
    std::vector<std::string> filters; // If any image filters are specified, we'll put them here

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
            if (*it == "colormap")
            {
                it++; // Get the value of the colormap
                if (*it == "viridis")
                    udataset.volume->setColorMap(pbnj::viridis);
                else if (*it == "magma")
                    udataset.volume->setColorMap(pbnj::magma);
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
        renderer[renderer_index]->renderImage("data/" + save_filename + ".png");
        response.send(Http::Code::Ok, "saved");
    }
    else
    {
        renderer[renderer_index]->renderToPNGObject(png);
        std::string png_data(png.begin(), png.end());

        std::vector<Async::Promise<Http::Response>> responses;
		Http::Client client;
		auto options = Http::Client::options()
			.threads(1)
			.maxConnectionsPerHost(8);
		client.init(options);

        // apply the filters if any 
        for (auto it = filters.begin(); it != filters.end(); it++)
        {
            std::string filter = *it;


            auto resp = client.get("http://accona.eecs.utk.edu:8050/" + filter).send();
            std::cout<<"wjha"<<std::endl;
            resp.then([&](Http::Response response)
                    {
                        std::cout<<"Got a response"<<std::endl;
                    }, Async::IgnoreException);

			responses.push_back(std::move(resp));
        }
        auto sync = Async::whenAll(responses.begin(), responses.end());
        Async::Barrier<std::vector<Http::Response>> barrier(sync);
        barrier.wait_for(std::chrono::seconds(5));
        client.shutdown();

        auto mime = Http::Mime::MediaType::fromString("image/png");
        response.send(Http::Code::Ok, png_data, mime);
    }

}


}
