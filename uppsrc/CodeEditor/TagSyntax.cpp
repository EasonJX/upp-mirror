#include "CodeEditor.h"

NAMESPACE_UPP

void TagSyntax::Clear()
{
	hl_ink = INK_NORMAL;
	hl_paper = PAPER_NORMAL;
}

void TagSyntax::Put0(int ink, int n, int paper)
{
	if(hout)
		hout->Put(n, hl_style[ink], hl_style[paper]);
}

inline static bool IsXmlNameChar(int c)
{
	return IsAlNum(c) || c == '.' || c == '-' || c == '_' || c == ':';
}

const wchar *TagSyntax::Spaces(const wchar *s, const wchar *e)
{
	while(s < e && IsSpace(*s)) {
		s++;
		Put();
	}
	return s;
}

void TagSyntax::DoScript(const wchar *s, const wchar *e)
{
	if(hout)
		script.Highlight(s, e, *hout, NULL, 0, 0);
	else
		script.ScanSyntax(s, e, 0, 0);
}

inline
static const wchar *sSkipSpaces(const wchar *s, const wchar *e)
{
	while(s < e && IsSpace(*s))
		s++;
	return s;
}

inline bool cmp_wi(const wchar *s, const wchar *t, int len)
{
	while(len--) {
		if(ToUpper(*s++) != ToUpper(*t++))
			return false;
	}
	return true;
}

inline
bool IsEndTag(const wchar *s, const wchar *e, const wchar *tag, int len)
{
	if(*s != '<' || s >= e)
		return false;
	s = sSkipSpaces(s + 1, e);
	if(*s != '/' || s >= e)
		return false;
	s = sSkipSpaces(s + 1, e);
	return s < e && e - s >= len && cmp_wi(s, tag, len);
}

const wchar *IsScriptEnd(const wchar *s, const wchar *e, bool script)
{
	while(s < e) {
		int c = *s;
		if(c == '\"' || c == '\'') {
			s++;
			while(s < e && *s != c) {
				if(*s == '\\' && s + 1 < e)
					s += 2;
				else
					s++;
			}
		}
		else {
			static WString s_script("script");
			static WString s_style("style");
			const WString& tag = script ? s_script : s_style;
			if(IsEndTag(s, e, ~tag, tag.GetLength()))
				return s;
		}
		s++;
	}
	return NULL;
}

void TagSyntax::Do(const wchar *s, const wchar *e)
{
doscript:
	if(status == SCRIPT) { // TODO: Improve ending detection...
		const wchar *q = IsScriptEnd(s, e, script_type == JS);
		if(q) {
			DoScript(s, q);
			s = q;
			status = TEXT;
			Set(INK_NORMAL);
		}
		else {
			DoScript(s, e);
			return;
		}
	}
	while(s < e) {
		s = Spaces(s, e);
		if(s >= e)
			break;
		if(status == COMMENT) {
			if(s + 2 < e && s[0] == '-' && s[1] == '-' && s[2] == '>') {
				SetPut(INK_COMMENT, 3);
				s += 3;
				status = TEXT;
				Set(INK_NORMAL);
			}
			else {
				Put();
				s++;
			}
		}
		else
		if(*s == '&') {
			s++;
			Put0(INK_CONST_STRING, 1);
			while(s < e && *s != ';') {
				Put0(INK_CONST_STRING, 1);
				s++;
			}
		}
		else
		if(*s == '<') {
			tagname.Clear();
			s++;
			if(s + 2 < e && s[0] == '!' && s[1] == '-' && s[2] == '-') {
				SetPut(INK_COMMENT, 4);
				s += 3;
				status = COMMENT;
			}
			else
			if(*s == '!') {
				status = DECL;
				s++;
				SetPut(INK_MACRO, 2, PAPER_MACRO);
			}
			else
			if(*s == '?') {
				status = PI;
				s++;
				SetPut(INK_IFDEF, 2, PAPER_IFDEF);
			}
			else {
				status = TAG0;
				SetPut(INK_KEYWORD);
			}
		}
		else
		if(*s == '>') {
			s++;
			Put();
			Set(INK_NORMAL);
			status = TEXT;
			if(html) {
				if(tagname == "script") {
					script.SetHighlight(CSyntax::HIGHLIGHT_JAVASCRIPT);
					status = SCRIPT;
					script_type = JS;
					goto doscript;
				}
				else
				if(tagname == "style") {
					script.SetHighlight(CSyntax::HIGHLIGHT_CSS);
					status = SCRIPT;
					script_type = CSS;
					goto doscript;
				}
			}
		}
		else
		if(status == TAG0) {
			tagname.Clear();
			while(s < e && !IsSpace(*s) && *s != '>') {
				tagname.Cat(*s++);
				Put();
			}
			if(*tagname == '/')
				status = ENDTAG;
			else
				status = TAG;
		}
		else
		if(status == TAG) {
			int n = 0;
			while(s < e && !IsSpace(*s) && *s != '>' && *s != '=') {
				n++;
				s++;
			}
			Put0(INK_UPP, n);
			status = ATTR;
		}
		else
		if(status == ATTR) {
			if(*s == '=') {
				s++;
				Put();
			}
			s = Spaces(s, e);
			int c = *s;
			if(c == '\"' || c == '\'') {
				int n = 1;
				s++;
				while(s < e) {
					n++;
					if(*s++ == c)
						break;
				}
				Put0(INK_CONST_STRING, n);
			}
			else {
				int n = 0;
				while(s < e && !IsSpace(*s) && *s != '>') {
					s++;
					n++;
				}
				Put0(INK_CONST_FLOAT, n);
			}
			status = TAG;
		}
		else {
			s++;
			Put();
		}
	}
}

void TagSyntax::CheckSyntaxRefresh(CodeEditor& e, int pos, const WString& text)
{
	script.CheckSyntaxRefresh(e, pos, text);
	for(const wchar *s = text; *s; s++)
		if(*s == '<' || *s == '>' || *s == '/') {
			e.Refresh();
			return;
		}
	int l = max(pos - 6, 0);
	int h = min(pos + text.GetLength() + 6, e.GetLength());
	if(h - l > 200) {
		e.Refresh();
		return;
	}
	WString w = ToLower(e.GetW(l, h - l));
	if(w.Find("style") >= 0 || w.Find("script") >= 0)
		e.Refresh();
}

void TagSyntax::ScanSyntax(const wchar *s, const wchar *e, int, int)
{
	Do(s, e);
}

void TagSyntax::Highlight(const wchar *s, const wchar *end, HighlightOutput& hls, CodeEditor *editor, int line, int pos)
{
	hout = &hls;
	Do(s, end);
	hout = NULL;
}

void TagSyntax::Serialize(Stream& s)
{
	s % hl_ink
	  % hl_paper
	  % status
	  % tagname
	  % script
	  % script_type
	  % html;
}

END_UPP_NAMESPACE