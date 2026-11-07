# Porting

The library uses `sd_card_ops_t` (`src/sd_card.h`): nine entries, with no `context` pointer. Seven required callbacks carry the `host_` prefix and operate the controller. `get_tick_ms` is a required time source; `sleep_ms` is an optional task-scheduling service. Neither time service introduces an OS dependency into the library. Ports normally keep controller state in file-scope variables next to the callbacks.

Call `sd_card_init()` before any other card operation. It validates the card pointer, table pointer and all eight required callbacks; `sleep_ms` may be NULL. The table is stored by pointer and must remain valid while bound, normally as a static const object. Initialise every member: omitted members of a designated aggregate initializer are zero-initialised, but assigning the original eight fields individually leaves `sleep_ms` uninitialised unless it is assigned too. Both public structure layouts changed in 0.3.0, so recompile the port and library together.

`sd_card_reset()` clears identification and capacity state but keeps the ops binding. It does not send CMD0, remove power, or reconfigure the host. Before every probe, restore the host to 1-bit mode at or below 400 kHz. Do not use a failed CMD8 as proof of a bad clock or an empty socket: a pre-2.00 SD card also does not answer it.

## Order of calls

For a successful probe, the library issues, in this order:

| Step | Command | Ops used |
| --- | --- | --- |
| 1 | CMD0 go idle state | `host_send_cmd` (no response) |
| 2 | CMD8 send interface condition; failure is tolerated | `host_send_cmd` (R7) |
| 3 | CMD55 + ACMD41, repeated until busy clears | `host_send_cmd` (R1, R3), `get_tick_ms`, optional `sleep_ms` |
| 4 | CMD2 all send CID | `host_send_cmd` (R2, 128 bit) |
| 5 | CMD3 send relative address | `host_send_cmd` (R6: RCA in `[31:16]`) |
| 6 | CMD9 send CSD | `host_send_cmd` (R2, 128 bit) |
| 7 | CMD7 select card | `host_send_cmd` (R1b) |
| 8 | CMD16 set block length to 512, SDSC only | `host_send_cmd` (R1) |

A successful CMD8 must echo the requested 2.7-3.6 V window and check pattern. A failed exchange follows the legacy path instead. ACMD41 sets HCS only after a valid CMD8 reply. Probe stops after eight consecutive CMD55 failures; any successful CMD55 resets that count, including when the following ACMD41 fails. This is an initialisation-only policy, not a retry rule for every APP_CMD. The 1000 ms initialisation deadline is still checked between commands.

`sd_card_config_bus_width()` sends CMD55 + ACMD6 before calling `host_set_bus_width`, so the host follows the card's width change. `sd_card_config_high_speed()` checks CSD CCC bit 10 before sending two CMD6 exchanges, check and select. It does not read SCR `SD_SPEC`. A confirmed switch sets `max_clock_frequency` to 50 MHz; a failed switch leaves that field unchanged. `sd_card_config_clock_frequency()` sends no card command and rejects requests above the stored maximum. These configuration calls require a ready card. A higher decoded `TRAN_SPEED` is not evidence that UHS or 1.8 V signalling has been configured; the caller and port must choose a clock suitable for the active mode.

Reads follow `host_prepare_read` -> CMD17/18 -> `host_read` -> CMD12 for multi-block transfers. Writes follow CMD13 readiness polling -> best-effort CMD55/ACMD23 for multi-block transfers -> CMD24/25 -> `host_write` -> CMD12 for multi-block transfers -> another CMD13 readiness poll. A failed read/write command aborts without CMD12. A failed data phase still attempts CMD12 when multi-block, then aborts. A failed CMD12 also aborts. Failed readiness polls return an error without calling `host_abort_transfer`; successful transfers do not call it either. Hosts must handle their normal completion cleanup themselves.

## Capacity and block addressing

The decoder requires CSD v1 to agree with OCR `CCS = 0`, and CSD v2 with `CCS = 1`. CSD v1 capacity, expressed as 512-byte blocks, is `(C_SIZE + 1) * 2^(C_SIZE_MULT + 2) * 2^(READ_BL_LEN - 9)`. Read/write block-length fields must match and encode 512, 1024 or 2048 bytes, and the decoded capacity must not exceed the implementation's 2028 MiB ceiling. CSD v2 accepts `C_SIZE` in `0x1010..0xFF5F` (SDHC) or `0xFFFF..0x3FFEFF` (SDXC). A reported CMD16 failure prevents an SDSC card from becoming ready.

