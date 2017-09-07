
namespace ench {
    pid_t pcreate(int fds[2], const char* cmd);
    std::string exec_filter(const char* cmd, std::string data);
    std::string exec(const char* cmd, std::string data="");
}
