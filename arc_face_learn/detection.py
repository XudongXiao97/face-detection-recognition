from face_recogntion.detection_face import DetectionFace
import argparse
import logging
import os
from conf import *
import cv2

'''
arc_face SDK(V2.0)

#run
python3 detection.py --image_path test.jpg

developer:jeffa
'''


def logging_handle(logging_path):
    logger_name = os.path.splitext(os.path.split(logging_path)[1])[0]
    # 日志定义
    logger_handle = logging.getLogger(logger_name)
    logger_handle.setLevel(level=logging.DEBUG)
    formatter = logging.Formatter('%(asctime)s - %(name)s - %(levelname)s  - %(message)s', datefmt='%Y-%m-%d %H:%M:%S')
    file_handler = logging.FileHandler(logging_path)
    file_handler.setFormatter(formatter)
    logger_handle.addHandler(file_handler)
    return logger_handle


def run(image_path):
    # 更换设备时，第一次使用需要激活
    if FACE_ACTION:
        if os.path.exists(".asf_install.dat") and os.path.exists("freesdk_121232.dat"):
            os.remove(".asf_install.dat")
            os.remove("freesdk_121232.dat")
    logger_handle = logging_handle(LOG_PATH)
    df = DetectionFace(DBFACE_PATH, FACE_ACTION, logger_handle)

    img = cv2.imread(image_path)
    bounding_box_list, predict_name_list, predict_score_list = df.update(img)
    if bounding_box_list:
        for box, name, score in zip(bounding_box_list, predict_name_list, predict_score_list):
            label = "{}:{}".format(name, str(score))
            cv2.rectangle(img, (int(box[0]), int(box[1])), (int(box[2]), int(box[3])), (0, 0, 255), thickness=2)
            labelSize, baseLine = cv2.getTextSize(label, cv2.FONT_HERSHEY_SIMPLEX, 0.5, 1)
            cv2.rectangle(img, (int(box[0]), int(box[1]) - round(1.5 * labelSize[1])),
                          (int(box[0]) + round(1.5 * labelSize[0]), int(box[1]) + baseLine),
                          (255, 255, 255), cv2.FILLED)
            cv2.putText(img, label, (int(box[0]), int(box[1])), cv2.FONT_HERSHEY_SIMPLEX, 0.75, (0, 0, 0), 1)
        cv2.imwrite("show.jpg", img)


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("-i", "--image_path", help="image_path")
    args = parser.parse_args()
    if args.image_path:
        image_path = args.image_path
        run(image_path)
        print('||| Completed')
    else:
        print('||| Without query parameters, exit.')
