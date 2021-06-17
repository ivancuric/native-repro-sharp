import chalk from 'chalk';
import os from 'os';
import { exit } from 'process';

const isVsCode = process.env.TERM_PROGRAM === 'vscode';
const isMacOs = os.type() === 'Darwin';

if (isVsCode && isMacOs) {
  console.error(
    chalk.red.bold(
      '\n\nDev builds must be launched from Terminal or iTerm2 on MacOS!',
    ),
    `\nVScode's terminal can't be registered for camera permissions\n\n`,
  );
  exit(1);
}
