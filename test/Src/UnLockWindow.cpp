#if 0

#include "UnLockWindow.h"
#include "Base/Base.h"
#include <GdiPlus.h>
#include "JSON/json.h"
using namespace Gdiplus;
#define WM_ON_DESTROY				WM_USER + 1
#define WM_ON_PAINT					WM_USER + 2
#define DF_BTN_ID					0
#define DF_BTN_CLOSE				DF_BTN_ID + 1
#define DF_BTN_OK					DF_BTN_ID + 2
#define DF_BTN_CANCEL				DF_BTN_ID + 3

#define IDB_BACKGROUND                  101
#define IDB_BTN_CLOSE_NORMAL            102
#define IDB_BTN_CLOSE_FOCUS             103
#define IDB_BTN_NORMAL                  104
#define IDB_BTN_FOCUS                   105
#define IDB_BTN_SELECTED                106
#define IDB_ICON_LOCK                   108
#define IDB_EDIT_BACKGROUND             109
#define IDB_BACKGOUND_GROUP             110

std::wstring c2w(const char* str)
{
	int length = strlen(str);
	size_t conver = 0;
	WCHAR *wszText = (WCHAR*)calloc(length + 1, sizeof(WCHAR));
	if (str)
	{
		int n = MultiByteToWideChar(CP_ACP, 0, str, length, NULL, NULL);
		if (n > length)
			n = length;

		MultiByteToWideChar(CP_ACP, 0, str, length, wszText, n);
}
	std::wstring strText = wszText;
	free(wszText);
	return strText;
}

#define VALUEINVALID(VALUE, INVALID, RETURNVALUE) if (VALUE == INVALID) return RETURNVALUE
#ifdef _DEBUG
#define EXENAME	"PlayerSdk_debug.dll"
#else
#define EXENAME "PlayerSdk.dll"
#endif

bool g_isCompatibleBitmap;
class Windows
{
	Windows() { g_isCompatibleBitmap = true; }

public:
	virtual ~Windows() { }

	static Windows* instance()
	{
		static Windows inst;
		return &inst;
	}

	void addWindow(HWND hWnd, WindowsBase* window)
	{
		Guard locker(mutex);
		mWindows.insert(std::make_pair(hWnd, window));
	}

	void removeWindow(HWND hWnd, WindowsBase* window)
	{
		Guard locker(mutex);
		mWindows.erase(hWnd);
	}

	WindowsBase* getWindow(HWND hWnd)
	{
		Guard locker(mutex);
		std::map<HWND, WindowsBase*>::iterator it = mWindows.find(hWnd);
		if (it == mWindows.end())
		{
			return NULL;
		}
		return it->second;
	}
	static std::map<HWND, WindowsBase*> mWindows;
	static Mutex mutex;
};

std::map<HWND, WindowsBase*> Windows::mWindows;
Mutex Windows::mutex;


bool GetCompatibleBitmap()
{
	return g_isCompatibleBitmap;
}

void SetCompatibleBitmap(bool isCompatibleBitmap)
{
	g_isCompatibleBitmap = isCompatibleBitmap;
}

HBITMAP CreateMemryBitmap(HDC hDC, int width, int height)
{
	SetMapMode(hDC, MM_TEXT);
	SetBkMode(hDC, TRANSPARENT);
	HBITMAP hBitmap = NULL;
	if (GetCompatibleBitmap())
	{
		hBitmap = CreateCompatibleBitmap(hDC, width, height);
		if (hBitmap != NULL)
		{
			return hBitmap;
		}
	}

	BITMAPINFO bmi;
	ZeroMemory(&bmi.bmiHeader, sizeof(BITMAPINFOHEADER));
	bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bmi.bmiHeader.biWidth = width;
	bmi.bmiHeader.biHeight = -height;
	bmi.bmiHeader.biPlanes = 1;
	bmi.bmiHeader.biCompression = BI_RGB;
	bmi.bmiHeader.biBitCount = 32;
	hBitmap = CreateDIBSection(hDC, &bmi, DIB_RGB_COLORS,
		NULL, NULL, 0);
	return hBitmap;
}

/// 加载PNG格式资源文件
BOOL LoadImageFromIDResource(UINT uID, LPCTSTR sTR, shared_ptr<Image>& pImg)
{
	//AFX_MANAGE_STATE(AfxGetStaticModuleState());
	HINSTANCE hInst = GetModuleHandle(EXENAME);
	HRSRC hRsrc = ::FindResource(hInst, MAKEINTRESOURCE(uID), sTR);
	if (!hRsrc)
	{
		return FALSE;
	}
	DWORD len = SizeofResource(hInst, hRsrc);
	BYTE* lpRsrc = (BYTE*)LoadResource(hInst, hRsrc);
	if (!lpRsrc)
	{
		return FALSE;
	}

	HGLOBAL m_hMem = GlobalAlloc(GMEM_FIXED, len);
	BYTE* pmem = (BYTE*)GlobalLock(m_hMem);
	memcpy(pmem, lpRsrc, len);
	GlobalUnlock(m_hMem);
	IStream *pstm;
	HRESULT hr = CreateStreamOnHGlobal(m_hMem, TRUE, &pstm);
	pImg = std::shared_ptr<Image>(Gdiplus::Image::FromStream(pstm));
	pstm->Release();
	FreeResource(lpRsrc);
	return TRUE;
}

