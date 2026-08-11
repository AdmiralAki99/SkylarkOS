import rclpy
from  rclpy.node import Node
from rclpy.qos import QoSProfile, ReliabilityPolicy, HistoryPolicy, qos_profile_sensor_data

from std_msgs.msg import String

import asyncio
import math
import json
import threading

from fastapi import FastAPI, WebSocket, WebSocketDisconnect
import uvicorn

VALID_COMMANDS = {
    'TAKEOFF', 'LAND', 'MOVE_LEFT', 'MOVE_RIGHT',
    'MOVE_FORWARD', 'MOVE_BACKWARD', 'HOVER', 'STOP',
}

class ApiNode(Node):
    def __init__(self):
        super().__init__('api_node')

        self.app = FastAPI()
        self.app.add_api_websocket_route('/ws', self.websocket_endpoint)

        qos = QoSProfile(
            reliability=ReliabilityPolicy.RELIABLE,
            history=HistoryPolicy.KEEP_LAST,
            depth=1
        )

        self.command_publisher = self.create_publisher(
            String,
            '/api/command',
            qos
        )

        threading.Thread(target=self._run_server, daemon=True).start()

    async def websocket_endpoint(self, websocket: WebSocket):
        await websocket.accept()
        try:
            while True:
                data = await websocket.receive_text()
                try:
                    command = json.loads(data)
                except json.JSONDecodeError:
                    continue

                cmd = command.get('cmd')
                if cmd not in VALID_COMMANDS:
                    self.get_logger().warn(f"Rejected unknown command: {cmd!r}")
                    continue

                message = String()
                message.data = cmd
                self.command_publisher.publish(message)
        except WebSocketDisconnect:
            pass

    def _run_server(self):
        uvicorn.run(self.app, host='0.0.0.0', port=8766)

def main(args=None):
    rclpy.init(args=args)
    node = ApiNode()
    rclpy.spin(node)
    rclpy.shutdown()

        