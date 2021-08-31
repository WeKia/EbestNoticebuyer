// ebsetvcDlg.cpp: 구현 파일
//

#include "pch.h"
#include "framework.h"
#include "ebsetvc.h"
#include "ebsetvcDlg.h"
#include "afxdialogex.h"

#include "packet/S3_.h"
#include "packet/K3_.h"
#include "packet/CSPAT00600.h"
#include "packet/t1101.h"
#include "packet/NWS.h"
#include "packet/t0424.h"
#include "packet/t0167.h"
#include "packet/t8430.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CebsetvcDlg 대화 상자



CebsetvcDlg::CebsetvcDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_EBSETVC_DIALOG, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);

	pswd = "1024";
}

void CebsetvcDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_ACNT_TEXT, account_static);
	DDX_Control(pDX, IDC_TIME_TEXT, timer_static);
	DDX_Control(pDX, IDC_SERVER_STATE, state_static);
	DDX_Control(pDX, IDC_ACCOUNT_LIST, account_listctrl);
}

BEGIN_MESSAGE_MAP(CebsetvcDlg, CDialogEx)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_WM_TIMER()
	ON_MESSAGE(WM_USER + XM_LOGIN, OnLogin)
	ON_MESSAGE(WM_USER + XM_DISCONNECT, OnDisconnect)
	ON_MESSAGE(WM_USER + XM_RECEIVE_DATA, OnXMReceiveData)
	ON_MESSAGE(WM_USER + XM_GETBALANCE, OnReqBalance)
END_MESSAGE_MAP()


// CebsetvcDlg 메시지 처리기

BOOL CebsetvcDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// 이 대화 상자의 아이콘을 설정합니다.  응용 프로그램의 주 창이 대화 상자가 아닐 경우에는
	//  프레임워크가 이 작업을 자동으로 수행합니다.
	SetIcon(m_hIcon, TRUE);			// 큰 아이콘을 설정합니다.
	SetIcon(m_hIcon, FALSE);		// 작은 아이콘을 설정합니다.

	// TODO: 여기에 추가 초기화 작업을 추가합니다.

	if (g_iXingAPI.Init("c:\\eBest\\xingAPI") == FALSE)
	{
		MessageBox("xingAPI DLL을 Load 할 수 없습니다.");
		return -1;
	}

	ConnectServer();
	Login();

	InitListCtrl();

	return TRUE;  // 포커스를 컨트롤에 설정하지 않으면 TRUE를 반환합니다.
}

// 대화 상자에 최소화 단추를 추가할 경우 아이콘을 그리려면
//  아래 코드가 필요합니다.  문서/뷰 모델을 사용하는 MFC 애플리케이션의 경우에는
//  프레임워크에서 이 작업을 자동으로 수행합니다.

void CebsetvcDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 그리기를 위한 디바이스 컨텍스트입니다.

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 클라이언트 사각형에서 아이콘을 가운데에 맞춥니다.
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 아이콘을 그립니다.
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

// 사용자가 최소화된 창을 끄는 동안에 커서가 표시되도록 시스템에서
//  이 함수를 호출합니다.
HCURSOR CebsetvcDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