void setAlpha(HDC hDC, int width, int height, int alpha)	// 设置透明通道
{
	HDC hCompatibleDC = CreateCompatibleDC(hDC);
	HBITMAP hBitmap = CreateMemryBitmap(hDC, width, height);
	HBITMAP hOldBitmap = (HBITMAP)SelectObject(hCompatibleDC, hBitmap);
	Graphics g(hCompatibleDC);
	////g.Clear(Color(0xFF, 0xFF, 0xFF, 0xFF));
	LinearGradientBrush linGrBrush(Point(0, 0), Point(0, height), Color(alpha, 0, 0, 0), Color(alpha, 0, 0, 0));
	Color colors[] = { Color(alpha, 0, 0, 0), Color(alpha, 0, 0, 0), Color(alpha, 0, 0, 0), Color(alpha, 0, 0, 0) };
	REAL positions[] = { 0.0f, 1.0f };
	linGrBrush.SetInterpolationColors(colors, positions, 2);
	g.FillRectangle(&linGrBrush, 0, 0, width, height);
	BitBlt(hDC, 0, 0, width, height, hCompatibleDC, 0, 0, SRCPAINT);
	SelectObject(hCompatibleDC, hOldBitmap);
	DeleteObject(hBitmap);
	DeleteDC(hCompatibleDC);
}

int GetWidth(RECT &rt)
{
	return rt.right - rt.left;
}

int GetHeight(RECT &rt)
{
	return rt.bottom - rt.top;
}

bool isInRetangle(POINT *lpPoint, RECT *lpRect)
{
	if (lpPoint == NULL || lpRect == NULL)
	{
		return false;
	}

	if (lpPoint->x >= lpRect->left && lpPoint->x <= lpRect->right && lpPoint->y >= lpRect->top && lpPoint->y <= lpRect->bottom)
	{
		return true;
	}
	return false;
}

WindowsBase::WindowsBase(HWND hWnd)
	: _hWnd(hWnd)
{
}

WindowsBase::WindowsBase()
{

}

WindowsBase::~WindowsBase()
{

}

ULONG_PTR token;
Gdiplus::GdiplusStartupInput input;

bool WindowsBase::init()
{
	Gdiplus::GdiplusStartup(&token, &input, NULL);
	return true;
}

bool WindowsBase::unint()
{
	return false;
}

bool WindowsBase::create(DWORD dwExStyle, LPCTSTR lpClassName, LPCTSTR lpWindowName, 
	DWORD dwStyle, int x, int y, int nWidth, int nHeight, HWND hWndParent,
	HMENU hMenu, HINSTANCE hInstance, LPVOID lpParam)
{
	CREATESTRUCT cs;
	cs.cx = nWidth; cs.cy = nHeight; cs.dwExStyle = dwExStyle;
	cs.hInstance = hInstance; cs.hMenu = hMenu; cs.hwndParent = hWndParent;
	cs.lpCreateParams = lpParam; cs.lpszClass = lpClassName; cs.lpszName = lpWindowName;
	cs.style = dwStyle; cs.x = x; cs.y = y;
	if (!onPreCreateWindow(cs))
	{
		logerror("WindowsBase::create() fail! file:%s line:%d\r\n", __FILE__, __LINE__);
		return false;
	}

	_hWnd = CreateWindowEx(dwExStyle, lpClassName, lpWindowName,
		dwStyle, x, x, nWidth, nHeight, hWndParent, hMenu, hInstance, lpParam);
	if (_hWnd == NULL)
	{
		logerror("WindowsBase::create() fail! error:%d file:%s line:%d\r\n", GetLastError(), __FILE__, __LINE__);
		return false;
	}
	Windows::instance()->addWindow(_hWnd, this);
	return true;
}

bool WindowsBase::destroy()
{
	VALUEINVALID(_hWnd, NULL, false);
	PostMessage(_hWnd, WM_ON_DESTROY, NULL, NULL);
	return true;
}

bool WindowsBase::show(DWORD dwShowStyle)
{
	PostMessage(_hWnd, WM_ON_PAINT, NULL, NULL);
	VALUEINVALID(_hWnd, NULL, false);
	return ShowWindow(_hWnd, dwShowStyle) == TRUE;
}

bool WindowsBase::center()
{
	VALUEINVALID(_hWnd, NULL, false);
	int cx = GetSystemMetrics(SM_CXSCREEN);
	int cy = GetSystemMetrics(SM_CYSCREEN);
	RECT rt = {0};
	if (!GetClientRect(_hWnd, &rt))
	{
		logerror("%s fail! error:%d file:%s line:%d\r\n", __FUNCTION__, GetLastError(), __FILE__, __LINE__);
		return false;
	}
	int nWidth = rt.right - rt.left;
	int nHeight = rt.bottom - rt.top;
	if (!MoveWindow(_hWnd, cx / 2 - nWidth / 2, cy / 2 - nHeight / 2, nWidth, nHeight, FALSE) == TRUE)
	{
		logerror("%s fail! file:%s line:%d\r\n", __FUNCTION__, GetLastError(), __FILE__, __LINE__);
		return false;
	}
	
	return true;
}

bool WindowsBase::msgloop()
{
	VALUEINVALID(_hWnd, NULL, false);
	MSG msg;
	while (GetMessage(&msg, NULL, NULL, NULL) == TRUE)
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	return true;
}

