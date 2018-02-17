#pragma once

// preprocessor variable enforcing type checking, see:
// https://support.microsoft.com/en-us/kb/83456
// this should be the only windows include in the program
#include <windows.h>
#include <atlbase.h> //ccomptr
#include <winnt.h>

#include "tacPlatform.h"

void AppendWin32Error(
  FixedString< DEFAULT_ERR_LEN >& errors,
  DWORD winErrorValue );

