
// "$Id: $"
//
// Unicode to UTF-8 conversion functions.
//
//                Copyright 2000-2003 by O'ksi'D.
//                      All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
//
//      Redistributions of source code must retain the above copyright
//      notice, this list of conditions and the following disclaimer.
//
//      Redistributions in binary form must reproduce the above copyright
//      notice, this list of conditions and the following disclaimer in the
//      documentation and/or other materials provided with the distribution.
//
//      Neither the name of O'ksi'D nor the names of its contributors
//      may be used to endorse or promote products derived from this software
//      without specific prior written permission.
//
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER
// OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//



#include <config.h>
#include <FL/filename.H>
#include <stdarg.h>

#if defined(WIN32)
#include <ctype.h>
#include <io.h>
#include <direct.h>
#include <windows.h>
#include <winbase.h>
#include <process.h>

extern "C" {
  int XUtf8Tolower(int ucs);
  unsigned short XUtf8IsNonSpacing(unsigned int ucs);
};

#elif defined(__APPLE__)
#include <stdio.h>
#include <time.h>
#include <fcntl.h>
#include <unistd.h>
#include <wchar.h>
#include <stdlib.h>

extern "C" {
  int XUtf8Tolower(int ucs);
  unsigned short XUtf8IsNonSpacing(unsigned int ucs);
};

#else

#include <FL/Xutf8.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#endif //WIN32

#include <FL/fl_utf8.H>
#include <string.h>
#include <stdlib.h>

#undef fl_open

/*** NOTE : all functions are LIMITED to 24 bits Unicode values !!! ***/
/***        but only 16 bits are realy used under Linux and win32  ***/


#if MSDOS
#define NBC 255
#else
#define NBC 0xFFFF + 1
#endif
static int 
Toupper(
	int ucs)
{
	long i;
	static unsigned short *table = NULL;

	if (!table) {
		table = (unsigned short*) malloc(
			sizeof(unsigned short) * (NBC));
		for (i = 0; i < NBC; i++) {
			table[i] = (unsigned short) i;
		}	
		for (i = 0; i < NBC; i++) {
			int l;
			l = XUtf8Tolower(i);			
			if (l != i) table[l] = (unsigned short) i;
		}	

	}
	if (ucs >= NBC || ucs < 0) return ucs;
	return table[ucs];
}


/*** converts the first char UTF-8 string to an Unicode value ***/
/*** returns the byte length of the converted UTF-8 char **/
/*** returns -1 if the UTF-8 string is not valid ***/
int
fl_utf2ucs(
        const unsigned char     *buf,
        int                     len,
        unsigned int          	*ucs)
{
      if (buf[0] & 0x80) {
        if (buf[0] & 0x40) {
          if (buf[0] & 0x20) {
            if (buf[0] & 0x10) {
              if (buf[0] & 0x08) {
                if (buf[0] & 0x04) {
                  if (buf[0] & 0x02) {
                        /* bad UTF-8 string */
                  } else {
                        /* 0x04000000 - 0x7FFFFFFF */
                  }
                } else if (len > 4) {
                  /* 0x00200000 - 0x03FFFFFF */
                  *ucs =  ((buf[0] & ~0xF8) << 24) +
                          ((buf[1] & ~0x80) << 18) +
                          ((buf[2] & ~0x80) << 12) +
                          ((buf[3] & ~0x80) << 6) +
                           (buf[4] & ~0x80);
                  if (*ucs > 0x001FFFFF && *ucs < 0x01000000) return 5;
                }
              } else if (len > 3) {
                /* 0x00010000 - 0x001FFFFF */
                *ucs =  ((buf[0] & ~0xF0) << 18) +
                        ((buf[1] & ~0x80) << 12) +
                        ((buf[2] & ~0x80) << 6) +
                         (buf[3] & ~0x80);
                if (*ucs > 0x0000FFFF) return 4;
              }
            } else if (len > 2) {
              /* 0x00000800 - 0x0000FFFF */
              *ucs =  ((buf[0] & ~0xE0) << 12) +
                      ((buf[1] & ~0x80) << 6) +
                       (buf[2] & ~0x80);
              if (*ucs > 0x000007FF) return 3;
            }
          } else if (len > 1) {
            /* 0x00000080 - 0x000007FF */
            *ucs = ((buf[0] & ~0xC0) << 6) +
                    (buf[1] & ~0x80);
            if (*ucs > 0x0000007F) return 2;
          }
        }
      } else if (len > 0) {
        /* 0x00000000 - 0x0000007F */
        *ucs = buf[0];
        return 1;
      }

      *ucs = (unsigned int) '?'; /* bad utf-8 string */
      return -1;
}

