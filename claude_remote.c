#include <furi.h>
#include <furi_hal.h>
#include <furi_hal_random.h>
#include <furi_hal_usb_hid.h>
#include <gui/gui.h>
#include <gui/view_port.h>
#include <input/input.h>
#include <notification/notification.h>
#include <notification/notification_messages.h>
#include <storage/storage.h>

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

/* ── LED color sequences ── */

static const NotificationSequence sequence_solid_blue = {
    &message_red_0,
    &message_green_0,
    &message_blue_255,
    &message_do_not_reset,
    NULL,
};

static const NotificationSequence sequence_solid_green = {
    &message_red_0,
    &message_green_255,
    &message_blue_0,
    &message_do_not_reset,
    NULL,
};

/* ── Modes ── */

typedef enum {
    ModeSplash,
    ModeTour,
    ModeHome,
    ModeRemote,
    ModeManual,
    ModeSettings,
    ModeMacros,
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
     " claude --resume\n"
     " or /resume in chat\n\n"
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
     " /resume in chat, or\n"
     " claude --resume from CLI\n"},

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
     " ~/.claude/settings.json\n"},

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

/* ── Settings & Macros constants ── */

#define APP_DATA_DIR APP_DATA_PATH("")
#define SETTINGS_PATH APP_DATA_PATH("settings.cfg")
#ifdef HID_TRANSPORT_BLE
#define SETTINGS_COUNT 6
#else
#define SETTINGS_COUNT 5
#endif

#define MACRO_MAX_COUNT 10
#define MACRO_MAX_LEN 64
#define MACROS_PATH APP_DATA_PATH("macros.txt")

/* ── Quiz cards ── */

