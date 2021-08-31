#include "pch.h"
#include "framework.h"
#include "ebsetvc.h"
#include "NWSWorker.h"
#include "afxdialogex.h"

#include "packet/SC0.h"
#include "packet/SC1.h"
#include "packet/S3_.h"
#include "packet/K3_.h"
#include "packet/CSPAT00700.h"
#include "packet/t1101.h"
#include "packet/NWS.h"
#include "packet/t0424.h"
#include "packet/t0167.h"
#include "packet/t8430.h"


BEGIN_MESSAGE_MAP(NWSWorker, CDialogEx)
	ON_MESSAGE(WM_USER + XM_RECEIVE_DATA, OnXMReceiveData)
	ON_MESSAGE(WM_USER + XM_RECEIVE_REAL_DATA, OnXMReceiveRealData)
	ON_MESSAGE(WM_USER + XM_CALLFROMPARENT, OnReceiveDataFromParent)
END_MESSAGE_MAP()

NWSWorker::NWSWorker(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_NWS_DIALOG, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);

}

void NWSWorker::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_NWS_LIST, nws_listctrl);
}

BOOL NWSWorker::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// 이 대화 상자의 아이콘을 설정합니다.  응용 프로그램의 주 창이 대화 상자가 아닐 경우에는
	//  프레임워크가 이 작업을 자동으로 수행합니다.
	SetIcon(m_hIcon, TRUE);			// 큰 아이콘을 설정합니다.
	SetIcon(m_hIcon, FALSE);		// 작은 아이콘을 설정합니다.

	nws_listctrl.InsertColumn(0, "시간", LVCFMT_LEFT, 90);
	nws_listctrl.InsertColumn(1, "뉴스 제목", LVCFMT_LEFT, 200);

	ScribeNWS();
	g_iXingAPI.AdviseRealData(GetSafeHwnd(), "SC0", "", 0);
	g_iXingAPI.AdviseRealData(GetSafeHwnd(), "SC1", "", 0);

	t8430InBlock pckInBlock;

	FillMemory(&pckInBlock, sizeof(pckInBlock), ' ');

	SetPacketData(pckInBlock.gubun, sizeof(pckInBlock.gubun), "0", DATA_TYPE_STRING);

	int i = g_iXingAPI.Request(GetSafeHwnd(), "t8430", &pckInBlock, sizeof(pckInBlock));

	if (i < 0)
	{
		MessageBox("조회실패", "에러", MB_ICONSTOP);
	}
	InitStock();
	InitBlock();

	return TRUE;  // 포커스를 컨트롤에 설정하지 않으면 TRUE를 반환합니다.
}

void NWSWorker::InitStock()
{
	buyProcess = TRUE;
	lastPrice = 0;
	stockQty = 0;
	buyPrice = 0;
	fastEMA = 0;
	offset = 0;
	orgordno = "";
	pswd = "1024";
	tickList.RemoveAll();
}