WNDPROC WindowsBase::DefualtWindowProc()
{
	return MyWindowProc;
}

HWND WindowsBase::getHWnd()
{
	return _hWnd;
}

LRESULT WindowsBase::MyWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	WindowsBase *pWindowsBase = Windows::instance()->getWindow(hwnd);
	if (pWindowsBase != NULL)
	{
		pWindowsBase->WindowProc(hwnd, uMsg, wParam, lParam);
	}	
	return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

LRESULT WindowsBase::WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_ON_DESTROY:
		onDestroy();
		break;
	case WM_NCHITTEST:
		return HTCAPTION;
		break;
	default:
		message(uMsg, wParam, lParam);
		break;
	}
	return NULL;
}

void WindowsBase::onPaint()
{
	HDC hDC = GetDC(_hWnd);
	RECT rt;
	GetClientRect(_hWnd, &rt);
	HDC hCompatibleDC = CreateCompatibleDC(hDC);
	HBITMAP hBitmap = CreateMemryBitmap(hDC, GetWidth(rt), GetHeight(rt));
	HBITMAP hOldBitmap = (HBITMAP)SelectObject(hCompatibleDC, hBitmap);
	Graphics g(hCompatibleDC);

	/// 背景

	SolidBrush brush(Color(100, 255, 255, 255));
	g.FillRectangle(&brush, Rect(rt.left, rt.top, rt.right - rt.left, rt.bottom - rt.top));
//	shared_ptr<Image> img_shaed;
//	LoadImageFromIDResource(IDB_BACKGROUND, "PNG", img_shaed);
//	g.DrawImage(img_shaed.get(), 0, 0, GetWidth(rt), GetHeight(rt));


//	shared_ptr<Image> img_backgound_group;
//	LoadImageFromIDResource(IDB_BACKGOUND_GROUP, "PNG", img_backgound_group);
//	g.DrawImage(img_backgound_group.get(), 20, 26, img_backgound_group->GetWidth(), img_backgound_group->GetHeight());

	RECT rtWindows;
	GetWindowRect(_hWnd, &rtWindows);
	POINT ptWinPos = { rtWindows.left, rtWindows.top };
	SIZE sizeWindow = { GetWidth(rtWindows), GetHeight(rtWindows) };
	POINT ptSrc = { 0, 0 };
	BLENDFUNCTION blend = { AC_SRC_OVER, 0, 255, AC_SRC_ALPHA };
	if (!::UpdateLayeredWindow(_hWnd, hDC, &ptWinPos, &sizeWindow, hCompatibleDC, &ptSrc, 0, &blend, ULW_ALPHA))
	{
		SetCompatibleBitmap(false);
	}

	//BitBlt(hDC, 0, 0, rt.right - rt.left, rt.bottom - rt.top, hCompatibleDC, 0, 0, SRCCOPY);
	SelectObject(hCompatibleDC, hOldBitmap);
	DeleteObject(hBitmap);
	DeleteDC(hCompatibleDC);
}

void WindowsBase::message(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_ON_PAINT:
		onPaint();
		break;
	default:
		break;
	}
}

bool WindowsBase::onPreCreateWindow(CREATESTRUCT &cs)
{
	WNDCLASSEX clsex;
	clsex.cbSize = sizeof(WNDCLASSEX);
	clsex.cbWndExtra = clsex.cbClsExtra = 0;
	clsex.hbrBackground = NULL;
	clsex.hCursor = LoadCursor(NULL, IDC_ARROW);
	clsex.hIcon = clsex.hIconSm = LoadIcon(NULL, IDI_APPLICATION);
	clsex.hInstance = cs.hInstance;
	clsex.lpfnWndProc = DefualtWindowProc();
	clsex.lpszClassName = cs.lpszClass;
	clsex.lpszMenuName = NULL;
	clsex.style = CS_VREDRAW | CS_HREDRAW;
	if (!RegisterClassEx(&clsex))
	{
		if (GetLastError() == ERROR_CLASS_ALREADY_EXISTS)
		{
			return true;
		}
		logerror("RegisterClassEx() fail! file:%s line:%d err:%d\r\n", __FILE__, __LINE__, GetLastError());
		return false;
	}
	return true;
}

bool WindowsBase::onAfterDestroyWindow()
{
	return true;
}

void WindowsBase::onDestroy()
{
	PostMessage(_hWnd, WM_QUIT, NULL, NULL);
	DestroyWindow(_hWnd);
	_hWnd = NULL;
	onAfterDestroyWindow();
	Windows::instance()->removeWindow(_hWnd, this);
}

Control::Control()
{

}

Control::~Control()
{

}

void Control::message(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
}

bool Control::isStatic()
{
	return false;
}

UIWidget::UIWidget()
{
	memset(&rtClient, 0, sizeof(RECT));
}

UIWidget::~UIWidget()
{

}

bool UIWidget::create(int x, int y, int width, int height, unsigned int id)
{
	if (!isValidWindow(x, y, width, height, rtClient))
	{
		return false;
	}
	cid = id;
	return true;
}

bool UIWidget::destroy()
{
	return true;
}

