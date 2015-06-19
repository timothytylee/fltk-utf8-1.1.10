//
// "$Id: fl_font_mac.cxx 5651 2007-02-01 20:13:57Z matt $"
//
// MacOS font selection routines for the Fast Light Tool Kit (FLTK).
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

#include <config.h>

// if no font has been selected yet by the user, get one.
#define check_default_font() {if (!fl_fontsize) fl_font(0, 12);}

Fl_FontSize::Fl_FontSize(const char* name, int Size) {
  next = 0;
#  if HAVE_GL
  listbase = 0;
#  endif
#ifdef __APPLE_QD__
  knowMetrics = 0;
  switch (*name++) {
  case 'I': face = italic; break;
  case 'P': face = italic | bold; break;
  case 'B': face = bold; break;
  default: face = 0; break;
  }
  unsigned char fn[80]; 
  fn[0] = strlen(name); strcpy((char*)(fn+1), name);
  GetFNum(fn, &font);
  size = Size;
  FMInput fIn = { font, size, face, 0, 0, { 1, 1}, { 1, 1} };
  FMOutput *fOut = FMSwapFont(&fIn);
  ascent = fOut->ascent;	//: the following three lines give only temporary aproimations
  descent = fOut->descent;
  for (int i=0; i<256; i++) width[i] = fOut->widMax;
  minsize = maxsize = size;
#elif defined(__APPLE_QUARTZ__)
    // OpenGL needs those for its font handling
  q_name = strdup(name);
  size = Size;
  OSStatus err;
    // fill our structure with a few default values
  ascent = Size*3/4;
  descent = Size-ascent;
  q_width = Size*2/3;
  minsize = maxsize = Size;
    // now use ATS to get the actual Glyph size information
    // say that our passed-in name is encoded as UTF-8, since this works for plain ASCII names too...
  CFStringRef cfname = CFStringCreateWithCString(0L, name, kCFStringEncodingUTF8);
  ATSFontRef font = ATSFontFindFromName(cfname, kATSOptionFlagsDefault);
  if (font) {
    ATSFontMetrics m = { 0 };
    ATSFontGetHorizontalMetrics(font, kATSOptionFlagsDefault, &m);
    if (m.avgAdvanceWidth) q_width = int(m.avgAdvanceWidth*Size);
      // playing with the offsets a little to make standard sizes fit
    if (m.ascent) ascent  = int(m.ascent*Size-0.5f);
    if (m.descent) descent = -int(m.descent*Size-1.5f);
  }
  CFRelease(cfname);
    // now we allocate everything needed to render text in this font later
    // get us the default layout and style
  err = ATSUCreateTextLayout(&layout);
  UniChar mTxt[2] = { 65, 0 };
  err = ATSUSetTextPointerLocation(layout, mTxt, kATSUFromTextBeginning, 1, 1);
  err = ATSUCreateStyle(&style);
  err = ATSUSetRunStyle(layout, style, kATSUFromTextBeginning, kATSUToTextEnd);
    // now set the actual font, size and attributes. We also set the font matrix to 
    // render our font up-side-down, so when rendered through our inverted CGContext,
    // text will appear normal again.
  Fixed fsize = IntToFixed(Size);
  ATSUFontID fontID;
  ATSUFindFontFromName(name, strlen(name), kFontFullName, kFontMacintoshPlatform, kFontRomanScript, kFontEnglishLanguage, &fontID);

  // draw the font upside-down... Compensate for fltk/OSX origin differences
  static CGAffineTransform font_mx = { 1, 0, 0, -1, 0, 0 };
  ATSUAttributeTag sTag[] = { kATSUFontTag, kATSUSizeTag, kATSUFontMatrixTag };
  ByteCount sBytes[] = { sizeof(ATSUFontID), sizeof(Fixed), sizeof(CGAffineTransform) };
  ATSUAttributeValuePtr sAttr[] = { &fontID, &fsize, &font_mx };
  err = ATSUSetAttributes(style, 3, sTag, sBytes, sAttr);
    // next, make sure that Quartz will only render at integer coordinates
  ATSLineLayoutOptions llo = kATSLineUseDeviceMetrics|kATSLineDisableAllLayoutOperations;
  ATSUAttributeTag aTag[] = { kATSULineLayoutOptionsTag };
  ByteCount aBytes[] = { sizeof(ATSLineLayoutOptions) };
  ATSUAttributeValuePtr aAttr[] = { &llo };
  err = ATSUSetLineControls (layout, kATSUFromTextBeginning, 1, aTag, aBytes, aAttr);
    // now we are finally ready to measure some letter to get the bounding box
  Fixed bBefore, bAfter, bAscent, bDescent;
  err = ATSUGetUnjustifiedBounds(layout, kATSUFromTextBeginning, 1, &bBefore, &bAfter, &bAscent, &bDescent);
    // Requesting a certain height font on Mac does not guarantee that ascent+descent
    // equal the requested height. fl_height will reflect the actual height that we got.
    // The font "Apple Chancery" is a pretty extreme example of overlapping letters.
  float fa = -FixedToFloat(bAscent), fd = -FixedToFloat(bDescent);
  if (fa>0.0f && fd>0.0f) {
    //float f = Size/(fa+fd);
    ascent = fa; //int(fa*f+0.5f);
    descent = fd; //Size - ascent;
  }
  int w = FixedToInt(bAfter);
  if (w)  
    q_width = FixedToInt(bAfter);

# define ENABLE_TRANSIENT_FONTS  1

# ifdef ENABLE_TRANSIENT_FONTS
  // Now, by way of experiment, try enabling Transient Font Matching, this will
  // cause ATSU to find a suitable font to render any chars the current font can't do...
  ATSUSetTransientFontMatching (layout, true);
# endif
#endif
}

