// cli.cpp
//
// Copyright (C) 2020 by the contributors
//
// This program is free software: you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the Free
// Software Foundation, either version 3 of the License, or (at your option)
// any later version.
//
// This program is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
// FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
// more details.
//
// You should have received a copy of the GNU General Public License along with
// this program. If not, see <https://www.gnu.org/licenses/>.

// --- Includes ----------------------------------------------------------------

#include <arpa/inet.h>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <errno.h>
#include <iomanip>
#include <iostream>
#include <map>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <vector>

// --- Types -------------------------------------------------------------------

enum command_t {
    COMMAND_PING,
    COMMAND_UPLOAD,
    COMMAND_PREPARE,
    COMMAND_START,
    COMMAND_STOP,
    COMMAND_RENDER_FRAME
};

enum result_t {
    RESULT_OK,
    RESULT_NOT_MASTER
};

// --- Constants and macros ----------------------------------------------------

#define BROADCAST_IP "10.255.255.255"

#define UDP_PORT 1972

#define PING_COUNT 1000
#define PING_INTERVAL 20
#define PING_TIMEOUT 100

// --- Globals -----------------------------------------------------------------

// --- Helper declarations -----------------------------------------------------

static void usage();
static void run_ping();
static uint64_t get_us();

// --- Main --------------------------------------------------------------------

int main(int argc, char *argv[])
{
    if (argc < 2) {
        usage();
        return 1;
    }

    const std::string command{argv[1]};

    if (command == "ping") {
        run_ping();
        return 0;
    }

    usage();
	return 1;
}

// --- Helpers -----------------------------------------------------------------

static void usage()
{
    std::cerr << "usage: cli ping" << std::endl;
}

static void run_ping()
{
    int32_t sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    assert(sock >= 0);

    static const timeval timeout = {0, PING_TIMEOUT * 1000};
    static const int32_t one = 1;
    int32_t res;

    res = setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof timeout);
    assert(res == 0);

    res = setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &one, sizeof one);
    assert(res == 0);

    sockaddr_in out_addr;

    out_addr.sin_family = AF_INET;
    out_addr.sin_port = htons(UDP_PORT);
    out_addr.sin_addr.s_addr = inet_addr(BROADCAST_IP);

    int32_t count;
    uint64_t out_us, now_us, delay_us, rtt_us;
    uint8_t out_buffer[2], in_buffer[2];
    ssize_t len;

    std::map<std::string, std::vector<uint32_t>> rtt_map;

    for (count = 0; count < PING_COUNT; ++count) {
        if (count % 100 == 0) {
            std::cout << count << std::endl;
        }

        out_buffer[0] = COMMAND_PING;
        out_buffer[1] = (uint8_t)count;

        out_us = get_us();

        len = sendto(sock, out_buffer, sizeof out_buffer, MSG_NOSIGNAL,
                (sockaddr *)&out_addr, sizeof out_addr);

        assert(len >= 0 && (size_t)len == sizeof out_buffer);

        while (true) {
            sockaddr_storage in_addr;
            socklen_t in_addr_len = sizeof in_addr;

            len = recvfrom(sock, in_buffer, sizeof in_buffer, 0,
                    (sockaddr *)&in_addr, &in_addr_len);

            if (len < 0) {
                assert(errno == EAGAIN);
                break;
            }

            assert(len == 2);
            assert(in_addr_len == sizeof (sockaddr_in));

            if (in_buffer[0] != (uint8_t)count) {
                continue;
            }

            rtt_us = get_us() - out_us;

            const sockaddr_in *in_addr_v4 = (sockaddr_in *)&in_addr;
            const std::string str(inet_ntoa(in_addr_v4->sin_addr));

            rtt_map[str].push_back((uint32_t)rtt_us);
        }

        now_us = get_us();
        delay_us = out_us + PING_INTERVAL;

        if (now_us < delay_us) {
            usleep((uint32_t)(delay_us - now_us));
        }
    }

    close(sock);

    std::cout << "        address    min    avg    max    std    #" << std::endl;
    std::cout << "------------------------------------------------" << std::endl;

    for (auto kv: rtt_map) {
        const std::string &addr = kv.first;
        const std::vector<uint32_t> &rtts = kv.second;

        double min = 999999999.0;
        double max = 0.0;
        double sum = 0.0;
        double avg, var, std;

        for (auto rtt: rtts) {
            if (rtt > max) {
                max = rtt;
            }

            if (rtt < min) {
                min = rtt;
            }

            sum += rtt;
        }

        min /= 1000.0;
        max /= 1000.0;
        sum /= 1000.0;

        avg = sum / (double)rtts.size();
        sum = 0.0;

        for (auto rtt: rtts) {
            double diff = rtt / 1000.0 - avg;
            sum += diff * diff;
        }

        var = sum / (double)rtts.size();
        std = std::sqrt(var);

        std::cout <<
                std::setw(15) << addr << " " <<
                std::setw(6) << std::setprecision(5) << min << " " <<
                std::setw(6) << std::setprecision(5) << avg << " " <<
                std::setw(6) << std::setprecision(5) << max << " " <<
                std::setw(6) << std::setprecision(5) << std << " " <<
                std::setw(4) << rtts.size() << std::endl;
    }
}

static uint64_t get_us()
{
    timeval now;

    int32_t res = gettimeofday(&now, NULL);
    assert(res == 0);

    return (uint64_t)now.tv_sec * 1000000 + (uint64_t)now.tv_usec;
}
