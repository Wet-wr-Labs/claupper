# Claupper

A one-handed Flipper Zero remote for [Claude Code](https://claude.ai/code) + offline Claude Code manual & quiz.

Built by [Wetware Labs](https://WetwareOfficial.com).

## Why Claupper?

Claude Code is a terminal-based AI coding assistant. It asks questions, proposes changes, and waits for your input — but you're stuck at the keyboard. Claupper lets you **control the entire conversation from a Flipper Zero**, one-handed, from across the room.

Approve changes while you're reading docs on your phone. Decline from the couch. Dictate instructions by voice without touching your computer. Switch between terminal windows with Cmd+`. Nuke an entire input line instantly when Claude goes off-track.

Five buttons. No keyboard required.

## Remote Mode

Every interaction with Claude Code boils down to choosing option 1, 2, or 3, then hitting Enter. Claupper maps these to the Flipper's d-pad:

| Button | Single Press | Double Press |
|--------|-------------|--------------|
| Left | `1` (approve/yes) | **Clear entire line** |
| Up | `2` (decline/no) | Page Up |
| Right | `3` (other/skip) | **Previous command** (history recall) |
| OK | Enter (confirm) | **Cmd+`** (switch terminal window) |
| Down | **Voice dictation** | Page Down |
| Back | Return home | — |

### Double-Click Actions

The double-click layer is where it gets powerful:

- **Clear entire line** (double-Left) — Sends `Ctrl+A` then `Ctrl+K`: jumps to the start of the line and kills everything to the end. One gesture wipes the entire input buffer. Works in any terminal, any shell.
- **Cmd+`** (double-OK) — Instantly cycles between terminal windows without touching your computer. Bounce between Claude Code and your shell, your logs, your test runner — all from the Flipper.
- **Previous command** (double-Right) — Sends Up Arrow to recall the last terminal command. Re-run tests, restart servers, repeat builds.
- **Page Up / Page Down** (double-Up / double-Down) — Scroll through long Claude Code output without reaching for your keyboard.

### Voice Dictation

Single-press Down triggers your OS dictation service (macOS Dictation, Windows Speech Recognition). Talk to Claude Code through your Flipper — describe bugs, dictate instructions, explain what you want built. No typing.

### Visual Feedback

Every keypress flashes a rounded overlay on the Flipper screen showing exactly what was sent ("1", "Enter", "Dictate", "Clear", "Switch"). You always know what just happened.

## Manual Mode

A complete offline Claude Code reference guide on the Flipper's 128x64 screen. No internet, no phone, no computer needed.

Seven categories, 29 sections:

- **Getting Started** — Installing, first launch, system requirements
- **Workspace** — Project setup, CLAUDE.md, /init, .claude/ directory
- **Commands** — Navigation, sessions, config, debugging
- **Tools** — File ops, search, sub-agents, web
- **Advanced** — Hooks, MCP, permissions, headless mode
- **Workflows** — New project, debug & test, code review
- **Quiz** — 24-question multiple choice quiz

### Quiz Mode

Pick your difficulty from a classic Mac-style modal: Easy (8 questions), Medium (16), or Hard (24). All multiple choice, questions shuffled each round.

When you answer, a Mac-style modal pops up over the question showing whether you got it right and the correct answer — so you learn as you go. Tracks your score, percentage, and best streak.

## Claupper Mode

For the best experience, add `claupper_mode.md` to your project as `CLAUDE.md` (or append it to an existing one). This tells Claude Code to always present decisions as numbered 1/2/3 choices — so you can control everything from the remote without typing.

See [`claupper_mode.md`](claupper_mode.md) for the full template.

## Two Builds

| Build | Firmware | Transport | Name on Flipper |
|-------|----------|-----------|-----------------|
| `claude_remote_usb` | Stock | USB HID | Claupper USB |
| `claude_remote_ble` | Momentum / Unleashed | Bluetooth HID | Claupper BLE |

The BLE version is the primary build — wireless control from anywhere in the room. The USB version works on stock firmware for users who don't want to switch.

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

## Build From Source

Requires [ufbt](https://github.com/flipperdevices/flipperzero-ufbt):

```bash
ufbt                                    # builds both .fap files
ufbt launch APPID=claude_remote_usb     # deploy + run USB version
ufbt launch APPID=claude_remote_ble     # deploy + run BLE version
```

## License

[MIT](LICENSE)

## Links

- [Wetware Labs](https://WetwareOfficial.com)
- [Claude Code](https://claude.ai/code)
- [Momentum Firmware](https://momentum-fw.dev)
