/* eslint-disable @typescript-eslint/no-unused-vars */

const { spawn } = require('child_process');
const fs = require('fs-extra');
const path = require('path');

const config = {
  packagerConfig: {
    name: 'demo',
    executableName: 'demo',
    // @electron-forge/plugin-auto-unpack-natives doesn't work, so we have to do
    // it manually. It's actually not required if using a bundler.
    // asar: {
    //   unpack: '*.+(node|dll)',
    // },
    appBundleId: 'com.demo.poc',
    usageDescription: {
      Camera: 'Access is needed to use the Camera',
      Microphone: 'Access is to use the Microphone',
    },
    appCategoryType: 'public.app-category.video',
    osxSign: {
      identity: 'demo',
      hardenedRuntime: true,
      'gatekeeper-assess': false,
      entitlements: 'entitlements.plist',
      'entitlements-inherit': 'entitlements.plist',
      'signature-flags': 'library',
    },
    // osxNotarize: {
    //   appleId: '****',
    //   appleIdPassword: '****',
    // },
  },

  makers: [
    {
      name: '@electron-forge/maker-squirrel',
      config: {
        name: 'demo',
      },
    },
    {
      name: '@electron-forge/maker-zip',
      platforms: ['darwin'],
    },
    {
      name: '@electron-forge/maker-deb',
      config: {},
    },
    {
      name: '@electron-forge/maker-rpm',
      config: {},
    },
  ],
  plugins: [
    [
      '@electron-forge/plugin-webpack',
      {
        mainConfig: './webpack.main.config.js',
        renderer: {
          config: './webpack.renderer.config.js',
          entryPoints: [
            {
              html: './app/renderer/index.html',
              js: './app/renderer/index.tsx',
              name: 'main_window',
            },
          ],
        },
      },
    ],
    [
      '@timfish/forge-externals-plugin',
      {
        externals: ['sharp', 'video-module', 'native-hello-world'],
      },
    ],
  ],
  // https://github.com/electron-userland/electron-forge/issues/1250#issuecomment-667543166
  // hooks: {
  //   readPackageJson: async (forgeConfig, packageJson) => {
  //     // only copy deps if there isn't any
  //     if (Object.keys(packageJson.dependencies).length === 0) {
  //       const originalPackageJson = await fs.readJson(
  //         path.resolve(__dirname, 'package.json'),
  //       );
  //       const webpackConfigJs = require('./webpack.renderer.config.js');
  //       Object.keys(webpackConfigJs.externals).forEach((package) => {
  //         packageJson.dependencies[package] =
  //           originalPackageJson.dependencies[package];
  //       });
  //     }
  //     return packageJson;
  //   },
  //   packageAfterPrune: async (forgeConfig, buildPath) => {
  //     console.log(buildPath);
  //     return new Promise((resolve, reject) => {
  //       const npmInstall = spawn('npm', ['install'], {
  //         cwd: buildPath,
  //         stdio: 'inherit',
  //       });

  //       npmInstall.on('close', (code) => {
  //         if (code === 0) {
  //           resolve();
  //         } else {
  //           reject(new Error('process finished with error code ' + code));
  //         }
  //       });

  //       npmInstall.on('error', (error) => {
  //         reject(error);
  //       });
  //     });
  //   },
  // },
};

module.exports = config;