/*** converts an Unicode value to an UTF-8 string  ***/
/*** NOTE : the buffer (buf) must be at least 5 bytes long !!!  ***/
int 
fl_ucs2utf(
	unsigned int 	ucs, 
	char 		*buf)
{
	if (ucs < 0x000080) {
		buf[0] = ucs;
		return 1;
	} else if (ucs < 0x000800) {
		buf[0] = 0xC0 | (ucs >> 6);
		buf[1] = 0x80 | (ucs & 0x3F);
		return 2;
	} else if (ucs < 0x010000) { 
		buf[0] = 0xE0 | (ucs >> 12);
		buf[1] = 0x80 | ((ucs >> 6) & 0x3F);
		buf[2] = 0x80 | (ucs & 0x3F);
		return 3;
	} else if (ucs < 0x00200000) {
		buf[0] = 0xF0 | (ucs >> 18);
		buf[1] = 0x80 | ((ucs >> 12) & 0x3F);
		buf[2] = 0x80 | ((ucs >> 6) & 0x3F);
		buf[3] = 0x80 | (ucs & 0x3F);
		return 4;
	} else if (ucs < 0x01000000) {
		buf[0] = 0xF8 | (ucs >> 24);
		buf[1] = 0x80 | ((ucs >> 18) & 0x3F);
		buf[2] = 0x80 | ((ucs >> 12) & 0x3F);
		buf[3] = 0x80 | ((ucs >> 6) & 0x3F);
		buf[4] = 0x80 | (ucs & 0x3F);
		return 5;
	}
	buf[0] = '?';
	return -1;
}

/*** returns the byte length of the first UTF-8 char ***/
/*** (returns -1 if not valid) ***/
int
fl_utflen(
        const unsigned char     *buf,
        int                     len)
{
	unsigned int ucs;
	return fl_utf2ucs(buf, len, &ucs);
}

/**** returns the number of Unicode chars in the UTF-8 string ***/
int 
fl_utf_nb_char(
	const unsigned char 	*buf, 
	int 			len)
{
	int i = 0;
	int nbc = 0;
	while (i < len) {
		int cl = fl_utflen(buf + i, len - i);
		if (cl < 1) cl = 1;
		nbc++;
		i += cl;
	}
	return nbc;
}

/*
 * compare only the first n bytes
 * return 0 if the strings are equal;
 * return 1 if s1 is greater than s2
 * return -1 if s1 is less than s2
 */
int fl_utf_strncasecmp(const char *s1, const char *s2, int n)
{
        int i;
        int s1_l;
        int s2_l;

        s1_l = 0;
        while (s1_l < n && s1[s1_l]) s1_l++;
        s2_l = 0;
        while (s2_l < n && s2[s2_l]) s2_l++;

        if (s1_l < s2_l) {
                return -1;
        } else if (s1_l > s2_l) {
                return 1;
        }
        for (i = 0; i < n;) {
                int l1, l2;
                unsigned int u1, u2;
                int res;

                l1 = fl_utf2ucs((unsigned char*)s1 + i, n - i, &u1);
                l2 = fl_utf2ucs((unsigned char*)s2 + i, n - i, &u2);
                if (l1 - l2 != 0) return l1 - l2;
                res = XUtf8Tolower(u1) - XUtf8Tolower(u2);
                if (res != 0) return res;
                if (l1 < 1) {
                        i += 1;
                } else {
                        i += l1;
                }
        }
        return 0;
}

