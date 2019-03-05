1.将lib下所有的库拷贝到psdatacall_demo下，包括HCNetSDKCom文件夹
2.Device.ini中填入相关的设备信息
3.在终端输入make命令即可生成getpsdata，使用./getpsdata即可执行
  


代码移植：
将HK_SDK下psdatacall_demo 全部移植，并将main.cpp上相应目录改为当前目录，并使用 g++ -o getpsdata.so -shared -fPIC main.cpp 生成相应C++动态链接库，接着修改face_match_snap.py

