import rclpy
from rclpy.node import Node
from std_msgs.msg import String
import time

class Publisher(Node):
    def __init__(self):
        super().__init__('publisher') # 여기에 노드 이름 들어감
        self.publisher_ = self.create_publisher(String, '/demo_topic', 10)
        self.start_time = time.time()
        self.timer = self.create_timer(1.0, self.publish_message)

    def publish_message(self):

        key_input = input("키 입력 : ")
        
        msg = String()
        msg.data = key_input
        self.publisher_.publish(msg)
        self.get_logger().info(f'Publishing: "{msg.data}"')

def main():
    rclpy.init()
    node = Publisher()
    rclpy.spin(node)
    node.destroy_node()
    rclpy.shutdown()

if __name__ == '__main__':
    main()
