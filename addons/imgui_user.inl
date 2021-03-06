// requires:
// defining IMGUI_INCLUDE_IMGUI_USER_H and IMGUI_INCLUDE_IMGUI_USER_INL
// at the project level

#pragma once
#ifndef IMGUI_USER_ADDONS_INL_
#define IMGUI_USER_ADDONS_INL_

#ifndef NO_IMGUISTRING
#include "./imguistring/imguistring.cpp"
#endif //NO_IMGUISTRING
#ifndef NO_IMGUIHELPER
#include "./imguihelper/imguihelper.cpp"
#endif //NO_IMGUIHELPER
#ifndef NO_IMGUITABWINDOW
#include "./imguitabwindow/imguitabwindow.cpp"
#endif //NO_IMGUITABWINDOW

#ifdef IMGUI_USE_AUTO_BINDING	// defined in imgui_user.h
#	ifdef __EMSCRIPTEN__
#	include <emscripten.h>		// wip: doesn't work by itself (ATM I'm currently preparing it for use with IMGUI_USE_SDL2_BINDING only)
#	endif //__EMSCRIPTEN__
#	ifdef IMGUI_USE_GLUT_BINDING
#		include "./imguibindings/ImImpl_Binding_Glut.h"
#	elif IMGUI_USE_SDL2_BINDING
#		include "./imguibindings/ImImpl_Binding_SDL2.h"
#	elif IMGUI_USE_GLFW_BINDING
#		include "./imguibindings/ImImpl_Binding_Glfw3.h"
#	elif IMGUI_USE_DIRECT3D9_BINDING
#		include "./imguibindings/ImImpl_Binding_Direct3D9.h"
#	elif (defined(_WIN32) || defined(IMGUI_USE_WINAPI_BINDING))
#		include "./imguibindings/ImImpl_Binding_WinAPI.h"
#	else // IMGUI_USE_SOME_BINDING
#		include "./imguibindings/ImImpl_Binding_Glfw3.h"
#	endif // IMGUI_USE_SOME_BINDING
#	include "./imguibindings/imguibindings.cpp"
#endif //IMGUI_USE_AUTO_BINDING

#ifdef IMGUI_USE_MINIZIP	// requires linking to library -lZlib
//extern "C" {
#include "./imguifilesystem/minizip/ioapi.c"
#include "./imguifilesystem/minizip/unzip.c"
#include "./imguifilesystem/minizip/zip.c"
//}
#endif //IMGUI_USE_MINIZIP

#ifdef __EMSCRIPTEN__
#   ifndef NO_IMGUIEMSCRIPTEN
#       include "./imguiemscripten/imguiemscripten.cpp"
#   endif //NO_IMGUIEMSCRIPTEN
#else //__EMSCRIPTEN__
#   undef NO_IMGUIEMSCRIPTEN
#   define NO_IMGUIEMSCRIPTEN
#endif //__EMSCRIPTEN__


#ifndef NO_IMGUILISTVIEW
#include "./imguilistview/imguilistview.cpp"
#endif //NO_IMGUILISTVIEW
#ifndef NO_IMGUIFILESYSTEM
#include "./imguifilesystem/imguifilesystem.cpp"
#endif //NO_IMGUIFILESYSTEM
#ifndef NO_IMGUITOOLBAR
#include "./imguitoolbar/imguitoolbar.cpp"
#endif //NO_IMGUITOOLBAR
#ifndef NO_IMGUIPANELMANAGER
#include "./imguipanelmanager/imguipanelmanager.cpp"
#endif //NO_IMGUIPANELMANAGER
#ifndef NO_IMGUIVARIOUSCONTROLS
#include "./imguivariouscontrols/imguivariouscontrols.cpp"
#endif //NO_IMGUIVARIOUSCONTROLS
#ifndef NO_IMGUISTYLESERIALIZER
#include "./imguistyleserializer/imguistyleserializer.cpp"
#endif //NO_IMGUISTYLESERIALIZER
#ifndef NO_IMGUIDATECHOOSER
#include "./imguidatechooser/imguidatechooser.cpp"
#endif //NO_IMGUIDATECHOOSER
#ifndef NO_IMGUICODEEDITOR
#include "./imguicodeeditor/imguicodeeditor.cpp"
#endif //NO_IMGUICODEEDITOR
#ifdef IMGUISCINTILLA_ACTIVATED
#include "./imguiscintilla/imguiscintilla.cpp"
#endif //IMGUISCINTILLA_ACTIVATED
#ifndef NO_IMGUINODEGRAPHEDITOR
#include "./imguinodegrapheditor/imguinodegrapheditor.cpp"
#endif //NO_IMGUINODEGRAPHEDITOR

#ifdef YES_IMGUIPDFVIEWER
#include "./imguiyesaddons/imguipdfviewer.cpp"
#endif //YES_IMGUIPDFVIEWER
#ifdef YES_IMGUIDOCK
#include "./imguiyesaddons/imguidock.cpp"
#endif //YES_IMGUIDOCK
#ifdef YES_IMGUISDF
#include "./imguiyesaddons/imguisdf.cpp"
#endif //YES_IMGUISDF
#ifdef YES_IMGUISOLOUD
#include "./imguiyesaddons/imguisoloud.cpp" // This is huge. Better adding it as the last addon.
#endif //YES_IMGUISOLOUD

#endif //IMGUI_USER_ADDONS_INL_

