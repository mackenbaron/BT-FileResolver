// BT FileResolverDoc.cpp : CBTFileResolverDoc ���ʵ��
//

#include "stdafx.h"
#include "BT FileResolver.h"

#include "BT FileResolverDoc.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CBTFileResolverDoc

IMPLEMENT_DYNCREATE(CBTFileResolverDoc, CDocument)

BEGIN_MESSAGE_MAP(CBTFileResolverDoc, CDocument)
END_MESSAGE_MAP()


// CBTFileResolverDoc ����/����

CBTFileResolverDoc::CBTFileResolverDoc()
{
    // TODO: �ڴ����һ���Թ������

}

CBTFileResolverDoc::~CBTFileResolverDoc()
{
}

BOOL CBTFileResolverDoc::OnNewDocument()
{
    if(!CDocument::OnNewDocument())
        return FALSE;

    // TODO: �ڴ�������³�ʼ������
    // (SDI �ĵ������ø��ĵ�)

    return TRUE;
}




// CBTFileResolverDoc ���л�

void CBTFileResolverDoc::Serialize(CArchive& ar)
{
    if(ar.IsStoring())
    {
        // TODO: �ڴ���Ӵ洢����
    }
    else
    {
        // TODO: �ڴ���Ӽ��ش���
    }
}


// CBTFileResolverDoc ���

#ifdef _DEBUG
void CBTFileResolverDoc::AssertValid() const
{
    CDocument::AssertValid();
}

void CBTFileResolverDoc::Dump(CDumpContext& dc) const
{
    CDocument::Dump(dc);
}
#endif //_DEBUG

