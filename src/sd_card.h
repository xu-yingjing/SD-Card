#ifndef SD_CARD_H
#define SD_CARD_H

#include <stdint.h>
#include "sd_card_host.h"

typedef enum {
    SD_CARD_OK = 0U,
    SD_CARD_ERROR,
    SD_CARD_NO_CARD,
    SD_CARD_UNSUPPORTED,
} sd_card_status_t;

typedef enum {
    SD_CARD_STATE_NO_CARD = 0U,
    SD_CARD_STATE_READY,
} sd_card_state_t;

#define SD_CARD_CLOCK_FREQUENCY_25000000HZ  (25000000U)
#define SD_CARD_CLOCK_FREQUENCY_50000000HZ  (50000000U)
#define SD_CARD_CLOCK_FREQUENCY_100000000HZ (100000000U)
#define SD_CARD_CLOCK_FREQUENCY_200000000HZ (200000000U)

typedef struct {
    sd_card_state_t state;
    uint32_t ocr;
    uint32_t cid[4];
    uint16_t rca;
    uint32_t csd[4];
    uint32_t block_size;
    uint32_t block_count;
    uint32_t max_clock_frequency;
} sd_card_t;

typedef enum {
    SD_CARD_BUS_WIDTH_1B = 0U,
    SD_CARD_BUS_WIDTH_4B,
    SD_CARD_BUS_WIDTH_COUNT,
} sd_card_bus_width_t;

sd_card_status_t sd_card_probe(sd_card_t *card,
                               const sd_card_host_t *host);
sd_card_status_t sd_card_reset(sd_card_t *card);
sd_card_status_t sd_card_config_bus_width(const sd_card_t *card,
                                          const sd_card_host_t *host,
                                          sd_card_bus_width_t bus_width);
sd_card_status_t sd_card_config_clock_frequency(const sd_card_t *card,
                                                const sd_card_host_t *host,
                                                uint32_t clock_frequency);
sd_card_state_t sd_card_get_state(const sd_card_t *card);
uint32_t sd_card_get_block_size(const sd_card_t *card);
uint32_t sd_card_get_block_count(const sd_card_t *card);
uint32_t sd_card_get_max_clock_frequency(const sd_card_t *card);
sd_card_status_t sd_card_read_blocks(const sd_card_t *card,
                                     const sd_card_host_t *host,
                                     uint8_t *buffer,
                                     uint32_t block_start,
                                     uint32_t block_count);
sd_card_status_t sd_card_write_blocks(const sd_card_t *card,
                                      const sd_card_host_t *host,
                                      const uint8_t *buffer,
                                      uint32_t block_start,
                                      uint32_t block_count);

#endif
