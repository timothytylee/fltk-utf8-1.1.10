//
// "$Id: Fl_Menu_.cxx 5190 2006-06-09 16:16:34Z mike $"
//
// Common menu code for the Fast Light Tool Kit (FLTK).
//
// Copyright 1998-2005 by Bill Spitzak and others.
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

// This is a base class for all items that have a menu:
//	Fl_Menu_Bar, Fl_Menu_Button, Fl_Choice
// This provides storage for a menu item, functions to add/modify/delete
// items, and a call for when the user picks a menu item.

// More code in Fl_Menu_add.cxx

#include <FL/Fl.H>
#include <FL/Fl_Menu_.H>
#include "flstring.h"
#include <stdio.h>
#include <stdlib.h>

// Set 'pathname' of specified menuitem
//    If finditem==NULL, mvalue() is used (the most recently picked menuitem)
//    Returns:
//       0 : OK
//      -1 : item not found (name="")
//      -2 : 'name' not large enough (name="")
//
#define SAFE_STRCAT(s) \
    { len += strlen(s); if ( len >= namelen ) { *name='\0'; return(-2); } else strcat(name,(s)); }
int Fl_Menu_::item_pathname(char *name, int namelen, const Fl_Menu_Item *finditem) const {
    int len = 0;
    finditem = finditem ? finditem : mvalue();    
    name[0] = '\0';
    for ( int t=0; t<size(); t++ ) {
        const Fl_Menu_Item *m = &(menu()[t]);
	if ( m->submenu() ) {				// submenu? descend
	    if (*name) SAFE_STRCAT("/");
	    if (m->label()) SAFE_STRCAT(m->label());
	} else {
	    if (m->label()) {				// menu item?
		if ( m == finditem ) {			// found? tack on itemname, done.
		    SAFE_STRCAT("/");
		    SAFE_STRCAT(m->label());
		    return(0);
		}
	    } else {					// end of submenu? pop
	        char *ss = strrchr(name, '/');
		if ( ss ) { *ss = 0; len = strlen(name); }	// "File/Edit" -> "File"
		else { name[0] = '\0'; len = 0; }		// "File" -> ""
		continue;
	    }
	}
    }
    *name = '\0';
    return(-1);						// item not found
}

// FIND MENU ITEM INDEX, GIVEN MENU PATHNAME
//     eg. "Edit/Copy"
//     Will also return submenus, eg. "Edit"
//     Returns NULL if not found.
//
const Fl_Menu_Item *
Fl_Menu_::find_item(const char *name)
{
  char menupath[1024] = "";	// File/Export

  for ( int t=0; t < size(); t++ ) {
    Fl_Menu_Item *m = menu_ + t;

    if (m->flags&FL_SUBMENU) {
      // IT'S A SUBMENU
      // we do not support searches through FL_SUBMENU_POINTER links
      if (menupath[0]) strlcat(menupath, "/", sizeof(menupath));
      strlcat(menupath, m->label(), sizeof(menupath));
      if (!strcmp(menupath, name)) return m;
    } else {
      if (!m->label()) {
	// END OF SUBMENU? Pop back one level.
	char *ss = strrchr(menupath, '/');
	if ( ss ) *ss = 0;
	else menupath[0] = '\0';
	continue;
      }

      // IT'S A MENU ITEM
      char itempath[1024];	// eg. Edit/Copy
      strcpy(itempath, menupath);
      if (itempath[0]) strlcat(itempath, "/", sizeof(itempath));
      strlcat(itempath, m->label(), sizeof(itempath));
      if (!strcmp(itempath, name)) return m;
    }
  }

  return (const Fl_Menu_Item *)0;
}

int Fl_Menu_::value(const Fl_Menu_Item* m) {
  clear_changed();
  if (value_ != m) {value_ = m; return 1;}
  return 0;
}