Once ready, `sd_card_get_capacity_type()` reports `SDSC`, `SDHC` or `SDXC`; a NULL or non-ready card reports `UNKNOWN`. All public block numbers and counts use 512-byte units. Only the library converts SDSC block numbers to byte addresses for CMD17/18/24/25: the host must send `cmd->argument` unchanged, not multiply it again. SDHC and SDXC use block-addressed commands.

## Contracts

### `host_send_cmd(cmd)`

`cmd->index` is the raw command number (0..63), `cmd->argument` the 32-bit argument, and `cmd->response_type` one of `NONE`, `SHORT`, `SHORT_NO_CRC`, `LONG`. On return the library reads `cmd->response[0]` for the short forms and all four words for `LONG`. The response words must be ordered so that `response[0]` holds the most significant 32 bits of the 128-bit CID/CSD register: the CSD decoder indexes words from the top bit down, and reversing them yields a plausible-looking but wrong capacity.

`SHORT` requires response CRC checking. `SHORT_NO_CRC` is for R3 (ACMD41), whose CRC field is not meaningful. CMD7 and CMD12 request `SHORT` but answer R1b: handle any DAT0 busy period in the port under its own timeout. The library has no dedicated R1b-wait callback. Return `SD_CARD_ERROR` when a command times out; do not hide a failed exchange or return fabricated success. CMD8 failure alone is tolerated, while repeated CMD55 failures can terminate probe early. A host timeout therefore needs to be bounded independently of the library's polling deadline.

`cmd->data_size` is 0 for every command the library issues except CMD6 `SWITCH_FUNC`, which answers with a 64-byte status block on the data line. When it is not 0, `cmd->data` points at a buffer of that many bytes and the host owns the whole data phase: arm the block (the size is a power of two from 4 to 512) *before* sending the command, because the data path has to be listening the moment the card starts driving the line, then collect what arrives into `cmd->data` after the response. Store the bytes in wire order, low byte of each word first: the library reads the status back as bytes, and the offsets in it only mean anything with that order fixed. If a command with a data phase fails, the library calls `host_abort_transfer()` for you, so an armed block that never got its data only has to be recoverable that way. The library sets no timer on that phase: CMD6 is a standard single block read and the specification gives it 100 ms, so the window belongs to the port — and so does the power cycle the same section asks for when the status block comes back with a CRC error, since the status enum has no room to report one.

### `host_set_bus_width(width)`

Reconfigure the host data path to 1 or 4 bit. The card has already been switched by the time this is called; if the host switch fails, return an error and treat the card as unusable, because the two sides are now mismatched.

### `host_set_clock_frequency(clock_frequency)`

Choose an actual frequency at or below the request and within the host's supported range. The library has already checked the request against the card object's stored maximum. A divider producing a lower frequency is acceptable; never report success for a faster clock. This callback does not select a card signalling mode, and the library does not verify the actual frequency or automatically retry failed transfers at a lower one.

### `host_prepare_read(buffer, size)`

Arm the data path and, if used, DMA for `size` bytes into `buffer`, then return without waiting. `size` is a multiple of 512. The read command has not been issued yet, so this callback must not wait for its data. A polled host can record the destination and length. If preparation fails, unwind any partial setup before returning: the library returns immediately on that failure without an abort callback.

### `host_read(buffer, size)`

Return after all `size` bytes have reached `buffer`, or after a bounded data error/timeout. A multi-block read is followed by CMD12 even when the data phase failed. On failure, leave the host recoverable by `host_abort_transfer`; on success, perform the host's required completion cleanup without relying on an abort callback.

### `host_write(buffer, size)`

Must not return while the data path is still feeding the card. The library issues CMD12 immediately after it on a multi-block write, so a write that returns on FIFO acceptance rather than on the card accepting the data truncates the tail of every large write.

### `host_abort_transfer()`

Tear down any armed or in-flight transfer: clear the data-path busy bits, disable the DMA stream, drop any half-built block. It is called on error paths only, and must also be safe when nothing is in flight. No return value: a host that cannot recover should reset its SDIO block here.

### `get_tick_ms()`

