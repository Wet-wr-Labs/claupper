#include <furi.h>
#include <furi_hal.h>
#include <furi_hal_random.h>
#include <furi_hal_usb_hid.h>
#include <gui/gui.h>
#include <gui/view_port.h>
#include <input/input.h>
#include <notification/notification.h>
#include <notification/notification_messages.h>

#ifdef HID_TRANSPORT_BLE
#include <bt/bt_service/bt.h>
#include <extra_profiles/hid_profile.h>
#endif

#ifdef HID_TRANSPORT_BLE
#include <claude_remote_ble_icons.h>
#else
#include <claude_remote_usb_icons.h>
#endif

#define TAG "CRemote"

/* ── Claude orange LED ── */

static const NotificationMessage message_green_128 = {
    .type = NotificationMessageTypeLedGreen,
    .data.led.value = 128,
};

static const NotificationSequence sequence_solid_orange = {
    &message_red_255,
    &message_green_128,
    &message_blue_0,
    &message_do_not_reset,
    NULL,
};

/* ── Modes ── */

typedef enum {
    ModeSplash,
    ModeHome,
    ModeRemote,
    ModeManual,
#ifndef HID_TRANSPORT_BLE
    ModeBlePromo,
#endif
} AppMode;

typedef enum {
    ManualViewCategories,
    ManualViewSections,
    ManualViewRead,
    ManualViewQuiz,
} ManualView;

/* ── Manual content structures (all const, zero malloc) ── */

typedef struct {
    const char* title;
    const char* content;
} ManualSection;

typedef struct {
    const char* name;
    const ManualSection* sections;
    uint8_t section_count;
} ManualCategory;

typedef enum {
    QuizTypeFlashcard,
    QuizTypeMultiChoice,
} QuizType;

typedef struct {
    QuizType type;
    const char* description;
    const char* command;
    const char* option_a;
    const char* option_b;
    const char* option_c;
    uint8_t correct_option;
} QuizCard;

/* ══════════════════════════════════════════════════════════
 *  Compiled-in manual content (≤30 chars/line for 128x64)
 * ══════════════════════════════════════════════════════════ */

/* ── Getting Started ── */

static const ManualSection setup_sections[] = {
    {"Installing Claude",
     "Install via npm:\n"
     " npm i -g @anthropic-ai/\n"
     "   claude-code\n\n"
     "Or via brew:\n"
     " brew install claude-code\n\n"
     "Requires Node.js 18+\n"
     "and an Anthropic API key.\n\n"
     "Set your key:\n"
     " export ANTHROPIC_API_KEY\n"
     "   =sk-ant-...\n\n"
     "Or log in with:\n"
     " claude login\n"},

    {"First Launch",
     "Open any terminal and type:\n"
     " claude\n\n"
     "Claude starts in your\n"
     "current directory. It reads\n"
     "all files in your project.\n\n"
     "Start fresh:\n"
     " claude --new\n\n"
     "Resume last session:\n"
     " claude --continue\n\n"
     "Print mode (no chat):\n"
     " claude -p \"your query\"\n\n"
     "Type your request at the\n"
     "> prompt and press Enter.\n"},

    {"System Requirements",
     "Supported platforms:\n"
     " macOS, Linux, WSL\n\n"
     "Requirements:\n"
     " Node.js 18 or later\n"
     " npm or brew\n"
     " Terminal with UTF-8\n\n"
     "Recommended:\n"
     " Git installed\n"
     " Project in a git repo\n"
     " CLAUDE.md in project root\n\n"
     "API key or Claude login\n"
     "required for operation.\n"},

    {"Authentication",
     "Two ways to authenticate:\n\n"
     "1. Claude Max subscription\n"
     "   claude login\n"
     "   Opens browser to sign in\n\n"
     "2. API key\n"
     "   export ANTHROPIC_API_KEY\n"
     "     =sk-ant-...\n\n"
     "Manage sessions:\n"
     " /login   sign in again\n"
     " /logout  sign out\n"
     " /status  check auth state\n\n"
     "Max: flat monthly rate.\n"
     "API: pay per token used.\n"},
};

/* ── Workspace ── */

static const ManualSection workspace_sections[] = {
    {"Ideal Project Setup",
     "Best results when Claude\n"
     "can see your full project.\n\n"
     "Recommended layout:\n"
     " project/\n"
     "   CLAUDE.md\n"
     "   .claude/\n"
     "   src/\n"
     "   tests/\n"
     "   package.json\n\n"
     "Always run claude from\n"
     "your project root dir.\n\n"
     "Keep projects in git so\n"
     "Claude can see history\n"
     "and you can revert.\n"},

    {"CLAUDE.md Guide",
     "CLAUDE.md is a file Claude\n"
     "reads automatically when\n"
     "starting in a directory.\n\n"
     "Use it to tell Claude:\n"
     " - Project description\n"
     " - Architecture notes\n"
     " - Coding conventions\n"
     " - Build/test commands\n"
     " - Key file locations\n\n"
     "Claude reads CLAUDE.md at\n"
     "every session start. No\n"
     "need to repeat context.\n\n"
     "Place it in project root.\n"},

    {"The /init Command",
     "/init creates a CLAUDE.md\n"
     "for your project.\n\n"
     "Usage:\n"
     " 1. Open claude in your\n"
     "    project directory\n"
     " 2. Type /init\n"
     " 3. Claude analyzes your\n"
     "    codebase\n"
     " 4. Generates a CLAUDE.md\n\n"
     "The file includes:\n"
     " - Tech stack details\n"
     " - Project structure\n"
     " - Build commands\n"
     " - Coding patterns found\n\n"
     "Edit it to add your own\n"
     "instructions or rules.\n"},

    {".claude/ Directory",
     "The .claude/ directory\n"
     "stores project settings.\n\n"
     "Structure:\n"
     " .claude/\n"
     "   settings.json\n"
     "   skills/\n"
     "     custom_skill.md\n\n"
     "settings.json:\n"
     " Stores allowed tools\n"
     " and permissions.\n\n"
     "skills/:\n"
     " Custom skill files\n"
     " Claude loads on start.\n\n"
     "Commit .claude/ to share\n"
     "config with your team.\n"},

    {"Skills System",
     "Skills are reusable prompts\n"
     "Claude loads automatically.\n\n"
     "Location:\n"
     " .claude/skills/*.md\n\n"
     "Each .md file is a skill.\n"
     "Claude reads them at start\n"
     "like CLAUDE.md.\n\n"
     "Use for:\n"
     " - Custom workflows\n"
     " - Code review rules\n"
     " - Response templates\n"
     " - Domain knowledge\n\n"
     "Skills replace the old\n"
     "custom slash commands.\n"},
};

/* ── Commands ── */

static const ManualSection commands_sections[] = {
    {"Navigation & Basics",
     "Key shortcuts:\n"
     " Ctrl+C Cancel/interrupt\n"
     " Ctrl+D Exit Claude Code\n"
     " Esc    Cancel input\n"
     " Tab    Autocomplete\n\n"
     "Approval prompts:\n"
     " 1  Yes / approve\n"
     " 2  No / decline\n"
     " 3  Other / alternative\n\n"
     "Scrolling:\n"
     " Up/Down for history\n\n"
     "Voice (macOS):\n"
     " Down  Start Dictation\n"},

    {"Session Management",
     "/clear\n"
     " Wipe conversation history\n"
     " Start fresh in same dir\n\n"
     "/compact\n"
     " Summarize conversation\n"
     " Reduces token usage\n"
     " Use when context is big\n\n"
     "/cost\n"
     " Show token usage and\n"
     " estimated cost for\n"
     " current session\n\n"
     "Sessions persist. Use\n"
     " --continue to resume\n"
     " --new to start fresh\n"},

    {"Configuration",
     "/config\n"
     " Open settings editor\n"
     " Set model, theme, key\n\n"
     "/terminal-setup\n"
     " Fix terminal rendering\n"
     " and display issues\n\n"
     "Key settings:\n"
     " Model selection\n"
     " Auto-approve patterns\n"
     " Theme (light/dark)\n"
     " Notification sounds\n\n"
     "Settings stored in:\n"
     " ~/.claude/config.json\n"},

    {"Debugging",
     "/doctor\n"
     " Diagnose setup issues\n"
     " Check API connection\n"
     " Verify permissions\n\n"
     "Common issues:\n\n"
     " \"API key not found\"\n"
     "  -> export your key\n\n"
     " \"Permission denied\"\n"
     "  -> Check /config\n\n"
     " Slow responses:\n"
     "  -> /compact to reduce\n"
     "     conversation size\n\n"
     " Display broken:\n"
     "  -> /terminal-setup\n"},

    {"Slash Commands A-M",
     "/bug\n"
     " Report a Claude Code bug\n\n"
     "/commit\n"
     " Auto-generate a commit\n"
     " message and commit\n\n"
     "/listen\n"
     " Pause and wait for\n"
     " file changes to resume\n\n"
     "/login\n"
     " Sign in to Anthropic\n\n"
     "/logout\n"
     " Sign out of session\n\n"
     "/memory\n"
     " Edit CLAUDE.md quickly\n\n"
     "/model\n"
     " Switch AI model mid-chat\n"
     " Opus, Sonnet, or Haiku\n\n"
     "/mcp\n"
     " Manage MCP servers\n"},

    {"Slash Commands N-Z",
     "/permissions\n"
     " View and edit tool\n"
     " access for this session\n\n"
     "/pr-comments\n"
     " Fetch GitHub PR review\n"
     " comments into session\n\n"
     "/rewind\n"
     " Undo to a previous\n"
     " point in conversation\n\n"
     "/status\n"
     " Show auth, model, and\n"
     " session info\n\n"
     "/vim\n"
     " Toggle vim keybindings\n"
     " in the input editor\n"},
};

