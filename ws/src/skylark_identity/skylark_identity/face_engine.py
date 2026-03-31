import numpy
from insightface.app  import FaceAnalysis
from pathlib import Path

class FaceEngine:
    
    def __init__(self, model_pack, embedding_path: str, detection_size=(320,320),use_GPU = False):
        self.owner_embedding = None
        self.embedding_path = Path(embedding_path)
        
        if use_GPU:
            providers = ['TensorrtExecutionProvider', 'CUDAExecutionProvider']
        else:
            providers = ['CPUExecutionProvider']
            
        self.app = FaceAnalysis(name= model_pack, providers= providers)
        self.app.prepare(ctx_id=0, det_size=detection_size)
        
        self._load_embedding(self.embedding_path)
        
    def _load_embedding(self, embedding_path: Path | str):
        if isinstance(embedding_path, str):
            embedding_path = Path(embedding_path)
            
        if embedding_path.exists():
            self.owner_embedding = numpy.load(embedding_path) # The embeddings are done in a numpy file
            
    
    def detect_and_match(self, image, tracking_bounding_box):
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
            return -1.0
        
        # There was a face detected
        faces = self.app.get(crop)
        
        if not faces:
            return -1.0
        
        if self.owner_embedding is None:
            return -1.0
        
        # There is a face at this point
        embeddings = faces[0].normed_embedding
        similarity = numpy.dot(embeddings, self.owner_embedding)
        return float(similarity)
            
    def enroll(self, embeddings_list):
        mean_embedding = numpy.mean(embeddings_list, axis= 0)
        normalize_embedding = mean_embedding / numpy.linalg.norm(mean_embedding)
        numpy.save(self.embedding_path,normalize_embedding)
        
        self.owner_embedding = normalize_embedding
    
    def is_enrolled(self):
        return self.owner_embedding is not None