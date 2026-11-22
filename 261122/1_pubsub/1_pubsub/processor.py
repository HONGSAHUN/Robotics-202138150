import rclpy
from rclpy.node import Node
from std_msgs.msg import String

class Processor(Node):
    def __init__(self):
        super().__init__('processor') # 여기에 노드 이름 들어감
        self.x = 0
        self.y = 0
        self.subscription_ = self.create_subscription(String, '/demo_topic', self.callback, 10)
        self.subscription_
        self.publisher_ = self.create_publisher(String, '/position_topic', 10)

    def callback(self, msg):

        key_input = msg.data.lower()

        if key_input == 'w':
            self.y += 1
        elif key_input == 's':
            self.y -= 1
        elif key_input == 'a':
            self.x -= 1
        elif key_input == 'd':
            self.x += 1

        result = String()
        result.data = f'({self.x}, {self.y})'
        self.publisher_.publish(result)


def main():
    rclpy.init()
    node = Processor()
    rclpy.spin(node)
    node.destroy_node()
    rclpy.shutdown()

if __name__ == '__main__':
    main()