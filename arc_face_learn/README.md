#### arc_face_learn

虹软人脸识别SDK使用Demo

enviroment:  ubuntu18.04,python3.6+

1.Downloads the SDK
> [虹软SDK下载](https://ai.arcsoft.com.cn/product/arcface.html)

>需要注册并下载Liunx下的SDK

2.Modification the conf.py and SDK
> ./face_recognition/arc_face/__init__.py
> 修改APP_ID，SDK_KEY，ARC_INIT_SO，ARC_ENGINE_SO

> First using, Need set FACE_ACTION=True

>input face to dbface

3.Run
> python3 detection.py --image_path test.jpg

4.Attention
>关于离线的问题,第一次需要激活SDK,即将FACE_ACTION=True。
>
>之后只要是物理IP不变就可以离线使用