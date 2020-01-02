// network.cpp
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

#include <HardwareSerial.h>
#include <WiFi.h>

#include "warnings.h"
#include "network.h"
#include "shared.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

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

#define NETWORK_ID 10

#define WIFI_SSID "No Noise"
#define WIFI_PASSWORD "Amplifix2000"
#define WIFI_CHANNEL 1

#define SCAN_TRIES 3

#define STATS_INTERVAL 60000

#define UDP_PORT 1972
#define TCP_PORT 1972

#define IO_BUFFER_SZ 5000

#define DELAY_LIMIT 1000

// --- Globals -----------------------------------------------------------------

static bool g_is_access_point;
static uint32_t g_last_stats;

static WiFiUDP wifi_udp;
static WiFiServer wifi_tcp;

static uint8_t io_buffer[IO_BUFFER_SZ];

// --- Helper declarations -----------------------------------------------------

static IPAddress build_ip_address();
static bool find_network();
static void join_network(const IPAddress &me);
static void create_network(const IPAddress &me);
static void open_ports(const IPAddress &me);
static void enter_mode(wifi_mode_t mode);
static void print_stats();
static int32_t handle_udp();
static int32_t handle_ping(const IPAddress &rem_addr, uint16_t rem_port,
        int32_t read_sz);
static int32_t handle_render_frame(const IPAddress &rem_addr, uint16_t rem_port,
        int32_t read_sz);
static void handle_tcp();

// --- API ---------------------------------------------------------------------

void network_initialize()
{
    delay(2000);

    const IPAddress me = build_ip_address();
    int32_t tries;

    for (tries = SCAN_TRIES; tries > 0; --tries) {
        if (find_network()) {
            break;
        }

        delay(2000);
    }

    if (tries > 0) {
        join_network(me);
    } else {
        create_network(me);
    }

    g_last_stats = 0;

    open_ports(me);

    randomSeed((uint32_t)me[1] ^ (uint32_t)me[2] * (uint32_t)me[3]);
}

int32_t network_handle_io()
{
    print_stats();

    int32_t frame_id = handle_udp();

    if (frame_id >= 0) {
        return frame_id;
    }

    handle_tcp();
    return -1;
}

// --- Helpers -----------------------------------------------------------------

