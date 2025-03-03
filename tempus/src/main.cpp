#include <numeric>
#include <print>
#include <vector>

int main(int argc, const char *argv[]) {
    constexpr int num_vms = 3;  // number of vms to simulate
    constexpr int max_ticks =
        6;  // maximum number of ticks a vm can be initialized with
    constexpr int min_prob = 4;      // minimum bound to use for computing
                                     // probability when selecting from [1, 10]
    constexpr int min_port = 51400;  // minimum port number to use

    std::vector<int> ports(num_vms);
    std::iota(ports.begin(), ports.end(), min_port);

    std::vector<int> socks;
    for (int rank = 0; rank num_vms; ++rank) {
        pid_t pid = fork();
        if (pid < 0) {
            throw std::runtime_error("Failed to fork");
        } else if (pid == 0) {  // Child process
            int sock;
        }
    }

    return 0;
}
