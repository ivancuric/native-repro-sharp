#!/bin/bash
CODEC=libavcodec.58.dylib
DEVICE=libavdevice.58.dylib
FORMAT=libavformat.58.dylib
UTIL=libavutil.56.dylib
SWRESAMPLE=libswresample.3.dylib
SWSCALE=libswscale.5.dylib
FILTER=libavfilter.7.dylib

cp $1/$CODEC $2/
cp $1/$DEVICE $2/
cp $1/$FORMAT $2/
cp $1/$UTIL $2/
cp $1/$SWRESAMPLE $2/
cp $1/$SWSCALE $2/
cp $1/$FILTER $2/

cd $2
FILES=`find . -type f`
for f in $FILES
do
  REPLACED=`otool -L $f|grep FFmpeg`
  for path in $REPLACED
  do
    LIBNAME=`echo $path|cut -d' ' -f1|grep -o '[^/]*$'`
    install_name_tool -change $path "@rpath/$LIBNAME" $f
  done
  install_name_tool -add_rpath $2 $f
  MYNAME=`echo $f|cut -d' ' -f1|grep -o '[^/]*$'`
  install_name_tool -id "@rpath" $f
done
