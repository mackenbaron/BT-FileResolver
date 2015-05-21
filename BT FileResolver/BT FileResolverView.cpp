// BT FileResolverView.cpp : CBTFileResolverView ���ʵ��
#include "stdafx.h"

#include "BT FileResolver.h"

#include "BT FileResolverDoc.h"
#include "BT FileResolverView.h"
#include "global.h"
#include "WorkThread.h"
#include <afxpriv.h>	//��Ϣ - WM_IDLEUPDATECMDUI

#pragma warning( disable: 4996 )

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#define FREE_THREAD_HANDLE(h) { if(h) CloseHandle(h); h = NULL; }
#define UPDATE_TOOLBAR_UI { (CToolBar*) AfxGetMainWnd()->GetDescendantWindow(AFX_IDW_TOOLBAR)->SendMessage(WM_IDLEUPDATECMDUI, (WPARAM) TRUE, NULL); }
#define SET_PROGRESS_BAR_MARQUEE_STYLE(h, b) { b ? ::SetWindowLong(h, GWL_STYLE, ::GetWindowLong(h, GWL_STYLE) | PBS_MARQUEE) : \
	::SetWindowLong(h, GWL_STYLE, ::GetWindowLong(h, GWL_STYLE) & ~PBS_MARQUEE); }

//���½����ʱ���������ٺ������һ�ν���
#define UPDATE_UI_INTERVAL			200
#define UPDATE_PROGRESS_INTERVAL	50

//�����ļ�����չ��
#define BT_FILE_EXT_LOWER _T(".torrent")

//������𣬿��Զ����Щ���ע��Ҫ��д����β�ģ�������
#define FILTER_TYPE_ALL		_T("")
#define FILTER_TYPE_VIDEO	_T("AVI;ASF;WMV;AVS;FLV;MKV;MOV;3GP;MP4;MPG;MPEG;DAT;OGM;VOB;RM;RMVB;TS;TP;IFO;NSV;M2TS;")
#define FILTER_TYPE_MUSIC	_T("MP3;AAC;WAV;WMA;CDA;FLAC;M4A;MID;MKA;MP2;MPA;MPC;APE;OFR;OGG;RA;WV;TTA;AC3;DTS;")
#define FILTER_TYPE_PICTURE	_T("BMP;GIF;JPEG;JPG;PNG;TIF;")
#define FILTER_TYPE_SOFT	_T("7Z;RAR;ZIP;ISO;ISZ;")

//������̵�״̬
typedef enum PROCESS_STATE
{
    PROCESS_STATE_IDLE/* ���� */, PROCESS_STATE_GENERATING/* �����ļ� */, PROCESS_STATE_RUNNING/* ���� */
};

//���ļ�ȫ�ֱ������壬������global.h
vector<CString> vecBTFiles;
vector<CString> vecErrorFiles;
DWORD dwLastUpdateUI = 0;
HANDLE hThread = NULL;
BOOL bWantTerminate = FALSE;
CString Filter_Type;
CString Filter_Keyword;
BOOL Filter_Keyword_BTFile;
int Filter_Operator;
CString Filter_FileExt;
DWORDLONG Filter_FileSize;

//ȫ�ֱ�������
CString FindStr;
SORT_PARAM SortParam;//�������
LIST_ITEM CurrentSelItem;//����Ǹ���ϸ��Ϣ�Ի���Ļص�ʹ�õģ����ݵ�ǰѡ����б���Ŀ

//���Ŀ¼��ѡ��ָ���ļ�
void ExploreFile(const CString& FileName)
{
    if(FileName.IsEmpty()) return;

    ShellExecute(::GetDesktopWindow(), _T("open"), _T("explorer.exe"),
                 _T("/select,") + FileName, NULL, SW_SHOWNORMAL);
}

//ȡ��ָ���ļ�����������ƣ������ض�Ӧ��ͼ������
int GetFileInfo(CString FileName, CString& TypeName)
{
    SHFILEINFO shFileInfo;
    ZeroMemory(&shFileInfo, sizeof(SHFILEINFO));

    if(SHGetFileInfo(FileName, FILE_ATTRIBUTE_NORMAL, &shFileInfo, sizeof(SHFILEINFO),
        SHGFI_SYSICONINDEX | SHGFI_SMALLICON | SHGFI_TYPENAME | SHGFI_USEFILEATTRIBUTES))
        TypeName.Format(_T("%s"), shFileInfo.szTypeName);
    else
        TypeName = _T("�ļ�");

    return shFileInfo.iIcon;
}

/* �º���
 * ��������Ҳ����ͨ������LIST_ITEM��<�����ʵ�֣�������֪����ô����
 * ���ֵ�ƴ�����������������е��ң������Ǹ��ݱʻ������ */
