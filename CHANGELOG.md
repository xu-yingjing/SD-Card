# Changelog

## 0.2.0

Breaking change at the port boundary. The host wrapper is gone: `src/sd_card_host.[ch]` and `sd_card_host_init()` are removed, and the ops table is now `sd_card_ops_t`, stored by pointer inside `sd_card_t` and bound once by the new `sd_card_init()`.

- Every ops entry takes only the arguments it needs: the `void *context` parameter is gone. Port state now belongs in file-scope variables next to the port functions. The table itself is stored by pointer, so it must have static storage duration.
- `sd_card_init(card, ops)` validates the card pointer, the table pointer and all eight entries, then stores the table in the card object. `sd_card_probe()` refuses to run (returns `SD_CARD_ERROR`) if no table is bound, so an uninitialised card object fails the probe instead of dereferencing a random ops pointer.
- `sd_card_probe()`, `sd_card_config_bus_width()`, `sd_card_config_clock_frequency()`, `sd_card_read_blocks()` and `sd_card_write_blocks()` take only the card object; the table is read from it. `sd_card_reset()` clears card state but keeps the binding, so a re-probe after a hot-plug needs no second `sd_card_init()`.
- The status enum is reduced to `SD_CARD_OK` and `SD_CARD_ERROR`: `SD_CARD_NO_CARD` and `SD_CARD_UNSUPPORTED` are removed, and so is `sd_card_host_status_t`. Port functions that returned `SD_CARD_HOST_OK`/`SD_CARD_HOST_ERROR` now return `sd_card_status_t`. An empty socket, a Standard Capacity card and a CSD that does not decode all surface as `SD_CARD_ERROR`; an application that must tell them apart needs its own card-detect.
- `examples/sd_card_host_template.c`, `doc/porting.md`, `build.sh` and the README were updated to the new boundary.
- The four `SD_CARD_CLOCK_FREQUENCY_*HZ` constants changed from object-like macros to enumerators. Their values and spellings are unchanged, so code using them as values compiles as before, but a port that tested `#ifdef SD_CARD_CLOCK_FREQUENCY_50000000HZ` now always takes the false branch.

Known gaps carried over from 0.1.0: no Standard Capacity (CSD v1) support, no high-speed or 1.8 V signalling, no acceptance of `TRAN_SPEED` codes outside the four recognised ones, no reliable way to tell an empty socket from a card the library rejects, no yield hook for the busy-wait timeouts, and no second host port or mock-based test to prove the ops boundary is honest.

## 0.1.0

Initial public release. Extracted from an in-tree driver component that had been brought up against SDHC cards behind a 4-bit SDIO host on a Cortex-M4 part. The release contains the protocol layer only: no host driver, no vendor register code, no file system.

Changes made while separating it from the tree it came from:

- The millisecond time source moved from a weakly linked global (`sd_card_get_tick_ms()`, defaulting to a function that returns 0) to `sd_card_host_ops_t.get_tick_ms`. The protocol files no longer include any CMSIS header, and a port that does not supply a time source now gets `SD_CARD_ERROR` from the first timed call instead of an unbreakable loop in `sd_card_probe()`.
- The two timeout loops (`sd_card_probe()` initialisation, `sd_card_wait_transfer_ready()`) were restructured so the timeout is checked once per iteration with an explicit return. The 1000 ms windows and the command order are unchanged.
- The CSD `TRAN_SPEED` handling is left as it was: the four recognised codes (25, 50, 100 and 200 MHz) are validated before use, and a card reporting any other code fails `sd_card_probe()` instead of being assumed to run at 25 MHz. It is documented as a restriction in the README rather than changed in this release.

Known gaps tracked for 0.2.0: no Standard Capacity (CSD v1) support, no high-speed or 1.8 V signalling, no acceptance of `TRAN_SPEED` codes outside the four recognised ones, no reliable way to tell an empty socket from a card the library rejects (the probe result flattens both the CMD8 timeout and the CSD status), no yield hook for the busy-wait timeouts, and no second host port or mock-based test to prove the ops boundary is honest.