void UIWidget::onPaint(HDC hDC)
{
	Graphics g(hDC);
	SolidBrush brush(Color(0xFF, 0, 0, 0));
	RECT rt;
	getClientRect(&rt);
	Rect fillRect(rt.left, rt.top, GetWidth(rt), GetHeight(rt));
	g.FillRectangle(&brush, fillRect);
}

bool UIWidget::moveWindow(int x, int y, int width, int height)
{
	if (!isValidWindow(x, y, width, height, rtClient))
	{
		return false;
	}
	return true;
}

bool UIWidget::getClientRect(RECT *lpRect)
{
	*lpRect = rtClient;
	return true;
}

unsigned int UIWidget::getID()
{
	return cid;
}

bool UIWidget::isValidWindow(int x, int y, int width, int height, RECT &rt)
{
	RECT rtTemp;
	rtTemp.left = x;
	rtTemp.top = y;
	rtTemp.right = rtTemp.left + width;
	rtTemp.bottom = rtTemp.top + height;
	if (rtTemp.right <= rtTemp.left || rtTemp.bottom <= rtTemp.top)
	{
		return false;
	}
	rt = rtTemp;
	return true;
}

UIButton::UIButton()
	: _status(BTN_NORMAL)
{
}

UIButton::~UIButton()
{

}

bool UIButton::setStatusResourceID(ButtonStatus status, DWORD dwResourceID)
{
	Guard locker(mutex);
	mResource.insert(std::make_pair(status, dwResourceID));
	return true;
}

void UIButton::onPaint(HDC hDC)
{
	DWORD sourceid = 0;
	{
		Guard locker(mutex);
		std::map<ButtonStatus, DWORD>::iterator it = mResource.find(_status);
		if (it == mResource.end())
		{
			return;
		}
		sourceid = it->second;
	}
	Graphics g(hDC);
	shared_ptr<Image> img;
	if (!LoadImageFromIDResource(sourceid, "PNG", img))
	{
		logerror("LoadImageFromIDResource() fail! file:%s line:%d\r\n", __FILE__, __LINE__);
		return;
	}
	RECT rt;
	getClientRect(&rt);
	g.DrawImage(img.get(), rt.left, rt.top, img->GetWidth(), img->GetHeight());
}

void UIButton::setStatus(ButtonStatus status)
{
	_status = status;
}

UIButton::ButtonStatus UIButton::getStatus()
{
	return _status;
}


UILabel::UILabel()
	: fontSize(10)
	, fontFamily("微软雅黑")
	, fontStyle(FontStyleRegular)
	, dwColor(RGB(0xFF, 0xFF, 0xFF))
{

}

UILabel::~UILabel()
{

}

void UILabel::setText(const std::string &string)
{
	text = string;
}

void UILabel::setFontSize(unsigned int size)
{
	fontSize = size;
}

void UILabel::setFontFamily(const std::string &family)
{
	fontFamily = family;
}

void UILabel::setFontStyle(unsigned int style)
{
	fontStyle = style;
}

void UILabel::setColor(unsigned int color)
{
	dwColor = color;
}

void UILabel::onPaint(HDC hDC)
{
	RECT rt;
	getClientRect(&rt);
	Graphics g(hDC);
	g.SetSmoothingMode(SmoothingModeAntiAlias);
	g.SetInterpolationMode(InterpolationModeHighQualityBicubic);
	wstring strWide = c2w(text.c_str());
	FontFamily *pFamily = NULL;
	int y = rt.top;
	pFamily = &FontFamily(c2w(fontFamily.c_str()).c_str());
	if (pFamily->GetLastStatus() != Gdiplus::Ok)
	{
		y += 3;
		pFamily = &FontFamily(L"宋体");
	}

	SolidBrush fontbrush(Color(GetRValue(dwColor), GetBValue(dwColor), GetGValue(dwColor)));
	Gdiplus::Font font(pFamily, (REAL)fontSize, fontStyle);
	RectF rtString;
	rtString.X = (REAL)rt.left;
	rtString.Y = (REAL)rt.top;
	rtString.Width = (REAL)GetWidth(rt);
	rtString.Height = (REAL)GetHeight(rt);
	StringFormat format;
	g.DrawString(strWide.c_str(), strWide.length(), &font, rtString, &format, &fontbrush);
}

bool UILabel::isStatic()
{
	return true;
}

UIEdit::UIEdit()
{
	str.reserve(64);
}

UIEdit::~UIEdit()
{

}

void DrawRoundRectange(Graphics &g, Color &pens, int x, int y, int width, int height, int offset)
{
	g.SetSmoothingMode(SmoothingModeAntiAlias);
	Pen pen(pens, 1);
	g.DrawLine(&pen, x + offset, y, width + offset, y);
	g.DrawLine(&pen, x + offset, y + height, width + offset, y + height);
	g.DrawArc(&pen, x, y, offset * 2, offset * 2, 180, 90);
	g.DrawArc(&pen, x + width - offset * 2, y + height - offset * 2, offset * 2, offset * 2, 360, 90);
	g.DrawArc(&pen, x + width - offset * 2, y, offset * 2, offset * 2, 270, 90);
	g.DrawArc(&pen, x, y + height - offset * 2, offset * 2, offset * 2, 90, 90);
}

