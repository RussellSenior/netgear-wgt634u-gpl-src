/* 
   Unix SMB/Netbios implementation.
   Version 1.9.
   Kanji Extensions
   Copyright (C) Andrew Tridgell 1992-1994
   
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
   
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   
   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

   Adding for Japanese language by <fujita@ainix.isac.co.jp> 1994.9.5
     and extend coding system to EUC/SJIS/JIS/HEX at 1994.10.11
     and add all jis codes sequence type at 1995.8.16
     Notes: Hexadecimal code by <ohki@gssm.otuka.tsukuba.ac.jp>
*/
#ifdef KANJI

#define _KANJI_C_
#include "includes.h"

/* coding system keep in */
int coding_system = SJIS_CODE;

/* jis si/so sequence */
char jis_kso = JIS_KSO;
char jis_ksi = JIS_KSI;
char hex_tag = HEXTAG;

/*******************************************************************
  SHIFT JIS functions
********************************************************************/
/*******************************************************************
 search token from S1 separated any char of S2
 S1 contain SHIFT JIS chars.
********************************************************************/
char *
sj_strtok (char *s1, const char *s2)
{
    static char *s = NULL;
    char *q;
    if (!s1) {
	if (!s) {
	    return NULL;
	}
	s1 = s;
    }
    for (q = s1; *s1; ) {
	if (is_shift_jis (*s1)) {
	    s1 += 2;
	} else if (is_kana (*s1)) {
	    s1++;
	} else {
	    char *p = strchr (s2, *s1);
	    if (p) {
		if (s1 != q) {
		    s = s1 + 1;
		    *s1 = '\0';
		    return q;
		}
		q = s1 + 1;
	    }
	    s1++;
	}
    }
    s = NULL;
    if (*q) {
	return q;
    }
    return NULL;
}

/*******************************************************************
 search string S2 from S1
 S1 contain SHIFT JIS chars.
********************************************************************/
char *
sj_strstr (const char *s1, const char *s2)
{
    register int len = strlen ((char *) s2);
    if (!*s2) 
	return (char *) s1;
    for (;*s1;) {
	if (*s1 == *s2) {
	    if (strncmp (s1, s2, len) == 0)
		return (char *) s1;
	}
	if (is_shift_jis (*s1)) {
	    s1 += 2;
	} else {
	    s1++;
	}
    }
    return 0;
}

/*******************************************************************
 Search char C from beginning of S.
 S contain SHIFT JIS chars.
********************************************************************/
char *
sj_strchr (const char *s, int c)
{
    for (; *s; ) {
	if (*s == c)
	    return (char *) s;
	if (is_shift_jis (*s)) {
	    s += 2;
	} else {
	    s++;
	}
    }
    return 0;
}

/*******************************************************************
 Search char C end of S.
 S contain SHIFT JIS chars.
********************************************************************/
char *
sj_strrchr (const char *s, int c)
{
    register char *q;

    for (q = 0; *s; ) {
	if (*s == c) {
	    q = (char *) s;
	}
	if (is_shift_jis (*s)) {
	    s += 2;
	} else {
	    s++;
	}
    }
    return q;
}

/*******************************************************************
  Code conversion
********************************************************************/
/* convesion buffer */
static char cvtbuf[1024];

/*******************************************************************
  EUC <-> SJIS
********************************************************************/
static int
euc2sjis (register int hi, register int lo)
{
    if (hi & 1)
	return ((hi / 2 + (hi < 0xdf ? 0x31 : 0x71)) << 8) |
	    (lo - (lo >= 0xe0 ? 0x60 : 0x61));
    else
	return ((hi / 2 + (hi < 0xdf ? 0x30 : 0x70)) << 8) | (lo - 2);
}

static int
sjis2euc (register int hi, register int lo)
{
    if (lo >= 0x9f)
	return ((hi * 2 - (hi >= 0xe0 ? 0xe0 : 0x60)) << 8) | (lo + 2);
    else
	return ((hi * 2 - (hi >= 0xe0 ? 0xe1 : 0x61)) << 8) |
	    (lo + (lo >= 0x7f ? 0x60 : 0x61));
}