void NWSWorker::InitBlock()
{
	FillMemory(&BuyBlock, sizeof(BuyBlock), ' ');

	SetPacketData(BuyBlock.AcntNo, sizeof(BuyBlock.AcntNo), account, DATA_TYPE_STRING);
	SetPacketData(BuyBlock.InptPwd, sizeof(BuyBlock.InptPwd), pswd, DATA_TYPE_STRING);
	SetPacketData(BuyBlock.BnsTpCode, sizeof(BuyBlock.BnsTpCode), "2", DATA_TYPE_STRING);
	SetPacketData(BuyBlock.OrdprcPtnCode, sizeof(BuyBlock.OrdprcPtnCode), "00", DATA_TYPE_STRING);
	SetPacketData(BuyBlock.MgntrnCode, sizeof(BuyBlock.MgntrnCode), "000", DATA_TYPE_STRING);
	SetPacketData(BuyBlock.LoanDt, sizeof(BuyBlock.LoanDt), "        ", DATA_TYPE_STRING);
	SetPacketData(BuyBlock.OrdCndiTpCode, sizeof(BuyBlock.OrdCndiTpCode), "0", DATA_TYPE_STRING);

	FillMemory(&BuyBlock2, sizeof(BuyBlock), ' ');

	SetPacketData(BuyBlock2.AcntNo, sizeof(BuyBlock2.AcntNo), account, DATA_TYPE_STRING);
	SetPacketData(BuyBlock2.InptPwd, sizeof(BuyBlock2.InptPwd), pswd, DATA_TYPE_STRING);
	SetPacketData(BuyBlock2.BnsTpCode, sizeof(BuyBlock2.BnsTpCode), "2", DATA_TYPE_STRING);
	SetPacketData(BuyBlock2.OrdprcPtnCode, sizeof(BuyBlock2.OrdprcPtnCode), "00", DATA_TYPE_STRING);
	SetPacketData(BuyBlock2.MgntrnCode, sizeof(BuyBlock2.MgntrnCode), "000", DATA_TYPE_STRING);
	SetPacketData(BuyBlock2.LoanDt, sizeof(BuyBlock2.LoanDt), "        ", DATA_TYPE_STRING);
	SetPacketData(BuyBlock2.OrdCndiTpCode, sizeof(BuyBlock2.OrdCndiTpCode), "0", DATA_TYPE_STRING);
	SetPacketData(BuyBlock2.OrdPrc, sizeof(BuyBlock2.OrdPrc), "0", DATA_TYPE_FLOAT_DOT, 2);
	SetPacketData(BuyBlock2.OrdprcPtnCode, sizeof(BuyBlock2.OrdprcPtnCode), "03", DATA_TYPE_STRING);
}

LRESULT NWSWorker::OnXMReceiveData(WPARAM wParam, LPARAM lParam)
{
	if (wParam == REQUEST_DATA)
	{
		LPRECV_PACKET pRpData = (LPRECV_PACKET)lParam;

		if (strcmp(pRpData->szTrCode, "t8430") == 0)
		{
			if (strcmp(pRpData->szBlockName, NAME_t8430OutBlock) == 0)
			{
				LPt8430OutBlock p8430OutBlock = (LPt8430OutBlock)pRpData->lpData;

				int nCount = pRpData->nDataLength / sizeof(t8430OutBlock);

				for (int i = 0; i < nCount; i++)
				{
					CString code = GetDispData(p8430OutBlock[i].shcode, sizeof(p8430OutBlock[i].shcode), DATA_TYPE_STRING);
					CString gubun = GetDispData(p8430OutBlock[i].gubun, sizeof(p8430OutBlock[i].gubun), DATA_TYPE_STRING);
					CString upPrice = GetDispData(p8430OutBlock[i].uplmtprice, sizeof(p8430OutBlock[i].uplmtprice), DATA_TYPE_STRING);
					
					stockList.SetAt(code, atoi(upPrice));

					if (gubun == "1")
					{
						kospi_list.AddTail(code);
					}
					else
					{
						kosdaq_list.AddTail(code);
					}

				}
			}
		}
	}

	return 0L;
}


