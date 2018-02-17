// tac
#include "tacLibrary/tacDefines.h"
#include "tacLibrary/imgui/imgui.h"
#include "tacLibrary/tacMath.h"
#include "tacLibrary/tacPlatformWin32.h"
#include "tacLibrary/tacFilesystem.h"
#include "tacLibrary/imgui/imgui.h"
#include "tacGraphics/tac3camera.h"
#include "tacGraphics/tacRenderer.h"
#include "tacGraphics/tacRendererDX11.h"

// std libs
#include <iostream>
#include <algorithm>
#include <string>
#include <sstream>
#include <iomanip>
#include <fstream>
#include <memory>

// window icon
#include "resource.h"

void RunGame( 
  HINSTANCE hInstance,
  int nCmdShow,
  const char* dllname,
  FixedString< DEFAULT_ERR_LEN >& unrecoverableErrors );

void HandleErrors(
  FixedString< DEFAULT_ERR_LEN >& unrecoverableErrors );