/*
 * return 0 if the strings are equal;
 * return 1 if s1 is greater than s2
 * return -1 if s1 is less than s2
 */
int fl_utf_strcasecmp(const char *s1, const char *s2)
{
	int s1_l = strlen(s1);
	int s2_l = strlen(s2);

        if (s1_l < s2_l) {
                return -1;
        } else if (s1_l > s2_l) {
                return 1;
	}	
	return fl_utf_strncasecmp(s1, s2, s1_l);	
}

/*
 * return the Unicode lower case value of ucs
 */
int fl_tolower(unsigned int ucs) 
{
	return XUtf8Tolower(ucs);
}

/*
 * return the Unicode upper case value of ucs
 */
int fl_toupper(unsigned int ucs)
{
	return Toupper(ucs);
}

/*
 * converts the str string to the lower case equivalent into buf.
 * Warning: to be safe buf length must be at least 3 * len
 */
int fl_utf_tolower(const unsigned char *str, int len, char *buf)
{
	int i;
	int l = 0;
        for (i = 0; i < len;) {
                int l1, l2;
                unsigned int u1;

                l1 = fl_utf2ucs((unsigned char*)str + i, len - i, &u1);
                l2 = fl_ucs2utf((unsigned int) XUtf8Tolower(u1), buf + l);
                if (l1 < 1) {
                        i += 1;
                } else {
                        i += l1;
		}
                if (l2 < 1) {
                        l += 1;
                } else {
                        l += l2;
		}

	}
	return l;
}

/*
 * converts the str string to the upper case equivalent into buf.
 * Warning: to be safe buf length must be at least 3 * len
 */
int fl_utf_toupper(const unsigned char *str, int len, char *buf)
{
	int i;
	int l = 0;
        for (i = 0; i < len;) {
                int l1, l2;
                unsigned int u1;

                l1 = fl_utf2ucs((unsigned char*)str + i, len - i, &u1);
                l2 = fl_ucs2utf((unsigned int) Toupper(u1), buf + l);
                if (l1 < 1) {
                        i += 1;
                } else {
                        i += l1;
		}
                if (l2 < 1) {
                        l += 1;
                } else {
                        l += l2;
		}
	}
	return l;
}

 
/*
 * converts a UTF-8 str to unicode
 * Warning: buf must a least len long
 */
int fl_utf2unicode(const unsigned char *str, int len, xchar *buf)
{
	int i;
	int l = 0;
        for (i = 0; i < len;) {
                unsigned int u1;
		int l1;

                l1 = fl_utf2ucs((unsigned char*)str + i, len - i, &u1);
		buf[l] = (xchar) u1;
                if (l1 < 1) {
                        i += 1;
                } else {
                        i += l1;
		}
		l++;

	}
	return l;
}

/*
 * convert Unicode str to UTF-8
 * Warning: buf must be at least 3 * len long
 */
int fl_unicode2utf(const xchar *str, int len, char *buf)
{
	int i;
	int l = 0;
        for (i = 0; i < len; i++) {
		int l1;
		l1 = fl_ucs2utf((unsigned int) str[i], buf + l);
                if (l1 < 1) {
                        l += 1;
                } else {
                        l += l1;
		}

	}
	return l;
}

/*
 * convert UTF-8 str to latin1
 * Warning: buf must be at least len long
 */
int fl_utf2latin1(const unsigned char *str, int len, char *buf)
{
	int i;
	int l = 0;
        for (i = 0; i < len;) {
                unsigned int u1;
		int l1;

                l1 = fl_utf2ucs((unsigned char*)str + i, len - i, &u1);
		if (u1 > 0xFF) u1 = '?';
		buf[l] = (char) u1;
                if (l1 < 1) {
                        i += 1;
                } else {
                        i += l1;
		}
		l++;

	}
	return l;
}

/*
 * convert latin1 str to UTF-8
 * Warning: buf must be at least 2 * len long
 */
