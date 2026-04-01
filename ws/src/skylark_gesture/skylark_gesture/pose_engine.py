import numpy
import onnxruntime as ort
from pathlib import Path
import cv2

class PoseEngine:
    def __init__(self, model_path: str, use_GPU: bool = False):
        self.model_path = Path(model_path)
        self.input_size = (640,640)
        
        if use_GPU:
            providers = ['TensorrtExecutionProvider', 'CUDAExecutionProvider']
        else:
            providers = ['CPUExecutionProvider']
            
        if not self.model_path.exists():
            return
            
        self.session = ort.InferenceSession(str(self.model_path), providers= providers)
        
    def _preprocess(self, image, tracking_bounding_box):
        H, W = image.shape[:2]
        
        x1 = int(tracking_bounding_box[0] * W)
        y1 = int(tracking_bounding_box[1] * H)
        x2 = int(tracking_bounding_box[2] * W)
        y2 = int(tracking_bounding_box[3] * H)
        
        x1 = max(0,x1)
        y1 = max(0,y1)
        x2 = min(W,x2)
        y2 = min(H,y2)
        
        crop = image[y1:y2, x1:x2]
        
        if crop.size == 0:
            return
        
        # Resizing the image into the input the model needs
        resized_crop = cv2.resize(crop, self.input_size)
        
        # Changing it from BGR to RGB
        crop_rgb = cv2.cvtColor(resized_crop, cv2.COLOR_BGR2RGB)
        
        crop_rgb = crop_rgb / 255.0
        
        transposed_crop = crop_rgb.transpose((2,0,1))
        
        batched_image = numpy.expand_dims(transposed_crop, axis=0).astype(numpy.float32)
        
        return batched_image, (crop.shape[1],crop.shape[0])
    
    def _postprocess(self, output, crop_w, crop_h):
        
        # Squeezing the output
        squeezed_output = numpy.squeeze(output, axis=0)
        
        transposed_output = squeezed_output.transpose(1,0) # (640x640, 56)
        
        confidence = transposed_output[:,4]
        
        if numpy.max(confidence) < 0.3:
            return None
        
        best_row = transposed_output[numpy.argmax(confidence)]
        
        best_features = best_row[5:]
        
        best_features = best_features.reshape(17,3)
        
        best_features[:,0] = best_features[:,0] / self.input_size[0] * crop_w
        best_features[:,1] = best_features[:,1] / self.input_size[1] * crop_h
        
        return best_features
    
    def extract_keypoints(self, image, tracking_bounding_box):
        result = self._preprocess(image, tracking_bounding_box)
        
        if result is None:
            return None
        
        tensor, crop_dimensions = result
        
        # Passing it through the model
        output = self.session.run(['output0'], {'images': tensor})
        
        # Passing it through the postprocessing
        
        best_features = self._postprocess(output[0], crop_dimensions[0], crop_dimensions[1])
        
        if best_features is None:
            return None
        
        return best_features
        
        
        
        
        
        
        
        
    
    