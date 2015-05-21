/*
 * SeedResolver.h �汾��2.0
 *
 * ���ߣ�bluekitty(��������һ����) @ CSDN 2012-01
 *
 * ˵����
 * ��ͷ�ļ��������� CSeedResolver�����ڽ���BT�����ļ��������ļ�����ȫbencode������ֽڴ��ļ�������
 * bencode���뷽ʽ�������ļ��Ĺؼ��ֲο�http://wiki.theory.org/BitTorrentSpecification��
 * ���������ļ�������һ��bencode���ֵ䣬��������ؼ��֣����԰�һ�������ļ�������һ��������ṹ����
 * ��Ȼ���ݹ��ǽ��������ļ��Ƚ�ֱ�ӵķ�����
 *
 * ����˵����
 * ��Դ����Ŀ¼��STLĿ¼���� CSeedResolver �����һ��ʵ�֣�ʹ�ñ�׼���map��vector���ݳ�Ϊ1.0�棬
 * 1.0����2.0�湦�ܻ���һ������1.0�����ĳЩ���⡣
 * ����bencode����Ķ��壬�������뵽�ľ���ʹ�ñ�׼���mapʵ��bencode�ֵ䣬��vectorʵ��bencode�б�
 * ʵ�ʲ��Է��֣�
 * ���mapֱ�ӱ���ڵ㣨����ΪNode�����󣬻�ǳ������ڴ棬����һ����6000����ļ�������ʱʹ���ڴ���
 * 70M���ң�����������Ҫ���ƶ���Ͳ����������Ȼmap�Ĳ�������Ż����ٶȷǳ��죬�����д�������ʱ��Ȼ
 * �Ե�ʱ�俪���ܴ󣬻����Ǹ�6000���ļ������ӣ��������Ҫ14�����ҡ�
 * ���map����һ���ڵ��ָ�룬��Ȼ�����ٶȺ��ڴ����Ķ��кܴ�ĸ��ƣ���������ڴ�ܲ����ͷţ���������
 * һ������������ļ�ʱ����Ȼ�����ڴ�й©������ˮƽ���ޣ���Ҳû���ҵ�����İ취��Ҳ����map��allocator
 * ���Խ���ڴ��ͷŵ����⣬����ʵ���Ƕ�stl���ڴ���������Ǻ���Ϥ��������Խ���ڴ��ͷŵ����⣬��ôʹ��
 * ��׼�⻹�ǲ���ģ�һ�Ǵ���Ƚ������׶������Ǳ�׼���зḻ���㷨��û׼�Ժ���õõ������ڱ��������ò�����
 * ������ֻ�Ƕ�ȡ���ӵ���Ϣ����Ƕ�ļ��������������ݴ�����
 *
 */

#pragma once

#include <vector>

#define SAFE_CLEAN(p) if(p)    \
	                  delete p;\
	                  p = NULL;
#define SAFE_RETURN(p, ret) { SAFE_CLEAN(p);return ret; }

/* bencode��ǰ��׺����string�ķָ��� */
#define BENCODE_PREFIX_INT			'i'
#define BENCODE_PREFIX_LIST			'l'
#define BENCODE_PREFIX_DICT			'd'
#define BENCODE_SUFFIX				'e'
#define BENCODE_STRING_DELIMITER	':'

/* ������BitComet����Ƕ�ļ�ǰ׺��������Щ�ļ� */
#define BITCOMET_PADDING_FILE_PREFIX "_____padding_file"

/* �����ļ��Ĺؼ��֣������ļ�������һ��bencode�ֵ����ͣ�������ֵ�Ĺؼ��֣��������ر�˵�����������е��ַ���ֵ����utf-8����
 * ������һЩ�ؼ��֣������Ĺؼ����б�ο�˵��http://wiki.theory.org/BitTorrentSpecification�� ���Ƕ������Щ�ؼ���Ҳ���Ƕ��õõ�*/
#define KEYWORD_ANNOUNCE		"announce"
//bencode�ַ�����tracker������url�����ݹٷ����ͣ��������announce-list�ؼ�����Ӧ�ú�������ؼ���

