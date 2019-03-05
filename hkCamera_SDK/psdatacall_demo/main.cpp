/*
* Copyright(C) 2011,Hikvision Digital Technology Co., Ltd 
* 
* File   name��main.cpp
* Discription��demo for muti thread get stream
* Version    ��1.0
* Author     ��luoyuhua
* Create Date��2011-12-10
* Modification History��
*/

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include "HCNetSDK.h"
#include "iniFile.h"
#include <stdlib.h>
#include <iostream>
#include <iconv.h>
#include <memory.h>
#include <time.h>
#include<typeinfo>
#include<fstream>
#include<string>
#include<iostream>
using namespace std;
#ifdef _WIN32
#elif defined(__linux__) || defined(__APPLE__)
#include <unistd.h>
#endif
//时间解释宏定义
#define GET_YEAR(_time_) (((_time_) >> 26) + 2000)
#define GET_MONTH(_time_) (((_time_) >> 22) & 15)
#define GET_DAY(_time_) (((_time_) >> 17) & 31)
#define GET_HOUR(_time_) (((_time_) >> 12) & 31)
#define GET_MINUTE(_time_) (((_time_) >> 6) & 63)
#define GET_SECOND(_time_) (((_time_) >> 0) & 63)
#define HPR_OK 0
#define HPR_ERROR -1

// 代码转换操作类
class CodeConverter
{
  private:
	iconv_t cd;

  public:
	// 构造
	CodeConverter(const char *from_charset, const char *to_charset)
	{
		cd = iconv_open(to_charset, from_charset);
	}
	// 析构
	~CodeConverter()
	{
		iconv_close(cd);
	}
	// 转换输出
	int convert(char *inbuf, int inlen, char *outbuf, int outlen)
	{
		char **pin = &inbuf;
		char **pout = &outbuf;
		memset(outbuf, 0, outlen);
		return iconv(cd, pin, (size_t *)&inlen, pout, (size_t *)&outlen);
	}
};

typedef struct tagREAL_PLAY_INFO
{
	char szIP[16];
	int iUserID;
	int iChannel;
} REAL_PLAY_INFO, *LPREAL_PLAY_INFO;
FILE *g_pFile = NULL;
//导入人脸数据条件
void g_RealDataCallBack_V30(LONG lRealHandle, DWORD dwDataType, BYTE *pBuffer, DWORD dwBufSize, void *dwUser)
{
	//LPREAL_PLAY_INFO pPlayInfo = (LPREAL_PLAY_INFO)dwUser;
	//printf("[g_RealDataCallBack_V30]Get data, ip=%s, channel=%d, handle=%d, data size is %d, thread=%d\n",
	//pPlayInfo->szIP, pPlayInfo->iChannel, lRealHandle, dwBufSize, pthread_self());

	//printf("[g_RealDataCallBack_V30]Get data, handle=%d, data size is %d, thread=%d\n",
	//lRealHandle, dwBufSize, pthread_self());
	//NET_DVR_SaveRealData(lRealHandle, cFilename);
}

