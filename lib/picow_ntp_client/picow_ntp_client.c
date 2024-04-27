#include <string.h>
#include <time.h>

#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "pico/util/datetime.h"

#include "hardware/rtc.h"

#include "lwip/dns.h"
#include "lwip/pbuf.h"
#include "lwip/udp.h"

#include "wifi_config.h"
#include "picow_ntp_client.h"

typedef struct NTP_T_ {
    ip_addr_t ntp_server_address;
    bool dns_request_sent;
    struct udp_pcb *ntp_pcb;
    absolute_time_t ntp_test_time;
    alarm_id_t ntp_resend_alarm;
} NTP_T;

const char NTP_SERVER[]         = "pool.ntp.org";
const int NTP_MSG_LEN           = 48;
const int NTP_PORT              = 123;
const uint32_t NTP_DELTA        = 2208988800;   // seconds between 1 Jan 1900 and 1 Jan 1970
const uint32_t NTP_TEST_TIME    = 120 * 1000;   // Minimum interval between NTP requests (ms)
const uint32_t NTP_RESEND_TIME  = 5 * 1000;     // Interval between NTP request reattempts that failed (ms)

static NTP_T *state;

static void ntp_request(NTP_T *state);
static void ntp_result(NTP_T* state, int status, time_t *result);
static int64_t ntp_failed_handler(alarm_id_t id, void *user_data);
static void ntp_dns_found(const char *hostname, const ip_addr_t *ipaddr, void *arg);
static void ntp_recv(void *arg, struct udp_pcb *pcb, struct pbuf *p, const ip_addr_t *addr, u16_t port);
static NTP_T* ntp_init(void);

const time_t TIME_OFFSET = -4 * 60 * 60; // Time offset to use to shift UTC to local time (seconds)

// Called with results of operation
static void ntp_result(NTP_T* state, int status, time_t *result) {
    if (status == 0 && result) {
        *result = *result + TIME_OFFSET;
        struct tm *utc = gmtime(result);
        printf("Received NTP response: %02d/%02d/%04d %02d:%02d:%02d\n", utc->tm_mday, utc->tm_mon + 1, utc->tm_year + 1900,
               utc->tm_hour, utc->tm_min, utc->tm_sec);

        // Update RTC when successful
        datetime_t temp;
        temp.year = utc->tm_year + 1900;
        temp.month = utc->tm_mon + 1;
        temp.day = utc->tm_mday;
        temp.hour = utc->tm_hour;
        temp.min = utc->tm_min;
        temp.sec = utc->tm_sec;
        temp.dotw = utc->tm_wday;

        rtc_set_datetime(&temp);
    }

    if (state->ntp_resend_alarm > 0) {
        cancel_alarm(state->ntp_resend_alarm);
        state->ntp_resend_alarm = 0;
    }
    state->ntp_test_time = make_timeout_time_ms(NTP_TEST_TIME);
    state->dns_request_sent = false;
}

// Make an NTP request
static void ntp_request(NTP_T *state) {
    cyw43_arch_lwip_begin();
    struct pbuf *p = pbuf_alloc(PBUF_TRANSPORT, NTP_MSG_LEN, PBUF_RAM);
    uint8_t *req = (uint8_t *) p->payload;
    memset(req, 0, NTP_MSG_LEN);
    req[0] = 0x1b;
    udp_sendto(state->ntp_pcb, p, &state->ntp_server_address, NTP_PORT);
    pbuf_free(p);
    cyw43_arch_lwip_end();
}

static int64_t ntp_failed_handler(alarm_id_t id, void *user_data) {
    NTP_T* state = (NTP_T*)user_data;
    printf("NTP request failed\n");
    ntp_result(state, -1, NULL);
    return 0;
}

// Call back with a DNS result
static void ntp_dns_found(const char *hostname, const ip_addr_t *ipaddr, void *arg) {
    NTP_T *state = (NTP_T*)arg;
    if (ipaddr) {
        state->ntp_server_address = *ipaddr;
        printf("NTP address %s\n", ipaddr_ntoa(ipaddr));
        ntp_request(state);
    } else {
        printf("NTP DNS request failed\n");
        ntp_result(state, -1, NULL);
    }
}