/*******************************************************************
 Convert FROM contain SHIFT JIS codes to EUC codes
 return converted buffer
********************************************************************/
static char *
sj_to_euc (const char *from, BOOL overwrite)
{
    register char *out;
    char *save;

    save = (char *) from;
    for (out = cvtbuf; *from;) {
	if (is_shift_jis (*from)) {
	    int code = sjis2euc ((int) from[0] & 0xff, (int) from[1] & 0xff);
	    *out++ = (code >> 8) & 0xff;
	    *out++ = code;
	    from += 2;
	} else if (is_kana (*from)) {
	    *out++ = euc_kana;
	    *out++ = *from++;
	} else {
	    *out++ = *from++;
	}
    }
    *out = 0;
    if (overwrite) {
	strcpy((char *) save, (char *) cvtbuf);
	return (char *) save;
    } else {
	return cvtbuf;
    }
}

/*******************************************************************
 Convert FROM contain EUC codes to SHIFT JIS codes
 return converted buffer
********************************************************************/
static char *
euc_to_sj (const char *from, BOOL overwrite)
{
    register char *out;
    char *save;

    save = (char *) from;
    for (out = cvtbuf; *from; ) {
	if (is_euc (*from)) {
	    int code = euc2sjis ((int) from[0] & 0xff, (int) from[1] & 0xff);
	    *out++ = (code >> 8) & 0xff;
	    *out++ = code;
	    from += 2;
	} else if (is_euc_kana (*from)) {
	    *out++ = from[1];
	    from += 2;
	} else {
	    *out++ = *from++;
	}
    }
    *out = 0;
    if (overwrite) {
	strcpy(save, (char *) cvtbuf);
	return save;
    } else {
	return cvtbuf;
    }
}

/*******************************************************************
  JIS7,JIS8,JUNET <-> SJIS
********************************************************************/
static int
sjis2jis (register int hi, register int lo)
{
    if (lo >= 0x9f)
	return ((hi * 2 - (hi >= 0xe0 ? 0x160 : 0xe0)) << 8) | (lo - 0x7e);
    else
	return ((hi * 2 - (hi >= 0xe0 ? 0x161 : 0xe1)) << 8) |
	    (lo - (lo >= 0x7f ? 0x20 : 0x1f));
}

static int
jis2sjis (register int hi, register int lo)
{
    if (hi & 1)
	return ((hi / 2 + (hi < 0x5f ? 0x71 : 0xb1)) << 8) |
	    (lo + (lo >= 0x60 ? 0x20 : 0x1f));
    else
	return ((hi / 2 + (hi < 0x5f ? 0x70 : 0xb0)) << 8) | (lo + 0x7e);
}

/*******************************************************************
 Convert FROM contain JIS codes to SHIFT JIS codes
 return converted buffer
********************************************************************/
static char *
jis8_to_sj (const char *from, BOOL overwrite)
{
    register char *out;
    register int shifted;
    char *save;

    shifted = _KJ_ROMAN;
    save = (char *) from;
    for (out = cvtbuf; *from;) {
	if (is_esc (*from)) {
	    if (is_so1 (from[1]) && is_so2 (from[2])) {
		shifted = _KJ_KANJI;
		from += 3;
	    } else if (is_si1 (from[1]) && is_si2 (from[2])) {
		shifted = _KJ_ROMAN;
		from += 3;
	    } else {			/* sequence error */
		goto normal;
	    }
	} else {
	normal:
	    switch (shifted) {
	    default:
	    case _KJ_ROMAN:
		*out++ = *from++;
		break;
	    case _KJ_KANJI:
		{
		    int code = jis2sjis ((int) from[0] & 0xff, (int) from[1] & 0xff);
		    *out++ = (code >> 8) & 0xff;
		    *out++ = code;
		    from += 2;
		}
		break;
	    }
	}
    }
    *out = 0;
    if (overwrite) {
	strcpy (save, (char *) cvtbuf);
	return save;
    } else {
	return cvtbuf;
    }
}

