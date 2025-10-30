from ultralytics import YOLO
import cv2

class HumanDetector:
    def __init__(self, model_path, conf_threshold=0.25):
        self.model = YOLO(model_path)
        self.conf_threshold = conf_threshold
    
    def detect(self, frame):
        results = self.model(frame, conf=self.conf_threshold)
        return results