Fl_FontSize* fl_fontsize = 0L;

Fl_FontSize::~Fl_FontSize() {
/*
#if HAVE_GL
 // ++ todo: remove OpenGL font alocations
// Delete list created by gl_draw().  This is not done by this code
// as it will link in GL unnecessarily.  There should be some kind
// of "free" routine pointer, or a subclass?
// if (listbase) {
//  int base = font->min_char_or_byte2;
//  int size = font->max_char_or_byte2-base+1;
//  int base = 0; int size = 256;
//  glDeleteLists(listbase+base,size);
// }
#endif
  */
  if (this == fl_fontsize) fl_fontsize = 0;
#ifdef __APPLE_QUARTZ__
  ATSUDisposeTextLayout(layout);
  ATSUDisposeStyle(style);
#endif
}

////////////////////////////////////////////////////////////////

static Fl_Fontdesc built_in_table[] = {
#ifdef __APPLE_QD__
{" Arial"},
{"BArial"},
{"IArial"},
{"PArial"},
{" Courier New"},
{"BCourier New"},
{"ICourier New"},
{"PCourier New"},
{" Times New Roman"},
{"BTimes New Roman"},
{"ITimes New Roman"},
{"PTimes New Roman"},
{" Symbol"},
{" Chicago"},
{"BChicago"},
{" Webdings"},
#elif defined(__APPLE_QUARTZ__)
{"Arial"},
{"Arial Bold"},
{"Arial Italic"},
{"Arial Bold Italic"},
{"Courier New"},
{"Courier New Bold"},
{"Courier New Italic"},
{"Courier New Bold Italic"},
{"Times New Roman"},
{"Times New Roman Bold"},
{"Times New Roman Italic"},
{"Times New Roman Bold Italic"},
{"Symbol"},
{"Monaco"},
{"Andale Mono"}, // there is no bold Monaco font on standard Mac
{"Webdings"},
#endif
};

#ifdef __APPLE_QUARTZ__
static UniChar *utf16buf = 0;
static int utf16len = 0;
UniChar *fl_macToUtf16(const char *txt, int len, int *new_len)
{
  wchar_t *wbuf = (wchar_t*)malloc(sizeof(wchar_t) *(len + 10));
  *new_len = fl_utf2unicode((const unsigned char*)txt, len, wbuf);
  if ((1 + *new_len) > utf16len) {
    utf16len = 100 + *new_len;
    free(utf16buf);
    utf16buf = (UniChar*)malloc((utf16len+1)*sizeof(UniChar));
  }
  int i;
  wchar_t *src = (wchar_t*)wbuf;
  UniChar *dst = utf16buf;
  for (i = *new_len;  i--;  ) *dst++ = *src++;
  *dst = 0;
  free(wbuf);
  return utf16buf;
}
#endif

Fl_Fontdesc* fl_fonts = built_in_table;

void fl_font(Fl_FontSize* s) {
  fl_fontsize = s;
#ifdef __APPLE_QD__
  if (fl_window) SetPort( GetWindowPort(fl_window) );
  TextFont(fl_fontsize->font);	//: select font into current QuickDraw GC
  TextFace(fl_fontsize->face);
  TextSize(fl_fontsize->size);
  if (!fl_fontsize->knowMetrics) {	//: get the true metrics for the currnet GC 
                                        //: (fails on multiple monitors with different dpi's!)
    FontInfo fi; GetFontInfo(&fi);
    fl_fontsize->ascent = fi.ascent;
    fl_fontsize->descent = fi.descent;
    FMetricRec mr; FontMetrics(&mr);
    short *f = (short*)*mr.wTabHandle; //: get the char size table
    for (int i=0; i<256; i++) fl_fontsize->width[i] = f[2*i];
    fl_fontsize->knowMetrics = 1;
  }
#elif defined(__APPLE_QUARTZ__)
  // we will use fl_fontsize later to access the required style and layout
#else
# error : need to defined either Quartz or Quickdraw
#endif
}

