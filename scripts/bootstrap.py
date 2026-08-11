import os
import sys
import socket
import subprocess
import requests
import shutil
import time

BASE = os.environ.get('SKYLARK_BASE','/skylark')
LAUNCH_FILE = os.environ.get('SKYLARK_LAUNCH', 'drone.launch.py')

SCRIPTS_DIR = os.path.dirname(os.path.abspath(__file__))

EMBEDDING_PATH = os.path.join(BASE, 'data', 'owner_embedding.npy')
CERT_PATH = os.path.join(BASE, 'data', 'certs', 'cert.pem')
KEY_PATH = os.path.join(BASE, 'data', 'certs', 'key.pem')

ENROLL_SERVER = os.path.join(BASE, 'src', 'skylark_identity', 'skylark_identity', 'enroll_server.py')
GEN_CERT_SCRIPT = os.path.join(SCRIPTS_DIR, 'gen_cert.sh')

LAUNCH_CMD = ['ros2', 'launch', 'skylark_bringup', LAUNCH_FILE]
    
def is_enrolled():
    return os.path.exists(EMBEDDING_PATH)

def get_local_ip():
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    s.connect(('8.8.8.8', 80))
    ip = s.getsockname()[0]
    s.close()
    return ip

def ensure_certs():
    if not os.path.exists(CERT_PATH) or not os.path.exists(KEY_PATH):
        print("[Skylark] Generating SSL certificates...")
        subprocess.run(['bash',GEN_CERT_SCRIPT], check= True)
        print("[Skylark] Certificates Generated...")
    else:
        print("[Skylark] Certificates already present...")

def run_enrollment():
    # Checking for the certificates
    ensure_certs()
    
    env = os.environ.copy()
    env['SKYLARK_BASE'] = BASE
    env['PYTHONPATH'] = os.path.join(BASE, 'install', 'skylark_identity', 'lib', 'python3.10', 'site-packages') + ':' + env.get('PYTHONPATH', '')
    process = subprocess.Popen([sys.executable, ENROLL_SERVER], stderr=subprocess.PIPE, env= env)
    print(f"[Skylark] SecureID not setup. Opening https://{get_local_ip()}:8888 for user to enroll...")
    
    while True:
        # Checking if the server is enrolled
        try:
            time.sleep(2)
            if process.poll() is not None:
                print("[Skylark] ERROR: Enrollment server crashed:")
                print(process.stderr.read().decode())
                sys.exit(1)
                
            resp = requests.get(f"https://{get_local_ip()}:8888/enroll/status", verify=False)
            if resp.json()['enrolled']:
                break
        except requests.exceptions.RequestException:
            pass
        
    process.terminate()
    print(f"Enrollment complete for user, saved and stored embeddings...")
    
def main():
    print("[Skylark] Bootstrap initializing.....")
    if not is_enrolled():
        # Need to start the server and then get the user enrolled
        run_enrollment()
        print("[Skylark] Bootstrap User Enrollment Complete")
    else:
        print("[Skylark] User Enrollment already complete")
        
    # Launching the drone
    print("[Skylark] Starting device...")
    ros2_bin  = shutil.which('ros2')
    os.execv(ros2_bin, LAUNCH_CMD)
    
if __name__ == '__main__':
    main()