// When user picks a menu item, call this.  It will do the callback.
// Unfortunatly this also casts away const for the checkboxes, but this
// was necessary so non-checkbox menus can really be declared const...
const Fl_Menu_Item* Fl_Menu_::picked(const Fl_Menu_Item* v) {
  if (v) {
    if (v->radio()) {
      if (!v->value()) { // they are turning on a radio item
	set_changed();
	((Fl_Menu_Item*)v)->setonly();
      }
      redraw();
    } else if (v->flags & FL_MENU_TOGGLE) {
      set_changed();
      ((Fl_Menu_Item*)v)->flags ^= FL_MENU_VALUE;
      redraw();
    } else if (v != value_) { // normal item
      set_changed();
    }
    value_ = v;
    if (when()&(FL_WHEN_CHANGED|FL_WHEN_RELEASE)) {
      if (changed() || when()&FL_WHEN_NOT_CHANGED) {
	if (value_ && value_->callback_) value_->do_callback((Fl_Widget*)this);
	else do_callback();
      }
    }
  }
  return v;
}

// turn on one of a set of radio buttons
void Fl_Menu_Item::setonly() {
  flags |= FL_MENU_RADIO | FL_MENU_VALUE;
  Fl_Menu_Item* j;
  for (j = this; ; ) {	// go down
    if (j->flags & FL_MENU_DIVIDER) break; // stop on divider lines
    j++;
    if (!j->text || !j->radio()) break; // stop after group
    j->clear();
  }
  for (j = this-1; ; j--) { // go up
    if (!j->text || (j->flags&FL_MENU_DIVIDER) || !j->radio()) break;
    j->clear();
  }
}

Fl_Menu_::Fl_Menu_(int X,int Y,int W,int H,const char* l)
: Fl_Widget(X,Y,W,H,l) {
  set_flag(SHORTCUT_LABEL);
  box(FL_UP_BOX);
  when(FL_WHEN_RELEASE_ALWAYS);
  value_ = menu_ = 0;
  alloc = 0;
  selection_color(FL_SELECTION_COLOR);
  textfont(FL_HELVETICA);
  textsize((uchar)FL_NORMAL_SIZE);
  textcolor(FL_FOREGROUND_COLOR);
  down_box(FL_NO_BOX);

#if defined(WIN32) && (!defined(__GNUC__) || __GNUC__ >= 3)
  // Initialize MSAA stuff
  msaa_menu_items_   = 0;
  msaa_menu_popups_  = 0;
#endif // WIN32 && (!__GNUC__ || __GNUC__ >= 3)
}

int Fl_Menu_::size() const {
  if (!menu_) return 0;
  return menu_->size();
}

void Fl_Menu_::menu(const Fl_Menu_Item* m) {
  clear();
  value_ = menu_ = (Fl_Menu_Item*)m;
}

// this version is ok with new Fl_Menu_add code with fl_menu_array_owner:

void Fl_Menu_::copy(const Fl_Menu_Item* m, void* ud) {
  int n = m->size();
  Fl_Menu_Item* newMenu = new Fl_Menu_Item[n];
  memcpy(newMenu, m, n*sizeof(Fl_Menu_Item));
  menu(newMenu);
  alloc = 1; // make destructor free array, but not strings
  // for convienence, provide way to change all the user data pointers:
  if (ud) for (; n--;) {
    if (newMenu->callback_) newMenu->user_data_ = ud;
    newMenu++;
  }
}

Fl_Menu_::~Fl_Menu_() {
  clear();
}

// Fl_Menu::add() uses this to indicate the owner of the dynamically-
// expanding array.  We must not free this array:
Fl_Menu_* fl_menu_array_owner = 0;

void Fl_Menu_::clear() {
#if defined(WIN32) && (!defined(__GNUC__) || __GNUC__ >= 3)
  msaa_menu_clear();
#endif // WIN32 && (!__GNUC__ || __GNUC__ >= 3)
  if (alloc) {
    if (alloc>1) for (int i = size(); i--;)
      if (menu_[i].text) free((void*)menu_[i].text);
    if (this == fl_menu_array_owner)
      fl_menu_array_owner = 0;
    else
      delete[] menu_;
    menu_ = 0;
    value_ = 0;
    alloc = 0;
  }
}


#if defined(WIN32) && (!defined(__GNUC__) || __GNUC__ >= 3)
#include <FL/Fl_Msaa_Proxy.H>
#include <FL/fl_draw.H>
#include <FL/x.H>
///////////////////////////////////////////////////////////////////////
//
//  The following code implements Microsoft Active Accessibility