//================================================================================================
// 서버 접속
//================================================================================================
BOOL CebsetvcDlg::ConnectServer()
{
	//-----------------------------------------------------------------------------
	// 이미 접속되어 있으면 접속을 종료한다.
	if (g_iXingAPI.IsConnected())
	{
		g_iXingAPI.Disconnect();
	}

	//-----------------------------------------------------------------------------
	// 서버 IP
	CString strServerIP = "hts.ebestsec.co.kr";
	//-----------------------------------------------------------------------------
	// 서버 Port
	int nServerPort = 20001;
	//-----------------------------------------------------------------------------
	// 공유기 사용
	int nSendPacketSize = -1;		// -1 로 설정하면 기본값을 사용한다.
	//-----------------------------------------------------------------------------
	// 서버연결시간
	int nConnectTimeOut = -1;		// -1 로 설정하면 기본값을 사용한다.

	//-----------------------------------------------------------------------------
	// 서버접속
	BOOL bResult = g_iXingAPI.Connect(
		AfxGetMainWnd()->GetSafeHwnd(),			// Connect가 된 이후에 Disconnect 메시지를 받을 윈도우 Handle
												// Login 윈도우는 Login 만 처리하는 윈도우 이므로 Disconnect는 메인윈도우에서 받는다.

		strServerIP,							// 서버주소

		nServerPort,							// 서버포트

		WM_USER,								// XingAPI에서 사용하는 메시지의 시작번호, Windows에서는 사용자메시지를 0x400(=WM_USER) 이상을
												// 사용해야 함. 기본적으로는 WM_USER를 사용하면 되지만 프로그램 내부에서 메시지 ID가 겹치게 되면
												// 이 값을 조정하여 메시지 ID 충돌을 피할수 있음

		nConnectTimeOut,						// 지정한 시간이상(1/1000 초 단위)으로 시간이 걸리게 될 경우 연결실패로 간주함

		nSendPacketSize							// 보내어지는 Packet Size, -1 이면 기본값 사용
												// 인터넷 공유기등에서는 특정 크기 이상의 데이터를 한번에 보내면 에러가 떨어지는 경우가 발생
												// 이럴 경우에 한번에 보내는 Packet Size를 지정하여 그 이상 되는 Packet은 여러번에 걸쳐 전송
	);

	//-----------------------------------------------------------------------------
	// 접속실패 처리
	if (bResult == FALSE)
	{
		int nErrorCode = g_iXingAPI.GetLastError();
		CString strMsg = g_iXingAPI.GetErrorMessage(nErrorCode);
		MessageBox(strMsg, "서버접속실패", MB_ICONSTOP);
		return FALSE;
	}

	return TRUE;
}

//================================================================================================
// 로그인
//================================================================================================
BOOL CebsetvcDlg::Login()
{
	CString strID = "wekia";
	CString strPwd = "pjy@1004";
	CString strCertPwd = "qofqm@akstp12";
	int nServerType = 0;
	BOOL bShowCertErrDlg = FALSE;

	BOOL bResult = g_iXingAPI.Login(
		GetSafeHwnd(),							// Login 성공여부 메시지를 받을 윈도우
		strID,									// 사용자 ID
		strPwd,									// 사용자 비밀번호
		strCertPwd,								// 공인인증 비밀번호
		nServerType,							// 0 : 실서버, 1 : 모의투자서버
		bShowCertErrDlg							// 로그인 중에 공인인증 에러가 발생시 에러메시지 표시여부
	);

	//-----------------------------------------------------------------------------
	// 로그인 에러 발생,
	//		이것은 로그인 사전단계에서 발생한 에러이며 로그인 과정에서 발생한 에러는
	//		메시지로 알려준다.
	if (bResult == FALSE)
	{
		EnableWindow(TRUE);

		int nErrorCode = g_iXingAPI.GetLastError();
		CString strMsg = g_iXingAPI.GetErrorMessage(nErrorCode);
		MessageBox(strMsg, "로그인 실패", MB_ICONSTOP);
		return FALSE;
	}

	return TRUE;
}

//================================================================================================
// 로그인 결과
//================================================================================================
LRESULT CebsetvcDlg::OnLogin(WPARAM wParam, LPARAM lParam)
{
	LPCSTR pszCode = (LPCSTR)wParam;
	LPCSTR pszMsg = (LPCSTR)lParam;

	CString strMsg;
	strMsg.Format("[%s] %s", pszCode, pszMsg);


	if (atoi(pszCode) != 0)
	{
		// 로그인 실패
		MessageBox(strMsg, "로그인 실패", MB_ICONSTOP);
	}

	SetAccountText();

	nwsWorkerChild.Create(IDD_NWS_DIALOG, this);
	nwsWorkerChild.CenterWindow();
	nwsWorkerChild.ShowWindow(SW_SHOW);

	InquiryBalance();

	SetTimer(MAIN_TIMER, 1000, NULL);

	return 0L;
}

LRESULT CebsetvcDlg::OnDisconnect(WPARAM wParam, LPARAM lParam)
{
	MessageBox("서버와의 연결이 종료되었습니다.", "통신에러", MB_ICONSTOP);
	return 0L;
}


void CebsetvcDlg::SetAccountText()
{
	char szAccount[20];

	if (g_iXingAPI.GetAccountList(0, szAccount, sizeof(szAccount)) == FALSE)
	{
		MessageBox("계좌 정보를 불러 올 수 없음", "통신에러", MB_ICONSTOP);
	}

	account_static.SetWindowTextA(szAccount);
	account = szAccount;
	nwsWorkerChild.account = szAccount;
}

