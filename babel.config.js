// babel.config.js
module.exports = {
  presets: [
    ['@babel/preset-env', { targets: { node: 'current' } }],
    [
      '@babel/preset-react',
      { runtime: 'automatic', importSource: '@emotion/react' },
    ],
    ['@babel/preset-typescript', { allowDeclareFields: true }],
  ],
  plugins: ['@emotion'],
};

// TODO: different presets?

// module.exports = api => {
//   const isTest = api.env('test');
//   // You can use isTest to determine what presets and plugins to use.

//   return {
//     // ...
//   };
// };
