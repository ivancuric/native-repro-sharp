const TsconfigPathsPlugin = require('tsconfig-paths-webpack-plugin');
const plugins = require('./webpack.plugins');
const externals = require('./webpack.externals');

module.exports = {
  /**
   * This is the main entry point for your application, it's the first file
   * that runs in the main process.
   */
  entry: './app/main/index.ts',
  // Put your normal webpack config below here
  module: {
    rules: require('./webpack.rules'),
  },
  plugins,
  resolve: {
    extensions: ['.js', '.ts', '.jsx', '.tsx', '.css', '.json'],
    plugins: [new TsconfigPathsPlugin()],
  },
  externals,
};