void CALLBACK MessageCallback(LONG lCommand, NET_DVR_ALARMER *pAlarmer, char *pAlarmInfo, DWORD dwBufLen, void *pUser)
{

	int i;
	NET_DVR_ALARMINFO_V30 struAlarmInfo;
	memcpy(&struAlarmInfo, pAlarmInfo, sizeof(NET_DVR_ALARMINFO_V30));
	// static char cIP[256];
	static char chTime_back[1024];

	switch (lCommand)
	{
		
		case COMM_ALARM_V30:
		{
			switch (struAlarmInfo.dwAlarmType)
			{
				case 3:						 //�ƶ���ⱨ��
					for (i = 0; i < 16; i++) //#define MAX_CHANNUM   16  //���ͨ����
					{
						if (struAlarmInfo.byChannel[i] == 1)
						{
							printf("Motion detection %d\n", i + 1);
						}
					}
					break;
				default:
					break;
			}
		}
		break;


		case COMM_SNAP_MATCH_ALARM:
		{

			NET_VCA_FACESNAP_MATCH_ALARM struFaceMatchAlarm = {0};
			memcpy(&struFaceMatchAlarm, pAlarmInfo, sizeof(NET_VCA_FACESNAP_MATCH_ALARM));

			if (struFaceMatchAlarm.fSimilarity < 0.01)
			{
				cout << "||| Ignore: "
					<< struFaceMatchAlarm.fSimilarity
					<< endl;
				break;
			}
			// Match the face
			NET_DVR_TIME struAbsTime = {0};
			struAbsTime.dwYear = GET_YEAR(struFaceMatchAlarm.struSnapInfo.dwAbsTime);
			struAbsTime.dwMonth = GET_MONTH(struFaceMatchAlarm.struSnapInfo.dwAbsTime);
			struAbsTime.dwDay = GET_DAY(struFaceMatchAlarm.struSnapInfo.dwAbsTime);
			struAbsTime.dwHour = GET_HOUR(struFaceMatchAlarm.struSnapInfo.dwAbsTime);
			struAbsTime.dwMinute = GET_MINUTE(struFaceMatchAlarm.struSnapInfo.dwAbsTime);
			struAbsTime.dwSecond = GET_SECOND(struFaceMatchAlarm.struSnapInfo.dwAbsTime);

			char strName[32];
			memcpy(strName, struFaceMatchAlarm.struBlackListInfo.struBlackListInfo.struAttribute.byName,
			   sizeof(struFaceMatchAlarm.struBlackListInfo.struBlackListInfo.struAttribute.byName));
			strName[sizeof(struFaceMatchAlarm.struBlackListInfo.struBlackListInfo.struAttribute.byName)] = '\0';

			char NameOut[255];
			CodeConverter cc = CodeConverter("gb2312", "utf-8");
			cc.convert(strName, strlen(strName), NameOut, 255);

			char strID[32]; 
			memcpy(strID, struFaceMatchAlarm.struBlackListInfo.struBlackListInfo.struAttribute.byCertificateNumber,
				sizeof(struFaceMatchAlarm.struBlackListInfo.struBlackListInfo.struAttribute.byCertificateNumber));
				
			if (struFaceMatchAlarm.fSimilarity < 0.7)
			{
				sprintf(strID, "00000000");
				sprintf(NameOut, "未知人员");
			}
			

			if (struFaceMatchAlarm.struSnapInfo.dwSnapFacePicLen > 0 && struFaceMatchAlarm.struSnapInfo.pBuffer1 != NULL && chTime_back[0]!='\0')
			{

				char cFilename[256] = {0};
				char chFilePWD[256] = {0};
				char cnowday[256];
				char cSimilariry[128]={0};
				char cIP[256];
				// char chTime_back[1024];

				HANDLE hFile;
				DWORD dwReturn;
				// sprintf(chTime_back, "%4.4d%2.2d%2.2d%2.2d%2.2d%2.2d", struAbsTime.dwYear, struAbsTime.dwMonth, struAbsTime.dwDay, struAbsTime.dwHour, struAbsTime.dwMinute, struAbsTime.dwSecond);
				sprintf(cnowday, "%4.4d-%2.2d-%2.2d", struAbsTime.dwYear, struAbsTime.dwMonth, struAbsTime.dwDay);
				sprintf(cSimilariry, "%1.3f", struFaceMatchAlarm.fSimilarity);
				sprintf(cFilename, "%s_%s_%s_%s_%s.jpg",cIP,chTime_back,strID, NameOut, cSimilariry);
				sprintf(cIP, "%s", struFaceMatchAlarm.struDevInfo.struDevIP.sIpV4);
				sprintf(chFilePWD, "/usr/local/icv/img/%s/%s/face_snap/%s",cIP,cnowday,cFilename);
				FILE *pFile;
				pFile = fopen(chFilePWD, "wb");

				fwrite(
					struFaceMatchAlarm.struSnapInfo.pBuffer1,
					sizeof(char),
					struFaceMatchAlarm.struSnapInfo.dwSnapFacePicLen,
					pFile
				);
				fclose(pFile);
				cout << "CPP match face: "<< cFilename<< endl;
			}
			
		}break;


		// 2018.08.07 by Jeff
		case COMM_UPLOAD_FACESNAP_RESULT:
		{

			NET_VCA_FACESNAP_RESULT result={0};
			NET_VCA_FACESNAP_MATCH_ALARM struFaceMatchAlarm = {0};
			memcpy(&result, pAlarmInfo, sizeof(NET_VCA_FACESNAP_RESULT));
			memcpy(&struFaceMatchAlarm, pAlarmInfo, sizeof(NET_VCA_FACESNAP_MATCH_ALARM));
			// Match the face
			NET_DVR_TIME struAbsTime = {0};
			struAbsTime.dwYear = GET_YEAR(result.dwAbsTime);
			struAbsTime.dwMonth = GET_MONTH(result.dwAbsTime);
			struAbsTime.dwDay = GET_DAY(result.dwAbsTime);
			struAbsTime.dwHour = GET_HOUR(result.dwAbsTime);
			struAbsTime.dwMinute = GET_MINUTE(result.dwAbsTime);
			struAbsTime.dwSecond = GET_SECOND(result.dwAbsTime);
			char cIP[256];
			char cFilenameb[256] = {0};
			char chFilePWDb_back[256] = {0};
			char chFilePWD[256] = {0};
			char cnowdayb[128];

			sprintf(cIP, "%s", result.struDevInfo.struDevIP.sIpV4);
			if (chTime_back[0]!='\0')
			{
				
				sprintf(cnowdayb, "%4.4d-%2.2d-%2.2d", struAbsTime.dwYear, struAbsTime.dwMonth, struAbsTime.dwDay);
				sprintf(chTime_back, "%4.4d%2.2d%2.2d%2.2d%2.2d%2.2d", struAbsTime.dwYear, struAbsTime.dwMonth, struAbsTime.dwDay, struAbsTime.dwHour, struAbsTime.dwMinute, struAbsTime.dwSecond);
				sprintf(cFilenameb, "%s_%s.jpg",cIP,chTime_back);
				sprintf(chFilePWDb_back, "/usr/local/icv/img/%s/%s/face_back/%s",cIP,cnowdayb,cFilenameb);

				FILE *fp1;
				fp1 = fopen(chFilePWDb_back, "wb");
				fwrite(
					result.pBuffer2,
					sizeof(char),
					result.dwBackgroundPicLen,
					fp1
				);
				fclose(fp1);

			}
			
		}break;
		default:
			break;

	}
}