/* ── Tools ── */

static const ManualSection tools_sections[] = {
    {"File Operations",
     "Read\n"
     " View file contents\n"
     " Supports images & PDFs\n\n"
     "Write\n"
     " Create new files\n"
     " Overwrites if exists\n\n"
     "Edit\n"
     " Modify existing files\n"
     " Find and replace text\n"
     " Preserves context\n\n"
     "NotebookEdit\n"
     " Edit Jupyter notebooks\n"
     " Add/remove/replace cells\n"},

    {"Search & Explore",
     "Grep\n"
     " Search file contents\n"
     " Regex pattern matching\n"
     " Filter by file type\n\n"
     "Glob\n"
     " Find files by name\n"
     " Pattern matching\n"
     " e.g. \"**/*.ts\"\n\n"
     "Task (Explore agent)\n"
     " Search across codebase\n"
     " Find implementations\n"
     " Answer architecture Q's\n\n"
     "Let Claude search rather\n"
     "than pasting file paths.\n"},

    {"Sub-agents & Web",
     "Task\n"
     " Launch sub-agents:\n"
     " Bash, Explore, Plan,\n"
     " general-purpose\n\n"
     "Bash\n"
     " Run shell commands\n"
     " Git, npm, tests, builds\n\n"
     "WebFetch\n"
     " Fetch and analyze URLs\n\n"
     "WebSearch\n"
     " Search the internet\n"
     " Get current info\n\n"
     "Agents run independently\n"
     "and return results.\n"},
};

/* ── Workflows ── */

static const ManualSection workflows_sections[] = {
    {"New Project Setup",
     "Step 1:\n"
     " claude --new\n\n"
     "Step 2:\n"
     " Type /init\n"
     " Creates CLAUDE.md\n\n"
     "Step 3:\n"
     " Describe what you want.\n"
     " Be specific about\n"
     " your tech stack.\n\n"
     "Step 4:\n"
     " Approve actions with 1\n"
     " Decline with 2\n\n"
     "Tip: Commit after each\n"
     "major milestone.\n"},

    {"Debug & Test",
     "Step 1:\n"
     " Paste error or test\n"
     " output into Claude.\n\n"
     "Step 2:\n"
     " Claude reads relevant\n"
     " source files.\n\n"
     "Step 3:\n"
     " Review proposed fix.\n"
     " Press 1 to approve\n"
     " or 2 to decline.\n\n"
     "Step 4:\n"
     " Run tests to verify.\n\n"
     "Tip: /compact if the\n"
     "conversation gets long.\n"},

    {"Code Review",
     "Step 1:\n"
     " Type /review or paste\n"
     " a GitHub PR URL.\n\n"
     "Step 2:\n"
     " Claude reads all\n"
     " changes in the diff.\n\n"
     "Step 3:\n"
     " Get feedback on:\n"
     "  - Bug risks\n"
     "  - Style issues\n"
     "  - Performance\n"
     "  - Missing tests\n\n"
     "Step 4:\n"
     " Ask Claude to fix\n"
     " issues it found.\n"},

    {"Git & PRs",
     "Commit workflow:\n"
     " 1. Make changes w/ Claude\n"
     " 2. Type /commit\n"
     " 3. Claude writes message\n"
     " 4. Approve to commit\n\n"
     "PR review:\n"
     " Paste a PR URL or use\n"
     " /pr-comments to fetch\n"
     " review feedback.\n\n"
     "Tips:\n"
     " Keep project in git repo\n"
     " Commit after milestones\n"
     " Claude reads git history\n"
     " for better context.\n"},
};

/* ── Advanced ── */

static const ManualSection advanced_sections[] = {
    {"Permissions",
     "Claude asks before using\n"
     "tools like Bash or Write.\n\n"
     "Settings hierarchy:\n"
     " 1. Project .claude/\n"
     "    settings.json\n"
     " 2. User ~/.claude/\n"
     "    settings.json\n"
     " 3. Defaults\n\n"
     "In settings.json:\n"
     " \"allowedTools\": [...]\n"
     " \"deniedTools\":  [...]\n\n"
     "/permissions to view\n"
     "current session rules.\n"},

    {"MCP Servers",
     "MCP = Model Context\n"
     "Protocol. Connect external\n"
     "tools to Claude Code.\n\n"
     "Examples:\n"
     " Database queries\n"
     " API integrations\n"
     " Custom data sources\n\n"
     "Setup:\n"
     " /mcp to manage servers\n"
     " Configure in settings\n"
     " or .claude/settings\n\n"
     "MCP servers run as local\n"
     "processes Claude can call\n"
     "like built-in tools.\n"},

    {"Hooks",
     "Hooks run your scripts\n"
     "before/after Claude acts.\n\n"
     "Types:\n"
     " PreToolUse  before tool\n"
     " PostToolUse after tool\n\n"
     "Config in settings.json:\n"
     " \"hooks\": {\n"
     "   \"PreToolUse\": [{\n"
     "     \"command\": \"./lint\"\n"
     "   }]\n"
     " }\n\n"
     "Use cases:\n"
     " Auto-lint on file save\n"
     " Run tests after edit\n"
     " Enforce code standards\n"},

    {"Extended Thinking",
     "Claude can think deeply\n"
     "before responding.\n\n"
     "When active, Claude uses\n"
     "extra tokens reasoning\n"
     "through hard problems.\n\n"
     "Enable:\n"
     " Type \"think\" in prompt\n"
     " or toggle via /model\n\n"
     "Best for:\n"
     " Complex architecture\n"
     " Multi-step debugging\n"
     " Tricky logic problems\n\n"
     "Uses more tokens but\n"
     "improves output quality.\n"},
};

/* ── Headless & CI ── */

static const ManualSection headless_sections[] = {
    {"Headless Mode",
     "Run Claude without chat:\n"
     " claude -p \"your query\"\n\n"
     "Pipe input:\n"
     " cat file | claude -p\n"
     "   \"review this code\"\n\n"
     "JSON output:\n"
     " claude -p \"query\"\n"
     "   --output-format json\n\n"
     "Returns structured result\n"
     "for scripting.\n\n"
     "No interactive prompts.\n"
     "No approval needed for\n"
     "read-only operations.\n"},

    {"CI Integration",
     "Use Claude in CI:\n\n"
     " claude -p \"review PR\"\n"
     "   --no-input\n\n"
     "Key flags:\n"
     " -p         print mode\n"
     " --no-input no prompts\n"
     " --model    pick model\n"
     " --output-format json\n\n"
     "Set ANTHROPIC_API_KEY in\n"
     "CI environment variables.\n\n"
     "Exit codes:\n"
     " 0 = success\n"
     " 1 = error\n\n"
     "Great for automated review\n"
     "and code generation.\n"},

    {"Model Selection",
     "Switch models anytime:\n"
     " /model in chat\n"
     " --model flag at launch\n\n"
     "Available models:\n\n"
     " Opus (most capable)\n"
     "  Best reasoning\n"
     "  Highest cost\n\n"
     " Sonnet (balanced)\n"
     "  Good speed + quality\n"
     "  Default model\n\n"
     " Haiku (fastest)\n"
     "  Quick responses\n"
     "  Lowest cost\n\n"
     "Pick based on task\n"
     "complexity and budget.\n"},
};

/* ── Category index ── */

static const ManualCategory categories[] = {
    {"Getting Started", setup_sections, 4},
    {"Workspace", workspace_sections, 5},
    {"Commands", commands_sections, 6},
    {"Tools", tools_sections, 3},
    {"Workflows", workflows_sections, 4},
    {"Advanced", advanced_sections, 4},
    {"Headless & CI", headless_sections, 3},
};

