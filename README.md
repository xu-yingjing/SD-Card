# sd-card

SD memory card protocol layer for controllers with an SDIO-style host (command register, response registers, block data path). C99, no OS, no HAL, no dynamic allocation, no writable static data.

The library owns the protocol: the initialisation command sequence (CMD0, CMD8, ACMD41, CMD2, CMD3, CMD9, CMD7), bus-width switching (CMD55/ACMD6), high-speed function selection (CMD6), CSD decoding, and single/multi-block read and write with CMD12/CMD13 handling. You own the transport: an eight-function ops table over whatever SDIO controller your part provides, bound to the card once with `sd_card_init()`, plus pin muxing, power, clock start-up and card detection, which the library never touches.

## Scope

Supported:

- SD initialisation and identification (MMC is not attempted), 1-bit and 4-bit bus, clock up to the ceiling the card declares in its CSD, or up to 50 MHz once `sd_card_config_high_speed()` has selected the high-speed function.
- Single and multi-block read (CMD17/CMD18 + CMD12) and write (CMD24/CMD25 + ACMD23 + CMD12), with CMD13 `READY_FOR_DATA` polling before and after a write.
- CSD v2 capacity decoding.

Not supported, by design:

- Standard Capacity cards (CSD v1, up to 2 GB): `sd_card_probe()` rejects them with `SD_CARD_ERROR`; the status enum has no separate code for an unsupported card ([doc/porting.md](doc/porting.md) maps every failure to its return code). The block count is computed with the SDHC/SDXC formula only.
- Bus widths other than 1 and 4 bit (no 8-bit), DDR, and 1.8 V signalling (ACMD41 is sent with `S18R = 0`). CMD6 selects only the high-speed function, never a UHS one, so 50 MHz is as fast as this library asks; the higher `TRAN_SPEED` bands belong to signalling it does not enter.
- Cards whose CSD `TRAN_SPEED` is not one of the four codes the library recognises (25, 50, 100 or 200 MHz): `sd_card_probe()` returns `SD_CARD_ERROR` instead of assuming a speed.
- Voltage ranges outside 2.7-3.6 V (CMD8 and ACMD41 both request that window only).
- CMD16 (block length stays at the power-on value of 512 bytes), so block numbers are used directly as command arguments; valid for SDHC/SDXC.
- Card removal, power cycling, erase (CMD35/CMD38), write protection (CMD28/CMD29), CID decoding, and file systems.

## Requirements

- C99 and `<stdint.h>`, `<stdbool.h>`, `<stddef.h>`, `<string.h>`. Nothing else: the library includes no CMSIS, device or OS header. Only `sd_card_config_high_speed()` reaches for the C library, to clear the 64-byte switch status on the stack before each CMD6 exchange.
- One caller-owned `sd_card_t` per card (60 bytes on a 32-bit target: nine fields, the `uint16_t rca` followed by two bytes of padding).
- The block size is fixed at 512 bytes. The library imposes no alignment requirement on your buffer, but your DMA engine probably does.

## Quick start

```c
#include <stddef.h>

#include "sd_card.h"

static sd_card_t card;

static const sd_card_ops_t host_ops = {
    .host_send_cmd            = my_send_cmd,
    .host_set_bus_width       = my_set_bus_width,
    .host_set_clock_frequency = my_set_clock_frequency,
    .host_abort_transfer      = my_abort_transfer,
    .host_prepare_read        = my_prepare_read,
    .host_read                = my_read,
    .host_write               = my_write,
    .get_tick_ms              = my_get_tick_ms,
};

/* Before probing: power the card, mux the pins, and start the host clock at
   no more than 400 kHz in 1-bit mode. The library does not do any of that. */
if (sd_card_init(&card, &host_ops) != SD_CARD_OK) {
    /* A NULL entry in the table, or a NULL card pointer. */
}

if (sd_card_probe(&card) != SD_CARD_OK) {
    /* No card, or a card this library does not speak. */
}

sd_card_config_bus_width(&card, SD_CARD_BUS_WIDTH_4B);

/* Optional. A card that has no high-speed function, or refuses the switch,
   leaves max_clock_frequency at its CSD value and stays on the slower clock. */
(void)sd_card_config_high_speed(&card);

sd_card_config_clock_frequency(&card, sd_card_get_max_clock_frequency(&card));

uint8_t sector[512];
sd_card_read_blocks(&card, sector, 0U, 1U);
```