BOOL IsLesser(const LIST_ITEM& item1, const LIST_ITEM& item2)
{
    CString s1, s2;
    DWORDLONG dwl1 = _I64_MAX;
    DWORDLONG dwl2 = _I64_MAX;

    switch(SortParam.nColIndex)
    {
        case 0://�ļ���
            s1 = item1.FileName;
            s2 = item2.FileName;
            break;
        case 1://�ļ���С
            dwl1 = item1.FileSize;
            dwl2 = item2.FileSize;
            break;
        case 2://�б�����
            s1 = item1.FileTypeName;
            s2 = item2.FileTypeName;
            break;
        case 3://·��
            s1 = item1.InnerPath;
            s2 = item2.InnerPath;
            break;

            /*
            case 4://���ӷ�����
            s1 = item1.BTPublisher;
            s2 = item2.BTPublisher;
            break;
            case 5://�����ļ���
            s1 = item1.BTFileName;
            s2 = item2.BTFileName;
            break;
            case 6://���Ӵ�������
            s1 = item1.BTCreator;
            s2 = item2.BTCreator;
            break;
            case 7://���Ӵ�������
            s1 = item1.BTCreationDate;
            s2 = item2.BTCreationDate;
            break;
            case 8://��ע
            s1 = item1.BTComment;
            s2 = item2.BTComment;
            break;
            */
    }

    if(dwl1 != _I64_MAX)//�Ƚ��ļ���С
        return SortParam.bSortAsc ? dwl1 < dwl2 : dwl2 < dwl1;
    else//�Ƚ��ַ���
        return SortParam.bSortAsc ? s1 < s2 : s2 < s1;

}

// CBTFileResolverView

IMPLEMENT_DYNCREATE(CBTFileResolverView, CListView)

BEGIN_MESSAGE_MAP(CBTFileResolverView, CListView)
    /* �Զ�����Ϣ���� */
    ON_MESSAGE(WM_THREAD_PROCESS_DONE, &CBTFileResolverView::OnWorkDone)
    ON_MESSAGE(WM_THREAD_PROCESS_RUNNING, &CBTFileResolverView::OnProcess)

    /*  */
    ON_COMMAND(ID_FILE_OPENPATH, &CBTFileResolverView::OnFileOpenpath)
    ON_COMMAND(ID_ACTION_PROCESS, &CBTFileResolverView::OnActionProcess)
    ON_UPDATE_COMMAND_UI(ID_ACTION_PROCESS, &CBTFileResolverView::OnUpdateActions)
    ON_COMMAND(ID_ACTION_CANCEL, &CBTFileResolverView::OnActionCancel)
    ON_UPDATE_COMMAND_UI(ID_ACTION_CANCEL, &CBTFileResolverView::OnUpdateActions)
    ON_COMMAND(ID_ACIION_DELETE, &CBTFileResolverView::OnActionDelete)
    ON_COMMAND(ID_ACTION_CLEAR, &CBTFileResolverView::OnActionClear)
    ON_UPDATE_COMMAND_UI(ID_ACIION_DELETE, &CBTFileResolverView::OnUpdateActions)
    ON_UPDATE_COMMAND_UI(ID_ACTION_CLEAR, &CBTFileResolverView::OnUpdateActions)
    ON_UPDATE_COMMAND_UI(ID_FILE_OPENPATH, &CBTFileResolverView::OnUpdateActions)
    ON_WM_RBUTTONDOWN()
    ON_COMMAND(ID_BT_OPEN_FILE, &CBTFileResolverView::OnBtOpenFile)
    ON_COMMAND(ID_BT_OPEN_PATH, &CBTFileResolverView::OnBtOpenPath)
    ON_UPDATE_COMMAND_UI(ID_BT_OPEN_FILE, &CBTFileResolverView::OnUpdateActions)
    ON_UPDATE_COMMAND_UI(ID_BT_OPEN_PATH, &CBTFileResolverView::OnUpdateActions)
    ON_WM_INITMENUPOPUP()
    ON_WM_LBUTTONDBLCLK()
    ON_BN_CLICKED(IDC_DEBUG_TEST, &CBTFileResolverView::OnBnClickedDebugTest)
    ON_NOTIFY_REFLECT(LVN_GETDISPINFO, &CBTFileResolverView::OnLvnGetdispinfo)
    ON_WM_KEYDOWN()
    ON_NOTIFY_REFLECT(LVN_COLUMNCLICK, &CBTFileResolverView::OnLvnColumnclick)
    ON_BN_CLICKED(IDC_BUTTON1, &CBTFileResolverView::OnBnClickedButton1)
    ON_COMMAND(ID_VIEW_ERROR, &CBTFileResolverView::OnViewError)
    ON_UPDATE_COMMAND_UI(ID_VIEW_ERROR, &CBTFileResolverView::OnUpdateActions)
    ON_COMMAND(ID_VIEW_DETAIL, &CBTFileResolverView::OnViewDetail)
    ON_UPDATE_COMMAND_UI(ID_VIEW_DETAIL, &CBTFileResolverView::OnUpdateActions)
    //    ON_COMMAND(ID_32788,&CBTFileResolverView::On32788)
    ON_COMMAND(ID_FILE_RENAMEZYF, &CBTFileResolverView::OnFileRenamezyf)
END_MESSAGE_MAP()

// CBTFileResolverView ����/����

CBTFileResolverView::CBTFileResolverView()
{
    // TODO: �ڴ˴���ӹ������
    m_ProcessState = PROCESS_STATE_IDLE;
}

CBTFileResolverView::~CBTFileResolverView()
{
    //����һ��Ҫ����
    m_ListViewIL.Detach();
}

