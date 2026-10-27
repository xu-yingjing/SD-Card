# Porting

The library talks to hardware through `sd_card_host_ops_t` (`src/sd_card_host.h`). Eight functions, all called with the `context` pointer you passed to `sd_card_host_init()`. Each wrapper in `sd_card_host.c` checks for a `NULL` host, `NULL` ops table, `NULL` function pointer (and, where it takes a buffer, a `NULL` buffer or zero size) and returns `SD_CARD_HOST_ERROR` instead of calling through, so a partially filled table fails at the call site instead of crashing; `sd_card_host_abort_transfer` returns nothing and does nothing in that case.

## Order of calls

For a successful probe, the library issues, in this order:

| Step | Command | Host ops used |
| --- | --- | --- |
| 1 | CMD0 goto idle state | `send_cmd` (no response) |
| 2 | CMD8 send interface condition | `send_cmd` (R7) |
| 3 | CMD55 + ACMD41, repeated until busy clears | `send_cmd` (R1, R3), `get_tick_ms` |
| 4 | CMD2 all send CID | `send_cmd` (R2, 128 bit) |
| 5 | CMD3 send relative address | `send_cmd` (R1) |
| 6 | CMD9 send CSD | `send_cmd` (R2, 128 bit) |
| 7 | CMD7 select card | `send_cmd` (R1) |

`sd_card_config_bus_width()` then sends CMD55 + ACMD6 and only afterwards calls your `set_bus_width`, so the card is already in 4-bit mode when the host switches: getting that order wrong is the classic stuck-at-1-bit symptom. `sd_card_config_clock_frequency()` sends no card command at all; it only moves your host clock. Both refuse to run while the card object is not `READY` (before a successful probe, or after `sd_card_reset()`), and the clock call also refuses a request above the card's declared maximum.

Reads call `prepare_read` before the command, and writes call no prepare function at all. The read sequence is `prepare_read` → CMD18 or CMD17 → `read` → CMD12 (multi-block only) → `abort_transfer` whenever anything after `prepare_read` failed. A failed read command aborts immediately, without CMD12; a failed data phase still gets its CMD12 first, then the abort. Writes are CMD13 polling → CMD55 + ACMD23 (multi-block, best effort) → CMD25 or CMD24 → `write` → CMD12 (multi-block only) → a second CMD13 poll; on any failure the abort replaces that second poll.

## Contracts

### `send_cmd(context, cmd)`

`cmd->index` is the raw command number (0..63), `cmd->argument` the 32-bit argument, and `cmd->response_type` one of `NONE`, `SHORT`, `SHORT_NO_CRC`, `LONG`. On return the library reads `cmd->response[0]` for the short forms and all four words for `LONG`. The response words must be ordered so that `response[0]` holds the most significant 32 bits of the 128-bit CID/CSD register: the CSD decoder indexes words from the top bit down, and reversing them yields a plausible-looking but wrong capacity.

`SHORT` means the host must check the response CRC and report an error if it is bad. `SHORT_NO_CRC` is for R3 (ACMD41), where the CRC field is not a valid CRC by specification and must be ignored. Return an error on command timeout: that is how `sd_card_probe()` decides a card is not there.

### `set_bus_width(context, width)`

Reconfigure the host data path to 1 or 4 bit. The card has already been switched by the time this is called; if the host switch fails, return an error and treat the card as unusable, because the two sides are now mismatched.

### `set_clock_frequency(context, clock_frequency)`

The value is the clock the card should see, already bounded by the card's own `TRAN_SPEED`. Compute your own divider. Return an error if you cannot produce a frequency at or below the request; never silently return OK for a faster clock.

### `prepare_read(context, buffer, size)`

Arm the data path and, if you use DMA, point it at `buffer` for `size` bytes, then return without waiting. `size` is always a multiple of 512. The command that triggers the transfer has not been sent yet, so this function must not depend on any status bit. On a polled host, record the destination and return OK.

### `read(context, buffer, size)`

