import { ipcRenderer } from 'electron';
import fs from 'fs-extra';
import path from 'path';
import sharp from 'sharp';
import videoModule from 'video-module';

// SHARP IS EXPOSED IN window.api.sharp
const exposedApi = {
  ipcRenderer,
  path,
  fs,
  sharp,
};

export type ExposedApi = typeof exposedApi;
export type VideoModule = typeof videoModule;

window.api = exposedApi;
window.videoModule = videoModule;
