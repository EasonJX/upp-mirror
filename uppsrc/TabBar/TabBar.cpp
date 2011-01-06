#include "TabBar.h"

#define TFILE <TabBar/TabBar.t>
#include <Core/t.h>

#define IMAGECLASS TabBarImg
#define IMAGEFILE <TabBar/TabBar.iml>
#include <Draw/iml_source.h>

NAMESPACE_UPP

// AlignedFrame
void AlignedFrame::FrameLayout(Rect &r)
{
	switch(layout)
	{
		case LEFT:
			LayoutFrameLeft(r, this, framesize);
			break;
		case TOP:
			LayoutFrameTop(r, this, framesize);
			break;
		case RIGHT:
			LayoutFrameRight(r, this, framesize);
			break;
		case BOTTOM:
			LayoutFrameBottom(r, this, framesize);
			break;
	}
	r.top += border;
	r.left += border;
	r.right -= border;
	r.bottom -= border;
}

void AlignedFrame::FrameAddSize(Size& sz)
{
	sz += border * 2;
	IsVert() ? sz.cx += framesize : sz.cy += framesize;
}

void AlignedFrame::FramePaint(Draw& w, const Rect& r)
{
	if(border > 0)
	{
		Rect n = r;
		switch(layout)
		{
			case LEFT:
				n.left += framesize;
				break;
			case TOP:
				n.top += framesize;
				break;
			case RIGHT:
				n.right -= framesize;
				break;
			case BOTTOM:
				n.bottom -= framesize;
				break;		
		}
		ViewFrame().FramePaint(w, n);
	}
	else
		FrameCtrl<Ctrl>::FramePaint(w, r);		
}

AlignedFrame& AlignedFrame::SetFrameSize(int sz, bool refresh)
{
	framesize = sz; 
	if (refresh) RefreshParentLayout(); 
	return *this;	
}

void AlignedFrame::Fix(Size& sz)
{
	if(IsVert())
		Swap(sz.cx, sz.cy);
}

void AlignedFrame::Fix(Point& p)
{
	if(IsVert())
		Swap(p.x, p.y);
}

Size AlignedFrame::Fixed(const Size& sz)
{
	return IsVert() ? Size(sz.cy, sz.cx) : Size(sz.cx, sz.cy);
}

Point AlignedFrame::Fixed(const Point& p)
{
	return IsVert() ? Point(p.y, p.x) : Point(p.x, p.y);
}

// TabScrollBar
TabScrollBar::TabScrollBar()
{
	Clear();
}

void TabScrollBar::Clear()
{
	total = 0;
	pos = 0;
	ps = 0;
	start_pos = 0;
	new_pos = 0;
	old_pos = 0;
	sz.Clear();
	ready = false;	
}

void TabScrollBar::UpdatePos(bool update)
{
	sz = GetSize();
	Fix(sz);
	if(total <= 0 || sz.cx <= 0)
		cs = ics = 0;
	else
	{
		cs = sz.cx / ((double) total + 0.5);
		ics = total / ((double) sz.cx);
	}
	size = sz.cx * cs;
	if(update)
		pos = new_pos - start_pos;
	if(pos < 0)
		pos = 0;
	else if(pos + size > sz.cx)
		pos = sz.cx - size;

	ps = total > sz.cx ? pos * ics : 0;
}

void TabScrollBar::Paint(Draw &w)
{
	if(!ready)
	{
		UpdatePos();
		ready = true;
	}
	Size rsz = GetSize();
	#ifdef TABBAR_DEBUG
	w.DrawRect(rsz, Red);
	#else
	w.DrawRect(rsz, White);
	#endif
	Point p;
	
	if(total > sz.cx) {
		p = Point(ffloor(pos), 1);
		rsz = Size(fceil(size), (IsVert() ? rsz.cx : rsz.cy) - 2);
	}
	else {
		p = Point(0, 1);
		rsz = Size(sz.cx, (IsVert() ? rsz.cx : rsz.cy) - 2);
	}
	Fix(p);
	Fix(rsz);
	w.DrawRect(p.x, p.y, rsz.cx, rsz.cy, Blue);
}

void TabScrollBar::Layout()
{
	UpdatePos(false);
}

void TabScrollBar::LeftDown(Point p, dword keyflags)
{
	SetCapture();
	Fix(p);
	old_pos = new_pos = p.x;
	if(p.x < pos || p.x > pos + size)
		start_pos = size / 2;
	else
		start_pos = tabs(p.x - pos);
	UpdatePos();
	UpdateActionRefresh();	
}

void TabScrollBar::LeftUp(Point p, dword keyflags)
{
	ReleaseCapture();
	Fix(p);
	old_pos = p.x;
}

void TabScrollBar::MouseMove(Point p, dword keyflags)
{
	if(!HasCapture())
		return;

	Fix(p);
	new_pos = p.x;
	UpdatePos();
	UpdateActionRefresh();
}

void TabScrollBar::MouseWheel(Point p, int zdelta, dword keyflags)
{
	AddPos(-zdelta / 4, true);
	UpdateActionRefresh();
}

int TabScrollBar::GetPos() const
{
	return ffloor(ps);
}

void TabScrollBar::SetPos(int p, bool dontscale)
{
	pos = total > 0 ? dontscale ? p : iscale(p, sz.cx, total) : 0;
	UpdatePos(false);
	Refresh();
}

void TabScrollBar::AddPos(int p, bool dontscale)
{
	pos += total > 0 ? dontscale ? p : iscale(p, sz.cx, total) : 0;
	UpdatePos(false);
	Refresh();
}

int TabScrollBar::GetTotal() const
{
	return total;
}

void TabScrollBar::SetTotal(int t)
{
	bool upd = total < t;
	total = t;
	UpdatePos(upd);
	Refresh();
}

void TabScrollBar::AddTotal(int t)
{
	sz = GetSize();
	Fix(sz);
	total += t;
	if(total <= 0 || sz.cx <= 0)
		cs = ics = 0;
	else
		cs = sz.cx / ((double) total + 0.5);
	size = sz.cx * cs;
	ps = min(ps, (double)(total-sz.cx));
	pos = (int)(ps * cs);		
	old_pos = new_pos = (int)(pos - start_pos);
	
	Refresh();
}

void TabScrollBar::GoEnd()
{
	pos = total;
	UpdatePos(false);
	Refresh();
}

void TabScrollBar::GoBegin()
{
	pos = 0;
	UpdatePos(false);
	Refresh();
}

void TabScrollBar::Set(const TabScrollBar& t)
{
	total = t.total;
	pos = t.pos;
	ps = t.ps;
	Refresh();
}

bool TabScrollBar::IsScrollable() const
{
	// Note: sz already 'fixed'
	return total > sz.cx && sz.cx > 0;
}

// Group

void TabBar::Group::Serialize(Stream& s)
{
	s % name % active % count % first % last;
}


// TabBar

TabBar::TabBar()
{
	Clear();

	id = 0;
	display = NULL;
	crosses = true;
	crosses_side = RIGHT;
	grouping = true;
	isctrl = false;
	isdrag = false;
	inactivedisabled = false;
	autoscrollhide = true;
	stacking = false;
	groupseps = false;
	allownullcursor = false;
	icons = true;
	mintabcount = 1;
	scrollbar_sz = TB_SBHEIGHT;
	allowreorder = true;

	// Init sorting
	groupsort = false;
	tabsort = false;
	stacksort = true;
	contextmenu = true;
	keysorter_inst.vo = &Single<StdValueOrder>();
	valuesorter_inst.vo = &Single<StdValueOrder>();
	stacksorter_inst.vo = &Single<StdValueOrder>();
	tabsorter = &keysorter_inst;
	groupsorter = &Single<TabGroupSort>();
	stacksorter = &stacksorter_inst;

	SetAlign(TOP);
	SetFrameSize(GetHeight(false));
	BackPaint();
}

