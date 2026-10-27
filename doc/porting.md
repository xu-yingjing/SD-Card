# Porting

The library talks to hardware through `sd_card_ops_t` (`src/sd_card.h`). Eight functions, none of them takes a `context` pointer: the port keeps its state wherever it likes, normally in file-scope variables next to the functions. Seven of them are the bus operations and carry the `host_` prefix (`host_send_cmd`, `host_read`, ...); `get_tick_ms` does not, because it is a time source rather than something the SDIO host does. `sd_card_init()` checks every entry once and stores the table pointer in the card object, so a table with a missing entry is rejected there instead of being called through later. The table is stored by pointer: it must have static storage duration.

Call `sd_card_init()` before `sd_card_probe()`. Probe and everything after it read the table from the card object, and `sd_card_probe()` returns `SD_CARD_ERROR` if no table is bound. `sd_card_reset()` only clears card state, so the binding survives a hot-plug and a re-probe needs no second init. It does not remember the bus width or the clock either: the card object has no field for either one, so after a reset the host is still in whatever `host_set_bus_width()` and `host_set_clock_frequency()` last left it in, and a re-probe needs you to bring it back to 1 bit at or below 400 kHz yourself. Leaving it at 4-bit 25 MHz makes CMD8 fail, which `sd_card_probe()` reports as `SD_CARD_ERROR`—the same code as an empty socket.

## Order of calls

For a successful probe, the library issues, in this order:

| Step | Command | Host ops used |
| --- | --- | --- |
| 1 | CMD0 goto idle state | `host_send_cmd` (no response) |
| 2 | CMD8 send interface condition | `host_send_cmd` (R7) |
| 3 | CMD55 + ACMD41, repeated until busy clears | `host_send_cmd` (R1, R3), `get_tick_ms` |
| 4 | CMD2 all send CID | `host_send_cmd` (R2, 128 bit) |
| 5 | CMD3 send relative address | `host_send_cmd` (R1) |
| 6 | CMD9 send CSD | `host_send_cmd` (R2, 128 bit) |
| 7 | CMD7 select card | `host_send_cmd` (R1) |

`sd_card_config_bus_width()` then sends CMD55 + ACMD6 and only afterwards calls your `host_set_bus_width`, so the card is already in 4-bit mode when the host switches: getting that order wrong is the classic stuck-at-1-bit symptom. `sd_card_config_clock_frequency()` sends no card command at all; it only moves your host clock. Both refuse to run while the card object is not `READY` (before a successful probe, or after `sd_card_reset()`), and the clock call also refuses a request above the card's declared maximum.

Reads call `host_prepare_read` before the command, and writes call no prepare function at all. The read sequence is `host_prepare_read` → CMD18 or CMD17 → `host_read` → CMD12 (multi-block only) → `host_abort_transfer` whenever anything after `host_prepare_read` failed. A failed read command aborts immediately, without CMD12; a failed data phase still gets its CMD12 first, then the abort. Writes are CMD13 polling → CMD55 + ACMD23 (multi-block, best effort) → CMD25 or CMD24 → `host_write` → CMD12 (multi-block only) → a second CMD13 poll; on any failure the abort replaces that second poll.

## Contracts

### `host_send_cmd(cmd)`

`cmd->index` is the raw command number (0..63), `cmd->argument` the 32-bit argument, and `cmd->response_type` one of `NONE`, `SHORT`, `SHORT_NO_CRC`, `LONG`. On return the library reads `cmd->response[0]` for the short forms and all four words for `LONG`. The response words must be ordered so that `response[0]` holds the most significant 32 bits of the 128-bit CID/CSD register: the CSD decoder indexes words from the top bit down, and reversing them yields a plausible-looking but wrong capacity.

`SHORT` means the host must check the response CRC and report an error if it is bad. `SHORT_NO_CRC` is for R3 (ACMD41), where the CRC field is not a valid CRC by specification and must be ignored. Return an error on command timeout: that is how `sd_card_probe()` decides a card is not there.

### `host_set_bus_width(width)`

Reconfigure the host data path to 1 or 4 bit. The card has already been switched by the time this is called; if the host switch fails, return an error and treat the card as unusable, because the two sides are now mismatched.

