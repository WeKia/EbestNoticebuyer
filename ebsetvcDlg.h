
// ebsetvcDlg.h: 헤더 파일
//

#pragma once

#include "NWSWorker.h"

#define MAIN_TIMER 1000
#define INQUIRY_TIMER 1001

// CebsetvcDlg 대화 상자
class CebsetvcDlg : public CDialogEx
{
// 생성입니다.
public:
	CebsetvcDlg(CWnd* pParent = nullptr);	// 표준 생성자입니다.

	CString account;
	CString money;
	CString pswd;
	CStatic account_static;
	CStatic state_static;
	CStatic timer_static;
	CListCtrl account_listctrl;

	NWSWorker nwsWorkerChild;

// 대화 상자 데이터입니다.
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_EBSETVC_DIALOG };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 지원입니다.

// 구현입니다.
protected:
	BOOL ConnectServer();
	BOOL Login();

	void InitListCtrl();
	void SetAccountText();
	void GetCodeList(bool bNext);
	void InquiryBalance();
	void ChkServerConn();

	HICON m_hIcon;

	// 생성된 메시지 맵 함수
	virtual BOOL OnInitDialog();
	afx_msg void OnClose();
	afx_msg void OnPaint();
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg LRESULT OnLogin(WPARAM wParam, LPARAM lParam);
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg LRESULT OnDisconnect(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnReqBalance(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnXMReceiveData(WPARAM wParam, LPARAM lParam);
	DECLARE_MESSAGE_MAP()
};
