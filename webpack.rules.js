module.exports = [
  {
    test: /\.node$/,
    use: 'node-loader',
  },
  {
    test: /\.(m?js|node)$/,
    parser: { amd: false },
    use: {
      loader: '@timfish/webpack-asset-relocator-loader',
      options: {
        outputAssetBase: 'native_modules',
      },
    },
  },
  {
    test: /\.tsx?$/,
    exclude: /(node_modules|\.webpack)/,
    use: [
      {
        loader: 'babel-loader',
        options: {
          plugins: ['@emotion', 'react-hot-loader/babel'],
        },
      },
      {
        loader: 'ts-loader',
        options: { transpileOnly: true },
      },
    ],
  },
];