static const QuizCard quiz_cards[] = {
    /* slash-command multi-choice */
    {QuizTypeMultiChoice, "Show help and\navailable commands",
     "/help", "/help", "/info", "/commands", 0},
    {QuizTypeMultiChoice, "Clear conversation\nhistory",
     "/clear", "/reset", "/clear", "/clean", 1},
    {QuizTypeMultiChoice, "Summarize context to\nreduce token usage",
     "/compact", "/shrink", "/summarize", "/compact", 2},
    {QuizTypeMultiChoice, "Open configuration\nsettings editor",
     "/config", "/config", "/setup", "/preferences", 0},
    {QuizTypeMultiChoice, "Show token usage\nand session cost",
     "/cost", "/usage", "/cost", "/tokens", 1},
    {QuizTypeMultiChoice, "Diagnose setup issues\nand check API",
     "/doctor", "/debug", "/check", "/doctor", 2},
    {QuizTypeMultiChoice, "Create CLAUDE.md\nfor your project",
     "/init", "/init", "/create", "/new", 0},
    {QuizTypeMultiChoice, "Review PR or\ncode changes",
     "/review", "/diff", "/review", "/inspect", 1},
    {QuizTypeMultiChoice, "Fix terminal display\nand rendering",
     "/terminal-setup", "/fix-term", "/display", "/terminal-setup", 2},
    {QuizTypeMultiChoice, "Start a brand new\nsession from scratch",
     "claude --new", "--new", "--fresh", "--reset", 0},
    {QuizTypeMultiChoice, "Resume your previous\nconversation",
     "claude --resume", "--resume", "--continue", "--last", 0},
    {QuizTypeMultiChoice, "Run a one-shot query\nwithout chat mode",
     "claude -p \"query\"", "-q", "-e", "-p", 2},
    {QuizTypeMultiChoice, "Switch AI model\nduring a chat session",
     "/model", "/model", "/switch", "/engine", 0},
    {QuizTypeMultiChoice, "Edit your project's\nCLAUDE.md memory file",
     "/memory", "/memo", "/memory", "/notes", 1},
    {QuizTypeMultiChoice, "Undo to a previous\npoint in conversation",
     "/rewind", "/back", "/undo", "/rewind", 2},
    {QuizTypeMultiChoice, "Generate commit msg\nand commit changes",
     "/commit", "/commit", "/save", "/push", 0},

    /* concept multi-choice */
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
    bool hid_connected;
    FuriMutex* mutex;
    NotificationApp* notifications;

#ifdef HID_TRANSPORT_BLE
    bool use_ble;
    bool ble_connected;
    BtStatus bt_status;
    bool bt_pair_screen;  /* showing BT pairing sub-screen in settings */
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
    uint8_t quiz_correct;
    uint8_t quiz_total;
    uint8_t quiz_streak;
    uint8_t quiz_best_streak;
    uint8_t quiz_order[24]; /* QUIZ_CARD_COUNT — shuffled indices */
    int8_t  quiz_selected;  /* multi-choice: -1=none, 0/1/2 */
    bool    quiz_answered;  /* multi-choice: showing feedback */
    bool    quiz_selecting; /* showing difficulty picker */
    uint8_t quiz_count;     /* questions this round (8/16/24) */

    /* double-click detection (remote mode) */
    InputKey dc_key;
    uint32_t dc_tick;
    bool dc_pending;

    /* visual feedback flash */
    uint32_t flash_tick;
    const char* flash_label;

    /* splash screen */
    uint32_t splash_start;

    /* tour */
    uint8_t tour_page;      /* 0=ask, 1-4=content */
    bool tour_skip;         /* "don't ask again" checkbox */
    bool show_tour;         /* loaded from settings */

    /* settings */
    bool haptics_enabled;
    bool led_enabled;
    uint8_t os_mode;    /* 0=Mac, 1=Windows, 2=Linux */
    uint8_t dc_speed;   /* 0=Normal(300ms), 1=Slow(500ms), 2=Fast(200ms) */
    uint8_t settings_index;

    /* macros */
    char macros[MACRO_MAX_COUNT][MACRO_MAX_LEN + 1];
    uint8_t macro_count;
    uint8_t macro_index;
    bool macros_loaded;
    uint32_t macro_scroll_tick; /* for marquee scroll on long macros */

    /* hotkey overlay (remote mode) */
    bool show_hotkeys;
    bool right_held;
    bool down_held;
    uint32_t hotkeys_tick;

    /* hold-left backspace repeat */
    bool left_holding;
    uint32_t left_hold_start;
    uint32_t left_repeat_tick;
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

/* ── Settings persistence ── */

static void load_settings(ClaudeRemoteState* state) {
    state->haptics_enabled = true;
    state->led_enabled = true;
    state->os_mode = 0;
    state->dc_speed = 0;
    state->show_tour = true;

    Storage* storage = furi_record_open(RECORD_STORAGE);
    File* file = storage_file_alloc(storage);

    if(storage_file_open(file, SETTINGS_PATH, FSAM_READ, FSOM_OPEN_EXISTING)) {
        char buf[80];
        uint16_t bytes_read = storage_file_read(file, buf, sizeof(buf) - 1);
        buf[bytes_read] = '\0';
        storage_file_close(file);

        char* p = buf;
        while(*p) {
            if(strncmp(p, "haptics=", 8) == 0) {
                state->haptics_enabled = (p[8] == '1');
            } else if(strncmp(p, "led=", 4) == 0) {
                state->led_enabled = (p[4] == '1');
            } else if(strncmp(p, "os=", 3) == 0) {
                if(p[3] == 'w') state->os_mode = 1;
                else if(p[3] == 'l') state->os_mode = 2;
                else state->os_mode = 0;
            } else if(strncmp(p, "tour=", 5) == 0) {
                state->show_tour = (p[5] == '1');
            } else if(strncmp(p, "dc_speed=", 9) == 0) {
                state->dc_speed = (uint8_t)(p[9] - '0');
                if(state->dc_speed > 2) state->dc_speed = 0;
            }
            while(*p && *p != '\n') p++;
            if(*p == '\n') p++;
        }
    } else {
        storage_file_close(file);
    }

    storage_file_free(file);
    furi_record_close(RECORD_STORAGE);
}

static void save_settings(ClaudeRemoteState* state) {
    Storage* storage = furi_record_open(RECORD_STORAGE);
    storage_simply_mkdir(storage, APP_DATA_DIR);
    File* file = storage_file_alloc(storage);

    if(storage_file_open(file, SETTINGS_PATH, FSAM_WRITE, FSOM_CREATE_ALWAYS)) {
        char buf[80];
        int len = snprintf(buf, sizeof(buf),
                           "haptics=%d\nled=%d\nos=%s\ntour=%d\ndc_speed=%d\n",
                           state->haptics_enabled ? 1 : 0,
                           state->led_enabled ? 1 : 0,
                           state->os_mode == 1 ? "win" : state->os_mode == 2 ? "linux" : "mac",
                           state->show_tour ? 1 : 0,
                           state->dc_speed);
        if(len > 0) storage_file_write(file, buf, len);
        storage_file_close(file);
    } else {
        storage_file_close(file);
    }

    storage_file_free(file);
    furi_record_close(RECORD_STORAGE);
}

/* ── Macro helpers ── */

static uint16_t char_to_hid(char c) {
    if(c >= 'a' && c <= 'z') return HID_KEYBOARD_A + (c - 'a');
    if(c >= 'A' && c <= 'Z') return (HID_KEYBOARD_A + (c - 'A')) | KEY_MOD_LEFT_SHIFT;
    if(c >= '1' && c <= '9') return HID_KEYBOARD_1 + (c - '1');
    if(c == '0') return HID_KEYBOARD_0;
    switch(c) {
    case ' ':  return HID_KEYBOARD_SPACEBAR;
    case '\t': return HID_KEYBOARD_TAB;
    case '-':  return HID_KEYBOARD_MINUS;
    case '=':  return HID_KEYBOARD_EQUAL_SIGN;
    case '[':  return HID_KEYBOARD_OPEN_BRACKET;
    case ']':  return HID_KEYBOARD_CLOSE_BRACKET;
    case '\\': return HID_KEYBOARD_BACKSLASH;
    case ';':  return HID_KEYBOARD_SEMICOLON;
    case '\'': return HID_KEYBOARD_APOSTROPHE;
    case '`':  return HID_KEYBOARD_GRAVE_ACCENT;
    case ',':  return HID_KEYBOARD_COMMA;
    case '.':  return HID_KEYBOARD_DOT;
    case '/':  return HID_KEYBOARD_SLASH;
    /* shifted symbols */
    case '!':  return HID_KEYBOARD_1 | KEY_MOD_LEFT_SHIFT;
    case '@':  return HID_KEYBOARD_2 | KEY_MOD_LEFT_SHIFT;
    case '#':  return HID_KEYBOARD_3 | KEY_MOD_LEFT_SHIFT;
    case '$':  return HID_KEYBOARD_4 | KEY_MOD_LEFT_SHIFT;
    case '%':  return HID_KEYBOARD_5 | KEY_MOD_LEFT_SHIFT;
    case '^':  return HID_KEYBOARD_6 | KEY_MOD_LEFT_SHIFT;
    case '&':  return HID_KEYBOARD_7 | KEY_MOD_LEFT_SHIFT;
    case '*':  return HID_KEYBOARD_8 | KEY_MOD_LEFT_SHIFT;
    case '(':  return HID_KEYBOARD_9 | KEY_MOD_LEFT_SHIFT;
    case ')':  return HID_KEYBOARD_0 | KEY_MOD_LEFT_SHIFT;
    case '_':  return HID_KEYBOARD_MINUS | KEY_MOD_LEFT_SHIFT;
    case '+':  return HID_KEYBOARD_EQUAL_SIGN | KEY_MOD_LEFT_SHIFT;
    case '{':  return HID_KEYBOARD_OPEN_BRACKET | KEY_MOD_LEFT_SHIFT;
    case '}':  return HID_KEYBOARD_CLOSE_BRACKET | KEY_MOD_LEFT_SHIFT;
    case '|':  return HID_KEYBOARD_BACKSLASH | KEY_MOD_LEFT_SHIFT;
    case ':':  return HID_KEYBOARD_SEMICOLON | KEY_MOD_LEFT_SHIFT;
    case '"':  return HID_KEYBOARD_APOSTROPHE | KEY_MOD_LEFT_SHIFT;
    case '~':  return HID_KEYBOARD_GRAVE_ACCENT | KEY_MOD_LEFT_SHIFT;
    case '<':  return HID_KEYBOARD_COMMA | KEY_MOD_LEFT_SHIFT;
    case '>':  return HID_KEYBOARD_DOT | KEY_MOD_LEFT_SHIFT;
    case '?':  return HID_KEYBOARD_SLASH | KEY_MOD_LEFT_SHIFT;
    default:   return 0;
    }
}

/* ── BLE status callback ── */

#ifdef HID_TRANSPORT_BLE
static void bt_status_callback(BtStatus status, void* context) {
    ClaudeRemoteState* state = (ClaudeRemoteState*)context;
    state->bt_status = status;
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
#define SEND_HID(state, k) do { (void)(state); send_hid_key_usb((k)); } while(0)
#define SEND_CONSUMER(state, k) do { (void)(state); send_consumer_key_usb((k)); } while(0)
#endif

/* dc_speed: 0=Normal(300ms), 1=Slow(500ms), 2=Fast(200ms) */
static inline uint32_t dc_timeout(const ClaudeRemoteState* state) {
    if(state->dc_speed == 1) return 500;
    if(state->dc_speed == 2) return 200;
    return 300;
}
#define FLASH_DURATION_TICKS 200 /* ~200ms visual feedback */

/* ── Macro string sender (needs SEND_HID macro) ── */

static void send_macro_string(ClaudeRemoteState* state, const char* str) {
    for(const char* p = str; *p; p++) {
        uint16_t key = char_to_hid(*p);
        if(key != 0) {
            SEND_HID(state, key);
            furi_delay_ms(20);
        }
    }
    SEND_HID(state, HID_KEYBOARD_RETURN);
}

/* ── Macro loader from SD ── */

static const char* const default_macros_text =
    "/compact\n"
    "/rename\n"
    "/resume\n"
    "/review\n"
    "/remote-control\n"
    "claude --dangerously-skip-permissions --continue\n"
    "fix this\n"
    "explain this error\n"
    "run the tests\n"
    "/commit\n";

static void write_default_macros(Storage* storage) {
    storage_simply_mkdir(storage, APP_DATA_DIR);
    File* file = storage_file_alloc(storage);
    if(storage_file_open(file, MACROS_PATH, FSAM_WRITE, FSOM_CREATE_ALWAYS)) {
        storage_file_write(file, default_macros_text, strlen(default_macros_text));
        storage_file_close(file);
    } else {
        storage_file_close(file);
    }
    storage_file_free(file);
}

static void parse_macros_buf(ClaudeRemoteState* state, char* buf) {
    char* p = buf;
    while(*p && state->macro_count < MACRO_MAX_COUNT) {
        char* line_start = p;
        while(*p && *p != '\n' && *p != '\r') p++;

        int len = p - line_start;
        if(len > MACRO_MAX_LEN) len = MACRO_MAX_LEN;

        if(len > 0) {
            memcpy(state->macros[state->macro_count], line_start, len);
            state->macros[state->macro_count][len] = '\0';
            state->macro_count++;
        }

        while(*p == '\n' || *p == '\r') p++;
    }
}

static void load_macros_from_sd(ClaudeRemoteState* state) {
    state->macro_count = 0;
    state->macros_loaded = true;

    Storage* storage = furi_record_open(RECORD_STORAGE);
    File* file = storage_file_alloc(storage);

    bool loaded = false;
    if(storage_file_open(file, MACROS_PATH, FSAM_READ, FSOM_OPEN_EXISTING)) {
        char buf[512];
        uint16_t bytes_read = storage_file_read(file, buf, sizeof(buf) - 1);
        buf[bytes_read] = '\0';
        storage_file_close(file);

        if(bytes_read > 0) {
            parse_macros_buf(state, buf);
            loaded = true;
        }
    } else {
        storage_file_close(file);
    }
    storage_file_free(file);

    /* Write defaults if file was missing or empty */
    if(!loaded) {
        write_default_macros(storage);
        char buf[512];
        strncpy(buf, default_macros_text, sizeof(buf) - 1);
        buf[sizeof(buf) - 1] = '\0';
        parse_macros_buf(state, buf);
    }

    furi_record_close(RECORD_STORAGE);
}

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
        if(state->os_mode == 1) {
            SEND_HID(state, HID_KEYBOARD_H | KEY_MOD_LEFT_GUI);
            label = "Voice";
            FURI_LOG_I(TAG, "Sent: Win+H (Windows Voice Typing)");
        } else {
            SEND_CONSUMER(state, HID_CONSUMER_DICTATION);
            label = "Dictate";
            FURI_LOG_I(TAG, "Sent: Dictation (consumer 0x00CF)");
        }
        break;
    default:
        break;
    }
    if(label) {
        state->flash_label = label;
        state->flash_tick = furi_get_tick();
        if(state->haptics_enabled) {
            notification_message(state->notifications, &sequence_single_vibro);
        }
    }
}