// NTP data received
static void ntp_recv(void *arg, struct udp_pcb *pcb, struct pbuf *p, const ip_addr_t *addr, u16_t port) {
    NTP_T *state = (NTP_T*)arg;
    uint8_t mode = pbuf_get_at(p, 0) & 0x7;
    uint8_t stratum = pbuf_get_at(p, 1);

    // Check the result
    if (ip_addr_cmp(addr, &state->ntp_server_address) && port == NTP_PORT && p->tot_len == NTP_MSG_LEN &&
        mode == 0x4 && stratum != 0) {
        uint8_t seconds_buf[4] = {0};
        pbuf_copy_partial(p, seconds_buf, sizeof(seconds_buf), 40);
        uint32_t seconds_since_1900 = seconds_buf[0] << 24 | seconds_buf[1] << 16 | seconds_buf[2] << 8 | seconds_buf[3];
        uint32_t seconds_since_1970 = seconds_since_1900 - NTP_DELTA;
        time_t epoch = seconds_since_1970;
        ntp_result(state, 0, &epoch);
    } else {
        printf("Invalid NTP response\n");
        ntp_result(state, -1, NULL);
    }
    pbuf_free(p);
}

// Perform initialisation
static NTP_T* ntp_init(void) {
    rtc_init();

    NTP_T *state = (NTP_T*)calloc(1, sizeof(NTP_T));
    if (!state) {
        printf("Failed to allocate state\n");
        return NULL;
    }
    state->ntp_pcb = udp_new_ip_type(IPADDR_TYPE_ANY);
    if (!state->ntp_pcb) {
        printf("Failed to create pcb\n");
        free(state);
        return NULL;
    }
    udp_recv(state->ntp_pcb, ntp_recv, state);
    return state;
}

/**
 * \brief Checks the time over NTP regularly
 * 
 * \note Had an internal timeout check to pace updates regardless of the frequency this is called
 */
void checkNTP(void) {
    // Return if it is not time for a time check or waiting for a DNS request
    if (absolute_time_diff_us(get_absolute_time(), state->ntp_test_time) > 0 || state->dns_request_sent) return;

    // Set alarm in case UDP requests are lost
    state->ntp_resend_alarm = add_alarm_in_ms(NTP_RESEND_TIME, ntp_failed_handler, state, true);

    cyw43_arch_lwip_begin();
    int err = dns_gethostbyname(NTP_SERVER, &state->ntp_server_address, ntp_dns_found, state);
    cyw43_arch_lwip_end();

    state->dns_request_sent = true;
    if (err == ERR_OK) {
        ntp_request(state); // Cached result
    } else if (err != ERR_INPROGRESS) { // ERR_INPROGRESS means expect a callback
        printf("DNS request failed\n");
        ntp_result(state, -1, NULL);
    }
}

/**
 * \brief Start NTP service
 * 
 * \note Assumes it is the only WiFi service so it also starts the WiFi
 * 
 * \return Non-zero if an error occured
 */
int setupNTP(void) {
    if (cyw43_arch_init()) {
        printf("Failed to initialise WiFi.\n");
        return 1;
    }

    cyw43_arch_enable_sta_mode();

    if (cyw43_arch_wifi_connect_timeout_ms(SSID_NAME, WIFI_PASSWORD, CYW43_AUTH_WPA2_AES_PSK, 10000)) {
        printf("Failed to connect to network. Check network settings.\n");
        return 1;
    }

    printf("Successfully connected to the network.\n");

    state = ntp_init();
    if (!state) return 1;

    return 0;
}

/**
 * \brief Ends NTP service
 * 
 * \note Assumes it is the only WiFi service so it also terminates the WiFi
 * 
 * \return Non-zero if an error occured
 */
int closeNTP() {
    free(state);
    cyw43_arch_deinit();
    return 0;
}