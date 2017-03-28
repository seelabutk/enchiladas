#ifndef ENCH_SERVER_H
#define ENCH_SERVER_H

#include <enchiladas.h>

#include "pbnj.h"
#include "Renderer.h"
#include "Configuration.h"
#include "Camera.h"
#include "TimeSeries.h"

#include "http.h"
#include "router.h"
#include "endpoint.h"

namespace ench {
    using namespace Net;

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
            EnchiladaServer(Net::Address addr, std::map<std::string, pbnj_container> vm);

            void init(size_t threads=2);
            void start();
            void shutdown();

        private:

            void setupRoutes();
            void handleRoot(const Rest::Request &request,
                    Net::Http::ResponseWriter response);
            void handleJS(const Rest::Request &request, 
                    Net::Http::ResponseWriter response);
            void handleCSS(const Rest::Request &request, 
                    Net::Http::ResponseWriter response);
            void handleImage(const Rest::Request &request,
                    Net::Http::ResponseWriter response);

            std::shared_ptr<Net::Http::Endpoint> httpEndpoint;
            Rest::Router router;

            std::map<std::string, pbnj_container> volume_map;
    };
}

#endif