void TabBar::Set(const TabBar& t)
{
	CopyBaseSettings(t);
	
	id = t.id;
	
	tabs.Clear();
	tabs <<= t.tabs;
	groups.Clear();
	groups <<= t.groups;
	
	active = t.active;
	cross = -1;
	highlight = -1;
	target = -1;
	
	sc.Set(t.sc);
	SetAlign(t.GetAlign());
}

void TabBar::CloseAll(int exception)
{
	Vector<Value> vv;
	for(int i = 0; i < tabs.GetCount(); i++)
		if(i != exception)
			vv.Add(tabs[i].key);
		
	if (exception < 0 && CancelCloseAll())
		return;
	else if (exception >= 0 && CancelCloseSome(Vector<Value>(vv,0))) 
		return;
	
	for(int i = tabs.GetCount() - 1; i >= 0; i--)
		if(i != exception)
			tabs.Remove(i);

	SetCursor(0);
	if (exception >= 0)
		WhenCloseSome(vv);
	else
		WhenCloseAll();

	MakeGroups();
	Repos();
	Refresh();
}

int TabBar::GetNextId()
{
	return id++;
}

void TabBar::ContextMenu(Bar& bar)
{
	if (highlight >= 0 && crosses) {
		bar.Add(tabs.GetCount() > mintabcount, t_("Close"), THISBACK1(Close, highlight));
		bar.Separator();
	}
	int cnt = groups.GetCount();
	for(int i = 0; i < cnt; i++)
	{
		String name = Format("%s (%d)", groups[i].name, groups[i].count);
		Bar::Item &it = i > 0 ? bar.Add(name, THISBACK1(GroupMenu, i))
		                      : bar.Add(name, THISBACK1(DoGrouping, i));
		if(i == group)
			it.Image(TabBarImg::CHK);
		if(i == 0 && cnt > 1)
			bar.Separator();
	}
	bool sep = true;
	if (GetCursor() >= 0 && crosses) {
		bar.Separator();
		sep = false;
		bar.Add(t_("Close others"), THISBACK1(CloseAll, GetCursor()));
	}
	if (mintabcount <= 0 && crosses) {
		if (sep) bar.Separator();
		bar.Add(t_("Close all"), THISBACK1(CloseAll, -1));
	}
}

void TabBar::GroupMenu(Bar &bar, int n)
{
	bar.Add(t_("Set active"), THISBACK1(DoGrouping, n));
	bar.Add(t_("Close"), THISBACK1(DoCloseGroup, n));
}

void TabBar::Tab::Set(const Tab& t)
{
	id = t.id;
	
	img = t.img;
	key = t.key;
	value = t.value;
	group = t.group;
	
	stackid = t.stackid;
	stack = t.stack;

	visible = t.visible;

	pos = t.pos;
	size = t.size;
	
	cross_pos = t.cross_pos;
	cross_size = t.cross_size;
	
	tab_pos = t.tab_pos;
	tab_size = t.tab_size;
	
	items <<= t.items;
}

void TabBar::Tab::Serialize(Stream& s)
{
	s % id % key % value % group % stackid % stack % visible;
}

bool TabBar::Tab::HasMouse(const Point& p) const
{
	if(!visible)
		return false;
	
	return p.x >= tab_pos.x && p.x < tab_pos.x + tab_size.cx &&
	       p.y >= tab_pos.y && p.y < tab_pos.y + tab_size.cy;
}

bool TabBar::Tab::HasMouseCross(const Point& p) const
{
	if(!visible)
		return false;

	return p.x >= cross_pos.x && p.x < cross_pos.x + cross_size.cx &&
	       p.y >= cross_pos.y && p.y < cross_pos.y + cross_size.cy;
}

int TabBar::FindGroup(const String& g) const
{
	for(int i = 0; i < groups.GetCount(); i++)
		if(groups[i].name == g)
			return i;
	return -1;
}

void TabBar::DoStacking()
{
	Value v = GetData();
		
	// Reset stack info
	for (int i = 0; i < tabs.GetCount(); i++) {
		Tab &t = tabs[i];
		t.stack = -1;
		t.stackid = GetStackId(t);
	}
	// Create stacks
	Vector< Vector<Tab> > tstack;
	for (int i = 0; i < tabs.GetCount(); i++) {
		Tab &ti = tabs[i];
		if (ti.stack < 0) {
			ti.stack = tstack.GetCount();
			Vector<Tab> &ttabs = tstack.Add();
			ttabs.Add(ti);
			for (int j = i + 1; j < tabs.GetCount(); j++)	{
				Tab &tj = tabs[j];
				if (tj.stack < 0 && tj.stackid == ti.stackid && (!grouping || tj.group == ti.group)) {
					tj.stack = ti.stack;
					ttabs.Add(tj);
				}
			}
		}
	}
	stackcount = tstack.GetCount();
	// Recombine
	tabs.SetCount(0);
	for (int i = 0; i < tstack.GetCount(); i++) {
		if (stacksort)
			StableSort(tstack[i], *stacksorter);
		tabs.AppendPick(tstack[i]);
	}
	highlight = -1;
	SetData(v);
	MakeGroups();	
	Repos();
}

void TabBar::DoUnstacking() 
{
	stackcount = 0;
	for (int i = 0; i < tabs.GetCount(); i++)
		tabs[i].stack = -1;
	highlight = -1;
	MakeGroups();	
	Repos();
	if (HasCursor())	
		SetCursor(-1);
	else
		Refresh();
}

void TabBar::SortStack(int stackix)
{
	if (!stacksort) return;	
	
	int head = FindStackHead(stackix);
	int tail = head;
	while (tail < tabs.GetCount() && tabs[tail].stack == stackix)
		++tail;	
	SortStack(stackix, head, tail-1);
}

void TabBar::SortStack(int stackix, int head, int tail)
{
	if (!stacksort) return;
	
	int headid = tabs[head].id;
	StableSort(tabs.GetIter(head), tabs.GetIter(tail), *stacksorter);
	while (tabs[head].id != headid)
		CycleTabStack(head, stackix);
}

void TabBar::MakeGroups()
{
	groups[0].count = tabs.GetCount();
	groups[0].first = 0;
	groups[0].last = tabs.GetCount() - 1;

	if (groupsort)
		StableSort(tabs, *groupsorter);

	for(int i = 1; i < groups.GetCount(); i++)
	{
		groups[i].count = 0;
		groups[i].first = 10000000;
		groups[i].last = 0;
	}

	for(int i = 0; i < tabs.GetCount(); i++)
	{
		Tab &tab = tabs[i];
		int n = FindGroup(tab.group);
		ASSERT(n >= 0);
		if (n > 0) {
			if(groups[n].active < 0)
				groups[n].active = tab.id;
			groups[n].count++;
			groups[n].last = i;
	
			if(i < groups[n].first)
				groups[n].first = i;
			if(i > groups[n].last)
				groups[n].last = i;
		}
	}

	int removed = 0;
	for(int i = 1; i < groups.GetCount(); i++)
		if(groups[i].count == 0)
		{
			groups.Remove(i - removed);
			removed++;
		}
	if(group > groups.GetCount() - 1 && group > 0)
		group--;
}

void TabBar::DoGrouping(int n)
{
	group = n;
	Repos();
	SyncScrollBar();
	SetCursor(-1);
}

