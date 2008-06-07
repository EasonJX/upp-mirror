#include "Common.h"

String GetCommonDir()
{
	return GetVar("COMMON");
}

String CommonPath(const String& filename)
{
	String s = GetCommonDir();
	if(s.IsEmpty())
		return ConfigFile(filename);
	return AppendFileName(s, filename);
}

String GetLocalDir()
{
	return ConfigFile("UppLocal");
}

String LocalPath(const String& filename)
{
	return AppendFileName(GetLocalDir(), filename);
}

String FollowCygwinSymlink(const String& file) {
	for(String fn = file;;) {
		if(fn.IsEmpty())
			return fn;
		FileIn fi(fn);
		if(!fi.IsOpen())
			return fn;
		char buffer[10];
		if(!fi.GetAll(buffer, 10) || memcmp(buffer, "!<symlink>", 10))
			return fn;
		fn = NormalizePath(LoadStream(fi), GetFileDirectory(fn));
	}
}

const char *SemiTextTest::Accept(const char *s) const {
	if(*s != ';') return NULL;
	s++;
	while(*s == ' ') s++;
	return s;
};

Vector<String> SplitDirs(const char *s) {
	return Split(s, Single<SemiTextTest>());
}

static GLOBAL_VARP(String, varsname, ("default"));
typedef VectorMap<String, String> sHT;
static GLOBAL_VAR(sHT, sVar);

String GetVarsName() {
	return varsname();
}

String VarFilePath(String name) {
	return ConfigFile(name + ".var");
}

String VarFilePath() {
	return VarFilePath(GetVarsName());
}

bool SaveVarFile(const char *filename, const VectorMap<String, String>& var)
{
	FileOut out(filename);
	for(int i = 0; i < var.GetCount(); i++)
		out << var.GetKey(i) << " = " << AsCString(var[i]) << ";\n";
	out.Close();
	return !out.IsError();
}

bool SaveVars(const char *name)
{
	if(!SaveVarFile(VarFilePath(name), sVar()))
		return false;
	varsname() = name;
	return true;
}

bool LoadVarFile(const char *name, VectorMap<String, String>& _var)
{
	try {
		VectorMap<String, String> var;
		String env = LoadFile(name);
		CParser p(env);
		while(!p.IsEof()) {
			String v = p.ReadId();
			p.Char('=');
			if(p.IsString())
				var.GetAdd(v) = p.ReadString();
			else {
				String ln;
				while(p.PeekChar() != '\r' && p.PeekChar() != '\n' && p.PeekChar() != ';')
					ln.Cat(p.GetChar());
				var.GetAdd(v) = ln;
				p.Spaces();
			}
			p.Char(';');
		}
		_var = var;
		return true;
	}
	catch(...) {
		return false;
	}
}

bool   LoadVars(const char *name) {
	varsname() = (name && *name ? name : "default");
	return LoadVarFile(VarFilePath(), sVar());
}

String GetVar(const String& var) {
	return sVar().Get(var, Null);
}

void SetVar(const String& var, const String& val) {
	sVar().GetAdd(var) = val;
	SaveVars(GetVarsName());
}

Vector<String> GetUppDirs() {
	return SplitDirs(GetVar("UPP"));
}

String GetUppDir() {
	Vector<String> s = GetUppDirs();
#ifdef PLATFORM_WIN32
	return s.GetCount() == 0 ? GetFileFolder(GetExeFilePath()) : s[0];
#endif
#ifdef PLATFORM_POSIX
	return s.GetCount() == 0 ? GetHomeDirectory() : s[0];
#endif
}

bool IsCSourceFile(const char *path)
{
	String ext = ToLower(GetFileExt(path));
	return ext == ".cpp" || ext == ".c" || ext == ".cc" || ext == ".cxx" || ext == ".icpp";
}

bool IsCHeaderFile(const char *path)
{
	String ext = ToLower(GetFileExt(path));
	return ext == ".h" || ext == ".hpp" || ext == ".hh" || ext == ".hxx";
}

bool IsFullDirectory(const String& d) {
	if(IsFullPath(d)) {
		FindFile ff(d);
		if(ff) return ff.IsDirectory();
	}
	return false;
}

String PackagePath(const String& name) {
	String uppfile = NativePath(name);
	if(IsFullPath(uppfile)) return NormalizePath(uppfile);
	Vector<String> d = GetUppDirs();
	String p;
	for(int i = 0; i < d.GetCount(); i++) {
		p = NormalizePath(AppendFileName(AppendFileName(d[i], uppfile),
		                  GetFileName(uppfile)) + ".upp");
		if(FileExists(p)) return p;
	}
	return d.GetCount() ? NormalizePath(AppendFileName(AppendFileName(d[0], uppfile),
		                                GetFileName(uppfile)) + ".upp") : "";
}

String SourcePath(const String& package, const String& file) {
	if(IsFullPath(file)) return NativePath(file);
	return NormalizePath(AppendFileName(GetFileFolder(PackagePath(package)), file));
}

String GetAnyFileName(const char *path)
{
	const char *p = path;
	for(char c; (c = *path++) != 0;)
		if(c == '\\' || c == '/' || c == ':')
			p = path;
	return p;
}

String GetAnyFileTitle(const char *path)
{
	String fn = GetAnyFileName(path);
	const char *p = fn;
	const char *e = p;
	while(*e && *e != '.')
		e++;
	return String(p, e);
}