#define KEYWORD_ANNOUNCE_LIST	"announce-list"
//bencode�б���б��Ǳ�Ҫ�ؼ��֣�tracker�������б�announce-list�б������Ƕ���б�ÿ���б����һ������url�ַ���

#define KEYWORD_CREATION_DATE	"creation date"
//bencode�������Ǳ�Ҫ�ؼ��֣����ӵĴ���ʱ��,����1970-1-1 00:00:00 UTC ������������

#define KEYWORD_COMMENT			"comment"
//bencode�ַ������Ǳ�Ҫ�ؼ��֣����ӵı�ע

#define KEYWORD_COMMENT_UTF8	"comment.utf-8"
//bencode�ַ���,����ؼ����Ҵӹٷ��鲻����������BitComet��չ�Ĺؼ��֣������������ؼ��־ͺ��������Ǹ�����Ϊ���һ����UTF-8�����

#define KEYWORD_CREATED_BY		"created by"
//bencode�ַ������Ǳ�Ҫ�ؼ��֣������������ļ����������汾����ʽ������ uTorrent/183B����ʾuTorrent 183B�洴��

#define KEYWORD_ENCODING		"encoding"
/*bencode�ַ������Ǳ�Ҫ�ؼ��֣��ٷ������������ļ�info�ֵ���pieces�ε��ַ�����ʹ�õı��뷽ʽ��
 *��ʵ�ʷ��ָô�����ı��뷽ʽͬ����Ӱ��info�ֵ��ڵ������ַ�������֪��ʲô��� */

#define KEYWORD_PUBLISHER		"publisher"	
//bencode�ַ��������ӵķ����ߣ�����ؼ����Ҵӹٷ�Ҳ�鲻��������publisher-url�ؼ���

#define KEYWORD_PUBLISHER_UTF8	"publisher.utf-8"
//ͬKEYWORD_COMMENT_UTF8

#define KEYWORD_INFO			"info"
//bencode�ֵ䣬���������ڵ������ļ������忴˵��http://wiki.theory.org/BitTorrentSpecification

/* ������Щ�ؼ�����info�ֵ�Ĺؼ��� */
#define KEYWORD_INFO_PIECE_LENGTH	"piece length"
/* bencode������ÿһ��ĳ��ȣ���λ�ֽڣ��ٷ�������524288��512k�ֽڣ��������в�ͬ��*/

#define KEYWORD_INFO_PIECES			"pieces"
/* bencode�ַ�����ÿ������Ӵ�ֵ��SHA1���̶�20�ֽڳ�������ϣ�������������ܺܳ���
 * �ٷ�˵����byte string, i.e. not urlencoded������Ȼ������ĳ��ȱ�����20�ı�����Ҳ��ͨ��
 * ������ĳ��ȳ���20����һ���ж��ٿ顣
 * ע�⣬����ַ��������Ǳ�׼��c�ַ����������κ�λ�ö�������null�ַ����ַ����������������ڸ��ַ��� */

#define KEYWORD_INFO_PRIVATE		"private"
/* bencode�������Ǳ�Ҫ�ؼ��֣�ʵ��������Կ�����һ��BOOL����Ϊ1ʱ����ʾֻͨ�������ڵķ������õ�peers��
 * ��Ϊ0��δ����ʱ�����Դ��ⲿ���peers������DHT���� */

/* ���¹ؼ����Ǻ��ļ���صģ����忴˵��http://wiki.theory.org/BitTorrentSpecification */
#define KEYWORD_NAME		"name"//���ļ�ģʽ���ļ��������ļ�ģʽ�ǽ����Ե��ļ���Ŀ¼
#define KEYWORD_NAME_UTF8	"name.utf-8"//ͬKEYWORD_COMMENT_UTF8
#define KEYWORD_FILES		"files"//�ļ��б�
#define KEYWORD_PATH		"path"//һ���б����һ�����ļ�����ǰ�����Ŀ¼��
#define KEYWORD_PATH_UTF8	"path.utf-8"//ͬKEYWORD_COMMENT_UTF8
#define KEYWORD_LENGTH		"length"//�ļ�����

/* bencode���������� */
enum BC_TYPE_NAME
{
    BC_STRING, BC_INT, BC_LIST, BC_DICT
};