void TabBar::DoCloseGroup(int n)
{
	int cnt = groups.GetCount();
	if(cnt <= 0)
		return;

	String groupName = groups[n].name;
	
	/*
		do WhenCloseSome()/CancelCloseSome() checking
		before WhenClose()/CancelClose() stuff
		(that code must be reviewed anyways...)
		In order to leave existing code as it is, following
		changes have effect *ONLY* if WhenCloseSome()/CancelCloseSome()
		callbacks are used, otherwise previous path is taken.
		I think we should anyways review some parts of it later
	*/
	
	if(WhenCloseSome || CancelCloseSome)
	{
		Vector<Value>vv;
		int nTabs = 0;
		for(int i = 0; i < tabs.GetCount(); i++)
			if(groupName == tabs[i].group) {
				vv.Add(tabs[i].key);
				nTabs++;
			}
		// at first, we check for CancelCloseSome()
		if(vv.GetCount() && !CancelCloseSome(Vector<Value>(vv,0))) {
			// we didn't cancel globally, now we check CancelClose()
			// for each tab -- group gets removed ONLY if ALL of
			// group tabs are closed
			for(int i = tabs.GetCount() - 1; i >= 0; i--) {
				if(groupName == tabs[i].group && tabs.GetCount() > 1) {
					Value v = tabs[i].key;
					if(!CancelClose(v))
					{
						nTabs--;
						WhenClose(v);
						tabs.Remove(i);
					}
				}
				// remove group if all of its tabs get closed
				if(!nTabs) {
					if(cnt == n)
						group--;
					if(cnt > 1)
						groups.Remove(n);
				}
				MakeGroups();
				Repos();
				SetCursor(-1);
			}
		}
		return;
	}

	// previous code path, taken if WhenCancelSome()/WhenCloseSome()
	for(int i = tabs.GetCount() - 1; i >= 0; i--)
	{
		if(groupName == tabs[i].group && tabs.GetCount() > 1) {
			Value v = tabs[i].value; // should be key ??
			if (!CancelClose(v)) {
				WhenClose(v);
				tabs.Remove(i);
			}
		}
	}

	if (cnt == n)
		group--;

	if(cnt > 1) // what if CancelClose suppressed some tab closing ?
		groups.Remove(n);
	MakeGroups();
	Repos();
	SetCursor(-1);
}

void TabBar::NewGroup(const String &name)
{
	Group &g = groups.Add();
	g.name = name;
	g.count = 0;
	g.first = 10000000;
	g.last = 0;
	g.active = -1;
}

Image TabBar::AlignImage(int align, const Image& img)
{
	switch(align) {
		case AlignedFrame::LEFT: 
			return RotateAntiClockwise(img);
		case AlignedFrame::RIGHT: 
			return RotateClockwise(img);
		case AlignedFrame::BOTTOM:
			return MirrorVert(img);
		default:
			return img;
	}
}

Value TabBar::AlignValue(int align, const Value &v, const Size &sz)
{
	Size isz = sz;
	if(align == AlignedFrame::LEFT || align == AlignedFrame::RIGHT)
		Swap(isz.cx, isz.cy);

	ImageDraw w(isz.cx, isz.cy);
	w.DrawRect(isz, SColorFace());
	ChPaint(w, isz, v);
	ImageBuffer img;
	return AlignImage(align, (Image)w);
}

void TabBar::Tab::Clear()
{
	items.Clear();
}

TabBar::TabItem& TabBar::Tab::AddImage(const Image& img, int side)
{
	TabItem& ti = items.Add();
	ti.img = img;
	ti.size = img.GetSize();
	ti.side = side;
	return ti;
}

TabBar::TabItem& TabBar::Tab::AddValue(const Value& q, const Font& font, const Color& ink)
{
	TabItem& ti = items.Add();
	
	ti.font = font;
	ti.ink = ink;
	
	if(IsType<AttrText>(q)) {
		const AttrText& t = ValueTo<AttrText>(q);
		ti.text = t.text;
		if(!IsNull(t.font))
			ti.font = t.font;
		
		if(!IsNull(t.ink))
			ti.ink = t.ink;
	}
	else
		ti.text = IsString(q) ? q : StdConvert().Format(q);
	
	ti.size = GetTextSize(ti.text, ti.font);
	return ti;
}

TabBar::TabItem& TabBar::Tab::AddText(const WString& s, const Font& font, const Color& ink)
{
	TabItem& ti = items.Add();

	ti.font = font;
	ti.ink = ink;
	ti.text = s;
	ti.size = GetTextSize(ti.text, ti.font);
	return ti;
}

TabBar::TabItem& TabBar::Tab::AddSpace(int space, int side)
{
	TabItem& ti = items.Add();
	
	ti.size.cx = space;
	ti.size.cy = 0;
	ti.side = side;
	return ti;
}

void TabBar::ComposeTab(Tab& tab, const Font &font, Color ink, int style)
{
	if(PaintIcons() && tab.HasIcon())
	{
		tab.AddImage(tab.img);
		tab.AddSpace(TB_SPACEICON);
	}
	tab.AddValue(tab.value, font, ink).Clickable();
}

void TabBar::ComposeStackedTab(Tab& tab, const Tab& stacked_tab, const Font& font, Color ink, int style)
{
	tab.AddImage(stacked_tab.img);
	tab.AddText("|...", font, ink);	
}

int TabBar::GetTextAngle()
{
	return AlignedFrame::IsVert() ? (GetAlign() == LEFT ? 900 : 2700) : 0;
}

Point TabBar::GetTextPosition(int align, const Rect& r, int cy, int space) const
{
	Point 	p;
	
	if(align == LEFT)
	{
		p.y = r.bottom - space;
		p.x = r.left + (r.GetWidth() - cy) / 2;
	}
	else if(align == RIGHT)
	{
		p.y = r.top + space;
		p.x = r.right - (r.GetWidth() - cy) / 2;
	}
	else
	{
		p.x = r.left + space;
		p.y = r.top + (r.GetHeight() - cy) / 2;
	}
	return p;
}

Point TabBar::GetImagePosition(int align, const Rect& r, int cx, int cy, int space, int side) const
{
	Point p;

	if (align == LEFT)
	{
		p.x = r.left + (r.GetWidth() - cy) / 2;
		p.y = side == LEFT ? r.bottom - space - cx : r.top + space;
	}
	else if (align == RIGHT)
	{
		p.x = r.right - (r.GetWidth() + cy) / 2;
		p.y = side == LEFT ? r.top + space : r.bottom - space - cx;
	}
	else if (align == TOP)
	{
		p.x = side == LEFT ? r.left + space : r.right - cx - space;
		p.y = r.top + (r.GetHeight() - cy) / 2;
	}
	else if (align == BOTTOM)
	{
		p.x = side == LEFT ? r.left + space : r.right - cx - space;
		p.y = r.bottom - (r.GetHeight() + cy) / 2;
	}
	return p;
}

void TabBar::PaintTabItems(Tab& t, Draw &w, const Rect& rn, int align)
{
	int pos_left = TB_MARGIN;
	int pos_right = (IsVert() ? rn.GetHeight() : rn.GetWidth()) - TB_MARGIN;
	
	for(int i = 0; i < t.items.GetCount(); i++)
	{
		const TabItem& ti = t.items[i];
		
		Point p;
		int pos = ti.side == LEFT ? pos_left : pos_right - ti.size.cx;
		
		if(!IsNull(ti.img))
		{
			p = GetImagePosition(align, rn, ti.size.cx, ti.size.cy, pos, LEFT);
			w.DrawImage(p.x, p.y, IsVert() ? AlignImage(align, ti.img) : ti.img);
		}
		
		if(!IsNull(ti.text))
		{
			p = GetTextPosition(align, rn, ti.size.cy, pos);
			w.DrawText(p.x, p.y, GetTextAngle(), ti.text, ti.font, ti.ink);
		}

		if(ti.cross)
		{
			t.cross_size = ti.size;
			t.cross_pos = p;
		}
		
		if(ti.stacked_tab >= 0 && ti.clickable)
		{
			Tab& st = tabs[ti.stacked_tab];
			
			if(align == RIGHT)
			{
				st.tab_pos = Point(rn.left, rn.top + pos);
				st.tab_size = Size(rn.GetWidth(), ti.size.cx);	
			}
			else if(align == LEFT)
			{
				st.tab_pos = Point(rn.left, rn.bottom - pos - ti.size.cx);
				st.tab_size = Size(rn.GetWidth(), ti.size.cx);	
			}
			else
			{
				st.tab_pos = Point(rn.left + pos, rn.top);
				st.tab_size = Size(ti.size.cx, rn.GetHeight());
			}
			
			#ifdef TABBAR_DEBUG
			DrawFrame(w, Rect(st.tab_pos, st.tab_size), Red);
			#endif
		}		
				
		if(ti.side == LEFT)
			pos_left += ti.size.cx;
		else
			pos_right -= ti.size.cx;
	}	
}