bool UIEdit::create(HWND hWnd, int x, int y, int width, int height, unsigned int id)
{
	if (!UIWidget::create(x, y, width, height, id))
	{
		return false;
	}
	label = std::make_shared<UILabel>();
	label->create(x, y, width, height, id);
	label->setFontFamily("微软雅黑");
	label->setFontSize(9);
	label->setText(str);
	label->setColor(RGB(0, 0, 0));
	hMainWnd = hWnd;
	return true;
}

std::string UIEdit::getText()
{
	return str;
}

void UIEdit::setText(const std::string strText)
{
	str = strText;
}

void UIEdit::onPaint(HDC hDC)
{
	RECT rt;
	getClientRect(&rt);
	HDC hCompatibleDC = CreateCompatibleDC(hDC);
	HBITMAP hBitmap = CreateMemryBitmap(hDC, GetWidth(rt) * 2, GetHeight(rt));
	HBITMAP hOldBitmap = (HBITMAP)SelectObject(hCompatibleDC, hBitmap);

	Graphics g(hDC);
	g.SetSmoothingMode(SmoothingModeHighQuality);
	g.SetInterpolationMode(InterpolationModeHighQualityBicubic);

	
	shared_ptr<Image> imgbackground;
	if (!LoadImageFromIDResource(IDB_EDIT_BACKGROUND, "PNG", imgbackground))
	{
		logerror("LoadImageFromIDResource() fail! file:%s line:%d\r\n", __FILE__, __LINE__);
		return;
	}

	g.DrawImage(imgbackground.get(), rt.left, rt.top, imgbackground->GetWidth(), imgbackground->GetHeight());

	shared_ptr<Image> img;
	if (!LoadImageFromIDResource(IDB_ICON_LOCK, "PNG", img))
	{
		logerror("LoadImageFromIDResource() fail! file:%s line:%d\r\n", __FILE__, __LINE__);
		return;
	}

	g.DrawImage(img.get(), rt.left + 5, rt.top + 5, img->GetWidth(), img->GetHeight());

	Pen pen(Color(0, 0, 0));
	Point pt1, pt2;
	pt1.X = rt.left + img->GetWidth() + 10;
	pt1.Y = rt.top + 6;
	pt2.X = pt1.X;
	pt2.Y = pt1.Y + 16;

	//label->moveWindow(pt1.X, pt1.Y, GetWidth(rt), GetHeight(rt));
	//label->setText(str);
	//label->onPaint(hDC);

	SetBkColor(hCompatibleDC, RGB(255, 255, 255));
	SetTextColor(hCompatibleDC, RGB(0, 0, 0));
	//SelectObject(hCompatibleDC, GetStockObject(WHITE_BRUSH));

	RECT rtFill = { 0, 0, GetWidth(rt), GetHeight(rt) };
	FillRect(hCompatibleDC, &rtFill, (HBRUSH)GetStockObject(WHITE_BRUSH));
	SIZE size;
	GetTextExtentPoint32(hCompatibleDC, str.c_str(), str.length(), &size);
	TextOut(hCompatibleDC, 0, 0, str.c_str(), str.length());

	RECT rtEdit;
	getClientRect(&rtEdit);

	Point ptStart = pt1;

	int offsetX = 0;
	int cursorX = size.cx;
	int editWidth = GetWidth(rtEdit) - pt1.X;	// 可编辑区域
	pt1.X += size.cx + 3;
	if (size.cx >= editWidth)
	{
		offsetX = size.cx - editWidth;
		cursorX = editWidth;
		pt1.X = ptStart.X + editWidth + 3;
	}
	pt2.X = pt1.X;
	setAlpha(hCompatibleDC, GetWidth(rt) * 2, GetHeight(rt), 255);
	BitBlt(hDC, ptStart.X, ptStart.Y, editWidth, size.cy, hCompatibleDC, offsetX, 0, SRCCOPY);
	g.DrawLine(&pen, pt1, pt2);

	SelectObject(hCompatibleDC, hOldBitmap);
	DeleteObject(hBitmap);
	DeleteDC(hCompatibleDC);
}

void UIEdit::message(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_CHAR:
		if (wParam == VK_BACK)
		{
			if (str.length() > 0)
			{
				std::string tmp(str.c_str(), str.length() - 1);
				str = tmp;
			}
		}
		else if (str.length() < 32)
		{
			if (wParam >= 32 && wParam <= 126)
			{
				// 该范围可显示
				str += wParam;
			}
		}
		PostMessage(hMainWnd, WM_ON_PAINT, NULL, NULL);
		break;
	case WM_SETFOCUS:
		break;
	case WM_KILLFOCUS:
		break;
	default:
		break;
	}
}

#define DEFAULT_UNLOCK_CLASS_NAME	"XUNMEI_PLAYER_UNLOCK_WINDOW_CLASS"
#define DEFAULT_UNLOCK_WINDOW_NAME	"讯美录像文件解密窗口"
#define DEFAULT_WIDTH				300
#define DEFAULT_HEIGHT				250

UnLockWindow::UnLockWindow()
	: isLButtonDown(false)
{
	memset(&rtPlay, 0, sizeof(RECT));
}


UnLockWindow::~UnLockWindow()
{
}

