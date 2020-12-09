// wifi.c
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

#include <wifi.h>

#include <esp_err.h>
#include <esp_event.h>
#include <esp_log.h>
#include <esp_netif.h>
#include <esp_wifi.h>
#include <freertos/task.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <util.h>

#include <warnings.h>

// --- Types and constants -----------------------------------------------------

typedef enum {
    SCAN_PENDING,
    SCAN_FAILED,
    SCAN_DONE
} scan_state_t;

#define N_SCAN_ATTEMPTS 3

#define SSID "No Noise 3000"
#define PASSWORD "no-noise"

// --- Macros and inline functions ---------------------------------------------

#define wait_for(flag)                        \
    while (!(flag)) {                         \
        vTaskDelay(500 / portTICK_PERIOD_MS); \
    }

// --- Globals -----------------------------------------------------------------

static volatile bool g_up, g_down, g_join, g_leave;
static volatile scan_state_t g_scan_state;

static esp_netif_t *g_station_if, *g_network_if;

// --- Helper declarations -----------------------------------------------------

static bool try_init(void);
static bool start(void);
static void stop(void);
static void event_handler(void *arg, esp_event_base_t base, int32_t event,
        void *data);
static void scan_event(wifi_event_sta_scan_done_t *data);
static void station_up_event(void);
static void station_down_event(void);
static void station_join_event(wifi_event_sta_connected_t *data);
static void station_leave_event(wifi_event_sta_disconnected_t *data);
static void network_up_event(void);
static void network_down_event(void);
static void network_join_event(wifi_event_ap_staconnected_t *data);
static void network_leave_event(wifi_event_ap_stadisconnected_t *data);
static bool run_scan(void);
static bool network_exists(void);
static bool connect(void);
static bool create_network(void);
static bool assign_addr(wifi_interface_t wifi_if, esp_netif_t *net_if);
static uint32_t mac_to_ip(const uint8_t *mac);

// --- API ---------------------------------------------------------------------

void wifi_init(void)
{
    util_never_fails(esp_netif_init);

    g_station_if = esp_netif_create_default_wifi_sta();
    g_network_if = esp_netif_create_default_wifi_ap();

    while (!try_init()) {
        vTaskDelay(2000 / portTICK_PERIOD_MS);
    }
}

// --- Helpers -----------------------------------------------------------------

static bool try_init(void)
{
    wifi_init_config_t conf = WIFI_INIT_CONFIG_DEFAULT();

    if (util_failed(esp_wifi_init, &conf)) {
        goto done_0;
    }

    if (util_failed(esp_event_handler_register, WIFI_EVENT, ESP_EVENT_ANY_ID,
                event_handler, NULL)) {
        goto done_1;
    }

    if (util_failed(esp_wifi_set_mode, WIFI_MODE_STA) || !start()) {
        goto done_2;
    }

    bool found;

    for (uint32_t i = 0; i < N_SCAN_ATTEMPTS; ++i) {
        if (!run_scan()) {
            stop();
            goto done_2;
        }

        found = network_exists();

        if (found) {
            break;
        }

        vTaskDelay(2000 / portTICK_PERIOD_MS);
    }

    stop();

    bool ok = found ? connect() : create_network();

    if (!ok) {
        goto done_2;
    }

    return true;

done_2:
    util_never_fails(esp_event_handler_unregister, WIFI_EVENT,
            ESP_EVENT_ANY_ID, event_handler);

done_1:
    util_never_fails(esp_wifi_deinit);

done_0:
    return false;
}

static bool start(void)
{
    g_up = false;

    if (util_failed(esp_wifi_start)) {
        return false;
    }

    wait_for(g_up);
    return true;
}

static void stop(void)
{
    g_down = false;
    util_never_fails(esp_wifi_stop);
    wait_for(g_down);
}