#define CATEGORY_COUNT 7
#define MENU_ITEM_COUNT (CATEGORY_COUNT + 1) /* +1 for Quiz */

/* ── Quiz cards ── */

static const QuizCard quiz_cards[] = {
    /* existing flashcards */
    {QuizTypeFlashcard, "Show help and\navailable commands",
     "/help", NULL, NULL, NULL, 0},
    {QuizTypeFlashcard, "Clear conversation\nhistory",
     "/clear", NULL, NULL, NULL, 0},
    {QuizTypeFlashcard, "Summarize context to\nreduce token usage",
     "/compact", NULL, NULL, NULL, 0},
    {QuizTypeFlashcard, "Open configuration\nsettings editor",
     "/config", NULL, NULL, NULL, 0},
    {QuizTypeFlashcard, "Show token usage\nand session cost",
     "/cost", NULL, NULL, NULL, 0},
    {QuizTypeFlashcard, "Diagnose setup issues\nand check API",
     "/doctor", NULL, NULL, NULL, 0},
    {QuizTypeFlashcard, "Create CLAUDE.md\nfor your project",
     "/init", NULL, NULL, NULL, 0},
    {QuizTypeFlashcard, "Review PR or\ncode changes",
     "/review", NULL, NULL, NULL, 0},
    {QuizTypeFlashcard, "Fix terminal display\nand rendering",
     "/terminal-setup", NULL, NULL, NULL, 0},
    {QuizTypeFlashcard, "Start a brand new\nsession from scratch",
     "claude --new", NULL, NULL, NULL, 0},
    {QuizTypeFlashcard, "Resume your previous\nconversation",
     "claude --continue", NULL, NULL, NULL, 0},
    {QuizTypeFlashcard, "Run a one-shot query\nwithout chat mode",
     "claude -p \"query\"", NULL, NULL, NULL, 0},

    /* new flashcards */
    {QuizTypeFlashcard, "Switch AI model\nduring a chat session",
     "/model", NULL, NULL, NULL, 0},
    {QuizTypeFlashcard, "Edit your project's\nCLAUDE.md memory file",
     "/memory", NULL, NULL, NULL, 0},
    {QuizTypeFlashcard, "Undo to a previous\npoint in conversation",
     "/rewind", NULL, NULL, NULL, 0},
    {QuizTypeFlashcard, "Generate commit msg\nand commit changes",
     "/commit", NULL, NULL, NULL, 0},

    /* multi-choice */
    {QuizTypeMultiChoice, "Where do Skills\nfiles live?",
     ".claude/skills/",
     ".claude/skills/", "CLAUDE.md", "~/.config/claude/", 0},
    {QuizTypeMultiChoice, "Which model is the\ndefault for Claude?",
     "Sonnet",
     "Opus", "Sonnet", "Haiku", 1},
    {QuizTypeMultiChoice, "What does the -p\nflag do?",
     "Print mode (no chat)",
     "Print mode", "Profile mode", "Plugin mode", 0},
    {QuizTypeMultiChoice, "Settings hierarchy\nhighest priority?",
     "Project settings",
     "Project", "User", "Default", 0},
    {QuizTypeMultiChoice, "What is MCP?",
     "Model Context Protocol",
     "Model Context", "Manual Command", "Memory Cache", 0},
    {QuizTypeMultiChoice, "Which hook runs\nBEFORE a tool?",
     "PreToolUse",
     "PostToolUse", "PreToolUse", "OnToolUse", 1},
    {QuizTypeMultiChoice, "How to get JSON\noutput from CLI?",
     "--output-format json",
     "--json", "--output-format", "--format=json", 1},
    {QuizTypeMultiChoice, "Ctrl+C in Claude\nCode does what?",
     "Cancel/interrupt",
     "Copy text", "Cancel/interrupt", "Clear screen", 1},
};

#define QUIZ_CARD_COUNT 24

/* ── App state ── */

typedef struct {
    AppMode mode;
    bool is_flipped;
    bool hid_connected;
    FuriMutex* mutex;

#ifdef HID_TRANSPORT_BLE
    bool use_ble;
    bool ble_connected;
    Bt* bt;
    FuriHalBleProfileBase* ble_profile;
#endif
    FuriHalUsbInterface* usb_prev;

    /* manual navigation */
    ManualView manual_view;
    uint8_t cat_index;
    uint8_t section_index;
    int16_t scroll_offset;

    /* quiz */
    uint8_t quiz_index;
    bool quiz_revealed;
    uint8_t quiz_correct;
    uint8_t quiz_total;
    uint8_t quiz_streak;
    uint8_t quiz_best_streak;
    uint8_t quiz_order[24]; /* QUIZ_CARD_COUNT — shuffled indices */
    int8_t  quiz_selected;  /* multi-choice: -1=none, 0/1/2 */
    bool    quiz_answered;  /* multi-choice: showing feedback */

    /* double-click detection (remote mode) */
    InputKey dc_key;
    uint32_t dc_tick;
    bool dc_pending;

    /* visual feedback flash */
    uint32_t flash_tick;
    const char* flash_label;

    /* splash screen */
    uint32_t splash_start;
} ClaudeRemoteState;

/* ── Utility ── */

static uint16_t count_lines(const char* text) {
    uint16_t count = 1;
    for(const char* p = text; *p; p++) {
        if(*p == '\n') count++;
    }
    return count;
}

static void quiz_shuffle(ClaudeRemoteState* state) {
    for(uint8_t i = 0; i < QUIZ_CARD_COUNT; i++) {
        state->quiz_order[i] = i;
    }
    for(uint8_t i = QUIZ_CARD_COUNT - 1; i > 0; i--) {
        uint8_t j = furi_hal_random_get() % (i + 1);
        uint8_t tmp = state->quiz_order[i];
        state->quiz_order[i] = state->quiz_order[j];
        state->quiz_order[j] = tmp;
    }
}

/* ── BLE status callback ── */

#ifdef HID_TRANSPORT_BLE
static void bt_status_callback(BtStatus status, void* context) {
    ClaudeRemoteState* state = (ClaudeRemoteState*)context;
    state->ble_connected = (status == BtStatusConnected);
    if(state->use_ble) {
        state->hid_connected = state->ble_connected;
    }
    FURI_LOG_I(TAG, "BT status: %d, connected: %d", status, state->ble_connected);
}
#endif

/* ── HID helpers ── */

#define HID_CONSUMER_DICTATION 0x00CF /* Consumer Page: Voice Command (triggers Edit > Start Dictation on macOS) */

#ifdef HID_TRANSPORT_BLE
static void send_hid_key_ble(FuriHalBleProfileBase* profile, uint16_t keycode) {
    ble_profile_hid_kb_press(profile, keycode);
    furi_delay_ms(150);
    ble_profile_hid_kb_release(profile, keycode);
}
static void send_consumer_key_ble(FuriHalBleProfileBase* profile, uint16_t usage) {
    ble_profile_hid_consumer_key_press(profile, usage);
    furi_delay_ms(150);
    ble_profile_hid_consumer_key_release(profile, usage);
}
#endif

static void send_hid_key_usb(uint16_t keycode) {
    furi_hal_hid_kb_press(keycode);
    furi_delay_ms(50);
    furi_hal_hid_kb_release(keycode);
}
static void send_consumer_key_usb(uint16_t usage) {
    furi_hal_hid_consumer_key_press(usage);
    furi_delay_ms(50);
    furi_hal_hid_consumer_key_release(usage);
}

/* ── WETWARE logo bitmap (128x20, XBM format) ── */

#define WETWARE_LOGO_W 128
#define WETWARE_LOGO_H 20