void TabBar::PaintTab(Draw &w, const Size &sz, int n, bool enable, bool dragsample)
{
	TabBar::Tab &t = tabs[n];
	const Style& s = GetStyle();
	int align = GetAlign();
	int cnt = dragsample ? 1 : tabs.GetCount();
	
	bool ac = (n == active && enable);
	bool hl = (n == highlight && enable) || (stacking && highlight >= 0 && tabs[highlight].stack == t.stack);

	int ndx = !enable ? CTRL_DISABLED :
		       ac ? CTRL_PRESSED :
		       hl ? CTRL_HOT : CTRL_NORMAL;

	int c = align == LEFT ? cnt - n : n;	
	int lx = n > 0 ? s.extendleft : 0;
	int x = t.pos.x - sc.GetPos() - lx + s.margin;
	
	int dy = -s.sel.top * ac;
	int sel = s.sel.top;

	int df = 0;
	
	if (IsBR())
	{
		dy = -dy;
		sel = s.sel.bottom;
		df = Fixed(sz).cy;
	}

	Size  sa = Size(t.size.cx + lx + s.sel.right + s.sel.left, t.size.cy + s.sel.bottom);
	Point pa = Point(x - s.sel.left, IsBR() ? df - sa.cy : 0);

	Size  sn = Size(t.size.cx + lx, t.size.cy - s.sel.top);
	Point pn = Point(x, IsBR() ? df - sn.cy - s.sel.top : s.sel.top);

	Rect ra(Fixed(pa), Fixed(sa));
	Rect rn(Fixed(pn), Fixed(sn));
	
	t.tab_pos = (ac ? ra : rn).TopLeft();
	t.tab_size = (ac ? ra : rn).GetSize();

	const Value& sv = (cnt == 1 ? s.both : c == 0 ? s.first : c == cnt - 1 ? s.last : s.normal)[ndx];
	
	Image img = AlignValue(align, sv, t.tab_size);

	if(dragsample)
	{
		w.DrawImage(Rect(Point(0, 0), t.tab_size), img);
		rn = Rect(Fixed(Point(s.sel.left * ac, sel * ac + dy)), Fixed(sn));
	}
	else
	{
		w.DrawImage(Rect(t.tab_pos, t.tab_size), img);
		rn = Rect(Fixed(Point(pn.x, pn.y + dy)), Fixed(sn));
	}
	
	#ifdef TABBAR_DEBUG
	DrawFrame(w, rn, Green);
	#endif
		
	if (display)
		display->Paint(w, rn, t.value, s.text_color[ndx], SColorDisabled(), ndx);

	t.Clear();

	if(crosses && cnt > mintabcount && !dragsample) {
		TabItem& ti = t.items.Add();
		ti.img = s.crosses[(cross == n) ? 2 : ((ac || hl) ? 1 : 0)];
		ti.side = crosses_side;
		ti.cross = true;
		ti.size = s.crosses[0].GetSize();
		t.AddSpace(3, crosses_side);		
	}
	
	ComposeTab(t, s.font, s.text_color[ndx], ndx);
	
	if (stacking) {
		int ix = n + 1;

		while (ix < tabs.GetCount() && tabs[ix].stack == t.stack) {
			Tab &q = tabs[ix];			
			int ndx = !enable ? CTRL_DISABLED : 
					   highlight == ix ? CTRL_HOT : CTRL_NORMAL;

			int sn = t.items.GetCount();
			ComposeStackedTab(t, q, s.font, s.text_color[ndx], ndx);
			if(t.items.GetCount() > sn)
				for(; sn < t.items.GetCount(); sn++)
					t.items[sn].stacked_tab = ix;
			
			ix++;					
		}
	}
	
	PaintTabItems(t, w, rn, align);
}

void TabBar::Paint(Draw &w)
{
	LOG("Paint");
	int align = GetAlign();
	const Style &st = StyleDefault();
	Size ctrlsz = GetSize();
	Size sz = GetBarSize(ctrlsz);
	
	if (align == BOTTOM || align == RIGHT)
		w.Offset(ctrlsz.cx - sz.cx, ctrlsz.cy - sz.cy);
	
	#ifdef TABBAR_DEBUG
	w.DrawRect(sz, Yellow);
	#else
	w.DrawRect(sz, SColorFace());
	#endif

	IsVert() ? w.DrawRect(align == LEFT ? sz.cx - 1 : 0, 0, 1, sz.cy, Blend(SColorDkShadow, SColorShadow)):
		w.DrawRect(0, align == TOP ? sz.cy - 1 : 0, sz.cx, 1, Blend(SColorDkShadow, SColorShadow));	

	if (!tabs.GetCount()) return;
	
	int limt = sc.GetPos() + (IsVert() ? sz.cy : sz.cx);	
	int first = 0;
	int last = tabs.GetCount() - 1;
	// Find first visible tab
	for(int i = 0; i < tabs.GetCount(); i++) {
		Tab &tab = tabs[i]; 
		if (tab.pos.x + tab.size.cx > sc.GetPos()) {
			first = i;
			break;
		}
	}
	// Find last visible tab
	for(int i = first + 1; i < tabs.GetCount(); i++) { 
		if (tabs[i].visible && tabs[i].pos.x > limt) {
			last = i;
			break;
		}
	}
	// Draw active group
	for (int i = first; i <= last; i++) {
		if(tabs[i].visible && i != active)
			PaintTab(w, sz, i, IsEnabled());
	}
	// Clear tab_size for non-visible tabs to prevent mouse handling bugs
	for (int i = 0; i < first; i++)
		tabs[i].tab_size = Size(0, 0);	
	for (int i = last + 1; i < tabs.GetCount(); i++)
		tabs[i].tab_size = Size(0, 0);		
	// Draw inactive groups
	if (inactivedisabled)
		for (int i = first; i <= last; i++) {
			if(!tabs[i].visible && i != active && (!stacking || IsStackHead(i)))
				PaintTab(w, sz, i, !IsEnabled());
		}
			
	// Draw selected tab
	if(active >= first && active <= last)
		PaintTab(w, sz, active, true);
	
	// Separators
	if (grouping && groupseps) {
		int cy = IsVert() ? sz.cx : sz.cy;
		for (int i = 0; i < separators.GetCount(); i++) {
			int x = separators[i];
			if (x > sc.GetPos() && x < limt) {
				// Paint separator
				ChPaint(w, Rect(Fixed(Point(x - sc.GetPos() + GetStyle().sel.left, 0)), 
					Fixed(Size(TB_SPACE - GetStyle().sel.left, cy-1))), 
					st.group_separators[IsVert() ? 1 : 0]);						
			}
		}
	}
	
	// Draw drag highlights
	if(target >= 0)
	{
		// Draw target marker
		int drag = isctrl ? highlight : active;
		if(target != drag && target != GetNext(drag, true))
		{
			last = GetLast();
			first = GetFirst();
			int x = (target == last + 1 ? tabs[last].Right() : tabs[target].pos.x)
			        - sc.GetPos() - (target <= first ? 1 : 2)
			        + st.margin - (target > 0 ? st.extendleft : 0);

			if (IsHorz())
				DrawVertDrop(w, x + 1, 0, sz.cy);
			else
				DrawHorzDrop(w, 0, x + 1, sz.cx);
		}
		// Draw transparent drag image
		Point mouse = GetMousePos() - GetScreenRect().TopLeft();
		Size isz = dragtab.GetSize();
		int p = 0;
		int sep = TB_SBSEPARATOR * sc.IsVisible();
		
		int top = drag == active ? st.sel.bottom : st.sel.top;
		if (align == BOTTOM || align == RIGHT)
			p = int(drag == active) * -top + sep;
		else
			p = int(drag != active) * top;
		
		if (IsHorz())
			w.DrawImage(mouse.x - isz.cx / 2, p, isz.cx, isz.cy, dragtab);
		else
			w.DrawImage(p, mouse.y - isz.cy / 2, isz.cx, isz.cy, dragtab);
	}
	
	// If not in a frame fill any spare area
	if (!InFrame())
		w.DrawRect(GetClientArea(), SColorFace());
	
	if (align == BOTTOM || align == RIGHT)
		w.EndOp();	
}