/*******************************************************************
 Convert FROM contain SHIFT JIS codes to JIS codes
 return converted buffer
********************************************************************/
static char *
sj_to_jis8 (const char *from, BOOL overwrite)
{
    register char *out;
    register int shifted;
    char *save;

    shifted = _KJ_ROMAN;
    save = (char *) from;
    for (out = cvtbuf; *from; ) {
	if (is_shift_jis (*from)) {
	    int code;
	    switch (shifted) {
	    case _KJ_ROMAN:		/* to KANJI */
		*out++ = jis_esc;
		*out++ = jis_so1;
		*out++ = jis_kso;
		shifted = _KJ_KANJI;
		break;
	    }
	    code = sjis2jis ((int) from[0] & 0xff, (int) from[1] & 0xff);
	    *out++ = (code >> 8) & 0xff;
	    *out++ = code;
	    from += 2;
	} else {
	    switch (shifted) {
	    case _KJ_KANJI:		/* to ROMAN/KANA */
		*out++ = jis_esc;
		*out++ = jis_si1;
		*out++ = jis_ksi;
		shifted = _KJ_ROMAN;
		break;
	    }
	    *out++ = *from++;
	}
    }
    switch (shifted) {
    case _KJ_KANJI:			/* to ROMAN/KANA */
	*out++ = jis_esc;
	*out++ = jis_si1;
	*out++ = jis_ksi;
	shifted = _KJ_ROMAN;
	break;
    }
    *out = 0;
    if (overwrite) {
	strcpy (save, (char *) cvtbuf);
	return save;
    } else {
	return cvtbuf;
    }
}

/*******************************************************************
 Convert FROM contain 7 bits JIS codes to SHIFT JIS codes
 return converted buffer
********************************************************************/
static char *
jis7_to_sj (const char *from, BOOL overwrite)
{
    register char *out;
    register int shifted;
    char *save;

    shifted = _KJ_ROMAN;
    save = (char *) from;
    for (out = cvtbuf; *from;) {
	if (is_esc (*from)) {
	    if (is_so1 (from[1]) && is_so2 (from[2])) {
		shifted = _KJ_KANJI;
		from += 3;
	    } else if (is_si1 (from[1]) && is_si2 (from[2])) {
		shifted = _KJ_ROMAN;
		from += 3;
	    } else {			/* sequence error */
		goto normal;
	    }
	} else if (is_so (*from)) {
	    shifted = _KJ_KANA;		/* to KANA */
	    from++;
	} else if (is_si (*from)) {
	    shifted = _KJ_ROMAN;	/* to ROMAN */
	    from++;
	} else {
	normal:
	    switch (shifted) {
	    default:
	    case _KJ_ROMAN:
		*out++ = *from++;
		break;
	    case _KJ_KANJI:
		{
		    int code = jis2sjis ((int) from[0] & 0xff, (int) from[1] & 0xff);
		    *out++ = (code >> 8) & 0xff;
		    *out++ = code;
		    from += 2;
		}
		break;
	    case _KJ_KANA:
		*out++ = ((int) from[0]) + 0x80;
		break;
	    }
	}
    }
    *out = 0;
    if (overwrite) {
	strcpy (save, (char *) cvtbuf);
	return save;
    } else {
	return cvtbuf;
    }
}

/*******************************************************************
 Convert FROM contain SHIFT JIS codes to 7 bits JIS codes
 return converted buffer
********************************************************************/
static char *
sj_to_jis7 (const char *from, BOOL overwrite)
{
    register char *out;
    register int shifted;
    char *save;

    shifted = _KJ_ROMAN;
    save = (char *) from;
    for (out = cvtbuf; *from; ) {
	if (is_shift_jis (*from)) {
	    int code;
	    switch (shifted) {
	    case _KJ_KANA:
		*out++ = jis_si;	/* to ROMAN and through down */
	    case _KJ_ROMAN:		/* to KANJI */
		*out++ = jis_esc;
		*out++ = jis_so1;
		*out++ = jis_kso;
		shifted = _KJ_KANJI;
		break;
	    }
	    code = sjis2jis ((int) from[0] & 0xff, (int) from[1] & 0xff);
	    *out++ = (code >> 8) & 0xff;
	    *out++ = code;
	    from += 2;
	} else if (is_kana (from[0])) {
	    switch (shifted) {
	    case _KJ_KANJI:		/* to ROMAN */
		*out++ = jis_esc;
		*out++ = jis_si1;
		*out++ = jis_ksi;
	    case _KJ_ROMAN:		/* to KANA */
		*out++ = jis_so;
		shifted = _KJ_KANA;
		break;
	    }
	    *out++ = ((int) *from++) - 0x80;
	} else {
	    switch (shifted) {
	    case _KJ_KANA:
		*out++ = jis_si;	/* to ROMAN */
		shifted = _KJ_ROMAN;
		break;
	    case _KJ_KANJI:		/* to ROMAN */
		*out++ = jis_esc;
		*out++ = jis_si1;
		*out++ = jis_ksi;
		shifted = _KJ_ROMAN;
		break;
	    }
	    *out++ = *from++;
	}
    }
    switch (shifted) {
    case _KJ_KANA:
	*out++ = jis_si;		/* to ROMAN */
	break;
    case _KJ_KANJI:			/* to ROMAN */
	*out++ = jis_esc;
	*out++ = jis_si1;
	*out++ = jis_ksi;
	break;
    }
    *out = 0;
    if (overwrite) {
	strcpy (save, (char *) cvtbuf);
	return save;
    } else {
	return cvtbuf;
    }
}