// Fortify ����
int Demo_AlarmFortify(char *DeviceIP, int port, char *userName, char *password)
{
	LONG lUserID;
	NET_DVR_DEVICEINFO_V30 struDeviceInfo;
	//printf("从python传进来的参数,IP:%s,端口:%d,用户名：%s,密码：%s\n", DeviceIP, port, userName, password);
	NET_DVR_Init();
	lUserID = NET_DVR_Login_V30(DeviceIP, port, userName, password, &struDeviceInfo);
	if (lUserID < 0)
	{
		printf("Login error, %d\n", NET_DVR_GetLastError());
		NET_DVR_Cleanup();
		return HPR_ERROR;
	}

	//cout << "设置回调函数！！！" << endl;
	NET_DVR_SetDVRMessageCallBack_V30(MessageCallback,NULL);
	// cout << "回调设置完成！！！" << endl;
	LONG lHandle;
	lHandle = NET_DVR_SetupAlarmChan_V30(lUserID);
	if (lHandle < 0)
	{
		printf("NET_DVR_SetupAlarmChan_V30 error, %d\n", NET_DVR_GetLastError());
		NET_DVR_Logout(lUserID);
		NET_DVR_Cleanup();
		return HPR_ERROR;
	}
}

void PsDataCallBack(LONG lRealHandle, DWORD dwDataType, BYTE *pPacketBuffer, DWORD nPacketSize, void *pUser)
{
	if (dwDataType == NET_DVR_SYSHEAD)
	{
		//д��ͷ����
		g_pFile = fopen("/usr/local/icv/HK-face/HK_SDK/psdatacall_demo/record/ps.dat", "wb");

		if (g_pFile == NULL)
		{
			printf("CreateFileHead fail\n");
			return;
		}

		//д��ͷ����
		fwrite(pPacketBuffer, sizeof(unsigned char), nPacketSize, g_pFile);

		printf("write head len=%d\n", nPacketSize);
	}
	else
	{
		if (g_pFile != NULL)
		{

			fwrite(pPacketBuffer, sizeof(unsigned char), nPacketSize, g_pFile);
			printf("write data len=%d\n", nPacketSize);
		}
	}
}