static const uint8_t wetware_logo[] = {
    0x3f, 0x1f, 0x1f, 0xff, 0xff, 0xff, 0xff, 0xef, 0xe3, 0xe3, 0xe3, 0x03, 0xfe, 0x1f, 0xfc, 0xff,
    0x3e, 0x1e, 0x9f, 0xdf, 0xff, 0xff, 0xff, 0xef, 0xe7, 0xe3, 0xe1, 0x07, 0xfe, 0x3f, 0xfc, 0xff,
    0x3e, 0x3e, 0x8f, 0xcf, 0xff, 0xff, 0xff, 0xcf, 0xc7, 0xf3, 0xf1, 0x07, 0xfe, 0x7f, 0xfc, 0xff,
    0x7e, 0x3e, 0x8f, 0xcf, 0xff, 0xff, 0xff, 0xcf, 0xc7, 0xf7, 0xf1, 0x07, 0xfe, 0xff, 0xfc, 0xff,
    0x7c, 0xbc, 0xcf, 0xcf, 0xff, 0xff, 0xff, 0xcf, 0xcf, 0xf7, 0xf0, 0x0f, 0xfe, 0xff, 0xfc, 0xff,
    0x7c, 0xfc, 0xc7, 0x07, 0xc0, 0x07, 0x7e, 0x80, 0x8f, 0xff, 0xf8, 0x0f, 0x3e, 0xf8, 0x7d, 0x00,
    0xf8, 0xfc, 0xc7, 0x07, 0xc0, 0x07, 0x7e, 0x80, 0x8f, 0xff, 0xf8, 0x0f, 0x3e, 0xf8, 0x7d, 0x00,
    0xf8, 0xf8, 0xe7, 0x07, 0xc0, 0x07, 0x7e, 0x80, 0x9f, 0x7f, 0x78, 0x1f, 0x3e, 0xf8, 0xfd, 0x00,
    0xf8, 0xf8, 0xe3, 0x83, 0xff, 0x07, 0x7e, 0x00, 0x1f, 0x7f, 0x7c, 0x1f, 0x7e, 0xf8, 0xfc, 0x7f,
    0xf0, 0xf9, 0xe3, 0x83, 0xff, 0x07, 0x7e, 0x00, 0x1f, 0x7f, 0x3c, 0x1e, 0xfe, 0xff, 0xfc, 0x7f,
    0xf0, 0xf9, 0xe3, 0x81, 0xff, 0x07, 0x7e, 0x00, 0x3e, 0x7f, 0x3c, 0x3e, 0xfe, 0xff, 0xfc, 0x7f,
    0xf0, 0xf9, 0xf7, 0x81, 0xff, 0x07, 0x7e, 0x00, 0xbe, 0x7f, 0x3e, 0x3e, 0xfe, 0x7f, 0xfc, 0x3f,
    0xe0, 0xff, 0xf7, 0x01, 0xc0, 0x07, 0x7e, 0x00, 0xbe, 0xff, 0xfe, 0x3f, 0xfe, 0x3f, 0x7c, 0x00,
    0xe0, 0xff, 0xff, 0x00, 0xc0, 0x07, 0x7e, 0x00, 0xfc, 0xff, 0xff, 0x7f, 0xfe, 0x3f, 0x7c, 0x00,
    0xc0, 0xbf, 0xff, 0x00, 0xc0, 0x07, 0x7e, 0x00, 0xfc, 0xf7, 0xff, 0x7f, 0x3e, 0x7e, 0x7c, 0x00,
    0xc0, 0x3f, 0xff, 0xc0, 0xff, 0x07, 0x7e, 0x00, 0xf8, 0xf3, 0xff, 0x7f, 0x3e, 0xfc, 0xfc, 0xff,
    0xc0, 0x1f, 0x7f, 0xc0, 0xff, 0x07, 0x7e, 0x00, 0xf8, 0xe3, 0x0f, 0xfc, 0x3e, 0xfc, 0xfc, 0xff,
    0x80, 0x1f, 0x7e, 0xc0, 0xff, 0x07, 0x7e, 0x00, 0xf8, 0xe1, 0x07, 0xf8, 0x3e, 0xf8, 0xfd, 0xff,
    0x80, 0x0f, 0x3e, 0xc0, 0xff, 0x07, 0x7e, 0x00, 0xf0, 0xc1, 0x07, 0xf8, 0x3e, 0xf8, 0xfd, 0xff,
    0x00, 0x0f, 0x3e, 0xc0, 0xff, 0x07, 0x3e, 0x00, 0xf0, 0xc1, 0x07, 0xf8, 0x3f, 0xf0, 0xff, 0xff,
};

/* ── Transport-agnostic key send ── */

#ifdef HID_TRANSPORT_BLE
#define SEND_HID(state, k) do { \
    if((state)->use_ble) send_hid_key_ble((state)->ble_profile, (k)); \
    else send_hid_key_usb((k)); \
} while(0)
#define SEND_CONSUMER(state, k) do { \
    if((state)->use_ble) send_consumer_key_ble((state)->ble_profile, (k)); \
    else send_consumer_key_usb((k)); \
} while(0)
#else
#define SEND_HID(state, k) send_hid_key_usb((k))
#define SEND_CONSUMER(state, k) send_consumer_key_usb((k))
#endif

#define DC_TIMEOUT_TICKS 300 /* ~300ms at 1kHz tick */
#define FLASH_DURATION_TICKS 200 /* ~200ms visual feedback */

static void flush_pending_single(ClaudeRemoteState* state) {
    if(!state->dc_pending) return;
    state->dc_pending = false;
    if(!state->hid_connected) return;

    const char* label = NULL;
    switch(state->dc_key) {
    case InputKeyLeft:
        SEND_HID(state, HID_KEYBOARD_1);
        label = "1";
        FURI_LOG_I(TAG, "Sent: 1");
        break;
    case InputKeyUp:
        SEND_HID(state, HID_KEYBOARD_2);
        label = "2";
        FURI_LOG_I(TAG, "Sent: 2");
        break;
    case InputKeyRight:
        SEND_HID(state, HID_KEYBOARD_3);
        label = "3";
        FURI_LOG_I(TAG, "Sent: 3");
        break;
    case InputKeyOk:
        SEND_HID(state, HID_KEYBOARD_RETURN);
        label = "Enter";
        FURI_LOG_I(TAG, "Sent: Enter");
        break;
    case InputKeyDown:
        SEND_CONSUMER(state, HID_CONSUMER_DICTATION);
        label = "Dictate";
        FURI_LOG_I(TAG, "Sent: Dictation (consumer 0x00CF)");
        break;
    default:
        break;
    }
    if(label) {
        state->flash_label = label;
        state->flash_tick = furi_get_tick();
    }
}

static void send_double_action(ClaudeRemoteState* state, InputKey key) {
    if(!state->hid_connected) return;

    const char* label = NULL;
    switch(key) {
    case InputKeyLeft:
        /* Ctrl+A (start of line) then Ctrl+K (kill to end) = clear entire line */
        SEND_HID(state, HID_KEYBOARD_A | KEY_MOD_LEFT_CTRL);
        furi_delay_ms(30);
        SEND_HID(state, HID_KEYBOARD_K | KEY_MOD_LEFT_CTRL);
        label = "Clear";
        FURI_LOG_I(TAG, "Double: Ctrl+A,Ctrl+K (clear entire line)");
        break;
    case InputKeyUp:
        SEND_HID(state, HID_KEYBOARD_PAGE_UP);
        label = "Pg Up";
        FURI_LOG_I(TAG, "Double: Page Up");
        break;
    case InputKeyRight:
        SEND_HID(state, HID_KEYBOARD_UP_ARROW);
        label = "Prev Cmd";
        FURI_LOG_I(TAG, "Double: Up Arrow (prev command)");
        break;
    case InputKeyOk:
        SEND_HID(state, HID_KEYBOARD_GRAVE_ACCENT | KEY_MOD_LEFT_GUI);
        label = "Switch";
        FURI_LOG_I(TAG, "Double: Cmd+` (switch window)");
        break;
    case InputKeyDown:
        SEND_HID(state, HID_KEYBOARD_PAGE_DOWN);
        label = "Pg Down";
        FURI_LOG_I(TAG, "Double: Page Down");
        break;
    default:
        break;
    }
    if(label) {
        state->flash_label = label;
        state->flash_tick = furi_get_tick();
    }
}

/* ══════════════════════════════════════════════
 *  Draw callbacks
 * ══════════════════════════════════════════════ */

static void draw_splash(Canvas* canvas) {
    /* Landscape: 128w x 64h */
    canvas_clear(canvas);

    canvas_set_font(canvas, FontPrimary);
    canvas_draw_str_aligned(canvas, 64, 6, AlignCenter, AlignCenter, "Claupper Remote");

    /* WETWARE logo bitmap — full 128px width */
    canvas_draw_xbm(canvas, 0, 14, WETWARE_LOGO_W, WETWARE_LOGO_H, wetware_logo);

    /* LABS — right-aligned under logo */
    canvas_set_font(canvas, FontSecondary);
    canvas_draw_str_aligned(canvas, 127, 40, AlignRight, AlignCenter, "LABS");

    canvas_draw_line(canvas, 0, 46, 128, 46);

    canvas_set_font(canvas, FontSecondary);
    canvas_draw_str_aligned(canvas, 64, 58, AlignCenter, AlignCenter, "Flipper's claudepanion");
}