/*******************************************************************
 Convert FROM contain 7 bits JIS(junet) codes to SHIFT JIS codes
 return converted buffer
********************************************************************/
static char *
junet_to_sj (const char *from, BOOL overwrite)
{
    register char *out;
    register int shifted;
    char *save;

    shifted = _KJ_ROMAN;
    save = (char *) from;
    for (out = cvtbuf; *from;) {
	if (is_esc (*from)) {
	    if (is_so1 (from[1]) && is_so2 (from[2])) {
		shifted = _KJ_KANJI;
		from += 3;
	    } else if (is_si1 (from[1]) && is_si2 (from[2])) {
		shifted = _KJ_ROMAN;
		from += 3;
	    } else if (is_juk1(from[1]) && is_juk2 (from[2])) {
		shifted = _KJ_KANA;
		from += 3;
	    } else {			/* sequence error */
		goto normal;
	    }
	} else {
	normal:
	    switch (shifted) {
	    default:
	    case _KJ_ROMAN:
		*out++ = *from++;
		break;
	    case _KJ_KANJI:
		{
		    int code = jis2sjis ((int) from[0] & 0xff, (int) from[1] & 0xff);
		    *out++ = (code >> 8) & 0xff;
		    *out++ = code;
		    from += 2;
		}
		break;
	    case _KJ_KANA:
		*out++ = ((int) from[0]) + 0x80;
		break;
	    }
	}
    }
    *out = 0;
    if (overwrite) {
	strcpy (save, (char *) cvtbuf);
	return save;
    } else {
	return cvtbuf;
    }
}

/*******************************************************************
 Convert FROM contain SHIFT JIS codes to 7 bits JIS(junet) codes
 return converted buffer
********************************************************************/
static char *
sj_to_junet (const char *from, BOOL overwrite)
{
    register char *out;
    register int shifted;
    char *save;

    shifted = _KJ_ROMAN;
    save = (char *) from;
    for (out = cvtbuf; *from; ) {
	if (is_shift_jis (*from)) {
	    int code;
	    switch (shifted) {
	    case _KJ_KANA:
	    case _KJ_ROMAN:		/* to KANJI */
		*out++ = jis_esc;
		*out++ = jis_so1;
		*out++ = jis_so2;
		shifted = _KJ_KANJI;
		break;
	    }
	    code = sjis2jis ((int) from[0] & 0xff, (int) from[1] & 0xff);
	    *out++ = (code >> 8) & 0xff;
	    *out++ = code;
	    from += 2;
	} else if (is_kana (from[0])) {
	    switch (shifted) {
	    case _KJ_KANJI:		/* to ROMAN */
	    case _KJ_ROMAN:		/* to KANA */
		*out++ = jis_esc;
		*out++ = junet_kana1;
		*out++ = junet_kana2;
		shifted = _KJ_KANA;
		break;
	    }
	    *out++ = ((int) *from++) - 0x80;
	} else {
	    switch (shifted) {
	    case _KJ_KANA:
	    case _KJ_KANJI:		/* to ROMAN */
		*out++ = jis_esc;
		*out++ = jis_si1;
		*out++ = jis_si2;
		shifted = _KJ_ROMAN;
		break;
	    }
	    *out++ = *from++;
	}
    }
    switch (shifted) {
    case _KJ_KANA:
    case _KJ_KANJI:			/* to ROMAN */
	*out++ = jis_esc;
	*out++ = jis_si1;
	*out++ = jis_si2;
	break;
    }
    *out = 0;
    if (overwrite) {
	strcpy (save, (char *) cvtbuf);
	return save;
    } else {
	return cvtbuf;
    }
}