BOOL CBTFileResolverView::PreCreateWindow(CREATESTRUCT& cs)
{
    // TODO: �ڴ˴�ͨ���޸�
    //  CREATESTRUCT cs ���޸Ĵ��������ʽ

    cs.style = cs.style & ~LVS_TYPEMASK | LVS_REPORT |
        LVS_SHOWSELALWAYS | LVS_OWNERDATA;

    return CListView::PreCreateWindow(cs);
}

void CBTFileResolverView::OnInitialUpdate()
{
    CListView::OnInitialUpdate();

    GetListCtrl().SetExtendedStyle(GetListCtrl().GetExtendedStyle() | LVS_EX_INFOTIP | LVS_EX_FULLROWSELECT);

    SortParam.bSortAsc = TRUE;
    SortParam.nColIndex = 0;

    //dialogbar��ָ��
    m_wndDialogBar = AfxGetMainWnd()->GetDescendantWindow(AFX_IDW_DIALOGBAR);

    m_wndDialogBar->CheckRadioButton(IDC_RADIO_ALL, IDC_RADIO_SOFT, IDC_RADIO_ALL);
    m_wndDialogBar->GetDlgItem(IDC_COMBO_SIZE_TYPE)->SendMessage(CB_SETCURSEL, 0, NULL);
    m_wndDialogBar->SetDlgItemText(IDC_FILE_SIZE, _T("0"));
    m_wndDialogBar->CheckDlgButton(IDC_CHECK_AUTO_START, TRUE);
    m_wndDialogBar->CheckDlgButton(IDC_CHECK_KEYWORD_BTFILE, TRUE);
    ((CEdit*)m_wndDialogBar->GetDlgItem(IDC_FILE_SIZE))->SetLimitText(8);//��� ��GB

#ifndef _DEBUG
    m_wndDialogBar->GetDlgItem(IDC_DEBUG_TEST)->ShowWindow(SW_HIDE);//��Release�汾�����ز��԰�ť
#endif

    //ͼ���б�
    /*
     *	����һ��Ҫ��CBTFileResolverView������ǰ��m_ListViewIL��������ͼ���б���Ϊm_ListViewIL��ͼ���б���ָ��
     *	ϵͳ��ͼ���б�ģ�CBTFileResolverView����ʱ�����m_ListViewIL���������ϵͳͼ���б��ĳЩͼ�걻�ͷţ�
     *	��CBTFileResolverView������������
     *	m_ListViewIL��ͼ���б�����ϵͳά���ģ���Ҫ�滻����ӻ�ɾ����������ͼ�꣬�õ���Ӧ��ͼ�������ķ�����GetFileInfo������
     */
    SHFILEINFO shFileInfo;
    m_ListViewIL.Attach((HIMAGELIST)SHGetFileInfo(_T(""), FILE_ATTRIBUTE_NORMAL,
        &shFileInfo, sizeof(SHFILEINFO), SHGFI_SYSICONINDEX | SHGFI_SMALLICON | SHGFI_USEFILEATTRIBUTES));
    GetListCtrl().SetImageList(&m_ListViewIL, LVSIL_SMALL);

    //����������
    CStatusBar* pBar = (CStatusBar*)AfxGetMainWnd()->GetDescendantWindow(AFX_IDW_STATUS_BAR);
    if(pBar)
    {
        CRect r;
        pBar->GetItemRect(pBar->CommandToIndex(ID_INDICATOR_PROGRESS), &r);
        r.InflateRect(-1, -1, -1, -1);
        m_Progress.Create(WS_CHILD, r, pBar, NULL);
        m_Progress.SetRange(0, 100);
    }

    CString headers[] =
    {
        _T("�ļ���"),
        _T("��С"),
        _T("����"),
        _T("������·��"),

        /* �����⼸��Ͳ�����ʾ�ˣ������е��ң��ĵ���ϸ��Ϣ�Ի���ȥ��ʾ��
        _T("������"),
        _T("�����ļ���"),
        _T("���Ӵ�������"),
        _T("���Ӵ�������"),
        _T("���ӱ�ע"),
        _*/
    };

    int colsize[] =
    {
        350,
        120,
        150,
        200,
    };

    int nLen = sizeof(headers) / sizeof(CString);

    CListCtrl& list = GetListCtrl();
    for(int i = 0; i != nLen; ++i)
    {
        if(i == 1)
            list.InsertColumn(i, headers[i], LVCFMT_RIGHT);
        else
            list.InsertColumn(i, headers[i], LVCFMT_LEFT);

        list.SetColumnWidth(i, colsize[i]);
    }
}


// CBTFileResolverView ���

#ifdef _DEBUG
void CBTFileResolverView::AssertValid() const
{
    CListView::AssertValid();
}

void CBTFileResolverView::Dump(CDumpContext& dc) const
{
    CListView::Dump(dc);
}

CBTFileResolverDoc* CBTFileResolverView::GetDocument() const // �ǵ��԰汾��������
{
    ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CBTFileResolverDoc)));
    return (CBTFileResolverDoc*)m_pDocument;
}
#endif //_DEBUG


// CBTFileResolverView ��Ϣ�������

void CBTFileResolverView::OnFileOpenpath()
{
    // TODO: �ڴ���������������
    ::BROWSEINFO bi;
    ZeroMemory(&bi, sizeof(BROWSEINFO));
    bi.hwndOwner = AfxGetMainWnd()->GetSafeHwnd();
    bi.lpszTitle = _T("Select the directory that contains the seed file");
    bi.ulFlags = BIF_RETURNONLYFSDIRS;

    LPITEMIDLIST pItemIdList = ::SHBrowseForFolder(&bi);

    if(pItemIdList)
    {
        TCHAR szPath[MAX_PATH];
        if(SHGetPathFromIDList(pItemIdList, szPath))
        {
            m_CurrentPath = szPath;

            if(m_CurrentPath.ReverseFind(_T('\\')) != m_CurrentPath.GetLength() - 1)
                m_CurrentPath += _T("\\");

            SetStatusText(ID_INDICATOR_CURRENT_FILE_PATH, _T("��ǰĿ¼��") + m_CurrentPath);

            if(m_wndDialogBar->IsDlgButtonChecked(IDC_CHECK_AUTO_START)) OnActionProcess();
        }
    }
}

void CBTFileResolverView::OnActionProcess()
{
    ASSERT(m_ProcessState == PROCESS_STATE_IDLE);

    /* ȡ�����еĹ�����Ϣ */
    switch(m_wndDialogBar->GetCheckedRadioButton(IDC_RADIO_ALL, IDC_RADIO_SOFT))
    {
        case IDC_RADIO_ALL:
            Filter_Type = FILTER_TYPE_ALL;
            break;
        case IDC_RADIO_VIDEO:
            Filter_Type = FILTER_TYPE_VIDEO;
            break;
        case IDC_RADIO_MUSIC:
            Filter_Type = FILTER_TYPE_MUSIC;
            break;
        case IDC_RADIO_PICTURE:
            Filter_Type = FILTER_TYPE_PICTURE;
            break;
        case IDC_RADIO_SOFT:
            Filter_Type = FILTER_TYPE_SOFT;
            break;
        default:
            Filter_Type = FILTER_TYPE_ALL;
            break;
    }

    m_wndDialogBar->GetDlgItemText(IDC_FILE_EXTENDSION, Filter_FileExt);
    Filter_FileExt.Trim();Filter_FileExt.MakeUpper();Filter_FileExt += _T(" ");

    m_wndDialogBar->GetDlgItemText(IDC_COMBO_KEYWORD, Filter_Keyword);
    Filter_Keyword.Trim();Filter_Keyword.MakeUpper();


    Filter_Operator = ((CComboBox*)m_wndDialogBar-> \
                       GetDlgItem(IDC_COMBO_SIZE_TYPE))->GetCurSel();

    CString FileSize;
    m_wndDialogBar->GetDlgItemText(IDC_FILE_SIZE, FileSize);
    if(FileSize.IsEmpty())
        Filter_Operator = OPERATOR_UNDEFINE;
    else
    {
        Filter_FileSize = _tstoi64(FileSize.GetString());

        if(Filter_FileSize > (DWORDLONG)_UI64_MAX / 1024)
        {
            AfxMessageBox(_T("���������Ŀ���ļ��Ĵ�С��Ч��"));
            return;
        }

        if(!Filter_FileSize && Filter_Operator == OPERATOR_LESS_THAN)
        {
            AfxMessageBox(_T("������������Ŀ���ļ��Ĵ�С������0KB��"));
            return;
        }
    }

    Filter_Keyword_BTFile = m_wndDialogBar->IsDlgButtonChecked(IDC_CHECK_KEYWORD_BTFILE);
    /* ȡ�����еĹ�����Ϣ */

    //���
    m_vecListItems.clear();
    vecErrorFiles.clear();
    vecBTFiles.clear();
    GetListCtrl().SetItemCountEx(0);
    GetListCtrl().Invalidate();
    GetListCtrl().UpdateWindow();
    UpdateFileCount();

    //��ʼ����ָ��Ŀ¼�ڵ������ļ�
    m_ProcessState = PROCESS_STATE_GENERATING;
    UPDATE_TOOLBAR_UI;

    //�����ļ�
    BeginWaitCursor();
    SetStatusText(ID_INDICATOR_PROCESS_STATE, _T("���������ļ�..."));
    SET_PROGRESS_BAR_MARQUEE_STYLE(m_Progress.GetSafeHwnd(), TRUE);
    m_Progress.ShowWindow(SW_SHOW);
    FindFile(m_CurrentPath);
    SET_PROGRESS_BAR_MARQUEE_STYLE(m_Progress.GetSafeHwnd(), FALSE);
    m_Progress.ShowWindow(SW_HIDE);
    EndWaitCursor();

    //���������߳�
    bWantTerminate = FALSE;
    hThread = ::CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)&ResolveFun,
                             (LPVOID)GetSafeHwnd(), CREATE_SUSPENDED, NULL);

    if(!hThread)
    {
        MessageBox(_T("���������߳�ʧ�ܡ�"), _T("����"), MB_ICONERROR);
        m_ProcessState = PROCESS_STATE_IDLE;
        SetStatusText(ID_INDICATOR_PROCESS_STATE, _T("���������߳�ʧ��"));
        return;
    }
    else
    {
        m_Progress.SetPos(0);
        m_Progress.ShowWindow(SW_SHOW);
        AddKeywordToList();//���ؼ��ּ��뵽combo���б�

        //�߳̿�ʼִ��
        m_ProcessState = PROCESS_STATE_RUNNING;
        UPDATE_TOOLBAR_UI;
        ResumeThread(hThread);
    }
}