void CebsetvcDlg::InitListCtrl()
{
	account_listctrl.InsertColumn(0, "주문가능금액", LVCFMT_LEFT, 90);
	account_listctrl.InsertColumn(1, "총매입", LVCFMT_LEFT, 90);
	account_listctrl.InsertColumn(2, "총평가", LVCFMT_LEFT, 90);
	account_listctrl.InsertColumn(3, "총손익", LVCFMT_LEFT, 90);
	account_listctrl.InsertColumn(4, "총수익률(%)", LVCFMT_LEFT, 90);
	account_listctrl.InsertColumn(5, "추정자산", LVCFMT_LEFT, 90);
}

void CebsetvcDlg::GetCodeList(bool bNext)
{
	t8430InBlock pckInBlock;

	char			szNextKey[] = "";

	FillMemory(&pckInBlock, sizeof(pckInBlock), ' ');

	SetPacketData(pckInBlock.gubun, sizeof(pckInBlock.gubun), "1", DATA_TYPE_STRING);	// 구분

	int nRqID = g_iXingAPI.Request(
		GetSafeHwnd(),				// 데이터를 받을 윈도우, XM_RECEIVE_DATA 으로 온다.
		"t8430",						// TR 번호
		&pckInBlock,
		sizeof(pckInBlock),		// InBlock 데이터 크기
		bNext,						// 다음조회 여부 : 다음조회일 경우에 세팅한다.
		szNextKey,					// 다음조회 Key : Header Type이 B 일 경우엔 이전 조회때 받은 Next Key를 넣어준다.
		30							// Timeout(초) : 해당 시간(초)동안 데이터가 오지 않으면 Timeout에 발생한다. XM_TIMEOUT_DATA 메시지가 발생한다.
	);

	if (nRqID < 0)
	{
		MessageBox("실패", "조회실패", MB_ICONSTOP);
	}

	FillMemory(&pckInBlock, sizeof(pckInBlock), ' ');

	SetPacketData(pckInBlock.gubun, sizeof(pckInBlock.gubun), "0", DATA_TYPE_STRING);	// 구분

	nRqID = g_iXingAPI.Request(
		GetSafeHwnd(),				// 데이터를 받을 윈도우, XM_RECEIVE_DATA 으로 온다.
		"t8430",						// TR 번호
		&pckInBlock,
		sizeof(pckInBlock),		// InBlock 데이터 크기
		bNext,						// 다음조회 여부 : 다음조회일 경우에 세팅한다.
		szNextKey,					// 다음조회 Key : Header Type이 B 일 경우엔 이전 조회때 받은 Next Key를 넣어준다.
		30							// Timeout(초) : 해당 시간(초)동안 데이터가 오지 않으면 Timeout에 발생한다. XM_TIMEOUT_DATA 메시지가 발생한다.
	);

	if (nRqID < 0)
	{
		MessageBox("실패", "조회실패", MB_ICONSTOP);
	}

}

LRESULT CebsetvcDlg::OnXMReceiveData(WPARAM wParam, LPARAM lParam)
{
	if (wParam == REQUEST_DATA)
	{
		LPRECV_PACKET pRpData = (LPRECV_PACKET)lParam;

		
		if (strcmp(pRpData->szTrCode, "t0424") == 0)
		{
			LPt0424OutBlock p0424OutBlock = (LPt0424OutBlock)pRpData->lpData;

			account_listctrl.InsertItem(0, "");

			CString available_money;
			CString total_purchase = GetDispData(p0424OutBlock[0].mamt, sizeof(p0424OutBlock[0].mamt), DATA_TYPE_LONG);
			CString total_eval = GetDispData(p0424OutBlock[0].tappamt, sizeof(p0424OutBlock[0].tappamt), DATA_TYPE_LONG);
			CString plval = GetDispData(p0424OutBlock[0].tdtsunik, sizeof(p0424OutBlock[0].tdtsunik), DATA_TYPE_LONG);
			CString pl_ratio;
			float ratio;
			if (atoi(total_purchase) != 0) ratio = (double)atoi(plval) / atoi(total_purchase) * 100;
			else ratio = 0;
			pl_ratio.Format("%.2f", ratio);

			CString amt = GetDispData(p0424OutBlock[0].sunamt, sizeof(p0424OutBlock[0].sunamt), DATA_TYPE_LONG);

			money.Format("%d", atoi(amt) - atoi(total_purchase));
			available_money = money;
			nwsWorkerChild.money = money;

			account_listctrl.SetItemText(0, 0, available_money);
			account_listctrl.SetItemText(0, 1, total_purchase);
			account_listctrl.SetItemText(0, 2, total_eval);
			account_listctrl.SetItemText(0, 3, plval);
			account_listctrl.SetItemText(0, 4, pl_ratio);
			account_listctrl.SetItemText(0, 5, amt);

			nwsWorkerChild.SendMessage(WM_USER + XM_CALLFROMPARENT, 0, 0);

		}
	}

	return 0L;
}