static void draw_home(Canvas* canvas) {
    /* Portrait: 64w x 128h */
    canvas_clear(canvas);
    canvas_draw_rframe(canvas, 0, 0, 64, 128, 3);

    canvas_set_font(canvas, FontPrimary);
    canvas_draw_str_aligned(canvas, 32, 12, AlignCenter, AlignCenter, "Claude");
    canvas_draw_str_aligned(canvas, 32, 26, AlignCenter, AlignCenter, "Remote");
    canvas_draw_line(canvas, 10, 34, 54, 34);

    /* Mini D-pad illustration */
    canvas_draw_frame(canvas, 26, 42, 12, 13);
    canvas_draw_frame(canvas, 14, 54, 13, 12);
    canvas_draw_box(canvas, 26, 54, 12, 12);
    canvas_draw_frame(canvas, 37, 54, 13, 12);
    canvas_draw_frame(canvas, 26, 65, 12, 13);

    canvas_draw_line(canvas, 32, 45, 29, 49);
    canvas_draw_line(canvas, 32, 45, 35, 49);
    canvas_draw_line(canvas, 18, 60, 22, 57);
    canvas_draw_line(canvas, 18, 60, 22, 63);
    canvas_draw_line(canvas, 46, 60, 42, 57);
    canvas_draw_line(canvas, 46, 60, 42, 63);
    canvas_draw_line(canvas, 32, 75, 29, 71);
    canvas_draw_line(canvas, 32, 75, 35, 71);

    canvas_set_color(canvas, ColorWhite);
    canvas_draw_disc(canvas, 32, 60, 3);
    canvas_set_color(canvas, ColorBlack);

    canvas_set_font(canvas, FontSecondary);

    /* OK button (filled) → Remote */
    canvas_draw_box(canvas, 6, 86, 14, 10);
    canvas_set_color(canvas, ColorWhite);
    canvas_draw_str_aligned(canvas, 13, 91, AlignCenter, AlignCenter, "OK");
    canvas_set_color(canvas, ColorBlack);
#ifdef HID_TRANSPORT_BLE
    canvas_draw_str(canvas, 24, 94, "USB Remote");
#else
    canvas_draw_str(canvas, 24, 94, "Remote");
#endif

    /* Down arrow → Manual */
    canvas_draw_frame(canvas, 6, 100, 14, 10);
    canvas_draw_line(canvas, 13, 103, 10, 107);
    canvas_draw_line(canvas, 13, 103, 16, 107);
    canvas_draw_str(canvas, 24, 108, "Manual");

    /* Right arrow → BT */
    canvas_draw_frame(canvas, 6, 114, 14, 10);
    canvas_draw_line(canvas, 16, 119, 13, 116);
    canvas_draw_line(canvas, 16, 119, 13, 122);
#ifdef HID_TRANSPORT_BLE
    canvas_draw_str(canvas, 24, 122, "BT Remote");
#else
    canvas_draw_str(canvas, 24, 122, "Go BT");
#endif

}

#ifndef HID_TRANSPORT_BLE
static void draw_ble_promo(Canvas* canvas) {
    /* Landscape: 128w x 64h */
    canvas_clear(canvas);

    canvas_set_font(canvas, FontPrimary);
    canvas_draw_str_aligned(canvas, 64, 8, AlignCenter, AlignCenter, "Go Wireless!");
    canvas_draw_line(canvas, 0, 14, 128, 14);

    canvas_set_font(canvas, FontSecondary);
    canvas_draw_str(canvas, 2, 26, "This USB version works on");
    canvas_draw_str(canvas, 2, 36, "stock firmware. For wireless");
    canvas_draw_str(canvas, 2, 46, "BLE, install Momentum FW:");

    canvas_draw_str(canvas, 2, 60, "momentum-fw.dev/update");
}
#endif

static void draw_remote(Canvas* canvas, ClaudeRemoteState* state) {
    /* Portrait: 64w x 128h */
    canvas_clear(canvas);

    if(!state->hid_connected) {
        canvas_set_font(canvas, FontPrimary);
        canvas_draw_str_aligned(canvas, 32, 30, AlignCenter, AlignCenter, "Not");
        canvas_draw_str_aligned(canvas, 32, 44, AlignCenter, AlignCenter, "Connected");
        canvas_set_font(canvas, FontSecondary);
#ifdef HID_TRANSPORT_BLE
        if(state->use_ble) {
            canvas_draw_str_aligned(canvas, 32, 64, AlignCenter, AlignCenter, "Connect via");
            canvas_draw_str_aligned(canvas, 32, 74, AlignCenter, AlignCenter, "Bluetooth");
        } else {
            canvas_draw_str_aligned(canvas, 32, 64, AlignCenter, AlignCenter, "Connect via");
            canvas_draw_str_aligned(canvas, 32, 74, AlignCenter, AlignCenter, "USB-C cable");
        }
#else
        canvas_draw_str_aligned(canvas, 32, 64, AlignCenter, AlignCenter, "Connect via");
        canvas_draw_str_aligned(canvas, 32, 74, AlignCenter, AlignCenter, "USB-C cable");
#endif
        return;
    }

    canvas_set_font(canvas, FontPrimary);
    canvas_draw_str_aligned(canvas, 32, 10, AlignCenter, AlignCenter, "Claude");
#ifdef HID_TRANSPORT_BLE
    canvas_draw_str_aligned(canvas, 32, 22, AlignCenter, AlignCenter,
        state->use_ble ? "BT Remote" : "USB Remote");
#else
    canvas_draw_str_aligned(canvas, 32, 22, AlignCenter, AlignCenter, "Remote");
#endif
    canvas_draw_line(canvas, 4, 30, 60, 30);

    /* D-pad pixel art — slightly bigger outer buttons */
    canvas_draw_box(canvas, 22, 52, 20, 24);       /* center filled */
    canvas_draw_frame(canvas, 21, 32, 22, 23);     /* up */
    canvas_draw_frame(canvas, 0, 53, 23, 22);      /* left */
    canvas_draw_frame(canvas, 41, 53, 23, 22);     /* right */
    canvas_draw_frame(canvas, 21, 74, 22, 23);     /* down */

    /* Up: "2" + X mark */
    canvas_set_font(canvas, FontPrimary);
    canvas_draw_str_aligned(canvas, 32, 38, AlignCenter, AlignCenter, "2");
    canvas_draw_line(canvas, 28, 44, 36, 51);
    canvas_draw_line(canvas, 36, 44, 28, 51);

    /* Left: "1" + checkmark */
    canvas_draw_str_aligned(canvas, 11, 59, AlignCenter, AlignCenter, "1");
    canvas_draw_line(canvas, 6, 67, 9, 70);
    canvas_draw_line(canvas, 9, 70, 17, 63);

    /* Center: Enter arrow + OK (white on black, shifted up) */
    canvas_set_color(canvas, ColorWhite);
    canvas_draw_line(canvas, 37, 55, 37, 61);
    canvas_draw_line(canvas, 37, 61, 26, 61);
    canvas_draw_line(canvas, 26, 61, 30, 57);
    canvas_draw_line(canvas, 26, 61, 30, 65);
    canvas_set_font(canvas, FontSecondary);
    canvas_draw_str_aligned(canvas, 32, 69, AlignCenter, AlignCenter, "OK");
    canvas_set_color(canvas, ColorBlack);

    /* Right: "3" + "?" */
    canvas_set_font(canvas, FontPrimary);
    canvas_draw_str_aligned(canvas, 52, 59, AlignCenter, AlignCenter, "3");
    canvas_set_font(canvas, FontSecondary);
    canvas_draw_str_aligned(canvas, 52, 70, AlignCenter, AlignCenter, "?");

    /* Down: Mic icon (Dictation) */
    canvas_draw_rframe(canvas, 29, 78, 6, 7, 2);
    canvas_draw_line(canvas, 27, 82, 27, 85);
    canvas_draw_line(canvas, 27, 85, 37, 85);
    canvas_draw_line(canvas, 37, 82, 37, 85);
    canvas_draw_line(canvas, 32, 85, 32, 89);
    canvas_draw_line(canvas, 29, 89, 35, 89);
    canvas_draw_line(canvas, 32, 95, 29, 92);
    canvas_draw_line(canvas, 32, 95, 35, 92);

    /* Flash overlay: inverted bar showing what was sent */
    if(state->flash_label &&
       (furi_get_tick() - state->flash_tick) < FLASH_DURATION_TICKS) {
        canvas_draw_box(canvas, 0, 102, 64, 26);
        canvas_set_color(canvas, ColorWhite);
        canvas_set_font(canvas, FontPrimary);
        canvas_draw_str_aligned(canvas, 32, 115, AlignCenter, AlignCenter, state->flash_label);
        canvas_set_color(canvas, ColorBlack);
    }
}

/* ── Manual: Category list (landscape 128x64) ── */