`sd_card_init()` is called once; every later call takes only the card object, because the ops table is stored in it. `sd_card_reset()` clears the card state after removal but keeps the binding and does not send CMD0 or drop the power, so pair it with your own host de-initialisation if you re-probe after a hot-plug.

## Timing

Every timeout loop in this library is a busy wait with no yield hook, issuing commands back to back: card initialisation runs for up to 1000 ms, and each `READY_FOR_DATA` poll for up to 1000 ms. A write polls twice, once before the data and once after it, so a single `sd_card_write_blocks()` call can spin for 2000 ms. Calling `sd_card_probe()` from a task therefore blocks that priority level for the whole initialisation, and calling it from an interrupt is a bad idea on any system with a shorter tick. Put it in its own task, or drive it from a low-priority context. The library provides no cancellation: if the card never signals busy-complete, the loop runs until the timeout and returns `SD_CARD_ERROR`.

## Porting

See [doc/porting.md](doc/porting.md) for the contract of each ops entry, including the two ordering rules that a naive host implementation gets wrong (`host_prepare_read` runs before the read command is issued; `host_read`/`host_write` return only after the data has reached or left the card), and the trap in `get_tick_ms` that a port is most likely to fall into. [examples/sd_card_host_template.c](examples/sd_card_host_template.c) is a non-functional skeleton with all eight signatures and the expectations as comments.

## Size

Measured with GNU arm-none-eabi-gcc 12.3 (`-std=c99 -O2 -mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard`), reading `arm-none-eabi-size -A` on `sd_card.o`: 1688 bytes of `.text`, zero `.data`, zero `.bss`, and no `.rodata` section at all. The 252 bytes over 0.2.1 are the two CMD6 exchanges and the status decode; the 1436 of 0.2.1 was re-measured from the `v0.2.1` tag with these same flags rather than quoted from its note, and neither build emits a `.rodata`, so the `memset()` length costs nothing in read-only data. The 0.1.0 figures (1890 bytes of code and 184 of read-only data, armclang 6.24, two objects) were measured before the refactor, included the now-deleted `sd_card_host.c`, and do not carry over.

The absence of writable statics is structural: all state lives in the caller-owned `sd_card_t`, and the bound ops table is caller-owned `const` data. The library itself is therefore reentrant per card, but the table carries no context pointer, so a port that keeps its host state in file scope serves one card at a time.

## Layout

```
src/        sd_card.[ch]                     protocol layer, libc only
doc/        porting.md                       host ops contract
examples/   sd_card_host_template.c          annotated skeleton
build.sh    strict-warning compile check, no hardware involved
```

There is no build system: drop `src/` into your project and add `sd_card.c`. To check the sources with a host compiler first, run `sh build.sh` (`CC=clang sh build.sh`, `CC=arm-none-eabi-gcc sh build.sh`): it compiles both `.c` files with `-Werror` on top of `-Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Wshadow -Wundef -Wwrite-strings`, and needs no hardware.

## Status

0.2.2. The code is extracted from an in-tree component of a MH2457 (Cortex-M4) firmware, where it ran against real SDHC cards over a 4-bit SDIO host. The high-speed switch is the same CMD6 exchange that firmware used, so it has been exercised on four cards at 48 MHz — but only ever against that one host, and no second host port exists yet, which is the main thing standing between this and a 1.0.0. [CHANGELOG.md](CHANGELOG.md) lists the known gaps.

## License

BSD-3-Clause. See [LICENSE](LICENSE).