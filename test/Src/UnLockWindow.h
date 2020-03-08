#ifndef _UNLOCKWINDOW_H__
#define _UNLOCKWINDOW_H__

#include "Base/Base.h"
#include <Windows.h>

using namespace Public::Base;
class Control
{
public:
	Control();

	virtual ~Control();

	virtual void message(UINT uMsg, WPARAM wParam, LPARAM lParam);

	virtual bool isStatic();
};

class WindowsBase : public Control
{
	WindowsBase(HWND hWnd);
public:

	WindowsBase();

	static bool init();

	static bool unint();

	virtual ~WindowsBase();

	virtual bool create(DWORD dwExStyle, LPCTSTR lpClassName, LPCTSTR lpWindowName,
		DWORD dwStyle, int x, int y, int nWidth, int nHeight, HWND hWndParent,
		HMENU hMenu, HINSTANCE hInstance, LPVOID lpParam);

	virtual bool destroy();

	virtual bool show(DWORD dwShowStyle);

	virtual bool center();

	virtual bool msgloop();

	static WindowsBase* FromHandle(HWND hWnd);

	WNDPROC DefualtWindowProc();
	
	HWND getHWnd();
private:
	virtual void message(UINT uMsg, WPARAM wParam, LPARAM lParam);

	void onPaint();

	void onDestroy();

	virtual bool onPreCreateWindow(CREATESTRUCT &cs);

	virtual bool onAfterDestroyWindow();

	static LRESULT CALLBACK MyWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

	LRESULT WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
protected:
	HWND _hWnd;
};


// Ðé¼ÙµÄ´°¿Ú£¬ÎÞ¾ä±ú
class UIWidget : public Control
{
public:
	UIWidget();

	virtual ~UIWidget();

	virtual bool create(int x, int y, int width, int height, unsigned int id);

	virtual bool destroy();

	virtual void onPaint(HDC hDC);

	virtual bool moveWindow(int x, int y, int width, int height);
	
	virtual bool getClientRect(RECT *lpRect);

	virtual unsigned int getID();
private:
	bool isValidWindow(int x, int y, int width, int height, RECT &rt);
private:
	RECT rtClient;
	unsigned int cid;
};

class UIButton : public UIWidget
{
public:
	enum ButtonStatus
	{
		BTN_NORMAL = 1,
		BTN_FOCUS,
		BTN_SELECTED,
		BTN_DISABLE,
	};
public:
	UIButton();

	virtual ~UIButton();

	bool setStatusResourceID(ButtonStatus status, DWORD dwResourceID);

	void onPaint(HDC hDC);
	
	void setStatus(ButtonStatus status);

	ButtonStatus getStatus();
private:
	std::map<ButtonStatus, DWORD> mResource;
	Mutex mutex;
	ButtonStatus _status;
};


class UILabel : public UIWidget
{
public:
	UILabel();

	virtual ~UILabel();

	void setText(const std::string &string);

	void setFontSize(unsigned int size);

	void setFontFamily(const std::string &family);

	void setFontStyle(unsigned int style);

	void setColor(unsigned int color);

	void onPaint(HDC hDC);

	bool isStatic();
private:
	std::string text;
	std::string fontFamily;
	unsigned int fontSize;
	unsigned int fontStyle;
	unsigned int dwColor;
	RECT rtText;
};

class UIEdit : public UIWidget
{
public:
	UIEdit();

	virtual ~UIEdit();

	virtual bool create(HWND hWnd, int x, int y, int width, int height, unsigned int id);

	void onPaint(HDC hDC);

	std::string getText();

	void setText(const std::string strText);
private:
	void message(UINT uMsg, WPARAM wParam, LPARAM lParam);
private:
	HWND hMainWnd;
	shared_ptr<UILabel> label;
	std::string str;
};

class UnLockWindow : public WindowsBase
{
	typedef std::list<shared_ptr<UIWidget> > UIList;
//	typedef std::list<shared_ptr<Control> > UIControls;
public:
	UnLockWindow();

	~UnLockWindow();

	bool create();

	bool destroy();

	bool show(DWORD dwShowStyle);

	bool setPlayRect(HWND playWnd);

	bool setPlayRect(HWND playWnd, const RECT &rt);

	bool setPasswordDescribe(const std::string &describe);

	std::string getPassword();
private:
	virtual void message(UINT uMsg, WPARAM wParam, LPARAM lParam);

	virtual bool onPreCreateWindow(CREATESTRUCT &cs);

	virtual bool onAfterDestroyWindow();

	bool onPaint();

	void onLButtonDown(LPPOINT lpPoint);

	void onLButtonUp(LPPOINT lpPoint);

	void onMouseMove(LPPOINT lpPoint);

	void onNCHitTest(LPPOINT lpPoint);

	void PiantWindow();

	void onOk();

	UnLockWindow* addControl(const shared_ptr<UIWidget> &control);
private:
	HWND hPlayWnd;
	RECT rtPlay;
	shared_ptr<Control> focus;
	shared_ptr<UIEdit> edit;
	UIList listUI;
	Mutex mutex;
	bool isLButtonDown;
	shared_ptr<UILabel> labelTitle;
	shared_ptr<UILabel> labelAddress;
	shared_ptr<UILabel> labelTime;
	shared_ptr<UILabel> labelMsg;
};

#endif