Image TabBar::GetDragSample()
{
	int h = highlight;
	if(stacking)
		h = FindStackHead(tabs[h].stack);
	return GetDragSample(h);
}

Image TabBar::GetDragSample(int n)
{
	if (n < 0) return Image();
	Tab &t = tabs[n];

	Size tsz(t.tab_size);
	ImageDraw iw(tsz);
	iw.DrawRect(tsz, SColorFace()); //this need to be fixed - if inactive tab is dragged gray edges are visible
	
	PaintTab(iw, tsz, n, true, true);
	
	Image img = iw;
	ImageBuffer ib(img);
	Unmultiply(ib);
	RGBA *s = ~ib;
	RGBA *e = s + ib.GetLength();
	while(s < e) {
		s->a = 180;
		s++;
	}
	Premultiply(ib);
	return ib;
}

void TabBar::Scroll()
{
	Refresh();
}

int TabBar::GetWidth(int n)
{
	return GetStdSize(tabs[n]).cx + GetExtraWidth();
}

int TabBar::GetExtraWidth()
{
	return TB_MARGIN * 2 + (TB_SPACE + GetStyle().crosses[0].GetSize().cx) * crosses;	
}

Size TabBar::GetStdSize(const Value &q)
{
	if (display)
		return display->GetStdSize(q);
	else if (q.GetType() == STRING_V || q.GetType() == WSTRING_V)
		return GetTextSize(WString(q), GetStyle().font);
	else
		return GetTextSize("A Tab", GetStyle().font);	
}

Size TabBar::GetStackedSize(const Tab &t)
{
	if (!IsNull(t.img))
		return t.img.GetSize();
	return GetTextSize("...", GetStyle().font, 3);
}

Size TabBar::GetStdSize(const Tab &t)
{
	return (PaintIcons() && t.HasIcon()) ? (GetStdSize(t.value) + Size(TB_ICON+2, 0)) : GetStdSize(t.value);
}

TabBar& TabBar::Add(const Value &value, Image icon, String group, bool make_active)
{
	return InsertKey(tabs.GetCount(), value, value, icon, group, make_active);
}

TabBar& TabBar::Insert(int ix, const Value &value, Image icon, String group, bool make_active)
{
	return InsertKey(tabs.GetCount(), value, value, icon, group, make_active);
}

TabBar& TabBar::AddKey(const Value &key, const Value &value, Image icon, String group, bool make_active)
{
	return InsertKey(tabs.GetCount(), key, value, icon, group, make_active);
}

TabBar& TabBar::InsertKey(int ix, const Value &key, const Value &value, Image icon, String group, bool make_active)
{
	int id = InsertKey0(ix, key, value, icon, group);
	
	SortTabs0();
	MakeGroups();	
	Repos();
	active = -1;
	if (make_active || (!allownullcursor && active < 0)) 
		SetCursor((groupsort || stacking) ? FindId(id) : ( minmax(ix, 0, tabs.GetCount() - 1)));		
	return *this;	
}

int TabBar::InsertKey0(int ix, const Value &key, const Value &value, Image icon, String group)
{
	ASSERT(ix >= 0);
	int g = 0;
	if (!group.IsEmpty()) {
		g = FindGroup(group);
		if (g < 0) {
			NewGroup(group);
			g = groups.GetCount() - 1;
		}
	}	
	
	group = groups[g].name;
	Tab t;
	t.value = value;
	t.key = key;
	t.img = icon;
	t.id = GetNextId();
	t.group = Nvl(TrimBoth(group), "Unnamed Group");	
	if (stacking) {
		t.stackid = GetStackId(t);
		
		// Override index
		int tail = -1;
		for (int i = 0; i < tabs.GetCount(); i++) {
			if (tabs[i].stackid == t.stackid && (!grouping || tabs[i].group == t.group)) {
				tail = FindStackTail(tabs[i].stack);
				break;
			}
		}
		if (tail >= 0) {
			ix = tail+1;
			t.stack = tabs[tail].stack;
			tail++;
		}
		else {
			ix = (ix < tabs.GetCount()) ? FindStackHead(tabs[ix].stack) : ix;
			t.stack = stackcount++;
		}
		tabs.Insert(ix, t);
		if (tail >= 0)
			SortStack(t.stack, FindStackHead(t.stack), ix);	
			
	}
	else
		tabs.Insert(ix, t);
	return t.id;
}

int TabBar::GetWidth() const
{
	if (!tabs.GetCount()) return 0;
	int ix = GetLast();
	const Style& s = StyleDefault();
	if (IsStackHead(ix)) 
		return tabs[ix].Right() + s.margin * 2;
	int stack = tabs[ix].stack;
	ix--;
	while (ix >= 0 && tabs[ix].stack == stack)
		ix--;
	return tabs[ix+1].Right() + s.margin * 2;
	
}

int TabBar::GetHeight(bool scrollbar) const
{
	return TabBar::GetStyleHeight() + TB_SBSEPARATOR * int(scrollbar);
}

int TabBar::GetStyleHeight()
{
	const Style& s = GetStyle();
	return s.tabheight + s.sel.top;
}

void TabBar::Repos()
{
	if(!tabs.GetCount())
		return;

	String g = GetGroupName();

	int j;
	bool first = true;
	j = 0;
	separators.Clear();
	for(int i = 0; i < tabs.GetCount(); i++)
		j = TabPos(g, first, i, j, false);
	if (inactivedisabled)
		for(int i = 0; i < tabs.GetCount(); i++)
			if (!tabs[i].visible)
				j = TabPos(g, first, i, j, true);
	SyncScrollBar();
}

Size TabBar::GetBarSize(Size ctrlsz) const
{
	return IsVert() ? Size(GetFrameSize() - scrollbar_sz * int(sc.IsShown()), ctrlsz.cy) 
			: Size(ctrlsz.cx, GetFrameSize() - scrollbar_sz * int(sc.IsShown()));
}

Rect TabBar::GetClientArea() const
{
	Rect rect = GetSize();
	switch (GetAlign()) {
		case TOP:
			rect.top += GetFrameSize();	
			break;
		case BOTTOM:
			rect.bottom -= GetFrameSize();	
			break;
		case LEFT:
			rect.left += GetFrameSize();	
			break;
		case RIGHT:
			rect.right -= GetFrameSize();	
			break;
	};
	return rect;	
}

int TabBar::TabPos(const String &g, bool &first, int i, int j, bool inactive)
{
	bool ishead = IsStackHead(i);
	bool v = IsNull(g) ? true : g == tabs[i].group;
	Tab& t = tabs[i];

	if(ishead && (v || inactive))
	{
		t.visible = v;
		t.pos.y = 0;
		t.size.cy = GetStyleHeight();
				
		// Normal visible or inactive but greyed out tabs
		t.pos.x = first ? 0 : tabs[j].Right();
		
		// Separators
		if (groupseps && grouping && !first && t.group != tabs[j].group) {
			separators.Add(t.pos.x);
			t.pos.x += TB_SPACE;
		}
		
		int cx = GetStdSize(t).cx;

		// Stacked/shortened tabs
		if (stacking) {
			for(int n = i + 1; n < tabs.GetCount() && tabs[n].stack == t.stack; n++)
				cx += GetStackedSize(tabs[n]).cx;	
		}
			
		t.size.cx = cx + GetExtraWidth();

		if (stacking) {
			for(int n = i + 1; n < tabs.GetCount() && tabs[n].stack == t.stack; n++) {
				Tab &q = tabs[n];
				q.visible = false;
				q.pos = t.pos;
				q.size = t.size;
			}
		}
		
		j = i;
		first = false;
	}
	else if (!(v || inactive)) {
		t.visible = false;
		t.pos.x = sc.GetTotal() + GetBarSize(GetSize()).cx;
	}
	return j;	
}

