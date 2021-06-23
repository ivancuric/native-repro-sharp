const rimraf = require('rimraf');

rimraf('FFmpeg', (e) => console.log(e));
rimraf('scripts', (e) => console.log(e));
