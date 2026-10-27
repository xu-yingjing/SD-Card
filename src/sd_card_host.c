#include "sd_card_host.h"
#include <stddef.h>

void sd_card_host_init(sd_card_host_t *host, const sd_card_host_ops_t *ops, void *context)
{
    if (host == NULL) {
        return;
    }

    host->ops = ops;
    host->context = context;
}

sd_card_host_status_t sd_card_host_send_cmd(const sd_card_host_t *host, sd_card_host_cmd_t *cmd)
{
    if ((host == NULL) ||
        (host->ops == NULL) ||
        (host->ops->send_cmd == NULL) ||
        (cmd == NULL)) {
        return SD_CARD_HOST_ERROR;
    }

    return host->ops->send_cmd(host->context, cmd);
}

sd_card_host_status_t sd_card_host_set_bus_width(const sd_card_host_t *host, sd_card_host_bus_width_t width)
{
    if ((host == NULL) ||
        (host->ops == NULL) ||
        (host->ops->set_bus_width == NULL)) {
        return SD_CARD_HOST_ERROR;
    }

    return host->ops->set_bus_width(host->context, width);
}

sd_card_host_status_t sd_card_host_set_clock_frequency(const sd_card_host_t *host, uint32_t clock_frequency)
{
    if ((host == NULL) ||
        (host->ops == NULL) ||
        (host->ops->set_clock_frequency == NULL)) {
        return SD_CARD_HOST_ERROR;
    }

    return host->ops->set_clock_frequency(host->context, clock_frequency);
}

void sd_card_host_abort_transfer(const sd_card_host_t *host)
{
    if ((host == NULL) ||
        (host->ops == NULL) ||
        (host->ops->abort_transfer == NULL)) {
        return;
    }

    host->ops->abort_transfer(host->context);
}

sd_card_host_status_t sd_card_host_prepare_read(const sd_card_host_t *host, uint8_t *buffer, uint32_t size)
{
    if ((host == NULL) ||
        (host->ops == NULL) ||
        (host->ops->prepare_read == NULL) ||
        (buffer == NULL) ||
        (size == 0U)) {
        return SD_CARD_HOST_ERROR;
    }

    return host->ops->prepare_read(host->context, buffer, size);
}

sd_card_host_status_t sd_card_host_read(const sd_card_host_t *host, uint8_t *buffer, uint32_t size)
{
    if ((host == NULL) ||
        (host->ops == NULL) ||
        (host->ops->read == NULL) ||
        (buffer == NULL) ||
        (size == 0U)) {
        return SD_CARD_HOST_ERROR;
    }

    return host->ops->read(host->context, buffer, size);
}

sd_card_host_status_t sd_card_host_write(const sd_card_host_t *host, const uint8_t *buffer, uint32_t size)
{
    if ((host == NULL) ||
        (host->ops == NULL) ||
        (host->ops->write == NULL) ||
        (buffer == NULL) ||
        (size == 0U)) {
        return SD_CARD_HOST_ERROR;
    }

    return host->ops->write(host->context, buffer, size);
}

sd_card_host_status_t sd_card_host_get_tick_ms(const sd_card_host_t *host, uint32_t *tick_ms)
{
    if ((host == NULL) ||
        (host->ops == NULL) ||
        (host->ops->get_tick_ms == NULL) ||
        (tick_ms == NULL)) {
        return SD_CARD_HOST_ERROR;
    }

    *tick_ms = host->ops->get_tick_ms(host->context);

    return SD_CARD_HOST_OK;
}
