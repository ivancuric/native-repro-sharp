import 'react-hot-loader';

import helloWorld from 'native-hello-world';
import ReactDOM from 'react-dom';
import sharp from 'sharp';

import App from '~/components/App';

console.log(sharp);
helloWorld();

ReactDOM.render(<App />, document.getElementById('root'));
