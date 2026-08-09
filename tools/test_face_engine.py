import cv2
import sys
sys.path.insert(0, '/mnt/d/dev/SkylarkOS/ws/src/skylark_identity')

from skylark_identity.face_engine import FaceEngine

IMAGE_PATH = '../test_face.jpg'
EMBEDDING_PATH = '/mnt/d/dev/SkylarkOS/data/owner_embedding.npy'

engine = FaceEngine(
    model_pack='buffalo_sc',
    embedding_path=EMBEDDING_PATH
)

print(f"Enrolled before: {engine.is_enrolled()}")

img = cv2.imread(IMAGE_PATH)
if img is None:
    print(f"ERROR: Could not load image at {IMAGE_PATH}")
    sys.exit(1)

print(f"Image shape: {img.shape}")

# Detect faces in the full image
faces = engine.app.get(img)
print(f"Faces detected: {len(faces)}")

if not faces:
    print("No faces found in image — try a different image with a clear face")
    sys.exit(1)

for i, face in enumerate(faces):
    print(f"  Face {i}: bbox={face.bbox.astype(int).tolist()}, embedding shape={face.normed_embedding.shape}")

# Enroll using all detected face embeddings
embeddings = [face.normed_embedding for face in faces]
engine.enroll(embeddings)
print(f"\nEnrolled after: {engine.is_enrolled()}")

# Match against the same image (should be close to 1.0)
bbox_normalized = (0.0, 0.0, 1.0, 1.0)
score = engine.detect_and_match(img, bbox_normalized)
print(f"Self-match similarity score: {score:.4f}  (expected: close to 1.0)")