### `host_set_clock_frequency(clock_frequency)`

The value is the clock the card should see, already bounded by the card's own `TRAN_SPEED`. Compute your own divider. Return an error if you cannot produce a frequency at or below the request; never silently return OK for a faster clock.

### `host_prepare_read(buffer, size)`

Arm the data path and, if you use DMA, point it at `buffer` for `size` bytes, then return without waiting. `size` is always a multiple of 512. The command that triggers the transfer has not been sent yet, so this function must not depend on any status bit. On a polled host, record the destination and return OK.

### `host_read(buffer, size)`

Returns when all `size` bytes are in `buffer`, or with an error after your own data timeout. A blocking wait is expected here: this is the one place where waiting for the card is the host's job. If it fails, the library calls `host_abort_transfer` next, so leave the state such that a following abort is enough to make the next command work.

### `host_write(buffer, size)`

Must not return while the data path is still feeding the card. The library issues CMD12 immediately after it on a multi-block write, so a write that returns on FIFO acceptance rather than on the card accepting the data truncates the tail of every large write.

### `host_abort_transfer()`

Tear down any armed or in-flight transfer: clear the data-path busy bits, disable the DMA stream, drop any half-built block. It is called on error paths only, and must also be safe when nothing is in flight. No return value: a host that cannot recover should reset its SDIO block here.

### `get_tick_ms()`

Milliseconds, monotonic, from any source. The library only ever computes `tick - start_tick` in `uint32_t`, so a counter that wraps at 2^32 ms (about 49.7 days) is fine. Must not block. `sd_card_init()` rejects a table whose `get_tick_ms` is `NULL`, so the entry cannot be left out; the trap that remains is a hook that returns a constant, which makes both timeout loops unbounded and is indistinguishable from a very slow card.

Do not derive the millisecond value directly from a free-running cycle counter. A cycle counter wraps every 2^32 cycles - 14.3 s at 300 MHz, 89.5 s at 48 MHz - not every 49.7 days, and a wrap that lands inside a 1000 ms wait makes the subtraction look like a timeout. Accumulate milliseconds in software, or accept an occasional spurious probe failure.

## What stays yours

Power sequencing and the 1-bit, sub-400 kHz clock that must be running before `sd_card_probe()`. Pin muxing and pull-ups. Card detection, removal handling, and any power cycle you want before re-probing (`sd_card_reset()` only clears the card object; it sends no command and keeps the port binding). Block gap, bus timeout and CRC handling inside the host. File systems.

## Reading the probe result

| Return | What produced it |
| --- | --- |
| `SD_CARD_OK` | The card completed initialisation, its CSD decoded, and CMD7 selected it. |
| `SD_CARD_ERROR` | Everything else: CMD0 did not complete (on most SDIO hosts a command that asks for no response cannot fail, so expect this to be rare), CMD8 did not answer or echoed the wrong voltage window or check pattern (an empty socket, an MMC, or an SD card from before the 2.00 specification), ACMD41 reported a Standard Capacity card (`CSS = 0`) or did not complete within the 1000 ms window, any later command failed, or a CSD that does not decode (including CSD v1). |

The status enum has no separate "no card" or "unsupported" result: an empty socket and a card this library does not speak both surface as `SD_CARD_ERROR`. An application that must distinguish them needs its own card-detect pin.

## Porting checklist

1. Power on, pins muxed, host clock at or below 400 kHz, 1-bit mode.
2. Fill `sd_card_ops_t`, including `get_tick_ms`.
3. `sd_card_init(card, &ops)`, then `sd_card_probe()` from a task or main loop, never from an interrupt.
4. `sd_card_get_block_count()` and `sd_card_get_max_clock_frequency()` only report anything after a successful probe: both return 0 while the card object is not `READY`, and `sd_card_reset()` puts it back to that state.
5. Verify with a first read of block 0 and check for the MBR signature at offset 510 (`0x55, 0xAA`); a card that probes but reads back 0x00 has a data-path or response-ordering bug, not a protocol bug.
6. Only then widen the bus and raise the clock.