LRESULT NWSWorker::OnXMReceiveRealData(WPARAM wParam, LPARAM lParam)
{
	LPRECV_REAL_PACKET pRealPacket = (LPRECV_REAL_PACKET)lParam;

	if (strcmp(pRealPacket->szTrCode, "NWS") == 0)
	{
		LPNWS_OutBlock pOutBlock = (LPNWS_OutBlock)pRealPacket->pszData;

		CString title = GetDispData(pOutBlock->title, sizeof(pOutBlock->title), DATA_TYPE_STRING);
		CString code = GetDispData(pOutBlock->code, sizeof(pOutBlock->code), DATA_TYPE_STRING);
		code = code.Right(6);
		CString id = GetDispData(pOutBlock->id, sizeof(pOutBlock->id), DATA_TYPE_STRING);

		if (id == "15")
		{
			if ((title.Find("정정") == -1) && ((title.Find("계약체결") != -1) || (title.Find("특허") != -1)))
			{
				if (buyProcess)
				{
					buyProcess = FALSE;
					targetCorp = code;

					BuyStock(targetCorp);
				}

				nws_listctrl.InsertItem(0, "");

				nws_listctrl.SetItemText(0, 0, GetDispData(pOutBlock->time, sizeof(pOutBlock->time), DATA_TYPE_STRING));
				nws_listctrl.SetItemText(0, 1, title);

			}

		}
	}
	else if (strcmp(pRealPacket->szTrCode, "S3_") == 0)
	{
		LPS3__OutBlock pOutBlock = (LPS3__OutBlock)pRealPacket->pszData;

		CString price = GetDispData(pOutBlock->price, sizeof(pOutBlock->price), DATA_TYPE_LONG);
		CString volume = GetDispData(pOutBlock->cvolume, sizeof(pOutBlock->cvolume), DATA_TYPE_LONG);

		ChkStockSell(price, volume);
	}
	else if (strcmp(pRealPacket->szTrCode, "K3_") == 0)
	{
		LPK3__OutBlock pOutBlock = (LPK3__OutBlock)pRealPacket->pszData;

		CString price = GetDispData(pOutBlock->price, sizeof(pOutBlock->price), DATA_TYPE_LONG);
		CString volume = GetDispData(pOutBlock->cvolume, sizeof(pOutBlock->cvolume), DATA_TYPE_LONG);

		ChkStockSell(price, volume);
	}
	else if (strcmp(pRealPacket->szTrCode, "SC0") == 0)
	{
		LPSC0_OutBlock pOutBlock = (LPSC0_OutBlock)pRealPacket->pszData;

		CString type = GetDispData(pOutBlock->bnstp, sizeof(pOutBlock->bnstp), DATA_TYPE_STRING);

		if (type == "1")
		{
			orgordno = GetDispData(pOutBlock->orgordno, sizeof(pOutBlock->orgordno), DATA_TYPE_LONG);

			ScribeStock(targetCorp);
		}

	}
	else if (strcmp(pRealPacket->szTrCode, "SC1") == 0)
	{
		LPSC1_OutBlock pOutBlock = (LPSC1_OutBlock)pRealPacket->pszData;

		CString type = GetDispData(pOutBlock->bnstp, sizeof(pOutBlock->bnstp), DATA_TYPE_STRING);
		CString price = GetDispData(pOutBlock->execprc, sizeof(pOutBlock->execprc), DATA_TYPE_LONG);

		if (type == "2")
		{
			long offset = CalculateOffset(targetCorp, atoi(price));
			long target = atoi(price) * 1.008;

			SellStock(targetCorp, stockQty, target - target % offset, "00");

			lastPrice = atoi(price);
			buyPrice = lastPrice;
			fastEMA = lastPrice;
			slowEMA = lastPrice;
		}
		else
		{
			UnscribeStock(targetCorp);
			InitStock();
		}

		GetParent()->SendMessage(WM_USER + XM_GETBALANCE, 0, 0);
	}
	else
	{
		assert(lastPrice != 0);
		assert(stockQty != 0);
	}

	return 0L;
}

LRESULT NWSWorker::OnReceiveDataFromParent(WPARAM wParam, LPARAM lParam)
{
	

	return 0L;
}


void NWSWorker::ScribeNWS()
{
	TCHAR	szTrCode[] = "NWS";

	NWS_InBlock pckInBlock;

	FillMemory(&pckInBlock, sizeof(pckInBlock), ' ');

	SetPacketData(pckInBlock.nwcode, sizeof(pckInBlock.nwcode), "NWS001", DATA_TYPE_STRING);

	//-----------------------------------------------------------
	// 데이터 전송
	BOOL bSuccess = g_iXingAPI.AdviseRealData(
		GetSafeHwnd(),				// 데이터를 받을 윈도우, XM_RECEIVE_REAL_DATA 으로 온다.
		szTrCode,					// TR 번호
		(LPCTSTR)&pckInBlock,
		sizeof(pckInBlock)
	);

	//-----------------------------------------------------------
	// 에러체크
	if (bSuccess == FALSE)
	{
		MessageBox("조회실패", "에러", MB_ICONSTOP);
	}
}