int fl_latin12utf(const unsigned char *str, int len, char *buf)
{
	int i;
	int l = 0;
	int l1 = 0;
        for (i = 0; i < len; i++) {
		unsigned int n = (unsigned int) str[i];
		l1 = fl_ucs2utf(n, buf + l);
                if (l1 < 1) {
                        l = l + 1;
                } else {
                        l = l + l1;
		}

	}
	return l;
}

unsigned int fl_nonspacing(unsigned int ucs)
{
	return (unsigned int) XUtf8IsNonSpacing(ucs);
}

#ifdef WIN32

static xchar *mbwbuf = NULL;
unsigned int fl_codepage = 0;
static char *buf = NULL;
static int buf_len = 0;
static unsigned short *wbufa = NULL;

char *fl_utf82locale(const char *s, int len, UINT codepage)
{
	if (!s) return "";
	int l = 0;
	if (buf_len < len * 2 + 1) {
		buf_len = len * 2 + 1;
		buf = (char*) realloc(buf, buf_len);
		wbufa = (unsigned short*) realloc(wbufa, buf_len * sizeof(short));
	}
	if (codepage < 1) codepage = fl_codepage;
	l = fl_utf2unicode((const unsigned char *)s, len, (xchar*) wbufa);
	buf[l] = 0;
	l = WideCharToMultiByte(codepage, 0, (WCHAR*)wbufa, l, buf, 
		buf_len, NULL, NULL);
	if (l < 0) l = 0;
	buf[l] = 0;
	return buf;	
}

char *fl_locale2utf8(const char *s, int len, UINT codepage)
{
	if (!s) return "";
	int l = 0;
	if (buf_len < len * 5 + 1) {
		buf_len = len * 5 + 1;
		buf = (char*) realloc(buf, buf_len);
		wbufa = (unsigned short*) realloc(wbufa, buf_len * sizeof(short));
	}
	if (codepage < 1) codepage = fl_codepage;
	buf[l] = 0;
	
	l = MultiByteToWideChar(codepage, 0, s, len, (WCHAR*)wbufa, buf_len);
	if (l < 0) l = 0;
	wbufa[l] = 0;
	l = fl_unicode2utf((xchar*)wbufa, l, buf);
	buf[l] = 0;
	return buf;	
}
#endif

char * fl_utf2mbcs(const char *s)
{
	if (!s) return NULL;
#if defined(WIN32)
	int l = strlen(s);
	static char *buf = NULL;

	mbwbuf = (xchar*)realloc(mbwbuf, (l+6) * sizeof(xchar));
	l = fl_utf2unicode((unsigned char*)s, l, mbwbuf);
	mbwbuf[l] = 0;
	buf = (char*)realloc(buf, l * 6 + 1);
	l = wcstombs(buf, mbwbuf, l * 6);
	buf[l] = 0;
	return buf;
#else
	return (char*) s;
#endif
}

char * fl_mbcs2utf(const char *s)
{
	if (!s) return NULL;
#if defined(WIN32)
	int l = strlen(s);
	static char *buf = NULL;
	
	mbwbuf = (xchar*)realloc(mbwbuf,
		(l * 6 +6) * sizeof(xchar));
	l = mbstowcs(mbwbuf, s, l);
	buf = (char*)realloc(buf, l * 6 + 1);
	l = fl_unicode2utf(mbwbuf, l, buf);
	buf[l] = 0;
	return buf;
#else
	return (char*) s;
#endif
}

int fl_is_nt4(void)
{
#ifdef WIN32
	static int ret = -1;
	if (ret >= 0) return ret;
	OSVERSIONINFO o;
	o.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	GetVersionEx(&o);
	if (o.dwPlatformId == VER_PLATFORM_WIN32_NT &&
		o.dwMajorVersion >= 4)
	{
		ret = (int) o.dwMajorVersion;
	} else {
		ret = 0;
	}
	return ret;
#else
	return 0;
#endif
}


#if defined(WIN32)
static xchar *wbuf = NULL;
static xchar *wbuf1 = NULL;
#endif 


