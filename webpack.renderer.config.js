const rules = require('./webpack.rules');
const plugins = require('./webpack.plugins');
const TsconfigPathsPlugin = require('tsconfig-paths-webpack-plugin');

rules.push(
  {
    test: /\.css$/,
    use: [{ loader: 'style-loader' }, { loader: 'css-loader' }],
  },
  {
    test: /\.glsl$/,
    use: [{ loader: 'raw-loader' }],
  },
  {
    test: /\.(ttf|woff|woff2)$/,
    use: {
      loader: 'file-loader',
      options: {
        name: 'fonts/[name].[ext]',
        publicPath: '../.',
      },
    },
  },
);

module.exports = {
  module: {
    rules,
  },
  externals: {
    sharp: 'commonjs2 sharp',
    'video-module': 'commonjs2 video-module',
  },
  plugins,
  resolve: {
    extensions: ['.js', '.ts', '.jsx', '.tsx', '.css', '.glsl'],
    plugins: [new TsconfigPathsPlugin()],
    alias: {
      'react-dom': '@hot-loader/react-dom',
    },
  },
};