Returns when all `size` bytes are in `buffer`, or with an error after your own data timeout. A blocking wait is expected here: this is the one place where waiting for the card is the host's job. If it fails, the library calls `abort_transfer` next, so leave the state such that a following abort is enough to make the next command work.

### `write(context, buffer, size)`

Must not return while the data path is still feeding the card. The library issues CMD12 immediately after it on a multi-block write, so a write that returns on FIFO acceptance rather than on the card accepting the data truncates the tail of every large write.

### `abort_transfer(context)`

Tear down any armed or in-flight transfer: clear the data-path busy bits, disable the DMA stream, drop any half-built block. It is called on error paths only, and must also be safe when nothing is in flight. No return value: a host that cannot recover should reset its SDIO block here.

### `get_tick_ms(context)`

Milliseconds, monotonic, from any source. The library only ever computes `tick - start_tick` in `uint32_t`, so a counter that wraps at 2^32 ms (about 49.7 days) is fine. Must not block. If you leave it `NULL`, every call that needs a timeout returns `SD_CARD_ERROR` at its first look at the clock, which is deliberate: the alternative is a timeout loop that never expires. In `sd_card_probe()` that first look happens after CMD0 and CMD8 have already been sent, so expect two commands on the bus before the failure. The same trap opens if the hook exists but is frozen: a `get_tick_ms` that returns a constant makes both timeout loops unbounded, and the library cannot tell the difference between a slow card and a stopped clock.

## What stays yours

Power sequencing and the 1-bit, sub-400 kHz clock that must be running before `sd_card_probe()`. Pin muxing and pull-ups. Card detection, removal handling, and any power cycle you want before re-probing (`sd_card_reset()` only clears the card object; it sends no command). Block gap, bus timeout and CRC handling inside the host. File systems.

## Reading the probe result

| Return | What produced it |
| --- | --- |
| `SD_CARD_NO_CARD` | CMD0 failed. On most SDIO hosts a command that asks for no response cannot fail, so this needs your `send_cmd` to report an error for CMD0 specifically; expect it to be rare. |
| `SD_CARD_UNSUPPORTED` | CMD8 failed, i.e. no R7 came back: an empty socket, an MMC, or an SD card from before the 2.00 specification. Or ACMD41 completed with `CSS = 0`, i.e. a Standard Capacity card. |
| `SD_CARD_ERROR` | Everything else: CMD8 answered with the wrong voltage window or check pattern, any later command failing, ACMD41 not completing within the 1000 ms window, a CSD that does not decode, or a CSD whose `TRAN_SPEED` is not one of the four codes the library recognises. CSD v1 lands here as well: the CSD decoder itself returns `SD_CARD_UNSUPPORTED` for it, but `sd_card_probe()` flattens every decoder status to `SD_CARD_ERROR`. |

So the return value does not reliably tell "nothing in the socket" from "a card this library does not speak": an empty socket normally surfaces as `SD_CARD_UNSUPPORTED` because CMD8 times out, while a Standard Capacity card surfaces as `SD_CARD_ERROR`. An application that must distinguish them needs its own card-detect pin. Both the CMD8-based empty-socket detection and the flattened CSD status are candidates for the next release; see `CHANGELOG.md`.

## Porting checklist

1. Power on, pins muxed, host clock at or below 400 kHz, 1-bit mode.
2. Fill `sd_card_host_ops_t`, including `get_tick_ms`.
3. `sd_card_host_init()`, then `sd_card_probe()` from a task or main loop, never from an interrupt.
4. `sd_card_get_block_count()` and `sd_card_get_max_clock_frequency()` only report anything after a successful probe: both return 0 while the card object is not `READY`, and `sd_card_reset()` puts it back to that state.
5. Verify with a first read of block 0 and check for the MBR signature at offset 510 (`0x55, 0xAA`); a card that probes but reads back 0x00 has a data-path or response-ordering bug, not a protocol bug.
6. Only then widen the bus and raise the clock.