void TabBar::ShowScrollbarFrame(bool b)
{
	SetFrameSize((b ? sc.GetFrameSize() : TB_SBSEPARATOR) + GetHeight(b), false);
	sc.Show(b);
	RefreshParentLayout();
}

void TabBar::SyncScrollBar(bool synctotal)
{
	if (synctotal)
		sc.SetTotal(GetWidth());			
	if (autoscrollhide) {
		bool v = sc.IsScrollable();
		if (sc.IsShown() != v) {
			PostCallback(THISBACK1(ShowScrollbarFrame, v));
		}
	}
	else {
		SetFrameSize(sc.GetFrameSize() + GetHeight(true), false);
		sc.Show();
	}
}

int TabBar::FindId(int id) const
{
	for(int i = 0; i < tabs.GetCount(); i++)
		if(tabs[i].id == id)
			return i;
	return -1;
}

int TabBar::GetNext(int n, bool drag) const
{
	for(int i = n + 1; i < tabs.GetCount(); i++)
		if(tabs[i].visible)
			return i;
	return drag ? tabs.GetCount() : -1;
}

int TabBar::GetPrev(int n, bool drag) const
{
	for(int i = n - 1; i >= 0; i--)
		if(tabs[i].visible)
			return i;
	return -1;
}

void TabBar::Clear()
{
	highlight = -1;
	active = -1;
	target = -1;
	cross = -1;	
	stackcount = 0;
	tabs.Clear();
	groups.Clear();
	NewGroup(t_("TabBarGroupAll\aAll"));
	group = 0;
	Refresh();
}

TabBar& TabBar::Crosses(bool b, int side)
{
	crosses = b;
	crosses_side = side;
	Repos();
	Refresh();
	return *this;
}

TabBar& TabBar::SortTabs(bool b)
{
	tabsort = b;
	if (b)
		DoTabSort(*tabsorter);	
	return *this;
}

TabBar& TabBar::SortTabsOnce()
{
	DoTabSort(*tabsorter);
	return *this;	
}

TabBar& TabBar::SortTabsOnce(TabSort &sort)
{
	DoTabSort(sort);
	return *this;	
}

TabBar& TabBar::SortTabs(TabSort &sort)
{
	tabsorter = &sort;
	return SortTabs(true);	
}

TabBar& TabBar::SortTabValues(ValueOrder &sort)
{
	valuesorter_inst.vo = &sort;
	tabsorter = &valuesorter_inst;
	return SortTabs(true);	
}

TabBar& TabBar::SortTabValuesOnce(ValueOrder &sort)
{
	TabValueSort q;
	q.vo = &sort;
	DoTabSort(q);
	return *this;	
}

TabBar& TabBar::SortTabKeys(ValueOrder &sort)
{
	keysorter_inst.vo = &sort;
	tabsorter = &keysorter_inst;
	return SortTabs(true);	
}

TabBar& TabBar::SortTabKeysOnce(ValueOrder &sort)
{
	TabKeySort q;
	q.vo = &sort;
	DoTabSort(q);
	return *this;		
}

TabBar& TabBar::SortGroups(bool b)
{
	groupsort = b;
	if (!b) return *this;;
	
	Value v = GetData();
	MakeGroups();
	Repos();
	if (!IsNull(v))
		SetData(v);
	Refresh();
	return *this;	
}

TabBar& TabBar::SortGroupsOnce()
{
	if (!grouping) return *this;;
	
	Value v = GetData();
	MakeGroups();
	Repos();
	if (!IsNull(v))
		SetData(v);
	Refresh();
	return *this;	
}

TabBar& TabBar::SortGroupsOnce(TabSort &sort)
{
	TabSort *current = groupsorter;
	groupsorter = &sort;
	SortGroupsOnce();
	groupsorter = current;	
	return *this;
}

TabBar& TabBar::SortGroups(TabSort &sort)
{
	groupsorter = &sort;
	return SortGroups(true);	
}

TabBar& TabBar::SortStacks(bool b)
{
	stacksort = b;
	if (stacking) {
		DoStacking();
		Refresh();
	}
	return *this;
}

TabBar& TabBar::SortStacksOnce()
{
	if (stacking) {
		DoStacking();
		Refresh();
	}
	return *this;
}

TabBar& TabBar::SortStacksOnce(TabSort &sort)
{
	TabSort *current = stacksorter;
	stacksorter = &sort;
	SortStacksOnce();
	stacksorter = current;
	return *this;
}

TabBar& TabBar::SortStacks(TabSort &sort)
{
	stacksorter = &sort;
	return SortStacks(true);	
}

TabBar& TabBar::SortStacks(ValueOrder &sort)
{
	stacksorter_inst.vo = &sort;
	stacksorter = &stacksorter_inst;
	return SortStacks(true);	
}

void TabBar::DoTabSort(TabSort &sort)
{
	Value v = GetData();
	StableSort(tabs, sort);
	Repos();
	if (!IsNull(v))
		SetData(v);
	Refresh();
}

void TabBar::SortTabs0()
{
	if (tabsort)
		StableSort(tabs, *tabsorter);
}

TabBar& TabBar::Grouping(bool b)
{
	grouping = b;
	Repos(); 
	Refresh();	
	return *this;
}

TabBar& TabBar::ContextMenu(bool b)
{
	contextmenu = b;
	return *this;
}

TabBar& TabBar::GroupSeparators(bool b)
{
	groupseps = b;
	Repos();
	Refresh();
	return *this;
}

TabBar& TabBar::AutoScrollHide(bool b)
{
	autoscrollhide = b;
	sc.Hide();
	SetFrameSize(GetHeight(false), false);
	SyncScrollBar(GetWidth());
	return *this;
}

TabBar& TabBar::InactiveDisabled(bool b)
{
	inactivedisabled = b; 
	Repos(); 
	Refresh();	
	return *this;
}

TabBar& TabBar::AllowNullCursor(bool b)
{
	allownullcursor = b;
	return *this;
}

TabBar& TabBar::Icons(bool v)
{
	icons = v;
	Repos();
	Refresh();
	return *this;
}

TabBar& TabBar::Stacking(bool b)
{
	stacking = b;	
	if (b)
		DoStacking();
	else
		DoUnstacking();
	Refresh();
	return *this;
}

void TabBar::FrameSet()
{
	int al = GetAlign();
	Ctrl::ClearFrames();
	sc.Clear();
	sc.SetFrameSize(scrollbar_sz).SetAlign((al >= 2) ? al - 2 : al + 2);
	sc <<= THISBACK(Scroll);
	sc.Hide();
	
	if (sc.IsChild()) sc.Remove();
	switch (al) {
		case LEFT:
			Ctrl::Add(sc.LeftPos(GetHeight(), scrollbar_sz).VSizePos());
			break;
		case RIGHT:
			Ctrl::Add(sc.RightPos(GetHeight(), scrollbar_sz).VSizePos());
			break;
		case TOP:
			Ctrl::Add(sc.TopPos(GetHeight(), scrollbar_sz).HSizePos());
			break;
		case BOTTOM:
			Ctrl::Add(sc.BottomPos(GetHeight(), scrollbar_sz).HSizePos());
			break;			
	};

	SyncScrollBar(true);
}

TabBar& TabBar::SetScrollThickness(int sz)
{
	scrollbar_sz = max(sz + 2, 3);
	FrameSet(); 
	RefreshLayout();
	return *this;	
}

void TabBar::Layout()
{
	if (autoscrollhide && tabs.GetCount()) 
		SyncScrollBar(false); 
}

int TabBar::FindValue(const Value &v) const
{
	for (int i = 0; i < tabs.GetCount(); i++)
		if (tabs[i].value == v)
			return i;
	return -1;
}

int TabBar::FindKey(const Value &v) const
{
	for (int i = 0; i < tabs.GetCount(); i++)
		if (tabs[i].key == v)
			return i;
	return -1;
}

bool TabBar::IsStackHead(int n) const
{
	return tabs[n].stack < 0 
		|| n == 0 
		|| (n > 0 && tabs[n - 1].stack != tabs[n].stack);
}

