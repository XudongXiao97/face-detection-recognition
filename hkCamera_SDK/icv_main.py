#!/usr/bin/python3
# -*- coding:utf-8 -*-
import multiprocessing as mp
import numpy as np
import os
import time
import shutil
import json
import logging
import cv2
import subprocess

logger_main = logging.getLogger(__name__)
logger_main.setLevel(level=logging.DEBUG)
formatter = logging.Formatter('%(asctime)s - %(name)s - %(levelname)s - %(message)s', datefmt='%Y-%m-%d %H:%M:%S')
file_handler = logging.FileHandler(LOGGING_PATH_DICT["main"])
file_handler.setFormatter(formatter)
logger_main.addHandler(file_handler)


def customer_face(queue_face, camera_name, camera_pwd, camera_ip):
    previous_working_directory = os.getcwd()
    # python 启动子进程模块subprocess接受 c++ print(信息)
    # 不是一个好的思路，不过这边总框架是python，要做到信息的调度，目前是这样子做的
    out = subprocess.Popen(
        "python3 {} {} {} {} {}".format("face_match_snap.py", camera_ip, "./psdatacall_demo", camera_name,
                                        camera_pwd),
        stdout=subprocess.PIPE,
        stdin=subprocess.PIPE,
        shell=True)
    os.chdir(previous_working_directory)
    while True:
        localtime = time.strftime('%Y-%m-%d', time.localtime(time.time()))
        face_snap_dir = os.path.join(ICV_IMG_PATH, camera_ip, localtime, IMG_NAME_DICT["face_snap_name"])
        face_back_dir = os.path.join(ICV_IMG_PATH, camera_ip, localtime, IMG_NAME_DICT["face_back_name"])
        std_out = out.stdout.readline().decode('utf-8')
        if std_out.find('CPP match face: ') == 0:
            std_out = std_out[len("CPP match face: "):-1]
            camera_ip, time_stamp, face_id, face_name, probability = std_out.split('_')
            probability = probability.split('.jpg')[0]

            back_img_name = "%s_%s.jpg" % (camera_ip, time_stamp)
            snap_img_name = "%s_%s_%s.jpg" % (face_id, camera_ip, time_stamp)
            all_save_path = os.path.join(face_back_dir, back_img_name)
            face_save_path = os.path.join(face_snap_dir, snap_img_name)
            time.sleep(3)
            if os.path.exists(all_save_path) and os.path.exists(face_save_path):
                logger_main.info(
                    f"[INFO] camera_ip:{camera_ip} face_id:{face_id} face_name:{face_name} probability:{probability}")


def run():
    ## ICV system using Process
    process_icv = []
    ## Get camera info for mysql
    # face_camera_ip_list, face_camera_label_list = dbf.query_face_camera_ip()
    face_camera_ip_list = ["192.168.1.165"]
    if face_camera_ip_list:
        queue_face_list = [mp.Queue(maxsize=255) for _ in face_camera_ip_list]

        for queue_face, camera_ip in zip(queue_face_list, face_camera_ip_list):
            process_icv.append(
                mp.Process(target=customer_face,
                           args=(queue_face, CAMERA_NAME, CAMERA_PWD, camera_ip)))

    [process.start() for process in process_icv]
    [process.join() for process in process_icv]


if __name__ == '__main__':
    run()