char *fl_getenv(const char* v)
{
#ifdef WIN32
	int l = strlen(v);
	static xchar* wbuf = NULL;
	wbuf = (xchar*)realloc(wbuf, sizeof(xchar) * (l+1));
	wbuf[fl_utf2unicode((const unsigned char*)v, l, wbuf)] = 0;
	xchar *ret = _wgetenv(wbuf);
	static char *buf = NULL;
	if (ret) {
		l = wcslen(ret);
		buf = (char*) realloc(buf, l);
		buf[fl_unicode2utf(ret, l, buf)] = 0;
		return buf;
	} else {
		return NULL;
	}
#else 
	return getenv(v);
#endif
}

int fl_open(const char* f, int oflags, ...)
{
	int pmode;
	va_list ap;
	va_start(ap, oflags);
	pmode = va_arg (ap, int);
	va_end(ap);
#ifdef WIN32
	if (fl_is_nt4()) {
		int l = strlen(f);
		wbuf = (xchar*)realloc(wbuf, sizeof(xchar) * (l+1));
		wbuf[fl_utf2unicode((const unsigned char*)f, l, wbuf)] = 0;
		if (pmode == -1) return _wopen(wbuf, oflags);
		else return _wopen(wbuf, oflags, pmode);
	} else {
		if (pmode == -1) return _open(fl_utf2mbcs(f), oflags);
		else return _open(fl_utf2mbcs(f), oflags, pmode);
	}
#else
	if (pmode == -1) return open(f, oflags);
	else return open(f, oflags, pmode);
#endif
}

FILE *fl_fopen(const char* f, const char *mode)
{
#ifdef WIN32
	if (fl_is_nt4()) {
		int l = strlen(f);
		wbuf = (xchar*)realloc(wbuf, sizeof(xchar) * (l+1));
		wbuf[fl_utf2unicode((const unsigned char*)f, l, wbuf)] = 0;
		l = strlen(mode);
		wbuf1 = (xchar*)realloc(wbuf1, sizeof(xchar) * (l+1));
		wbuf1[fl_utf2unicode((const unsigned char*)mode, l, wbuf1)] = 0;
		return _wfopen(wbuf, wbuf1);
	} else {
		return fopen(fl_utf2mbcs(f), mode);
	}
#else
	return fopen(f, mode);
#endif
}

int fl_system(const char* f)
{
#ifdef WIN32
#ifdef __MINGW32__
	return system(fl_utf2mbcs(f));
#else
	if (fl_is_nt4()) {
		int l = strlen(f);
		wbuf = (xchar*)realloc(wbuf, sizeof(xchar) * (l+1));
		wbuf[fl_utf2unicode((const unsigned char*)f, l, wbuf)] = 0;	
		return _wsystem(wbuf);
	} else {
		return system(fl_utf2mbcs(f));
	}
#endif
#else
	return system(f);
#endif
}

int fl_execvp(const char *file, char *const *argv)
{
#ifdef WIN32
#ifdef __MINGW32__
	return _execvp(fl_utf2mbcs(file), argv);
#else
	if (fl_is_nt4()) {
		int l = strlen(file);
		int i, n, ret;
		xchar **ar;
		wbuf = (xchar*)realloc(wbuf, sizeof(xchar) * (l+1));
		wbuf[fl_utf2unicode((const unsigned char*)file, l, wbuf)] = 0;

		i = 0; n = 0;
		while (argv[i]) {i++; n++;}
		ar = (xchar**) malloc(sizeof(xchar*) * (n + 1));
		i = 0;
		while (i <= n) {
			l = strlen(argv[i]);
			ar[i] = (xchar*)malloc(sizeof(xchar) * (l+1));
			ar[i][fl_utf2unicode(
				(const unsigned char*)argv[i], 
				l, ar[i])] = 0;
			i++;
		}
		ar[n] = NULL;
		ret = _wexecvp(wbuf, ar);
		i = 0;
		while (i <= n) {
			free(ar[i]);
			i++;
		}
		free(ar);
		return ret;
	} else {
		return _execvp(fl_utf2mbcs(file), argv);
	}
#endif
#else
	return execvp(file, argv);
#endif
}