static IPAddress build_ip_address()
{
    uint8_t mac[6];
    WiFi.macAddress(mac);

    Serial.printf("MAC address %02x:%02x:%02x:%02x:%02x:%02x\r\n",
            mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

    IPAddress me{NETWORK_ID, mac[3], mac[4], mac[5]};

    Serial.printf("IP address %d.%d.%d.%d\r\n", me[0], me[1], me[2], me[3]);

    return me;
}

static bool find_network()
{
    Serial.println("scanning...");

    int32_t n_networks = WiFi.scanNetworks();
    bool found = false;

    for (int32_t i = 0; i < n_networks; ++i) {
        const String &ssid = WiFi.SSID((uint8_t)i);
        int32_t rssi = WiFi.RSSI((uint8_t)i);

        const char *marker;

        if (ssid == WIFI_SSID) {
            found = true;
            marker = ">>";
        } else {
            marker = "  ";
        }

        Serial.printf("%s %3d %32s %4d dBm\r\n",
                marker, i + 1, ssid.c_str(), rssi);
    }

    return found;
}

static void join_network(const IPAddress &me)
{
    Serial.println("joining WiFi network");

    enter_mode(WIFI_MODE_STA);

    IPAddress gw{NETWORK_ID, 0, 0, 0};
    IPAddress mask{255, 0, 0, 0};

    while (!WiFi.config(me, gw, mask)) {
        Serial.println("couldn't configure station");
        delay(1000);
    }

    wl_status_t stat;

    while ((stat = WiFi.begin(WIFI_SSID, WIFI_PASSWORD)) != WL_CONNECTED) {
        Serial.printf("couldn't start station (%d)\r\n", stat);
        delay(1000);
    }

    g_is_access_point = false;
}

static void create_network(const IPAddress &me)
{
    Serial.println("creating WiFi network");

    enter_mode(WIFI_MODE_AP);

    while (!WiFi.softAP(WIFI_SSID, WIFI_PASSWORD, WIFI_CHANNEL, 0, 8)) {
        Serial.println("couldn't start access point");
        delay(1000);
    }

#if defined ARDUINO_ARCH_ESP32
    // Otherwise the ESP32 won't honor softAPConfig().
    while ((WiFi.getStatusBits() & AP_STARTED_BIT) == 0) {
        delay(100);
    }
#endif

    IPAddress gw{NETWORK_ID, 0, 0, 0};
    IPAddress mask{255, 0, 0, 0};

    while (!WiFi.softAPConfig(me, gw, mask)) {
        Serial.println("couldn't configure access point");
        delay(1000);
    }

    g_is_access_point = true;
}

static void open_ports(const IPAddress &me)
{
    Serial.println("opening ports");

    while (wifi_udp.begin(UDP_PORT) != 1) {
        Serial.println("couldn't open UDP port");
        delay(1000);
    }

    while (true) {
        wifi_tcp.begin(TCP_PORT);

        if (wifi_tcp) {
            break;
        }

        Serial.println("couldn't open TCP port");
        delay(1000);
    }
}

static void enter_mode(wifi_mode_t mode)
{
    Serial.print("entering mode ");
    Serial.println(mode);

    while (!WiFi.disconnect(true, true)) {
        Serial.println("couldn't disconnect station");
        delay(1000);
    }

    while (!WiFi.softAPdisconnect(true)) {
        Serial.println("couldn't disconnect access point");
        delay(1000);
    }

    while (!WiFi.mode(mode)) {
        Serial.println("couldn't set mode");
        delay(1000);
    }
}

static void print_stats()
{
    uint32_t now = millis();

    if (now - g_last_stats >= STATS_INTERVAL) {
        if (g_is_access_point) {
            Serial.println("--- AP info ------------------------------");
            Serial.println(WiFi.softAPIP());
            Serial.println(WiFi.softAPBroadcastIP());
            Serial.printf("%d station(s)\r\n", WiFi.softAPgetStationNum());
        }
        else {
            Serial.println("--- Station info -------------------------");
            Serial.println(WiFi.localIP());
            Serial.println(WiFi.broadcastIP());
            Serial.println(WiFi.dnsIP());
        }

        Serial.println("--- Diagnostics --------------------------");
        WiFi.printDiag(Serial);

        g_last_stats = now;
    }
}

static int32_t handle_udp()
{
    int32_t parse_sz = wifi_udp.parsePacket();

    if (parse_sz == 0) {
        return -1;
    }

    const IPAddress &rem_addr = wifi_udp.remoteIP();
    uint16_t rem_port = wifi_udp.remotePort();

    Serial.printf("UDP %d %d.%d.%d.%d:%d\r\n",
            parse_sz, rem_addr[0], rem_addr[1], rem_addr[2], rem_addr[3],
            rem_port);

    int32_t read_sz = wifi_udp.read(io_buffer, sizeof io_buffer);
    assert(read_sz == parse_sz);

    hex_dump(io_buffer, (size_t)read_sz);

    switch (io_buffer[0]) {
    case COMMAND_PING:
        return handle_ping(rem_addr, rem_port, read_sz);

    case COMMAND_RENDER_FRAME:
        return handle_render_frame(rem_addr, rem_port, read_sz);
    }

    return -1;
}

static int32_t handle_ping(const IPAddress &rem_addr, uint16_t rem_port,
        int32_t read_sz)
{
    Serial.println("ping");

    if (read_sz != 2) {
        Serial.printf("bad ping message size %d\r\n", read_sz);
        return -1;
    }

    delayMicroseconds((uint32_t)random(DELAY_LIMIT));

    uint8_t seq_no = io_buffer[1];

    int32_t res = wifi_udp.beginPacket(rem_addr, rem_port);
    assert(res == 1);

    io_buffer[0] = seq_no;
    io_buffer[1] = (uint8_t)g_is_access_point;

    size_t len = wifi_udp.write(io_buffer, 2);
    assert(len == 2);

    res = wifi_udp.endPacket();
    assert(res == 1);

    return -1;
}

static int32_t handle_render_frame(const IPAddress &rem_addr, uint16_t rem_port,
        int32_t read_sz)
{
    Serial.println("render frame");

    if (read_sz != 4) {
        Serial.printf("bad render frame message size %d\r\n", read_sz);
        return -1;
    }

    int32_t frame_id = (io_buffer[1] << 16) | (io_buffer[2] << 8) |
            io_buffer[3];

    Serial.printf("frame #%d\r\n", frame_id);

    return frame_id;
}

static void handle_tcp()
{
}