String CatAnyPath(String path, const char *more)
{
	if(!more || !*more)
		return path;
	if(!path.IsEmpty() && *path.Last() != '\\' && *path.Last() != '/' &&
	*more != '\\' && *more != '/')
#ifdef PLATFORM_WIN32
		path.Cat('\\');
#else
		path.Cat('/');
#endif
	path.Cat(more);
	return path;
}


void SplitHostName(const char *hostname, String& host, int& port) {
	enum { DEFAULT_REMOTE_PORT = 2346 };
	const char *p = hostname;
	while(p && *p != ':')
		p++;
	host = String(hostname, p);
	if(*p++ == ':' && IsDigit(*p))
		port = stou(p);
	else
		port = DEFAULT_REMOTE_PORT;
}

Vector<String> SplitFlags0(const char *flags) {
	return Split(flags, CharFilterTextTest(CharFilterWhitespace));
}

Vector<String> SplitFlags(const char *flags, bool main, const Vector<String>& accepts)
{
	Vector<String> v = SplitFlags0(flags);
	Vector<String> h;
	for(int i = 0; i < v.GetCount(); i++)
		if(v[i][0] == '.') {
			String f = v[i].Mid(1);
			if(main || FindIndex(accepts, f) >= 0)
				h.Add(v[i].Mid(1));
		}
		else
			h.Add(v[i]);
	if(main)
		h.Add("MAIN");
	return h;
}

Vector<String> SplitFlags(const char *flags, bool main)
{
	Vector<String> dummy;
	return SplitFlags(flags, main, dummy);
}

bool HasFlag(const Vector<String>& conf, const char *flag) {
	return FindIndex(conf, flag) >= 0;
}

Vector<String> Combine(const Vector<String>& conf, const char *flags) {
	Vector<String> cfg(conf, 1);
	CParser p(flags);
	while(!p.IsEof()) {
		bool isnot = p.Char('!');
		if(!p.IsId()) break;
		String flg = p.ReadId();
		int i = FindIndex(cfg, flg);
		if(isnot) {
			if(i >= 0) cfg.Remove(i);
		}
		else
			if(i < 0) cfg.Add(flg);
	}
	return cfg;
}

String Gather(const Array<OptItem>& set, const Vector<String>& conf) {
	String s;
	for(int i = 0; i < set.GetCount(); i++)
		if(MatchWhen(set[i].when, conf)) {
			if(!s.IsEmpty()) s.Cat(' ');
			s.Cat(set[i].text);
		}
	return s;
}

int    GetType(const Vector<String>& conf, const char *flags) {
	Vector<String> f = SplitFlags(flags);
	int q = FLAG_UNDEFINED;
	for(int i = 0; i < f.GetCount(); i++)
		if(HasFlag(conf, f[i]))
			if(q == FLAG_UNDEFINED)
				q = i;
			else
				q = FLAG_MISMATCH;
	return q;
}

String RemoveType(Vector<String>& conf, const char *flags)
{
	String old;
	Index<String> f = SplitFlags(flags);
	for(int i = conf.GetCount(); --i >= 0;)
		if(f.Find(conf[i]) >= 0)
		{
			old = conf[i];
			conf.Remove(i);
		}
	return old;
}

int    GetType(const Vector<String>& conf, const char *flags, int def) {
	int q = GetType(conf, flags);
	return q == FLAG_UNDEFINED ? def : q;
}

bool   GetFlag(const Vector<String>& conf, const char *flag) {
	return FindIndex(conf, flag) >= 0;
}

void Workspace::AddLoad(const String& name, bool match, const Vector<String>& flag)
{
	package.Add(name).Load(PackagePath(name));
}

void Workspace::AddUses(Package& p, bool match, const Vector<String>& flag)
{
	int q = package.GetCount();
	for(int i = 0; i < p.uses.GetCount(); i++)
		if((!match || MatchWhen(p.uses[i].when, flag)) && package.Find(p.uses[i].text) < 0)
			AddLoad(p.uses[i].text, match, flag);
	for(int i = q; i < package.GetCount(); i++)
		AddUses(package[i], match, flag);
}

void Workspace::Scan(const char *prjname) {
	package.Clear();
	AddLoad(prjname, false, Vector<String>());
	AddUses(package[0], false, Vector<String>());
}

void Workspace::Scan(const char *prjname, const Vector<String>& flag) {
	package.Clear();
	AddLoad(prjname, true, flag);
	AddUses(package[0], true, flag);
}

void Workspace::Dump() {
	for(int i = 0; i < package.GetCount(); i++) {
		Package& prj = package[i];
		LOG("Package " << package.GetKey(i));
		LOG(" file ");
		for(int i = 0; i < prj.file.GetCount(); i++)
			LOG("\t" << prj.file[i]);
	}
}

Vector<String> Workspace::GetAllAccepts(int pk) const
{
	Index<String> accepts;
	Index<int> pkg;
	pkg.Add(pk);
	for(int i = 0; i < pkg.GetCount(); i++) {
		const Package& p = package[pkg[i]];
		FindAppend(accepts, p.accepts);
		for(int u = 0; u < p.uses.GetCount(); u++) {
			int f = package.Find(p.uses[u].text);
			if(f >= 0)
				pkg.FindAdd(f);
		}
	}
	return accepts.PickKeys();
}
