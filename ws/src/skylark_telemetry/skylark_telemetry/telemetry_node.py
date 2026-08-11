import rclpy
from  rclpy.node import Node
from rclpy.qos import QoSProfile, ReliabilityPolicy, HistoryPolicy, qos_profile_sensor_data

from std_msgs.msg import String
from px4_msgs.msg import VehicleOdometry, VehicleStatus, BatteryStatus, VehicleAttitude, SensorGps
from skylark_interfaces.msg import TrackArray
from std_srvs.srv import Trigger

import asyncio
import math
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
        self.state_lock = threading.Lock()
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
            "gesture": "",
            "heading": 0.0,
            "pitch": 0.0,
            "roll": 0.0,
            "latitude": 0.0,
            "longitude": 0.0,
            "satellites": 0
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
        
        self.vehicle_attitude_subscriber = self.create_subscription(
            VehicleAttitude,
            '/fmu/out/vehicle_attitude',
            self.vehicle_attitude_callback,
            qos
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
            qos_profile_sensor_data
        )
        
        self.gesture_subscriber = self.create_subscription(
            String,
            '/gesture/command',
            self.gesture_callback,
            qos
        )
        
        self.gps_subscriber = self.create_subscription(
            SensorGps,
            '/fmu/out/vehicle_gps_position',
            self.gps_callback,
            qos
        )
        
        threading.Thread(target=self._run_server, daemon=True).start()
        
    def odometry_callback(self, message):
        with self.state_lock:
            self.state['position']['x'] = float(message.position[0])
            self.state['position']['y'] = float(message.position[1])
            self.state['position']['z'] = float(message.position[2])

            self.state['velocity']['x'] = float(message.velocity[0])
            self.state['velocity']['y'] = float(message.velocity[1])
            self.state['velocity']['z'] = float(message.velocity[2])

            self.state['altitude'] = -float(message.position[2])

    def status_callback(self, message):
        with self.state_lock:
            self.state['arming_state'] = int(message.arming_state)
            self.state['nav_state'] = int(message.nav_state)

    def battery_callback(self, message):
        with self.state_lock:
            self.state['battery_voltage'] = float(message.voltage_v)
            self.state['battery_remaining'] = float(message.remaining)
            
    def _quaternion_to_euler(self, x,y,z,w):
        roll = math.atan2(2*(w*x + y*z),1-2*((x**2)+(y**2)))
        pitch = math.asin(2*(w*y - z*x))
        yaw = math.atan2(2*(w*z + x*y), 1-2*((y**2)+(z**2)))
        
        return roll,pitch,yaw
            
    def vehicle_attitude_callback(self, message):
        with self.state_lock:
            w = float(message.q[0])
            x = float(message.q[1])
            y = float(message.q[2])
            z = float(message.q[3])
            roll, pitch, yaw = self._quaternion_to_euler(x,y,z,w)
            self.state['roll'] = roll
            self.state['pitch'] = pitch
            self.state['heading'] = yaw
    
    def gps_callback(self, message):
        with self.state_lock:
            self.state['latitude'] = float(message.lat) * 1e-7
            self.state['longitude'] = float(message.lon) * 1e-7
            self.state['satellites'] = int(message.satellites_used)
            
    def track_callback(self, message):
        tracks = message.tracks
        track_list = []
        if tracks != []:
            for track in tracks:
                track_list.append({'id': int(track.tracking_id), 'class_id': int(track.class_id), 'x1': float(track.x1), 'y1': float(track.y1), 'x2': float(track.x2), 'y2': float(track.y2)})

            with self.state_lock:
                self.state['tracks'] = track_list

    def gesture_callback(self, message):
        with self.state_lock:
            self.state['gesture'] = message.data
        
    async def websocket_endpoint(self, websocket: WebSocket):
        
        await websocket.accept()
        self.ws_clients.add(websocket)
        try:
            while True:
                with self.state_lock:
                    snapshot = json.dumps(self.state)
                await websocket.send_text(snapshot)
                try:
                    data = await asyncio.wait_for(websocket.receive_text(), timeout=0.1)
                    await self._handle_command(json.loads(data))
                except asyncio.TimeoutError:
                    pass
        except WebSocketDisconnect:
            self.ws_clients.discard(websocket)
        except Exception:
            import traceback
            traceback.print_exc()
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