void CBTFileResolverView::OnActionCancel()
{
    ASSERT(m_ProcessState == PROCESS_STATE_RUNNING);

    // TODO: ֹͣ����
    if(IDYES == MessageBox(_T("ȷ��ֹͣ����ô���Ѿ��������Ľ������"),
        _T("ֹͣ����"), MB_ICONQUESTION + MB_YESNO + MB_DEFBUTTON2))
        bWantTerminate = TRUE;
}

void CBTFileResolverView::OnActionDelete()
{
    // TODO: ɾ��ѡ����ļ�
    if(!GetListCtrl().GetSelectedCount()) return;

    while(POSITION pos =
          GetListCtrl().GetFirstSelectedItemPosition())
    {
        int nPos = GetListCtrl().GetNextSelectedItem(pos);
        m_vecListItems.erase(m_vecListItems.begin() + nPos);
        GetListCtrl().DeleteItem(nPos);
    }

    UpdateFileCount();
}

void CBTFileResolverView::OnActionClear()
{
    // TODO: ����ļ��б�
    if(IDYES == MessageBox(_T("����б��ڵ��ļ���"), _T("���"),
        MB_ICONEXCLAMATION + MB_YESNO))
    {
        m_vecListItems.clear();
        GetListCtrl().DeleteAllItems();
        UpdateFileCount();
    }
}

void CBTFileResolverView::OnUpdateActions(CCmdUI *pCmdUI)
{
    // TODO: �ڴ������������û����洦��������

    switch(pCmdUI->m_nID)
    {
        case ID_FILE_OPENPATH:
            pCmdUI->Enable(m_ProcessState == PROCESS_STATE_IDLE);
            break;
        case ID_ACTION_PROCESS:
            pCmdUI->Enable(m_ProcessState == PROCESS_STATE_IDLE &&
                           !m_CurrentPath.IsEmpty());
            break;
        case ID_ACTION_CANCEL:
            pCmdUI->Enable(m_ProcessState == PROCESS_STATE_RUNNING);
            break;
        case ID_ACIION_DELETE:
            pCmdUI->Enable(GetListCtrl().GetSelectedCount() != 0 &&
                           m_ProcessState == PROCESS_STATE_IDLE);
            break;
        case ID_ACTION_CLEAR:
            pCmdUI->Enable(GetListCtrl().GetItemCount() != 0 &&
                           m_ProcessState == PROCESS_STATE_IDLE);
            break;
        case ID_BT_OPEN_FILE:
            pCmdUI->Enable(GetListCtrl().GetSelectedCount() == 1);
            break;
        case ID_BT_OPEN_PATH:
            pCmdUI->Enable(GetListCtrl().GetSelectedCount() == 1);
            break;
        case ID_VIEW_ERROR:
            pCmdUI->Enable(m_ProcessState == PROCESS_STATE_IDLE);
            break;
        case ID_VIEW_DETAIL:
            pCmdUI->Enable(GetListCtrl().GetSelectedCount() == 1);
            break;
    }
}

/* �Զ�����Ϣ���� - WM_THREAD_PROCESS_DONE */
LRESULT CBTFileResolverView::OnWorkDone(WPARAM wParam, LPARAM lParam)
{
    /* �߳��Ѿ�������ϲ��˳� */
    m_ProcessState = PROCESS_STATE_IDLE;
    FREE_THREAD_HANDLE(hThread);

    UPDATE_TOOLBAR_UI;
    UpdateFileCount();
    SetStatusText(ID_INDICATOR_PROCESS_STATE, _T("����"));
    m_Progress.ShowWindow(SW_HIDE);
    vecBTFiles.clear();

    //GetListCtrl().SetItemCountEx(m_vecListItems.size(), LVSICF_NOSCROLL | LVSICF_NOINVALIDATEALL);
    //original
    GetListCtrl().SetItemCountEx((int)m_vecListItems.size(), LVSICF_NOSCROLL | LVSICF_NOINVALIDATEALL);
    return S_OK;
}

