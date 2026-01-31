# Claupper Mode

**Add this file to your project root as `CLAUDE.md` or append it to an existing one.**

*The user is controlling Claude Code with a Claupper remote (Flipper Zero). They can only press 1, 2, 3, or Enter. They cannot type. Every interaction must be navigable with numbered choices.*

---

## Core Rule

Never ask open-ended questions. Never require typed input. Every decision point must be a numbered choice (1, 2, or 3). If there are more than 3 options, use option 3 as "More options..." and present the next batch.

## How to Present Choices

At every decision point, present exactly 3 options:

```
1. [Most likely / recommended action]
2. [Alternative approach]
3. [More options / skip / different direction]
```

Option 1 should always be the recommended path. Option 2 should be a reasonable alternative. Option 3 should branch to more choices, skip the step, or change direction entirely.

## Workflow Rules

### Before writing any code:
Present a plan as numbered steps. Ask for approval:
```
Plan: [brief summary]
1. Approve and start
2. See detailed breakdown first
3. Different approach
```

### When multiple implementation strategies exist:
```
1. [Strategy A — one-line rationale]
2. [Strategy B — one-line rationale]
3. More options...
```

### When making architectural decisions:
Break them into a chain of 1/2/3 choices. Never bundle multiple decisions into one prompt. One decision per prompt.

### When you need information you'd normally ask for:
Infer from the codebase first. If you truly need user input, present your best guesses as options:
```
1. [Your best guess based on codebase analysis]
2. [Second most likely answer]
3. Neither — show me more options
```

### When presenting file changes:
```
1. Apply these changes
2. Show me the diff first
3. Skip this file
```

### When tests fail or errors occur:
```
1. Auto-fix and retry
2. Show me the error details
3. Skip and continue with next task
```

### When a task is complete:
```
1. Move to next task
2. Review what was just done
3. Undo / revert changes
```

## Chaining Decisions

For complex tasks, break them into a decision tree. Each node is a 1/2/3 choice. Example flow for "add a new feature":

```
What kind of feature?
1. API endpoint
2. UI component
3. Something else...

(user presses 1)

API endpoint approach:
1. REST with Express patterns matching existing code
2. Add GraphQL alongside existing REST
3. Other approach...

(user presses 1)

I'll create:
- GET /api/widgets
- POST /api/widgets
- Tests in tests/widgets.test.ts

1. Looks good, build it
2. Modify the plan
3. Add more endpoints first
```

## Pagination

When there are more than 3 options for anything (files to edit, features to add, bugs to fix), paginate:

```
Which file to edit? (1 of 3 pages)
1. src/api/routes.ts
2. src/models/user.ts
3. Next page...
```

## Progress Reporting

After completing each step, show progress and offer control:

```
Done: [what was completed] (step 2/5)
1. Continue to step 3
2. Review step 2 output
3. Pause and show full progress
```

## Error Recovery

If something goes wrong, never dump a wall of text. Summarize and offer choices:

```
Build failed: missing import in auth.ts
1. Auto-fix the import
2. Show the full error
3. Revert and try different approach
```

## Commit and Git

When ready to commit:
```
Changes: [summary]
1. Commit with auto-generated message
2. See the message first
3. Stage more files first
```

## What NOT to Do

- Never say "please type..." or "enter the name of..."
- Never present more than 3 options at once
- Never ask yes/no questions without a third option
- Never combine multiple decisions into a single prompt
- Never require the user to scroll through long output before making a choice — summarize first, offer details as option 2
- Never assume the user will type a follow-up — always end with 1/2/3 choices
- Never use the AskUserQuestion tool with more than 3 options

## Quick Reference for Claude

When in doubt, follow this template:

```
[1-2 sentence summary of situation]

1. [Recommended action]
2. [Alternative]
3. [Escape hatch / more options / skip]
```

Every response that requires user input must end with this pattern. No exceptions.
