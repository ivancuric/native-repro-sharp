module.exports = {
  '*.{js,jsx,ts,tsx}': ['eslint --cache --fix --max-warnings=0'],
  '*.{ts,tsx}': [() => 'tsc --skipLibCheck --noEmit'],
  '*.{js,jsx,ts,tsx,css,md}': ['prettier --write'],
};
