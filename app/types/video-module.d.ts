// types are declared outside of the module for easier development

declare module 'video-module' {
  export interface Frame {
    width: (number) => number;
    height: (number) => number;
    // Buffers are valid only as long as Frame is alive. In addition there
    // seems to be bug in V8 (?) that sometimes causes crashing if this is
    // called multiple times, so beware!
    getTextures: () => [Buffer];
    frameNumber: () => number;
    planes: () => number;
  }

  export interface VideoMode {
    width: number;
    height: number;
    fps: number;
  }

  export type FrameHandler = (frame: Frame) => void;

  export type PixelFormat = 'uyvu422' | 'yuvj422p';

  export type LogLevel = 0 | 1 | 2 | 3 | 4;

  export interface Stats {
    decode: number;
    passing: number;
    nodeDelay: number;
    render: number;
    total: number;
    frameNumber: number;
  }

  /** Events that can be handled on a `Stream` */
  export type EventType =
    | 'fatal-error'
    | 'started'
    | 'start-failed'
    | 'stopped'
    | 'stop-requested'
    | 'started-recording'
    | 'stopped-recording'
    | 'snapshot-taken'
    | 'failed-start-recording';

  export interface Event {
    timestamp: number;
    type: EventType;
  }

  export type StreamEventHandler = (event: Event) => void;

  export interface Stream {
    name: () => string;
    addFrameCallback: (T: FrameHandler) => void;
    clearFrameCallbacks: () => void;
    start: (mode: VideoMode) => void;
    stop: () => void;
    videoModes: () => [VideoMode];
    /** Can only be returned on a running stream */
    format: () => PixelFormat;
    isActive: () => boolean;
    latestFrameStats: () => ?Stats;
    setEventListener: (T: StreamEventHandler) => void;
    removeEventListener: () => void;
  }

  export interface RecordingOptions {
    /** Path to output file, including extension (`[path-to-file].mp4`) */
    outputPath: string;
  }

  export interface SnapshotOptions extends VideoMode {
    /** Path to output file, including extension (`[path-to-file].[png/jpeg]` */
    outputPath: string;
  }

  export interface RecordableStream extends Stream {
    isRecording: () => boolean;
    startRecording: (options: RecordingOptions) => void;
    stopRecording: () => void;
    // TODO: shouldn't be possible to take snapshot while running
    takeSnapshot: (options: SnapshotOptions) => void; // <- Only for non running stream
  }

  export class PerfLogger {
    constructor(name: string, writeToFile: bool);
    log: (key: LogLevel, T: number) => void;
    flush: () => void;
  }

  /** Returns the list of video streams (devices) available to FFmpeg */
  export function listStreams(): RecordableStream[];

  export function logPerf(
    name: string,
    key: LogLevel,
    n: number,
    toFile: boolean,
  ): void;
}