void NWSWorker::UnscribeNWS()
{
	TCHAR	szTrCode[] = "NWS";

	NWS_InBlock pckInBlock;

	FillMemory(&pckInBlock, sizeof(pckInBlock), ' ');

	SetPacketData(pckInBlock.nwcode, sizeof(pckInBlock.nwcode), "NWS001", DATA_TYPE_STRING);

	//-----------------------------------------------------------
	// 데이터 전송
	BOOL bSuccess = g_iXingAPI.UnadviseRealData(
		GetSafeHwnd(),				// 데이터를 받을 윈도우, XM_RECEIVE_REAL_DATA 으로 온다.
		szTrCode,					// TR 번호
		(LPCTSTR)&pckInBlock,
		sizeof(pckInBlock)
	);

	//-----------------------------------------------------------
	// 에러체크
	if (bSuccess == FALSE)
	{
		MessageBox("조회실패", "에러", MB_ICONSTOP);
	}
}


void NWSWorker::ChkStockSell(CString price, CString volume)
{
	assert(lastPrice != 0);
	assert(stockQty != 0);

	double fastAlpha = 1 / 5.;
	double slowAlpha = 1 / 10.;

	double tickAvg = 0;

	long lPrice = atoi(price);

	if (((double)lPrice - (double)buyPrice) / buyPrice <= -0.005)
	{
		ReOrder(targetCorp, stockQty, 0, "03");
		return;
	}

	tickData tick;
	tick.price = lPrice;
	tick.volume = atoi(volume);

	tickList.AddTail(tick);

	if (tickList.GetSize() >= 5)
	{
		int totalVol = 0;
		long totalPrice = 0;
		for (int i = 0; i < 5; i++)
		{
			tickData data = tickList.GetHead();
			totalPrice += data.price * data.volume;
			totalVol += data.volume;
			tickList.RemoveHead();
		}

		tickAvg = (double)totalPrice / totalVol;

		fastEMA = (1 - fastAlpha) * fastEMA + (fastAlpha)*tickAvg;
		slowEMA = (1 - slowAlpha) * slowEMA + (slowAlpha)*tickAvg;

		if (fastEMA < slowEMA && tickAvg < fastEMA)
		{
			ReOrder(targetCorp, stockQty, 0, "03");
		}
	}
}

void NWSWorker::ScribeStock(CString code)
{
	CString	szTrCode;


	BOOL bSuccess1 = g_iXingAPI.AdviseRealData(GetSafeHwnd(), "S3_", code, code.GetLength());
	BOOL bSuccess2 = g_iXingAPI.AdviseRealData(GetSafeHwnd(), "K3_", code, code.GetLength());

	if ((bSuccess1 || bSuccess2) == FALSE)
	{
		MessageBox("조회실패", "에러", MB_ICONSTOP);
	}

}

void NWSWorker::UnscribeStock(CString code)
{

	BOOL bSuccess1 = g_iXingAPI.UnadviseRealData(GetSafeHwnd(), "S3_", code, code.GetLength());
	BOOL bSuccess2 = g_iXingAPI.UnadviseRealData(GetSafeHwnd(), "K3_", code, code.GetLength());

	//-----------------------------------------------------------
	// 에러체크
	if ((bSuccess1 || bSuccess2) == FALSE)
	{
		MessageBox("조회실패", "에러", MB_ICONSTOP);
	}
}

