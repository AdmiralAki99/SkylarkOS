from fastapi import FastAPI, UploadFile
from fastapi.staticfiles import StaticFiles
from fastapi.responses import FileResponse
import cv2
import numpy
import uvicorn
import sys
import os

from skylark_identity.face_engine import FaceEngine

# TODO: Add mechanism to store the certificate to be stored properly

FRAMES_REQUIRED = 30
EMBEDDING_PATH = '/mnt/d/dev/SkylarkOS/data/owner_embedding.npy'

CERT = '/mnt/d/dev/SkylarkOS/data/certs/cert.pem'
KEY  = '/mnt/d/dev/SkylarkOS/data/certs/key.pem'

app = FastAPI()
engine = FaceEngine(model_pack='buffalo_sc', embedding_path= EMBEDDING_PATH)
embeddings_buffer = []

static_dir = os.path.join(os.path.dirname(__file__), 'static')
app.mount("/static",StaticFiles(directory=static_dir), name="static")

@app.get("/")
async def read_root():
    return FileResponse(path=os.path.join(os.path.dirname(__file__), 'static', 'index.html'))

@app.post("/enroll/frame")
async def post_enroll_frame(image: UploadFile):
    data = await image.read()
    
    arr = numpy.frombuffer(data, numpy.uint8)
    img = cv2.imdecode(arr, cv2.IMREAD_COLOR)
    
    if img is None:
        return {"status": "invalid_image"}
    
    faces = engine.app.get(img)
    if not faces:
        return {"status": 'no_face'}
    
    normed_embedding = faces[0].normed_embedding
    embeddings_buffer.append(normed_embedding)
    
    # Checking if enough images are given
    if len(embeddings_buffer) >= FRAMES_REQUIRED:
        engine.enroll(embeddings_list= embeddings_buffer)
        embeddings_buffer.clear()
        return {'status':'enrolled'}
    
    return {"status": "ok", "frames_collected": len(embeddings_buffer)}
    
@app.get("/enroll/status")
async def get_enroll_status():
    return {
        'enrolled': engine.is_enrolled(),
        'frames_collected': len(embeddings_buffer),
        'frames_required': FRAMES_REQUIRED
    }

if __name__ == "__main__":
    import os
    if os.path.exists(CERT) and os.path.exists(KEY):
        uvicorn.run(app, host="0.0.0.0", port=8888, ssl_certfile=CERT, ssl_keyfile=KEY)
    else:
        print("WARNING: No certs found, running HTTP (camera will not work on mobile)")
        # Add script to create certification
        uvicorn.run(app, host="0.0.0.0", port=8888)