static Fl_FontSize* find(int fnum, int size) {
  Fl_Fontdesc* s = fl_fonts+fnum;
  if (!s->name) s = fl_fonts; // use 0 if fnum undefined
  Fl_FontSize* f;
  for (f = s->first; f; f = f->next)
    if (f->minsize <= size && f->maxsize >= size) return f;
  f = new Fl_FontSize(s->name, size);
  f->next = s->first;
  s->first = f;
  return f;
}

////////////////////////////////////////////////////////////////
// Public interface:

int fl_font_ = 0;
int fl_size_ = 0;

void fl_font(int fnum, int size) {
  if (fnum==-1) {
    fl_font_ = 0; 
    fl_size_ = 0;
    return;
  }
  fl_font_ = fnum;
  fl_size_ = size;
  fl_font(find(fnum, size));
}

int fl_height() {
  check_default_font();
  if (fl_fontsize) return fl_fontsize->ascent+fl_fontsize->descent;
  else return -1;
}

int fl_descent() {
  check_default_font();
  if (fl_fontsize) return fl_fontsize->descent;
  else return -1;
}

double fl_width(const UniChar* txt, int n) {
  check_default_font();
  if (!fl_fontsize) {
    check_default_font(); // avoid a crash!
    if (!fl_fontsize)
      return 8*n; // user must select a font first!
  }

      OSStatus err;
  Fixed bBefore, bAfter, bAscent, bDescent;
  ATSUTextLayout layout;
  ByteCount iSize;
  ATSUAttributeTag iTag;
  ATSUAttributeValuePtr iValuePtr;

// Here's my ATSU text measuring attempt... This seems to do the Right Thing
  // now collect our ATSU resources and measure our text string
  layout = fl_fontsize->layout;
        // activate the current GC
  iSize = sizeof(CGContextRef);
  iTag = kATSUCGContextTag;
  iValuePtr = &fl_gc;
      ATSUSetLayoutControls(layout, 1, &iTag, &iSize, &iValuePtr);
        // now measure the bounding box
  err = ATSUSetTextPointerLocation(layout, txt, kATSUFromTextBeginning, n, n);
  err = ATSUGetUnjustifiedBounds(layout, kATSUFromTextBeginning, n, &bBefore, &bAfter, &bAscent, &bDescent);
  // If err is OK then return length, else return 0. Or something...
  int len = FixedToInt(bAfter);
  return len;
}

double fl_width(const char* txt, int n) {
#ifdef __APPLE_QD__
  return (double)TextWidth(txt, 0, n);
#else
  int wc_len = n;
  UniChar *uniStr = fl_macToUtf16(txt, n, &wc_len);
  return fl_width(uniStr, wc_len);
#endif
}

double fl_width(uchar c) {
  return fl_width((const char*)(&c), 1);
}

double fl_width(unsigned int wc) {
  return fl_width((const UniChar*)(&wc), 1);
}

void fl_draw(const char *str, int n, float x, float y);

void fl_draw(const char* str, int n, int x, int y) {
#ifdef __APPLE_QD__
  MoveTo(x, y);
  DrawText((const char *)str, 0, n);
#elif defined(__APPLE_QUARTZ__)
  fl_draw(str, n, (float)x-0.0f, (float)y-0.5f);
#else
#  error : neither Quartz no Quickdraw chosen
#endif
}

void fl_draw(const char *str, int n, float x, float y) {
#ifdef __APPLE_QD__
  fl_draw(str, n, (int)x, (int)y);
#elif defined(__APPLE_QUARTZ__)
  OSStatus err;
    // convert to UTF-16 first 
  UniChar *uniStr = fl_macToUtf16(str, n, &n);

  // avoid a crash if no font has been selected by user yet !
  check_default_font();
    // now collect our ATSU resources
  ATSUTextLayout layout = fl_fontsize->layout;

  ByteCount iSize = sizeof(CGContextRef);
  ATSUAttributeTag iTag = kATSUCGContextTag;
  ATSUAttributeValuePtr iValuePtr=&fl_gc;
  ATSUSetLayoutControls(layout, 1, &iTag, &iSize, &iValuePtr);

  err = ATSUSetTextPointerLocation(layout, uniStr, kATSUFromTextBeginning, n, n);
  err = ATSUDrawText(layout, kATSUFromTextBeginning, n, FloatToFixed(x), FloatToFixed(y));
#else
#  error : neither Quartz no Quickdraw chosen
#endif
}

void fl_rtl_draw(const char *str, int n, int x, int y) {
}

//
// End of "$Id: fl_font_mac.cxx 5651 2007-02-01 20:13:57Z matt $".
//
