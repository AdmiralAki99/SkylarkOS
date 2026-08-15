import rclpy
from  rclpy.node import Node
from rclpy.qos import QoSProfile, ReliabilityPolicy, HistoryPolicy, qos_profile_sensor_data

from std_msgs.msg import String, Bool

import asyncio
import math
import json
import threading

from fastapi import FastAPI, WebSocket, WebSocketDisconnect
import uvicorn

VALID_COMMANDS = {
    'TAKEOFF', 'LAND', 'MOVE_LEFT', 'MOVE_RIGHT',
    'MOVE_FORWARD', 'MOVE_BACKWARD', 'HOVER', 'STOP',
    'heartbeat'
}

class ApiNode(Node):
    def __init__(self):
        super().__init__('api_node')

        self.app = FastAPI()
        self.app.add_api_websocket_route('/ws', self.websocket_endpoint)
        
        self.last_hearbeat_time = self.get_clock().now()
        self._link_ok = False
        self._link_time_out_threshold_ns = (3*(10**9))

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
        
        self.link_status_publisher = self.create_publisher(
            Bool,
            '/system/link_status',
            qos
        )
        
        self.create_timer(1.0, self._check_link_health)

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

                self.last_hearbeat_time = self.get_clock().now()

                if cmd == 'heartbeat':
                    continue

                message = String()
                message.data = cmd
                self.command_publisher.publish(message)
        except WebSocketDisconnect:
            pass

    def _check_link_health(self):
        duration = self.get_clock().now() - self.last_hearbeat_time
        link_ok = duration.nanoseconds < self._link_time_out_threshold_ns

        if link_ok != self._link_ok:
            self._link_ok = link_ok
            self.link_status_publisher.publish(Bool(data=link_ok))

    def _run_server(self):
        uvicorn.run(self.app, host='0.0.0.0', port=8766)

def main(args=None):
    rclpy.init(args=args)
    node = ApiNode()
    rclpy.spin(node)
    rclpy.shutdown()

        