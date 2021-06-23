/* eslint-disable @typescript-eslint/no-unused-vars */
import 'react-hot-loader';

import ReactDOM from 'react-dom';
import sharp from 'sharp';
import videoModule from 'video-module';

import App from '~/components/App';

// @ts-ignore
// window.sharp = sharp;
// @ts-ignore
window.videoModule = videoModule;

console.log(sharp);
console.log(videoModule);

ReactDOM.render(<App />, document.getElementById('root'));