bool TabBar::IsStackTail(int n) const
{
	return tabs[n].stack < 0
		|| n >= tabs.GetCount() - 1
		|| (n < tabs.GetCount() && tabs[n + 1].stack != tabs[n].stack);
}

int TabBar::FindStackHead(int stackix) const
{
	int i = 0;
	while (tabs[i].stack != stackix) 
		i++;
	return i;
}

int TabBar::FindStackTail(int stackix) const
{
	int i = tabs.GetCount() - 1;
	while (tabs[i].stack != stackix) 
		i--;
	return i;
}

int TabBar::SetStackHead(Tab &t)
// Returns index of stack head
{
	ASSERT(stacking);
	int id = t.id;
	int stack = t.stack;
	int head = FindStackHead(stack);
	while (tabs[head].id != id)
		CycleTabStack(head, stack);
	return head;
}

int TabBar::CycleTabStack(int n)
// Returns index of stack head
{
	int head = FindStackHead(n);
	CycleTabStack(head, n);
	return head;
}

void TabBar::CycleTabStack(int head, int n)
{
	// Swap tab to end of stack
	int ix = head;
	while (!IsStackTail(ix)) {
		tabs.Swap(ix, ix + 1);
		++ix;
	}
}

Value TabBar::GetData() const
{
	return (HasCursor() && active < GetCount())
		? GetKey(active)
		: Value();
}

void TabBar::SetData(const Value &key)
{
	int n = FindKey(key); 
	if (n >= 0) {
		if (stacking && tabs[n].stack >= 0)
			n = SetStackHead(tabs[n]);
		SetCursor(n);
	}
}

void TabBar::Set(int n, const Value &newkey, const Value &newvalue)
{
	Set(n, newkey, newvalue, tabs[n].img);
}

void TabBar::Set(int n, const Value &newkey, const Value &newvalue, Image icon)
{
	ASSERT(n >= 0 && n < tabs.GetCount());
	tabs[n].key = newkey;
	tabs[n].value = newvalue;
	tabs[n].img = icon;
	if (stacking) {
		String id = tabs[n].stackid;
		tabs[n].stackid = GetStackId(tabs[n]);
		if (tabs[n].stackid != id) {
			tabs.Remove(n);
			InsertKey0(GetCount(), newkey, newvalue, tabs[n].img, tabs[n].group);
		}
	}
	Repos();
	Refresh();
}

void TabBar::SetValue(const Value &key, const Value &newvalue)
{
	Set(FindKey(key), key, newvalue);
}

void TabBar::SetValue(int n, const Value &newvalue)
{
	Set(n, tabs[n].key, newvalue);
}

void TabBar::SetKey(int n, const Value &newkey)
{
	Set(n, newkey, tabs[n].value);	
}

void TabBar::SetIcon(int n, Image icon)
{
	ASSERT(n >= 0 && n < tabs.GetCount());
	tabs[n].img = icon;
	Repos();
	Refresh();
}

void TabBar::LeftDown(Point p, dword keyflags)
{
	if(keyflags & K_SHIFT)
	{
		highlight = -1;
		Refresh();
		SetCapture();
		Fix(p);
		oldp = p;
		return;
	}

	isctrl = keyflags & K_CTRL;
	if(isctrl)
		return;

	if(cross != -1) {
		Value v = tabs[cross].key;
		Vector<Value>vv;
		vv.Add(v);
		int ix = cross;
		if (!CancelClose(v) && !CancelCloseSome(Vector<Value>(vv, 0))) {
			Close(ix);
			WhenClose(v);
			WhenCloseSome(vv);
		}
	}
	else if(highlight >= 0) {
		if (stacking && highlight == active) {
			CycleTabStack(tabs[active].stack);
			Repos();
			CursorChanged();
			UpdateActionRefresh();
		}
		else
			SetCursor(highlight);
	}
}

void TabBar::LeftUp(Point p, dword keyflags)
{
	ReleaseCapture();
}

void TabBar::LeftDouble(Point p, dword keysflags)
{
	WhenLeftDouble();
}

void TabBar::RightDown(Point p, dword keyflags)
{
	if (contextmenu)
		MenuBar::Execute(THISBACK(ContextMenu), GetMousePos());
}

void TabBar::MiddleDown(Point p, dword keyflags)
{
	if (highlight >= 0)
	{
		Value v = tabs[highlight].key;
		Vector<Value>vv;
		vv.Add(v);
		if (!CancelClose(v) && ! CancelCloseSome(Vector<Value>(vv, 0))) {
			Value v = tabs[highlight].key;
			Close(highlight);
			WhenClose(v);
			WhenCloseSome(vv);
		}
	}
}

void TabBar::MiddleUp(Point p, dword keyflags)
{
}

int TabBar::GetTargetTab(Point p)
{
	p.x += sc.GetPos();

	int f = GetFirst();
	int l = GetLast();
	
	if(tabs[f].visible && p.x < tabs[f].pos.x + tabs[f].size.cx / 2)
		return f;

	int t = -1;
	
	for(int i = l; i >= f; i--)
		if(tabs[i].visible && p.x >= tabs[i].pos.x + tabs[i].size.cx / 2)
		{
			t = i;
			break;
		}
	
	if(stacking)
		l = FindStackHead(tabs[l].stack);
		
	if(t == l)
		t = tabs.GetCount();
	else
	 	t = GetNext(t);

	return t;
}

void TabBar::MouseWheel(Point p, int zdelta, dword keyflags)
{
	sc.AddPos(-zdelta / 4, true);
	Scroll();
	MouseMove(p, 0);
}

bool TabBar::ProcessMouse(int i, const Point& p)
{
	if(i >= 0 && i < tabs.GetCount() && tabs[i].HasMouse(p))
	{
		if (stacking && ProcessStackMouse(i, p))
			return true;
		bool iscross = crosses ? tabs[i].HasMouseCross(p) : false;
		if(highlight != i || (iscross && cross != i || !iscross && cross == i))
		{
			cross = iscross ? i : -1;
			SetHighlight(i);
		}
		return true;
	}
	return false;
}

bool TabBar::ProcessStackMouse(int i, const Point& p)
{
	int j = i + 1;
	while (j < tabs.GetCount() && tabs[j].stack == tabs[i].stack) {
		if (Rect(tabs[j].tab_pos, tabs[j].tab_size).Contains(p)) {
			cross = -1;
			if (highlight != j)
				SetHighlight(j);
			return true;
		}
		j++;
	}
	return false;	
}

void TabBar::SetHighlight(int n)
{
	highlight = n;
	WhenHighlight();
	Refresh();	
}

void TabBar::MouseMove(Point p, dword keyflags)
{
	if(HasCapture())
	{
		Fix(p);
		sc.AddPos(p.x - oldp.x, true);
		oldp = p;
		Refresh();
		return;
	}
	
	if(ProcessMouse(active, p))
		return;
		
	for(int i = 0; i < tabs.GetCount(); i++)
	{
		if(i == active)
			continue;
		
		if(ProcessMouse(i, p))
			return;
	}

	if(highlight >= 0 || cross >= 0)
	{
		highlight = cross = -1;
		WhenHighlight();
		Refresh();
	}
}

void TabBar::MouseLeave()
{
	if(isdrag)
		return;
	highlight = cross = -1;
	WhenHighlight();
	Refresh();
}

