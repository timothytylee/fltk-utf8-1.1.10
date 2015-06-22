//
// "$Id: Fl_Widget.cxx 6978 2009-12-23 12:54:41Z AlbrechtS $"
//
// Base widget class for the Fast Light Tool Kit (FLTK).
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

#include <FL/Fl.H>
#include <FL/Fl_Widget.H>
#include <FL/Fl_Group.H>
#include <FL/Fl_Tooltip.H>
#include <FL/fl_draw.H>
#include <stdlib.h>
#include "flstring.h"


////////////////////////////////////////////////////////////////
// for compatability with Forms, all widgets without callbacks are
// inserted into a "queue" when they are activated, and the forms
// compatability interaction functions (fl_do_events, etc) will
// read one widget at a time from this queue and return it:

const int QUEUE_SIZE = 20;

static Fl_Widget *obj_queue[QUEUE_SIZE];
static int obj_head, obj_tail;

void Fl_Widget::default_callback(Fl_Widget *o, void * /*v*/) {
#if 0
  // This is necessary for strict forms compatibility but is confusing.
  // Use the parent's callback if this widget does not have one.
  for (Fl_Widget *p = o->parent(); p; p = p->parent())
    if (p->callback() != default_callback) {
      p->do_callback(o,v);
      return;
    }
#endif
  obj_queue[obj_head++] = o;
  if (obj_head >= QUEUE_SIZE) obj_head = 0;
  if (obj_head == obj_tail) {
    obj_tail++;
    if (obj_tail >= QUEUE_SIZE) obj_tail = 0;
  }
}
/*
    All Fl_Widgets that don't have a callback defined use a
    default callback that puts a pointer to the widget in this queue,
    and this method reads the oldest widget out of this queue.
*/
Fl_Widget *Fl::readqueue() {
  if (obj_tail==obj_head) return 0;
  Fl_Widget *o = obj_queue[obj_tail++];
  if (obj_tail >= QUEUE_SIZE) obj_tail = 0;
  return o;
}
/*
    This static internal function removes all pending callbacks for a
    specific widget from the default callback queue (Fl::readqueue()).
    It is only called from Fl_Widget's destructor if the widget
    doesn't have an own callback.
    Note: There's no need to have this in the Fl:: namespace.
*/
static void cleanup_readqueue(Fl_Widget *w) {

  if (obj_tail==obj_head) return;
  
  // Read the entire queue and copy over all valid entries.
  // The new head will be determined after the last copied entry.

  int old_head = obj_head;	// save newest entry
  int entry = obj_tail;		// oldest entry
  obj_head = obj_tail;		// new queue start
  for (;;) {
    Fl_Widget *o = obj_queue[entry++];
    if (entry >= QUEUE_SIZE) entry = 0;
    if (o != w) { // valid entry
      obj_queue[obj_head++] = o;
      if (obj_head >= QUEUE_SIZE) obj_head = 0;
    } // valid entry
    if (entry == old_head) break;
  }
  return;
}

////////////////////////////////////////////////////////////////

int Fl_Widget::handle(int) {
  return 0;
}

int FL_NORMAL_SIZE = 14;

Fl_Widget::Fl_Widget(int X, int Y, int W, int H, const char* L) {

  x_ = X; y_ = Y; w_ = W; h_ = H;

  label_.value	 = L;
  label_.image   = 0;
  label_.deimage = 0;
  label_.type	 = FL_NORMAL_LABEL;
  label_.font	 = FL_HELVETICA;
  label_.size	 = (uchar)FL_NORMAL_SIZE;
  label_.color	 = FL_FOREGROUND_COLOR;
  tooltip_       = 0;
  callback_	 = default_callback;
  user_data_ 	 = 0;
  type_		 = 0;
  flags_	 = VISIBLE_FOCUS;
  damage_	 = 0;
  box_		 = FL_NO_BOX;
  color_	 = FL_GRAY;
  color2_	 = FL_GRAY;
  align_	 = FL_ALIGN_CENTER;
  when_		 = FL_WHEN_RELEASE;

#if defined(WIN32) && (!defined(__GNUC__) || __GNUC__ >= 3)
  // Initialize MSAA stuff
  msaa_proxy_	 = 0;
#endif // WIN32 && (!__GNUC__ || __GNUC__ >= 3)

  parent_ = 0;
  if (Fl_Group::current()) Fl_Group::current()->add(this);
}

void Fl_Widget::resize(int X, int Y, int W, int H) {
  x_ = X; y_ = Y; w_ = W; h_ = H;
}

// this is useful for parent widgets to call to resize children:
int Fl_Widget::damage_resize(int X, int Y, int W, int H) {
  if (x() == X && y() == Y && w() == W && h() == H) return 0;
  resize(X, Y, W, H);
  redraw();
  return 1;
}

