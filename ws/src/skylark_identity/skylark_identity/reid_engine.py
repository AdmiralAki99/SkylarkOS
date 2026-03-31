import numpy
import onnxruntime as ort
from pathlib import Path
import cv2

class ReIDEngine:
    
    def __init__(self, model_path: str, embedding_path: str, use_GPU = False):
        self.owner_embeddings = None
        self.embedding_path = Path(embedding_path)
        self.model_path = Path(model_path)
        
        self.mean = numpy.array([0.485, 0.456, 0.406])
        self.std = numpy.array([0.229, 0.224, 0.225])
        
        if use_GPU:
            providers = ['TensorrtExecutionProvider', 'CUDAExecutionProvider']
        else:
            providers = ['CPUExecutionProvider']
            
        # Checking if the model path is right
        if not self.model_path.exists():
            return
        
        self.session = ort.InferenceSession(str(self.model_path), providers=providers)
        self._load_embedding(self.embedding_path)
        
        
    def _load_embedding(self, embedding_path: Path | str):
        if isinstance(embedding_path, str):
            embedding_path = Path(embedding_path)
            
        if embedding_path.exists():
            self.owner_embeddings = numpy.load(embedding_path) # The embeddings are done in a numpy file
        
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
        resized_crop = cv2.resize(crop, (128,256))
        
        # Changing it from BGR to RGB
        crop_rgb = cv2.cvtColor(resized_crop, cv2.COLOR_BGR2RGB)
        
        crop_rgb = crop_rgb / 255.0
        
        normalized_crop = (crop_rgb - self.mean) / self.std
        
        transposed_crop = normalized_crop.transpose((2,0,1))
        
        batched_embedding = numpy.expand_dims(transposed_crop, axis=0).astype(numpy.float32)
        
        return batched_embedding
                
    def _extract_embedding(self, image, tracking_bounding_box):
        tensor = self._preprocess(image, tracking_bounding_box)
        if tensor is None:
            return None
        
        # Calling the model
        results = self.session.run(['output'], {'input': tensor})
        
        embedding = results[0][0]
        
        normalize_embedding = embedding / numpy.linalg.norm(embedding)
        
        return normalize_embedding
    
    def is_enrolled(self):
        return False if self.owner_embeddings is None else True
    
    def enroll(self, image_bbox_pairs):
        embeddings = []
        for image, bbox in image_bbox_pairs:
            embedding = self._extract_embedding(image, bbox)
            if embedding is not None:
                embeddings.append(embedding)
        if not embeddings:
            return
        
        mean_embedding = numpy.mean(embeddings, axis=0)
        self.owner_embeddings = mean_embedding / numpy.linalg.norm(mean_embedding)
        numpy.save(self.embedding_path, self.owner_embeddings)
    
    def match(self, image, tracking_bounding_box):
        if self.owner_embeddings  is None:
            return -1.0
        
        embedding = self._extract_embedding(image, tracking_bounding_box)
        if embedding is None:
            return -1.0
        
        return float(numpy.dot(embedding, self.owner_embeddings))
        
        
       