/* �Զ�����Ϣ���� - WM_THREAD_PROCESS_RUNNING */
LRESULT CBTFileResolverView::OnProcess(WPARAM wParam, LPARAM lParam)
{
    if(!wParam && !lParam) return S_FALSE;

    if(!lParam)//��lParamΪNULLʱ���½�����
    {
        CString s;

        s.Format(_T("%d"), (int)wParam);//wParam�ǵ�ǰ�Ľ���
        SetStatusText(ID_INDICATOR_PROGRESS, s);
    }
    else//�����б��ļ�
    {
        PInnerFile pInnerFile = (PInnerFile)wParam;//wParam���ݵ���InnerFile�ṹ��ָ��
        CSeedResolver* pReso = (CSeedResolver*)lParam;//lParam���ݵ���CBTFileResolver���ָ��

        LIST_ITEM litem;
        tagForRename tagRename;

        litem.FileName = pInnerFile->FileName;
        GetFileInfo(litem.FileName, litem.FileTypeName);
        litem.FileSize = pInnerFile->FileSize;//�ļ���С����OnLvnGetdispinfo�б���ʽ��
        litem.InnerPath = pInnerFile->PathName;
        litem.BTPublisher = pReso->SeedInfo.Seed_Publisher;
        litem.BTFileName = pReso->SeedInfo.Seed_FileName;
        litem.BTCreator = pReso->SeedInfo.Seed_Creator;
        litem.BTCreationDate = pReso->SeedInfo.Seed_CreationDate;
        litem.BTComment = pReso->SeedInfo.Seed_Comment;

        tagRename.BTFileName = pReso->SeedInfo.Seed_InnerName;//zyf
        tagRename.BTInnerName = pReso->SeedInfo.Seed_InnerName;//zyf

        m_vecListItems.push_back(litem);
        m_vecRenam.push_back(tagRename);//zyf
        //GetListCtrl().SetItemCountEx(m_vecListItems.size(), LVSICF_NOSCROLL | LVSICF_NOINVALIDATEALL);
        //original
        GetListCtrl().SetItemCountEx((int)m_vecListItems.size(), LVSICF_NOSCROLL | LVSICF_NOINVALIDATEALL);

        /* ��Ҫ����ĸ��½��棬���½����Ƿǳ���ʱ�Ĳ��� */
        if(::GetTickCount() - dwLastUpdateUI >= UPDATE_UI_INTERVAL)
        {
            CString s;
            s.Format(_T("���ڽ��� %s"), pReso->SeedInfo.Seed_FileName);
            SetStatusText(ID_INDICATOR_PROCESS_STATE, s);
            UpdateFileCount();
            dwLastUpdateUI = ::GetTickCount();
        }
    }

    return S_OK;
}

void CBTFileResolverView::SetStatusText(const int nCommanddID, const CString& Text)
{
    CStatusBar* pBar = (CStatusBar*)AfxGetMainWnd()->GetDescendantWindow(AFX_IDW_STATUS_BAR);

    if(!pBar) return;

    if(ID_INDICATOR_PROGRESS != nCommanddID)
    {
        pBar->SetPaneText(pBar->CommandToIndex(nCommanddID), Text);
    }
    else
    {
        /* ���½����� */
        int nPos = _tstoi(Text);
        m_Progress.SetPos(nPos <= 100 ? nPos : 100);
    }
}

BOOL CBTFileResolverView::IsTorrentFile(const CString& FileName)
{
    int n = FileName.ReverseFind('.');
    if(n == -1) return FALSE;

    CString s = FileName.Mid(n);

    return s.CompareNoCase(BT_FILE_EXT_LOWER) == 0;
}

