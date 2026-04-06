import sys
import os
import numpy as np
import pytest

sys.path.insert(0, os.path.join(os.path.dirname(__file__), '../ws/src/skylark_gesture'))
from skylark_gesture.gesture_engine import GestureEngine


# baseline neutral pose - person standing, arms at sides
# COCO order: nose, eyes, ears, shoulders, elbows, wrists, hips, knees, ankles
# [x, y, confidence] - y increases downward
def make_keypoints(overrides=None):
    kps = np.array([
        [320, 100, 0.9],  # 0 nose
        [310,  90, 0.9],  # 1 left eye
        [330,  90, 0.9],  # 2 right eye
        [300,  95, 0.9],  # 3 left ear
        [340,  95, 0.9],  # 4 right ear
        [290, 200, 0.9],  # 5 left shoulder
        [350, 200, 0.9],  # 6 right shoulder
        [280, 280, 0.9],  # 7 left elbow
        [360, 280, 0.9],  # 8 right elbow
        [275, 360, 0.9],  # 9 left wrist
        [365, 360, 0.9],  # 10 right wrist
        [300, 380, 0.9],  # 11 left hip
        [340, 380, 0.9],  # 12 right hip
        [300, 460, 0.9],  # 13 left knee
        [340, 460, 0.9],  # 14 right knee
        [300, 540, 0.9],  # 15 left ankle
        [340, 540, 0.9],  # 16 right ankle
    ], dtype=np.float32)
    if overrides:
        for idx, val in overrides.items():
            kps[idx] = np.array(val, dtype=np.float32)
    return kps


def right_arm_raised():
    # wrist y < shoulder y means wrist is above shoulder in image space
    return make_keypoints({10: [365, 80, 0.9]})

def left_arm_raised():
    return make_keypoints({9: [275, 80, 0.9]})

def both_arms_raised():
    return make_keypoints({9: [275, 80, 0.9], 10: [365, 80, 0.9]})

def t_pose():
    # arms horizontal - wrist y ~= shoulder y, arms extend outward
    return make_keypoints({
        5:  [200, 200, 0.9],  # left shoulder
        6:  [440, 200, 0.9],  # right shoulder
        7:  [130, 200, 0.9],  # left elbow
        8:  [510, 200, 0.9],  # right elbow
        9:  [ 60, 200, 0.9],  # left wrist
        10: [580, 200, 0.9],  # right wrist
    })


class TestVisibility:
    engine = GestureEngine(keypoint_confidence_threshold=0.5)

    def test_visible_above_threshold(self):
        assert self.engine._is_visible([100, 200, 0.9]) is True

    def test_visible_at_threshold(self):
        assert self.engine._is_visible([100, 200, 0.5]) is True

    def test_not_visible_below_threshold(self):
        assert self.engine._is_visible([100, 200, 0.3]) is False

    def test_not_visible_zero_confidence(self):
        assert self.engine._is_visible([100, 200, 0.0]) is False

    def test_custom_threshold(self):
        strict = GestureEngine(keypoint_confidence_threshold=0.8)
        assert strict._is_visible([100, 200, 0.7]) is False
        assert strict._is_visible([100, 200, 0.9]) is True


class TestRightArmRaised:
    engine = GestureEngine(keypoint_confidence_threshold=0.3)

    def test_detects_right_arm_raised(self):
        assert self.engine._right_arm_raised(right_arm_raised()) is True

    def test_neutral_pose_not_raised(self):
        assert not self.engine._right_arm_raised(make_keypoints())

    def test_low_confidence_wrist_returns_false(self):
        kps = right_arm_raised()
        kps[10][2] = 0.1
        assert not self.engine._right_arm_raised(kps)

    def test_low_confidence_shoulder_returns_false(self):
        kps = right_arm_raised()
        kps[6][2] = 0.1
        assert not self.engine._right_arm_raised(kps)


