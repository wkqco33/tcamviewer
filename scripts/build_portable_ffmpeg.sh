#!/usr/bin/env bash
# 낮은 glibc 베이스라인 배포용 정적 FFmpeg 빌드.
#
# 동적 FFmpeg를 링크하면 배포판마다 libavcodec.so 메이저가 달라(20.04/22.04=58,
# 24.04=60) 한 바이너리로 여러 배포판을 지원할 수 없다. 이 스크립트는 디코딩에
# 필요한 컴포넌트만 담은 LGPL 정적 FFmpeg를 빌드해 libtcamviewer에 넣는다.
#
# 사용법:
#   scripts/build_portable_ffmpeg.sh [PREFIX] [FFMPEG_VERSION]
# 기본값: PREFIX=<repo>/build/ffmpeg-static, VERSION=6.1.1
#
# 결과물: $PREFIX/lib/libav*.a, libsw*.a (PIC), $PREFIX/include/...
set -euo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
PREFIX="${1:-${TCAMVIEWER_FFMPEG_DIR:-$REPO_ROOT/build/ffmpeg-static}}"
VERSION="${2:-${FFMPEG_VERSION:-6.1.1}}"
JOBS="${JOBS:-$(nproc)}"
WORK_DIR="${WORK_DIR:-$PREFIX/_src}"
STAMP="$PREFIX/.ffmpeg-config"

# 로컬 파일 재생에 필요한 디코더/디멀서/프로토콜만 포함한다. --disable-autodetect로
# 시스템 라이브러리가 자동으로 붙는 것을 막는다(정적 링크 결과가 시스템에 의존하지 않도록).
# 오디오 컴포넌트는 출력하지 않지만, mov/matroska 디멀서가 참조하는 심볼을
# 만족시키기 위해 일부 코덱을 포함한다.
build_flags=(
    --prefix="$PREFIX"
    --enable-static --disable-shared --enable-pic
    --disable-programs --disable-doc --disable-debug --disable-autodetect
    --disable-everything
    --enable-avcodec --enable-avformat --enable-avutil
    --enable-swscale --enable-swresample
    --enable-network
    --enable-protocol=file,pipe,data,rtsp,rtp,tcp,udp,http
    --enable-demuxer=mov,matroska,avi,mpegts,mpegps,mpegvideo,flv,asf,rawvideo,image2,image2pipe,gif,ivf,h264,hevc,m4v,av1
    --enable-decoder=h264,hevc,mpeg1video,mpeg2video,mpeg4,mjpeg,h263,vp8,vp9,av1,theora,wmv1,wmv2,wmv3,vc1,prores,dnxhd,ffv1,huffyuv,rawvideo,flv,gif,msmpeg4v3
    --enable-decoder=aac,aac_latm,mp3,ac3,eac3,vorbis,opus,flac,alac,dca,truehd,pcm_s16le,pcm_s16be,pcm_u8,pcm_f32le
    --enable-parser=h264,hevc,mpeg4video,mpegvideo,vp8,vp9,av1,mjpeg,h263,aac,aac_latm,mpegaudio,vorbis,opus,flac,dca,ac3
)

config_key="$VERSION $(printf '%s\n' "${build_flags[@]}" | sha256sum | cut -c1-12)"
if [ -f "$STAMP" ] && [ "$(cat "$STAMP")" = "$config_key" ] && [ -f "$PREFIX/lib/libavcodec.a" ]; then
    echo "정적 FFmpeg $VERSION 이 이미 있습니다: $PREFIX"
    exit 0
fi

mkdir -p "$PREFIX" "$WORK_DIR"
tarball="$WORK_DIR/ffmpeg-$VERSION.tar.xz"
url="https://ffmpeg.org/releases/ffmpeg-$VERSION.tar.xz"

if [ ! -f "$tarball" ]; then
    echo "다운로드: $url"
    curl -fsSL "$url" -o "$tarball"
fi

src="$WORK_DIR/ffmpeg-$VERSION"
if [ ! -d "$src" ]; then
    tar -C "$WORK_DIR" -xf "$tarball"
fi

echo "정적 FFmpeg $VERSION 빌드: $PREFIX"
(
    cd "$src"
    ./configure "${build_flags[@]}"
    make -j "$JOBS"
    make install
)

echo "$config_key" > "$STAMP"
echo "완료: $PREFIX (정적 라이브러리 5종)"
