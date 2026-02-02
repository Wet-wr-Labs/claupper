# Claupper

A one-handed Flipper Zero remote for [Claude Code](https://claude.ai/code) + offline Claude Code manual reader.

Built by [Wetware Labs](https://WetwareOfficial.com).

## What It Does

**Remote Mode** — Control Claude Code from your Flipper. Press numbered buttons to approve, decline, or pick options. Press Enter to confirm. Trigger voice dictation. Switch terminal tabs. All one-handed, from across the room.

**Manual Mode** — Browse an offline Claude Code reference guide on the Flipper's 128x64 display. Seven categories, 29 sections, and a 24-question multiple choice quiz to test your knowledge.

## Button Mapping (Remote Mode)

| Button | Single Press | Double Press |
|--------|-------------|--------------|
| Left | `1` (approve) | Clear line |
| Up | `2` (decline) | Page Up |
| Right | `3` (other) | Prev command |
| OK | Enter | Cmd+` (switch window) |
| Down | Dictation | Page Down |
| Back | Flip screen / home | — |

## Two Builds

| Build | Firmware | Transport | Name on Flipper |
|-------|----------|-----------|-----------------|
| `claude_remote_usb` | Stock | USB HID | Claupper USB |
| `claude_remote_ble` | Momentum / Unleashed | Bluetooth HID | Claupper BLE |

The BLE version is the primary build — wireless control, no cable needed.

## Install

### BLE Version (Momentum / Unleashed)

1. Install [Momentum](https://momentum-fw.dev/update) or Unleashed firmware
2. Download `claude_remote_ble.fap` from [Releases](https://github.com/Wet-wr-Labs/claupper/releases)
3. Copy to `SD Card/apps/Bluetooth/` on your Flipper
4. Open from **Apps → Bluetooth → Claupper BLE**
5. Pair via Bluetooth on your computer

### USB Version (Stock Firmware)

1. Download `claude_remote_usb.fap` from [Releases](https://github.com/Wet-wr-Labs/claupper/releases)
2. Copy to `SD Card/apps/Bluetooth/` on your Flipper
3. Plug Flipper into your computer via USB
4. Open from **Apps → Bluetooth → Claupper USB**

## Claupper Mode

For the best experience, add `claupper_mode.md` to your project as `CLAUDE.md` (or append it to an existing one). This tells Claude Code to always present decisions as numbered 1/2/3 choices — so you can control everything from the remote without typing.

See [`claupper_mode.md`](claupper_mode.md) for the full template.

## Build From Source

Requires [ufbt](https://github.com/flipperdevices/flipperzero-ufbt):

```bash
ufbt                                    # builds both .fap files
ufbt launch APPID=claude_remote_usb     # deploy + run USB version
ufbt launch APPID=claude_remote_ble     # deploy + run BLE version
```

## Manual Mode

Browse offline Claude Code docs on the Flipper screen:

- **Getting Started** — Installing, first launch, system requirements
- **Workspace** — Project setup, CLAUDE.md, /init, .claude/ directory
- **Commands** — Navigation, sessions, config, debugging
- **Tools** — File ops, search, sub-agents, web
- **Advanced** — Hooks, MCP, permissions, headless mode
- **Workflows** — New project, debug & test, code review
- **Quiz** — 24-question multiple choice quiz with answer modals

## License

[MIT](LICENSE)

## Links

- [Wetware Labs](https://WetwareOfficial.com)
- [Claude Code](https://claude.ai/code)
- [Momentum Firmware](https://momentum-fw.dev)
