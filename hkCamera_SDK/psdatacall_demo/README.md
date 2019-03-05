### C++调用海康人脸相机SDK接口

---


1. 从海康官网下载liunx版本SDK开发包，将lib下的所以库copy到psdatacall_demo下，包括HCNetSDKCom文件夹。make编译

---
2. 修改main.cpp，参考海康开发文档，大体思路为调用两个回调函数。

```
case COMM_SNAP_MATCH_ALARM
case COMM_UPLOAD_FACESNAP_RESULT
```
3. 新增人脸录入功能3. 新增人脸录入功能3. 新增人脸录入功能


