#include "EnchiladaServer.h"
#include "utils.h"

namespace ench {
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

    std::string exec_filter(const char* cmd, std::string data) 
    {
        int fds[2];
        pid_t pid = pcreate(fds, cmd);

        long unsigned int data_size = data.length();
        //write(fds[1], &data_size, sizeof(data_size)); // write the length of the data to the pipe
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
