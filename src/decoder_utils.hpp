#pragma once

#include <cstddef>
#include <cstdint>

extern "C" {
#include <libavcodec/packet.h>
#include <libavformat/avformat.h>
}

namespace tcamviewer::detail {

inline const uint8_t* getDisplayMatrixSideData(const AVStream* stream, size_t* size) {
    if (size)
        *size = 0;
    if (!stream || !stream->codecpar)
        return nullptr;

#if LIBAVFORMAT_VERSION_MAJOR >= 62
    const AVPacketSideData* sideData = av_packet_side_data_get(
        stream->codecpar->coded_side_data, stream->codecpar->nb_coded_side_data, AV_PKT_DATA_DISPLAYMATRIX);
    if (!sideData)
        return nullptr;
    if (size)
        *size = sideData->size;
    return sideData->data;
#else
    return av_stream_get_side_data(stream, AV_PKT_DATA_DISPLAYMATRIX, size);
#endif
}

}  // namespace tcamviewer::detail