/*******************************************************************
  HEX <-> SJIS
********************************************************************/
/* ":xx" -> a byte */
static char *
hex_to_sj (const char *from, BOOL overwrite)
{
    char *sp, *dp;
    
    sp = (char *) from;
    dp = cvtbuf;
    while (*sp) {
	if (*sp == hex_tag && isxdigit (sp[1]) && isxdigit (sp[2])) {
	    *dp++ = (hex2bin (sp[1])<<4) | (hex2bin (sp[2]));
	    sp += 3;
	} else
	    *dp++ = *sp++;
    }
    *dp = '\0';
    if (overwrite) {
	strcpy ((char *) from, (char *) cvtbuf);
	return (char *) from;
    } else {
	return cvtbuf;
    }
}
 
/*******************************************************************
  kanji/kana -> ":xx" 
********************************************************************/
static char *
sj_to_hex (const char *from, BOOL overwrite)
{
    unsigned char *sp, *dp;
    
    sp = (unsigned char*) from;
    dp = (unsigned char*) cvtbuf;
    while (*sp) {
	if (is_kana(*sp)) {
	    *dp++ = hex_tag;
	    *dp++ = bin2hex (((*sp)>>4)&0x0f);
	    *dp++ = bin2hex ((*sp)&0x0f);
	    sp++;
	} else if (is_shift_jis (*sp) && is_shift_jis2 (sp[1])) {
	    *dp++ = hex_tag;
	    *dp++ = bin2hex (((*sp)>>4)&0x0f);
	    *dp++ = bin2hex ((*sp)&0x0f);
	    sp++;
	    *dp++ = hex_tag;
	    *dp++ = bin2hex (((*sp)>>4)&0x0f);
	    *dp++ = bin2hex ((*sp)&0x0f);
	    sp++;
	} else
	    *dp++ = *sp++;
    }
    *dp = '\0';
    if (overwrite) {
	strcpy ((char *) from, (char *) cvtbuf);
	return (char *) from;
    } else {
	return cvtbuf;
    }
}

/*******************************************************************
  kanji/kana -> ":xx" 
********************************************************************/
static char *
sj_to_cap (const char *from, BOOL overwrite)
{
    unsigned char *sp, *dp;

    sp = (unsigned char*) from;
    dp = (unsigned char*) cvtbuf;
    while (*sp) {
	if (*sp >= 0x80) {
	    *dp++ = hex_tag;
	    *dp++ = bin2hex (((*sp)>>4)&0x0f);
	    *dp++ = bin2hex ((*sp)&0x0f);
	    sp++;
	} else {
	    *dp++ = *sp++;
	}
    }
    *dp = '\0';
    if (overwrite) {
	strcpy ((char *) from, (char *) cvtbuf);
	return (char *) from;
    } else {
	return cvtbuf;
    }
}

/*******************************************************************
 sj to sj
********************************************************************/
static char *
sj_to_sj (const char *from, BOOL overwrite)
{
    if (!overwrite) {
	strcpy (cvtbuf, (char *) from);
	return cvtbuf;
    } else {
	return (char *) from;
    }
}

/************************************************************************
 conversion:
 _dos_to_unix		_unix_to_dos
************************************************************************/

char* (*_dos_to_unix) (const char *str, BOOL overwrite) = sj_to_sj;
char* (*_unix_to_dos) (const char *str, BOOL overwrite) = sj_to_sj;