void TabBar::DragAndDrop(Point p, PasteClip& d)
{
	Fix(p);
	int c = GetTargetTab(p);
	int tab = isctrl ? highlight : active;

	if (&GetInternal<TabBar>(d) != this || tabsort || c < 0 || !allowreorder) return;

	if (stacking) {
		tab = FindStackHead(tabs[tab].stack);
		if(c < tabs.GetCount())
			c = FindStackHead(tabs[c].stack);
	}

	bool sametab = c == tab || c == GetNext(tab, true);
	bool internal = AcceptInternal<TabBar>(d, "tabs");

	if(!sametab && internal && d.IsAccepted())
	{
		int id = active >= 0 ? tabs[active].id : -1;
		
		// Count stack
		int count = 1;
		if (stacking) {
			int ix = tab + 1;
			int stack = tabs[tab].stack;
			while (ix < tabs.GetCount() && tabs[ix].stack == stack)
				ix++;
			count = ix - tab;
		}
		// Copy tabs
		Vector<Tab> stacktemp;
		stacktemp.SetCount(count);
		//Copy(&stacktemp[0], &tabs[tab], count);
		for(int i = 0; i < count; i++)
			stacktemp[i].Set(tabs[tab + i]);
		// Remove
		tabs.Remove(tab, count);
		if (tab < c)
			c -= count;
		// Re-insert
		tabs.InsertPick(c, stacktemp);
		
		active = id >= 0 ? FindId(id) : -1;
		isdrag = false;
		target = -1;
		MakeGroups();
		Repos();
		Refresh();
		Sync();
		MouseMove(p, 0);
	}
	else if(isdrag)
	{
		if(internal)
		{
			target = -1;
			isdrag = false;
		}
		else
			target = c;
		Refresh();
	}
}

void TabBar::CancelMode()
{
	isdrag = false;
	target = -1;
	Refresh();
}

void TabBar::LeftDrag(Point p, dword keyflags)
{
	if(keyflags & K_SHIFT)
		return;
	if(highlight < 0)
		return;

	isdrag = true;
	dragtab = GetDragSample();
	DoDragAndDrop(InternalClip(*this, "tabs"));
}

void TabBar::DragEnter()
{
}

void TabBar::DragLeave()
{
	target = -1;
	Refresh();
}

void TabBar::DragRepeat(Point p)
{
	if(target >= 0)
	{
		Point dx = GetDragScroll(this, p, 16);
		Fix(dx);
		if(dx.x != 0)
			sc.AddPos(dx.x);
	}
}

bool TabBar::SetCursor0(int n)
{
	if(tabs.GetCount() == 0)
		return false;

	if(n < 0)
	{
		n = max(0, FindId(GetGroupActive()));
		active = -1;
		if (allownullcursor)
			return true;
	}

	bool is_all = IsGroupAll();
	bool same_group = tabs[n].group == GetGroupName();

	if((same_group || is_all) && active == n)
		return false;

	bool repos = false;
	if (!IsStackHead(n)) 
	{
		n = SetStackHead(tabs[n]);
		repos = true;		
	}
	active = n;
	if(!is_all && !same_group) 
	{
		SetGroup(tabs[n].group);
		repos = true;
	}
	if (repos)
		Repos();

	SetGroupActive(tabs[n].id);

	int cx = tabs[n].pos.x - sc.GetPos();
	if(cx < 0)
		sc.AddPos(cx - 10);
	else
	{
		Size sz = Ctrl::GetSize();
		Fix(sz);
		cx = tabs[n].pos.x + tabs[n].size.cx - sz.cx - sc.GetPos();
		if(cx > 0)
			sc.AddPos(cx + 10);
	}

	Refresh();

	if(Ctrl::HasMouse())
	{
		Sync();
		MouseMove(GetMouseViewPos(), 0);
	}
	return true;
}

void TabBar::SetCursor(int n)
{
	if (SetCursor0(n)) {
		CursorChanged();
		UpdateAction();
	}
}

void TabBar::SetTabGroup(int n, const String &group)
{
	ASSERT(n >= 0 && n < tabs.GetCount());
	int g = FindGroup(group);
	if (g <= 0) 
		NewGroup(group);
	else if (groups[g].active == tabs[n].id)
		SetGroupActive(tabs[n].id);
	tabs[n].group = group;
	MakeGroups();
	Repos();
}

void TabBar::Close(int n)
{
	if(tabs.GetCount() <= mintabcount)
		return;

	if(n == active)
	{
		int c = FindId(tabs[n].id);
		int nc = GetNext(c);
		if(nc < 0)
			nc = max(0, GetPrev(c));
		SetGroupActive(tabs[nc].id);
	}
	sc.AddTotal(-tabs[n].size.cx);
	tabs.Remove(n);
	MakeGroups();
	Repos();
	
	highlight = min(highlight, tabs.GetCount()-1);
	if(n == active)
		SetCursor(-1);
	else {
		if (n < active)
			active--;
		Refresh();
		if (n == highlight && Ctrl::HasMouse()) {
			Sync();
			MouseMove(GetMouseViewPos(), 0);
		}	
	}
}

void TabBar::CloseKey(const Value &key)
{
	int tabix = FindKey(key);
	if (tabix < 0) return;
	Close(tabix);
}

TabBar::Style& TabBar::Style::DefaultCrosses()
{
	crosses[0] = TabBarImg::CR0();
	crosses[1] = TabBarImg::CR1();
	crosses[2] = TabBarImg::CR2();
	return *this;
}

TabBar::Style& TabBar::Style::Variant1Crosses()
{
	crosses[0] = TabBarImg::VARIANT1_CR0();
	crosses[1] = TabBarImg::VARIANT1_CR1();
	crosses[2] = TabBarImg::VARIANT1_CR2();
	return *this;	
}

TabBar::Style& TabBar::Style::GroupSeparators(Value horz, Value vert)
{
	group_separators[0] = horz;
	group_separators[0] = vert;
	return *this;
}

TabBar::Style& TabBar::Style::DefaultGroupSeparators()
{
	return GroupSeparators(TabBarImg::SEP(), TabBarImg::SEPV());	
}

Vector<Value> TabBar::GetKeys() const
{
	Vector<Value> keys;
	keys.SetCount(tabs.GetCount());
	for (int i = 0; i < tabs.GetCount(); i++)
		keys[i] = tabs[i].key;
	return keys;
}

Vector<Image> TabBar::GetIcons() const
{
	Vector<Image> img;
	img.SetCount(tabs.GetCount());
	for (int i = 0; i < tabs.GetCount(); i++)
		img[i] = tabs[i].img;
	return img;
}

TabBar& TabBar::CopyBaseSettings(const TabBar& src)
{
	crosses = src.crosses;
	crosses_side = src.crosses_side;
	grouping = src.grouping;
	contextmenu = src.contextmenu;
	autoscrollhide = src.autoscrollhide;		
	nosel = src.nosel;
	nohl = src.nohl;
	inactivedisabled = src.inactivedisabled;
	stacking = src.stacking;
	groupsort = src.groupsort;
	groupseps = src.groupseps;
	tabsort = src.tabsort;
	allownullcursor = src.allownullcursor;
	icons = src.icons;
	mintabcount = src.mintabcount;
	return *this;
}

TabBar& TabBar::CopySettings(const TabBar &src)
{
	
	CopyBaseSettings(src);
	
	if (stacking != src.stacking)
		Stacking(src.stacking);
	else {
		MakeGroups();
		Repos();
		Refresh();
	}
	return *this;
}

void TabBar::Serialize(Stream& s)
{
	int version = 1;
	s / version;

	s % id;		
	s % crosses;
	s % crosses_side;
	s % grouping;
	s % autoscrollhide;
	s % nosel;
	s % nohl;
	s % inactivedisabled;
	s % stacking;
	s % groupsort;
	s % groupseps;
	s % tabsort;
	s % allownullcursor;
	s % icons;
	s % mintabcount;
	s % active;
	
	cross = -1;
	highlight = -1;
	target = -1;
	
	int n = groups.GetCount();
	s % n;
	groups.SetCount(n);
	
	for(int i = 0; i < groups.GetCount(); i++)
		s % groups[i];
	
	n = tabs.GetCount();
	s % n;
	tabs.SetCount(n);
	
	for(int i = 0; i < tabs.GetCount(); i++)
		s % tabs[i];
	
	int g = GetGroup();
	s % g;
	SetGroup(g);
	Repos();
}

CH_STYLE(TabBar, Style, StyleDefault)
{
	Assign(TabCtrl::StyleDefault());
	DefaultCrosses().DefaultGroupSeparators();
}

END_UPP_NAMESPACE
