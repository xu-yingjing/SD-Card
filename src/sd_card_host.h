#ifndef SD_CARD_HOST_H
#define SD_CARD_HOST_H

#include <stdint.h>

typedef enum {
    SD_CARD_HOST_OK = 0U,
    SD_CARD_HOST_ERROR,
} sd_card_host_status_t;

typedef enum {
    SD_CARD_HOST_RESPONSE_NONE = 0U,
    SD_CARD_HOST_RESPONSE_SHORT,
    SD_CARD_HOST_RESPONSE_SHORT_NO_CRC,
    SD_CARD_HOST_RESPONSE_LONG,
    SD_CARD_HOST_RESPONSE_COUNT,
} sd_card_host_response_type_t;

typedef struct {
    uint8_t index;
    uint32_t argument;
    sd_card_host_response_type_t response_type;
    uint32_t response[4];
} sd_card_host_cmd_t;

typedef enum {
    SD_CARD_HOST_BUS_WIDTH_1 = 0U,
    SD_CARD_HOST_BUS_WIDTH_4,
    SD_CARD_HOST_BUS_WIDTH_COUNT,
} sd_card_host_bus_width_t;

typedef struct {
    sd_card_host_status_t (*send_cmd)(void *context, sd_card_host_cmd_t *cmd);
    sd_card_host_status_t (*set_bus_width)(void *context, sd_card_host_bus_width_t width);
    sd_card_host_status_t (*set_clock_frequency)(void *context, uint32_t clock_frequency);
    void (*abort_transfer)(void *context);
    sd_card_host_status_t (*prepare_read)(void *context, uint8_t *buffer, uint32_t size);
    sd_card_host_status_t (*read)(void *context, uint8_t *buffer, uint32_t size);
    sd_card_host_status_t (*write)(void *context, const uint8_t *buffer, uint32_t size);
    uint32_t (*get_tick_ms)(void *context);
} sd_card_host_ops_t;

typedef struct {
    const sd_card_host_ops_t *ops;
    void *context;
} sd_card_host_t;

void sd_card_host_init(sd_card_host_t *host, const sd_card_host_ops_t *ops, void *context);
sd_card_host_status_t sd_card_host_send_cmd(const sd_card_host_t *host, sd_card_host_cmd_t *cmd);
sd_card_host_status_t sd_card_host_set_bus_width(const sd_card_host_t *host, sd_card_host_bus_width_t width);
sd_card_host_status_t sd_card_host_set_clock_frequency(const sd_card_host_t *host, uint32_t clock_frequency);
void sd_card_host_abort_transfer(const sd_card_host_t *host);
sd_card_host_status_t sd_card_host_prepare_read(const sd_card_host_t *host, uint8_t *buffer, uint32_t size);
sd_card_host_status_t sd_card_host_read(const sd_card_host_t *host, uint8_t *buffer, uint32_t size);
sd_card_host_status_t sd_card_host_write(const sd_card_host_t *host, const uint8_t *buffer, uint32_t size);
sd_card_host_status_t sd_card_host_get_tick_ms(const sd_card_host_t *host, uint32_t *tick_ms);

#endif