static void draw_manual_categories(Canvas* canvas, ClaudeRemoteState* state) {
    canvas_clear(canvas);

    canvas_set_font(canvas, FontPrimary);
    canvas_draw_str(canvas, 2, 10, "Claude Manual");
    canvas_draw_line(canvas, 0, 13, 128, 13);

    canvas_set_font(canvas, FontSecondary);

    /* show up to 3 items at a time */
    uint8_t first_visible = 0;
    if(state->cat_index > 2) first_visible = state->cat_index - 2;

    for(int i = 0; i < 3; i++) {
        uint8_t idx = first_visible + i;
        if(idx >= MENU_ITEM_COUNT) break;

        int y = 24 + i * 12;
        bool selected = (idx == state->cat_index);

        if(selected) {
            canvas_draw_box(canvas, 0, y - 9, 128, 12);
            canvas_set_color(canvas, ColorWhite);
        }

        if(idx < CATEGORY_COUNT) {
            /* folder icon */
            canvas_draw_frame(canvas, 4, y - 6, 8, 5);
            canvas_draw_line(canvas, 4, y - 7, 7, y - 7);
            canvas_draw_str(canvas, 16, y, categories[idx].name);
        } else {
            /* quiz entry — star icon */
            canvas_draw_line(canvas, 8, y - 7, 8, y - 2);
            canvas_draw_line(canvas, 5, y - 5, 11, y - 5);
            canvas_draw_str(canvas, 16, y, "Quiz Mode");
        }

        if(selected) {
            canvas_set_color(canvas, ColorBlack);
        }
    }

    /* scroll indicators */
    if(first_visible > 0) {
        canvas_draw_str_aligned(canvas, 124, 15, AlignRight, AlignTop, "^");
    }
    if(first_visible + 3 < MENU_ITEM_COUNT) {
        canvas_draw_str_aligned(canvas, 124, 50, AlignRight, AlignBottom, "v");
    }

    canvas_draw_str_aligned(canvas, 64, 62, AlignCenter, AlignBottom, "OK:open  Bk:home");
}

/* ── Manual: Section list (landscape 128x64) ── */

static void draw_manual_sections(Canvas* canvas, ClaudeRemoteState* state) {
    canvas_clear(canvas);

    const ManualCategory* cat = &categories[state->cat_index];

    canvas_set_font(canvas, FontPrimary);
    char header[32];
    snprintf(header, sizeof(header), "< %s", cat->name);
    canvas_draw_str(canvas, 2, 10, header);
    canvas_draw_line(canvas, 0, 13, 128, 13);

    canvas_set_font(canvas, FontSecondary);

    uint8_t first_visible = 0;
    if(state->section_index > 2) first_visible = state->section_index - 2;

    for(int i = 0; i < 3; i++) {
        uint8_t idx = first_visible + i;
        if(idx >= cat->section_count) break;

        int y = 24 + i * 12;
        bool selected = (idx == state->section_index);

        if(selected) {
            canvas_draw_box(canvas, 0, y - 9, 128, 12);
            canvas_set_color(canvas, ColorWhite);
        }

        canvas_draw_str(canvas, 6, y, cat->sections[idx].title);

        if(selected) {
            canvas_set_color(canvas, ColorBlack);
        }
    }

    if(first_visible > 0) {
        canvas_draw_str_aligned(canvas, 124, 15, AlignRight, AlignTop, "^");
    }
    if(first_visible + 3 < cat->section_count) {
        canvas_draw_str_aligned(canvas, 124, 50, AlignRight, AlignBottom, "v");
    }

    canvas_draw_str_aligned(canvas, 64, 62, AlignCenter, AlignBottom, "OK:read  Bk:back");
}

/* ── Manual: Content reader (landscape 128x64) ── */

static void draw_manual_read(Canvas* canvas, ClaudeRemoteState* state) {
    canvas_clear(canvas);

    const ManualCategory* cat = &categories[state->cat_index];
    const ManualSection* sec = &cat->sections[state->section_index];

    /* title bar */
    canvas_set_font(canvas, FontPrimary);
    char header[48];
    snprintf(
        header, sizeof(header), "%d/%d %s",
        state->section_index + 1, cat->section_count, sec->title);
    canvas_draw_str(canvas, 2, 10, header);
    canvas_draw_line(canvas, 0, 13, 128, 13);

    /* scrollable body */
    canvas_set_font(canvas, FontSecondary);

    const char* p = sec->content;
    int current_line = 0;

    /* skip lines before scroll offset */
    while(*p && current_line < state->scroll_offset) {
        if(*p == '\n') current_line++;
        p++;
    }

    /* render visible lines */
    int y = 24;
    const char* end_check = p;
    while(*p && y < 62) {
        char line_buf[32];
        int i = 0;
        while(*p && *p != '\n' && i < 30) {
            line_buf[i++] = *p++;
        }
        line_buf[i] = '\0';
        if(*p == '\n') p++;

        canvas_draw_str(canvas, 2, y, line_buf);
        y += 10;
    }

    /* scroll indicators */
    if(state->scroll_offset > 0) {
        canvas_draw_str_aligned(canvas, 124, 15, AlignRight, AlignTop, "^");
    }
    if(*p) {
        canvas_draw_str_aligned(canvas, 124, 62, AlignRight, AlignBottom, "v");
    }

    /* nav hint */
    UNUSED(end_check);
    canvas_draw_str_aligned(canvas, 64, 62, AlignCenter, AlignBottom, "<  >");
}

/* ── Manual: Quiz mode (landscape 128x64) ── */

/* ── Quiz: draw helpers ── */

static void draw_quiz_question(Canvas* canvas, const char* desc) {
    canvas_set_font(canvas, FontSecondary);
    const char* p = desc;
    int y = 26;
    while(*p && y < 40) {
        char line_buf[32];
        int i = 0;
        while(*p && *p != '\n' && i < 30) {
            line_buf[i++] = *p++;
        }
        line_buf[i] = '\0';
        if(*p == '\n') p++;
        canvas_draw_str(canvas, 4, y, line_buf);
        y += 10;
    }
}

static void draw_quiz_flashcard(Canvas* canvas, ClaudeRemoteState* state, const QuizCard* card) {
    draw_quiz_question(canvas, card->description);

    if(state->quiz_revealed) {
        int cmd_len = strlen(card->command);
        int box_w = cmd_len * 6 + 8;
        int box_x = 64 - box_w / 2;
        canvas_draw_box(canvas, box_x, 44, box_w, 12);
        canvas_set_color(canvas, ColorWhite);
        canvas_draw_str_aligned(canvas, 64, 50, AlignCenter, AlignCenter, card->command);
        canvas_set_color(canvas, ColorBlack);

        canvas_draw_str_aligned(canvas, 64, 62, AlignCenter, AlignBottom, "<:Knew  ^:Nope  >:skip");
    } else {
        canvas_draw_str_aligned(canvas, 64, 62, AlignCenter, AlignBottom, "OK:reveal  >:skip");
    }
}

static void draw_quiz_multichoice(Canvas* canvas, ClaudeRemoteState* state, const QuizCard* card) {
    draw_quiz_question(canvas, card->description);

    if(state->quiz_answered) {
        bool was_correct = (state->quiz_selected == card->correct_option);
        canvas_draw_box(canvas, 0, 38, 128, 14);
        canvas_set_color(canvas, ColorWhite);
        canvas_set_font(canvas, FontPrimary);
        canvas_draw_str_aligned(canvas, 64, 45, AlignCenter, AlignCenter,
            was_correct ? "CORRECT!" : "WRONG!");
        canvas_set_color(canvas, ColorBlack);

        if(!was_correct) {
            canvas_set_font(canvas, FontSecondary);
            canvas_draw_str_aligned(canvas, 64, 56, AlignCenter, AlignCenter, card->command);
        }

        canvas_set_font(canvas, FontSecondary);
        canvas_draw_str_aligned(canvas, 64, 62, AlignCenter, AlignBottom, "OK:next");
    } else {
        const char* opts[3] = {card->option_a, card->option_b, card->option_c};
        const char* labels[3] = {"<", "^", ">"};

        canvas_set_font(canvas, FontSecondary);
        for(int i = 0; i < 3; i++) {
            int oy = 44 + i * 8;
            bool selected = (state->quiz_selected == i);

            if(selected) {
                canvas_draw_box(canvas, 0, oy - 7, 128, 9);
                canvas_set_color(canvas, ColorWhite);
            }

            char opt_buf[32];
            snprintf(opt_buf, sizeof(opt_buf), "%s %s", labels[i], opts[i]);
            canvas_draw_str(canvas, 2, oy, opt_buf);

            if(selected) {
                canvas_set_color(canvas, ColorBlack);
            }
        }
    }
}

