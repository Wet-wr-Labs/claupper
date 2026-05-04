#!/usr/bin/env node

const fs = require('fs');
const path = require('path');
const os = require('os');

const home = os.homedir();
const skillDir = path.join(home, '.claude', 'skills', 'claupper');
const clapperDir = path.join(home, 'claupper');
const repoRoot = path.join(__dirname, '..');

const fapSrc = path.join(repoRoot, 'dist');
const macroSrc = path.join(repoRoot, 'macros');
const skillSrc = path.join(repoRoot, 'skill', 'claupper', 'SKILL.md');

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
  console.log('  [1/3] Skill installed -> ~/.claude/skills/claupper/');
} catch (err) {
  console.error('  Failed to install skill:', err.message);
  process.exit(1);
}

try {
  fs.mkdirSync(clapperDir, { recursive: true });
  for (const f of fs.readdirSync(fapSrc)) {
    if (!f.endsWith('.fap')) continue;
    fs.copyFileSync(path.join(fapSrc, f), path.join(clapperDir, f));
  }
  console.log('  [2/3] Flipper apps  -> ~/claupper/');
} catch (err) {
  console.error('  Failed to copy .fap files:', err.message);
  process.exit(1);
}

try {
  const macroDest = path.join(clapperDir, 'macros');
  fs.mkdirSync(macroDest, { recursive: true });
  for (const f of fs.readdirSync(macroSrc)) {
    if (!f.endsWith('.txt')) continue;
    fs.copyFileSync(path.join(macroSrc, f), path.join(macroDest, f));
  }
  console.log('  [3/3] Macro presets -> ~/claupper/macros/');
} catch (err) {
  console.error('  Failed to copy macros:', err.message);
  process.exit(1);
}

console.log('\n  Done! Next steps:\n');
console.log('  Claude Code:');
console.log('    Type /claupper in any Claude Code session to activate remote mode.\n');
console.log('  Flipper Zero:');
console.log('    Most users install the app directly from the Flipper App Catalog');
console.log('    (qFlipper or the Flipper mobile app). If you have the .fap locally,');
console.log('    copy it to your Flipper SD card:');
console.log('    - BLE (Momentum/Unleashed): ~/claupper/claude_remote_ble.fap');
console.log('      -> SD/apps/Bluetooth/');
console.log('    - USB (stock firmware):     ~/claupper/claude_remote_usb.fap');
console.log('      -> SD/apps/USB/\n');
console.log('  Macros (optional):');
console.log('    Copy a macro preset to your Flipper SD card:');
console.log('    ~/claupper/macros/default.txt -> SD/apps_data/claude_remote_ble/macros.txt\n');
console.log('  Uninstall:');
console.log('    npx github:Wet-wr-Labs/claupper --uninstall\n');
