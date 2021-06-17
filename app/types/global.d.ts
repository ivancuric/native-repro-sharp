import { ExposedApi, VideoModule } from '~/renderer/preload';

declare global {
  declare const MAIN_WINDOW_PRELOAD_WEBPACK_ENTRY: string;
  declare const MAIN_WINDOW_WEBPACK_ENTRY: string;

  interface Window {
    api: ExposedApi;
    videoModule: VideoModule;
  }

  const api = window.api;
  const videoModule = window.videoModule;
}