void CBTFileResolverView::FindFile(CString sPath)
{
    CString sFind;
    WIN32_FIND_DATA fdata;

    sFind = sPath + _T("*.*");
    HANDLE hFind = ::FindFirstFile(sFind, &fdata);

    if(INVALID_HANDLE_VALUE == hFind)
        return;

    while(TRUE)
    {
        //�Ƿ���Ҫ���½���
        if(::GetTickCount() - dwLastUpdateUI >= UPDATE_PROGRESS_INTERVAL)
        {
            m_Progress.StepIt();
            SetStatusText(ID_INDICATOR_PROCESS_STATE, _T("���� ") + sPath);
            dwLastUpdateUI = ::GetTickCount();
        }

        if(fdata.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        {
            if(fdata.cFileName[0] != _T('.'))
            {
                FindFile(sPath + fdata.cFileName + _T("\\"));
            }
        }
        else
        {
            if(IsTorrentFile(fdata.cFileName))
                vecBTFiles.push_back(sPath + fdata.cFileName);
        }

        if(!FindNextFile(hFind, &fdata)) break;
    }

    FindClose(hFind);
}

void CBTFileResolverView::UpdateFileCount()
{
    CString s;
    s.Format(_T("�ļ�������%d"), m_vecListItems.size());
    SetStatusText(ID_INDICATOR_FILE_COUNT, s);
}

void CBTFileResolverView::AddKeywordToList()
{
    if(Filter_Keyword.IsEmpty()) return;

    CComboBox* pCombo = (CComboBox*)m_wndDialogBar->GetDescendantWindow(IDC_COMBO_KEYWORD);

    if(!pCombo) return;

    if(pCombo->FindString(-1, Filter_Keyword) < 0)
        pCombo->InsertString(0, Filter_Keyword);
}

void CBTFileResolverView::OnRButtonDown(UINT nFlags, CPoint point)
{
    // TODO: �ڴ������Ϣ�����������/�����Ĭ��ֵ
    CListView::OnRButtonDown(nFlags, point);

    CMenu menu;

    menu.LoadMenu(IDR_POPUP);
    ClientToScreen(&point);
    menu.GetSubMenu(0)->TrackPopupMenu(TPM_LEFTALIGN, point.x, point.y, this);
}

void CBTFileResolverView::OnBtOpenFile()
{
    if(GetListCtrl().GetSelectedCount() != 1) return;

    POSITION pos = GetListCtrl().GetFirstSelectedItemPosition();
    int nPos = GetListCtrl().GetNextSelectedItem(pos);
    CString BT_File = m_vecListItems[nPos].BTFileName;

    if(!BT_File.IsEmpty())
        ShellExecute(::GetDesktopWindow(), _T("open"), BT_File, NULL, NULL, SW_SHOWNORMAL);
}

void CBTFileResolverView::OnBtOpenPath()
{
    if(GetListCtrl().GetSelectedCount() != 1) return;

    POSITION pos = GetListCtrl().GetFirstSelectedItemPosition();
    int nPos = GetListCtrl().GetNextSelectedItem(pos);
    CString BT_File = m_vecListItems[nPos].BTFileName;

    ExploreFile(BT_File);
}

void CBTFileResolverView::OnInitMenuPopup(CMenu* pPopupMenu, UINT nIndex, BOOL bSysMenu)
{
    CListView::OnInitMenuPopup(pPopupMenu, nIndex, bSysMenu);

    if(bSysMenu) return;

    int nCount = pPopupMenu->GetMenuItemCount();
    CCmdUI cmd;

    cmd.m_pMenu = pPopupMenu;
    cmd.m_nIndexMax = nCount;

    for(int i = 0; i < nCount; i++)
    {
        UINT nID = pPopupMenu->GetMenuItemID(i);
        if(nID != ID_SEPARATOR)
        {
            cmd.m_nIndex = i;
            cmd.m_nID = nID;
            cmd.DoUpdate(this, TRUE);
        }
    }
}

void CBTFileResolverView::OnLButtonDblClk(UINT nFlags, CPoint point)
{
    // TODO: �ڴ������Ϣ�����������/�����Ĭ��ֵ
    CListView::OnLButtonDblClk(nFlags, point);

    OnViewDetail();
}

void CBTFileResolverView::OnLvnGetdispinfo(NMHDR *pNMHDR, LRESULT *pResult)
{
    NMLVDISPINFO *pDispInfo = reinterpret_cast<NMLVDISPINFO*>(pNMHDR);
    // TODO: �ڴ���ӿؼ�֪ͨ����������

    LVITEM* pItem = &(pDispInfo)->item;
    int iItem = pItem->iItem;


    if(pItem->mask & LVIF_TEXT)
    {
        switch(pItem->iSubItem)
        {
            case 0://�ļ���
                pItem->pszText = (LPTSTR)m_vecListItems[iItem].FileName.GetString();
                break;
            case 1://��С��kb
            {
                CString FileSize = _T("");
                int nLen = 0;
                TCHAR szFileSize[64];
                _i64tot((__int64)(m_vecListItems[iItem].FileSize / 1024), szFileSize, 10);
                FileSize.Format(_T("%s"), szFileSize);
                FileSize.MakeReverse();
                int nPos = 3;
                while(nPos < FileSize.GetLength())
                {
                    FileSize.Insert(nPos, _T(','));
                    nPos += 4;
                }
                FileSize.MakeReverse();
                FileSize += _T(" KB");
                _tcscpy(pItem->pszText, FileSize.GetString());
            }
            break;
            case 2://�����
                pItem->pszText = (LPTSTR)m_vecListItems[iItem].FileTypeName.GetString();
                break;
            case 3://������·��
                pItem->pszText = (LPTSTR)m_vecListItems[iItem].InnerPath.GetString();
                break;

                ///* ������ʾ
            case 4://���ӷ�����
                pItem->pszText = (LPTSTR)m_vecListItems[iItem].BTPublisher.GetString();
                break;
            case 5://�����ļ���
                pItem->pszText = (LPTSTR)m_vecListItems[iItem].BTFileName.GetString();
                break;
            case 6://���Ӵ�������
                pItem->pszText = (LPTSTR)m_vecListItems[iItem].BTCreator.GetString();
                break;
            case 7://���Ӵ�������
                pItem->pszText = (LPTSTR)m_vecListItems[iItem].BTCreationDate.GetString();
                break;
            case 8://���ӱ�ע
                pItem->pszText = (LPTSTR)m_vecListItems[iItem].BTComment.GetString();
                break;
                //*/
        }
    }

    if(pItem->mask & LVIF_IMAGE)
        pItem->iImage = GetFileInfo(m_vecListItems[iItem].FileName,
        m_vecListItems[iItem].FileTypeName);

    *pResult = 0;
}

void CBTFileResolverView::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
    // TODO: �ڴ������Ϣ�����������/�����Ĭ��ֵ

    switch(nChar)
    {
        case VK_DELETE://����DEL��
            OnActionDelete();
            break;
    }

    CListView::OnKeyDown(nChar, nRepCnt, nFlags);
}

void CBTFileResolverView::OnLvnColumnclick(NMHDR *pNMHDR, LRESULT *pResult)
{
    LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
    // TODO: �ڴ���ӿؼ�֪ͨ����������

    if(m_ProcessState == PROCESS_STATE_IDLE)
    {
        if(SortParam.nColIndex == pNMLV->iSubItem)
            SortParam.bSortAsc = !SortParam.bSortAsc;
        else
        {
            SortParam.bSortAsc = TRUE;
            SortParam.nColIndex = pNMLV->iSubItem;
        }

        //����
        stable_sort(m_vecListItems.begin(), m_vecListItems.end(), IsLesser);
        GetListCtrl().Invalidate();
    }

    *pResult = 0;
}

//���԰�ť�ĵ����Ϣ����Release�汾�н����ز��԰�ť
void CBTFileResolverView::OnBnClickedDebugTest()
{

}


