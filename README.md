# Claupper

A one-handed Flipper Zero remote for [Claude Code](https://claude.ai/code). Five buttons. No keyboard required.

Built by [Kasen Sansonetti](https://github.com/w3t-wr3) & [Wetware Labs](https://WetwareOfficial.com).

## Install

```bash
npx claupper
```

One command installs everything:
- **Claude Code skill** → `~/.claude/skills/claupper/` (activate with `/claupper`)
- **Flipper Zero apps** → `~/claupper/` (BLE + USB `.fap` files)
- **Macro presets** → `~/claupper/macros/` (7 preset packs)

Then copy the right `.fap` to your Flipper's SD card and you're set.

---

## What's New in v0.24

- **Tour screen** — 4-page onboarding walkthrough on first launch with "don't ask again" option
- **Claude Code skill** — `/claupper` command installs via `npx claupper`, uses `AskUserQuestion` for native clickable buttons with built-in "Other" for free text
- **npx installer** — one command sets up skill + Flipper apps + macro presets
- **Improved UI** — text fits 128px screen, proper checkmark rendering, landscape tour pages

---

## Remote Mode

Every interaction with Claude Code boils down to choosing option 1, 2, or 3, then hitting Enter. Claupper maps these to the Flipper's d-pad:

| Button | Single Press | Double Press |
|--------|-------------|--------------|
| Left | `1` (approve/yes) | **Clear entire line** |
| Up | `2` (decline/no) | Page Up |
| Right | `3` (other/skip) | **Previous command** |
| OK | Enter (confirm) | **Switch window** (Cmd+\` / Alt+Tab) |
| Down | **Voice dictation** | Page Down |
| Back (short) | Return home | — |
| Back (long) | **Send Escape** | — |

### Double-Click Actions

The double-click layer is where it gets powerful:

- **Clear entire line** (Left+Left) — `Ctrl+A` then `Ctrl+K`. Wipes the entire input buffer.
- **Switch window** (OK+OK) — Cmd+\` on Mac, Alt+Tab on Windows/Linux.
- **Previous command** (Right+Right) — Up Arrow to recall last terminal command.
- **Page Up / Page Down** (Up+Up / Down+Down) — Scroll through long output.

### Voice Dictation

Down button triggers your OS dictation service (macOS Dictation, Windows Win+H). Talk to Claude Code through your Flipper — no typing.

### Visual Feedback

Every keypress flashes a label on the Flipper screen ("1", "Enter", "Clear", "Switch", "Dictate"). Haptic feedback pulses on every send (single pulse for regular, double for double-click).

---

## Claude Code Skill

The `/claupper` skill tells Claude to present every decision as exactly 3 numbered options using `AskUserQuestion`:

```
1. [What you most likely want — do it]
2. [Show details / slower path]
3. [Change direction / more options]
```

Claude predicts what you want based on codebase context and conversation history. Option 1 is always the most likely action. The built-in "Other" field provides free-text input — no button wasted on "type your own."

The skill also tells Claude to:
- Auto-continue when the next step is obvious
- Batch small decisions instead of asking one at a time
- Never stop without presenting choices
- Generate commit messages automatically

See [`skill/claupper/SKILL.md`](skill/claupper/SKILL.md) for the full spec.

---

## Manual Mode

Complete offline Claude Code reference on the Flipper's 128x64 screen. No internet needed.

Seven categories, 29 sections covering installation, workspace setup, commands, tools, workflows, and advanced topics (hooks, MCP, permissions, headless mode).

### Quiz Mode

Pick difficulty: Easy (8), Medium (16), or Hard (24). Multiple choice, shuffled each round. Mac-style answer modal, streak tracking, score percentage.

---

## Settings

| Setting | Values | Default | Effect |
|---------|--------|---------|--------|
| **Haptics** | ON / OFF | ON | Vibration on key send |
| **LED** | ON / OFF | ON | Blue=Remote, Green=Manual, Orange=Home |
| **OS** | Mac / Win / Linux | Mac | Platform-specific keybindings |

---

## Macros

Up to 20 custom macros from SD card. Two types:

- **Text macros** — typed character-by-character + Enter (e.g., `/commit`, `/doctor`)
- **Combo macros** — modifier key combos (e.g., `!Ctrl+C`, `!Cmd+K`)

**Preset packs** included in `macros/`:

| File | Style |
|------|-------|
| `default.txt` | Balanced mix of commands and prompts |
| `minimal.txt` | Slash commands only |
| `maximalist.txt` | Detailed natural-language prompts |
| `barebones.txt` | Just approval keys and basics |
| `workflow.txt` | Full dev cycle (init → code → test → commit) |
| `debugging.txt` | Bug hunting and diagnostics |
| `review.txt` | Code review prompts |

**Setup:** Copy a preset to `SD/apps_data/claude_remote_ble/macros.txt` (or `claude_remote_usb` for USB build).

---

## Tour Screen

First-launch onboarding walkthrough (4 pages, landscape):

1. **Quick Tour?** — opt-in screen with "don't ask again" checkbox
2. **Remote Control** — double-click, Escape, window switching
3. **Voice & Macros** — dictation, macro system, SD card editing
4. **Double-Click Guide** — full reference for all double-click actions
5. **More Features** — manual, quiz mode, OS settings

Persists preference to settings file. Skippable with Back button.

---

## Two Builds

| | **Claupper BLE** | **Claupper USB** |
|---|---|---|
| **File** | `claude_remote_ble.fap` | `claude_remote_usb.fap` |
| **Firmware** | Momentum or Unleashed | Stock (official) |
| **Connection** | Bluetooth wireless | USB cable |
| **Best for** | Couch mode, across the room | Desk use, no pairing needed |
| **SD path** | `apps/Bluetooth/` | `apps/Tools/` |

Same features, same source — just different HID transport. BLE build includes both Bluetooth and USB fallback.

---

## Flipper Setup

### BLE Version (Momentum / Unleashed)

1. Run `npx claupper` (or download from [Releases](https://github.com/Wet-wr-Labs/claupper/releases))
2. Copy `~/claupper/claude_remote_ble.fap` to `SD/apps/Bluetooth/` on your Flipper
3. Open from **Apps → Bluetooth → Claupper**
4. Pair via Bluetooth on your computer

### USB Version (Stock Firmware)

1. Run `npx claupper` (or download from [Releases](https://github.com/Wet-wr-Labs/claupper/releases))
2. Copy `~/claupper/claude_remote_usb.fap` to `SD/apps/Tools/` on your Flipper
3. Plug Flipper into your computer via USB
4. Open from **Apps → Tools → Claude Remote USB**

---

## Build From Source

Requires [ufbt](https://github.com/flipperdevices/flipperzero-ufbt):

```bash
ufbt                                    # builds both .fap files
ufbt launch APPID=claude_remote_ble     # deploy + run BLE version
ufbt launch APPID=claude_remote_usb     # deploy + run USB version
```

---

## Uninstall

```bash
npx claupper --uninstall
```

Removes the Claude Code skill and `~/claupper/` directory. Does not remove `.fap` files already on your Flipper SD card.

---

## Version History

| Version | Highlights |
|---------|-----------|
| **v0.24** | Tour screen, Claude Code skill (`/claupper`), `npx claupper` installer, AskUserQuestion integration |
| **v0.23** | Settings (Haptics/LED/OS), Windows + Linux support, macros, long-press Escape |
| **v0.2** | Quiz mode with difficulty picker, all multiple-choice, Mac-style answer modal |
| **v0.1** | Initial release — remote control, manual, double-click actions, voice dictation |

## License

[MIT](LICENSE)

## Links

- [Wetware Labs](https://WetwareOfficial.com)
- [Claude Code](https://claude.ai/code)
- [Momentum Firmware](https://momentum-fw.dev)
