#include "EnchiladaServer.h"
#include "utils.h"

namespace ench {
    /*
     * MOA::TODO This needs to be smarter. For starters, 
     * it should free the old stuff. Also, it should check
     * and only update a pbnj_container if it needs to. 
     */
    void apply_config(std::string config_name, 
            pbnj::Configuration *config,
            std::map<std::string, 
            ench::pbnj_container>* volume_map)
    {
        pbnj::Camera *camera = new pbnj::Camera(
                config->imageWidth, 
                config->imageHeight);

        // Let's keep a renderer per volume to support time series for now
        pbnj::Renderer **renderer; 
        pbnj::CONFSTATE single_multi = config->getConfigState();
        ench::Dataset dataset;

        // centerView has to be called before setCamera because the light position
        // depends on it. 
        camera->setPosition(config->cameraX, config->cameraY, config->cameraZ);
        camera->centerView();

        /*
         * If we have a single volume at hand
         */
        if (single_multi == pbnj::CONFSTATE::SINGLE_NOVAR 
                || single_multi == pbnj::CONFSTATE::SINGLE_VAR)
        {
            renderer = new pbnj::Renderer*[1];
            renderer[0] = new pbnj::Renderer();
            renderer[0]->setBackgroundColor(config->bgColor);
            renderer[0]->setCamera(camera);
            renderer[0]->setSamples(config->samples);

            pbnj::CONFTYPE subjectType = config->getConfigType(config->dataFilename);
            if (subjectType == pbnj::CONFTYPE::PBNJ_VOLUME)
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
                renderer[0]->addSubject(dataset.volume);
            }
            else if (subjectType == pbnj::CONFTYPE::PBNJ_STREAMLINES)
            {
                camera->setUpVector(config->cameraUpX, config->cameraUpY, config->cameraUpZ);
                camera->centerView();
                dataset.streamlines = new pbnj::Streamlines(config->dataFilename, config->streamlinesRadius);
                renderer[0]->addSubject(dataset.streamlines);
            }
            else if (subjectType == pbnj::CONFTYPE::PBNJ_PARTICLES)
            {
                camera->setView(config->cameraViewX, config->cameraViewY, config->cameraViewZ);
                camera->setUpVector(config->cameraUpX, config->cameraUpY, config->cameraUpZ);
                dataset.particles = new pbnj::Particles(config->dataFilename, true, true, config->particleRadius);
                renderer[0]->addSubject(dataset.particles);
            }
            else
            {
                std::cerr << "Don't know how to do this type yet!" << std::endl;
            }
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
                renderer[i]->addSubject(dataset.timeseries->getVolume(i));
                renderer[i]->setBackgroundColor(config->bgColor);
                renderer[i]->setCamera(camera);
                renderer[i]->setSamples(config->samples);
            }
        }
        else
        {
            std::cerr<<"Cannot open this type of PBNJ file: "<<config_name;
            return;
        }

        (*volume_map)[config_name] = std::make_tuple(config, 
                dataset, camera, renderer);
    }

    pid_t pcreate(int fds[2], const char* cmd)
    {
        pid_t pid;
        int pipes[4];

        pipe(&pipes[0]); // parent read, child write
        pipe(&pipes[2]); // child read, parent write

        if ((pid = fork()) > 0)
        {
            // parent process
            fds[0] = pipes[0];
            fds[1] = pipes[3];

            if (close(pipes[1]) < 0)
            {
                std::cerr<<"Error while closing pipes[1]\n";
            }

            if (close(pipes[2]) < 0)
            {
                std::cerr<<"Error while closing pipes[2]\n";
            }

            return pid;
        }
        else if (pid == 0)
        {
            // child process
            // close the other two descriptors
            if (close(pipes[0]) < 0)
            {
                std::cerr<<"Error while closing pipes[0]\n";
            }

            if (close(pipes[3]) < 0)
            {
                std::cerr<<"Error while closing pipes[3]\n";    
            }

            dup2(pipes[1], STDOUT_FILENO);
            dup2(pipes[2], STDIN_FILENO);

            // run an external plugin and give it the image 
            // no args added yet
            if (execvp(cmd, NULL) < 0)
            {
                std::cerr<<"Could not run external program "<<errno<<std::endl;
            }

            exit(0); // exit from the child
        }
        std::cerr<<"Could not fork child process\n";
        return -1;
    } 

    std::string exec_filter(const char* cmd, std::string request, 
            std::string data) 
    {
        int fds[2];
        pid_t pid = pcreate(fds, cmd);

        long unsigned int data_size = data.length();
        int max_req_length = 2000;
        if (write(fds[1], request.c_str(), max_req_length) < 0)
        {
            std::cerr<<"Error when writing request to child\n"; 
        }
        if (write(fds[1], data.c_str(), data.length()) < 0) // write the data itself
        {
            std::cerr<<"Error when writing to child\n";
        }
        close(fds[1]);

        std::string filtered_data = "";
        char temp;
        int err; 
        while(err = read(fds[0], &temp, sizeof(char)) > 0)
        {
            filtered_data += temp; 
        }

        if (err < 0)
        {
            std::cerr<<"Error when reading data from child\n";
        }

        return filtered_data;
    }

    std::string exec(const char* cmd) 
    {
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
}