void CebsetvcDlg::OnTimer(UINT_PTR nIDEvent)
{
	if (nIDEvent == MAIN_TIMER)
	{
		CTime cTime = CTime::GetCurrentTime(); // 현재 시스템으로부터 날짜 및 시간을 얻어 온다.
		CString StrDate;

		StrDate.Format(_T("%04d/%02d/%02d %02d:%02d:%02d"),
			cTime.GetYear(), cTime.GetMonth(), cTime.GetDay(),
			cTime.GetHour(), cTime.GetMinute(), cTime.GetSecond());

		timer_static.SetWindowTextA(StrDate);

	}

	CDialogEx::OnTimer(nIDEvent);
}

void CebsetvcDlg::InquiryBalance()
{
	t0424InBlock pckInBlock;

	char szNextKey[] = "";

	FillMemory(&pckInBlock, sizeof(pckInBlock), ' ');

	SetPacketData(pckInBlock.accno, sizeof(pckInBlock.accno), account, DATA_TYPE_STRING);
	SetPacketData(pckInBlock.passwd, sizeof(pckInBlock.passwd), pswd, DATA_TYPE_STRING);
	SetPacketData(pckInBlock.prcgb, sizeof(pckInBlock.prcgb), "1", DATA_TYPE_STRING);
	SetPacketData(pckInBlock.chegb, sizeof(pckInBlock.chegb), "0", DATA_TYPE_STRING);
	SetPacketData(pckInBlock.dangb, sizeof(pckInBlock.dangb), "0", DATA_TYPE_STRING);
	SetPacketData(pckInBlock.charge, sizeof(pckInBlock.charge), "0", DATA_TYPE_STRING);
	SetPacketData(pckInBlock.cts_expcode, sizeof(pckInBlock.cts_expcode), " ", DATA_TYPE_STRING);

	int nRqID = g_iXingAPI.Request(
		GetSafeHwnd(),				// 데이터를 받을 윈도우, XM_RECEIVE_DATA 으로 온다.
		"t0424",						// TR 번호
		&pckInBlock,
		sizeof(pckInBlock),		// InBlock 데이터 크기
		FALSE,						// 다음조회 여부 : 다음조회일 경우에 세팅한다.
		szNextKey,					// 다음조회 Key : Header Type이 B 일 경우엔 이전 조회때 받은 Next Key를 넣어준다.
		30							// Timeout(초) : 해당 시간(초)동안 데이터가 오지 않으면 Timeout에 발생한다. XM_TIMEOUT_DATA 메시지가 발생한다.
	);

	account_listctrl.DeleteAllItems();

	if (nRqID < 0)
	{
		int nErrorCode = g_iXingAPI.GetLastError();
		CString strMsg = g_iXingAPI.GetErrorMessage(nErrorCode);
		MessageBox(strMsg, "계좌 조회 실패", MB_ICONSTOP);
	}
}

LRESULT CebsetvcDlg::OnReqBalance(WPARAM wParam, LPARAM lParam)
{
	InquiryBalance();

	return 0L;
}

void CebsetvcDlg::ChkServerConn() 
{

}


void CebsetvcDlg::OnClose()
{
	// Logout 한다
	//		굳이 응답을 받을 필요는 없다.
	//		Login을 하지 않은 상태에서 Logout을 하면 FALSE가 Return 되므로 Login 여부를 체크하지 않아도 된다.
	g_iXingAPI.Logout(GetSafeHwnd());

	nwsWorkerChild.DestroyWindow();

	CDialogEx::OnClose();
}