#include "Ping.h"
#include "Process.h"
#include <algorithm>
#include <arpa/inet.h>
#include <fstream>
#include <iostream>
#include <iterator>
#include <net/if.h>
#include <netinet/in.h>
#include <sstream>
#include <stdio.h>
#include <string.h> /* For strncpy */
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <vector>

std::vector<std::string> tokenize(std::string tmp) {
    std::stringstream line(tmp);
    return {istream_iterator<string>{line}, istream_iterator<string>{}};
}

int main() {
    Process masscan(
        std::string{"masscan --ping 10.0.0.0/16 --rate=1000 --output-format "
                    "list --output-filename data > /dev/null 2>&1"});

    std::fstream             in("data");
    std::string              tmp;
    std::vector<std::string> toping;

    int          fd;
    struct ifreq ifr;

    fd = socket(AF_INET, SOCK_DGRAM, 0);

    /* I want to get an IPv4 IP address */
    ifr.ifr_addr.sa_family = AF_INET;

    /* I want an IP address attached to "eth0" */
    strncpy(ifr.ifr_name, "wlp3s0", IFNAMSIZ - 1);

    ioctl(fd, SIOCGIFADDR, &ifr);

    close(fd);

    /* Display result */
    toping.push_back(
        inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr));

    std::cout << "4c:34:88:a3:e0:12, " << toping[0] << ", 0, " << std::endl;

    while(std::getline(in, tmp)) {
        if(tmp[0] == '#') continue;

        toping.push_back(tokenize(tmp)[3]);
    }

    std::vector<std::vector<std::string>> data;
    data.reserve(toping.size());

    for(auto & address : toping) {
        Process arping(
            "arping -w 1 -I wlp3s0 -c 1 " + address +
            " | sed 's/\\[//g' | sed 's/\\]//g' | sed 's/ms//g' > arping_out");
        std::fstream ping_data("arping_out");
        std::string  tmp;
        std::getline(ping_data, tmp);
        std::getline(ping_data, tmp);

        if(tmp[0] == 'a') continue;
        if(tmp[0] == 'S') continue;

        std::string mac(tokenize(tmp)[4]);
        std::string ip(address);
        std::string ping(tokenize(tmp)[5]);

        Process traceroute("traceroute -w 0.2 " + address +
                           " | sed 's/(//g' | sed 's/)//g' > traceroute_out");
        std::fstream trace_data("traceroute_out");
        std::getline(trace_data, tmp);

        std::vector<std::string> routes;
        while(std::getline(trace_data, tmp)) routes.push_back(tmp);

        bool found = false;

        if(routes.size() == 1) {
            tmp = routes[0];

            // std::cout << tmp << std::endl;

            if(tmp[0] == 'a') continue;
            if(tmp[0] == 'S') continue;

            std::cout << mac << ", " << ip << ", " << ping << ", " << toping[0]
                      << std::endl;
        } else if(routes.size() > 1) {
            for(int i = routes.size() - 2; i > 0; i--) {
                if(tokenize(routes[i]).size() > 1) {
                    if(tokenize(routes[i])[2][0] == '*')
                        continue;
                    else {
                        tmp   = routes[i];
                        found = true;
                        break;
                    }
                }
            }

            if(found) {
                if(tmp[0] == 'a') continue;
                if(tmp[0] == 'S') continue;
                std::cout << mac << ", " << ip << ", " << ping << ", "
                          << tokenize(tmp)[2] << std::endl;
            } else
                continue;
        }

        // if(tokenize(tmp)[2][0] != '*')
        //     std::cout << mac << ", " << ip << ", " << ping << ", "
        //               << tokenize(tmp)[2] << std::endl;

        // std::cout << Ping::ping(address) << std::endl;
    }
}
