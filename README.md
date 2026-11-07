# sd-card

SD memory card protocol layer for controllers with an SDIO-style host (command register, response registers, block data path). C99, no OS dependency, no HAL, no dynamic allocation, no writable static data.

The library owns the protocol: the initialisation command sequence (CMD0, CMD8, CMD55/ACMD41, CMD2, CMD3, CMD9, CMD7, plus CMD16 for SDSC), bus-width switching (CMD55/ACMD6), high-speed function selection (CMD6), CSD decoding, and single/multi-block read and write with CMD12/CMD13 handling. You own the transport: seven host operations and a required millisecond clock, plus an optional `sleep_ms` hook for task scheduling. The nine-entry ops table is bound once with `sd_card_init()`. Pin muxing, power, clock start-up and card detection remain outside the library.

## Scope

Supported:

- SDSC, SDHC and SDXC initialisation and identification, including pre-2.00 SD cards that do not answer CMD8. MMC initialisation is not attempted.
- 1-bit and 4-bit bus, with a successful `sd_card_config_high_speed()` selecting a 50 MHz ceiling. The host supplies the actual clock at or below the requested frequency.
- Single and multi-block read (CMD17/CMD18 + CMD12) and write (CMD24/CMD25 + best-effort ACMD23 + CMD12), with CMD13 `READY_FOR_DATA` polling before and after a write.
- CSD v1 and v2 capacity decoding, cross-checked against OCR `CCS`. SDSC uses byte-addressed commands after CMD16 sets a 512-byte block length; SDHC/SDXC uses block-addressed commands. The public read/write API always takes 512-byte block numbers.
- `sd_card_get_capacity_type()` reports `UNKNOWN`, `SDSC`, `SDHC` or `SDXC`; SDHC and SDXC are distinguished by the decoded capacity.

Current limits and unsupported features:

- SDSC capacity is capped at **2028 MiB** (4,153,344 blocks) by the current implementation. CSD v1 read/write block-length fields must match and encode 512, 1024 or 2048 bytes; the exposed block size is always 512 bytes. See [doc/porting.md](doc/porting.md) for the accepted CSD v2 capacity ranges.
- Bus widths other than 1 and 4 bit (no 8-bit), DDR, and 1.8 V signalling (ACMD41 is sent with `S18R = 0`). CMD6 selects only the high-speed function, never UHS. Decoding a higher `TRAN_SPEED` does not enable that signalling mode.
- Unrecognised CSD `TRAN_SPEED` codes: CSD v1 accepts 25 or 50 MHz; CSD v2 accepts 25, 50, 100 or 200 MHz. Other codes produce `SD_CARD_ERROR` instead of an assumed speed. There is no automatic clock fallback after a transfer failure.
- Voltage ranges outside 2.7-3.6 V (CMD8 and ACMD41 request that window only).
- Card removal, power cycling, explicit erase (CMD32/CMD33/CMD38), write protection (CMD28/CMD29), CID decoding, and file systems.

## Requirements

- C99 and `<stdint.h>`, `<stdbool.h>`, `<stddef.h>`, `<string.h>`. Nothing else: the library includes no CMSIS, device or OS header. Only `sd_card_config_high_speed()` reaches for the C library, to clear the 64-byte switch status on the stack before each CMD6 exchange.
- One caller-owned `sd_card_t` per card (64 bytes with 32-bit pointers and enums: ten fields, the `uint16_t rca` followed by two bytes of padding). The nine-pointer `sd_card_ops_t` occupies 36 bytes on that target and must remain valid while bound.
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
    .sleep_ms                 = NULL,
};

/* Before probing: power the card, mux the pins, and start the host clock at
   no more than 400 kHz in 1-bit mode. The library does not do any of that. */
if (sd_card_init(&card, &host_ops) != SD_CARD_OK) {
    /* A NULL card/table pointer or a missing required callback; sleep_ms may be NULL. */
}

if (sd_card_probe(&card) != SD_CARD_OK) {
    /* No card, or a card this library does not speak. */
}

sd_card_config_bus_width(&card, SD_CARD_BUS_WIDTH_4B);

