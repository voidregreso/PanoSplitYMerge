#include "framework.h"
#include "LogUtil.h"

void CDECL logd(const LPCSTR szFormat, ...) {
	static CHAR szBuffer[1024];
	static CHAR szB2[1024];
	static SYSTEMTIME st;

	va_list pArgList;
	va_start(pArgList, szFormat);
	_vsnprintf_s(szBuffer, sizeof(szBuffer) / sizeof(CHAR), szFormat, pArgList);
	va_end(pArgList);

	GetLocalTime(&st);

	sprintf_s(szB2, TEXT("%02d:%02d:%02d.%03d [%d] %s %s\n"),
		// current time
		st.wHour, st.wMinute, st.wSecond, st.wMilliseconds,
		// process id
		GetCurrentProcessId(),
		// debug log
		szDebugPrefix, szBuffer);
	OutputDebugString(szB2);
}