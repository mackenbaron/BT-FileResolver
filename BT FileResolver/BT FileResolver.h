// BT FileResolver.h : BT FileResolver Ӧ�ó������ͷ�ļ�
//
#pragma once

#ifndef __AFXWIN_H__
#error "�ڰ������ļ�֮ǰ������stdafx.h�������� PCH �ļ�"
#endif

#include "resource.h"       // ������


// CBTFileResolverApp:
// �йش����ʵ�֣������ BT FileResolver.cpp
//

class CBTFileResolverApp: public CWinApp
{

public:
    CBTFileResolverApp();

    // ��д
public:
    virtual BOOL InitInstance();

    // ʵ��
    afx_msg void OnAppAbout();
    DECLARE_MESSAGE_MAP()
};

extern CBTFileResolverApp theApp;