int fl_chmod(const char* f, int mode)
{
#ifdef WIN32
	if (fl_is_nt4()) {
		int l = strlen(f);
		wbuf = (xchar*)realloc(wbuf, sizeof(xchar) * (l+1));
		wbuf[fl_utf2unicode((const unsigned char*)f, l, wbuf)] = 0;
		return _wchmod(wbuf, mode);
	} else {
		return _chmod(fl_utf2mbcs(f), mode);
	}
#else
	return chmod(f, mode);
#endif
}

int fl_access(const char* f, int mode)
{
#ifdef WIN32
	if (fl_is_nt4()) {
		int l = strlen(f);
		wbuf = (xchar*)realloc(wbuf, sizeof(xchar) * (l+1));
		wbuf[fl_utf2unicode((const unsigned char*)f, l, wbuf)] = 0;
		return _waccess(wbuf, mode);
	} else {
		return _access(fl_utf2mbcs(f), mode);
	}
#else
	return access(f, mode);
#endif
}


int fl_stat(const char* f, struct stat *b)
{
#ifdef WIN32
	if (fl_is_nt4()) {
		int l = strlen(f);
		wbuf = (xchar*)realloc(wbuf, sizeof(xchar) * (l+1));
		wbuf[fl_utf2unicode((const unsigned char*)f, l, wbuf)] = 0;
		return _wstat(wbuf, (struct _stat*)b);
	} else {
		return _stat(fl_utf2mbcs(f), (struct _stat*)b);
	}
#else
	return stat(f, b);
#endif
}

char *fl_getcwd(char* b, int l)
{
	if (b == NULL) {
		b = (char*) malloc(l+1);
	}
#ifdef WIN32
	if (fl_is_nt4()) {
		static xchar *wbuf = NULL;
		wbuf = (xchar*)realloc(wbuf, sizeof(xchar) * (l+1));
		xchar *ret = _wgetcwd(wbuf, l / 5);
		if (ret) {
			l = wcslen(wbuf);
			b[fl_unicode2utf(wbuf, l, b)] = 0;
			return b;
		} else {
			return NULL;
		}
	} else {
		char *ret = _getcwd(b, l / 5);
		if (ret) {
			char *s = fl_mbcs2utf(b);
			l = strlen(s);
			for(int i= 0; i < l; i++) {
				b[i] = s[i];
			}
			b[l+1] = 0;
			return b;
		} else {
			return NULL;
		}
	}
#elif MSDOS
	b[0] = 0;
	return b;
#else
	return getcwd(b, l);
#endif
}


int fl_unlink(const char* f)
{
#ifdef WIN32
	if (fl_is_nt4()) {
		int l = strlen(f);
		wbuf = (xchar*)realloc(wbuf, sizeof(xchar) * (l+1));
		wbuf[fl_utf2unicode((const unsigned char*)f, l, wbuf)] = 0;
		return _wunlink(wbuf);
	} else {
		return _unlink(fl_utf2mbcs(f));
	}
#else
	return unlink(f);
#endif
}

int fl_mkdir(const char* f, int mode)
{
#if defined(WIN32) && !defined(__CYGWIN__)
	if (fl_is_nt4()) {
		int l = strlen(f);
		wbuf = (xchar*)realloc(wbuf, sizeof(short) * (l+1));
		wbuf[fl_utf2unicode((const unsigned char*)f, l, wbuf)] = 0;
		return _wmkdir(wbuf);
	} else {
		return _mkdir(fl_utf2mbcs(f));
	}
#elif MSDOS
	return -1;
#else
	return mkdir(f, mode);
#endif
}


int fl_rmdir(const char* f)
{
#ifdef WIN32
	if (fl_is_nt4()) {
		int l = strlen(f);
		wbuf = (xchar*)realloc(wbuf, sizeof(xchar) * (l+1));
		wbuf[fl_utf2unicode((const unsigned char*)f, l, wbuf)] = 0;
		return _wrmdir(wbuf);
	} else {
		return _rmdir(fl_utf2mbcs(f));
	}
#else
	return rmdir(f);
#endif
}

