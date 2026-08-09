"""
Dev tool: compare face similarity between two images, two directories, or a directory against a single image.

Usage:
  python3 compare_faces.py <source> <target>

Where source/target can be:
  - A single image file  (e.g. face1.jpg)
  - A directory of images (e.g. ~/faces/)

Examples:
  python3 compare_faces.py face1.jpg face2.jpg
  python3 compare_faces.py ~/enrollment_imgs/ ~/test_imgs/
  python3 compare_faces.py ~/enrollment_imgs/ query.jpg
"""

import sys
import cv2
import numpy
from pathlib import Path
from itertools import product

sys.path.insert(0, '/mnt/d/dev/SkylarkOS/ws/src/skylark_identity')
from skylark_identity.face_engine import FaceEngine

SUPPORTED_EXTENSIONS = {'.jpg', '.jpeg', '.png', '.bmp'}


def load_embeddings(engine, path: Path):
    """Returns list of (filename, embedding) tuples from a file or directory."""
    results = []

    if path.is_file():
        img = cv2.imread(str(path))
        if img is None:
            print(f"  ERROR: could not load {path.name}")
            return results
        faces = engine.app.get(img)
        if not faces:
            print(f"  NO FACE: {path.name}")
            return results
        results.append((path.name, faces[0].normed_embedding))
        print(f"  OK: {path.name}")

    elif path.is_dir():
        files = sorted([f for f in path.iterdir() if f.suffix.lower() in SUPPORTED_EXTENSIONS])
        if not files:
            print(f"  ERROR: no images found in {path}")
            return results
        for f in files:
            img = cv2.imread(str(f))
            if img is None:
                print(f"  SKIP: {f.name} — could not load")
                continue
            faces = engine.app.get(img)
            if not faces:
                print(f"  NO FACE: {f.name}")
                continue
            results.append((f.name, faces[0].normed_embedding))
            print(f"  OK: {f.name}")
    else:
        print(f"ERROR: {path} is not a file or directory")

    return results


def main():
    if len(sys.argv) != 3:
        print("Usage: python3 compare_faces.py <source> <target>")
        sys.exit(1)

    source_path = Path(sys.argv[1])
    target_path = Path(sys.argv[2])

    engine = FaceEngine(model_pack='buffalo_sc', embedding_path='/tmp/unused.npy')

    print(f"\nLoading source: {source_path}")
    source_embeddings = load_embeddings(engine, source_path)

    print(f"\nLoading target: {target_path}")
    target_embeddings = load_embeddings(engine, target_path)

    if not source_embeddings:
        print("\nERROR: No faces found in source")
        sys.exit(1)
    if not target_embeddings:
        print("\nERROR: No faces found in target")
        sys.exit(1)

    print(f"\n--- Similarity Results ---")
    print(f"{'Source':<30} {'Target':<30} {'Score':>7}  {'Match'}")
    print("-" * 75)

    scores = []
    for (src_name, src_emb), (tgt_name, tgt_emb) in product(source_embeddings, target_embeddings):
        score = float(numpy.dot(src_emb, tgt_emb))
        match = "MATCH" if score >= 0.45 else ""
        print(f"{src_name:<30} {tgt_name:<30} {score:>7.3f}  {match}")
        scores.append(score)

    print(f"\n--- Summary ---")
    print(f"Pairs compared : {len(scores)}")
    print(f"Mean score     : {numpy.mean(scores):.3f}")
    print(f"Max score      : {numpy.max(scores):.3f}")
    print(f"Min score      : {numpy.min(scores):.3f}")
    print(f"Matches (≥0.45): {sum(1 for s in scores if s >= 0.45)} / {len(scores)}")


if __name__ == '__main__':
    main()
