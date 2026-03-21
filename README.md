# Agentic Remote

A one-handed Flipper Zero remote for [Claude Code](https://claude.ai/code) and AI coding agents. Five buttons. No keyboard required.

Built by [Kasen Sansonetti](https://github.com/w3t-wr3) & [Wetware Labs](https://WetwareOfficial.com).

**Fully dogfooded** — this entire app was built using the remote itself, controlling Claude Code from a Flipper Zero. In the spirit of Claude Code shipping as a CLI tool built with Claude, Agentic Remote was built with Agentic Remote.

---

## Install

```bash
npx claupper
```

One command installs everything:
- **Flipper Zero apps** → `~/claupper/` (BLE + USB `.fap` files)
- **Macro presets** → `~/claupper/macros/` (7 preset packs)
- **Claude Code skill** (optional) → `~/.claude/skills/claupper/` (activate with `/claupper`)

Then copy the `.fap` to your Flipper's SD card and you're set.

---

## How It Works

Claude Code is a terminal app. Every interaction is: read output, approve or decline, move on. Agentic Remote maps that entire workflow to five buttons.

Press 1 to approve, 2 to decline, 3 for alternatives, OK to confirm. When you need to type, hit Down for voice dictation. When you need to scroll, double-click Up or Down. Double-click OK to Cmd+\` between terminal windows. Need to fire off a `/commit` or a custom prompt? Hold Left+Down to jump into Macros, fully customizable from a text file on the SD card. The whole point is you never touch your keyboard.

Works great with mobile too. Switch between Mac, Windows, and Linux in Settings and all hotkeys adapt automatically (Cmd+\` vs Alt+Tab, etc).

Optionally, install the `/claupper` skill to have Claude present every decision as exactly 3 numbered options. One click per decision, no Enter needed.

---

## Button Map

### Remote Mode

| Button | Single | Double | Triple | Hold |
|--------|--------|--------|--------|------|
| **Left** | `1` (approve/yes) | Clear line (Ctrl+U) | End of line (Ctrl+N) | Backspace (repeat) |
| **Up** | `2` (decline/no) | Page Up | Up Arrow | — |
| **Right** | `3` (other/skip) | Previous command (Up Arrow) | Right Arrow | — |
| **OK** | Enter | Switch window (Cmd+\` / Alt+Tab) | — | — |
| **Down** | Voice dictation | Page Down | Down Arrow | — |
| **Back** | Return home | — | — | Send Escape |

### Combos

| Combo | Action |
|-------|--------|
| **Right + Down** (hold both) | Show hotkey overlay |
| **Left + Down** (hold both) | Open Macros menu |

### Double-Click Speed

Configurable in Settings: Normal (300ms), Slow (500ms), or Fast (200ms).

---

## Modes

### Remote

The main mode. Screen dims to save battery — stays visible but won't go black. Every keypress flashes a label ("1", "Enter", "Clear", "Switch") and pulses haptic feedback. All keystrokes sent over BLE (wireless) or USB depending on your build.

### Macros

Up from Home, or Left+Down from Remote. Up to 24 custom text macros loaded from SD card. Hold Up/Down to scroll continuously. Two types:

- **Text macros** — typed character-by-character (e.g., `/commit`, `Run the tests`)
- **Combo macros** — modifier key combos (e.g., `!Ctrl+C`, `!Cmd+K`)

**7 preset packs** included:

| File | Style |
|------|-------|
| `default.txt` | Balanced mix of commands and prompts |
| `minimal.txt` | Slash commands only |
| `maximalist.txt` | Detailed natural-language prompts |
| `barebones.txt` | Just approval keys and basics |
| `workflow.txt` | Full dev cycle (init → code → test → commit) |
| `debugging.txt` | Bug hunting and diagnostics |
| `review.txt` | Code review prompts |

Copy a preset to `SD/apps_data/claude_remote_ble/macros.txt` (or `claude_remote_usb` for USB build). Edit on the SD card anytime.

### Manual

Down from Home. Complete offline Claude Code reference — 7 categories, 29 sections covering installation, workspace setup, commands, tools, workflows, hooks, MCP, permissions, and headless mode. No internet needed.

Includes a **quiz mode** with three difficulties (Easy/Medium/Hard), multiple choice, shuffled each round, streak tracking.

### Settings

Left from Home.

| Setting | Values | Default |
|---------|--------|---------|
| **Haptics** | ON / OFF | ON |
| **LED** | ON / OFF | ON |
| **OS** | Mac / Win / Linux | Mac |
| **Double-Click Speed** | Normal / Slow / Fast | Normal |
| **Tour** | Show / Don't show | Show |

---

## Two Builds, One Codebase

| | **Agentic Remote BLE** | **Agentic Remote USB** |
|---|---|---|
| **File** | `claude_remote_ble.fap` | `claude_remote_usb.fap` |
| **Firmware** | Momentum or Unleashed | Stock (official) |
| **Connection** | Bluetooth wireless | USB cable |
| **Best for** | Couch mode, across the room | Desk use, no pairing needed |
| **SD path** | `apps/Bluetooth/` | `apps/USB/` |

Same C source file, same features — compile-time `#ifdef` switching selects the HID transport. The BLE build includes a USB fallback (toggle with Left+Down).

---

## Setup

### BLE (Momentum / Unleashed)

1. `npx claupper` (or grab from [Releases](https://github.com/Wet-wr-Labs/claupper/releases))
2. Copy `claude_remote_ble.fap` to `SD/apps/Bluetooth/`
3. Open **Apps → Bluetooth → Agentic Remote**
4. Pair via Bluetooth on your computer

### USB (Stock Firmware)

1. `npx claupper` (or grab from [Releases](https://github.com/Wet-wr-Labs/claupper/releases))
2. Copy `claude_remote_usb.fap` to `SD/apps/USB/`
3. Plug Flipper into your computer
4. Open **Apps → USB → Agentic Remote USB**

### First Launch

A 4-page tour walks you through the controls. Dismiss once and it won't come back (configurable in Settings).

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

## Claude Code Skill (Optional)

The `/claupper` skill activates Agentic Remote Mode in Claude Code:

```
1. [What you most likely want — do it]
2. [Show details / slower path]
3. [Change direction / more options]
```

Every `AskUserQuestion` call includes a built-in "Other" field for free-text input — no button wasted on "type your own."

The skill tells Claude to:
- **Predict** what you want — Option 1 is always the most likely action
- **Auto-continue** when the next step is obvious
- **Batch** small decisions instead of asking one at a time
- **Never stop** without presenting choices
- **Generate commit messages** automatically

See [`skill/claupper/SKILL.md`](skill/claupper/SKILL.md) for the full spec.

---

## Version History

| Version | Highlights |
|---------|-----------|
| **v0.27** | Fix combo state leaking between modes, portrait macros screen (9 visible), hold-to-scroll in macros, hint bar shows both combos, macros Back returns to Remote |
| **v0.26** | Dim backlight in remote modes, BLE default, triple-tap Ctrl+N, restored dictation, BLE release_all |
| **v0.25** | Dual-transport toggle (Left+Down), BLE crash fix, macro improvements |
| **v0.24** | Tour screen, Claude Code skill (`/claupper`), `npx claupper` installer |
| **v0.23** | Settings (Haptics/LED/OS), Windows + Linux, macros, long-press Escape |
| **v0.2** | Quiz mode with difficulty picker, Mac-style answer modal |
| **v0.1** | Initial release — remote control, manual, double-click actions, voice dictation |

## License

[MIT](LICENSE)

## Links

- [Wetware Labs](https://WetwareOfficial.com)
- [Claude Code](https://claude.ai/code)
- [Momentum Firmware](https://momentum-fw.dev)
