import os
import time
from face_recogntion.arc_face import APP_ID, SDK_KEY
import face_recogntion.arc_face.arc_face_function as arc_face


class DetectionFace(object):
    def __init__(self, compare_emb_path, is_action, logger_handle):
        self.logger_handle = logger_handle
        self.action_arc_face(is_action)
        self.engine_arc_face()
        self.feature_dict_dbface = self.load_face_data(compare_emb_path)

    def action_arc_face(self, is_action):
        '''
        arc face 激活
        :param is_action:判断是否需要激活
        :return:
        '''
        if is_action:
            res = arc_face.asfActivation(APP_ID, SDK_KEY)
            if res == 0 or res == 90114:
                self.logger_handle.info(f"Success arc face action res : {res}")
            else:
                self.logger_handle.error(f"Failed arc face action res : {res}")

    # arc face 功能初始化
    def engine_arc_face(self, detectMode=0xFFFFFFFF, detectFaceOrientPriority=0x5, detectFaceScaleVal=16,
                        detectFaceMaxNum=50,
                        combinedMask=1 | 4 | 8 | 16 | 32):
        # 保证初始化功能成功,目前离线还有问题
        is_connection = True
        while is_connection:
            res = arc_face.asfInitEngine(detectMode, detectFaceOrientPriority, detectFaceScaleVal, detectFaceMaxNum,
                                         combinedMask)
            if res[0] == 0:
                is_connection = False
                self.logger_handle.info(f"Success arc face engine res : {res[0]}")
            else:
                self.logger_handle.error(f"Failed arc face engine res : {res[0]}")
                time.sleep(60)

    # 录入人脸判断，保证一张照片一个人脸
    def detection_face_one(self, img_path):
        img = arc_face.ARCFaceImg()
        img.filePath = os.path.join(img_path)
        im = arc_face.loadImage(img)
        res_detect, faces = arc_face.asfDetectFaces(im, 0x201)
        if res_detect == 0:
            if faces.faceNum == 1:
                return True
            else:
                self.logger_handle.error("Detect face >1")
                return False
        else:
            self.logger_handle.error("Failed detect face")
            return False

    # 加载本地人脸库
    def load_face_data(self, dataset_path):
        # 存放特征库
        feature_dict = {}
        images = os.listdir(dataset_path)
        if images:
            for image in images:
                img = arc_face.ARCFaceImg()
                img.filePath = os.path.join(dataset_path, image)
                im = arc_face.loadImage(img)
                res_detect, faces = arc_face.asfDetectFaces(im, 0x201)
                if res_detect == 0:
                    res_feature = arc_face.asfFaceFeatureExtract(im, 0x201, arc_face.getSingleFaceInfo(faces, 0))
                    if res_feature[0] == 0:
                        person_id = str(img.filePath).split(os.path.sep)[-1].replace(".jpg", "")
                        feature_dict[person_id] = res_feature[1]
                    else:
                        self.logger_handle.error(f"Failed dbface feature face image_path : {image}")
                else:
                    self.logger_handle.error(f"Failed dbface detection face image_path : {image}")
        return feature_dict

    # 提取单张人脸图人脸特征
    def get_face_feature(self, frame):
        feature_dict = {}
        img = arc_face.ARCFaceImg()
        img.filePath = ""
        img.data = frame
        im = arc_face.loadImage(img)
        res_detect, faces = arc_face.asfDetectFaces(im, 0x201)
        if res_detect == 0:
            for i in range(faces.faceNum):
                res_feature = arc_face.asfFaceFeatureExtract(im, 0x201, arc_face.getSingleFaceInfo(faces, i))
                if res_feature[0] == 0:
                    feature_dict[i] = (faces.faceRect[i], res_feature[1])
                else:
                    self.logger_handle.error(f"Failed frame feature face res_feature : {res_feature[0]}")
        return feature_dict

    # 人脸识别,1:1
    def update(self, frame):
        # return
        bounding_box_list = []
        predict_name_list = []
        predict_score_list = []
        # 加载本地人脸特征库
        feature_dic_frame = self.get_face_feature(frame)
        if len(feature_dic_frame) > 0:
            for i in list(feature_dic_frame.keys()):
                # Tip: the point of box will be <0 or > imane.size
                # it should be do something
                # such as if box<0 , box=0 ,if box>iamge.shape[0],box=int(image.shape[0])
                bounding_box_list.append(((
                    feature_dic_frame[i][0].left, feature_dic_frame[i][0].top, feature_dic_frame[i][0].right,
                    feature_dic_frame[i][0].bottom)))
                if len(self.feature_dict_dbface) > 0:
                    score_list = []
                    name_list = []
                    for key in list(self.feature_dict_dbface.keys()):
                        res, score = arc_face.asfFaceFeatureCompare(self.feature_dict_dbface[key],
                                                                    feature_dic_frame[i][1])
                        score = round(score, 4)
                        if res == 0:
                            if score >= 0.0:
                                score_list.append(score)
                                name_list.append(key)
                    score_pred = max(score_list)
                    score_pred_index = score_list.index(score_pred)
                    name_pred = name_list[score_pred_index]
                    predict_name_list.append(name_pred)
                    predict_score_list.append(score_pred)
        else:
            self.logger_handle.error("Failed load local dbface")
        return bounding_box_list, predict_name_list, predict_score_list
