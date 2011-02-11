#include "CtrlPropCommon.h"
#include <CtrlLib/CtrlLib.h>

bool PropSetMin(const Value& v, EditTime& o) { if(!IsNumber(v)) return false; o.MinMax(v, o.GetMax()); return true; }
bool PropGetMin(Value& v, const EditTime& o) { v = o.GetMin(); return true; }
bool PropSetMax(const Value& v, EditTime& o) { if(!IsNumber(v)) return false; o.MinMax(o.GetMin(), v); return true; }
bool PropGetMax(Value& v, const EditTime& o) { v = o.GetMax(); return true; }
bool PropSetNotNull(const Value& v, EditTime& o) { if(!IsNumber(v)) return false; o.NotNull(v); return true; }
bool PropGetNotNull(Value& v, const EditTime& o) { v = o.IsNotNull(); return true; }
bool PropSetSeconds(const Value& v, EditTime& o) { if(!IsNumber(v)) return false; o.Seconds(v); return true; }
bool PropGetSeconds(Value& v, const EditTime& o) { v = o.IsSeconds(); return true; }

PROPERTIES(EditTime, EditField)
PROPERTY("min", PropSetMin, PropGetMin)
PROPERTY("max", PropSetMax, PropGetMax)
PROPERTY("notnull", PropSetNotNull, PropGetNotNull)
PROPERTY("seconds", PropSetSeconds, PropGetSeconds)
END_PROPERTIES

PROPS(Ctrl, EditTime, EditField)