// Specialized MSAA proxy for Fl_Menu_ object
class Fl_Menu_Proxy : public Fl_Msaa_Proxy
{
private:
  int  mPos;		// Offset of menu item in menu bar
  bool mIsPopup;	// Is this a proxy for menu item container

public:
  Fl_Menu_Proxy(Fl_Widget* o, int pos, bool isPopup) :
      Fl_Msaa_Proxy(o), mPos(pos), mIsPopup(isPopup)  {}

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


static int
count_top_level_items(const Fl_Menu_Item* item)
{
    int count = 0;
    while (item->label())
    {
        item = item->next();
        count++;
    }
    return count;
}


static int
get_parent_index(const Fl_Menu_* menubar, int index)
{
    index--;
    const Fl_Menu_Item* item = menubar->menu() + index;
    for (int nest = 1;  index >= 0;  index--, item--)
    {
        if (!item->label())  nest++;  
        else if (item->submenu())
        {
            nest--;
            if (nest < 0)  break;
        }
    }
    return index + 1;
}


static const Fl_Menu_Item*
get_child_item(const Fl_Menu_* menubar, int index, int child_id)
{
    if ((index == 0) && (child_id == 0))  return NULL;
    if (child_id == 0)  return menubar->menu() + index - 1;
    const Fl_Menu_Item* item = menubar->menu() + index;
    for (;  child_id > 1;  child_id--, item = item->next());
    return item;
}


HRESULT STDMETHODCALLTYPE
Fl_Menu_Proxy::get_accParent(
    /* [retval][out] */ IDispatch** ppdispParent)
{
  Fl_Menu_* menubar = (Fl_Menu_*)mpObj;
  if (!menubar)  return CO_E_OBJNOTCONNECTED;
  if (!ppdispParent)  return E_INVALIDARG;
  *ppdispParent = NULL;

  Fl_Msaa_Proxy* parent;
  if (mPos == 0)
  {
    // Fl_Menu_ has a Fl_Msaa_Proxy parent
    Fl_Widget* p = menubar->parent();
    if (p)  parent = p->msaa_proxy();
  }
  else if (mIsPopup)
  {
    // Parent of pop-up menu is corresponding item
    parent = menubar->msaa_menu_proxy(mPos, false);
  }
  else
  {
    // Parent of menu item is pop-up menu
    int parent_idx = get_parent_index(menubar, mPos);
    parent = menubar->msaa_menu_proxy(parent_idx, true);
  }

  if (!parent)  return S_FALSE;
  parent->QueryInterface(IID_IDispatch, (void**)ppdispParent);
  return S_OK;
}


HRESULT STDMETHODCALLTYPE
Fl_Menu_Proxy::get_accChildCount(
    /* [retval][out] */ long* pcountChildren)
{
  Fl_Menu_* menubar = (Fl_Menu_*)mpObj;
  if (!menubar)  return CO_E_OBJNOTCONNECTED;
  if (!pcountChildren)  return E_INVALIDARG;
  if (mIsPopup)
    *pcountChildren =
      count_top_level_items(menubar->menu() + mPos);
  else
    *pcountChildren = 1;
  return S_OK;
}


HRESULT STDMETHODCALLTYPE
Fl_Menu_Proxy::get_accChild(
    /* [in] */ VARIANT varChild,
    /* [retval][out] */ IDispatch** ppdispChild)
{
  Fl_Menu_* menubar = (Fl_Menu_*)mpObj;
  if (!menubar)  return CO_E_OBJNOTCONNECTED;
  if (varChild.vt != VT_I4)  return E_INVALIDARG;
  if (!ppdispChild)  return E_INVALIDARG;
  *ppdispChild = NULL;

  // Proxy for submenu item contains only a popup
  if (!mIsPopup)
  {
    if (varChild.lVal != 1)  return E_INVALIDARG;
    Fl_Menu_Proxy* proxy = menubar->msaa_menu_proxy(mPos, true);
    if (!proxy)  return S_FALSE;
    proxy->QueryInterface(IID_IDispatch, (void**)ppdispChild);
    return S_OK;
  }

  // Find position of required child item
  const Fl_Menu_Item* menu = menubar->menu();
  int pos = get_child_item(menubar, mPos, varChild.lVal) - menu;

  // Get proxy item only for sub-menu
  if (!menu[pos].submenu())  return S_FALSE;

  // Make sure proxy is valid
  Fl_Menu_Proxy* proxy = menubar->msaa_menu_proxy(pos + 1, false);
  if (!proxy)  return S_FALSE;

  // Add reference to proxy
  proxy->QueryInterface(IID_IDispatch, (void**)ppdispChild);
  return S_OK;
}


HRESULT STDMETHODCALLTYPE
Fl_Menu_Proxy::get_accName(
    /* [optional][in] */ VARIANT varChild,
    /* [retval][out] */ BSTR* pszName)
{
  Fl_Menu_* menubar = (Fl_Menu_*)mpObj;
  if (!menubar)  return CO_E_OBJNOTCONNECTED;
  if (!pszName)  return E_INVALIDARG;
  *pszName = NULL;
  if (varChild.vt != VT_I4)
  {
    varChild.vt   = VT_I4;
    varChild.lVal = 0;
  }

  if ((mPos == 0) && (varChild.lVal == 0))
  {
    fl_str_to_bstr(menubar->label(), -1, pszName);
    return S_OK;
  }
  const Fl_Menu_Item* item = get_child_item(menubar, mPos, varChild.lVal);
  if (!item)  return E_INVALIDARG;

  // Copy human-readable label and shortcut
  size_t len;
  char* name = fl_label_to_text(item->label(), &len);
  if (!item->shortcut())  fl_str_to_bstr(name, len, pszName);
  else
  {
    const char* shortcut = fl_shortcut_label(item->shortcut());
    size_t full_len = len + strlen(shortcut) + 1;
    char* s = (char*)malloc(full_len + 1);
    sprintf(s, "%s\t%s", name, shortcut);
    fl_str_to_bstr(s, full_len, pszName);
    free(s);
  }
  fl_free_str(name);
  return S_OK;
}


HRESULT STDMETHODCALLTYPE
Fl_Menu_Proxy::get_accValue(
    /* [optional][in] */ VARIANT varChild,
    /* [retval][out] */ BSTR* pszValue)
{
  Fl_Menu_* menubar = (Fl_Menu_*)mpObj;
  if (!menubar)  return CO_E_OBJNOTCONNECTED;
  if (!pszValue)  return E_INVALIDARG;
  *pszValue = NULL;
  return S_FALSE;
}


HRESULT STDMETHODCALLTYPE
Fl_Menu_Proxy::get_accDescription(
    /* [optional][in] */ VARIANT varChild,
    /* [retval][out] */ BSTR* pszDescription)
{
  Fl_Menu_* menubar = (Fl_Menu_*)mpObj;
  if (!menubar)  return CO_E_OBJNOTCONNECTED;
  if (!pszDescription)  return E_INVALIDARG;
  *pszDescription = NULL;
  return S_FALSE;
}


HRESULT STDMETHODCALLTYPE
Fl_Menu_Proxy::get_accRole(
    /* [optional][in] */ VARIANT varChild,
    /* [retval][out] */ VARIANT* pvarRole)
{
  Fl_Menu_* menubar = (Fl_Menu_*)mpObj;
  if (!menubar)  return CO_E_OBJNOTCONNECTED;
  if (!pvarRole)  return E_INVALIDARG;
  if (varChild.vt != VT_I4)
  {
    varChild.vt   = VT_I4;
    varChild.lVal = 0;
  }
  pvarRole->vt   = VT_I4;
  if (mPos == 0)
    pvarRole->lVal = ROLE_SYSTEM_MENUBAR;
  else if (mIsPopup && (varChild.lVal == 0))
    pvarRole->lVal = ROLE_SYSTEM_MENUPOPUP;
  else 
    pvarRole->lVal = ROLE_SYSTEM_MENUITEM;
  return S_OK;
}


HRESULT STDMETHODCALLTYPE
Fl_Menu_Proxy::get_accState(
    /* [optional][in] */ VARIANT varChild,
    /* [retval][out] */ VARIANT* pvarState)
{
  Fl_Menu_* menubar = (Fl_Menu_*)mpObj;
  if (!menubar)  return CO_E_OBJNOTCONNECTED;
  if (!pvarState)  return E_INVALIDARG;
  if (varChild.vt != VT_I4)
  {
    varChild.vt   = VT_I4;
    varChild.lVal = 0;
  }
  pvarState->vt = VT_I4;
  pvarState->lVal = 0;
  if ((mPos == 0) || !mIsPopup || (varChild.lVal != 0))
    pvarState->lVal = 0;
  else
    pvarState->lVal = STATE_SYSTEM_INVISIBLE;
  return S_OK;
}


HRESULT STDMETHODCALLTYPE
Fl_Menu_Proxy::get_accHelp(
    /* [optional][in] */ VARIANT varChild,
    /* [retval][out] */ BSTR* pszHelp)
{
  Fl_Menu_* menubar = (Fl_Menu_*)mpObj;
  if (!menubar)  return CO_E_OBJNOTCONNECTED;
  if (!pszHelp)  return E_INVALIDARG;
  *pszHelp = NULL;
  return S_FALSE;
}


HRESULT STDMETHODCALLTYPE
Fl_Menu_Proxy::get_accHelpTopic(
    /* [out] */ BSTR* pszHelpFile,
    /* [optional][in] */ VARIANT varChild,
    /* [retval][out] */ long* pidTopic)
{
  Fl_Menu_* menubar = (Fl_Menu_*)mpObj;
  if (!menubar)  return CO_E_OBJNOTCONNECTED;
  if (!pszHelpFile)  return E_INVALIDARG;
  if (!pidTopic)  return E_INVALIDARG;
  *pszHelpFile = NULL;
  *pidTopic = 0;
  return S_FALSE;
}


HRESULT STDMETHODCALLTYPE
Fl_Menu_Proxy::get_accKeyboardShortcut(
    /* [optional][in] */ VARIANT varChild,
    /* [retval][out] */ BSTR* pszKeyboardShortcut)
{
  Fl_Menu_* menubar = (Fl_Menu_*)mpObj;
  if (!menubar)  return CO_E_OBJNOTCONNECTED;
  if (!pszKeyboardShortcut)  return E_INVALIDARG;
  *pszKeyboardShortcut = NULL;
  if (varChild.vt != VT_I4)
  {
    varChild.vt   = VT_I4;
    varChild.lVal = 0;
  }
  if (((mPos == 0) || mIsPopup) && (varChild.lVal == 0))
  {
    // Menu bar and popup proxy has no shortcut
    return S_FALSE;
  }
  const Fl_Menu_Item* item = get_child_item(menubar, mPos, varChild.lVal);
  if (!item)  return E_INVALIDARG;
  size_t len;
  char* shortcut = fl_label_to_shortcut(item->label(), &len);
  fl_str_to_bstr(shortcut, len, pszKeyboardShortcut);
  fl_free_str(shortcut);
  return S_OK;
}


HRESULT STDMETHODCALLTYPE
Fl_Menu_Proxy::get_accFocus(
    /* [retval][out] */ VARIANT *pvarChild)
{
  Fl_Menu_* menubar = (Fl_Menu_*)mpObj;
  if (!menubar)  return CO_E_OBJNOTCONNECTED;
  if (!pvarChild)  return E_INVALIDARG;
  pvarChild->vt = VT_EMPTY;
  return S_FALSE;
}


HRESULT STDMETHODCALLTYPE
Fl_Menu_Proxy::get_accSelection(
    /* [retval][out] */ VARIANT* pvarChildren)
{
  Fl_Menu_* menubar = (Fl_Menu_*)mpObj;
  if (!menubar)  return CO_E_OBJNOTCONNECTED;
  if (!pvarChildren)  return E_INVALIDARG;
  pvarChildren->vt = VT_EMPTY;
  return S_FALSE;
}


HRESULT STDMETHODCALLTYPE
Fl_Menu_Proxy::get_accDefaultAction(
    /* [optional][in] */ VARIANT varChild,
    /* [retval][out] */ BSTR* pszDefaultAction)
{
  Fl_Menu_* menubar = (Fl_Menu_*)mpObj;
  if (!menubar)  return CO_E_OBJNOTCONNECTED;
  if (!pszDefaultAction)  return E_INVALIDARG;
  if (!mIsPopup ||
      (mIsPopup && (varChild.lVal == 0)))
  {
    *pszDefaultAction = NULL;
    return S_FALSE;
  }
  else
  {
    fl_str_to_bstr("Execute", -1, pszDefaultAction);
    return S_OK;
  }
}


HRESULT STDMETHODCALLTYPE
Fl_Menu_Proxy::accSelect(
    /* [in] */ long flagsSelect,
    /* [optional][in] */ VARIANT varChild)
{
  Fl_Menu_* menubar = (Fl_Menu_*)mpObj;
  if (!menubar)  return CO_E_OBJNOTCONNECTED;
  return S_FALSE;
}


HRESULT STDMETHODCALLTYPE
Fl_Menu_Proxy::accLocation(
    /* [out] */ long* pxLeft,
    /* [out] */ long* pyTop,
    /* [out] */ long* pcxWidth,
    /* [out] */ long* pcyHeight,
    /* [optional][in] */ VARIANT varChild)
{
  Fl_Menu_* menubar = (Fl_Menu_*)mpObj;
  if (!menubar)  return CO_E_OBJNOTCONNECTED;
  if (!pxLeft || !pyTop || !pcxWidth || !pcyHeight)  return E_INVALIDARG;
  if (varChild.vt != VT_I4)
  {
    varChild.vt   = VT_I4;
    varChild.lVal = 0;
  }

  // Find parent window's position on screen
  POINT offset = {0, 0};
  ClientToScreen(fl_xid(menubar->window()), &offset);

  // Return menubar location
  int pos = mPos;
  int child_id = varChild.lVal;
  if ((pos == 0) && (child_id == 0))
  {
    *pxLeft    = menubar->x() + offset.x;
    *pyTop     = menubar->y() + offset.y;
    *pcxWidth  = menubar->w();
    *pcyHeight = menubar->h();
    return S_OK;
  }

  // Find pointer for top level menu item
  const Fl_Menu_Item* target = NULL;
  if ((pos == 0) && !get_child_item(menubar, 0, child_id)->submenu())
    target = get_child_item(menubar, 0, child_id);
  else if ((child_id == 0) && !mIsPopup &&
      (get_parent_index(menubar, pos) == 0))
    target = menubar->menu() + pos - 1;

  // Return top level item location -- see code for Fl_Menu_::draw()
  if (target)
  {
    int X = 6;
    int skip = child_id - 1;
    const Fl_Menu_Item* item = menubar->menu();
    for (;  item->label() && (item != target);  item = item->next())
      X += item->measure(0, menubar) + 16;
    *pxLeft    = menubar->x() + offset.x + X;
    *pyTop     = menubar->y() + offset.y;
    *pcxWidth  = item->measure(0, menubar) + 6;
    *pcyHeight = menubar->h();
    return S_OK;
  }

  // Fill in dummy values
  *pxLeft    = 0;
  *pyTop     = 0;
  *pcxWidth  = 0;
  *pcyHeight = 0;

  // Only pop-up menus have a location
  if (mIsPopup && (child_id == 0))  return S_OK;
  return S_FALSE;
}


HRESULT STDMETHODCALLTYPE
Fl_Menu_Proxy::accNavigate(
    /* [in] */ long navDir,
    /* [optional][in] */ VARIANT varStart,
    /* [retval][out] */ VARIANT* pvarEndUpAt)
{
  Fl_Menu_* menubar = (Fl_Menu_*)mpObj;
  if (!menubar)  return CO_E_OBJNOTCONNECTED;
  if (!pvarEndUpAt)  return E_INVALIDARG;
  pvarEndUpAt->vt = VT_EMPTY;
  if (varStart.vt != VT_I4)
  {
    varStart.vt   = VT_I4;
    varStart.lVal = 0;
  }
  return S_FALSE;
}


HRESULT STDMETHODCALLTYPE
Fl_Menu_Proxy::accHitTest(
    /* [in] */ long xLeft,
    /* [in] */ long yTop,
    /* [retval][out] */ VARIANT* pvarChild)
{
  Fl_Menu_* menubar = (Fl_Menu_*)mpObj;
  if (!menubar)  return CO_E_OBJNOTCONNECTED;
  if (!pvarChild)  return E_INVALIDARG;
  pvarChild->vt = VT_EMPTY;

  // Obtain widget location in screen co-ordinates
  long x, y, w, h;
  VARIANT self;
  self.vt   = VT_I4;
  self.lVal = 0;
  if (S_OK != accLocation(&x, &y, &w, &h, self))  return S_FALSE;

  // Now perform check
  if ((xLeft >= x) && (xLeft < x + w) && (yTop >= y) && (yTop < y + h))
  {
    pvarChild->vt   = VT_I4;
    pvarChild->lVal = 0;
    return S_OK;
  }
  else
    return S_FALSE;
}


HRESULT STDMETHODCALLTYPE
Fl_Menu_Proxy::accDoDefaultAction(
    /* [optional][in] */ VARIANT varChild)
{
  Fl_Menu_* menubar = (Fl_Menu_*)mpObj;
  if (!menubar)  return CO_E_OBJNOTCONNECTED;
  if (varChild.vt != VT_I4)
  {
    varChild.vt   = VT_I4;
    varChild.lVal = 0;
  }

  if ((mPos == 0) && (varChild.lVal == 0))
  {
    menubar->do_callback();
    return S_OK;
  }
  const Fl_Menu_Item* item = get_child_item(menubar, mPos, varChild.lVal);
  if (!item)  return E_INVALIDARG;
  menubar->picked(item);
  return S_OK;
}


HRESULT STDMETHODCALLTYPE
Fl_Menu_Proxy::put_accName(
    /* [optional][in] */ VARIANT varChild,
    /* [in] */ BSTR szName)
{
  Fl_Menu_* menubar = (Fl_Menu_*)mpObj;
  if (!menubar)  return CO_E_OBJNOTCONNECTED;
  if (varChild.vt != VT_I4)
  {
    varChild.vt   = VT_I4;
    varChild.lVal = 0;
  }
  return S_FALSE;
}


HRESULT STDMETHODCALLTYPE
Fl_Menu_Proxy::put_accValue(
    /* [optional][in] */ VARIANT varChild,
    /* [in] */ BSTR szValue)
{
  Fl_Menu_* menubar = (Fl_Menu_*)mpObj;
  if (!menubar)  return CO_E_OBJNOTCONNECTED;
  if (varChild.vt != VT_I4)
  {
    varChild.vt   = VT_I4;
    varChild.lVal = 0;
  }
  return S_FALSE;
}


/*-------------------------- Fl_Menu_ MSAA support ------------------------*/

void
Fl_Menu_::msaa_menu_clear()
{
  // Destroy cache of MSAA proxy
  for (int i = size() - 1;  (i >= 0) && msaa_menu_items_;  i--)
  {
    Fl_Menu_Proxy* proxy;
    if ((proxy = msaa_menu_items_[i]))
    {
      proxy->Detach();
      proxy->Release();
    }
    if ((proxy = msaa_menu_popups_[i]))
    {
      proxy->Detach();
      proxy->Release();
    }
  }
  if (msaa_menu_items_)
  {
    free(msaa_menu_items_);
    free(msaa_menu_popups_);
    msaa_menu_items_ = 0;
    msaa_menu_popups_ = 0;
  }
}


Fl_Menu_Proxy*
Fl_Menu_::msaa_menu_proxy(int pos, bool isPopup)
{
  // Create new storage for proxies if necessary
  if (!msaa_menu_items_)
  {
    msaa_menu_items_ =
      (Fl_Menu_Proxy**)calloc(size(), sizeof(Fl_Menu_Proxy*));
    msaa_menu_popups_ =
      (Fl_Menu_Proxy**)calloc(size(), sizeof(Fl_Menu_Proxy*));
  }

  // Determine required proxy type
  Fl_Menu_Proxy** cache = isPopup ? msaa_menu_popups_ : msaa_menu_items_;
  if (cache[pos])  return cache[pos];

  // Create missing proxy
  Fl_Menu_Proxy* proxy = new Fl_Menu_Proxy(this, pos, isPopup);
  if (proxy)
  {
    proxy->AddRef();
    cache[pos] = proxy;
  }
  return proxy;
}


Fl_Msaa_Proxy*
Fl_Menu_::msaa_proxy()
{
  // Create specialized proxy for menu bar
  if (!msaa_proxy_)  msaa_proxy_ = msaa_menu_proxy(0, true);
  return msaa_proxy_;
}
#endif // WIN32 && (!__GNUC__ || __GNUC__ >= 3)


//
// End of "$Id: Fl_Menu_.cxx 5190 2006-06-09 16:16:34Z mike $".
//
