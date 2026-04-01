import numpy

class GestureEngine:
    
    def __init__(self, keypoint_confidence_threshold: float = 0.5):
        self.min_confidence = keypoint_confidence_threshold
        
    def _is_visible(self, keypoint):
        if keypoint[2] >= self.min_confidence:
            return True
        
        return False
    
    def _angle(self, point_a, point_b , point_c):
        ba = point_a - point_b
        bc = point_c - point_b
        
        cos_angle = numpy.dot(ba,bc) / (numpy.linalg.norm(ba) * numpy.linalg.norm(bc))
        cos_angle = numpy.clip(cos_angle, -1.0, 1.0)
        angle = numpy.arccos(cos_angle)
        
        return numpy.degrees(angle)
    
    def _right_arm_raised(self, keypoints):
        if not self._is_visible(keypoint= keypoints[10]) or not self._is_visible(keypoint= keypoints[6]):
            return False
        
        # Now checking if the coodinate handles the relationship
        if keypoints[6][1] > keypoints[10][1]:
            return True
        
    def _left_arm_raised(self, keypoints):
        if not self._is_visible(keypoint= keypoints[9]) or not self._is_visible(keypoint= keypoints[5]):
            return False
        
        # Now checking if the coodinate handles the relationship
        if keypoints[9][1] < keypoints[5][1]:
            return True
        
    def _both_arms_raised(self, keypoints):
        if not self._is_visible(keypoint= keypoints[9]) or not self._is_visible(keypoint= keypoints[5]) or not self._is_visible(keypoint= keypoints[10]) or not self._is_visible(keypoint= keypoints[6]):
            return False
        
        if self._left_arm_raised(keypoints) and self._right_arm_raised(keypoints):
            return True
        
        return False
        
    def _t_pose(self, keypoints):
        
        if not self._is_visible(keypoints[5]) or not self._is_visible(keypoints[6]) or not self._is_visible(keypoints[7]) or not self._is_visible(keypoints[8]) or not self._is_visible(keypoints[9]) or not self._is_visible(keypoints[10]):
            return False
        
        left_shoulder = keypoints[5]
        right_shoulder = keypoints[6]
        left_wrist = keypoints[9]
        right_wrist = keypoints[10]
        left_elbow = keypoints[7]
        right_elbow = keypoints[8]
        
        # Check if the arms are horizontal
        if abs(left_wrist[1] - left_shoulder[1]) < 40:
            if abs(right_wrist[1] - right_shoulder[1]) < 40:
                # The arms are outward
                if right_wrist[0] > right_elbow[0] > right_shoulder[0]:
                    if left_wrist[0] < left_elbow[0] < left_shoulder[0]:
                        return True
                    
        return False
    
    def detect(self, keypoints):
        if keypoints is None:
            return None
        
        if self._both_arms_raised(keypoints):
            return 'STOP'
        elif self._t_pose(keypoints):
            return 'HOVER'
        elif self._right_arm_raised(keypoints):
            return 'FOLLOW'
        elif self._left_arm_raised(keypoints):
            return 'LAND'
        else:
            return None
        
        