void NWSWorker::SellStock(CString code, long qty, long price, CString type)
{
	CString strQty;
	CString strPrice;

	strPrice.Format("%d", price);
	strQty.Format("%d", qty);

	CSPAT00600InBlock1 pckInBlock;

	char szNextKey[] = "";

	FillMemory(&pckInBlock, sizeof(pckInBlock), ' ');

	SetPacketData(pckInBlock.AcntNo, sizeof(pckInBlock.AcntNo), account, DATA_TYPE_STRING);
	SetPacketData(pckInBlock.InptPwd, sizeof(pckInBlock.InptPwd), pswd, DATA_TYPE_STRING);
	SetPacketData(pckInBlock.IsuNo, sizeof(pckInBlock.IsuNo), code, DATA_TYPE_STRING);
	SetPacketData(pckInBlock.OrdQty, sizeof(pckInBlock.OrdQty), strQty, DATA_TYPE_LONG);
	SetPacketData(pckInBlock.OrdPrc, sizeof(pckInBlock.OrdPrc), strPrice, DATA_TYPE_FLOAT_DOT, 2);
	SetPacketData(pckInBlock.BnsTpCode, sizeof(pckInBlock.BnsTpCode), "1", DATA_TYPE_STRING);
	SetPacketData(pckInBlock.OrdprcPtnCode, sizeof(pckInBlock.OrdprcPtnCode), type, DATA_TYPE_STRING);
	SetPacketData(pckInBlock.MgntrnCode, sizeof(pckInBlock.MgntrnCode), "000", DATA_TYPE_STRING);
	SetPacketData(pckInBlock.LoanDt, sizeof(pckInBlock.LoanDt), "        ", DATA_TYPE_STRING);
	SetPacketData(pckInBlock.OrdCndiTpCode, sizeof(pckInBlock.OrdCndiTpCode), "0", DATA_TYPE_STRING);

	int nRqID = g_iXingAPI.Request(
		GetSafeHwnd(),				// 데이터를 받을 윈도우, XM_RECEIVE_DATA 으로 온다.
		"CSPAT00600",						// TR 번호
		&pckInBlock,
		sizeof(pckInBlock),		// InBlock 데이터 크기
		FALSE,						// 다음조회 여부 : 다음조회일 경우에 세팅한다.
		szNextKey,					// 다음조회 Key : Header Type이 B 일 경우엔 이전 조회때 받은 Next Key를 넣어준다.
		30							// Timeout(초) : 해당 시간(초)동안 데이터가 오지 않으면 Timeout에 발생한다. XM_TIMEOUT_DATA 메시지가 발생한다.
	);

	if (nRqID < 0)
	{
		int nErrorCode = g_iXingAPI.GetLastError();
		CString strMsg = g_iXingAPI.GetErrorMessage(nErrorCode);
		MessageBox(strMsg, "매도 실패", MB_ICONSTOP);
	}
}

BOOL NWSWorker::BuyStock(CString code, long price)
{
	CString strQty;
	CString strPrice;
	long qty = long((double)atoi(money) / price);

	if (qty <= 0)
	{
		InitStock();
		return FALSE;
	}

	stockQty = qty;

	strQty.Format("%d", qty);
	strPrice.Format("%d", price);

	SetPacketData(BuyBlock.IsuNo, sizeof(BuyBlock.IsuNo), code, DATA_TYPE_STRING);
	SetPacketData(BuyBlock.OrdQty, sizeof(BuyBlock.OrdQty), strQty, DATA_TYPE_LONG);
	SetPacketData(BuyBlock.OrdPrc, sizeof(BuyBlock.OrdPrc), strPrice, DATA_TYPE_FLOAT_DOT, 2);
	SetPacketData(BuyBlock.OrdprcPtnCode, sizeof(BuyBlock.OrdprcPtnCode), "00", DATA_TYPE_STRING);

	int nRqID = g_iXingAPI.Request(
		GetSafeHwnd(),				// 데이터를 받을 윈도우, XM_RECEIVE_DATA 으로 온다.
		"CSPAT00600",						// TR 번호
		&BuyBlock,
		sizeof(BuyBlock)		// InBlock 데이터 크기
	);

	if (nRqID < 0)
	{
		int nErrorCode = g_iXingAPI.GetLastError();
		CString strMsg = g_iXingAPI.GetErrorMessage(nErrorCode);
		MessageBox(strMsg, "매수 실패", MB_ICONSTOP);
		return FALSE;
	}

	return TRUE;
}