static void send_double_action(ClaudeRemoteState* state, InputKey key) {
    if(!state->hid_connected) return;

    const char* label = NULL;
    switch(key) {
    case InputKeyLeft:
        /* Ctrl+U = kill line (standard terminal clear) */
        SEND_HID(state, HID_KEYBOARD_U | KEY_MOD_LEFT_CTRL);
        label = "Clear";
        FURI_LOG_I(TAG, "Double: Ctrl+U (clear line)");
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
        if(state->os_mode == 0) {
            SEND_HID(state, HID_KEYBOARD_GRAVE_ACCENT | KEY_MOD_LEFT_GUI);
            label = "Switch";
            FURI_LOG_I(TAG, "Double: Cmd+` (switch window)");
        } else {
            SEND_HID(state, HID_KEYBOARD_TAB | KEY_MOD_LEFT_ALT);
            label = "Switch";
            FURI_LOG_I(TAG, "Double: Alt+Tab (switch window)");
        }
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
        if(state->haptics_enabled) {
            notification_message(state->notifications, &sequence_double_vibro);
        }
    }
}

/* ══════════════════════════════════════════════
 *  Draw callbacks
 * ══════════════════════════════════════════════ */

/* Scrollbar: thin proportional bar on the right edge of the content area.
 * x        = right edge x position
 * y_top    = top of scrollable area
 * y_bottom = bottom of scrollable area
 * index    = currently selected item
 * total    = total number of items */
static void draw_scrollbar(Canvas* canvas, int x, int y_top, int y_bottom, int index, int total) {
    if(total <= 1) return;
    int track_h = y_bottom - y_top;
    int thumb_h = track_h / total;
    if(thumb_h < 3) thumb_h = 3;
    int thumb_y = y_top + (index * (track_h - thumb_h)) / (total - 1);
    /* track line */
    canvas_draw_line(canvas, x, y_top, x, y_bottom);
    /* thumb */
    canvas_draw_box(canvas, x - 1, thumb_y, 3, thumb_h);
}

static void draw_splash(Canvas* canvas) {
    /* Landscape: 128w x 64h */
    canvas_clear(canvas);

    canvas_set_font(canvas, FontPrimary);
    canvas_draw_str_aligned(canvas, 64, 6, AlignCenter, AlignCenter, "Claupper");

    /* WETWARE logo bitmap — full 128px width */
    canvas_draw_xbm(canvas, 0, 14, WETWARE_LOGO_W, WETWARE_LOGO_H, wetware_logo);

    /* LABS — right-aligned under logo */
    canvas_set_font(canvas, FontSecondary);
    canvas_draw_str_aligned(canvas, 127, 40, AlignRight, AlignCenter, "LABS");

    canvas_draw_line(canvas, 0, 46, 128, 46);

    canvas_set_font(canvas, FontSecondary);
    canvas_draw_str_aligned(canvas, 64, 58, AlignCenter, AlignCenter, "Flipper's claudepanion");
}

static void draw_tour(Canvas* canvas, ClaudeRemoteState* state) {
    canvas_clear(canvas);
    if(state->tour_page == 0) {
        canvas_set_font(canvas, FontPrimary);
        canvas_draw_str_aligned(canvas, 64, 12, AlignCenter, AlignCenter, "Quick Tour?");
        canvas_set_font(canvas, FontSecondary);
        canvas_draw_line(canvas, 0, 18, 128, 18);
        canvas_draw_str_aligned(canvas, 64, 28, AlignCenter, AlignCenter, "Learn Claupper basics");
        canvas_draw_str_aligned(canvas, 64, 38, AlignCenter, AlignCenter, "in 4 quick pages");
        canvas_draw_rframe(canvas, 18, 44, 7, 7, 0);
        if(state->tour_skip) {
            canvas_draw_line(canvas, 19, 46, 23, 50);
            canvas_draw_line(canvas, 23, 46, 19, 50);
        }
        canvas_draw_str(canvas, 27, 51, "Don't ask again (down)");
        canvas_draw_str(canvas, 2, 62, "[Back] Skip");
        canvas_draw_str_aligned(canvas, 126, 62, AlignRight, AlignBottom, "Start [OK]");
        return;
    }
    char pg[8];
    snprintf(pg, sizeof(pg), "%d/4", state->tour_page);
    switch(state->tour_page) {
    case 1:
        canvas_set_font(canvas, FontPrimary);
        canvas_draw_str(canvas, 2, 10, "Remote Control");
        canvas_set_font(canvas, FontSecondary);
        canvas_draw_str_aligned(canvas, 126, 10, AlignRight, AlignCenter, pg);
        canvas_draw_line(canvas, 0, 14, 128, 14);
        canvas_draw_str(canvas, 2, 24, "1/2/3 approve Claude");
        canvas_draw_str(canvas, 2, 33, "Double-click = alt action");
        canvas_draw_str(canvas, 2, 42, "Hold Back to send Esc");
        canvas_draw_str(canvas, 2, 51, "OK+OK = switch windows");
        break;
    case 2:
        canvas_set_font(canvas, FontPrimary);
        canvas_draw_str(canvas, 2, 10, "Voice & Macros");
        canvas_set_font(canvas, FontSecondary);
        canvas_draw_str_aligned(canvas, 126, 10, AlignRight, AlignCenter, pg);
        canvas_draw_line(canvas, 0, 14, 128, 14);
        canvas_draw_str(canvas, 2, 24, "Down = voice dictation");
        canvas_draw_str(canvas, 2, 33, "Macros auto-type cmds");
        canvas_draw_str(canvas, 2, 42, "Edit macros.txt on SD");
        canvas_draw_str(canvas, 2, 51, "20 defaults built in");
        break;
    case 3:
        canvas_set_font(canvas, FontPrimary);
        canvas_draw_str(canvas, 2, 10, "Double-Click Guide");
        canvas_set_font(canvas, FontSecondary);
        canvas_draw_str_aligned(canvas, 126, 10, AlignRight, AlignCenter, pg);
        canvas_draw_line(canvas, 0, 14, 128, 14);
        canvas_draw_str(canvas, 2, 24, "Left+Left = clear line");
        canvas_draw_str(canvas, 2, 33, "Up+Up = page up");
        canvas_draw_str(canvas, 2, 42, "Right+Right = prev cmd");
        canvas_draw_str(canvas, 2, 51, "Down+Down = page down");
        break;
    case 4:
        canvas_set_font(canvas, FontPrimary);
        canvas_draw_str(canvas, 2, 10, "More Features");
        canvas_set_font(canvas, FontSecondary);
        canvas_draw_str_aligned(canvas, 126, 10, AlignRight, AlignCenter, pg);
        canvas_draw_line(canvas, 0, 14, 128, 14);
        canvas_draw_str(canvas, 2, 24, "Claude User Guide in");
        canvas_draw_str(canvas, 2, 33, "Manual with quiz mode");
        canvas_draw_str(canvas, 2, 42, "Set OS in Settings for");
        canvas_draw_str(canvas, 2, 51, "correct key combos");
        break;
    }
}