int Fl_Widget::take_focus() {
  if (!takesevents()) return 0;
  if (!visible_focus()) return 0;
  if (!handle(FL_FOCUS)) return 0; // see if it wants it
  if (contains(Fl::focus())) return 1; // it called Fl::focus for us
  Fl::focus(this);
  return 1;
}

extern void fl_throw_focus(Fl_Widget*); // in Fl_x.cxx

// Destruction does not remove from any parent group!  And groups when
// destroyed destroy all their children.  This is convienent and fast.
// However, it is only legal to destroy a "root" such as an Fl_Window,
// and automatic destructors may be called.
Fl_Widget::~Fl_Widget() {
  Fl::clear_widget_pointer(this);
  if (flags() & COPIED_LABEL) free((void *)(label_.value));
  parent_ = 0; // Don't throw focus to a parent widget.
  fl_throw_focus(this);
<<<<<<< .mine

#if defined(WIN32) && (!defined(__GNUC__) || __GNUC__ >= 3)
  msaa_cleanup();
#endif // WIN32 && (!__GNUC__ || __GNUC__ >= 3)
=======
  // remove stale entries from default callback queue (Fl::readqueue())
  if (callback_ == default_callback) cleanup_readqueue(this);
>>>>>>> .r7555
}

// draw a focus box for the widget...
void
Fl_Widget::draw_focus(Fl_Boxtype B, int X, int Y, int W, int H) const {
  if (!Fl::visible_focus()) return;
  switch (B) {
    case FL_DOWN_BOX:
    case FL_DOWN_FRAME:
    case FL_THIN_DOWN_BOX:
    case FL_THIN_DOWN_FRAME:
      X ++;
      Y ++;
    default:
      break;
  }

  fl_color(fl_contrast(FL_BLACK, color()));

#if defined(WIN32) || defined(__APPLE_QD__)
  // Windows 95/98/ME do not implement the dotted line style, so draw
  // every other pixel around the focus area...
  //
  // Also, QuickDraw (MacOS) does not support line styles specifically,
  // and the hack we use in fl_line_style() will not draw horizontal lines
  // on odd-numbered rows...
  int i, xx, yy;

  X += Fl::box_dx(B);
  Y += Fl::box_dy(B);
  W -= Fl::box_dw(B) + 2;
  H -= Fl::box_dh(B) + 2;

  for (xx = 0, i = 1; xx < W; xx ++, i ++) if (i & 1) fl_point(X + xx, Y);
  for (yy = 0; yy < H; yy ++, i ++) if (i & 1) fl_point(X + W, Y + yy);
  for (xx = W; xx > 0; xx --, i ++) if (i & 1) fl_point(X + xx, Y + H);
  for (yy = H; yy > 0; yy --, i ++) if (i & 1) fl_point(X, Y + yy);
#else
  fl_line_style(FL_DOT);
  fl_rect(X + Fl::box_dx(B), Y + Fl::box_dy(B),
          W - Fl::box_dw(B) - 1, H - Fl::box_dh(B) - 1);
  fl_line_style(FL_SOLID);
#endif // WIN32
}


void Fl_Widget::activate() {
  if (!active()) {
    clear_flag(INACTIVE);
    if (active_r()) {
      redraw();
      redraw_label();
      handle(FL_ACTIVATE);
      if (inside(Fl::focus())) Fl::focus()->take_focus();
    }
  }
}

void Fl_Widget::deactivate() {
  if (active_r()) {
    set_flag(INACTIVE);
    redraw();
    redraw_label();
    handle(FL_DEACTIVATE);
    fl_throw_focus(this);
  } else {
    set_flag(INACTIVE);
  }
}

int Fl_Widget::active_r() const {
  for (const Fl_Widget* o = this; o; o = o->parent())
    if (!o->active()) return 0;
  return 1;
}

void Fl_Widget::show() {
  if (!visible()) {
    clear_flag(INVISIBLE);
    if (visible_r()) {
      redraw();
      redraw_label();
      handle(FL_SHOW);
      if (inside(Fl::focus())) Fl::focus()->take_focus();
    }
  }
}

void Fl_Widget::hide() {
  if (visible_r()) {
    set_flag(INVISIBLE);
    for (Fl_Widget *p = parent(); p; p = p->parent())
      if (p->box() || !p->parent()) {p->redraw(); break;}
    handle(FL_HIDE);
    fl_throw_focus(this);
  } else {
    set_flag(INVISIBLE);
  }
}

int Fl_Widget::visible_r() const {
  for (const Fl_Widget* o = this; o; o = o->parent())
    if (!o->visible()) return 0;
  return 1;
}

// return true if widget is inside (or equal to) this:
// Returns false for null widgets.
int Fl_Widget::contains(const Fl_Widget *o) const {
  for (; o; o = o->parent_) if (o == this) return 1;
  return 0;
}


