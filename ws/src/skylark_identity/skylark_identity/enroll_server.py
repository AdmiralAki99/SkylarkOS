from fastapi import FastAPI, UploadFile
from fastapi.staticfiles import StaticFiles
from fastapi.responses import FileResponse
import traceback
import cv2
import numpy
import uvicorn
import sys
import os

from skylark_identity.face_engine import FaceEngine

# TODO: Add mechanism to store the certificate to be stored properly

BASE = os.environ.get('SKYLARK_BASE', '/skylark')
BUCKETS_REQUIRED = 14
TOTAL_BUCKETS = 18
EMBEDDING_PATH = os.path.join(BASE, 'data', 'owner_embedding.npy')
CERT = os.path.join(BASE, 'data', 'certs', 'cert.pem')
KEY  = os.path.join(BASE, 'data', 'certs', 'key.pem')

app = FastAPI()
engine = FaceEngine(model_pack='buffalo_l', embedding_path= EMBEDDING_PATH)
embeddings_buffer = []
coverage_buckets = set()
enrollment_complete = False

static_dir = os.path.join(os.path.dirname(__file__), 'static')
app.mount("/static",StaticFiles(directory=static_dir), name="static")

@app.get("/")
async def read_root():
    return FileResponse(path=os.path.join(os.path.dirname(__file__), 'static', 'secureid_enrollment.html'))

@app.post("/enroll/frame")
async def post_enroll_frame(frame: UploadFile):
    try:
        data = await frame.read()
    
        arr = numpy.frombuffer(data, numpy.uint8)
        img = cv2.imdecode(arr, cv2.IMREAD_COLOR)
    
        if img is None:
            return {"status": "invalid_image"}
    
        faces = engine.app.get(img)
        print(f"[Enroll] faces={len(faces)}, buffer={len(embeddings_buffer)}", flush=True)
        if not faces:
            return {"status": 'no_face'}
    
        normed_embedding = faces[0].normed_embedding
        embeddings_buffer.append(normed_embedding)
        yaw, _, _ = faces[0].pose
        bucket = int((yaw + 90) / 10) % TOTAL_BUCKETS
        coverage_buckets.add(bucket)

        if len(coverage_buckets) >= BUCKETS_REQUIRED:
            global enrollment_complete
            engine.enroll(embeddings_list= embeddings_buffer)
            embeddings_buffer.clear()
            coverage_buckets.clear()
            enrollment_complete = True
            return {'status':'enrolled'}

        return {
            "status": "ok",
            "frames_collected": len(embeddings_buffer),
            "coverage": len(coverage_buckets) / TOTAL_BUCKETS
        }

    except Exception as e:
        traceback.print_exc()
        return {"status": "error", "detail": str(e)}
    
@app.get("/enroll/status")
async def get_enroll_status():
    return {
        'enrolled': enrollment_complete,
        'frames_collected': len(embeddings_buffer),
        'frames_required': BUCKETS_REQUIRED,
        'coverage': len(coverage_buckets) / TOTAL_BUCKETS
    }

if __name__ == "__main__":
    import os
    if os.path.exists(CERT) and os.path.exists(KEY):
        uvicorn.run(app, host="0.0.0.0", port=8888, ssl_certfile=CERT, ssl_keyfile=KEY)
    else:
        print("WARNING: No certs found, running HTTP (camera will not work on mobile)")
        # Add script to create certification
        uvicorn.run(app, host="0.0.0.0", port=8888)