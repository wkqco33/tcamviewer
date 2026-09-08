#include <rclcpp/rclcpp.hpp>
#include <sensor_msgs/msg/image.hpp>
#include <sensor_msgs/msg/compressed_image.hpp>
#include <cv_bridge/cv_bridge.hpp>
#include <opencv2/opencv.hpp>

#include "tcamviewer/terminal.hpp"
#include "tcamviewer/renderer.hpp"

#include <memory>
#include <string>

class TerminalViewerNode : public rclcpp::Node {
public:
    TerminalViewerNode() : Node("tcamviewer_node") {
        this->declare_parameter<std::string>("topic", "/camera/image_raw");
        this->declare_parameter<bool>("compressed", false);
        this->declare_parameter<int>("width", 0);
        this->declare_parameter<int>("height", 0);
        this->declare_parameter<bool>("use_diff", true);
        this->declare_parameter<bool>("alt_screen", true);

        std::string topic = this->get_parameter("topic").as_string();
        bool compressed = this->get_parameter("compressed").as_bool();
        int width = this->get_parameter("width").as_int();
        int height = this->get_parameter("height").as_int();
        bool use_diff = this->get_parameter("use_diff").as_bool();
        bool alt_screen = this->get_parameter("alt_screen").as_bool();

        tcamviewer::RenderConfig cfg;
        cfg.targetCols = width;
        cfg.targetRows = height;
        cfg.useDiff = use_diff;
        cfg.altScreen = alt_screen;
        cfg.hideCursor = true;

        renderer_ = std::make_unique<tcamviewer::Renderer>(cfg);

        RCLCPP_INFO(this->get_logger(), "Listening on topic: '%s' (compressed: %s)",
                    topic.c_str(), compressed ? "true" : "false");

        rclcpp::QoS qos(rclcpp::KeepLast(1));
        qos.best_effort();

        if (compressed) {
            compressed_sub_ = this->create_subscription<sensor_msgs::msg::CompressedImage>(
                topic, qos,
                std::bind(&TerminalViewerNode::onCompressedImage, this, std::placeholders::_1)
            );
        } else {
            raw_sub_ = this->create_subscription<sensor_msgs::msg::Image>(
                topic, qos,
                std::bind(&TerminalViewerNode::onRawImage, this, std::placeholders::_1)
            );
        }
    }

private:
    void onRawImage(const sensor_msgs::msg::Image::SharedPtr msg) {
        if (!renderer_) return;

        const std::string& enc = msg->encoding;
        if (enc == "rgb8") {
            renderer_->renderRgb24(msg->data.data(), msg->width, msg->height, msg->step);
        } else if (enc == "bgr8") {
            renderer_->renderBgr24(msg->data.data(), msg->width, msg->height, msg->step);
        } else {
            try {
                auto cv_ptr = cv_bridge::toCvCopy(msg, sensor_msgs::image_encodings::RGB8);
                renderer_->renderRgb24(cv_ptr->image.data, cv_ptr->image.cols, cv_ptr->image.rows, cv_ptr->image.step);
            } catch (const cv_bridge::Exception& e) {
                RCLCPP_ERROR_THROTTLE(this->get_logger(), *this->get_clock(), 5000,
                                     "cv_bridge conversion error: %s", e.what());
            }
        }
    }

    void onCompressedImage(const sensor_msgs::msg::CompressedImage::SharedPtr msg) {
        if (!renderer_) return;
        try {
            cv::Mat cv_img = cv::imdecode(cv::Mat(msg->data), cv::IMREAD_COLOR);
            if (!cv_img.empty()) {
                renderer_->renderBgr24(cv_img.data, cv_img.cols, cv_img.rows, cv_img.step);
            }
        } catch (const std::exception& e) {
            RCLCPP_ERROR_THROTTLE(this->get_logger(), *this->get_clock(), 5000,
                                 "Compressed decode error: %s", e.what());
        }
    }

    std::unique_ptr<tcamviewer::Renderer> renderer_;
    rclcpp::Subscription<sensor_msgs::msg::Image>::SharedPtr raw_sub_;
    rclcpp::Subscription<sensor_msgs::msg::CompressedImage>::SharedPtr compressed_sub_;
};

int main(int argc, char** argv) {
    rclcpp::init(argc, argv);
    auto node = std::make_shared<TerminalViewerNode>();
    rclcpp::spin(node);
    rclcpp::shutdown();
    return 0;
}
