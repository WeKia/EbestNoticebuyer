#pragma once

#include "packet/CSPAT00600.h"

typedef struct _tickdata {
	long price;
	long volume;
} tickData;


class NWSWorker : public CDialogEx
{
public:
	NWSWorker(CWnd* pParent = nullptr);	// 표준 생성자입니다.

	CList<tickData>tickList;
	CMap<CString, LPCSTR, long, long> stockList;

	CListCtrl nws_listctrl;

	CString account;
	CString money;
	CString targetCorp;
	CString pswd;
	CString orgordno;

	CStringList kospi_list;
	CStringList kosdaq_list;

	CSPAT00600InBlock1 BuyBlock;
	CSPAT00600InBlock1 BuyBlock2;

	BOOL buyProcess;
	long lastPrice;
	long stockQty;
	long buyPrice;
	long offset;
	
	double fastEMA, slowEMA;

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 지원입니다.

protected:
	HICON m_hIcon;

	// 생성된 메시지 맵 함수
	virtual BOOL OnInitDialog();

	void InitStock();
	void InitBlock();
	void ScribeNWS();
	void UnscribeNWS();
	BOOL BuyStock(CString code, long price);
	BOOL BuyStock(CString code);
	long CalculateOffset(CString  code, long price);
	void ScribeStock(CString code);
	void UnscribeStock(CString code);
	void ChkStockSell(CString price, CString volume);
	void SellStock(CString code, long qty, long price, CString type);
	void ReOrder(CString code, long qty, long price, CString type);

	afx_msg LRESULT OnXMReceiveData(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnXMReceiveRealData(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnReceiveDataFromParent(WPARAM wParam, LPARAM lParam);

	DECLARE_MESSAGE_MAP()
};