int fl_rename(const char* f, const char *n)
{
#ifdef WIN32
	if (fl_is_nt4()) {
		int l = strlen(f);
		wbuf = (xchar*)realloc(wbuf, sizeof(xchar) * (l+1));
		wbuf[fl_utf2unicode((const unsigned char*)f, l, wbuf)] = 0;
		l = strlen(n);
		wbuf1 = (xchar*)realloc(wbuf1, sizeof(xchar) * (l+1));
		wbuf1[fl_utf2unicode((const unsigned char*)n, l, wbuf1)] = 0;
		return _wrename(wbuf, wbuf1);
	} else {
		int ret;
		char *s = strdup(fl_utf2mbcs(f));
		ret = rename(s, fl_utf2mbcs(n));
		free(s);
		return ret;
	}
#else
	return rename(f, n);
#endif
}

// recursively create a path in the file system
char fl_make_path( const char *path ) {
  if (fl_access(path, 0)) {
    const char *s = strrchr( path, '/' );
    if ( !s ) return 0;
    int len = s-path;
    char *p = (char*)malloc( len+1 );
    memcpy( p, path, len );
    p[len] = 0;
    fl_make_path( p );
    free( p );
    fl_mkdir(path, 0700);
  }
  return 1;
}

// strip the filename and create a path
void fl_make_path_for_file( const char *path )
{
  const char *s = strrchr( path, '/' );
  if ( !s ) return;
  int len = s-path;
  char *p = (char*)malloc( len+1 );
  memcpy( p, path, len );
  p[len] = 0;
  fl_make_path( p );
  free( p );
}

// Returns non-zero value if character allows line-break after it
int
fl_is_linebreak(unsigned int ucs)
{
  /* Space or tab */
  if (ucs == ' ' || ucs == '\t')  return 1;
  
  /* U+2E80..U+2EFF: CJK Radical Supplement */
  /* U+2F00..U+2FDF: Kangxi Radicals */
  /* U+2FF0..U+2FFF: Ideographic Description Characters */
  /* U+3000..U+303F: CJK Symbols and Punctuation */
  /* U+3040..U+309F: Hiragana */
  /* U+30A0..U+30FF: Katakana */
  /* U+3100..U+312F: Bopomofo */
  /* U+3130..U+318F: Hangul Compatibility Jamo */
  /* U+3190..U+319F: Kanbun */
  /* U+31A0..U+31BF: Bopomofo Extended */
  /* U+31C0..U+31EF: CJK Strokes */
  /* U+31F0..U+31FF: Katakana Phonetic Extensions */
  /* U+3200..U+32FF: Enclosed CJK Letters and Months */
  /* U+3300..U+33FF: CJK Compatibility */
  /* U+3400..U+4DB5: CJK Ideographs Extension A */
  if (ucs >= 0x2e80 && ucs <= 0x4db5)  return 1;

  /* U+4E00..U+9FBB: CJK Ideographs */
  if (ucs >= 0x4e00 && ucs <= 0x9fbb)  return 1;

  /* U+A000..U+A48F: Yi Syllables */
  /* U+A490..U+A4CF: Yi Radicals */
  if (ucs >= 0xa000 && ucs <= 0xa4cf)  return 1;

  /* U+F900..U+FAFF: CJK Compatibility Ideographs */
  if (ucs >= 0xf900 && ucs <= 0xfaff)  return 1;

  /* U+FE30..U+FE4F: CJK Compatibility Forms */
  /* U+FE50..U+FE6F: Small Form Variants */
  if (ucs >= 0xfe30 && ucs <= 0xfe6f)  return 1;

  /* U+FF00..U+FFEF: Half-Width CJK Symbols and Punctuation */
  if (ucs == 0xff0c || ucs == 0xff0e || ucs == 0xff1a || ucs == 0xff1b ||
    (ucs >= 0xff60 && ucs <= 0xffdf))  return 1;

  return 0;
}

//
// End of "$Id: $".
//
 
