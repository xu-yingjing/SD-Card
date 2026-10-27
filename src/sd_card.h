#ifndef SD_CARD_H
#define SD_CARD_H

#include <stdint.h>

typedef enum {
    SD_CARD_OK = 0U,
    SD_CARD_ERROR,
} sd_card_status_t;

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
    sd_card_status_t (*send_cmd)(sd_card_host_cmd_t *cmd);
    sd_card_status_t (*set_bus_width)(sd_card_host_bus_width_t width);
    sd_card_status_t (*set_clock_frequency)(uint32_t clock_frequency);
    void (*abort_transfer)(void);
    sd_card_status_t (*prepare_read)(uint8_t *buffer, uint32_t size);
    sd_card_status_t (*read)(uint8_t *buffer, uint32_t size);
    sd_card_status_t (*write)(const uint8_t *buffer, uint32_t size);
    uint32_t (*get_tick_ms)(void);
} sd_card_ops_t;

typedef enum {
    SD_CARD_STATE_NO_CARD = 0U,
    SD_CARD_STATE_READY,
} sd_card_state_t;

typedef enum {
    SD_CARD_BUS_WIDTH_1B = 0U,
    SD_CARD_BUS_WIDTH_4B,
    SD_CARD_BUS_WIDTH_COUNT,
} sd_card_bus_width_t;

enum {
    SD_CARD_CLOCK_FREQUENCY_25000000HZ = 25000000U,
    SD_CARD_CLOCK_FREQUENCY_50000000HZ = 50000000U,
    SD_CARD_CLOCK_FREQUENCY_100000000HZ = 100000000U,
    SD_CARD_CLOCK_FREQUENCY_200000000HZ = 200000000U,
};

typedef struct {
    const sd_card_ops_t *ops;
    sd_card_state_t state;
    uint32_t ocr;
    uint32_t cid[4];
    uint16_t rca;
    uint32_t csd[4];
    uint32_t block_size;
    uint32_t block_count;
    uint32_t max_clock_frequency;
} sd_card_t;

sd_card_status_t sd_card_init(sd_card_t *card,
                              const sd_card_ops_t *ops);

sd_card_status_t sd_card_probe(sd_card_t *card);

sd_card_status_t sd_card_reset(sd_card_t *card);

sd_card_status_t sd_card_config_bus_width(const sd_card_t *card,
                                          sd_card_bus_width_t bus_width);

sd_card_status_t sd_card_config_clock_frequency(const sd_card_t *card,
                                                uint32_t clock_frequency);

sd_card_state_t sd_card_get_state(const sd_card_t *card);

uint32_t sd_card_get_block_size(const sd_card_t *card);

uint32_t sd_card_get_block_count(const sd_card_t *card);

uint32_t sd_card_get_max_clock_frequency(const sd_card_t *card);

sd_card_status_t sd_card_read_blocks(const sd_card_t *card,
                                     uint8_t *buffer,
                                     uint32_t block_start,
                                     uint32_t block_count);

sd_card_status_t sd_card_write_blocks(const sd_card_t *card,
                                      const uint8_t *buffer,
                                      uint32_t block_start,
                                      uint32_t block_count);

#endif
