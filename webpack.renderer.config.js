const rules = require('./webpack.rules');
const plugins = require('./webpack.plugins');
const TsconfigPathsPlugin = require('tsconfig-paths-webpack-plugin');
const externals = require('./webpack.externals');

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
  externals,
  plugins,
  resolve: {
    extensions: ['.js', '.ts', '.jsx', '.tsx', '.css', '.glsl'],
    plugins: [new TsconfigPathsPlugin()],
    alias: {
      'react-dom': '@hot-loader/react-dom',
    },
  },
};
