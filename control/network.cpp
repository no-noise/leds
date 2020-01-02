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

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

// --- Types -------------------------------------------------------------------

// --- Constants and macros ----------------------------------------------------

#define NETWORK_ID 10

#define WIFI_SSID "No Noise"
#define WIFI_PASSWORD "Amplifix2000"
#define WIFI_CHANNEL 1

#define STATS_INTERVAL 5000

// --- Globals -----------------------------------------------------------------

static bool g_is_access_point;
static uint32_t g_last_stats;

// --- Helper declarations -----------------------------------------------------

static bool find_network();
static void join_network(const IPAddress &me);
static void create_network(const IPAddress &me);
static void set_mode(wifi_mode_t mode);
static void print_stats();

// --- API ---------------------------------------------------------------------

void network_initialize()
{
    delay(2000);

    uint8_t mac[6];
    WiFi.macAddress(mac);
    Serial.printf("MAC address %02x:%02x:%02x:%02x:%02x:%02x\r\n",
            mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

    IPAddress me(NETWORK_ID, mac[3], mac[4], mac[5]);
    Serial.printf("IP address %d.%d.%d.%d\r\n", me[0], me[1], me[2], me[3]);

    int32_t tries;

    for (tries = 5; tries > 0; --tries) {
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
}

int32_t network_handle_io()
{
    print_stats();

    return -1;
}

// --- Helpers -----------------------------------------------------------------

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

    set_mode(WIFI_MODE_STA);

    IPAddress gw(NETWORK_ID, 0, 0, 0);
    IPAddress mask(255, 0, 0, 0);

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

    set_mode(WIFI_MODE_AP);

    while (!WiFi.softAP(WIFI_SSID, WIFI_PASSWORD, WIFI_CHANNEL, 0, 8)) {
        Serial.println("couldn't start access point");
        delay(1000);
    }

    while ((WiFi.getStatusBits() & AP_STARTED_BIT) == 0) {
        delay(100);
    }

    IPAddress gw(NETWORK_ID, 0, 0, 0);
    IPAddress mask(255, 0, 0, 0);

    while (!WiFi.softAPConfig(me, gw, mask)) {
        Serial.println("couldn't configure access point");
        delay(1000);
    }

    g_is_access_point = true;
}

static void set_mode(wifi_mode_t mode)
{
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
