//
// "$Id: $"
//
// WIN32 MSAA proxy header file for the Fast Light Tool Kit (FLTK).
//
// Copyright 2008 by Bill Spitzak and others.
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Library General Public
// License as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Library General Public License for more details.
//
// You should have received a copy of the GNU Library General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
// USA.
//
// Please report all bugs and problems on the following page:
//
//     http://www.fltk.org/str.php
//

#ifndef Fl_Msaa_Proxy_H
#define Fl_Msaa_Proxy_H

#if defined(WIN32) && (!defined(__GNUC__) || __GNUC__ >= 3)
#  include <windows.h>
#  include <oleacc.h>

// MSAA proxy for Fl_Widget object
class FL_EXPORT Fl_Msaa_Proxy : public IAccessible
{
private:
  LONG       mRefCount;		// Reference counter

public:
  Fl_Widget* mpObj;		// Pointer to widget being represented

  Fl_Msaa_Proxy(Fl_Widget* o) : mRefCount(0), mpObj(o)  {}
  void Detach()  {mpObj = 0;}

  // IUnknown implementation
  HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void**);
  ULONG STDMETHODCALLTYPE AddRef();
  ULONG STDMETHODCALLTYPE Release();

  // IDispatch implementation
  HRESULT STDMETHODCALLTYPE GetTypeInfoCount(UINT*);
  HRESULT STDMETHODCALLTYPE GetTypeInfo(UINT, LCID, ITypeInfo**);
  HRESULT STDMETHODCALLTYPE GetIDsOfNames(REFIID, LPOLESTR*,
      UINT, LCID, DISPID*);
  HRESULT STDMETHODCALLTYPE Invoke(DISPID, REFIID, LCID, WORD,
      DISPPARAMS*, VARIANT*, EXCEPINFO*, UINT*);

  // IAccessible implementation
  HRESULT STDMETHODCALLTYPE get_accParent(IDispatch**);
  HRESULT STDMETHODCALLTYPE get_accChildCount(long*);
  HRESULT STDMETHODCALLTYPE get_accChild(VARIANT, IDispatch**);
  HRESULT STDMETHODCALLTYPE get_accName(VARIANT, BSTR*);
  HRESULT STDMETHODCALLTYPE get_accValue(VARIANT, BSTR*);
  HRESULT STDMETHODCALLTYPE get_accDescription(VARIANT, BSTR*);
  HRESULT STDMETHODCALLTYPE get_accRole(VARIANT, VARIANT*);
  HRESULT STDMETHODCALLTYPE get_accState(VARIANT, VARIANT*);
  HRESULT STDMETHODCALLTYPE get_accHelp(VARIANT, BSTR*);
  HRESULT STDMETHODCALLTYPE get_accHelpTopic(BSTR*, VARIANT, long*);
  HRESULT STDMETHODCALLTYPE get_accKeyboardShortcut(VARIANT, BSTR*);
  HRESULT STDMETHODCALLTYPE get_accFocus(VARIANT*);
  HRESULT STDMETHODCALLTYPE get_accSelection(VARIANT*);
  HRESULT STDMETHODCALLTYPE get_accDefaultAction(VARIANT, BSTR*);
  HRESULT STDMETHODCALLTYPE accSelect(long, VARIANT);
  HRESULT STDMETHODCALLTYPE accLocation(long*, long*, long*, long*, VARIANT);
  HRESULT STDMETHODCALLTYPE accNavigate(long, VARIANT, VARIANT*);
  HRESULT STDMETHODCALLTYPE accHitTest(long, long, VARIANT*);
  HRESULT STDMETHODCALLTYPE accDoDefaultAction(VARIANT);
  HRESULT STDMETHODCALLTYPE put_accName(VARIANT, BSTR);
  HRESULT STDMETHODCALLTYPE put_accValue(VARIANT, BSTR);
};

// Utility functions for BSTR
extern FL_EXPORT void fl_str_to_bstr(const char* str, size_t len, BSTR* bstr);
extern FL_EXPORT char* fl_bstr_to_str(const BSTR bstr, size_t* pLen = 0);
extern FL_EXPORT char* fl_label_to_text(const char* label, size_t* pLen = 0);
extern FL_EXPORT char* fl_label_to_shortcut(const char* label, size_t* pLen = 0);
extern FL_EXPORT void fl_free_str(char* str);

// OLEACC.DLL symbols
typedef LRESULT (WINAPI* flTypeLresultFromObject)(REFIID, WPARAM, LPUNKNOWN);
extern FL_EXPORT flTypeLresultFromObject flLresultFromObject;
typedef HRESULT (WINAPI* flTypeAccessibleObjectFromWindow)(
    HWND, DWORD, REFIID, void**);
extern FL_EXPORT flTypeAccessibleObjectFromWindow flAccessibleObjectFromWindow;

#endif // WIN32 && (!__GNUC__ || __GNUC__ >= 3)

#endif

//
// End of "$Id: $".
//
