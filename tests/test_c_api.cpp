#include <gtest/gtest.h>
#include "tcamviewer/tcamviewer.h"
#include <vector>
#include <cstring>

TEST(CApiTest, TerminalSize) {
    int cols = 0, rows = 0;
    tcam_status_t status = tcam_get_terminal_size(&cols, &rows);
    EXPECT_EQ(status, TCAM_OK);
    EXPECT_GT(cols, 0);
    EXPECT_GT(rows, 0);
}

TEST(CApiTest, RendererLifecycleAndRender) {
    tcam_render_config_t config;
    config.target_cols = 4;
    config.target_rows = 2;
    config.use_diff = false;
    config.alt_screen = false;
    config.hide_cursor = false;

    tcam_renderer_t* renderer = tcam_renderer_create(&config);
    ASSERT_NE(renderer, nullptr);

    EXPECT_EQ(tcam_renderer_get_cols(renderer), 4);
    EXPECT_EQ(tcam_renderer_get_rows(renderer), 2);

    // 4 cols x 2 rows = 4x4 image (48 bytes)
    std::vector<uint8_t> rgb(4 * 4 * 3, 200);

    char buffer[4096];
    size_t out_len = 0;
    tcam_status_t status = tcam_renderer_render_to_buffer(
        renderer, rgb.data(), 4, 4, 12, false, buffer, sizeof(buffer), &out_len
    );

    EXPECT_EQ(status, TCAM_OK);
    EXPECT_GT(out_len, 0);
    EXPECT_NE(strstr(buffer, "\xE2\x96\x80"), nullptr);

    tcam_renderer_resize(renderer, 8, 4);
    EXPECT_EQ(tcam_renderer_get_cols(renderer), 8);
    EXPECT_EQ(tcam_renderer_get_rows(renderer), 4);

    tcam_renderer_destroy(renderer);
}

TEST(CApiTest, NullPointerHandling) {
    EXPECT_EQ(tcam_get_terminal_size(nullptr, nullptr), TCAM_ERR_INVALID_ARG);
    EXPECT_EQ(tcam_renderer_resize(nullptr, 10, 10), TCAM_ERR_INVALID_ARG);
    EXPECT_EQ(tcam_renderer_render_rgb24(nullptr, nullptr, 0, 0, 0), TCAM_ERR_INVALID_ARG);
    tcam_renderer_destroy(nullptr); // should be safe no-op
}