static void draw_manual_quiz(Canvas* canvas, ClaudeRemoteState* state) {
    canvas_clear(canvas);

    /* completion screen */
    if(state->quiz_index >= QUIZ_CARD_COUNT) {
        canvas_set_font(canvas, FontPrimary);
        canvas_draw_str_aligned(canvas, 64, 10, AlignCenter, AlignCenter, "Quiz Complete!");
        canvas_draw_line(canvas, 0, 18, 128, 18);

        canvas_set_font(canvas, FontSecondary);
        char score_buf[32];
        snprintf(score_buf, sizeof(score_buf), "Score: %d / %d",
                 state->quiz_correct, state->quiz_total);
        canvas_draw_str_aligned(canvas, 64, 30, AlignCenter, AlignCenter, score_buf);

        if(state->quiz_total > 0) {
            int pct = (state->quiz_correct * 100) / state->quiz_total;
            char pct_buf[16];
            snprintf(pct_buf, sizeof(pct_buf), "%d%% correct", pct);
            canvas_draw_str_aligned(canvas, 64, 40, AlignCenter, AlignCenter, pct_buf);
        }

        char streak_buf[24];
        snprintf(streak_buf, sizeof(streak_buf), "Best streak: %d",
                 state->quiz_best_streak);
        canvas_draw_str_aligned(canvas, 64, 50, AlignCenter, AlignCenter, streak_buf);

        canvas_draw_str_aligned(canvas, 64, 62, AlignCenter, AlignBottom, "OK:retry  Bk:menu");
        return;
    }

    uint8_t real_index = state->quiz_order[state->quiz_index];
    const QuizCard* card = &quiz_cards[real_index];

    /* header */
    canvas_set_font(canvas, FontPrimary);
    char header[32];
    snprintf(header, sizeof(header), "Quiz %d/%d",
             state->quiz_index + 1, QUIZ_CARD_COUNT);
    canvas_draw_str(canvas, 2, 10, header);

    /* score + streak */
    canvas_set_font(canvas, FontSecondary);
    if(state->quiz_total > 0 || state->quiz_streak > 0) {
        char score[20];
        if(state->quiz_streak >= 2) {
            snprintf(score, sizeof(score), "%d/%d %dx",
                     state->quiz_correct, state->quiz_total, state->quiz_streak);
        } else {
            snprintf(score, sizeof(score), "%d/%d",
                     state->quiz_correct, state->quiz_total);
        }
        canvas_draw_str_aligned(canvas, 124, 10, AlignRight, AlignCenter, score);
    }

    canvas_draw_line(canvas, 0, 13, 128, 13);

    if(card->type == QuizTypeFlashcard) {
        draw_quiz_flashcard(canvas, state, card);
    } else {
        draw_quiz_multichoice(canvas, state, card);
    }
}

/* ── Draw dispatcher ── */

static void draw_callback(Canvas* canvas, void* ctx) {
    ClaudeRemoteState* state = (ClaudeRemoteState*)ctx;
    furi_mutex_acquire(state->mutex, FuriWaitForever);

    switch(state->mode) {
    case ModeSplash:
        draw_splash(canvas);
        break;
    case ModeHome:
        draw_home(canvas);
        break;
    case ModeRemote:
        draw_remote(canvas, state);
        break;
    case ModeManual:
        switch(state->manual_view) {
        case ManualViewCategories:
            draw_manual_categories(canvas, state);
            break;
        case ManualViewSections:
            draw_manual_sections(canvas, state);
            break;
        case ManualViewRead:
            draw_manual_read(canvas, state);
            break;
        case ManualViewQuiz:
            draw_manual_quiz(canvas, state);
            break;
        }
        break;
#ifndef HID_TRANSPORT_BLE
    case ModeBlePromo:
        draw_ble_promo(canvas);
        break;
#endif
    }

    furi_mutex_release(state->mutex);
}

/* ── Input callback ── */

static void input_callback(InputEvent* input_event, void* ctx) {
    FuriMessageQueue* queue = (FuriMessageQueue*)ctx;
    furi_message_queue_put(queue, input_event, FuriWaitForever);
}

/* ══════════════════════════════════════════════
 *  Input handling
 * ══════════════════════════════════════════════ */

static bool handle_home_input(ClaudeRemoteState* state, InputEvent* event, ViewPort* vp) {
    if(event->type != InputTypeShort) return true;

    switch(event->key) {
    case InputKeyOk:
#ifdef HID_TRANSPORT_BLE
        state->use_ble = false;
        state->hid_connected = furi_hal_hid_is_connected();
#endif
        state->mode = ModeRemote;
        break;
    case InputKeyRight:
#ifdef HID_TRANSPORT_BLE
        state->use_ble = true;
        state->hid_connected = state->ble_connected;
        state->mode = ModeRemote;
#else
        state->mode = ModeBlePromo;
        view_port_set_orientation(vp, ViewPortOrientationHorizontal);
#endif
        break;
    case InputKeyDown:
    case InputKeyLeft:
        state->cat_index = 0;
        state->section_index = 0;
        state->scroll_offset = 0;
        state->manual_view = ManualViewCategories;
        state->mode = ModeManual;
        view_port_set_orientation(vp, ViewPortOrientationHorizontal);
        break;
    case InputKeyBack:
        return false;
    default:
        break;
    }
    return true;
}

static bool handle_remote_input(
    ClaudeRemoteState* state,
    InputEvent* event,
    ViewPort* view_port) {
    if(event->type != InputTypeShort) return true;

    if(event->key == InputKeyBack) {
        flush_pending_single(state);
        state->mode = ModeHome;
        state->is_flipped = false;
        view_port_set_orientation(view_port, ViewPortOrientationVertical);
        return true;
    }

#ifdef HID_TRANSPORT_BLE
    if(!state->use_ble) {
        state->hid_connected = furi_hal_hid_is_connected();
    }
#else
    state->hid_connected = furi_hal_hid_is_connected();
#endif
    if(!state->hid_connected) return true;

    /* All keys go through pending/deferred send for BLE reliability */
    uint32_t now = furi_get_tick();
    if(state->dc_pending && event->key == state->dc_key &&
       (now - state->dc_tick) < DC_TIMEOUT_TICKS) {
        /* double-click detected */
        state->dc_pending = false;
        send_double_action(state, event->key);
    } else {
        /* flush any different pending key first */
        flush_pending_single(state);
        /* start new pending */
        state->dc_key = event->key;
        state->dc_tick = now;
        state->dc_pending = true;
    }

    return true;
}

/* ── Manual sub-view input handlers ── */

static void handle_manual_categories(ClaudeRemoteState* state, InputEvent* event, ViewPort* vp) {
    switch(event->key) {
    case InputKeyUp:
        if(state->cat_index > 0) state->cat_index--;
        break;
    case InputKeyDown:
        if(state->cat_index < MENU_ITEM_COUNT - 1) state->cat_index++;
        break;
    case InputKeyOk:
    case InputKeyRight:
        if(state->cat_index < CATEGORY_COUNT) {
            state->section_index = 0;
            state->manual_view = ManualViewSections;
        } else {
            /* quiz mode */
            quiz_shuffle(state);
            state->quiz_index = 0;
            state->quiz_revealed = false;
            state->quiz_correct = 0;
            state->quiz_total = 0;
            state->quiz_streak = 0;
            state->quiz_best_streak = 0;
            state->quiz_selected = -1;
            state->quiz_answered = false;
            state->manual_view = ManualViewQuiz;
        }
        break;
    case InputKeyBack:
        state->mode = ModeHome;
        view_port_set_orientation(
            vp,
            state->is_flipped ? ViewPortOrientationVerticalFlip :
                                ViewPortOrientationVertical);
        break;
    default:
        break;
    }
}

static void handle_manual_sections(ClaudeRemoteState* state, InputEvent* event) {
    const ManualCategory* cat = &categories[state->cat_index];

    switch(event->key) {
    case InputKeyUp:
        if(state->section_index > 0) state->section_index--;
        break;
    case InputKeyDown:
        if(state->section_index < cat->section_count - 1) state->section_index++;
        break;
    case InputKeyOk:
    case InputKeyRight:
        state->scroll_offset = 0;
        state->manual_view = ManualViewRead;
        break;
    case InputKeyBack:
    case InputKeyLeft:
        state->manual_view = ManualViewCategories;
        break;
    default:
        break;
    }
}

static void handle_manual_read(ClaudeRemoteState* state, InputEvent* event) {
    const ManualCategory* cat = &categories[state->cat_index];
    const ManualSection* sec = &cat->sections[state->section_index];
    uint16_t lines = count_lines(sec->content);
    int max_scroll = lines > 4 ? lines - 4 : 0;

    switch(event->key) {
    case InputKeyUp:
        if(state->scroll_offset > 0) state->scroll_offset--;
        break;
    case InputKeyDown:
        if(state->scroll_offset < max_scroll) state->scroll_offset++;
        break;
    case InputKeyRight:
        if(state->section_index < cat->section_count - 1) {
            state->section_index++;
            state->scroll_offset = 0;
        }
        break;
    case InputKeyLeft:
        if(state->section_index > 0) {
            state->section_index--;
            state->scroll_offset = 0;
        }
        break;
    case InputKeyBack:
        state->manual_view = ManualViewSections;
        break;
    default:
        break;
    }
}