struct _Node;//���Ľڵ�����������������

/* bencode������ */
typedef char*			BC_String;
typedef DWORDLONG		BC_Int;
typedef struct _Node*	BC_List, **LPBC_List;
typedef struct _BC_Dict
{
    char* pszKey;
    struct _Node* pNode;
} BC_Dict, *LPBC_Dict;


/* ���ڵ㶨�壬��������һ�����ڵ�Ķ��岻��һ�����ο������
 * BC_Dict��BC_List��������ʵ��������һ���ݹ鶨�� */
typedef struct _Node
{
    UINT Type;

    union
    {
        BC_String	bcString;
        BC_Int		bcInt;
        LPBC_List	bcList;
        LPBC_Dict	bcDict;
    } Data;

} Node, *LPNode;

/* �����ڵ��ļ� */
typedef struct _Inner_File
{
    /* ·���� */
    CString PathName;

    /* �ļ��� */
    CString FileName;

    /* �ļ���С����λ���ֽ� */
    DWORDLONG FileSize;

    /* �ļ����ͣ������ļ���ֱ���ṩ���Լ���������BT FileResolverView.cpp�ļ��ж����GetFileInfo���� */
    CString FileType;

    _Inner_File()
    {
        FileSize = 0;
    }
} InnerFile, *PInnerFile;

/* ����ṹ����������Ҫ�õ����������ݣ������ڵ����������ò�����
 * ��ʵ�������̻������Ż���ֱ����������Ҫ���������ݣ���û��ʵ�֣�
 * һ�����������Ժ��п����õ������Ի��Ǳ�����ȫ���� */
typedef struct _Values_Needed
{
    CString Seed_FileName;
    CString Seed_CreationDate;
    CString Seed_Comment;
    CString Seed_Creator;
    CString Seed_Publisher;
    int		Seed_Encoding;//CodePage

    CString Seed_InnerName;//zyf add

    std::vector<InnerFile> Seed_Files;//�����������ļ����б�
} Values_Needed;

class CSeedResolver
{
private:
    UINT m_SeedFileSize;//�����ļ��Ĵ�С
    UINT m_Position;//��ǰָ�����ļ���������λ��

    /* ����������malloc��realloc�����ָ�룬����ʱ�ͷ���Щָ�룬
     * �ݹ���������ļ�ʱ�����ڴ�ķ��䶼ʹ��malloc��realloc��
     * ��������ͳһ�ͷţ�������������Ļ��Ƕ�ʹ��new���䣬ʹ��
     * ��SAFE_CLEAN��SAFE_RETURN�ͷ� */
    std::vector<INT_PTR> vecAllocated;

    /* ����Ƿ�Խ��(�����ļ���������Χ) */
    BOOL IsRangeValid(UINT nOffset = 0);

    /* ȡ��һ��bencode�ַ���(��׼c�ַ���) */
    char* GetBCString(char** pBuffer);

    /* ȡ��һ��bencode���� */
    DWORDLONG GetBCInt(char** pBuffer);

    /* ����ָ���Ļ����������Ǹ��ݹ麯�� */
    LPNode ResovleBuffer(char** pBuffer);

    /* ��pDict�õ�һ��ָ���ؼ��ֵĽڵ㣨LPNode*������������ֵΪ�ùؼ����Ƿ����
     * ���ܽ�pDict��������Ϊconst����ΪҪ�ƶ�pDictָ�� */
    BOOL GetNode(LPBC_Dict pDict, const char* pszKey, _Out_ LPNode* pNode);

    /* �ͷŽ��������з���������ڴ� */
    void DeallocAll();

    /* ��׼c�ַ��� -> Unicode��nCodePage = 0��ʾʹ�������ڶ���Ĵ���ҳ */
    void ConvertToUnicode(const char* pStr, CString& s, int nCodePage = 0);

public:
    Values_Needed SeedInfo;

    CSeedResolver(const CString& SeedFileName);
    ~CSeedResolver(void);

    /* �����������ȵ��øú������������ݷ���ֵȷ���Ƿ�����ɹ� */
    BOOL Resolve();

};
