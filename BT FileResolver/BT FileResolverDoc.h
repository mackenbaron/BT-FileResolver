// BT FileResolverDoc.h : CBTFileResolverDoc ��Ľӿ�
//


#pragma once


class CBTFileResolverDoc: public CDocument
{
protected: // �������л�����
    CBTFileResolverDoc();
    DECLARE_DYNCREATE(CBTFileResolverDoc)

    // ����
public:

    // ����
public:

    // ��д
public:
    virtual BOOL OnNewDocument();
    virtual void Serialize(CArchive& ar);

    // ʵ��
public:
    virtual ~CBTFileResolverDoc();
#ifdef _DEBUG
    virtual void AssertValid() const;
    virtual void Dump(CDumpContext& dc) const;
#endif

protected:

    // ���ɵ���Ϣӳ�亯��
protected:
    DECLARE_MESSAGE_MAP()
};


