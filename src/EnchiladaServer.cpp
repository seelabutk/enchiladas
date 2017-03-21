#include "EnchiladaServer.h"

#include <iostream>
#include <cstdlib>
#include <string>

#include "http.h"
#include "router.h"
#include "endpoint.h"

#include "pbnj.h"
#include "Camera.h"
#include "Configuration.h"
#include "Renderer.h"
#include "TransferFunction.h"

namespace ench {

using namespace Net;

EnchiladaServer::EnchiladaServer(Net::Address addr, std::map<std::string, 
        std::tuple<pbnj::Configuration*, pbnj::Volume*,pbnj::Camera*, 
        pbnj::Renderer*>> vm):  
    httpEndpoint(std::make_shared<Net::Http::Endpoint>(addr)), volume_map(vm)
{
}

void EnchiladaServer::init(size_t threads)
{
    auto opts = Net::Http::Endpoint::options()
        .threads(threads)
        .flags(Net::Tcp::Options::InstallSignalHandler);

    this->httpEndpoint->init(opts);

    setupRoutes();
}

void EnchiladaServer::start()
{
    std::cout << "Listening..." << std::endl;
    this->httpEndpoint->setHandler(router.handler());
    this->httpEndpoint->serve();
}

void EnchiladaServer::shutdown()
{
    std::cout<<"Shutting down."<<std::endl;
    this->httpEndpoint->shutdown();
}

void EnchiladaServer::setupRoutes()
{
    using namespace Net::Rest;
    // serving html files
    Routes::Get(router, "/", Routes::bind(&EnchiladaServer::handleRoot, this));
    // serving static js files
    Routes::Get(router, "/js/:filename", 
            Routes::bind(&EnchiladaServer::handleJS, this));
    // serving static css files
    Routes::Get(router, "/css/:filename",
            Routes::bind(&EnchiladaServer::handleCSS, this));
    // serving renders
    Routes::Get(router, "/image/:dataset/:x/:y/:z/:upx/:upy/:upz/:lowquality/:options?",
            Routes::bind(&EnchiladaServer::handleImage, this));
}

void EnchiladaServer::handleRoot(const Rest::Request &request,
        Net::Http::ResponseWriter response)
{
    Http::serveFile(response, "index.html");
}

void EnchiladaServer::handleJS(const Rest::Request &request, 
        Net::Http::ResponseWriter response)
{
    auto filename = request.param(":filename").as<std::string>();
    filename = "js/" + filename;
    Http::serveFile(response, filename.c_str());
}

void EnchiladaServer::handleCSS(const Rest::Request &request, 
        Net::Http::ResponseWriter response)
{
    auto filename = request.param(":filename").as<std::string>();
    filename = "css/" + filename;
    Http::serveFile(response, filename.c_str());
}

void EnchiladaServer::handleImage(const Rest::Request &request,
        Net::Http::ResponseWriter response)
{

    int camera_x = 0;
    int camera_y = 0;
    int camera_z = 0;

    float up_x = 0;
    float up_y = 1;
    float up_z = 0;

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

        lowquality = request.param(":lowquality").as<int>();
    }

    if (volume_map.count(dataset) == 0)
    {
        response.send(Http::Code::Not_Found, "Image does not exist");
        return;
    }

    pbnj::Configuration *config = std::get<0>(volume_map[dataset]);
    pbnj::Volume *volume = std::get<1>(volume_map[dataset]);
    pbnj::Camera *camera = std::get<2>(volume_map[dataset]);
    pbnj::Renderer *renderer = std::get<3>(volume_map[dataset]);
    //volume->setColorMap(config->colorMap);
    
    if (request.hasParam(":options"))
    {
        /*std::string options_line = request.param(":options").as<std::string>();
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
                    volume->setColorMap(pbnj::viridis);
                else if (*it == "magma")
                    volume->setColorMap(pbnj::magma);
            }
        }*/
        //volume->setColorMap(pbnj::magma);
    }

    std::vector<unsigned char> png;

    if (lowquality)
    {
        renderer->cameraWidth = camera->imageWidth = 64;
        renderer->cameraHeight = camera->imageHeight = 64;
    }
    else
    {
        renderer->cameraWidth = camera->imageWidth = config->imageWidth;
        renderer->cameraHeight = camera->imageHeight = config->imageHeight;
    }

    camera->setPosition(camera_x, camera_y, camera_z);
    camera->setUpVector(up_x, up_y, up_z);

    renderer->renderToPNGObject(png);

    std::string png_data(png.begin(), png.end());
    auto mime = Http::Mime::MediaType::fromString("image/png");
    response.send(Http::Code::Ok, png_data, mime);
}


}
