# Claupper — Next Session: Manual + Quiz Update

**Status:** COMPLETE. Implemented and tested on hardware 2026-02-01.

**File to edit:** `claude_remote.c` (all changes in this single file)

---

## What We're Doing

Two tasks in one pass:
1. **Expand the compiled-in manual** — add missing Claude Code commands, features, and topics
2. **Improve the quiz** — add multiple-choice questions, more cards (12→24), shuffle, streak tracking

---

## Part 1: Manual Content Expansion

### Current state: 5 categories, 17 sections
### Target state: 7 categories, 29 sections

| Category | Current Sections | After Update |
|----------|-----------------|--------------|
| Getting Started (3→4) | Installing Claude, First Launch, System Requirements | + **Authentication** (Max sub vs API key, /login, /logout, /status) |
| Workspace (4→5) | Ideal Project Setup, CLAUDE.md Guide, /init Command, .claude/ Directory | + **Skills System** (.claude/skills/*.md, replaces custom slash commands) |
| Commands (4→6) | Navigation & Basics, Session Management, Configuration, Debugging | + **Slash Commands A-M** (/bug, /commit, /listen, /login, /logout, /memory, /model, /mcp) + **Slash Commands N-Z** (/permissions, /pr-comments, /rewind, /status, /vim) |
| Tools (3, unchanged) | File Operations, Search & Explore, Sub-agents & Web | -- |
| Workflows (3→4) | New Project Setup, Debug & Test, Code Review | + **Git & PRs** (/commit workflow, /pr-comments, PR creation) |
| **Advanced** (NEW, 4) | -- | **Permissions** (allow/deny, settings hierarchy), **MCP Servers** (Model Context Protocol, /mcp), **Hooks** (PreToolUse/PostToolUse, automation), **Extended Thinking** (--thinking, when to use) |
| **Headless & CI** (NEW, 3) | -- | **Headless Mode** (-p, piping, --output-format json), **CI Integration** (--no-input, exit codes, env vars), **Model Selection** (Opus/Sonnet/Haiku, /model, --model) |
| Quiz Mode | (unchanged menu entry) | |

### Constants to update
- `CATEGORY_COUNT` from 5 to 7
- `MENU_ITEM_COUNT` auto-updates (CATEGORY_COUNT + 1)
- Section counts in `categories[]` array entries

### Content rules
- All text <=30 chars/line for 128x64 display with FontSecondary
- Format identical to existing sections (static const ManualSection arrays with \n line breaks)

---

## Part 2: Quiz Improvements

### New QuizCard struct

Replace the current struct:
```c
// OLD
typedef struct {
    const char* description;
    const char* command;
} QuizCard;

// NEW
typedef enum {
    QuizTypeFlashcard,    // existing: description → reveal answer
    QuizTypeMultiChoice,  // new: question + 3 options, pick correct one
} QuizType;

typedef struct {
    QuizType type;
    const char* description;   // question text (both types)
    const char* command;       // correct answer text (both types)
    const char* option_a;      // Left button option (NULL for flashcard)
    const char* option_b;      // Up button option (NULL for flashcard)
    const char* option_c;      // Right button option (NULL for flashcard)
    uint8_t correct_option;    // 0=A(Left), 1=B(Up), 2=C(Right) — multi-choice only
} QuizCard;
```

### New state variables to add to ClaudeRemoteState

```c
uint8_t quiz_streak;                    // consecutive correct answers
uint8_t quiz_best_streak;              // best streak this session
uint8_t quiz_order[QUIZ_CARD_COUNT];   // shuffled card indices
int8_t  quiz_selected;                 // multi-choice selected: -1=none, 0/1/2
bool    quiz_answered;                 // multi-choice: showing feedback screen
```

### 24 quiz cards (was 12)

- **Cards 1-12:** Existing flashcards (same content, updated to new struct with NULL options)
- **Cards 13-16:** New flashcards for /model, /memory, /rewind, /commit
- **Cards 17-24:** New multiple-choice cards:
  - Where do Skills files live? (.claude/skills/ vs CLAUDE.md vs ~/.config/claude/)
  - Default model? (Opus vs Sonnet vs Haiku)
  - What does -p flag do? (Print mode vs Profile mode vs Plugin mode)
  - Settings hierarchy highest priority? (Project vs User vs Default)
  - What is MCP? (Model Context Protocol vs Manual Command Parser vs Memory Cache Protocol)
  - Which hook runs BEFORE a tool? (PostToolUse vs PreToolUse vs OnToolUse)
  - JSON output flag? (--json vs --output-format json vs --format=json)
  - Ctrl+C does what? (Copy text vs Cancel/interrupt vs Clear screen)

### Multiple-choice UX flow

1. Question + 3 options shown with `<` `^` `>` button labels
2. Press Left/Up/Right to immediately answer (no confirm step)
3. CORRECT!/WRONG! feedback shown, plus correct answer if wrong
4. Press OK to advance to next card

### Multiple-choice screen layout (128x64 landscape)

```
[Quiz 5/24]              [3/4 5x]    header + score + streak
─────────────────────────────────    y=13 separator
  Where do Skills                    question line 1
  files live?                        question line 2
  < .claude/skills/                  option A (Left button)
  ^ CLAUDE.md                        option B (Up button)
  > ~/.config/claude/                option C (Right button)
```

### Shuffle implementation

- Fisher-Yates shuffle using `furi_hal_random_get()` (hardware RNG)
- Called at quiz start and on retry
- Requires `#include <furi_hal_random.h>`
- `quiz_order[]` array maps quiz_index → actual card index

### Streak tracking

- Increments on correct answer, resets to 0 on wrong answer, unchanged on skip
- Shown in header as multiplier when >= 2: "3/4 5x"
- Completion screen shows "Best streak: N"

---

## Implementation Order

1. Add `#include <furi_hal_random.h>` at top
2. Add `QuizType` enum, expand `QuizCard` struct
3. Add new state variables to `ClaudeRemoteState`
4. Append new sections to `setup_sections[]` (Authentication), update count 3→4
5. Append new section to `workspace_sections[]` (Skills System), update count 4→5
6. Append 2 sections to `commands_sections[]` (Slash Commands A-M, N-Z), update count 4→6
7. Append section to `workflows_sections[]` (Git & PRs), update count 3→4
8. Add new `static const ManualSection advanced_sections[]` (4 entries: Permissions, MCP, Hooks, Extended Thinking)
9. Add new `static const ManualSection headless_sections[]` (3 entries: Headless, CI, Model Selection)
10. Update `categories[]` array to include Advanced and Headless & CI
11. Update `#define CATEGORY_COUNT 7`
12. Rewrite `quiz_cards[]` with all 24 cards in new struct format, update `#define QUIZ_CARD_COUNT 24`
13. Add `quiz_shuffle()` function (Fisher-Yates with furi_hal_random_get)
14. Add `draw_quiz_flashcard()` helper (extracted from current draw_manual_quiz)
15. Add `draw_quiz_multichoice()` helper (new: options with `<`/`^`/`>` labels, feedback screen)
16. Rewrite `draw_manual_quiz()` to dispatch by card type, show streak in header, show best streak on completion
17. Update quiz init in `handle_manual_categories()` to call quiz_shuffle() and reset new state vars
18. Rewrite `handle_manual_quiz()` with separate flashcard/multi-choice input handling + streak tracking

## Verification

```bash
cd /Users/myrm/home/FAP && ufbt
```

- Build both .fap targets (USB + BLE)
- If Flipper connected: `ufbt launch APPID=claude_remote_ble`
- Check: all 7 categories show and scroll, new sections display <=30 chars/line, quiz shuffles, both card types work, streak tracks correctly, completion screen shows best streak

---

## Key Constraints (don't forget)

- C only, no C++
- All manual content must be `static const` (compiled-in, zero malloc)
- <=30 chars per line for FontSecondary on 128x64 display
- Menu list shows 3 items at a time (existing scroll logic handles any MENU_ITEM_COUNT)
- Stack size is 2KB — keep stack buffers small (char buf[32] max)
- Use `#ifdef HID_TRANSPORT_BLE` for any transport-specific code
- BLE connection detection: `bt_set_status_changed_callback()`, NOT `furi_hal_bt_is_active()`
- Clean up all allocations on exit