Return a non-blocking, monotonic millisecond counter that wraps modulo 2^32. The library computes unsigned `tick - start_tick`, so the approximately 49.7-day wrap is safe. The counter must advance independently of polling, including during host operations and sleeps. A constant counter can leave not-ready polling unbounded. `sd_card_init()` rejects a NULL time source.

Do not derive milliseconds by simply dividing a wrapping CPU cycle counter: it wraps in cycles, not in 2^32 milliseconds, so a wrap during a wait can appear as a large elapsed time. Use a real millisecond timebase or accumulate wrap-aware elapsed cycles.

This callback remains required even when `sleep_ms` is supplied. Counting requested sleeps misses command execution time and scheduler delays; it also cannot measure the initial non-sleeping polling interval.

### `sleep_ms(ms)`

Optional; NULL keeps the polling loops busy-waiting. An RTOS port should block the calling task using a scheduler delay rather than a spin loop or a bare yield. Convert the requested milliseconds to scheduler ticks, rounding up when necessary so a nonzero request does not become a zero-tick yield. Actual elapsed time depends on tick phase and scheduling; the library uses `get_tick_ms`, not the requested sleep duration, to decide whether a deadline has passed.

A ready response exits immediately. For a successful but not-ready reply, the library may call this hook once the elapsed millisecond count reaches `SD_POLL_SLEEP_THRESHOLD_MS` (2). Initialisation requests 10 ms; each transfer-ready wait requests 1 ms. The threshold is measured from the start of the current wait, not restarted after each poll. With a counter advancing every millisecond, a difference of 1 could mean only a few microseconds passed across a tick boundary; a difference of 2 ensures at least about 1 ms passed before sleeping. This preserves a short fast-polling interval at the cost of CPU use. Failed CMD55/ACMD41 calls skip this hook.

A blocking hook requires task context with the scheduler running; do not reach it from an interrupt, a critical section or a scheduler-suspended context. The library does not test those conditions or provide locking. Serialize access to the same card/controller even while the task sleeps. Bare-metal ports can leave the hook NULL.

The 1000 ms deadlines are checked at loop boundaries. A host call or scheduled sleep may return after a deadline, and cannot be preempted by the library. The hook only affects these two library polling loops, not waits inside the host callbacks.

## What stays yours

Power sequencing, pin muxing and pull-ups, and the initial 1-bit clock at or below 400 kHz. Card detection and removal handling: gate probe when the socket is known to be empty, and reset the card object after removal. Host timeouts, CRC handling, buffer alignment and DMA-accessible placement. Any power-cycle recovery, controller sharing and file systems. The library's capacity class does not identify a card's filesystem.

## Reading the probe result

| Return | What produced it |
| --- | --- |
| `SD_CARD_OK` | Initialisation completed, OCR and CSD agreed, CSD decoded, CMD7 selected the card, and any required SDSC CMD16 call succeeded. |
| `SD_CARD_ERROR` | Invalid card/binding; CMD0 failure; a successful CMD8 with a bad echo; eight consecutive CMD55 failures; initialisation timeout; completed ACMD41 with no requested voltage bit; rejected CSD fields/capacity or an OCR/CSD mismatch; or a later command failure, including SDSC CMD16. |

A failed CMD8 is not itself an error return from probe. The status enum still does not distinguish an empty socket from unsupported or failing media; use card detection when that distinction matters.

## Porting checklist

1. Power the card, configure pins, and start the host in 1-bit mode at or below 400 kHz.
2. Initialise all nine ops entries, including the required `get_tick_ms` and either a valid `sleep_ms` or NULL.
3. Call `sd_card_init(card, &ops)`, then `sd_card_probe()` from a main loop or an appropriate task, never an interrupt.
4. Check `sd_card_get_capacity_type()`, block size/count and maximum clock. Capacity type is `UNKNOWN` and numeric getters return zero for a non-ready card.
5. Verify a known sector at the initial clock, including host alignment requirements. An MBR signature at offsets 510-511 is useful on an MBR-formatted card, but absent signatures or all-zero data alone do not prove a transport fault.
6. Then configure bus width, try the optional high-speed switch and select a suitable host clock. Repeat data verification at the final clock.
7. For an RTOS port, verify both short-transfer latency and task scheduling during long busy waits; successful data verification alone does not measure CPU availability to other tasks.