void
Fl_Widget::label(const char *a) {
  if (flags() & COPIED_LABEL) {
    // reassigning a copied label remains the same copied label
    if (label_.value == a)
      return;
    free((void *)(label_.value));
    clear_flag(COPIED_LABEL);
  }
  label_.value=a;
  redraw_label();
}


void
Fl_Widget::copy_label(const char *a) {
  if (flags() & COPIED_LABEL) free((void *)(label_.value));
  if (a) {
    set_flag(COPIED_LABEL);
    label_.value=strdup(a);
  } else {
    clear_flag(COPIED_LABEL);
    label_.value=(char *)0;
  }
  redraw_label();
}


#if defined(WIN32) && (!defined(__GNUC__) || __GNUC__ >= 3)
#include <FL/Fl_Msaa_Proxy.H>
#include <FL/x.H>
///////////////////////////////////////////////////////////////////////
//
//  The following code implements Microsoft Active Accessibility

void
Fl_Widget::msaa_cleanup()
{
  // Detach MSAA proxy object and release reference
  if (msaa_proxy_)
  {
    msaa_proxy_->Detach();
    msaa_proxy_->Release();
  }
}


long
Fl_Widget::msaa_role()
{
  return ROLE_SYSTEM_STATICTEXT;
}


long
Fl_Widget::msaa_state()
{
  return visible() ? 0 : STATE_SYSTEM_INVISIBLE;
}


Fl_Msaa_Proxy*
Fl_Widget::msaa_proxy()
{
  // Allow some widgets to be represented as simple elements
  if (!msaa_use_proxy())  return 0;

  if (!msaa_proxy_)
  {
    // Link an IAccessible proxy to this widget
    msaa_proxy_ = new Fl_Msaa_Proxy(this);
    if (msaa_proxy_)  msaa_proxy_->AddRef();
  }
  return msaa_proxy_;
}


HRESULT 
Fl_Widget::msaa_get_accParent(IDispatch** ppdispParent)
{
  Fl_Group* p = parent();
  if (p)
  {
    // Traversing inside FLTK hierarchy
    Fl_Msaa_Proxy* proxy = p->msaa_proxy();
    if (!proxy)  return S_FALSE;
    proxy->QueryInterface(IID_IDispatch, (void**)ppdispParent);
    return S_OK;
  }
  else
  {
    // Moving outside FLTK hierarchy
    HWND hwnd = fl_xid((Fl_Window*)this);
    return flAccessibleObjectFromWindow(hwnd, OBJID_WINDOW,
	IID_IDispatch, (void**)ppdispParent);
  }
}


HRESULT 
Fl_Widget::msaa_get_accChildCount(long* pcountChildren)
{
  // Widget has not children by default
  return S_OK;
}


HRESULT 
Fl_Widget::msaa_get_accChild(VARIANT varChild, IDispatch** ppdispChild)
{
  // Widget has no children by default
  if (varChild.lVal != 0)  return E_INVALIDARG;
  Fl_Msaa_Proxy* proxy = msaa_proxy();
  if (!proxy)  return S_FALSE;
  proxy->QueryInterface(IID_IDispatch, (void**)ppdispChild);
  return S_OK;
}


HRESULT 
Fl_Widget::msaa_get_accName(VARIANT varChild, BSTR* pszName)
{
  // Use widget label as default name
  if (varChild.lVal != 0)  return E_INVALIDARG;
  fl_str_to_bstr(label(), -1, pszName);
  return S_OK;
}


HRESULT
Fl_Widget::msaa_get_accValue(VARIANT varChild, BSTR* pszValue)
{
  if (varChild.lVal != 0)  return E_INVALIDARG;
  return DISP_E_MEMBERNOTFOUND;
}


HRESULT 
Fl_Widget::msaa_get_accDescription(VARIANT varChild, BSTR* pszDescription)
{
  if (varChild.lVal != 0)  return E_INVALIDARG;
  return S_FALSE;
}


HRESULT 
Fl_Widget::msaa_get_accRole(VARIANT varChild, VARIANT* pvarRole)
{
  if (varChild.lVal != 0)  return E_INVALIDARG;
  pvarRole->vt   = VT_I4;
  pvarRole->lVal = msaa_role();
  return S_OK;
}


HRESULT 
Fl_Widget::msaa_get_accState(VARIANT varChild, VARIANT* pvarState)
{
  if (varChild.lVal != 0)  return E_INVALIDARG;
  pvarState->vt   = VT_I4;
  pvarState->lVal = msaa_state();
  return S_OK;
}


HRESULT 
Fl_Widget::msaa_get_accHelp(VARIANT varChild, BSTR* pszHelp)
{
  if (varChild.lVal != 0)  return E_INVALIDARG;
  return S_FALSE;
}


