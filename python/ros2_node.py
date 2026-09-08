#!/usr/bin/env python3
"""
ROS2 Terminal Camera Viewer Node (Python / rclpy)
Subscribes to ROS2 sensor_msgs/Image or CompressedImage and renders live video in the terminal.
"""

import sys
import os
import time

# Ensure tcamviewer python module is loadable
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

import rclpy
from rclpy.node import Node
from rclpy.qos import QoSProfile, ReliabilityPolicy, HistoryPolicy
from sensor_msgs.msg import Image, CompressedImage

from tcamviewer import TerminalRenderer, get_terminal_size

try:
    import cv2
    from cv_bridge import CvBridge
    HAS_CV_BRIDGE = True
except ImportError:
    HAS_CV_BRIDGE = False

class TerminalCameraNode(Node):
    def __init__(self):
        super().__init__("tcamviewer_node")

        # Declare parameters
        self.declare_parameter("topic", "/camera/image_raw")
        self.declare_parameter("compressed", False)
        self.declare_parameter("width", 0)
        self.declare_parameter("height", 0)
        self.declare_parameter("use_diff", True)
        self.declare_parameter("alt_screen", True)

        topic = self.get_parameter("topic").get_parameter_value().string_value
        is_compressed = self.get_parameter("compressed").get_parameter_value().bool_value
        width = self.get_parameter("width").get_parameter_value().integer_value
        height = self.get_parameter("height").get_parameter_value().integer_value
        use_diff = self.get_parameter("use_diff").get_parameter_value().bool_value
        alt_screen = self.get_parameter("alt_screen").get_parameter_value().bool_value

        self.get_logger().info(f"Subscribing to topic: '{topic}' (compressed={is_compressed})")

        # Initialize Terminal Renderer
        self.renderer = TerminalRenderer(
            cols=width,
            rows=height,
            use_diff=use_diff,
            alt_screen=alt_screen,
            hide_cursor=True
        )

        qos = QoSProfile(
            reliability=ReliabilityPolicy.BEST_EFFORT,
            history=HistoryPolicy.KEEP_LAST,
            depth=1
        )

        self.bridge = CvBridge() if HAS_CV_BRIDGE else None
        self.frame_count = 0
        self.last_fps_time = time.time()

        if is_compressed:
            self.sub = self.create_subscription(
                CompressedImage, topic, self.on_compressed_image, qos
            )
        else:
            self.sub = self.create_subscription(
                Image, topic, self.on_raw_image, qos
            )

    def on_raw_image(self, msg: Image):
        self.frame_count += 1
        encoding = msg.encoding.lower()

        try:
            if encoding == "rgb8":
                self.renderer.render_rgb(bytes(msg.data), msg.width, msg.height, msg.step)
            elif encoding == "bgr8":
                self.renderer.render_bgr(bytes(msg.data), msg.width, msg.height, msg.step)
            elif self.bridge:
                # Fallback to cv_bridge conversion
                cv_img = self.bridge.imgmsg_to_cv2(msg, desired_encoding="rgb8")
                self.renderer.render_rgb(cv_img, cv_img.shape[1], cv_img.shape[0], cv_img.strides[0])
        except Exception as e:
            self.get_logger().error(f"Render error: {e}")

    def on_compressed_image(self, msg: CompressedImage):
        self.frame_count += 1
        if not HAS_CV_BRIDGE:
            self.get_logger().error_once("OpenCV / cv_bridge is required for compressed image decoding")
            return
        try:
            cv_img = self.bridge.compressed_imgmsg_to_cv2(msg, desired_encoding="rgb8")
            self.renderer.render_rgb(cv_img, cv_img.shape[1], cv_img.shape[0], cv_img.strides[0])
        except Exception as e:
            self.get_logger().error(f"Compressed render error: {e}")

    def destroy_node(self):
        if self.renderer:
            self.renderer.close()
        super().destroy_node()

def main(args=None):
    rclpy.init(args=args)
    node = TerminalCameraNode()
    try:
        rclpy.spin(node)
    except KeyboardInterrupt:
        pass
    finally:
        node.destroy_node()
        rclpy.shutdown()

if __name__ == "__main__":
    main()
