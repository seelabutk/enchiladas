#ifndef ENCH_SERVER_H
#define ENCH_SERVER_H

#include <enchiladas.h>

#include "pbnj.h"
#include "Renderer.h"
#include "Configuration.h"
#include "Camera.h"
#include "TimeSeries.h"

#include "pistache/http.h"
#include "pistache/router.h"
#include "pistache/endpoint.h"

namespace ench {
    using namespace Pistache;

    union Dataset
    {
        pbnj::Volume* volume;
        pbnj::TimeSeries *timeseries;
    };

    typedef std::tuple<pbnj::Configuration*, ench::Dataset, 
            pbnj::Camera*, pbnj::Renderer**> pbnj_container;

    class EnchiladaServer 
    {
        public:
            EnchiladaServer(Pistache::Address addr, std::map<std::string, pbnj_container> vm);

            void init(std::string app_dir, size_t threads=2);
            void start();
            void shutdown();

        private:

            void setupRoutes();
            void serveFile(Pistache::Http::ResponseWriter &response,
                    std::string filename);
            void handleRoot(const Rest::Request &request,
                    Pistache::Http::ResponseWriter response);
            void handleJS(const Rest::Request &request, 
                    Pistache::Http::ResponseWriter response);
            void handleCSS(const Rest::Request &request, 
                    Pistache::Http::ResponseWriter response);
            void handleImage(const Rest::Request &request,
                    Pistache::Http::ResponseWriter response);
            void handleExternalCommand(const Rest::Request &request,
                    Pistache::Http::ResponseWriter response);
            void handleConfiguration(const Rest::Request &request,
                    Pistache::Http::ResponseWriter response);
            void handleAppData(const Rest::Request &request,
                    Pistache::Http::ResponseWriter response);

            std::shared_ptr<Pistache::Http::Endpoint> httpEndpoint;
            Rest::Router router;
            std::map<std::string, pbnj_container> volume_map;
            std::string app_dir;
    };
}

#endif
