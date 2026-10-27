# Changelog

## 0.1.0

Initial public release. Extracted from an in-tree driver component that had been brought up against SDHC cards behind a 4-bit SDIO host on a Cortex-M4 part. The release contains the protocol layer only: no host driver, no vendor register code, no file system.

Changes made while separating it from the tree it came from:

- The millisecond time source moved from a weakly linked global (`sd_card_get_tick_ms()`, defaulting to a function that returns 0) to `sd_card_host_ops_t.get_tick_ms`. The protocol files no longer include any CMSIS header, and a port that does not supply a time source now gets `SD_CARD_ERROR` from the first timed call instead of an unbreakable loop in `sd_card_probe()`.
- The two timeout loops (`sd_card_probe()` initialisation, `sd_card_wait_transfer_ready()`) were restructured so the timeout is checked once per iteration with an explicit return. The 1000 ms windows and the command order are unchanged.
- The CSD `TRAN_SPEED` handling is left as it was: the four recognised codes (25, 50, 100 and 200 MHz) are validated before use, and a card reporting any other code fails `sd_card_probe()` instead of being assumed to run at 25 MHz. It is documented as a restriction in the README rather than changed in this release.

Known gaps tracked for 0.2.0: no Standard Capacity (CSD v1) support, no high-speed or 1.8 V signalling, no acceptance of `TRAN_SPEED` codes outside the four recognised ones, no reliable way to tell an empty socket from a card the library rejects (the probe result flattens both the CMD8 timeout and the CSD status), no yield hook for the busy-wait timeouts, and no second host port or mock-based test to prove the ops boundary is honest.
