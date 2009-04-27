#ifndef _Docking_DockTabBar_h_
#define _Docking_DockTabBar_h_

#include <CtrlLib/CtrlLib.h>
using namespace Upp;

#include <TabBar/TabBar.h>

class DockTabBar : public TabBar { 
public:
	typedef DockTabBar CLASSNAME;

	Callback1<int> 	WhenContext;
	Callback1<int>  WhenDrag;
	
	virtual void 	ContextMenu(Bar& bar)			{ TabBar::ContextMenu(bar); }
	
	virtual void 	FrameAddSize(Size& sz);
	virtual void	FrameLayout(Rect& r);	
	bool 			IsAutoHide() const				{ return GetCount() <= autohide; }

	DockTabBar& 	AutoHideMin(int hidemin = 1)	{ autohide = hidemin; return *this; }
	DockTabBar& 	Icons(bool b = true)			{ icons = b; return *this; }

	void			SyncRepos()						{ Repos(); }
	void			ShowText(bool show)				{ showtext = show; }

	DockTabBar();
protected:
	int autohide;
	bool icons:1;
	bool showtext:1;

	virtual void PaintTabData(Draw& w, const Rect &t, const Tab& tab, const Font &font, 
		Color ink, dword style, int bl);
	virtual Size GetStdSize(const Value &q);

	virtual void RightDown(Point p, dword keyflags);
	virtual void LeftDown(Point p, dword keyflags)	{ TabBar::LeftDown(p, keyflags &= ~K_SHIFT); }
	virtual void LeftUp(Point p, dword keyflags)	{ TabBar::LeftUp(p, keyflags &= ~K_SHIFT); }
	virtual void LeftDrag(Point p, dword keyflags);
};

class DockCont;

class AutoHideBar : public DockTabBar {
public:
	typedef	AutoHideBar CLASSNAME;

	virtual void MouseEnter(Point p, dword keyflags);	
	virtual void MouseLeave();	

	void 	AddCtrl(DockCont& c, const String& group = Null);
	int 	FindCtrl(const DockCont& c) const;
	DockCont *GetCtrl(int ix)	const				{ return ValueTo<DockCont *>(Get(ix));  }	
	void	RemoveCtrl(int ix);
	void	RemoveCtrl(DockCont& c)					{ return RemoveCtrl(c, FindCtrl(c)); }
	void	RemoveCtrl(DockCont& c, int ix);
	bool 	HasCtrl(const DockCont& c) const		{ return (FindCtrl(c) >= 0); }
	
	void	ShowAnimate(Ctrl *c);	
	
	static void SetTimeout(int delay_ms)	  		{ ASSERT(delay_ms > 0); autohide_timeout = delay_ms; }
	
	AutoHideBar();
	
private:
	static int autohide_timeout;

	struct HidePopup : public Ctrl 	{
		HidePopup() { BackPaint(); }
		Callback WhenEnter;
		Callback WhenLeave;
		virtual void ChildMouseEvent(Ctrl *child, int event, Point p, int zdelta, dword keyflags); 
	};

	enum { TIMEID_ACTION_CHECK = Ctrl::TIMEID_COUNT,
		   TIMEID_HIDE_TIMEOUT,
           TIMEID_COUNT };
	Ctrl 		*ctrl;
	HidePopup 	 popup;
	
	void 	TabDrag(int ix);
	void 	TabHighlight();	
	void	TabClose(Value v);				
	void	HideAnimate(Ctrl *c);
	void 	AdjustSize(Rect& r, const Size& sz);
};

#endif
