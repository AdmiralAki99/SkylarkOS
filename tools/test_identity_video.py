"""
Dev tool: run face matching against a video to verify enrollment.
Crops the full frame as bbox and reports similarity scores per frame.
Usage: python3 test_identity_video.py <video_path> <embedding_path>
Example: python3 test_identity_video.py ~/charles.mp4 /mnt/d/dev/SkylarkOS/data/owner_embedding.npy
"""

import sys
import cv2
from pathlib import Path

sys.path.insert(0, '/mnt/d/dev/SkylarkOS/ws/src/skylark_identity')
from skylark_identity.face_engine import FaceEngine

MATCH_THRESHOLD = 0.45
SAMPLE_EVERY_N = 5  # check every Nth frame to keep it fast

def main():
    if len(sys.argv) != 3:
        print("Usage: python3 test_identity_video.py <video_path> <embedding_path>")
        sys.exit(1)

    video_path = sys.argv[1]
    embedding_path = sys.argv[2]

    if not Path(embedding_path).exists():
        print(f"ERROR: No embedding found at {embedding_path} — enroll first")
        sys.exit(1)

    engine = FaceEngine(model_pack='buffalo_sc', embedding_path=embedding_path)
    print(f"Enrolled: {engine.is_enrolled()}")

    cap = cv2.VideoCapture(video_path)
    if not cap.isOpened():
        print(f"ERROR: Could not open video: {video_path}")
        sys.exit(1)

    total_frames = int(cap.get(cv2.CAP_PROP_FRAME_COUNT))
    fps = cap.get(cv2.CAP_PROP_FPS)
    print(f"Video: {total_frames} frames @ {fps:.1f} fps")
    print(f"Sampling every {SAMPLE_EVERY_N} frames — threshold: {MATCH_THRESHOLD}\n")

    frame_idx = 0
    matches = 0
    no_face = 0
    checked = 0

    while True:
        ret, frame = cap.read()
        if not ret:
            break

        frame_idx += 1
        if frame_idx % SAMPLE_EVERY_N != 0:
            continue

        checked += 1
        bbox = (0.0, 0.0, 1.0, 1.0)
        score = engine.detect_and_match(frame, bbox)

        if score == -1.0:
            no_face += 1
            status = "NO FACE"
        elif score >= MATCH_THRESHOLD:
            matches += 1
            status = f"MATCH   score={score:.3f}"
        else:
            status = f"no match score={score:.3f}"

        timestamp = frame_idx / fps
        print(f"  frame {frame_idx:4d} ({timestamp:5.1f}s)  {status}")

    cap.release()

    print(f"\n--- Summary ---")
    print(f"Frames checked : {checked}")
    print(f"Matches        : {matches}  ({100*matches/checked:.1f}%)")
    print(f"No face        : {no_face}  ({100*no_face/checked:.1f}%)")
    print(f"No match       : {checked - matches - no_face}  ({100*(checked-matches-no_face)/checked:.1f}%)")

if __name__ == '__main__':
    main()
