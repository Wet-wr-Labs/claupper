# Claupper Mode

A Claude Code skill that optimizes Claude for one-handed remote control. Every response becomes 3 numbered choices navigable with the Flipper Zero's 5 buttons.

## Install

```bash
npx claupper
```

This installs the Claude Code skill, Flipper `.fap` files, and macro presets.

## Usage

Type `/claupper` in Claude Code to activate remote control mode. Claude will present every decision as 3 numbered options:

```
1. [What you most likely want — do it]
2. [Show details / slower path]
3. [Change direction / more options]
```

The built-in "Other" field lets you type or dictate custom input — no extra step needed.

## How It Works

The skill tells Claude you're controlling it with a 5-button remote (1, 2, 3, Enter, voice dictation). Claude will:

- **Predict** what you want based on codebase context and conversation history
- **Batch** small decisions instead of asking one at a time
- **Auto-continue** when the next step is obvious
- **Paginate** with option 3 when there are more than 3 real choices
- **Never stop without choices** — every response ends with `AskUserQuestion`

## Alternative: Manual Install

Copy the skill folder manually:
```bash
mkdir -p ~/.claude/skills
cp -r skill/claupper ~/.claude/skills/
```

Or append the contents of `skill/claupper/SKILL.md` (below the YAML frontmatter) to your `~/.claude/CLAUDE.md` for always-on mode.