static void event_handler(void *arg, esp_event_base_t base, int32_t event,
        void *data)
{
    assert(arg == NULL);
    assert(base == WIFI_EVENT);

    switch (event) {
    case WIFI_EVENT_SCAN_DONE:
        scan_event((wifi_event_sta_scan_done_t *)data);
        break;

    case WIFI_EVENT_STA_START:
        station_up_event();
        break;

    case WIFI_EVENT_STA_STOP:
        station_down_event();
        break;

    case WIFI_EVENT_STA_CONNECTED:
        station_join_event((wifi_event_sta_connected_t *)data);
        break;

    case WIFI_EVENT_STA_DISCONNECTED:
        station_leave_event((wifi_event_sta_disconnected_t *)data);
        break;

    case WIFI_EVENT_AP_START:
        network_up_event();
        break;

    case WIFI_EVENT_AP_STOP:
        network_down_event();
        break;

    case WIFI_EVENT_AP_STACONNECTED:
        network_join_event((wifi_event_ap_staconnected_t *)data);
        break;

    case WIFI_EVENT_AP_STADISCONNECTED:
        network_leave_event((wifi_event_ap_stadisconnected_t *)data);
        break;

    default:
        break;
    }
}

static void scan_event(wifi_event_sta_scan_done_t *data)
{
    if (data->status != 0) {
        ESP_LOGE("NN", "scan failed: %u", data->status);
        g_scan_state = SCAN_FAILED;
    }
    else {
        ESP_LOGI("NN", "found %d network(s)", data->number);
        g_scan_state = SCAN_DONE;
    }
}

static void station_up_event(void)
{
    ESP_LOGI("NN", "station up");
    g_up = true;
}

static void station_down_event(void)
{
    ESP_LOGI("NN", "station down");
    g_down = true;
}

static void station_join_event(wifi_event_sta_connected_t *data)
{
    ESP_LOGI("NN", "joined network %02x:%02x:%02x:%02x:%02x:%02x",
            data->bssid[0], data->bssid[1], data->bssid[2], data->bssid[3],
            data->bssid[4], data->bssid[5]);

    g_join = true;
}

static void station_leave_event(wifi_event_sta_disconnected_t *data)
{
    ESP_LOGI("NN", "left network %02x:%02x:%02x:%02x:%02x:%02x",
            data->bssid[0], data->bssid[1], data->bssid[2], data->bssid[3],
            data->bssid[4], data->bssid[5]);

    g_leave = true;
}

static void network_up_event(void)
{
    ESP_LOGI("NN", "network up");
    g_up = true;
}

static void network_down_event(void)
{
    ESP_LOGI("NN", "network down");
    g_down = true;
}

static void network_join_event(wifi_event_ap_staconnected_t *data)
{
    ESP_LOGI("NN", "node %02x:%02x:%02x:%02x:%02x:%02x joined",
            data->mac[0], data->mac[1], data->mac[2], data->mac[3],
            data->mac[4], data->mac[5]);
}

static void network_leave_event(wifi_event_ap_stadisconnected_t *data)
{
    ESP_LOGI("NN", "node %02x:%02x:%02x:%02x:%02x:%02x left",
            data->mac[0], data->mac[1], data->mac[2], data->mac[3],
            data->mac[4], data->mac[5]);
}

static bool run_scan(void)
{
    wifi_scan_config_t scan_conf = {
        .ssid = NULL,
        .bssid = NULL,
        .channel = 0,
        .show_hidden = false,
        .scan_type = WIFI_SCAN_TYPE_ACTIVE,
        .scan_time = {
            .active = { .min = 0, .max = 0 },
            .passive = 0
        }
    };

    ESP_LOGI("NN", "scanning...");
    g_scan_state = SCAN_PENDING;

    if (util_failed(esp_wifi_scan_start, &scan_conf, false)) {
        return false;
    }

    while (g_scan_state == SCAN_PENDING) {
        vTaskDelay(500 / portTICK_PERIOD_MS);
    }

    return g_scan_state == SCAN_DONE;
}