class TestLeftArmRaised:
    engine = GestureEngine(keypoint_confidence_threshold=0.3)

    def test_detects_left_arm_raised(self):
        assert self.engine._left_arm_raised(left_arm_raised()) is True

    def test_neutral_pose_not_raised(self):
        assert not self.engine._left_arm_raised(make_keypoints())

    def test_low_confidence_wrist_returns_false(self):
        kps = left_arm_raised()
        kps[9][2] = 0.1
        assert not self.engine._left_arm_raised(kps)


class TestBothArmsRaised:
    engine = GestureEngine(keypoint_confidence_threshold=0.3)

    def test_detects_both_arms_raised(self):
        assert self.engine._both_arms_raised(both_arms_raised()) is True

    def test_only_right_raised_returns_false(self):
        assert not self.engine._both_arms_raised(right_arm_raised())

    def test_only_left_raised_returns_false(self):
        assert not self.engine._both_arms_raised(left_arm_raised())

    def test_neutral_returns_false(self):
        assert not self.engine._both_arms_raised(make_keypoints())


class TestTPose:
    engine = GestureEngine(keypoint_confidence_threshold=0.3)

    def test_detects_t_pose(self):
        assert self.engine._t_pose(t_pose()) is True

    def test_neutral_not_t_pose(self):
        assert not self.engine._t_pose(make_keypoints())

    def test_arms_raised_not_t_pose(self):
        assert not self.engine._t_pose(both_arms_raised())

    def test_wrist_too_high_not_t_pose(self):
        # wrist more than 40px above shoulder = not horizontal
        kps = t_pose()
        kps[10][1] = kps[6][1] - 60
        assert not self.engine._t_pose(kps)

    def test_arm_not_extended_outward_not_t_pose(self):
        # right wrist tucked in behind elbow = folded arm
        kps = t_pose()
        kps[10][0] = kps[8][0] - 10
        assert not self.engine._t_pose(kps)

    def test_low_confidence_keypoint_returns_false(self):
        kps = t_pose()
        kps[9][2] = 0.1
        assert not self.engine._t_pose(kps)


class TestDetect:
    engine = GestureEngine(keypoint_confidence_threshold=0.3)

    def test_none_input_returns_none(self):
        assert self.engine.detect(None) is None

    def test_neutral_pose_returns_none(self):
        assert self.engine.detect(make_keypoints()) is None

    def test_both_arms_raised_returns_stop(self):
        assert self.engine.detect(both_arms_raised()) == 'STOP'

    def test_t_pose_returns_hover(self):
        assert self.engine.detect(t_pose()) == 'HOVER'

    def test_right_arm_raised_returns_follow(self):
        assert self.engine.detect(right_arm_raised()) == 'FOLLOW'

    def test_left_arm_raised_returns_land(self):
        assert self.engine.detect(left_arm_raised()) == 'LAND'

    def test_both_arms_takes_priority_over_single(self):
        assert self.engine.detect(both_arms_raised()) == 'STOP'

    def test_all_low_confidence_returns_none(self):
        kps = make_keypoints()
        kps[:, 2] = 0.1
        assert self.engine.detect(kps) is None


class TestAngle:
    engine = GestureEngine()

    def test_right_angle(self):
        a = np.array([1.0, 0.0])
        b = np.array([0.0, 0.0])
        c = np.array([0.0, 1.0])
        assert abs(self.engine._angle(a, b, c) - 90.0) < 1e-5

    def test_straight_line(self):
        a = np.array([-1.0, 0.0])
        b = np.array([0.0, 0.0])
        c = np.array([1.0, 0.0])
        assert abs(self.engine._angle(a, b, c) - 180.0) < 1e-5

    def test_zero_angle(self):
        a = np.array([1.0, 0.0])
        b = np.array([0.0, 0.0])
        c = np.array([1.0, 0.0])
        assert abs(self.engine._angle(a, b, c) - 0.0) < 1e-5
