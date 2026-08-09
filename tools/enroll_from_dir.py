"""
Dev tool: batch enroll from a directory of face images.
Usage: python3 enroll_from_dir.py <image_dir> <output_embedding_path>
Example: python3 enroll_from_dir.py ~/faces/ /mnt/d/dev/SkylarkOS/data/owner_embedding.npy
"""

import sys
import cv2
from pathlib import Path

sys.path.insert(0, '/mnt/d/dev/SkylarkOS/ws/src/skylark_identity')
from skylark_identity.face_engine import FaceEngine

SUPPORTED_EXTENSIONS = {'.jpg', '.jpeg', '.png', '.bmp'}

def main():
    if len(sys.argv) != 3:
        print("Usage: python3 enroll_from_dir.py <image_dir> <output_embedding_path>")
        sys.exit(1)

    image_dir = Path(sys.argv[1])
    embedding_path = sys.argv[2]

    if not image_dir.is_dir():
        print(f"ERROR: {image_dir} is not a directory")
        sys.exit(1)

    engine = FaceEngine(model_pack='buffalo_sc', embedding_path=embedding_path)

    image_files = [f for f in image_dir.iterdir() if f.suffix.lower() in SUPPORTED_EXTENSIONS]
    if not image_files:
        print(f"ERROR: No images found in {image_dir}")
        sys.exit(1)

    print(f"Found {len(image_files)} images")

    embeddings = []
    skipped = 0

    for img_path in sorted(image_files):
        img = cv2.imread(str(img_path))
        if img is None:
            print(f"  SKIP {img_path.name} — could not load")
            skipped += 1
            continue

        faces = engine.app.get(img)
        if not faces:
            print(f"  SKIP {img_path.name} — no face detected")
            skipped += 1
            continue

        embeddings.append(faces[0].normed_embedding)
        print(f"  OK   {img_path.name} — face detected (bbox={faces[0].bbox.astype(int).tolist()})")

    if not embeddings:
        print("\nERROR: No faces found in any image — enrollment failed")
        sys.exit(1)

    engine.enroll(embeddings)
    print(f"\nEnrolled from {len(embeddings)} images ({skipped} skipped)")
    print(f"Embedding saved to: {embedding_path}")

if __name__ == '__main__':
    main()