static void draw_home(Canvas* canvas) {
    /* Portrait: 64w x 128h */
    canvas_clear(canvas);
    canvas_draw_rframe(canvas, 0, 0, 64, 128, 3);

    canvas_set_font(canvas, FontPrimary);
    canvas_draw_str_aligned(canvas, 32, 12, AlignCenter, AlignCenter, "Claupper");
    canvas_draw_line(canvas, 10, 20, 54, 20);

    /* Mini D-pad illustration */
    canvas_draw_frame(canvas, 26, 28, 12, 13);
    canvas_draw_frame(canvas, 14, 40, 13, 12);
    canvas_draw_box(canvas, 26, 40, 12, 12);
    canvas_draw_frame(canvas, 37, 40, 13, 12);
    canvas_draw_frame(canvas, 26, 51, 12, 13);

    canvas_draw_line(canvas, 32, 31, 29, 35);
    canvas_draw_line(canvas, 32, 31, 35, 35);
    canvas_draw_line(canvas, 18, 46, 22, 43);
    canvas_draw_line(canvas, 18, 46, 22, 49);
    canvas_draw_line(canvas, 46, 46, 42, 43);
    canvas_draw_line(canvas, 46, 46, 42, 49);
    canvas_draw_line(canvas, 32, 61, 29, 57);
    canvas_draw_line(canvas, 32, 61, 35, 57);

    canvas_set_color(canvas, ColorWhite);
    canvas_draw_disc(canvas, 32, 46, 3);
    canvas_set_color(canvas, ColorBlack);

    canvas_set_font(canvas, FontSecondary);

    /* 5 rows, 10px each, y=74..123 — fits inside rframe */

    /* Up arrow → Macros */
    canvas_draw_frame(canvas, 4, 74, 14, 10);
    canvas_draw_line(canvas, 11, 76, 8, 80);
    canvas_draw_line(canvas, 11, 76, 14, 80);
    canvas_draw_str(canvas, 21, 82, "Macros");

    /* OK button (filled) → USB */
    canvas_draw_box(canvas, 4, 84, 14, 10);
    canvas_set_color(canvas, ColorWhite);
    canvas_draw_str_aligned(canvas, 11, 89, AlignCenter, AlignCenter, "OK");
    canvas_set_color(canvas, ColorBlack);
    canvas_draw_str(canvas, 21, 92, "USB");

    /* Left arrow → Settings */
    canvas_draw_frame(canvas, 4, 94, 14, 10);
    canvas_draw_line(canvas, 8, 99, 11, 96);
    canvas_draw_line(canvas, 8, 99, 11, 102);
    canvas_draw_str(canvas, 21, 102, "Settings");

    /* Down arrow → Manual */
    canvas_draw_frame(canvas, 4, 104, 14, 10);
    canvas_draw_line(canvas, 11, 111, 8, 107);
    canvas_draw_line(canvas, 11, 111, 14, 107);
    canvas_draw_str(canvas, 21, 112, "Manual");

    /* Right arrow → Bluetooth */
    canvas_draw_frame(canvas, 4, 114, 14, 10);
    canvas_draw_line(canvas, 14, 119, 11, 116);
    canvas_draw_line(canvas, 14, 119, 11, 122);
    canvas_draw_str(canvas, 21, 122, "Bluetooth");

}

#ifndef HID_TRANSPORT_BLE
static void draw_ble_promo(Canvas* canvas) {
    /* Landscape: 128w x 64h */
    canvas_clear(canvas);

    canvas_set_font(canvas, FontPrimary);
    canvas_draw_str_aligned(canvas, 64, 8, AlignCenter, AlignCenter, "Go Wireless!");
    canvas_draw_line(canvas, 0, 14, 128, 14);

    canvas_set_font(canvas, FontSecondary);
    canvas_draw_str(canvas, 2, 24, "This USB version works on");
    canvas_draw_str(canvas, 2, 34, "stock firmware. For wireless");
    canvas_draw_str(canvas, 2, 44, "BLE, install Momentum FW:");

    canvas_draw_str(canvas, 2, 58, "momentum-fw.dev/update");
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

    /* ══════ HOTKEY OVERLAY (full-screen, triggered by 3+Down) ══════ */
    if(state->show_hotkeys) {
        canvas_draw_rbox(canvas, 0, 0, 64, 128, 3);
        canvas_set_color(canvas, ColorWhite);

        canvas_set_font(canvas, FontPrimary);
        canvas_draw_str_aligned(canvas, 32, 10, AlignCenter, AlignCenter, "Hotkeys");
        canvas_draw_line(canvas, 6, 18, 58, 18);

        canvas_set_font(canvas, FontSecondary);
        canvas_draw_str_aligned(canvas, 32, 28, AlignCenter, AlignCenter, "Double Tap:");

        canvas_draw_str(canvas, 6, 42, "< Clear line");
        canvas_draw_str(canvas, 6, 54, "> Previous cmd");
        canvas_draw_str(canvas, 6, 66, "^ Page Up");
        canvas_draw_str(canvas, 6, 78, "v Page Down");
        canvas_draw_str(canvas, 6, 90, "o Switch win");

        canvas_draw_str_aligned(canvas, 32, 104, AlignCenter, AlignCenter, "Hold:");
        canvas_draw_str(canvas, 6, 114, "< Backspace");
        canvas_draw_str(canvas, 6, 122, "Bk Escape");

        canvas_set_color(canvas, ColorBlack);
        return;
    }

    /* ── Title bar: inverted rounded box ── */
    canvas_draw_rbox(canvas, 0, 0, 64, 14, 2);
    canvas_set_color(canvas, ColorWhite);
    canvas_set_font(canvas, FontPrimary);
    canvas_draw_str_aligned(canvas, 32, 8, AlignCenter, AlignCenter, "Claupper");
    canvas_set_color(canvas, ColorBlack);

    /* ══════ D-PAD SECTION ══════ */
    canvas_set_font(canvas, FontSecondary);

    /* "2" pill (Up direction) — rounded frame with label */
    canvas_draw_rframe(canvas, 25, 18, 14, 11, 3);
    canvas_draw_str_aligned(canvas, 32, 24, AlignCenter, AlignCenter, "2");
    /* Up arrow (filled triangle, tip up) */
    canvas_draw_line(canvas, 32, 31, 32, 31);
    canvas_draw_line(canvas, 31, 32, 33, 32);
    canvas_draw_line(canvas, 30, 33, 34, 33);
    canvas_draw_line(canvas, 29, 34, 35, 34);

    /* "1" pill (Left direction) */
    canvas_draw_rframe(canvas, 1, 37, 14, 11, 3);
    canvas_draw_str_aligned(canvas, 8, 43, AlignCenter, AlignCenter, "1");
    /* Left arrow (filled triangle, tip left) */
    canvas_draw_line(canvas, 18, 43, 18, 43);
    canvas_draw_line(canvas, 19, 42, 19, 44);
    canvas_draw_line(canvas, 20, 41, 20, 45);
    canvas_draw_line(canvas, 21, 40, 21, 46);

    /* OK button (filled disc in center) */
    canvas_draw_disc(canvas, 32, 43, 7);
    canvas_set_color(canvas, ColorWhite);
    canvas_draw_str_aligned(canvas, 32, 43, AlignCenter, AlignCenter, "OK");
    canvas_set_color(canvas, ColorBlack);

    /* Right arrow (filled triangle, tip right) */
    canvas_draw_line(canvas, 46, 43, 46, 43);
    canvas_draw_line(canvas, 45, 42, 45, 44);
    canvas_draw_line(canvas, 44, 41, 44, 45);
    canvas_draw_line(canvas, 43, 40, 43, 46);
    /* "3" pill (Right direction) */
    canvas_draw_rframe(canvas, 49, 37, 14, 11, 3);
    canvas_draw_str_aligned(canvas, 56, 43, AlignCenter, AlignCenter, "3");

    /* Down arrow (filled triangle, tip down) */
    canvas_draw_line(canvas, 29, 52, 35, 52);
    canvas_draw_line(canvas, 30, 53, 34, 53);
    canvas_draw_line(canvas, 31, 54, 33, 54);
    canvas_draw_line(canvas, 32, 55, 32, 55);
    /* "Voice" pill (Down direction) */
    canvas_draw_rframe(canvas, 17, 58, 30, 11, 3);
    canvas_draw_str_aligned(canvas, 32, 64, AlignCenter, AlignCenter, "Voice");

    /* ══════ HINT BAR ══════ */
    canvas_draw_line(canvas, 8, 74, 56, 74);
    canvas_draw_str_aligned(canvas, 32, 83, AlignCenter, AlignCenter, "Hotkeys");
    canvas_draw_str_aligned(canvas, 32, 93, AlignCenter, AlignCenter, "Right+Down");

    /* ══════ FLASH OVERLAY ══════ */
    if(state->flash_label &&
       (furi_get_tick() - state->flash_tick) < FLASH_DURATION_TICKS) {
        canvas_draw_rbox(canvas, 0, 76, 64, 52, 3);
        canvas_set_color(canvas, ColorWhite);
        canvas_set_font(canvas, FontPrimary);
        canvas_draw_str_aligned(canvas, 32, 98, AlignCenter, AlignCenter, state->flash_label);
        canvas_set_color(canvas, ColorBlack);
    }
}

