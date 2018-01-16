
namespace ench {
    pid_t pcreate(int fds[2], const char* cmd);
    std::string exec_filter(const char* cmd, std::string data);
    std::string exec(const char* cmd);
    void apply_config(std::string config_name, 
            pbnj::Configuration *config,
            std::map<std::string, 
            ench::pbnj_container>* volume_map);
}
