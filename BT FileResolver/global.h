#pragma once

#include "stdafx.h"
#include <vector>
#include <algorithm>

using namespace std;


/* �Զ�����Ϣ */
#ifndef WM_THREAD_PROCESS_RUNNING
#define WM_THREAD_PROCESS_RUNNING	(WM_USER + 100)
#endif

#ifndef WM_THREAD_PROCESS_DONE
#define WM_THREAD_PROCESS_DONE		(WM_USER + 101)
#endif

//�����ļ���С�Ĳ�������С�ڻ���ڻ�δ����
typedef enum FILTER_FILE_SIZE_OPERATOR
{
    OPERATOR_MORE_THAN, OPERATOR_LESS_THAN, OPERATOR_UNDEFINE
};

/* ���ļ�ȫ�ֱ�����������Щ����ȫ����BT FileResolverView.cpp�ж��� */
extern vector<CString> vecBTFiles;//�ļ��б������������������ļ�
extern vector<CString> vecErrorFiles;//����ʧ�ܵ��ļ��б�
extern DWORD dwLastUpdateUI;	//���һ�θ��½����ʱ��
extern HANDLE hThread;	//�߳̾��
extern BOOL bWantTerminate;	//�̵߳Ľ�����־
extern CString Filter_Type;//����������
extern CString Filter_Keyword;//�ؼ��ֹ���
extern BOOL Filter_Keyword_BTFile;//�Ƿ��������ļ����в��ҹؼ���
extern int Filter_Operator;//�����ļ���С�Ĳ�����
extern CString Filter_FileExt;//ָ����չ��
extern DWORDLONG Filter_FileSize;//�����ļ���С