static void handle_manual_quiz(ClaudeRemoteState* state, InputEvent* event) {
    /* completion screen */
    if(state->quiz_index >= QUIZ_CARD_COUNT) {
        if(event->key == InputKeyOk) {
            quiz_shuffle(state);
            state->quiz_index = 0;
            state->quiz_revealed = false;
            state->quiz_correct = 0;
            state->quiz_total = 0;
            state->quiz_streak = 0;
            state->quiz_best_streak = 0;
            state->quiz_selected = -1;
            state->quiz_answered = false;
        } else if(event->key == InputKeyBack) {
            state->manual_view = ManualViewCategories;
        }
        return;
    }

    uint8_t real_index = state->quiz_order[state->quiz_index];
    const QuizCard* card = &quiz_cards[real_index];

    if(card->type == QuizTypeFlashcard) {
        /* flashcard input */
        switch(event->key) {
        case InputKeyOk:
            if(!state->quiz_revealed) {
                state->quiz_revealed = true;
            }
            break;
        case InputKeyLeft:
            if(state->quiz_revealed) {
                state->quiz_correct++;
                state->quiz_total++;
                state->quiz_streak++;
                if(state->quiz_streak > state->quiz_best_streak)
                    state->quiz_best_streak = state->quiz_streak;
                state->quiz_index++;
                state->quiz_revealed = false;
            }
            break;
        case InputKeyUp:
            if(state->quiz_revealed) {
                state->quiz_total++;
                state->quiz_streak = 0;
                state->quiz_index++;
                state->quiz_revealed = false;
            }
            break;
        case InputKeyRight:
            if(state->quiz_index < QUIZ_CARD_COUNT - 1) {
                state->quiz_index++;
                state->quiz_revealed = false;
            }
            break;
        case InputKeyBack:
            state->manual_view = ManualViewCategories;
            break;
        default:
            break;
        }
    } else {
        /* multi-choice input */
        if(state->quiz_answered) {
            if(event->key == InputKeyOk) {
                state->quiz_index++;
                state->quiz_selected = -1;
                state->quiz_answered = false;
            } else if(event->key == InputKeyBack) {
                state->manual_view = ManualViewCategories;
            }
        } else {
            int8_t picked = -1;
            switch(event->key) {
            case InputKeyLeft:  picked = 0; break;
            case InputKeyUp:    picked = 1; break;
            case InputKeyRight: picked = 2; break;
            case InputKeyBack:
                state->manual_view = ManualViewCategories;
                return;
            default: break;
            }
            if(picked >= 0) {
                state->quiz_selected = picked;
                state->quiz_answered = true;
                state->quiz_total++;
                if(picked == card->correct_option) {
                    state->quiz_correct++;
                    state->quiz_streak++;
                    if(state->quiz_streak > state->quiz_best_streak)
                        state->quiz_best_streak = state->quiz_streak;
                } else {
                    state->quiz_streak = 0;
                }
            }
        }
    }
}

static bool handle_manual_input(ClaudeRemoteState* state, InputEvent* event, ViewPort* vp) {
    if(event->type != InputTypeShort && event->type != InputTypeRepeat) return true;

    switch(state->manual_view) {
    case ManualViewCategories:
        handle_manual_categories(state, event, vp);
        break;
    case ManualViewSections:
        handle_manual_sections(state, event);
        break;
    case ManualViewRead:
        handle_manual_read(state, event);
        break;
    case ManualViewQuiz:
        handle_manual_quiz(state, event);
        break;
    }
    return true;
}

/* ══════════════════════════════════════════════
 *  Main app logic
 * ══════════════════════════════════════════════ */

static int32_t claude_remote_main(void* p) {
    UNUSED(p);
    FURI_LOG_I(TAG, "Starting Claude Remote");

    ClaudeRemoteState* state = malloc(sizeof(ClaudeRemoteState));
    memset(state, 0, sizeof(ClaudeRemoteState));
    state->mode = ModeSplash;
    state->splash_start = furi_get_tick();
    state->mutex = furi_mutex_alloc(FuriMutexTypeNormal);

    NotificationApp* notifications = furi_record_open(RECORD_NOTIFICATION);
    notification_message(notifications, &sequence_solid_orange);

    FuriMessageQueue* queue = furi_message_queue_alloc(8, sizeof(InputEvent));

    ViewPort* view_port = view_port_alloc();
    view_port_draw_callback_set(view_port, draw_callback, state);
    view_port_input_callback_set(view_port, input_callback, queue);

    /* landscape for splash screen */
    view_port_set_orientation(view_port, ViewPortOrientationHorizontal);

    Gui* gui = furi_record_open(RECORD_GUI);
    gui_add_view_port(gui, view_port, GuiLayerFullscreen);

    /* Init USB HID (both builds) */
    state->usb_prev = furi_hal_usb_get_config();
    furi_hal_usb_unlock();
    furi_hal_usb_set_config(&usb_hid, NULL);

#ifdef HID_TRANSPORT_BLE
    /* Also init BLE HID */
    state->bt = furi_record_open(RECORD_BT);
    bt_disconnect(state->bt);
    furi_delay_ms(200);
    state->ble_profile = bt_profile_start(state->bt, ble_profile_hid, NULL);
    bt_set_status_changed_callback(state->bt, bt_status_callback, state);
    FURI_LOG_I(TAG, "BLE + USB HID profiles started");
#endif

    InputEvent event;
    bool running = true;

    while(running) {
        FuriStatus status = furi_message_queue_get(queue, &event, 100);

        furi_mutex_acquire(state->mutex, FuriWaitForever);

        /* auto-advance splash after 3 seconds, or skip on any press */
        if(state->mode == ModeSplash) {
            if(status == FuriStatusOk || (furi_get_tick() - state->splash_start) >= 3000) {
                state->mode = ModeHome;
                view_port_set_orientation(view_port, ViewPortOrientationVertical);
            }
            furi_mutex_release(state->mutex);
            view_port_update(view_port);
            continue;
        }

        if(status == FuriStatusOk) {
            switch(state->mode) {
            case ModeSplash:
                break; /* handled above */
            case ModeHome:
                running = handle_home_input(state, &event, view_port);
                break;
            case ModeRemote:
                running = handle_remote_input(state, &event, view_port);
                break;
            case ModeManual:
                running = handle_manual_input(state, &event, view_port);
                break;
#ifndef HID_TRANSPORT_BLE
            case ModeBlePromo:
                if(event.type == InputTypeShort && event.key == InputKeyBack) {
                    state->mode = ModeHome;
                    view_port_set_orientation(view_port, ViewPortOrientationVertical);
                }
                break;
#endif
            }
        }

        if(state->mode == ModeRemote) {
#ifdef HID_TRANSPORT_BLE
            if(state->use_ble) {
                state->hid_connected = state->ble_connected;
            } else {
                state->hid_connected = furi_hal_hid_is_connected();
            }
#else
            state->hid_connected = furi_hal_hid_is_connected();
#endif
            /* flush pending single-press after double-click timeout */
            if(state->dc_pending &&
               (furi_get_tick() - state->dc_tick) >= DC_TIMEOUT_TICKS) {
                flush_pending_single(state);
            }
        }

        furi_mutex_release(state->mutex);
        view_port_update(view_port);
    }

    FURI_LOG_I(TAG, "Exiting Claude Remote");

    notification_message(notifications, &sequence_reset_rgb);
    furi_record_close(RECORD_NOTIFICATION);

    /* Cleanup USB HID (both builds) */
    furi_hal_hid_kb_release_all();
    furi_hal_usb_set_config(state->usb_prev, NULL);

#ifdef HID_TRANSPORT_BLE
    /* Also cleanup BLE HID */
    bt_set_status_changed_callback(state->bt, NULL, NULL);
    ble_profile_hid_kb_release_all(state->ble_profile);
    bt_profile_restore_default(state->bt);
    furi_record_close(RECORD_BT);
#endif

    gui_remove_view_port(gui, view_port);
    furi_record_close(RECORD_GUI);
    view_port_free(view_port);
    furi_message_queue_free(queue);
    furi_mutex_free(state->mutex);
    free(state);

    return 0;
}

/* ── Entry points ── */

int32_t claude_remote_usb_app(void* p) {
    return claude_remote_main(p);
}

int32_t claude_remote_ble_app(void* p) {
    return claude_remote_main(p);
}
