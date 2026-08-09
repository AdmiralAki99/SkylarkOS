from insightface.app import FaceAnalysis
import cv2

app = FaceAnalysis(name='buffalo_sc', providers=['CPUExecutionProvider'])
app.prepare(ctx_id=0, det_size=(320, 320))

# Use any image with a face in it
img = cv2.imread('../test_face.jpg')
faces = app.get(img)

print(f"Number of faces: {len(faces)}")
if faces:
    face = faces[0]
    print(f"Attributes: {dir(face)}")
    print(f"bbox: {face.bbox}")
    print(f"det_score: {face.det_score}")
    print(f"embedding shape: {face.embedding.shape}")
    print(f"embedding dtype: {face.embedding.dtype}")
    print(f"embedding sample: {face.embedding[:5]}")