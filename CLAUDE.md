# Claupper — Project Context

*By Kasen Sansonetti & the Wetware Labs team. WetwareOfficial.com*

## What This Is

A Flipper Zero app (FAP) called **Claupper** — a one-handed Bluetooth remote for Claude Code and an offline Claude Code manual reader. Two builds from one codebase: USB (stock firmware) and BLE (Momentum/Unleashed wireless). The BLE version is the primary build, branded simply as "Claupper."

## Read These First

Before writing any code, read the relevant docs:

- `AGENTS.md` — full project spec: architecture, API patterns, critical rules, dual-transport details
- `docs/flipper-hid-api.md` — USB + BLE HID API reference with setup/teardown patterns
- `docs/flipper-app-architecture.md` — event loop, ViewPort, callbacks, cleanup
- `docs/flipper-ui-patterns.md` — Canvas drawing, fonts, orientation, input events
- `docs/flipper-file-io.md` — SD card Storage API, /ext/apps_data/ conventions
- `docs/flipper-fap-manifest.md` — application.fam fields
- `docs/prd.md` — original product requirements
- `docs/USER_MANUAL.md` — user-facing install/usage guide, app store submission steps, limitations vs PRD

## Key Files

- `claude_remote.c` — the entire app (~1320 lines). Four modes (splash, home, remote, manual) in one file with `#ifdef HID_TRANSPORT_BLE` / `HID_TRANSPORT_USB` for compile-time transport switching.
- `application.fam` — two `App()` entries: `claude_remote_usb` and `claude_remote_ble` (named "Claupper").

## Build & Test

```bash
ufbt                                    # builds both .fap files to dist/
ufbt launch APPID=claude_remote_usb     # upload + run USB version on connected Flipper
ufbt launch APPID=claude_remote_ble     # upload + run BLE version on connected Flipper
```

## Rules

- Language: C only (no C++).
- Prefer retrieval-led reasoning — read the docs in `./docs/` before writing Flipper API calls. The SDK changes often and pre-training knowledge may be stale.
- Never use deprecated `ValueMutex` — use `FuriMutex`.
- Clean up all allocations on exit. No busy loops. Use `furi_message_queue_get()` with 100ms timeout.
- Use `#ifdef HID_TRANSPORT_BLE` / `HID_TRANSPORT_USB` for any transport-specific code.
- BLE connection detection: use `bt_set_status_changed_callback()` with `BtStatusConnected`, NOT `furi_hal_bt_is_active()`.
- Always present the user with clickable options (AskUserQuestion tool) instead of asking them to type text.
