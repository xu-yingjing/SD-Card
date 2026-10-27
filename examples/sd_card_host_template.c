/*
 * Skeleton of a host port for the sd-card library.
 *
 * It compiles, touches no controller register, and fails every command: what
 * it pins down is the shape of the eight ops entries and the order the library
 * calls them in. Replace the bodies with your SDIO access, and read
 * doc/porting.md before changing any signature.
 */

#include <stddef.h>
#include <stdint.h>
#include "sd_card.h"
#include "sd_card_host.h"

typedef struct {
    /* Everything your controller needs to reach a card: the base address, the
       clock feeding the SDIO block, the frequency currently at the card pins,
       the DMA channel armed by prepare_read(), the destination the read() call
       should land in, and a millisecond counter. */
    uint32_t input_clock_hz;
    uint32_t bus_frequency_hz;
    uint8_t *pending_buffer;
    uint32_t pending_size;
    uint32_t tick_ms;
} example_host_t;

static example_host_t example_host = {
    .input_clock_hz = 48000000U,
    .bus_frequency_hz = 400000U,
    .pending_buffer = NULL,
    .pending_size = 0U,
    .tick_ms = 0U,
};

static example_host_t *example_context(void *context)
{
    return (example_host_t *)context;
}

/* cmd->index is the raw command number (0..63), cmd->response_type says
   whether a response is expected and whether its CRC is meaningful. On OK,
   cmd->response[] must hold it, response[0] being the most significant word
   for a 128 bit response. A command that times out must return an error:
   that is the only way sd_card_probe() learns the socket is empty. */
static sd_card_host_status_t example_send_cmd(void *context, sd_card_host_cmd_t *cmd)
{
    (void)context;
    (void)cmd;

    return SD_CARD_HOST_ERROR;
}

/* Called only after the card itself has been switched with ACMD6, so the host
   is the second half of one operation. If this fails the two sides disagree
   about the data width and the card is unusable until the next reset. */
static sd_card_host_status_t example_set_bus_width(void *context, sd_card_host_bus_width_t width)
{
    (void)context;
    (void)width;

    return SD_CARD_HOST_ERROR;
}

/* bus_frequency_hz is already capped by the card's own TRAN_SPEED, and the
   library never asks for a clock before this point: the sub-400 kHz 1-bit
   clock used for probe is yours to configure. Program your divider here and
   store what the divider actually produces (which may be lower than asked)
   in bus_frequency_hz, or return an error if it cannot reach at or below the
   requested frequency. Never return OK for a faster clock. */
static sd_card_host_status_t example_set_clock_frequency(void *context, uint32_t clock_frequency)
{
    (void)context;
    (void)clock_frequency;

    return SD_CARD_HOST_ERROR;
}

/* Tear down an armed or running data transfer: clear the busy bits, stop the
   DMA, discard any partial block. The library calls it on error paths only,
   and it must also be harmless when nothing is in flight. */
static void example_abort_transfer(void *context)
{
    example_host_t *host = example_context(context);

    host->pending_buffer = NULL;
    host->pending_size = 0U;
}

/* Runs before the read command, so it must not wait for anything: arm the
   length and the destination, then return. size is always a multiple of 512. */
static sd_card_host_status_t example_prepare_read(void *context, uint8_t *buffer, uint32_t size)
{
    example_host_t *host = example_context(context);

    host->pending_buffer = buffer;
    host->pending_size = size;

    return SD_CARD_HOST_OK;
}

/* The only function expected to block. Returns when all size bytes have
   reached the card-side of the bus and into buffer, or with an error after
   your own data timeout; the library calls abort_transfer() next either way.
   On a DMA host this is the wait-on-completion half of prepare_read(). */
static sd_card_host_status_t example_read(void *context, uint8_t *buffer, uint32_t size)
{
    (void)context;
    (void)buffer;
    (void)size;

    return SD_CARD_HOST_ERROR;
}

/* Must not return while the data path still has bytes to push: the library
   follows immediately with CMD12, and a write that reports FIFO acceptance
   instead of card acceptance truncates the end of every multi-block write. */
static sd_card_host_status_t example_write(void *context, const uint8_t *buffer, uint32_t size)
{
    (void)context;
    (void)buffer;
    (void)size;

    return SD_CARD_HOST_ERROR;
}

/* Milliseconds, monotonic, from a SysTick or RTOS tick. The counter is safe to
   wrap: the library only ever subtracts two values in uint32_t. A hook that
   returns a constant is as broken as a missing one, because the timeout loops
   cannot tell that time is standing still. */
static uint32_t example_get_tick_ms(void *context)
{
    example_host_t *host = example_context(context);

    return host->tick_ms++;
}

static const sd_card_host_ops_t example_ops = {
    .send_cmd = example_send_cmd,
    .set_bus_width = example_set_bus_width,
    .set_clock_frequency = example_set_clock_frequency,
    .abort_transfer = example_abort_transfer,
    .prepare_read = example_prepare_read,
    .read = example_read,
    .write = example_write,
    .get_tick_ms = example_get_tick_ms,
};

/* The order a real application needs, in executable form. With the template
   bodies above, every call here fails, which is the expected result. */
sd_card_status_t example_card_up(sd_card_t *card)
{
    sd_card_host_t host;
    sd_card_status_t ret;
    uint8_t sector[512];

    /* Before this point: card powered, pins muxed, host clock at or below
       400 kHz in 1-bit mode. The library does none of that. */
    sd_card_host_init(&host, &example_ops, &example_host);

    ret = sd_card_probe(card, &host);
    if (ret != SD_CARD_OK) {
        return ret;
    }

    ret = sd_card_config_bus_width(card, &host, SD_CARD_BUS_WIDTH_4B);
    if (ret != SD_CARD_OK) {
        return ret;
    }

    ret = sd_card_config_clock_frequency(card, &host, sd_card_get_max_clock_frequency(card));
    if (ret != SD_CARD_OK) {
        return ret;
    }

    /* Block 0 is a safe first read: the MBR signature at offset 510 tells you
       the data path works, not just the command path. On a DMA host this
       buffer has to live where the DMA engine can reach it, which usually
       rules out a plain stack array. */
    return sd_card_read_blocks(card, &host, sector, 0U, 1U);
}
