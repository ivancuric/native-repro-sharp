/* eslint-disable @typescript-eslint/no-unused-vars */
import 'react-hot-loader';

import helloWorld from 'native-hello-world';
import ReactDOM from 'react-dom';
import sharp from 'sharp';
import videoModule from 'video-module';

import App from '~/components/App';

console.log(sharp);
helloWorld();
console.log(videoModule);

ReactDOM.render(<App />, document.getElementById('root'));
