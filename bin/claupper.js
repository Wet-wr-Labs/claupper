#!/usr/bin/env node

const fs = require('fs');
const path = require('path');
const os = require('os');

const home = os.homedir();
const skillDir = path.join(home, '.claude', 'skills', 'claupper');
const clapperDir = path.join(home, 'claupper');
const repoRoot = path.join(__dirname, '..');

const skillSrc = path.join(repoRoot, 'skill', 'claupper', 'SKILL.md');
const macroSrc = path.join(repoRoot, 'macros');

if (process.argv.includes('--uninstall')) {
  let removed = false;
  if (fs.existsSync(skillDir)) {
    fs.rmSync(skillDir, { recursive: true });
    console.log('  Removed skill from', skillDir);
    removed = true;
  }
  if (fs.existsSync(clapperDir)) {
    fs.rmSync(clapperDir, { recursive: true });
    console.log('  Removed files from', clapperDir);
    removed = true;
  }
  console.log(removed ? '\n  Claupper uninstalled.' : '  Nothing to uninstall.');
  process.exit(0);
}

console.log('\n  Claupper Setup\n');

try {
  fs.mkdirSync(skillDir, { recursive: true });
  fs.copyFileSync(skillSrc, path.join(skillDir, 'SKILL.md'));
  console.log('  [1/2] Skill installed -> ~/.claude/skills/claupper/');
} catch (err) {
  console.error('  Failed to install skill:', err.message);
  process.exit(1);
}

try {
  const macroDest = path.join(clapperDir, 'macros');
  fs.mkdirSync(macroDest, { recursive: true });
  for (const f of fs.readdirSync(macroSrc)) {
    if (!f.endsWith('.txt')) continue;
    fs.copyFileSync(path.join(macroSrc, f), path.join(macroDest, f));
  }
  console.log('  [2/2] Macro presets -> ~/claupper/macros/');
} catch (err) {
  console.error('  Failed to copy macros:', err.message);
  process.exit(1);
}

console.log('\n  Done!\n');
console.log('  Claude Code:');
console.log('    Type /claupper in any session to activate remote mode.\n');
console.log('  Flipper Zero app:');
console.log('    Install from the Flipper App Catalog (lab.flipper.net) via');
console.log('    qFlipper or the Flipper mobile app. The catalog handles the');
console.log('    .fap deployment to your SD card. Or grab the .fap directly');
console.log('    from https://github.com/Wet-wr-Labs/claupper/releases\n');
console.log('  Macros (optional):');
console.log('    Copy a preset from ~/claupper/macros/ to your Flipper SD card:');
console.log('    SD/apps_data/claude_remote_ble/macros.txt\n');
console.log('  Uninstall:');
console.log('    npx github:Wet-wr-Labs/claupper --uninstall\n');