static int
setup_string_function (int codes)
{
    switch (codes) {
    default:
    case SJIS_CODE:
	_dos_to_unix = sj_to_sj;
	_unix_to_dos = sj_to_sj;

	break;
	
    case EUC_CODE:
	_dos_to_unix = sj_to_euc;
	_unix_to_dos = euc_to_sj;
	break;
	
    case JIS7_CODE:
	_dos_to_unix = sj_to_jis7;
	_unix_to_dos = jis7_to_sj;
	break;

    case JIS8_CODE:
	_dos_to_unix = sj_to_jis8;
	_unix_to_dos = jis8_to_sj;
	break;

    case JUNET_CODE:
	_dos_to_unix = sj_to_junet;
	_unix_to_dos = junet_to_sj;
	break;

    case HEX_CODE:
	_dos_to_unix = sj_to_hex;
	_unix_to_dos = hex_to_sj;
	break;

    case CAP_CODE:
	_dos_to_unix = sj_to_cap;
	_unix_to_dos = hex_to_sj;
	break;
    }
    return codes;
}

/*
 * Interpret coding system.
 */
int interpret_coding_system(char *str, int def)
{
    int codes = def;
    
    if (strequal (str, "sjis")) {
	codes = SJIS_CODE;
    } else if (strequal (str, "euc")) {
	codes = EUC_CODE;
    } else if (strequal (str, "cap")) {
	codes = CAP_CODE;
	hex_tag = HEXTAG;
    } else if (strequal (str, "hex")) {
	codes = HEX_CODE;
	hex_tag = HEXTAG;
    } else if (strncasecmp (str, "hex", 3)) {
	codes = HEX_CODE;
	hex_tag = (str[3] ? str[3] : HEXTAG);
    } else if (strequal (str, "j8bb")) {
	codes = JIS8_CODE;
	jis_kso = 'B';
	jis_ksi = 'B';
    } else if (strequal (str, "j8bj") || strequal (str, "jis8")) {
	codes = JIS8_CODE;
	jis_kso = 'B';
	jis_ksi = 'J';
    } else if (strequal (str, "j8bh")) {
	codes = JIS8_CODE;
	jis_kso = 'B';
	jis_ksi = 'H';
    } else if (strequal (str, "j8@b")) {
	codes = JIS8_CODE;
	jis_kso = '@';
	jis_ksi = 'B';
    } else if (strequal (str, "j8@j")) {
	codes = JIS8_CODE;
	jis_kso = '@';
	jis_ksi = 'J';
    } else if (strequal (str, "j8@h")) {
	codes = JIS8_CODE;
	jis_kso = '@';
	jis_ksi = 'H';
    } else if (strequal (str, "j7bb")) {
	codes = JIS7_CODE;
	jis_kso = 'B';
	jis_ksi = 'B';
    } else if (strequal (str, "j7bj") || strequal (str, "jis7")) {
	codes = JIS7_CODE;
	jis_kso = 'B';
	jis_ksi = 'J';
    } else if (strequal (str, "j7bh")) {
	codes = JIS7_CODE;
	jis_kso = 'B';
	jis_ksi = 'H';
    } else if (strequal (str, "j7@b")) {
	codes = JIS7_CODE;
	jis_kso = '@';
	jis_ksi = 'B';
    } else if (strequal (str, "j7@j")) {
	codes = JIS7_CODE;
	jis_kso = '@';
	jis_ksi = 'J';
    } else if (strequal (str, "j7@h")) {
	codes = JIS7_CODE;
	jis_kso = '@';
	jis_ksi = 'H';
    } else if (strequal (str, "jubb")) {
	codes = JUNET_CODE;
	jis_kso = 'B';
	jis_ksi = 'B';
    } else if (strequal (str, "jubj") || strequal (str, "junet")) {
	codes = JUNET_CODE;
	jis_kso = 'B';
	jis_ksi = 'J';
    } else if (strequal (str, "jubh")) {
	codes = JUNET_CODE;
	jis_kso = 'B';
	jis_ksi = 'H';
    } else if (strequal (str, "ju@b")) {
	codes = JUNET_CODE;
	jis_kso = '@';
	jis_ksi = 'B';
    } else if (strequal (str, "ju@j")) {
	codes = JUNET_CODE;
	jis_kso = '@';
	jis_ksi = 'J';
    } else if (strequal (str, "ju@h")) {
	codes = JUNET_CODE;
	jis_kso = '@';
	jis_ksi = 'H';
    }	
    return setup_string_function (codes);
}
#else 
 int kanji_dummy_procedure(void)
{return 0;}
#endif /* KANJI */
