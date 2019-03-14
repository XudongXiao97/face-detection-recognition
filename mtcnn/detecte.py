import tensorflow as tf
import argparse
from align.detect_face import detect_face, create_mtcnn
import numpy as np
import multiprocessing as mp
import cv2
import time
import os
from config import *


class FaceDetection:
    def __init__(self, gpu_memory_fraction):
        self.minsize = 20  # minimum size of face
        self.threshold = [0.8, 0.8, 0.8]  # three steps's threshold
        self.factor = 0.709  # scale factor
        print('Creating networks and loading parameters')
        with tf.Graph().as_default():
            gpu_options = tf.GPUOptions(per_process_gpu_memory_fraction=gpu_memory_fraction)
            sess = tf.Session(config=tf.ConfigProto(gpu_options=gpu_options, log_device_placement=False))
            with sess.as_default():
                self.pnet, self.rnet, self.onet = create_mtcnn(sess, None)

    def detect_face(self, image):
        bounding_boxes, points = detect_face(image, self.minsize, self.pnet, self.rnet, self.onet,
                                             self.threshold, self.factor)
        return bounding_boxes, points


def produce(queue_img):
    cap = cv2.VideoCapture(VIDEO_PATH)
    tmp = 0
    while True:
        is_opened, frame = cap.read()
        if is_opened:
            if tmp % 25 == 0:
                queue_img.put(frame)
        else:
            queue_img.put(None)
        tmp += 1
        if frame is None:
            print("[INFO the video is over.]")
            break


def customer(queue_img):
    print("[INFO] loading mtcnn model")
    fd = FaceDetection(GPU_MEMORY_LIMIT)
    is_detect = True
    while is_detect:
        time.sleep(1)
        if queue_img.qsize() > 0:
            img_name_tmp = time.strftime("%Y%m%d%H%M%S", time.localtime(time.time())) + ".jpg"
            img = queue_img.get()
            if isinstance(img, np.ndarray):
                rgb = cv2.cvtColor(img, cv2.COLOR_BGR2RGB)
                bounding_box, points = fd.detect_face(rgb.copy())
                bounding_box_list = bounding_box[:, 0:4].astype(int)
                print(bounding_box.shape[0])
                if bounding_box.shape[0] > 0:
                    for box in bounding_box_list:
                        cv2.rectangle(img, (box[0], box[1]), (box[2], box[3]), (0, 255, 0), 3)
                    cv2.imwrite(os.path.join(OUTPUT_FACE_DICT["face"], img_name_tmp), img)
                else:
                    cv2.imwrite(os.path.join(OUTPUT_FACE_DICT["noface"], img_name_tmp), img)

        else:
            is_detect = False
            print("[INFO] detecte has process over.")


def run():
    queue_img = [mp.Queue()]
    process_detection = [
        mp.Process(target=produce, args=(queue_img)),
        mp.Process(target=customer, args=(queue_img))
    ]
    [process.start() for process in process_detection]
    [process.join() for process in process_detection]


if __name__ == "__main__":
    run()