void CBTFileResolverView::OnBnClickedButton1()
{
    // ˵��
    CString Desciptions;

    Desciptions.Format(_T("���%s\n\n��չ����%s\n\n�����ؼ��֣�%s\n\n�ļ���С��%s\n\nѡ��Ŀ¼��������ʼ��%s"),
                       _T("������ͬ������������ļ��������������Ҫ������ָ����չ����"),
                       _T("ָ��Ҫ�������������ļ���չ����ָ���󽫺��ԡ���𡱲����������չ���Կո�ָ�����Ҫ����㣬���磺txt exe dat��"),
                       _T("�������ļ����������ָ���Ĺؼ��֣�����ؼ����Կո�ָ������磺�й� ������\
                          			\n            �����ѡ�ˡ�ͬʱ��BT�ļ����в��ҡ���������������ļ����������ؼ��֣�������BT�ļ����а���Ҳ���Ƿ���������"),
                                    _T("ָ���������ļ��Ĵ�С���������ڻ�С��ĳ��ֵ��KBΪ��λ��"),
                                    _T("ѡ��Ҫ������Ŀ¼��������ʼ�������̡�"));

    AfxMessageBox(Desciptions, MB_ICONINFORMATION | MB_OK);
}

//�����ļ��Ի���Ļص�
BOOL CALLBACK DLG_ERROR_PROC(HWND hwndDlg,
                             UINT message,
                             WPARAM wParam,
                             LPARAM lParam)
{
    switch(message)
    {
        case WM_COMMAND:
            switch(LOWORD(wParam))
            {
                case IDOK:
                    EndDialog(hwndDlg, wParam);
                    break;
                case IDC_LIST_ERROR_FILES:
                    if(HIWORD(wParam) == LBN_DBLCLK)
                    {
                        CListBox* pList = (CListBox*)CWnd::FromHandle(::GetDlgItem(hwndDlg, IDC_LIST_ERROR_FILES));

                        int nIndex = pList->GetCurSel();
                        if(nIndex >= 0)
                        {
                            CString FileName;
                            pList->GetText(nIndex, FileName);

                            ExploreFile(FileName);
                        }
                    }
                    break;
            }
            return TRUE;
            break;
        case WM_CLOSE:
            EndDialog(hwndDlg, wParam);
            return TRUE;
            break;
        case WM_INITDIALOG:
        {
            CListBox* pList = (CListBox*)CWnd::FromHandle(::GetDlgItem(hwndDlg, IDC_LIST_ERROR_FILES));

            CString Prompt;
            Prompt.Format(_T("�ļ�����%d"), vecErrorFiles.size());
            ::SetDlgItemText(hwndDlg, IDC_STATIC_ERROR_COUNT, Prompt);
            for(vector<CString>::iterator iter = vecErrorFiles.begin();
                iter != vecErrorFiles.end();
                ++iter)
            {
                pList->InsertString(0, *iter);
            }
        }
        return TRUE;
        break;
    }

    return FALSE;
}

//��ϸ��Ϣ�Ի���Ļص�
BOOL CALLBACK DLG_DETAIL_PROC(HWND hwndDlg,
                              UINT message,
                              WPARAM wParam,
                              LPARAM lParam)
{
    switch(message)
    {
        case WM_COMMAND:
            switch(LOWORD(wParam))
            {
                case IDOK:
                    EndDialog(hwndDlg, wParam);
                    break;
                case IDC_BUTTON1:
                    if(HIWORD(wParam) == BN_CLICKED)
                    {
                        ExploreFile(CurrentSelItem.BTFileName);
                    }
                    break;
            }
            return TRUE;
            break;
        case WM_CLOSE:
            EndDialog(hwndDlg, wParam);
            return TRUE;
            break;
        case WM_INITDIALOG:
            ::SetDlgItemText(hwndDlg, IDC_EDIT1, CurrentSelItem.BTFileName);
            ::SetDlgItemText(hwndDlg, IDC_EDIT2, CurrentSelItem.BTCreationDate);
            ::SetDlgItemText(hwndDlg, IDC_EDIT3, CurrentSelItem.BTCreator);
            ::SetDlgItemText(hwndDlg, IDC_EDIT4, CurrentSelItem.BTPublisher);
            ::SetDlgItemText(hwndDlg, IDC_EDIT5, CurrentSelItem.BTComment);
            return TRUE;
            break;
    }

    return FALSE;
}

void CBTFileResolverView::OnViewError()
{
    //DialogBox(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDD_ERROR), GetSafeHwnd(), DLG_ERROR_PROC);
    //original
}

void CBTFileResolverView::OnViewDetail()
{
    if(GetListCtrl().GetSelectedCount() != 1) return;
    POSITION pos = GetListCtrl().GetFirstSelectedItemPosition();
    int nIndex = GetListCtrl().GetNextSelectedItem(pos);

    CurrentSelItem = m_vecListItems[nIndex];

    //DialogBox(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDD_DETAIL), GetSafeHwnd(), DLG_DETAIL_PROC);
    //original
}

void CBTFileResolverView::OnFileRenamezyf()
{
    // TODO: Add your command handler code here
    vector<tagForRename>::iterator iter;
    for(iter = m_vecRenam.begin(); iter != m_vecRenam.end(); iter++)
    {
        //_T("\\") +
        MessageBox(iter->BTInnerName);
        if(!::MoveFile(m_CurrentPath + iter->BTFileName, m_CurrentPath + iter->BTInnerName))
            MessageBox(_T("Rename Error!"));
    }
}
