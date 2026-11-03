# Changelog

## 0.2.2

CMD6 `SWITCH_FUNC`: the library can now ask a card to enter its high-speed timing band. This was previously a host-side routine in the firmware the library came from, and moving it here is what lets a port raise the clock above 25 MHz without leaving the specification.

- `sd_card_config_high_speed(sd_card_t *card)` sends the check for function 1 and then the select, reads the 64-byte switch status each time, and on confirmation raises `max_clock_frequency` to 50 MHz — the ceiling for 3.3 V signalling. A card that reports no high-speed function, or that answers but does not confirm the selection, leaves the card object untouched and returns `SD_CARD_ERROR`, so keeping the CSD ceiling is the expected reaction rather than a failure to handle.
- A card declaring 100 or 200 MHz in `TRAN_SPEED` is capped to 50 MHz by a successful switch instead of keeping its declared value. Those bands belong to UHS functions, and ACMD41 is still sent with `S18R = 0`, so the library never enters them; the CSD number had been promising a clock no 3.3 V bus can carry.
- `sd_card_host_cmd_t` gains `data` and `data_size`: the optional data phase CMD6 needs. `data_size = 0` means no data phase, so no other command changes behaviour and the ops table stays at eight functions.
- Migration pitfall for ports: `host_send_cmd` now reads `cmd->data` and `cmd->data_size`, so a command structure declared as `sd_card_host_cmd_t cmd;` without an initializer feeds garbage to that check. All seventeen command helpers in the library set both fields before the call — sixteen to `data = NULL` with `data_size = 0U`, and `sd_card_switch_function()` to the status buffer with its 64 bytes — and a port that builds one by hand has to do the same.
- The status buffer is cleared with `memset()` before each of the two exchanges, so `src/sd_card.c` now includes `<string.h>`: the fourth libc header in the library. The buffer is a local array, so the no-writable-statics property is unchanged.
- The contract for a data phase is in `doc/porting.md`: arm the block before sending the command, collect `data_size` bytes into `cmd->data` after the response, low byte of each word first. A command that fails with the block armed now gets the `host_abort_transfer()` call that unwinds it.
- `sd_card_probe()`, the bus-width switch and the read/write paths are otherwise unchanged. Footprint grows by 252 bytes of `.text` at `-O2` (1436 to 1688 bytes, arm-none-eabi-gcc 12.3 on cortex-m4), still no `.data`, `.bss` or `.rodata`. Both numbers are from this release: the 1436 was re-measured from the `v0.2.1` tag with the same flags rather than taken from the 0.2.1 note.
- `doc/porting.md` fixes two response forms in the probe table. CMD3 answers R6 (the written RCA in the top half of the response word), not R1, and CMD7 answers R1b — so the busy a card may drive on DAT0 once selected or stopped is the port's to wait out, because the library issues no wait of its own after either command.
- Still open from 0.2.1: no Standard Capacity (CSD v1) support, no UHS or 1.8 V signalling, no yield hook for the busy-wait timeouts, and no second host port or mock-based test behind the ops table. Two more come with this release: CMD6 goes out without the SCR `SD_SPEC` or CSD `CCC` bit10 check that section 4.3.10.1 asks the host to make first, and it asks `Power Limit = no influence`, so a card left on its power-on limit of 0.72 W — the minimum setting — stays there, while the section notes that the higher performance needs the host to raise it. The two `-Wconversion` warnings in `sd_card_get_csd_field()` (`src/sd_card.c:536` and `:542`) still keep `build.sh` failing until the CSD field indices are cast.

## 0.2.1

Names only. The protocol layer behaves exactly as it did in 0.2.0, so a port written against 0.2.0 recompiles after one change: renaming its table initializers.

- Seven entries of `sd_card_ops_t` gain a `host_` prefix: `host_send_cmd`, `host_set_bus_width`, `host_set_clock_frequency`, `host_abort_transfer`, `host_prepare_read`, `host_read` and `host_write`. The types they traffic in were already `sd_card_host_cmd_t` and `sd_card_host_bus_width_t`; the entries now match them, and `sd_card_init()` still rejects a table with any NULL entry.
- `get_tick_ms` stays unprefixed on purpose. It is a time source rather than something the SDIO host does, so it does not belong to that subdomain.
- `examples/sd_card_host_template.c`, `doc/porting.md` and the README quick start follow the new names. Footprint is unchanged: `sd_card.o` measures 1436 bytes of `.text`, with no `.data`, `.bss` or `.rodata`, at `-O2` on cortex-m4 in both versions.
- Still open from 0.2.0: no Standard Capacity (CSD v1) support, no high-speed or 1.8 V signalling, no yield hook for the busy-wait timeouts, and no second host port or mock-based test behind the ops table. Two `-Wconversion` warnings in `sd_card_get_csd_field()` also keep `build.sh` failing until the CSD field indices are cast.

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
