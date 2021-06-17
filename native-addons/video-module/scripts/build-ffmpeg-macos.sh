set -x
git clone --depth 1 --branch n4.3.1 https://github.com/FFmpeg/FFmpeg.git

cd FFmpeg

# Instructions from https://trac.ffmpeg.org/wiki/CompilationGuide/macOS
# See what is really needed by running ./configure --help
./configure  --prefix=`pwd`/../../FFmpeg/macos --enable-shared #--enable-debug=2 --disable-stripping
make -j 8
make install
rm ../../FFmpeg/macos/libs/*.a
