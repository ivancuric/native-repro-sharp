/**
 * Extracts frames from a `<video />` element
 */
export class VideoFrameExtractor {
  declare canvas: OffscreenCanvas;
  declare ctx: OffscreenCanvasRenderingContext2D;
  declare video: HTMLVideoElement;

  constructor(video: HTMLVideoElement) {
    this.canvas = new OffscreenCanvas(video.width, video.height);
    this.video = video;

    const ctx = this.canvas.getContext('2d');
    this.canvas.width = video.videoWidth;
    this.canvas.height = video.videoHeight;

    if (!ctx) {
      return;
    }

    this.ctx = ctx;
  }

  captureFrame = (): Promise<Blob> => {
    return new Promise((resolve) => {
      requestAnimationFrame(async () => {
        this.ctx.drawImage(
          this.video,
          0,
          0,
          this.canvas.width,
          this.canvas.height,
        );
        const blob = await this.canvas.convertToBlob();

        resolve(blob);
      });
    });
  };
}