HRESULT 
Fl_Widget::msaa_get_accHelpTopic(BSTR* pszHelpFile, VARIANT varChild,
    long* pidTopic)
{
  if (varChild.lVal != 0)  return E_INVALIDARG;
  return S_FALSE;
}


HRESULT 
Fl_Widget::msaa_get_accKeyboardShortcut(VARIANT varChild,
    BSTR* pszKeyboardShortcut)
{
  if (varChild.lVal != 0)  return E_INVALIDARG;
  return S_FALSE;
}


HRESULT 
Fl_Widget::msaa_get_accFocus(VARIANT* pvarChild)
{
  if (Fl::focus() != this)  return S_FALSE;
  pvarChild->vt   = VT_I4;
  pvarChild->lVal = 0;
  return S_OK;
}


HRESULT 
Fl_Widget::msaa_get_accSelection(VARIANT* pvarChildren)
{
  return S_OK;
}


HRESULT 
Fl_Widget::msaa_get_accDefaultAction(VARIANT varChild, BSTR* pszDefaultAction)
{
  if (varChild.lVal != 0)  return E_INVALIDARG;
  return S_FALSE;
}


HRESULT 
Fl_Widget::msaa_accSelect(long flagsSelect, VARIANT varChild)
{
  if (varChild.lVal != 0)  return E_INVALIDARG;
  if (!(flagsSelect & SELFLAG_TAKEFOCUS))  return E_INVALIDARG;
  if (!handle(FL_FOCUS))  return S_FALSE;
  Fl::focus(this);
  return S_OK;
}


HRESULT 
Fl_Widget::msaa_accLocation(long* pxLeft, long* pyTop,
    long* pcxWidth, long* pcyHeight, VARIANT varChild)
{
  // Convert to screen co-ordinates
  POINT pt = {x(), y()};
  const Fl_Window* wnd = msaa_window();
  if (this != (Fl_Widget*)wnd)  ClientToScreen(fl_xid(wnd), &pt);
  *pxLeft    = pt.x;
  *pyTop     = pt.y;
  *pcxWidth  = w();
  *pcyHeight = h();
  return S_OK;
}


HRESULT 
Fl_Widget::msaa_accNavigate(long navDir, VARIANT varStart,
    VARIANT* pvarEndUpAt)
{
  if (varStart.lVal != 0)  return E_INVALIDARG;

  // Default IAccessible implementation has no children
  if ((navDir == NAVDIR_FIRSTCHILD) || (navDir == NAVDIR_LASTCHILD))
    return S_FALSE;

  // Forward request to parent widget
  if ((navDir == NAVDIR_PREVIOUS) || (navDir == NAVDIR_NEXT) ||
      (navDir == NAVDIR_UP)       || (navDir == NAVDIR_DOWN) ||
      (navDir == NAVDIR_LEFT)     || (navDir == NAVDIR_RIGHT))
  {
    // Get parent group
    Fl_Group* p = parent();
    if (!p)  return S_FALSE;

    // Forward request to parent widget
    varStart.vt   = VT_I4;
    varStart.lVal = p->msaa_childId(this);
    return p->msaa_accNavigate(navDir, varStart, pvarEndUpAt);
  }

  return E_INVALIDARG;
}


HRESULT 
Fl_Widget::msaa_accHitTest(long xLeft, long yTop, VARIANT* pvarChild)
{
  // Obtain screen co-ordinates of widget
  VARIANT child;
  child.vt   = VT_I4;
  child.lVal = 0;
  long x, y, w, h;
  msaa_accLocation(&x, &y, &w, &h, child);

  // Perform hit test now
  if ((xLeft < x) || (xLeft >= (x + w)) || (yTop < y) || (yTop >= (y + h)))
    return S_FALSE;
  pvarChild->vt   = VT_I4;
  pvarChild->lVal = 0;
  return S_OK;
}


HRESULT 
Fl_Widget::msaa_accDoDefaultAction(VARIANT varChild)
{
  if (varChild.lVal != 0)  return E_INVALIDARG;
  return S_FALSE;
}


HRESULT 
Fl_Widget::msaa_put_accName(VARIANT varChild, BSTR szName)
{
  if (varChild.lVal != 0)  return E_INVALIDARG;
  char* s = fl_bstr_to_str(szName);
  copy_label(s);
  fl_free_str(s);
  return S_OK;
}


HRESULT 
Fl_Widget::msaa_put_accValue(VARIANT varChild, BSTR szValue)
{
  if (varChild.lVal != 0)  return E_INVALIDARG;
  return DISP_E_MEMBERNOTFOUND;
}
#endif // WIN32 && (!__GNUC__ || __GNUC__ >= 3)


//
// End of "$Id: Fl_Widget.cxx 6978 2009-12-23 12:54:41Z AlbrechtS $".
//