bool UnLockWindow::create()
{
	if (!WindowsBase::create(WS_EX_TOOLWINDOW | WS_EX_WINDOWEDGE | WS_EX_LEFT | WS_EX_LTRREADING | WS_EX_TOPMOST | WS_EX_LAYERED, DEFAULT_UNLOCK_CLASS_NAME, DEFAULT_UNLOCK_WINDOW_NAME,
		WS_POPUP | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | WS_OVERLAPPED | WS_MINIMIZEBOX | WS_MAXIMIZEBOX,
		0, 0, DEFAULT_WIDTH, DEFAULT_HEIGHT, NULL, NULL, NULL, NULL))
	{
		return false;
	}

	{
		// 关闭按钮
		shared_ptr<Image> btnImg;
		LoadImageFromIDResource(IDB_BTN_CLOSE_NORMAL, "PNG", btnImg);
		RECT rt, rtMain;
		GetClientRect(_hWnd, &rtMain);
		shared_ptr<UIButton> btnClose(new UIButton());
		btnClose->create(rtMain.right - btnImg->GetWidth() - 5, 3, btnImg->GetWidth(), btnImg->GetHeight(), DF_BTN_CLOSE);
		btnClose->setStatusResourceID(UIButton::BTN_NORMAL, IDB_BTN_CLOSE_NORMAL);
		btnClose->setStatusResourceID(UIButton::BTN_FOCUS, IDB_BTN_CLOSE_FOCUS);
		btnClose->setStatusResourceID(UIButton::BTN_SELECTED, IDB_BTN_CLOSE_FOCUS);

		// 确定按钮
		LoadImageFromIDResource(IDB_BTN_NORMAL, "PNG", btnImg);
		shared_ptr<UIButton> btnOK(new UIButton());
		btnOK->create(rtMain.left + 20, rtMain.bottom - btnImg->GetHeight() - 15, btnImg->GetWidth(), btnImg->GetHeight(), DF_BTN_OK);
		btnOK->setStatusResourceID(UIButton::BTN_NORMAL, IDB_BTN_NORMAL);
		btnOK->setStatusResourceID(UIButton::BTN_FOCUS, IDB_BTN_FOCUS);
		btnOK->setStatusResourceID(UIButton::BTN_SELECTED, IDB_BTN_SELECTED);

		shared_ptr<UIButton> btnCancel(new UIButton());
		btnCancel->create(rtMain.right - btnImg->GetWidth() - 15, rtMain.bottom - btnImg->GetHeight() - 15, btnImg->GetWidth(), btnImg->GetHeight(), DF_BTN_CANCEL);
		btnCancel->setStatusResourceID(UIButton::BTN_NORMAL, IDB_BTN_NORMAL);
		btnCancel->setStatusResourceID(UIButton::BTN_FOCUS, IDB_BTN_FOCUS);
		btnCancel->setStatusResourceID(UIButton::BTN_SELECTED, IDB_BTN_SELECTED);

		shared_ptr<UILabel> labelOK(new UILabel());
		btnOK->getClientRect(&rt);
		labelOK->create(rt.left + btnImg->GetWidth() / 2 - 15, rt.top + 7, 40, 20, DF_BTN_ID);
		labelOK->setText("确定");
		labelOK->setFontStyle(FontStyleBold);
		labelOK->setFontSize(10);
		labelOK->setFontFamily("微软雅黑");

		shared_ptr<UILabel> labelCancel(new UILabel());
		btnCancel->getClientRect(&rt);
		labelCancel->create(rt.left + btnImg->GetWidth() / 2 - 15, rt.top + 7, 40, 20, DF_BTN_ID);
		labelCancel->setText("取消");
		labelCancel->setFontStyle(FontStyleBold);
		labelCancel->setFontSize(10);
		labelCancel->setFontFamily("微软雅黑");


		// 标题
		labelTitle = std::make_shared<UILabel>();
		labelTitle->create(rtMain.left + 5, rtMain.top + 5, GetWidth(rtMain) - 25, 20, DF_BTN_ID);
		labelTitle->setText("文件已被加密");
		labelTitle->setFontStyle(FontStyleRegular);
		labelTitle->setFontSize(10);
		labelTitle->setFontFamily("宋体");

		// IP地址
		labelAddress = std::make_shared<UILabel>();
		labelAddress->create(rtMain.left + 25, rtMain.top + 30, GetWidth(rtMain) - 25, 20, DF_BTN_ID);
		labelAddress->setText("加密主机：");
		labelAddress->setFontStyle(FontStyleRegular);
		labelAddress->setFontSize(10);
		labelAddress->setFontFamily("楷体");
		labelAddress->setColor(RGB(232, 232, 232));


		// 时间
		labelTime = std::make_shared<UILabel>();
		labelTime->create(rtMain.left + 25, rtMain.top + 50, GetWidth(rtMain) - 25, 20, DF_BTN_ID);
		labelTime->setText("加密时间：");
		labelTime->setFontStyle(FontStyleRegular);
		labelTime->setFontSize(10);
		labelTime->setFontFamily("楷体");
		labelTime->setColor(RGB(232, 232, 232));

		// 密码提示信息
		labelMsg = std::make_shared<UILabel>();
		labelMsg->create(rtMain.left + 25, rtMain.top + 70, GetWidth(rtMain) - 40, 80, DF_BTN_ID);
		labelMsg->setText("");
		labelMsg->setFontStyle(FontStyleRegular);
		labelMsg->setFontSize(10);
		labelMsg->setFontFamily("楷体");
		labelMsg->setColor(RGB(232, 232, 232));


		edit = std::make_shared<UIEdit>();
		edit->create(_hWnd, rtMain.left + 20, rtMain.bottom - btnImg->GetHeight() - 60, rtMain.right - 40, 30, DF_BTN_ID);
		focus = edit;
		Guard locker(mutex);
		listUI.clear();
		addControl(btnClose); addControl(btnOK); addControl(btnCancel);
		addControl(labelOK); addControl(labelCancel); addControl(edit);
		addControl(labelTitle); addControl(labelAddress); addControl(labelTime);
		addControl(labelMsg);
	}
	return true;
}

