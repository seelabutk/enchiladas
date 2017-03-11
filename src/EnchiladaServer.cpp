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

#include "lodepng.h"

namespace ench {

using namespace Net;

EnchiladaServer::EnchiladaServer(Net::Address addr, pbnj::Renderer *r,
        pbnj::Configuration *co, pbnj::Camera *ca) :
    httpEndpoint(std::make_shared<Net::Http::Endpoint>(addr)), renderer(r),
    config(co), camera(ca)
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
    // serving renders
    Routes::Get(router, "/image/:x?/:y/:z/:upx/:upy/:upz/:lowquality",
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

    if (request.hasParam(":x"))
    {
        camera_x = request.param(":x").as<std::int32_t>(); 
        camera_y = request.param(":y").as<std::int32_t>(); 
        camera_z = request.param(":z").as<std::int32_t>(); 

        up_x = request.param(":upx").as<float>();
        up_y = request.param(":upy").as<float>();
        up_z = request.param(":upz").as<float>();

        lowquality = request.param(":lowquality").as<int>();
    }

    unsigned char *image;
    std::vector<unsigned char> png;
    int width, height;

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

    renderer->renderToBuffer(&image, width, height);

    std::vector<unsigned char> image_vector(image, image + 4 * width * height);

    unsigned int error = lodepng::encode(png, image_vector, width, height);
    if (error) 
        std::cout << "encoder error " << error << ": " <<
            lodepng_error_text(error) << std::endl;

    std::string png_data(png.begin(), png.end());
    auto mime = Http::Mime::MediaType::fromString("image/png");
    response.send(Http::Code::Ok, png_data, mime);
}


}
