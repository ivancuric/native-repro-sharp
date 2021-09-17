import path from 'path';
import { useEffect, useRef, useState } from 'react';
import { hot } from 'react-hot-loader';
import sharp from 'sharp';

import { VideoFrameExtractor } from '../VideoFrameExtractor';

const videoPath = path.join(process.cwd(), 'video.mp4');

const App = () => {
  const ref = useRef<HTMLVideoElement>(null);
  const [videoLoaded, setVideoLoaded] = useState(false);

  useEffect(() => {
    if (!ref.current) {
      return;
    }

    ref.current.addEventListener('play', () => {
      setVideoLoaded(true);
    });
  }, []);

  useEffect(() => {
    if (!ref.current || !videoLoaded) {
      return;
    }

    const videoFrameExtractor = new VideoFrameExtractor(ref.current);

    (async () => {
      try {
        const blob = await videoFrameExtractor.captureFrame();
        const arrayBuffer = await blob.arrayBuffer();
        const buffer = Buffer.from(arrayBuffer);
        await sharp(buffer).resize(400).toFile('output.jpeg');
      } catch (error) {
        console.error(error);
      }
    })();
  }, [videoLoaded]);

  return (
    <video
      ref={ref}
      controls
      style={{
        position: 'absolute',
        width: '100%',
        height: '100%',
        objectFit: 'cover',
      }}
      src={'yolo:///' + videoPath}
    ></video>
  );
};

export default hot(module)(App);