bool UnLockWindow::destroy()
{
	Guard locker(mutex);
	listUI.clear();
	return WindowsBase::destroy();
}

UnLockWindow* UnLockWindow::addControl(const shared_ptr<UIWidget> &control)
{
	Guard locker(mutex);
	listUI.push_back(control);
	return this;
}

bool UnLockWindow::show(DWORD dwShowStyle)
{
	PiantWindow();
	return WindowsBase::show(dwShowStyle);
}

bool UnLockWindow::setPlayRect(HWND playWnd)
{
	hPlayWnd = playWnd;
	return true;
}

bool UnLockWindow::setPlayRect(HWND playWnd, const RECT &rt)
{
	hPlayWnd = playWnd;
	rtPlay = rt;
	return true;
}

bool UnLockWindow::setPasswordDescribe(const std::string &describe)
{
	if (labelMsg == NULL)
	{
		return false;
	}
	//Json::Reader reader;
	//Json::Value root;
	//if (!reader.parse(describe, root))
	//{
	//	std::string msg = "密码提示：";
	//	msg += describe;
	//	labelMsg->setText(msg);
	//}
	//Json::Value jsonDescription = root["description"];
	//if (!jsonDescription.empty())
	//{
	//	std::string str = "密码提示：";
	//	str += jsonDescription.asCString();
	//	int len = str.length();
	//	labelMsg->setText(str);
	//}

	//Json::Value jsonAddress = root["address"];
	//if (!jsonAddress.empty())
	//{
	//	std::string str = "加密主机：";
	//	str += jsonAddress.asCString();
	//	labelAddress->setText(str);
	//}

	//Json::Value jsonTime = root["time"];
	//if (!jsonTime.empty())
	//{
	//	std::string str = "加密时间：";
	//	str += jsonTime.asCString();
	//	labelTime->setText(str);
	//}

	return true;
}

std::string UnLockWindow::getPassword()
{
	return edit->getText();
}

void UnLockWindow::PiantWindow()
{
	PostMessage(_hWnd, WM_ON_PAINT, NULL, NULL);
}

void UnLockWindow::onOk()
{
	if (edit->getText().length() == 0)
	{
		MessageBox(_hWnd, "密码不能为空", "警告", MB_ICONWARNING | MB_OK);
		return;
	}
	destroy();
}

void UnLockWindow::message(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_ON_PAINT:
		onPaint();
		break;
	case WM_LBUTTONDOWN:
		{
			POINT pt = { LOWORD(lParam), HIWORD(lParam) };
			onLButtonDown(&pt);
			PostMessage(_hWnd, WM_ON_PAINT, NULL, NULL);
		}
		break;
	case WM_LBUTTONUP:
		{
			POINT pt = { LOWORD(lParam), HIWORD(lParam) };
			onLButtonUp(&pt);
			PostMessage(_hWnd, WM_ON_PAINT, NULL, NULL);
		}
		break;
	case WM_MOUSEMOVE:
		{
			POINT pt = { LOWORD(lParam), HIWORD(lParam) };
			onMouseMove(&pt);
			PostMessage(_hWnd, WM_ON_PAINT, NULL, NULL);
		}
		break;
	case WM_NCHITTEST:
		{
			POINT pt = { LOWORD(lParam), HIWORD(lParam) };
			onNCHitTest(&pt);
		}
		break;
	default:
		break;
	}

	Control* pControl = (Control*)focus.get();
	if (pControl != NULL)
	{
		pControl->message(uMsg, wParam, lParam);
	}
}

bool UnLockWindow::onPreCreateWindow(CREATESTRUCT &cs)
{
	WNDCLASSEX clsex;
	clsex.cbSize = sizeof(WNDCLASSEX);
	clsex.cbWndExtra = clsex.cbClsExtra = 0;
	clsex.hbrBackground = NULL;
	clsex.hCursor = LoadCursor(NULL, IDC_ARROW);
	clsex.hIcon = clsex.hIconSm = LoadIcon(NULL, IDI_APPLICATION);
	clsex.hInstance = cs.hInstance;
	clsex.lpfnWndProc = DefualtWindowProc();
	clsex.lpszClassName = cs.lpszClass;
	clsex.lpszMenuName = NULL;
	clsex.style = CS_VREDRAW | CS_HREDRAW;
	if (!RegisterClassEx(&clsex))
	{
		if (GetLastError() == ERROR_CLASS_ALREADY_EXISTS)
		{
			return true;
		}
		logerror("RegisterClassEx() fail! file:%s line:%d err:%d\r\n", __FILE__, __LINE__, GetLastError());
		return false;
	}
	return true;
}

