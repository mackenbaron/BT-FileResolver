#include "StdAfx.h"
#include "SeedResolver.h"

//�����������׳��쳣ʱ�Ƿ��жϣ����ڵ���
//#define BREAK_ON_THROW

using namespace std;

#pragma warning(disable : 4244 4996)

CSeedResolver::CSeedResolver(const CString& SeedFileName)
{
    ASSERT(!SeedFileName.IsEmpty());

    SeedInfo.Seed_FileName = SeedFileName;
}

CSeedResolver::~CSeedResolver(void)
{
    //����
    DeallocAll();
}

//�Ѵ�дת��ΪСд��ȡ�����ӵı����ʽʱҪ�õ�
void szCharToLower(char* pszString)
{
    int i = 0;
    while(pszString[i])
    {
        pszString[i] = tolower(pszString[i]);
        ++i;
    }
}

BOOL CSeedResolver::Resolve()
{
    char* pBuffer = NULL;
    char* pShadowBuffer = NULL;

    HANDLE hFile = CreateFile(SeedInfo.Seed_FileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

    if(hFile == INVALID_HANDLE_VALUE)
    {
        return FALSE;
    }

    CFile file(hFile);
    m_SeedFileSize = (UINT)file.GetLength();
    m_Position = 0;

    if(!m_SeedFileSize)
    {
        return FALSE;
    }

    pBuffer = new char[m_SeedFileSize];
    file.Read((void*)pBuffer, m_SeedFileSize);
    file.Close();

    //ResovleBuffer�������ƶ�ָ�룬���Դ���һ��pBuffer�ĸ��������������ͷ�pBuffer
    pShadowBuffer = pBuffer;

    LPNode pNode = NULL;

#ifndef BREAK_ON_THROW
    try
    {
#endif

        pNode = ResovleBuffer(&pShadowBuffer);

#ifndef BREAK_ON_THROW
    }
    catch(int)
    {
        SAFE_RETURN(pBuffer, FALSE);
    }
#endif	

    //����ļ�������
    SAFE_CLEAN(pBuffer);

    if(pNode->Type != BC_DICT) return FALSE;
    //�������Ǹ�bencode�ֵ�

    LPBC_Dict pRootDict = pNode->Data.bcDict;

    /* ��ʼȡֵ */

    //������ȡ�����ӵı����ʽ
    if(GetNode(pRootDict, KEYWORD_ENCODING, &pNode))
    {
        szCharToLower(pNode->Data.bcString);
        if(!strlen(pNode->Data.bcString) || strcmp(pNode->Data.bcString, "utf-8") == 0)
            SeedInfo.Seed_Encoding = CP_UTF8;
        else if(strcmp(pNode->Data.bcString, "utf-7") == 0)//utf-7
            SeedInfo.Seed_Encoding = CP_UTF7;
        else if(strcmp(pNode->Data.bcString, "gbk") == 0)//��������
            SeedInfo.Seed_Encoding = 936;
        else if(strcmp(pNode->Data.bcString, "big5") == 0)//��������
            SeedInfo.Seed_Encoding = 950;
        else if(strcmp(pNode->Data.bcString, "shift_jis") == 0)//����
            SeedInfo.Seed_Encoding = 932;
        else if(strcmp(pNode->Data.bcString, "windows-874") == 0)//̩��
            SeedInfo.Seed_Encoding = 874;
        else if(strcmp(pNode->Data.bcString, "ks_c_5601-1987") == 0)//����
            SeedInfo.Seed_Encoding = 949;
        else//������Ϊutf-8
            SeedInfo.Seed_Encoding = CP_UTF8;
    }
    else
    {
        SeedInfo.Seed_Encoding = CP_UTF8;//Ĭ��Ϊutf-8
    }

    //seed inner name for rename later zyf begin
    if(GetNode(pRootDict, KEYWORD_NAME_UTF8, &pNode) && pNode->Type == BC_STRING)
    {
        ConvertToUnicode(pNode->Data.bcString, SeedInfo.Seed_InnerName, CP_UTF8);
    }
    else if(GetNode(pRootDict, KEYWORD_NAME, &pNode) && pNode->Type == BC_STRING)
    {
        ConvertToUnicode(pNode->Data.bcString, SeedInfo.Seed_Comment);
    }
    else
    {
        SeedInfo.Seed_InnerName = _T("");
    }
    //zyf end

    //��������
    if(GetNode(pRootDict, KEYWORD_CREATION_DATE, &pNode) && pNode->Type == BC_INT)
    {
        CTime time(0);
        CTimeSpan ts(pNode->Data.bcInt);

        time += ts;
        SeedInfo.Seed_CreationDate.Format(_T("%d��%.2d��%.2d�� %.2dʱ%.2d��"), time.GetYear(), time.GetMonth(), time.GetDay(), time.GetHour(), time.GetMinute());
    }
    else
    {
        SeedInfo.Seed_CreationDate = _T("(δ֪����)");
    }

    //���ӵı�ע
    if(GetNode(pRootDict, KEYWORD_COMMENT_UTF8, &pNode) && pNode->Type == BC_STRING)//���ȿ���û��utf-8�����
    {
        ConvertToUnicode(pNode->Data.bcString, SeedInfo.Seed_Comment, CP_UTF8);
    }
    else if(GetNode(pRootDict, KEYWORD_COMMENT_UTF8, &pNode) && pNode->Type == BC_STRING)//���û��utf-8����ľ�ʹ�����ñ����
    {
        ConvertToUnicode(pNode->Data.bcString, SeedInfo.Seed_Comment);
    }
    else
    {
        SeedInfo.Seed_Comment = _T("(û�б�ע)");
    }

    //���ӵĴ�������
    if(GetNode(pRootDict, KEYWORD_CREATED_BY, &pNode) && pNode->Type == BC_STRING)
    {
        ConvertToUnicode(pNode->Data.bcString, SeedInfo.Seed_Creator, CP_UTF8);
    }
    else
    {
        SeedInfo.Seed_Creator = _T("(δ֪)");
    }

    //���ӵķ�����
    if(GetNode(pRootDict, KEYWORD_PUBLISHER_UTF8, &pNode) &&//���ȿ���û��utf-8�����
       pNode->Type == BC_STRING)
    {
        ConvertToUnicode(pNode->Data.bcString, SeedInfo.Seed_Publisher, CP_UTF8);
    }
    else if(GetNode(pRootDict, KEYWORD_PUBLISHER, &pNode) &&//���û��utf-8����ľ�ʹ�����ñ����
            pNode->Type == BC_STRING)
    {
        ConvertToUnicode(pNode->Data.bcString, SeedInfo.Seed_Publisher);
    }
    else
    {
        SeedInfo.Seed_Publisher = _T("(δ֪)");
    }

    //��ȡ�����ڵ������ļ�
    LPBC_Dict pInfoDict = NULL;//info�ֵ�

    if(!GetNode(pRootDict, KEYWORD_INFO, &pNode) || pNode->Type != BC_DICT) return FALSE;

    pInfoDict = pNode->Data.bcDict;
    pNode = NULL;

    GetNode(pInfoDict, KEYWORD_FILES, &pNode);

    //���pNodeΪNULL����û���ҵ��ؼ�KEYWORD_FILES����ô���Ǹ����ļ����ӣ�����Ϊ���ļ����ӻ�����Ч
    if(!pNode)
    {
        InnerFile iFile;
        int nCodePage = 0;

        //���ļ�ģʽinfo�ֵ��name�ؼ������ļ���
        if(GetNode(pInfoDict, KEYWORD_NAME_UTF8, &pNode))
            nCodePage = CP_UTF8;
        else if(GetNode(pInfoDict, KEYWORD_NAME, &pNode))
            nCodePage = 0;
        else
            return FALSE;

        if(pNode->Type != BC_STRING) return FALSE;

        ConvertToUnicode(pNode->Data.bcString, iFile.FileName, nCodePage);
        iFile.PathName = _T("\\");

        //�ļ���С
        if(!GetNode(pInfoDict, KEYWORD_LENGTH, &pNode) || pNode->Type != BC_INT)
            return FALSE;
        else
            iFile.FileSize = pNode->Data.bcInt;

        SeedInfo.Seed_Files.push_back(iFile);
    }
    else if(pNode->Type == BC_LIST)
    {
        /* ����Ĵ����е㲻̫���׿������Ҷ��ⶨ����pFileDict��pPathList��������
         * ʹ��������Щ��ֻҪ��סLPBC_Dict��ָ��BC_Dict�ṹ��ָ�룬��LPBC_List��
         * ָ��Node�ṹ��ָ���ָ��*/
        LPBC_List pFileList = pNode->Data.bcList;//info�ֵ��ڵ�files�б�

        while(*pFileList)
        {
            if((*pFileList)->Type != BC_DICT) return FALSE;//ÿ���б���Ŀ���붼���ֵ�����

            LPBC_Dict pFileDict = (*pFileList)->Data.bcDict;

            InnerFile iFile;
            int nCodePage = 0;

            //ȷ������ҳ����ȡ��path�б�
            if(GetNode(pFileDict, KEYWORD_PATH_UTF8, &pNode))
                nCodePage = CP_UTF8;
            else if(GetNode(pFileDict, KEYWORD_PATH, &pNode))
                nCodePage = 0;
            else
                return FALSE;

            if(pNode->Type != BC_LIST) return FALSE;//path�������б�

            LPBC_List pPathList = pNode->Data.bcList;
            vector<CString> vecTemp;//��ʱ�洢

            while(*pPathList)
            {
                if((*pPathList)->Type != BC_STRING) return FALSE;//path��ÿ����Ŀ�������ַ���

                CString s;

                ConvertToUnicode((*pPathList)->Data.bcString, s, nCodePage);
                vecTemp.push_back(s);

                ++(pPathList);
            }

            //��Ҫ��vecTemp���н������ֽ���ļ�����Ŀ¼
            if(vecTemp.front().Find(_T(BITCOMET_PADDING_FILE_PREFIX)) != 0)//������BitComet����Ƕ�ļ�
            {
                iFile.FileName = vecTemp.back();
                vecTemp.pop_back();
                for(vector<CString>::iterator iter = vecTemp.begin(); iter != vecTemp.end(); ++iter)
                {
                    iFile.PathName += *iter + _T("\\");
                }
                if(iFile.PathName.IsEmpty()) iFile.PathName = _T("\\");


                //ȡ���ļ���С
                if(!GetNode(pFileDict, KEYWORD_LENGTH, &pNode) ||
                   pNode->Type != BC_INT)
                   return FALSE;
                else
                    iFile.FileSize = pNode->Data.bcInt;

                //��iFile����SeedInfo.Seed_Files
                SeedInfo.Seed_Files.push_back(iFile);
            }

            ++pFileList;
        }
    }
    else
        return FALSE;

    return TRUE;
}

LPNode CSeedResolver::ResovleBuffer(char** pBuffer)
{
    Node* pNode = NULL;

    switch(**pBuffer)
    {
        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
            pNode = (LPNode)malloc(sizeof(Node));

            pNode->Type = BC_STRING;
            pNode->Data.bcString = GetBCString(pBuffer);//���ñ���GetBCString���ص�ָ�룬GetBCString������Ƿ񱣴����ָ��
            vecAllocated.push_back((INT_PTR)pNode);

            if(!pNode->Data.bcString) throw NULL;//�����ļ��д���
            break;
        case BENCODE_PREFIX_INT:
            pNode = (LPNode)malloc(sizeof(Node));

            pNode->Type = BC_INT;
            pNode->Data.bcInt = GetBCInt(pBuffer);
            vecAllocated.push_back((INT_PTR)pNode);

            if(pNode->Data.bcInt == _I64_MAX) throw NULL;//�����ļ��д���
            break;
        case BENCODE_PREFIX_LIST:
        {
            //������ͷ��BENCODE_PREFIX_LIST
            ++(*pBuffer);
            ++m_Position;

            pNode = (LPNode)malloc(sizeof(Node));

            pNode->Type = BC_LIST;
            pNode->Data.bcList = NULL;
            vecAllocated.push_back((INT_PTR)pNode);

            UINT i = 0;

            while(**pBuffer != BENCODE_SUFFIX)
            {
                pNode->Data.bcList = (LPBC_List)realloc((void*)pNode->Data.bcList, (i + 2)/*�����һ��Ԫ�ش��NULL*/ * sizeof(BC_List));

                try
                {
                    pNode->Data.bcList[i] = ResovleBuffer(pBuffer);
                }
                catch(INT)
                {
                    vecAllocated.push_back((INT_PTR)pNode->Data.bcList);
                    throw NULL;
                }

                ++i;
            }

            /* �����Ǹ�ѭ���������ļ��д�������Ǹ����б�������һ�ζ���ִ�У��׳��쳣 */
            if(!i)
                throw NULL;
            else
                vecAllocated.push_back((INT_PTR)pNode->Data.bcList);

            /* bcList�Ǹ�ָ��Ķ�̬���飨BC_Listʵ����LPNode���������޷�ȷ����̬����ĳ��ȣ�������ΪbcList����ռ��ʱ��
             * �������һ��Ԫ�صĿռ䣬����Ϊ���Ԫ�ظ�ֵNULL�������ͬ��׼c�ַ���һ������NULL
             * ��β�������������̬�����ʱ�򣬶���NULL����Ϊ������������*/
            pNode->Data.bcList[i] = NULL;

            //������β��BENCODE_SUFFIX
            ++(*pBuffer);
            ++m_Position;
        }
        break;
        case BENCODE_PREFIX_DICT:
        {
            //����bencode�ֵ�Ľ����ɲο�����Ķ�bencode�б�Ľ���
            ++(*pBuffer);
            ++m_Position;

            pNode = (LPNode)malloc(sizeof(Node));
            pNode->Type = BC_DICT;
            pNode->Data.bcDict = NULL;
            vecAllocated.push_back((INT_PTR)pNode);

            UINT i = 0;
            while(**pBuffer != BENCODE_SUFFIX)
            {
                pNode->Data.bcDict = (LPBC_Dict)realloc((void*)pNode->Data.bcDict, (i + 2) * sizeof(BC_Dict));

                try
                {
                    pNode->Data.bcDict[i].pszKey = GetBCString(pBuffer);
                    if(!pNode->Data.bcDict[i].pszKey) throw NULL;//GetBCString�����׳��쳣�����Ա����鷵��ֵ
                    pNode->Data.bcDict[i].pNode = ResovleBuffer(pBuffer);
                }
                catch(INT)
                {
                    vecAllocated.push_back((INT_PTR)pNode->Data.bcList);
                    throw NULL;
                }

                ++i;
            }

            if(!i)
                throw NULL;
            else
                vecAllocated.push_back((INT_PTR)pNode->Data.bcList);

            /* �ֵ�����洦���б��е㲻һ����bcDict������Ԫ�ز���ָ�룬���ԾͰѽ�βԪ�ص�pszKey��NULL��
             * ����������bcDict����ʱ������ĳ��Ԫ�ص�Key��NULLʱ��Ϊ���� */
            pNode->Data.bcDict[i].pszKey = NULL;

            ++(*pBuffer);
            ++m_Position;
        }
        break;
        default:
            //��������Ч��bencodeǰ׺��ֱ���׳��쳣
            throw NULL;
            break;
    }



    return pNode;
}
void CSeedResolver::DeallocAll()
{
    for(vector<INT_PTR>::iterator iter = vecAllocated.begin(); iter != vecAllocated.end(); ++iter)
    {
        free((void*)*iter);
    }
}

BOOL CSeedResolver::IsRangeValid(UINT nOffset /* = 0 */)
{
    return m_Position + nOffset <= m_SeedFileSize;
}

char* CSeedResolver::GetBCString(char** pBuffer)
{
    char* pszRet = NULL;
    char* pszStrLen;
    UINT nLen = 0;

    //bencode�ַ���������0-9�����ֿ�ʼ
    if(**pBuffer < '0' || **pBuffer >'9')
        return pszRet;

    while(*(*pBuffer + nLen) != BENCODE_STRING_DELIMITER)
    {
        ++nLen;

        //����Ƿ�Խ��
        if(!IsRangeValid(nLen))	return pszRet;
    }

    //nLenΪbcstringǰ���ı�ʾbcstring���ȵ��ַ�����
    pszStrLen = new char[nLen + 1];
    ZeroMemory(pszStrLen, nLen + 1);
    memcpy((void*)pszStrLen, (void*)*pBuffer, nLen);

    //����Ƿ�Խ��
    if(!IsRangeValid(nLen + 1)) return pszRet;

    //�ƶ�ָ��
    *pBuffer += nLen + 1/* 1Ϊ':' */;
    m_Position += nLen + 1;

    //nLen��ʱΪbcstring�ĳ���
    nLen = atoi(pszStrLen);
    SAFE_CLEAN(pszStrLen);

    if(!nLen)//nLen=0�Ǹ����ַ���
    {
        pszRet = (char*)malloc(sizeof(char));
        pszRet[0] = '\0';
        vecAllocated.push_back((INT_PTR)pszRet);
    }
    else
    {
        //����Ƿ�Խ��
        if(!IsRangeValid(nLen)) return pszRet;

        pszRet = (char*)malloc((nLen + 1) * sizeof(char));
        memset((void*)pszRet, NULL, nLen + 1);
        vecAllocated.push_back((INT_PTR)pszRet);
        memcpy((void*)pszRet, (void*)*pBuffer, nLen);
    }

    //�ƶ�ָ��
    *pBuffer += nLen;
    m_Position += nLen;

    return pszRet;
}

DWORDLONG CSeedResolver::GetBCInt(char** pBuffer)
{
    DWORDLONG dwlRet = _I64_MAX;
    char* psz;
    UINT nLen = 0;

    //bencode����������i��ʼ
    if(**pBuffer != BENCODE_PREFIX_INT)
        return dwlRet;

    //����Ƿ�Խ��
    if(!IsRangeValid(1)) return dwlRet;

    ++(*pBuffer);
    ++m_Position;

    while(m_Position <= m_SeedFileSize && *(*pBuffer + nLen) != BENCODE_SUFFIX)
    {
        ++nLen;

        //����Ƿ�Խ��
        if(!IsRangeValid(nLen)) return dwlRet;
    }

    psz = new char[nLen + 1];
    ZeroMemory(psz, nLen + 1);
    memcpy((void*)psz, (void*)*pBuffer, nLen);
    if(*psz == '0' && nLen == 1)//0��bencode��Ч������
        dwlRet = 0;
    else
        dwlRet = _atoi64(psz) ? _atoi64(psz) : _I64_MAX;
    SAFE_CLEAN(psz);

    //����Ƿ�Խ��
    if(!IsRangeValid(nLen + 1)) return _I64_MAX;

    //�ƶ�ָ��
    *pBuffer += nLen + 1/* 1Ϊ'e' */;
    m_Position += nLen + 1;

    return dwlRet;
}

BOOL CSeedResolver::GetNode(LPBC_Dict pDict, const char* pszKey, _Out_ LPNode* pNode)
{
    if(!pDict) return FALSE;

    while(pDict->pszKey)//��pszKey�ж϶�̬�����Ƿ����
    {
        if(strcmp(pDict->pszKey, pszKey) == 0)
        {
            *pNode = pDict->pNode;
            return TRUE;
        }

        ++pDict;
    }

    return FALSE;
}

void CSeedResolver::ConvertToUnicode(const char* pStr, CString& s, int nCodePage /*= 0*/)
{
    wchar_t* szUnicode = NULL;
    UINT nLen;
    int nCP;

    if(!pStr) return;

    /* ���ַ��������ã�Ӧ���Զ������룬�Ҷ��ַ��ı�����벻�죬ֻ����������ʱû�з����������*/
    if(!nCodePage)//û��ָ������ҳ��ʹ�������ڶ���Ĵ���ҳ
        nCP = SeedInfo.Seed_Encoding;
    else
        nCP = nCodePage;

    nLen = MultiByteToWideChar(nCP, NULL, pStr, (int)strlen(pStr) + 1, NULL, NULL);
    szUnicode = new wchar_t[nLen];
    MultiByteToWideChar(nCP, NULL, pStr, (int)strlen(pStr) + 1, szUnicode, nLen);

    s.Format(_T("%s"), szUnicode);

    SAFE_CLEAN(szUnicode);
}
