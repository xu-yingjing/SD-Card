# sd-card

SD memory card protocol layer for controllers with an SDIO-style host (command register, response registers, block data path). C99, no OS, no HAL, no dynamic allocation, no writable static data.

The library owns the protocol: the initialisation command sequence (CMD0, CMD8, ACMD41, CMD2, CMD3, CMD9, CMD7), bus-width switching (CMD55/ACMD6), CSD decoding, and single/multi-block read and write with CMD12/CMD13 handling. You own the transport: an eight-function ops table over whatever SDIO controller your part provides, plus pin muxing, power, clock start-up and card detection, which the library never touches.

## Scope

Supported:

- SD initialisation and identification (MMC is not attempted), 1-bit and 4-bit bus, clock up to the ceiling the card declares in its CSD.
- Single and multi-block read (CMD17/CMD18 + CMD12) and write (CMD24/CMD25 + ACMD23 + CMD12), with CMD13 `READY_FOR_DATA` polling before and after a write.
- CSD v2 capacity decoding.

Not supported, by design:

- Standard Capacity cards (CSD v1, up to 2 GB): `sd_card_probe()` rejects them with `SD_CARD_ERROR`, because the decoder's `SD_CARD_UNSUPPORTED` is flattened on the way out ([doc/porting.md](doc/porting.md) maps every failure to its return code). The block count is computed with the SDHC/SDXC formula only.
- Bus widths other than 1 and 4 bit (no 8-bit), DDR, and 1.8 V signalling (ACMD41 is sent with `S18R = 0`). CMD6 `SWITCH_FUNC` is not implemented either, so high-speed timing is never enabled: the ceiling comes from the card's `TRAN_SPEED`, but asking for more than 25 MHz puts the bus outside the specification even though the library permits it.
- Cards whose CSD `TRAN_SPEED` is not one of the four codes the library recognises (25, 50, 100 or 200 MHz): `sd_card_probe()` returns `SD_CARD_ERROR` instead of assuming a speed.
- Voltage ranges outside 2.7-3.6 V (CMD8 and ACMD41 both request that window only).
- CMD16 (block length stays at the power-on value of 512 bytes), so block numbers are used directly as command arguments; valid for SDHC/SDXC.
- Card removal, power cycling, erase (CMD35/CMD38), write protection (CMD28/CMD29), CID decoding, and file systems.

## Requirements

- C99 and `<stdint.h>`, `<stdbool.h>`, `<stddef.h>`. Nothing else: the library includes no CMSIS, device or OS header.
- One caller-owned `sd_card_t` per card (56 bytes: eight fields, `rca` followed by two bytes of padding).
- The block size is fixed at 512 bytes. The library imposes no alignment requirement on your buffer, but your DMA engine probably does.

## Quick start

```c
#include <stddef.h>

#include "sd_card.h"

static sd_card_t card;
static sd_card_host_t host;

static const sd_card_host_ops_t host_ops = {
    .send_cmd            = my_send_cmd,
    .set_bus_width       = my_set_bus_width,
    .set_clock_frequency = my_set_clock_frequency,
    .abort_transfer      = my_abort_transfer,
    .prepare_read        = my_prepare_read,
    .read                = my_read,
    .write               = my_write,
    .get_tick_ms         = my_get_tick_ms,
};

/* Before probing: power the card, mux the pins, and start the host clock at
   no more than 400 kHz in 1-bit mode. The library does not do any of that. */
sd_card_host_init(&host, &host_ops, NULL);

if (sd_card_probe(&card, &host) != SD_CARD_OK) {
    /* No card, or a card this library does not speak. */
}

sd_card_config_bus_width(&card, &host, SD_CARD_BUS_WIDTH_4B);
sd_card_config_clock_frequency(&card, &host, sd_card_get_max_clock_frequency(&card));

uint8_t sector[512];
sd_card_read_blocks(&card, &host, sector, 0U, 1U);
```

`sd_card_reset()` clears the card object after removal; it does not send CMD0 or drop the power, so pair it with your own host de-initialisation if you re-probe after a hot-plug.

## Timing

Every timeout loop in this library is a busy wait with no yield hook, issuing commands back to back: card initialisation runs for up to 1000 ms, and each `READY_FOR_DATA` poll for up to 1000 ms. A write polls twice, once before the data and once after it, so a single `sd_card_write_blocks()` call can spin for 2000 ms. Calling `sd_card_probe()` from a task therefore blocks that priority level for the whole initialisation, and calling it from an interrupt is a bad idea on any system with a shorter tick. Put it in its own task, or drive it from a low-priority context. The library provides no cancellation: if the card never signals busy-complete, the loop runs until the timeout and returns `SD_CARD_ERROR`.

## Porting

See [doc/porting.md](doc/porting.md) for the contract of each ops entry, including the two ordering rules that a naive host implementation gets wrong (`prepare_read` runs before the read command is issued; `read`/`write` return only after the data has reached or left the card), and why `get_tick_ms` is the one entry you cannot leave out. [examples/sd_card_host_template.c](examples/sd_card_host_template.c) is a non-functional skeleton with all eight signatures and the expectations as comments.

## Size

Measured with armclang 6.24 (`-O2`, `--target=arm-arm-none-eabi -mcpu=cortex-m4 -mfpu=fpv4-sp-d16 -mfloat-abi=hard`), reading the `Code`/`RO Data`/`RW Data`/`ZI Data` columns of `fromelf --text -z` on the two objects: 1890 bytes of code and 184 bytes of read-only data, with zero read/write and zero zero-init data. The absence of writable statics is what makes the library reentrant as long as each card has its own `sd_card_t` and `sd_card_host_t`.

## Layout

```
src/        sd_card.[ch] sd_card_host.[ch]   protocol layer, libc only
doc/        porting.md                       host ops contract
examples/   sd_card_host_template.c          annotated skeleton
build.sh    strict-warning compile check, no hardware involved
```

There is no build system: drop `src/` into your project and add the two `.c` files. To check the sources with a host compiler first, run `sh build.sh` (`CC=clang sh build.sh`, `CC=arm-none-eabi-gcc sh build.sh`): it compiles all three `.c` files with `-Werror` on top of `-Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Wshadow -Wundef -Wwrite-strings`, and needs no hardware.

## Status

0.1.0. The code is extracted from an in-tree component of a MH2457 (Cortex-M4) firmware, where it ran against real SDHC cards over a 4-bit SDIO host. No second host port exists yet, which is the main thing standing between this and a 1.0.0. [CHANGELOG.md](CHANGELOG.md) lists the known gaps.

## License

BSD-3-Clause. See [LICENSE](LICENSE).