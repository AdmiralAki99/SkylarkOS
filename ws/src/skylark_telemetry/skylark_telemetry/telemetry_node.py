import rclpy
from  rclpy.node import Node
from rclpy.qos import QoSProfile, ReliabilityPolicy, HistoryPolicy, qos_profile_sensor_data

from std_msgs.msg import String
from px4_msgs.msg import VehicleOdometry, VehicleStatus, BatteryStatus
from skylark_interfaces.msg import TrackArray
from std_srvs.srv import Trigger

import asyncio
import json
import threading

from fastapi import FastAPI, WebSocket, WebSocketDisconnect
import uvicorn

class TelemetryNode(Node):
    def __init__(self):
        super().__init__('telemetry_node')
        
        qos = QoSProfile(
            reliability=ReliabilityPolicy.BEST_EFFORT,
            history=HistoryPolicy.KEEP_LAST,
            depth=1
        )
        
        self.ws_clients = set()
        self.land_client = self.create_client(Trigger, '/land')
        self.takeoff_client = self.create_client(Trigger, '/takeoff')
        self.app = FastAPI()
        
        self.app.add_api_websocket_route('/ws', self.websocket_endpoint)
        
        self.state = {
            "position": {"x": 0.0, "y": 0.0, "z": 0.0},
            "velocity": {"x": 0.0, "y": 0.0, "z": 0.0},
            "arming_state": 0,
            "nav_state": 0,
            "battery_voltage": 0.0,
            "battery_remaining": 0.0,
            "altitude": 0.0,
            "tracks": [],
            "gesture": ""
        }
        
        self.get_logger().info('API Node initialized and publisher created for /api/command')
        
        # Create Subscriber for the API
        
        self.vehicle_odometry_subscriber = self.create_subscription(
            VehicleOdometry,
            '/fmu/out/vehicle_odometry',
            self.odometry_callback,
            qos_profile_sensor_data
        )
        
        self.vehicle_status_subscriber = self.create_subscription(
            VehicleStatus,
            '/fmu/out/vehicle_status',
            self.status_callback,
            qos_profile_sensor_data
        )
        
        self.battery_status_subscriber = self.create_subscription(
            BatteryStatus,
            '/fmu/out/battery_status',
            self.battery_callback,
            qos_profile_sensor_data
        )
        
        self.track_array_subscriber = self.create_subscription(
            TrackArray,
            '/tracking/tracks',
            self.track_callback,
            qos
        )
        
        self.gesture_subscriber = self.create_subscription(
            String,
            '/gesture/command',
            self.gesture_callback,
            qos
        )
        
        threading.Thread(target=self._run_server, daemon=True).start()
        
    def odometry_callback(self, message):
        
        self.state['position']['x'] = message.position[0]
        self.state['position']['y'] = message.position[1]
        self.state['position']['z'] = message.position[2]
        
        self.state['velocity']['x'] = message.velocity[0]
        self.state['velocity']['y'] = message.velocity[1]
        self.state['velocity']['z'] = message.velocity[2]
        
        self.state['altitude'] = -message.position[2]
    
    def status_callback(self, message):
        
        self.state['arming_state'] = message.arming_state
        self.state['nav_state'] = message.nav_state
        
    
    def battery_callback(self, message):
        
        self.state['battery_voltage'] = message.voltage_v
        self.state['battery_remaining'] = message.remaining
        
    def track_callback(self, message):
        
        tracks = message.tracks
        track_list = []
        if tracks != []:
            for track in tracks:
                track_list.append({'id':track.tracking_id, 'class_id': track.class_id, 'x1': track.x1, 'y1': track.y1, 'x2': track.x2, 'y2': track.y2})

            self.state['tracks'] = track_list
            
    def gesture_callback(self, message):
        self.state['gesture'] = message.data
        
    async def websocket_endpoint(self, websocket: WebSocket):
        
        await websocket.accept()
        self.ws_clients.add(websocket)
        try:
            while True:
                await websocket.send_text(json.dumps(self.state))
                try:
                    data = await asyncio.wait_for(websocket.receive_text(), timeout=0.1)
                    await self._handle_command(json.loads(data))
                except asyncio.TimeoutError:
                    pass
        except WebSocketDisconnect:
            self.ws_clients.discard(websocket)
            
    async def _handle_command(self, command: dict):
        cmd = command.get('cmd')
        if cmd == 'land':
            self.land_client.call_async(Trigger.Request())
        elif cmd == 'takeoff':
            self.takeoff_client.call_async(Trigger.Request())
            
    def _run_server(self):
        uvicorn.run(self.app, host='0.0.0.0', port=8765)
        
def main(args= None):
    rclpy.init(args=args)
    node = TelemetryNode()
    rclpy.spin(node)
    rclpy.shutdown()