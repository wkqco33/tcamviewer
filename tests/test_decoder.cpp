#include <gtest/gtest.h>

#include "../src/decoder_utils.hpp"
#include "tcamviewer/decoder.hpp"

extern "C" {
#include <libavcodec/codec_par.h>
#include <libavcodec/packet.h>
#include <libavutil/display.h>
#include <libavutil/mem.h>
}

#include <cstring>

using namespace tcamviewer;

TEST(DecoderTest, OpenInvalidSource) {
    VideoDecoder decoder("/non/existent/path/to/video.mp4");
    EXPECT_FALSE(decoder.open());
    EXPECT_FALSE(decoder.isOpened());
}

TEST(DecoderTest, ReadFrameUnopened) {
    VideoDecoder decoder("");
    EXPECT_EQ(decoder.readFrame(100, 100), nullptr);
}

#if LIBAVFORMAT_VERSION_MAJOR >= 62
TEST(DecoderTest, ReadsDisplayMatrixFromCodecParametersSideData) {
    AVCodecParameters* codecpar = avcodec_parameters_alloc();
    ASSERT_NE(codecpar, nullptr);

    AVStream stream{};
    stream.codecpar = codecpar;

    int32_t matrix[9];
    av_display_rotation_set(matrix, 90.0);
    auto* matrixData = static_cast<uint8_t*>(av_malloc(sizeof(matrix)));
    ASSERT_NE(matrixData, nullptr);
    std::memcpy(matrixData, matrix, sizeof(matrix));

    const AVPacketSideData* added = av_packet_side_data_add(&codecpar->coded_side_data, &codecpar->nb_coded_side_data,
                                                            AV_PKT_DATA_DISPLAYMATRIX, matrixData, sizeof(matrix), 0);
    if (!added)
        av_free(matrixData);
    ASSERT_NE(added, nullptr);

    size_t size = 0;
    const uint8_t* actual = detail::getDisplayMatrixSideData(&stream, &size);
    ASSERT_NE(actual, nullptr);
    EXPECT_EQ(size, sizeof(matrix));
    EXPECT_EQ(std::memcmp(actual, matrix, sizeof(matrix)), 0);

    avcodec_parameters_free(&codecpar);
}
#endif