extern "C" {
	//load face jeff
	int UploadFile(char *DeviceIP, int port, char *userName, char *password, char *picPath)
	{
		NET_DVR_Init();
		NET_DVR_LOCAL_SDK_PATH struLocalPath;
		strcpy(struLocalPath.sPath, "/usr/local/icv/HK-face/HK_SDK/psdatacall_demo");
		bool set_path = NET_DVR_SetSDKInitCfg(NET_SDK_INIT_CFG_SDK_PATH, &struLocalPath);

		NET_DVR_DEVICEINFO_V30 struDeviceInfo;

		printf("传进来的参数,IP:%s,端口:%d,用户名：%s,密码：%s,图片路径：%s\n", DeviceIP, port, userName, password, picPath);
		int iUserID = NET_DVR_Login_V30(DeviceIP, port, userName, password, &struDeviceInfo);
		if (iUserID >= 0)
		{
			NET_DVR_FACELIB_COND FaceInBuffer = {0};
			FaceInBuffer.dwSize = sizeof(FaceInBuffer);
			
			strcpy(FaceInBuffer.szFDID, "1");
			FaceInBuffer.byCover = 1;
			FaceInBuffer.byConcurrent = 0;
			int m_lUploadHandle = NET_DVR_UploadFile_V40(iUserID, IMPORT_DATA_TO_FACELIB, &FaceInBuffer, FaceInBuffer.dwSize, NULL, NULL, 0);
			printf("上传文件：%d\n", m_lUploadHandle);
	
			BYTE *pSendAppendData;
			BYTE *pSendPicData;
			//printf("BYTE*定义完成!!\n");
			NET_DVR_SEND_PARAM_IN m_struSendParam;
			
			printf("数据类型定义完成！\n");
			memset(&m_struSendParam, 0, sizeof(m_struSendParam));
			printf("内存清零操作完成！！\n");
			char szLan[1280] = {0};
			char szFileName[255]; //注，MFC综合实例中给出的长度是MAX_PATH,未找到定义
			DWORD dwFileSize = 0;
			//读取XML文件
			printf("开始读取xml文件！！\n");
			strcpy(szFileName, "/usr/local/icv/HK-face/HK_SDK/psdatacall_demo/tmp.xml");
			FILE *fp;
			if ((fp = fopen(szFileName, "r")) == NULL)
			{
				printf("文件打开不成功！\n");
			}
			fseek(fp, 0L, SEEK_END);
			dwFileSize = ftell(fp);
			printf("XML文件大小：%d\n", dwFileSize);
			if (dwFileSize == 0)
			{
				printf("XML文件为空！！\n");
			}
			fseek(fp, 0L, SEEK_SET);
			pSendAppendData = new BYTE[dwFileSize];
			fread(pSendAppendData, sizeof(BYTE), dwFileSize, fp);
			m_struSendParam.pSendAppendData = pSendAppendData;
			m_struSendParam.dwSendAppendDataLen = dwFileSize;
			fclose(fp);
			printf("xml文件读取完成，开始读取图片文件！！\n");
			//读取图片文件
			strcpy(szFileName, picPath);
			FILE *picFP;
			if ((picFP = fopen(szFileName, "r")) == NULL)
			{
				printf("图片打开失败！！\n");
			}
			fseek(picFP, 0L, SEEK_END);
			dwFileSize = ftell(picFP);
			fseek(picFP, 0L, SEEK_SET);
			printf("图片文件大小：%d\n", dwFileSize);
			if (dwFileSize == 0)
			{
				printf("图片文件为空！！\n");
			}
			pSendPicData = new BYTE[dwFileSize];
			fread(pSendPicData, sizeof(BYTE), dwFileSize, picFP);
			m_struSendParam.pSendData = pSendPicData;
			m_struSendParam.dwSendDataLen = dwFileSize;
			m_struSendParam.byPicType = 1;
			m_struSendParam.byPicURL = 0;
			fclose(picFP);

			printf("开始上传图片数据！！！\n");

			if (NET_DVR_UploadSend(m_lUploadHandle, &m_struSendParam, NULL) < 0)
			{
				printf("\n上传失败,错误代号：%d\n", NET_DVR_GetLastError());
				return 0;
			}
			else
			{
				uint pProgress = 0;
				
				while (pProgress < 100)
				{
					int iStatus = NET_DVR_GetUploadState(m_lUploadHandle, &pProgress);
					if (iStatus == -1)
					{
						printf("上传失败,错误号:%d\n", NET_DVR_GetLastError());
						usleep(100000);
						break;
					}
					else if (iStatus == 2)
					{
						printf("文件正在上传，进度：%d\n", pProgress);
						usleep(100000);
					}
					else if (iStatus == 1)
					{
						usleep(100000);
						printf("上传成功！\n");
						return 1;
						break;
					}
					else
					{
						printf("上传失败，状态标志位：%d,错误标志号：%d\n", iStatus, NET_DVR_GetLastError());
						break;
					}
				}
			}
		}
		return 0;
	}


	int FaceDetectAndContrast(char *DeviceIP, int port, char *userName, char *password)
	{
		Demo_AlarmFortify(DeviceIP, port, userName, password);
		return 0;
	}
}