static bool network_exists(void)
{
    uint16_t n_aps;
    util_never_fails(esp_wifi_scan_get_ap_num, &n_aps);

    wifi_ap_record_t aps[n_aps];
    util_never_fails(esp_wifi_scan_get_ap_records, &n_aps, aps);

    bool found = false;

    for (uint32_t i = 0; i < n_aps; ++i) {
        wifi_ap_record_t *ap = aps + i;
        const char *marker_l, *marker_r;

        if (strcmp((char *)ap->ssid, SSID) == 0) {
            marker_l = ">> ";
            marker_r = " <<";
            found = true;
        }
        else {
            marker_l = marker_r = "   ";
        }

        ESP_LOGI("NN", "%s%3u %32s %4d dBm%s", marker_l, i + 1, ap->ssid,
                ap->rssi, marker_r);
    }

    return found;
}

static bool connect(void)
{
    wifi_config_t conf = {
        .sta = {
            .ssid = SSID,
            .password = PASSWORD,
            .scan_method = WIFI_FAST_SCAN,
            .bssid_set = false,
            .bssid = { 0 },
            .channel = 0,
            .listen_interval = 0,
            .sort_method = WIFI_CONNECT_AP_BY_SIGNAL,
            .threshold = {
                .rssi = 0,
                .authmode = WIFI_AUTH_OPEN
            },
            .pmf_cfg = {
                .capable = false,
                .required = false
            }
        }
    };

    if (util_failed(esp_wifi_set_mode, WIFI_MODE_STA) ||
            util_failed(esp_wifi_set_config, WIFI_IF_STA, &conf) ||
            !start()) {
        return false;
    }

    util_never_fails(esp_netif_dhcpc_stop, g_station_if);

    if (!assign_addr(WIFI_IF_STA, g_station_if)) {
        stop();
        return false;
    }

    if (util_failed(esp_wifi_connect)) {
        stop();
        return false;
    }

    wait_for(g_join || g_leave);

    if (!g_join) {
        stop();
        return false;
    }

    return true;
}

static bool create_network(void)
{
    wifi_config_t conf = {
        .ap = {
            .ssid = SSID,
            .password = PASSWORD,
            .ssid_len = strlen(SSID),
            .channel = 0,
            .authmode = WIFI_AUTH_WPA_WPA2_PSK,
            .ssid_hidden = 0,
            .max_connection = 10,
            .beacon_interval = 100
        }
    };

    if (util_failed(esp_wifi_set_mode, WIFI_MODE_AP) ||
            util_failed(esp_wifi_set_config, WIFI_IF_AP, &conf) ||
            !start()) {
        return false;
    }

    util_never_fails(esp_netif_dhcps_stop, g_network_if);

    if (!assign_addr(WIFI_IF_AP, g_network_if)) {
        stop();
        return false;
    }

    return true;
}

static bool assign_addr(wifi_interface_t wifi_if, esp_netif_t *net_if)
{
    uint8_t mac[6];
    util_never_fails(esp_wifi_get_mac, wifi_if, mac);

    // esp_netif_htonl() evaluates its argument four times
    uint32_t tmp = mac_to_ip(mac);

    esp_netif_ip_info_t ip_info = {
        .ip      = { .addr = esp_netif_htonl(tmp) },
        .netmask = { .addr = esp_netif_htonl(0xff000000u) },
        .gw      = { .addr = esp_netif_htonl(0x00000000u) }
    };

    if (util_failed(esp_netif_set_ip_info, net_if, &ip_info)) {
        return false;
    }

    return true;
}

static uint32_t mac_to_ip(const uint8_t *mac)
{
    uint32_t ip[4];

    ip[0] = 10;
    ip[1] = mac[0] ^ mac[3];
    ip[2] = mac[1] ^ mac[4];
    ip[3] = mac[2] ^ mac[5];

    ESP_LOGI("NN", "MAC %02x:%02x:%02x:%02x:%02x:%02x -> IP %u.%u.%u.%u",
            mac[0], mac[1], mac[2], mac[3], mac[4], mac[5],
            ip[0], ip[1], ip[2], ip[3]);

    return ip[0] << 24 | ip[1] << 16 | ip[2] << 8 | ip[3];
}