/* ── Manual: Category list (landscape 128x64) ── */

static void draw_manual_categories(Canvas* canvas, ClaudeRemoteState* state) {
    canvas_clear(canvas);

    canvas_set_font(canvas, FontPrimary);
    canvas_draw_str(canvas, 2, 10, "Claupper Manual");
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

    draw_scrollbar(canvas, 125, 15, 62, state->cat_index, MENU_ITEM_COUNT);
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

    draw_scrollbar(canvas, 125, 15, 62, state->section_index, cat->section_count);
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
    while(*p && y < 64) {
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

    /* count total lines for scrollbar */
    int total_lines = 1;
    for(const char* c = sec->content; *c; c++) {
        if(*c == '\n') total_lines++;
    }
    if(total_lines > 4) {
        draw_scrollbar(canvas, 125, 15, 62, state->scroll_offset, total_lines - 3);
    }

    /* no bottom nav — user uses Left/Right to navigate sections */
}

/* ── Manual: Quiz mode (landscape 128x64) ── */

/* ── Quiz: draw helpers ── */

static void draw_quiz_question(Canvas* canvas, const char* desc) {
    canvas_set_font(canvas, FontSecondary);
    const char* p = desc;
    int y = 24;
    while(*p && y < 38) {
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

static void draw_quiz_multichoice(Canvas* canvas, ClaudeRemoteState* state, const QuizCard* card) {
    draw_quiz_question(canvas, card->description);

    if(state->quiz_answered) {
        bool was_correct = (state->quiz_selected == card->correct_option);
        const char* result_str = was_correct ? "CORRECT!" : "WRONG!";

        /* Window frame */
        canvas_set_color(canvas, ColorWhite);
        canvas_draw_rbox(canvas, 9, 19, 110, 40, 2);
        canvas_set_color(canvas, ColorBlack);
        canvas_draw_rframe(canvas, 8, 18, 112, 42, 2);

        /* Title bar stripes (classic Mac) */
        for(int sy = 21; sy <= 25; sy += 2) {
            canvas_draw_line(canvas, 10, sy, 118, sy);
        }

        /* Clear center of title bar for text */
        int title_w = strlen(result_str) * 7 + 6;
        int title_x = 64 - title_w / 2;
        canvas_set_color(canvas, ColorWhite);
        canvas_draw_box(canvas, title_x, 20, title_w, 8);
        canvas_set_color(canvas, ColorBlack);
        canvas_set_font(canvas, FontPrimary);
        canvas_draw_str_aligned(canvas, 64, 24, AlignCenter, AlignCenter, result_str);

        /* Title bar divider */
        canvas_draw_line(canvas, 8, 28, 120, 28);

        /* Answer text */
        canvas_set_font(canvas, FontSecondary);
        canvas_draw_str_aligned(canvas, 64, 40, AlignCenter, AlignCenter, card->command);

        /* Footer */
        canvas_draw_str_aligned(canvas, 64, 53, AlignCenter, AlignCenter, "OK:Next");
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

    /* difficulty picker */
    if(state->quiz_selecting) {
        canvas_set_font(canvas, FontPrimary);
        canvas_draw_str_aligned(canvas, 64, 10, AlignCenter, AlignCenter, "Quiz Mode");
        canvas_draw_line(canvas, 0, 18, 128, 18);

        /* Mac-style modal */
        canvas_set_color(canvas, ColorWhite);
        canvas_draw_rbox(canvas, 5, 21, 118, 42, 2);
        canvas_set_color(canvas, ColorBlack);
        canvas_draw_rframe(canvas, 4, 20, 120, 44, 2);

        /* Title bar stripes */
        for(int sy = 23; sy <= 27; sy += 2) {
            canvas_draw_line(canvas, 6, sy, 122, sy);
        }

        /* Clear center for title */
        canvas_set_color(canvas, ColorWhite);
        canvas_draw_box(canvas, 34, 22, 60, 8);
        canvas_set_color(canvas, ColorBlack);
        canvas_set_font(canvas, FontPrimary);
        canvas_draw_str_aligned(canvas, 64, 26, AlignCenter, AlignCenter, "Difficulty");

        /* Divider */
        canvas_draw_line(canvas, 4, 30, 124, 30);

        /* Options */
        canvas_set_font(canvas, FontSecondary);
        canvas_draw_str_aligned(canvas, 64, 38, AlignCenter, AlignCenter, "<  Easy (8)");
        canvas_draw_str_aligned(canvas, 64, 47, AlignCenter, AlignCenter, "^  Medium (16)");
        canvas_draw_str_aligned(canvas, 64, 56, AlignCenter, AlignCenter, ">  Hard (24)");
        return;
    }

    /* completion screen */
    if(state->quiz_index >= state->quiz_count) {
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

        canvas_draw_rframe(canvas, 16, 22, 96, 34, 3);
        return;
    }

    uint8_t real_index = state->quiz_order[state->quiz_index];
    const QuizCard* card = &quiz_cards[real_index];

    /* header */
    canvas_set_font(canvas, FontPrimary);
    char header[32];
    snprintf(header, sizeof(header), "Quiz %d/%d",
             state->quiz_index + 1, state->quiz_count);
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

    draw_quiz_multichoice(canvas, state, card);
}

/* ── Settings screen (landscape 128x64) ── */

static void draw_settings(Canvas* canvas, ClaudeRemoteState* state) {
    canvas_clear(canvas);

    canvas_set_font(canvas, FontPrimary);
    canvas_draw_str(canvas, 2, 10, "Settings");
    canvas_draw_line(canvas, 0, 13, 128, 13);

    canvas_set_font(canvas, FontSecondary);

    const char* labels[SETTINGS_COUNT] = {
        "Haptics", "LED", "OS", "Tour", "DblClk",
#ifdef HID_TRANSPORT_BLE
        "Bluetooth",
#endif
    };

    uint8_t first_visible = 0;
    if(state->settings_index > 2) first_visible = state->settings_index - 2;

    for(int i = 0; i < 3; i++) {
        uint8_t idx = first_visible + i;
        if(idx >= SETTINGS_COUNT) break;

        int y = 24 + i * 12;
        bool selected = (idx == state->settings_index);

        if(selected) {
            canvas_draw_box(canvas, 0, y - 9, 122, 12);
            canvas_set_color(canvas, ColorWhite);
        }

        canvas_draw_str(canvas, 6, y, labels[idx]);

        const char* val_str;
        if(idx == 0) val_str = state->haptics_enabled ? "[ON]" : "[OFF]";
        else if(idx == 1) val_str = state->led_enabled ? "[ON]" : "[OFF]";
        else if(idx == 2) val_str = state->os_mode == 1 ? "[Win]" : state->os_mode == 2 ? "[Linux]" : "[Mac]";
        else if(idx == 3) val_str = state->show_tour ? "[ON]" : "[OFF]";
        else if(idx == 4) val_str = state->dc_speed == 1 ? "[Slow]" : state->dc_speed == 2 ? "[Fast]" : "[Norm]";
#ifdef HID_TRANSPORT_BLE
        else if(idx == 5) val_str = state->ble_connected ? "[OK]" : "[...]";
#endif
        else val_str = "";
        canvas_draw_str_aligned(canvas, 110, y, AlignRight, AlignBottom, val_str);

        if(selected) {
            canvas_set_color(canvas, ColorBlack);
        }
    }

    draw_scrollbar(canvas, 125, 15, 62, state->settings_index, SETTINGS_COUNT);
}

/* ── BT Pairing sub-screen (landscape 128x64) ── */

#ifdef HID_TRANSPORT_BLE
static void draw_bt_pairing(Canvas* canvas, ClaudeRemoteState* state) {
    canvas_clear(canvas);

    canvas_set_font(canvas, FontPrimary);
    canvas_draw_str(canvas, 2, 10, "Bluetooth");
    canvas_draw_line(canvas, 0, 13, 128, 13);

    canvas_set_font(canvas, FontSecondary);

    /* Status indicator */
    const char* status_str;
    if(state->bt_status == BtStatusConnected) {
        canvas_draw_disc(canvas, 8, 25, 3);
        status_str = "Connected";
    } else if(state->bt_status == BtStatusAdvertising) {
        /* Pulsing dot: alternate filled/outline every 500ms */
        if((furi_get_tick() / 500) % 2) {
            canvas_draw_disc(canvas, 8, 25, 3);
        } else {
            canvas_draw_circle(canvas, 8, 25, 3);
        }
        status_str = "Searching...";
    } else {
        canvas_draw_circle(canvas, 8, 25, 3);
        status_str = "Unavailable";
    }
    canvas_draw_str(canvas, 16, 28, status_str);

    /* Instructions */
    canvas_draw_str(canvas, 2, 36, "Pair your device to:");

    canvas_set_font(canvas, FontPrimary);
    canvas_draw_str_aligned(canvas, 64, 50, AlignCenter, AlignBottom, "Flipper Zero");

}
#endif

/* ── Macros screen (landscape 128x64) ── */

static void draw_macros(Canvas* canvas, ClaudeRemoteState* state) {
    canvas_clear(canvas);

    canvas_set_font(canvas, FontPrimary);
    canvas_draw_str(canvas, 2, 10, "Macros");
    canvas_draw_line(canvas, 0, 13, 128, 13);

    canvas_set_font(canvas, FontSecondary);

    if(state->macro_count == 0) {
        canvas_draw_str_aligned(canvas, 64, 30, AlignCenter, AlignCenter, "No macros found");
        canvas_draw_str_aligned(canvas, 64, 42, AlignCenter, AlignCenter, "Add lines to:");
        canvas_draw_str(canvas, 4, 52, MACROS_PATH);
    } else {
        uint8_t first_visible = 0;
        if(state->macro_index > 2) first_visible = state->macro_index - 2;

        for(int i = 0; i < 3; i++) {
            uint8_t idx = first_visible + i;
            if(idx >= state->macro_count) break;

            int y = 24 + i * 12;
            bool selected = (idx == state->macro_index);

            if(selected) {
                canvas_draw_box(canvas, 0, y - 9, 122, 12);
                canvas_set_color(canvas, ColorWhite);
            }

            char display[MACRO_MAX_LEN + 8];
            snprintf(display, sizeof(display), "%d. %s", idx + 1, state->macros[idx]);

            if(selected) {
                uint16_t text_w = canvas_string_width(canvas, display);
                uint16_t max_w = 118; /* visible text area */
                if(text_w > max_w) {
                    /* Marquee: pause 1s, then scroll, pause at end, loop */
                    uint32_t elapsed = furi_get_tick() - state->macro_scroll_tick;
                    uint32_t pause_ms = 800;
                    uint32_t scroll_speed = 20; /* pixels per 100ms */
                    uint32_t overflow = text_w - max_w;
                    uint32_t scroll_time = (overflow * 100) / scroll_speed;
                    uint32_t cycle = pause_ms + scroll_time + pause_ms;
                    uint32_t phase = elapsed % cycle;
                    int offset = 0;
                    if(phase < pause_ms) {
                        offset = 0;
                    } else if(phase < pause_ms + scroll_time) {
                        offset = (int)((phase - pause_ms) * scroll_speed / 100);
                        if(offset > (int)overflow) offset = (int)overflow;
                    } else {
                        offset = overflow;
                    }
                    canvas_draw_str(canvas, 4 - offset, y, display);
                } else {
                    canvas_draw_str(canvas, 4, y, display);
                }
            } else {
                canvas_draw_str(canvas, 4, y, display);
            }

            if(selected) {
                canvas_set_color(canvas, ColorBlack);
            }
        }

        draw_scrollbar(canvas, 125, 15, 62, state->macro_index, state->macro_count);
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
    case ModeTour:
        draw_tour(canvas, state);
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
    case ModeSettings:
#ifdef HID_TRANSPORT_BLE
        if(state->bt_pair_screen) {
            draw_bt_pairing(canvas, state);
        } else
#endif
        {
            draw_settings(canvas, state);
        }
        break;
    case ModeMacros:
        draw_macros(canvas, state);
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

static void tour_go_home(ClaudeRemoteState* state, ViewPort* vp) {
    if(state->tour_skip) {
        state->show_tour = false;
        save_settings(state);
    }
    state->mode = ModeHome;
    view_port_set_orientation(vp, ViewPortOrientationVertical);
}

static void handle_tour_input(ClaudeRemoteState* state, InputEvent* event, ViewPort* vp) {
    if(event->type != InputTypeShort) return;
    if(state->tour_page == 0) {
        switch(event->key) {
        case InputKeyOk: state->tour_page = 1; break;
        case InputKeyUp: case InputKeyDown: state->tour_skip = !state->tour_skip; break;
        case InputKeyBack: tour_go_home(state, vp); break;
        default: break;
        }
        return;
    }
    switch(event->key) {
    case InputKeyRight: if(state->tour_page < 4) state->tour_page++; break;
    case InputKeyLeft: if(state->tour_page > 1) state->tour_page--; break;
    case InputKeyOk:
        if(state->tour_page < 4) state->tour_page++;
        else tour_go_home(state, vp);
        break;
    case InputKeyBack: tour_go_home(state, vp); break;
    default: break;
    }
}

static bool handle_home_input(ClaudeRemoteState* state, InputEvent* event, ViewPort* vp) {
    if(event->type != InputTypeShort) return true;

    switch(event->key) {
    case InputKeyOk:
#ifdef HID_TRANSPORT_BLE
        state->use_ble = false;
        state->hid_connected = furi_hal_hid_is_connected();
#endif
        state->mode = ModeRemote;
        if(state->led_enabled) {
            notification_message(state->notifications, &sequence_solid_blue);
        }
        break;
    case InputKeyRight:
#ifdef HID_TRANSPORT_BLE
        state->use_ble = true;
        state->hid_connected = state->ble_connected;
        state->mode = ModeRemote;
        if(state->led_enabled) {
            notification_message(state->notifications, &sequence_solid_blue);
        }
#else
        state->mode = ModeBlePromo;
        view_port_set_orientation(vp, ViewPortOrientationHorizontal);
#endif
        break;
    case InputKeyDown:
        state->cat_index = 0;
        state->section_index = 0;
        state->scroll_offset = 0;
        state->manual_view = ManualViewCategories;
        state->mode = ModeManual;
        if(state->led_enabled) {
            notification_message(state->notifications, &sequence_solid_green);
        }
        view_port_set_orientation(vp, ViewPortOrientationHorizontal);
        break;
    case InputKeyLeft:
        state->settings_index = 0;
        state->mode = ModeSettings;
        if(state->led_enabled) {
            notification_message(state->notifications, &sequence_solid_orange);
        }
        view_port_set_orientation(vp, ViewPortOrientationHorizontal);
        break;
    case InputKeyUp:
        if(!state->macros_loaded) {
            load_macros_from_sd(state);
        }
        state->macro_index = 0;
        state->macro_scroll_tick = furi_get_tick();
        state->mode = ModeMacros;
        if(state->led_enabled) {
            notification_message(state->notifications, &sequence_solid_orange);
        }
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

    /* ── Track held keys for Right+Down combo (hotkey overlay) ── */
    if(event->type == InputTypePress) {
        if(event->key == InputKeyRight) state->right_held = true;
        if(event->key == InputKeyDown) state->down_held = true;
        if(event->key == InputKeyLeft) {
            state->left_holding = true;
            state->left_hold_start = furi_get_tick();
            state->left_repeat_tick = 0;
        }
        if(state->right_held && state->down_held && !state->show_hotkeys) {
            state->show_hotkeys = true;
            state->hotkeys_tick = furi_get_tick();
            state->dc_pending = false;
            return true;
        }
    }
    if(event->type == InputTypeRelease) {
        if(event->key == InputKeyRight) state->right_held = false;
        if(event->key == InputKeyDown) state->down_held = false;
        if(event->key == InputKeyLeft) state->left_holding = false;
        return true;
    }

    /* ── Hold Left = backspace repeat with acceleration ── */
    if(event->key == InputKeyLeft && state->left_holding &&
       (event->type == InputTypeLong || event->type == InputTypeRepeat)) {
        /* Cancel double-click pending for Left (don't send "1") */
        if(state->dc_pending && state->dc_key == InputKeyLeft) {
            state->dc_pending = false;
        }
        if(state->hid_connected) {
            uint32_t now = furi_get_tick();
            uint32_t held_ms = now - state->left_hold_start;
            /* 250ms repeat initially, 100ms after 2 seconds (60% faster) */
            uint32_t interval = (held_ms > 2000) ? 100 : 250;
            if(state->left_repeat_tick == 0 ||
               (now - state->left_repeat_tick) >= interval) {
                SEND_HID(state, HID_KEYBOARD_DELETE);
                state->left_repeat_tick = now;
            }
        }
        return true;
    }

    /* ── Dismiss hotkey overlay (ignore first 400ms to absorb combo release) ── */
    if(state->show_hotkeys) {
        if(event->type == InputTypeShort &&
           (furi_get_tick() - state->hotkeys_tick) > 400) {
            state->show_hotkeys = false;
        }
        return true;
    }

    if(event->type != InputTypeShort && event->type != InputTypeLong) return true;

    if(event->key == InputKeyBack) {
        if(event->type == InputTypeLong) {
            /* Long-press Back → send Escape */
            if(state->hid_connected) {
                SEND_HID(state, HID_KEYBOARD_ESCAPE);
                state->flash_label = "Esc";
                state->flash_tick = furi_get_tick();
                if(state->haptics_enabled) {
                    notification_message(state->notifications, &sequence_single_vibro);
                }
            }
        } else {
            /* Short-press Back → go Home (discard pending key, don't send it) */
            state->dc_pending = false;
            state->mode = ModeHome;
            if(state->led_enabled) {
                notification_message(state->notifications, &sequence_solid_orange);
            }
            view_port_set_orientation(view_port, ViewPortOrientationVertical);
        }
        return true;
    }

    /* Only process short presses for d-pad keys (long-press would trigger double-click) */
    if(event->type != InputTypeShort) return true;

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
       (now - state->dc_tick) < dc_timeout(state)) {
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
            /* quiz mode — show difficulty picker */
            state->quiz_selecting = true;
            state->manual_view = ManualViewQuiz;
        }
        break;
    case InputKeyBack:
        state->mode = ModeHome;
        if(state->led_enabled) {
            notification_message(state->notifications, &sequence_solid_orange);
        }
        view_port_set_orientation(vp, ViewPortOrientationVertical);
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

static void quiz_start(ClaudeRemoteState* state, uint8_t count) {
    quiz_shuffle(state);
    state->quiz_selecting = false;
    state->quiz_count = count;
    state->quiz_index = 0;
    state->quiz_correct = 0;
    state->quiz_total = 0;
    state->quiz_streak = 0;
    state->quiz_best_streak = 0;
    state->quiz_selected = -1;
    state->quiz_answered = false;
}

static void handle_manual_quiz(ClaudeRemoteState* state, InputEvent* event) {
    /* difficulty picker */
    if(state->quiz_selecting) {
        switch(event->key) {
        case InputKeyLeft:  quiz_start(state, 8);  return;
        case InputKeyUp:    quiz_start(state, 16); return;
        case InputKeyRight: quiz_start(state, 24); return;
        case InputKeyBack:
            state->manual_view = ManualViewCategories;
            return;
        default: return;
        }
    }

    /* completion screen */
    if(state->quiz_index >= state->quiz_count) {
        if(event->key == InputKeyOk) {
            state->quiz_selecting = true;
        } else if(event->key == InputKeyBack) {
            state->manual_view = ManualViewCategories;
        }
        return;
    }

    uint8_t real_index = state->quiz_order[state->quiz_index];
    const QuizCard* card = &quiz_cards[real_index];

    /* multi-choice input */
    {
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

/* ── Settings input handler ── */

static bool handle_settings_input(ClaudeRemoteState* state, InputEvent* event, ViewPort* vp) {
    if(event->type != InputTypeShort) return true;

#ifdef HID_TRANSPORT_BLE
    /* BT pairing sub-screen input */
    if(state->bt_pair_screen) {
        switch(event->key) {
        case InputKeyOk:
            /* Forget all paired devices and re-advertise */
            bt_disconnect(state->bt);
            furi_delay_ms(200);
            bt_forget_bonded_devices(state->bt);
            FURI_LOG_I(TAG, "BT devices forgotten");
            if(state->haptics_enabled) {
                notification_message(state->notifications, &sequence_single_vibro);
            }
            break;
        case InputKeyBack:
            state->bt_pair_screen = false;
            break;
        default:
            break;
        }
        return true;
    }
#endif

    switch(event->key) {
    case InputKeyUp:
        if(state->settings_index > 0) state->settings_index--;
        break;
    case InputKeyDown:
        if(state->settings_index < SETTINGS_COUNT - 1) state->settings_index++;
        break;
    case InputKeyOk:
        if(state->settings_index == 0) {
            state->haptics_enabled = !state->haptics_enabled;
        } else if(state->settings_index == 1) {
            state->led_enabled = !state->led_enabled;
        } else if(state->settings_index == 2) {
            state->os_mode = (state->os_mode + 1) % 3;
        } else if(state->settings_index == 3) {
            state->show_tour = !state->show_tour;
        } else if(state->settings_index == 4) {
            state->dc_speed = (state->dc_speed + 1) % 3;
        }
#ifdef HID_TRANSPORT_BLE
        else if(state->settings_index == 5) {
            state->bt_pair_screen = true;
            return true; /* don't save settings for this */
        }
#endif
        save_settings(state);
        break;
    case InputKeyBack:
        state->mode = ModeHome;
        if(state->led_enabled) {
            notification_message(state->notifications, &sequence_solid_orange);
        }
        view_port_set_orientation(vp, ViewPortOrientationVertical);
        break;
    default:
        break;
    }
    return true;
}

/* ── Macros input handler ── */

static bool handle_macros_input(ClaudeRemoteState* state, InputEvent* event, ViewPort* vp) {
    if(event->type != InputTypeShort) return true;

    switch(event->key) {
    case InputKeyUp:
        if(state->macro_index > 0) {
            state->macro_index--;
            state->macro_scroll_tick = furi_get_tick();
        }
        break;
    case InputKeyDown:
        if(state->macro_count > 0 && state->macro_index < state->macro_count - 1) {
            state->macro_index++;
            state->macro_scroll_tick = furi_get_tick();
        }
        break;
    case InputKeyOk:
        if(state->macro_count > 0 && state->hid_connected) {
            /* Release mutex during macro send so draw_callback isn't blocked */
            char macro_buf[MACRO_MAX_LEN + 1];
            memcpy(macro_buf, state->macros[state->macro_index], sizeof(macro_buf));
            furi_mutex_release(state->mutex);
            send_macro_string(state, macro_buf);
            furi_mutex_acquire(state->mutex, FuriWaitForever);
        }
        break;
    case InputKeyBack:
        state->mode = ModeHome;
        if(state->led_enabled) {
            notification_message(state->notifications, &sequence_solid_orange);
        }
        view_port_set_orientation(vp, ViewPortOrientationVertical);
        break;
    default:
        break;
    }
    return true;
}

/* ══════════════════════════════════════════════
 *  Main app logic
 * ══════════════════════════════════════════════ */

static int32_t claude_remote_main(void* p) {
    UNUSED(p);
    FURI_LOG_I(TAG, "Starting Claupper");

    ClaudeRemoteState* state = malloc(sizeof(ClaudeRemoteState));
    memset(state, 0, sizeof(ClaudeRemoteState));
    state->mode = ModeSplash;
    state->splash_start = furi_get_tick();
    state->mutex = furi_mutex_alloc(FuriMutexTypeNormal);

    state->notifications = furi_record_open(RECORD_NOTIFICATION);
    load_settings(state);
    if(state->led_enabled) {
        notification_message(state->notifications, &sequence_solid_orange);
    }

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
    /* Init BLE HID with per-app key storage for bonding persistence */
    {
        Storage* storage = furi_record_open(RECORD_STORAGE);
        storage_simply_mkdir(storage, APP_DATA_DIR);
        furi_record_close(RECORD_STORAGE);
    }
    state->bt = furi_record_open(RECORD_BT);
    bt_disconnect(state->bt);
    furi_delay_ms(200);
    bt_keys_storage_set_storage_path(state->bt, APP_DATA_PATH(".bt.keys"));
    state->ble_profile = bt_profile_start(state->bt, ble_profile_hid, NULL);
    bt_set_status_changed_callback(state->bt, bt_status_callback, state);
    FURI_LOG_I(TAG, "BLE HID started, keys: %s", APP_DATA_PATH(".bt.keys"));
#endif

    InputEvent event;
    bool running = true;

    while(running) {
        FuriStatus status = furi_message_queue_get(queue, &event, 100);

        furi_mutex_acquire(state->mutex, FuriWaitForever);

        /* auto-advance splash after 3 seconds, or skip on any press */
        if(state->mode == ModeSplash) {
            if(status == FuriStatusOk || (furi_get_tick() - state->splash_start) >= 3000) {
                if(state->show_tour) {
                    state->mode = ModeTour;
                    state->tour_page = 0;
                } else {
                    state->mode = ModeHome;
                    view_port_set_orientation(view_port, ViewPortOrientationVertical);
                }
            }
            furi_mutex_release(state->mutex);
            view_port_update(view_port);
            continue;
        }

        if(status == FuriStatusOk) {
            switch(state->mode) {
            case ModeSplash:
                break; /* handled above */
            case ModeTour:
                handle_tour_input(state, &event, view_port);
                break;
            case ModeHome:
                running = handle_home_input(state, &event, view_port);
                break;
            case ModeRemote:
                running = handle_remote_input(state, &event, view_port);
                break;
            case ModeManual:
                running = handle_manual_input(state, &event, view_port);
                break;
            case ModeSettings:
                running = handle_settings_input(state, &event, view_port);
                break;
            case ModeMacros:
                running = handle_macros_input(state, &event, view_port);
                break;
#ifndef HID_TRANSPORT_BLE
            case ModeBlePromo:
                if(event.type == InputTypeShort && event.key == InputKeyBack) {
                    state->mode = ModeHome;
                    if(state->led_enabled) {
                        notification_message(state->notifications, &sequence_solid_orange);
                    }
                    view_port_set_orientation(view_port, ViewPortOrientationVertical);
                }
                break;
#endif
            }
        }

        if(state->mode == ModeRemote || state->mode == ModeMacros) {
#ifdef HID_TRANSPORT_BLE
            if(state->use_ble) {
                state->hid_connected = state->ble_connected;
            } else {
                state->hid_connected = furi_hal_hid_is_connected();
            }
#else
            state->hid_connected = furi_hal_hid_is_connected();
#endif
            /* flush pending single-press after double-click timeout (remote only) */
            if(state->mode == ModeRemote && state->dc_pending &&
               (furi_get_tick() - state->dc_tick) >= dc_timeout(state)) {
                flush_pending_single(state);
            }
        }

        furi_mutex_release(state->mutex);
        view_port_update(view_port);
    }

    FURI_LOG_I(TAG, "Exiting Claupper");

    notification_message(state->notifications, &sequence_reset_rgb);
    furi_record_close(RECORD_NOTIFICATION);

    /* Cleanup USB HID (both builds) */
    furi_hal_hid_kb_release_all();
    furi_hal_usb_set_config(state->usb_prev, NULL);

#ifdef HID_TRANSPORT_BLE
    /* Also cleanup BLE HID */
    bt_set_status_changed_callback(state->bt, NULL, NULL);
    ble_profile_hid_kb_release_all(state->ble_profile);
    bt_keys_storage_set_default_path(state->bt);
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