BOOL NWSWorker::BuyStock(CString code)
{
	CString strQty;

	long price;

	stockList.Lookup(code, price);

	long qty = long((double)atoi(money) / price);

	if (qty <= 0)
	{
		InitStock();
		return FALSE;
	}

	stockQty = qty;

	strQty.Format("%d", qty);

	SetPacketData(BuyBlock2.IsuNo, sizeof(BuyBlock2.IsuNo), code, DATA_TYPE_STRING);
	SetPacketData(BuyBlock2.OrdQty, sizeof(BuyBlock2.OrdQty), strQty, DATA_TYPE_LONG);

	int nRqID = g_iXingAPI.Request(
		GetSafeHwnd(),				// 데이터를 받을 윈도우, XM_RECEIVE_DATA 으로 온다.
		"CSPAT00600",						// TR 번호
		&BuyBlock2,
		sizeof(BuyBlock2)		// InBlock 데이터 크기
	);

	if (nRqID < 0)
	{
		int nErrorCode = g_iXingAPI.GetLastError();
		CString strMsg = g_iXingAPI.GetErrorMessage(nErrorCode);
		MessageBox(strMsg, "매수 실패", MB_ICONSTOP);
		return FALSE;
	}

	return TRUE;
}

void NWSWorker::ReOrder(CString code, long qty, long price, CString type)
{
	assert(orgordno != "");

	CString strQty;
	CString strPrice;

	strPrice.Format("%d", price);
	strQty.Format("%d", qty);

	CSPAT00700InBlock1 pckInBlock;

	char szNextKey[] = "";

	FillMemory(&pckInBlock, sizeof(pckInBlock), ' ');

	SetPacketData(pckInBlock.OrgOrdNo, sizeof(pckInBlock.OrgOrdNo), orgordno, DATA_TYPE_STRING);
	SetPacketData(pckInBlock.AcntNo, sizeof(pckInBlock.AcntNo), account, DATA_TYPE_STRING);
	SetPacketData(pckInBlock.InptPwd, sizeof(pckInBlock.InptPwd), pswd, DATA_TYPE_STRING);
	SetPacketData(pckInBlock.IsuNo, sizeof(pckInBlock.IsuNo), code, DATA_TYPE_STRING);
	SetPacketData(pckInBlock.OrdQty, sizeof(pckInBlock.OrdQty), strQty, DATA_TYPE_LONG);
	SetPacketData(pckInBlock.OrdPrc, sizeof(pckInBlock.OrdPrc), strPrice, DATA_TYPE_FLOAT_DOT, 2);
	SetPacketData(pckInBlock.OrdprcPtnCode, sizeof(pckInBlock.OrdprcPtnCode), type, DATA_TYPE_STRING);
	SetPacketData(pckInBlock.OrdCndiTpCode, sizeof(pckInBlock.OrdCndiTpCode), "0", DATA_TYPE_STRING);

	int nRqID = g_iXingAPI.Request(
		GetSafeHwnd(),				// 데이터를 받을 윈도우, XM_RECEIVE_DATA 으로 온다.
		"CSPAT00700",						// TR 번호
		&pckInBlock,
		sizeof(pckInBlock),		// InBlock 데이터 크기
		FALSE,						// 다음조회 여부 : 다음조회일 경우에 세팅한다.
		szNextKey,					// 다음조회 Key : Header Type이 B 일 경우엔 이전 조회때 받은 Next Key를 넣어준다.
		30							// Timeout(초) : 해당 시간(초)동안 데이터가 오지 않으면 Timeout에 발생한다. XM_TIMEOUT_DATA 메시지가 발생한다.
	);

	if (nRqID < 0)
	{
		int nErrorCode = g_iXingAPI.GetLastError();
		CString strMsg = g_iXingAPI.GetErrorMessage(nErrorCode);
		MessageBox(strMsg, "주문 정정 실패", MB_ICONSTOP);
	}
}


long NWSWorker::CalculateOffset(CString  code, long price)
{
	if (price < 1000) return 1;
	else if (price < 5000) return 5;
	else if (price < 10000) return 10;
	else if (price < 50000) return 50;

	if (kosdaq_list.Find(code) != NULL) return 100;

	if (price < 100000) return 100;
	else if (price < 500000) return 500;

	return 1000;
}