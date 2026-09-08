#include <gtest/gtest.h>
#include "tcamviewer/decoder.hpp"

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