bool UnLockWindow::onAfterDestroyWindow()
{
	return UnregisterClass(DEFAULT_UNLOCK_CLASS_NAME, NULL) == TRUE;
}

bool UnLockWindow::onPaint()
{
	HDC hDC = GetDC(_hWnd);
	RECT rt;
	GetClientRect(_hWnd, &rt);
	HDC hCompatibleDC = CreateCompatibleDC(hDC);
	HBITMAP hBitmap = CreateMemryBitmap(hDC, GetWidth(rt), GetHeight(rt));
	HBITMAP hOldBitmap = (HBITMAP)SelectObject(hCompatibleDC, hBitmap);
	Graphics g(hCompatibleDC);

	/// 背景
	shared_ptr<Image> img_shaed;
	LoadImageFromIDResource(IDB_BACKGROUND, "PNG", img_shaed);
	g.DrawImage(img_shaed.get(), 0, 0, GetWidth(rt), GetHeight(rt));


	shared_ptr<Image> img_backgound_group;
	LoadImageFromIDResource(IDB_BACKGOUND_GROUP, "PNG", img_backgound_group);
	g.DrawImage(img_backgound_group.get(), 20, 26, img_backgound_group->GetWidth(), img_backgound_group->GetHeight());

	UIList list;
	{
		Guard locker(mutex);
		list = listUI;
	}

	for (UIList::iterator it = list.begin(); it != list.end(); it++)
	{
		(*it)->onPaint(hCompatibleDC);
	}

	RECT rtWindows;
	GetWindowRect(_hWnd, &rtWindows);
	POINT ptWinPos = { rtWindows.left, rtWindows.top };
	SIZE sizeWindow = { GetWidth(rtWindows), GetHeight(rtWindows) };
	POINT ptSrc = { 0, 0 };
	BLENDFUNCTION blend = { AC_SRC_OVER, 0, 255, AC_SRC_ALPHA };
	if (!::UpdateLayeredWindow(_hWnd, hDC, &ptWinPos, &sizeWindow, hCompatibleDC, &ptSrc, 0, &blend, ULW_ALPHA))
	{
		SetCompatibleBitmap(false);
	}

	//BitBlt(hDC, 0, 0, rt.right - rt.left, rt.bottom - rt.top, hCompatibleDC, 0, 0, SRCCOPY);
	SelectObject(hCompatibleDC, hOldBitmap);
	DeleteObject(hBitmap);
	DeleteDC(hCompatibleDC);
	return true;
}

void UnLockWindow::onLButtonDown(LPPOINT lpPoint)
{
	UIList list;
	{
		Guard locker(mutex);
		list = listUI;
	}

	bool isDownBtn = true;
	for (UIList::iterator it = list.begin(); it != list.end(); it++)
	{
		std::shared_ptr<UIButton> btn = std::dynamic_pointer_cast<UIButton>(*it);
		if (btn == NULL)
		{
			continue;
		}
		RECT rt;
		(*it)->getClientRect(&rt);
		if (isInRetangle(lpPoint, &rt) && !(*it)->isStatic())
		{
			isDownBtn = false;
//			focus = *it;
			if (btn != NULL)
			{
				btn->setStatus(UIButton::BTN_SELECTED);
			}
		}
		else
		{
			btn->setStatus(UIButton::BTN_NORMAL);
		}
	}
	isLButtonDown = isDownBtn;
}

void UnLockWindow::onLButtonUp(LPPOINT lpPoint)
{
	UIList list;
	{
		Guard locker(mutex);
		list = listUI;
	}

	for (UIList::iterator it = list.begin(); it != list.end(); it++)
	{
		std::shared_ptr<UIButton> btn = std::dynamic_pointer_cast<UIButton>(*it);
		if (btn == NULL)
		{
			continue;
		}
		RECT rt;
		(*it)->getClientRect(&rt);

		unsigned int cid = 0;
		if (isInRetangle(lpPoint, &rt))
		{
			cid = (*it)->getID();
		}
		btn->setStatus(UIButton::BTN_NORMAL);

		switch (cid)
		{
		case DF_BTN_CLOSE:
		case DF_BTN_CANCEL:
			edit->setText("");
			destroy();
			break;
		case DF_BTN_OK:
			onOk();
			break;
		default:
			break;
		}
	}
	isLButtonDown = false;
}

void UnLockWindow::onMouseMove(LPPOINT lpPoint)
{
	UIList list;
	{
		Guard locker(mutex);
		list = listUI;
	}

	for (UIList::iterator it = list.begin(); it != list.end(); it++)
	{
		std::shared_ptr<UIButton> btn = std::dynamic_pointer_cast<UIButton>(*it);
		if (btn == NULL)
		{
			continue;
		}
		if (btn->getStatus() == UIButton::BTN_SELECTED)
		{
			continue;
		}
		RECT rt;
		(*it)->getClientRect(&rt);
		if (isInRetangle(lpPoint, &rt))
		{
			btn->setStatus(UIButton::BTN_FOCUS);
		}
		else
		{
			btn->setStatus(UIButton::BTN_NORMAL);
		}
	}
}

void UnLockWindow::onNCHitTest(LPPOINT lpPoint)
{
	if (isLButtonDown)
	{
		SendMessage(_hWnd, WM_NCLBUTTONDOWN, HTCAPTION, MAKELPARAM(lpPoint->x, lpPoint->y));
	}
}
#endif