/* Stay at default speed unless the high-speed switch is confirmed. */
uint32_t clock_frequency = SD_CARD_CLOCK_FREQUENCY_25000000HZ;
if (sd_card_config_high_speed(&card) == SD_CARD_OK) {
    clock_frequency = sd_card_get_max_clock_frequency(&card);
}

sd_card_config_clock_frequency(&card, clock_frequency);

uint8_t sector[512];
sd_card_read_blocks(&card, sector, 0U, 1U);
```

`sd_card_init()` is called once; every later call takes only the card object, because the ops table is stored in it. `sd_card_reset()` clears the card state after removal but keeps the binding and does not send CMD0 or drop the power, so pair it with your own host de-initialisation if you re-probe after a hot-plug.

## Timing

`get_tick_ms` measures elapsed time; the optional `sleep_ms` gives an RTOS port a place to block the calling task and let other tasks run. They are separate services: requested sleep durations cannot account for host-command time or scheduling delays, so the time source remains required.

| Polling loop | Timeout | Requested sleep after the threshold |
| --- | --- | --- |
| CMD55/ACMD41 initialisation | 1000 ms | 10 ms |
| CMD13 transfer-ready, before and after a write | 1000 ms per wait | 1 ms |

A ready response returns immediately. For a successful but not-ready response, sleeping starts only when the elapsed millisecond count reaches `SD_POLL_SLEEP_THRESHOLD_MS` (2 ms). With a clock that advances every millisecond, this avoids sleeping after merely crossing the first tick boundary and gives short waits at least about 1 ms of continuous polling. It is a latency/CPU-use trade-off, not a protocol requirement or a fixed delay on every call. If `sleep_ms` is `NULL`, both loops remain busy waits.

Probe returns `SD_CARD_ERROR` after eight consecutive failed CMD55 calls; a successful CMD55 resets the count even if the following ACMD41 fails. This budget applies only during initialisation. Failed CMD55/ACMD41 calls skip the sleep hook; successful ACMD41 replies that still report busy can use it. A CMD8 failure alone is not a missing-card test. A board with card detection should avoid probing a known-empty socket; without that, exit time depends on the host's command timeouts as well as the failure budget.

Timeouts are checked between commands, not enforced by preempting a host operation or a sleeping task. Host callbacks need their own bounded waits, and scheduling can delay the return past a deadline. There is no cancellation or built-in locking: use task/main-loop context, never an interrupt, and serialize access to a shared card and host. See [doc/porting.md](doc/porting.md#sleep_msms) before supplying a blocking hook.

## Porting

See [doc/porting.md](doc/porting.md) for the contract of each ops entry, including the data-path ordering rules, the independent `get_tick_ms` time source, and the optional `sleep_ms` scheduling hook. [examples/sd_card_host_template.c](examples/sd_card_host_template.c) is a non-functional skeleton with all eight required callbacks and an explicit `.sleep_ms = NULL`.

## Size

Measured for 0.3.0 with armclang 6.24 (`--target=arm-arm-none-eabi -std=c99 -O2 -mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard`), using `fromelf --text -z` on `sd_card.o`:

| Component | Bytes |
| --- | ---: |
| Code | 2312 |
| RO Data | 120 |
| RW Data | 0 |
| ZI Data | 0 |

These are object-file figures, not the final linked firmware size. The earlier 1688-byte GCC measurement belongs to 0.2.3 and was not repeated for this release; it is not a like-for-like size comparison. The strict warning set from `build.sh` passes with armclang 6.24. The historical GCC `-Wconversion` diagnostics in `sd_card_get_csd_field()` have not been rechecked with GCC.

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

0.3.0. Firmware-level regression testing on the MH2457 (Cortex-M4) 4-bit SDIO host covers nominal 512 MB, 1 GB and 2 GB SDSC cards and 8 GB and 32 GB SDHC cards: 20/20 repeated initialisations per card, boundary and multi-block reads, raw-block write/readback/restore, and FatFs file verification across re-probing. The sleep hook was exercised with that RTOS port; these tests do not measure CPU availability to a second application task.

SDXC decoding is implemented, but SDXC hardware validation, a second host port and mock-based automated tests remain outstanding. Clock compatibility is card- and host-dependent, and there is no automatic retry at a lower clock. [CHANGELOG.md](CHANGELOG.md) lists the remaining gaps.

## License

BSD-3-Clause. See [LICENSE](LICENSE).