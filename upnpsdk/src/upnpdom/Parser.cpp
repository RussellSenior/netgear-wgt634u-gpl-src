///////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2000 Intel Corporation
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of source code must retain the above copyright notice,
// this list of conditions and the following disclaimer.
// * Redistributions in binary form must reproduce the above copyright notice,
// this list of conditions and the following disclaimer in the documentation
// and/or other materials provided with the distribution.
// * Neither name of Intel Corporation nor the names of its contributors
// may be used to endorse or promote products derived from this software
// without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL INTEL OR
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
// OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
///////////////////////////////////////////////////////////////////////////
//	$Revision: 1.1.1.6 $
//	$Date: 2001/06/15 00:22:17 $

#include "../../inc/tools/config.h"
#if EXCLUDE_DOM == 0
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include "../../inc/upnpdom/DOMException.h"
#include "../../inc/upnpdom/Parser.h"

#ifdef _WIN32
#define strncasecmp strnicmp
#endif


static const char *WHITESPACE	= "\n\t\r\f ";
static const char *LESSTHAN		= "<";
static const char *GREATERTHAN	= ">";
static const char *SLASH		= "/";
static const char *EQUALS		= "=";
static const char *QUOTE		= "\"";
static const char *COMPLETETAG	= "/>";
static const char *ENDTAG		= "</";
static const char *BEGIN_COMMENT= "<!--";
static const char *END_COMMENT  = "-->";
static const char *BEGIN_PROCESSING="<?";
static const char *END_PROCESSING="?>";
static const char *BEGIN_DOCTYPE= "<!";
static const char *DEC_NUMBERS	= "0123456789";
static const char *HEX_NUMBERS	= "0123456789ABCDEFabcdef";

struct char_info_t
{
    unsigned short l, h;
};

typedef char utf8char[8];

#define N_TBL_SIZE 207
static char_info_t n_tbl[N_TBL_SIZE] = {
    {0x003A,0x003A}, {0x0041,0x005A}, {0x005F,0x005F}, {0x0061,0x007A}, 
    {0x00C0,0x00D6}, {0x00D8,0x00F6}, {0x00F8,0x00FF}, {0x0100,0x0131}, 
    {0x0134,0x013E}, {0x0141,0x0148}, {0x014A,0x017E}, {0x0180,0x01C3}, 
    {0x01CD,0x01F0}, {0x01F4,0x01F5}, {0x01FA,0x0217}, {0x0250,0x02A8}, 
    {0x02BB,0x02C1}, {0x0386,0x0386}, {0x0388,0x038A}, {0x038C,0x038C}, 
    {0x038E,0x03A1}, {0x03A3,0x03CE}, {0x03D0,0x03D6}, {0x03DA,0x03DA}, 
    {0x03DC,0x03DC}, {0x03DE,0x03DE}, {0x03E0,0x03E0}, {0x03E2,0x03F3}, 
    {0x0401,0x040C}, {0x040E,0x044F}, {0x0451,0x045C}, {0x045E,0x0481}, 
    {0x0490,0x04C4}, {0x04C7,0x04C8}, {0x04CB,0x04CC}, {0x04D0,0x04EB}, 
    {0x04EE,0x04F5}, {0x04F8,0x04F9}, {0x0531,0x0556}, {0x0559,0x0559}, 
    {0x0561,0x0586}, {0x05D0,0x05EA}, {0x05F0,0x05F2}, {0x0621,0x063A}, 
    {0x0641,0x064A}, {0x0671,0x06B7}, {0x06BA,0x06BE}, {0x06C0,0x06CE}, 
    {0x06D0,0x06D3}, {0x06D5,0x06D5}, {0x06E5,0x06E6}, {0x0905,0x0939}, 
    {0x093D,0x093D}, {0x0958,0x0961}, {0x0985,0x098C}, {0x098F,0x0990}, 
    {0x0993,0x09A8}, {0x09AA,0x09B0}, {0x09B2,0x09B2}, {0x09B6,0x09B9}, 
    {0x09DC,0x09DD}, {0x09DF,0x09E1}, {0x09F0,0x09F1}, {0x0A05,0x0A0A}, 
    {0x0A0F,0x0A10}, {0x0A13,0x0A28}, {0x0A2A,0x0A30}, {0x0A32,0x0A33}, 
    {0x0A35,0x0A36}, {0x0A38,0x0A39}, {0x0A59,0x0A5C}, {0x0A5E,0x0A5E}, 
    {0x0A72,0x0A74}, {0x0A85,0x0A8B}, {0x0A8D,0x0A8D}, {0x0A8F,0x0A91}, 
    {0x0A93,0x0AA8}, {0x0AAA,0x0AB0}, {0x0AB2,0x0AB3}, {0x0AB5,0x0AB9}, 
    {0x0ABD,0x0ABD}, {0x0AE0,0x0AE0}, {0x0B05,0x0B0C}, {0x0B0F,0x0B10}, 
    {0x0B13,0x0B28}, {0x0B2A,0x0B30}, {0x0B32,0x0B33}, {0x0B36,0x0B39}, 
    {0x0B3D,0x0B3D}, {0x0B5C,0x0B5D}, {0x0B5F,0x0B61}, {0x0B85,0x0B8A}, 
    {0x0B8E,0x0B90}, {0x0B92,0x0B95}, {0x0B99,0x0B9A}, {0x0B9C,0x0B9C}, 
    {0x0B9E,0x0B9F}, {0x0BA3,0x0BA4}, {0x0BA8,0x0BAA}, {0x0BAE,0x0BB5}, 
    {0x0BB7,0x0BB9}, {0x0C05,0x0C0C}, {0x0C0E,0x0C10}, {0x0C12,0x0C28}, 
    {0x0C2A,0x0C33}, {0x0C35,0x0C39}, {0x0C60,0x0C61}, {0x0C85,0x0C8C}, 
    {0x0C8E,0x0C90}, {0x0C92,0x0CA8}, {0x0CAA,0x0CB3}, {0x0CB5,0x0CB9}, 
    {0x0CDE,0x0CDE}, {0x0CE0,0x0CE1}, {0x0D05,0x0D0C}, {0x0D0E,0x0D10}, 
    {0x0D12,0x0D28}, {0x0D2A,0x0D39}, {0x0D60,0x0D61}, {0x0E01,0x0E2E}, 
    {0x0E30,0x0E30}, {0x0E32,0x0E33}, {0x0E40,0x0E45}, {0x0E81,0x0E82}, 
    {0x0E84,0x0E84}, {0x0E87,0x0E88}, {0x0E8A,0x0E8A}, {0x0E8D,0x0E8D}, 
    {0x0E94,0x0E97}, {0x0E99,0x0E9F}, {0x0EA1,0x0EA3}, {0x0EA5,0x0EA5}, 
    {0x0EA7,0x0EA7}, {0x0EAA,0x0EAB}, {0x0EAD,0x0EAE}, {0x0EB0,0x0EB0}, 
    {0x0EB2,0x0EB3}, {0x0EBD,0x0EBD}, {0x0EC0,0x0EC4}, {0x0F40,0x0F47}, 
    {0x0F49,0x0F69}, {0x10A0,0x10C5}, {0x10D0,0x10F6}, {0x1100,0x1100}, 
    {0x1102,0x1103}, {0x1105,0x1107}, {0x1109,0x1109}, {0x110B,0x110C}, 
    {0x110E,0x1112}, {0x113C,0x113C}, {0x113E,0x113E}, {0x1140,0x1140}, 
    {0x114C,0x114C}, {0x114E,0x114E}, {0x1150,0x1150}, {0x1154,0x1155}, 
    {0x1159,0x1159}, {0x115F,0x1161}, {0x1163,0x1163}, {0x1165,0x1165}, 
    {0x1167,0x1167}, {0x1169,0x1169}, {0x116D,0x116E}, {0x1172,0x1173}, 
    {0x1175,0x1175}, {0x119E,0x119E}, {0x11A8,0x11A8}, {0x11AB,0x11AB}, 
    {0x11AE,0x11AF}, {0x11B7,0x11B8}, {0x11BA,0x11BA}, {0x11BC,0x11C2}, 
    {0x11EB,0x11EB}, {0x11F0,0x11F0}, {0x11F9,0x11F9}, {0x1E00,0x1E9B}, 
    {0x1EA0,0x1EF9}, {0x1F00,0x1F15}, {0x1F18,0x1F1D}, {0x1F20,0x1F45}, 
    {0x1F48,0x1F4D}, {0x1F50,0x1F57}, {0x1F59,0x1F59}, {0x1F5B,0x1F5B}, 
    {0x1F5D,0x1F5D}, {0x1F5F,0x1F7D}, {0x1F80,0x1FB4}, {0x1FB6,0x1FBC}, 
    {0x1FBE,0x1FBE}, {0x1FC2,0x1FC4}, {0x1FC6,0x1FCC}, {0x1FD0,0x1FD3}, 
    {0x1FD6,0x1FDB}, {0x1FE0,0x1FEC}, {0x1FF2,0x1FF4}, {0x1FF6,0x1FFC}, 
    {0x2126,0x2126}, {0x212A,0x212B}, {0x212E,0x212E}, {0x2180,0x2182}, 
    {0x3007,0x3007}, {0x3021,0x3029}, {0x3041,0x3094}, {0x30A1,0x30FA}, 
    {0x3105,0x312C}, {0x4E00,0x9FA5}, {0xAC00,0xD7A3} 
};

#define NCH_TBL_SIZE 123
static char_info_t nch_tbl[NCH_TBL_SIZE] = {
    {0x002D,0x002D}, {0x002E,0x002E}, {0x0030,0x0039}, {0x00B7,0x00B7}, 
    {0x02D0,0x02D0}, {0x02D1,0x02D1}, {0x0300,0x0345}, {0x0360,0x0361}, 
    {0x0387,0x0387}, {0x0483,0x0486}, {0x0591,0x05A1}, {0x05A3,0x05B9}, 
    {0x05BB,0x05BD}, {0x05BF,0x05BF}, {0x05C1,0x05C2}, {0x05C4,0x05C4}, 
    {0x0640,0x0640}, {0x064B,0x0652}, {0x0660,0x0669}, {0x0670,0x0670}, 
    {0x06D6,0x06DC}, {0x06DD,0x06DF}, {0x06E0,0x06E4}, {0x06E7,0x06E8}, 
    {0x06EA,0x06ED}, {0x06F0,0x06F9}, {0x0901,0x0903}, {0x093C,0x093C}, 
    {0x093E,0x094C}, {0x094D,0x094D}, {0x0951,0x0954}, {0x0962,0x0963}, 
    {0x0966,0x096F}, {0x0981,0x0983}, {0x09BC,0x09BC}, {0x09BE,0x09BE}, 
    {0x09BF,0x09BF}, {0x09C0,0x09C4}, {0x09C7,0x09C8}, {0x09CB,0x09CD}, 
    {0x09D7,0x09D7}, {0x09E2,0x09E3}, {0x09E6,0x09EF}, {0x0A02,0x0A02}, 
    {0x0A3C,0x0A3C}, {0x0A3E,0x0A3E}, {0x0A3F,0x0A3F}, {0x0A40,0x0A42}, 
    {0x0A47,0x0A48}, {0x0A4B,0x0A4D}, {0x0A66,0x0A6F}, {0x0A70,0x0A71}, 
    {0x0A81,0x0A83}, {0x0ABC,0x0ABC}, {0x0ABE,0x0AC5}, {0x0AC7,0x0AC9}, 
    {0x0ACB,0x0ACD}, {0x0AE6,0x0AEF}, {0x0B01,0x0B03}, {0x0B3C,0x0B3C}, 
    {0x0B3E,0x0B43}, {0x0B47,0x0B48}, {0x0B4B,0x0B4D}, {0x0B56,0x0B57}, 
    {0x0B66,0x0B6F}, {0x0B82,0x0B83}, {0x0BBE,0x0BC2}, {0x0BC6,0x0BC8}, 
    {0x0BCA,0x0BCD}, {0x0BD7,0x0BD7}, {0x0BE7,0x0BEF}, {0x0C01,0x0C03}, 
    {0x0C3E,0x0C44}, {0x0C46,0x0C48}, {0x0C4A,0x0C4D}, {0x0C55,0x0C56}, 
    {0x0C66,0x0C6F}, {0x0C82,0x0C83}, {0x0CBE,0x0CC4}, {0x0CC6,0x0CC8}, 
    {0x0CCA,0x0CCD}, {0x0CD5,0x0CD6}, {0x0CE6,0x0CEF}, {0x0D02,0x0D03}, 
    {0x0D3E,0x0D43}, {0x0D46,0x0D48}, {0x0D4A,0x0D4D}, {0x0D57,0x0D57}, 
    {0x0D66,0x0D6F}, {0x0E31,0x0E31}, {0x0E34,0x0E3A}, {0x0E46,0x0E46}, 
    {0x0E47,0x0E4E}, {0x0E50,0x0E59}, {0x0EB1,0x0EB1}, {0x0EB4,0x0EB9}, 
    {0x0EBB,0x0EBC}, {0x0EC6,0x0EC6}, {0x0EC8,0x0ECD}, {0x0ED0,0x0ED9}, 
    {0x0F18,0x0F19}, {0x0F20,0x0F29}, {0x0F35,0x0F35}, {0x0F37,0x0F37}, 
    {0x0F39,0x0F39}, {0x0F3E,0x0F3E}, {0x0F3F,0x0F3F}, {0x0F71,0x0F84}, 
    {0x0F86,0x0F8B}, {0x0F90,0x0F95}, {0x0F97,0x0F97}, {0x0F99,0x0FAD}, 
    {0x0FB1,0x0FB7}, {0x0FB9,0x0FB9}, {0x20D0,0x20DC}, {0x20E1,0x20E1}, 
    {0x3005,0x3005}, {0x302A,0x302F}, {0x3031,0x3035}, {0x3099,0x3099}, 
    {0x309A,0x309A}, {0x309D,0x309E}, {0x30FC,0x30FE}
};


bool intbl(int c, char_info_t*tbl, int sz )
{
  int t=0, b=sz-1, m;
  while (t<=b) {
      m=(t+b)/2;
      if (c<tbl[m].l) 
        b=m-1;      
      else if (c>tbl[m].h)
          t=m+1;
      else return true;
  }
  return false;
}

bool isxmlch(int c)
{
  return((c>=0x20&&c<=0xD7FF)||c==0x9||c==0xA||c==0xD||
    (c>=0xE000&&c<=0xFFFD)||(c>=0x10000&&c<=0x10FFFF));
}

bool isnamech(int c, bool scnd_ch)
{
  if (intbl(c, n_tbl, N_TBL_SIZE))
      return true; 
    if (scnd_ch && intbl(c, nch_tbl, NCH_TBL_SIZE))
        return true;
    return false; 
}

// returns char len on success, -1 on error
int toutf8(int c, utf8char s)
{
    if (c<0) return -1;
       if (c<=127) { s[0]=c; s[1]=0; return 1; }
    else if (c<=0x07FF) { s[0]=0xC0|(c>>6); s[1]=0x80|(c&0x3f); s[2]=0; 
        return 2; }
   else if (c<=0xFFFF) { s[0]=0xE0|(c>>12); s[1]=0x80|((c>>6)&0x3f);
    s[2]=0x80|(c&0x3f); s[3]=0; return 3; }
   else if (c<=0x1FFFFF) { s[0]=0xF0|(c>>18); s[1]=0x80|((c>>12)&0x3f);
    s[2]=0x80|((c>>6)&0x3f); s[3]=0x80|(c&0x3f); s[4]=0; return 4; }
   else if (c<=0x3FFFFFF) { s[0]=0xF8|(c>>24); s[1]=0x80|((c>>18)&0x3f);
     s[2]=0x80|((c>>12)&0x3f); s[3]=0x80|((c>>6)&0x3f); s[4]=0x80|(c&0x3f);
    s[5]=0; return 5; }
   else { s[0]=0xFC|(c>>30); s[1]=0x80|((c>>24)&0x3f);
    s[2]=0x80|((c>>18)&0x3f); s[3]=0x80|((c>>12)&0x3f);
     s[4]=0x80|((c>>6)&0x3f); s[5]=0x80|(c&0x3f); s[6]=0; return 6;}
}

// returns -1 on error; else valid char
int toint(char *ss, int *len)
{
    unsigned char *s=(unsigned char*)ss;
    int c = *s;
    if (c<=127) { *len=1; return c; }
    if ((c&0xE0)==0xC0 && (s[1]&0xc0)==0x80) { *len=2;
        return (((c&0x1f)<<6)|(s[1]&0x3f)); }
     else if ((c&0xF0)==0xE0 && (s[1]&0xc0)==0x80 && (s[2]&0xc0)==0x80) { *len=3;
       return (((c&0xf)<<12)|((s[1]&0x3f)<<6)|(s[2]&0x3f)); }
     else if ((c&0xf8)==0xf0 && (s[1]&0xc0)==0x80 && (s[2]&0xc0)==0x80 && (s[3]&0xc0)==0x80){
         *len=4; return (((c&0x7)<<18)|((s[1]&0x3f)<<12)|
             ((s[2]&0x3f)<<6)|(s[3]&0x3f)); }
     else if ((c&0xfc)==0xf8 && (s[1]&0xc0)==0x80 && (s[2]&0xc0)==0x80 && (s[3]&0xc0)==0x80 &&
         (s[4]&0xc0)==0x80) { *len=5;
        return (((c&0x3)<<24)|((s[1]&0x3f)<<18)|((s[2]&0x3f)<<12)|
            ((s[3]&0x3f)<<6)|(s[4]&0x3f)); }
     else if ((c&0xfe)==0xfc && (s[1]&0xc0)==0x80 && (s[2]&0xc0)==0x80 && (s[3]&0xc0)==0x80 &&
         (s[4]&0xc0)==0x80 && (s[5]&0xc0)==0x80) { *len=6;
        return (((c&0x1)<<30)|((s[1]&0x3f)<<24)|((s[2]&0x3f)<<18)|
            ((s[3]&0x3f)<<12)|((s[4]&0x3f)<<6)|(s[5]&0x3f)); }
   else { *len = 0; return -1; }
}


//Character Match
//Returns true if character cUnknown is in the match set.
//Returns false otherwise.
bool char_match (char cUnknown, const char * strMatchSet)
{
	int iMatchLen = 0;		// Length of the match string
	int iIndex = 0;			// Index through match set looking for match with cUnknown
	bool bReturn = false;	// Return value: true if cUnknown is in strMatchSet

	iMatchLen = strlen (strMatchSet);

	if ((strMatchSet == NULL) || (iMatchLen <= 0))
		return false;

	while ((iIndex < iMatchLen) && (!bReturn))
	{
		if (*(strMatchSet + iIndex) == cUnknown)
			bReturn = true;

		iIndex++;
	}

	return bReturn;
}

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

Parser::Parser(char *ParseStr)
{
	if(ParseStr == NULL)
		throw DOMException(DOMException::FATAL_ERROR_DURING_PARSING);
	if(*ParseStr == '\0')
		throw DOMException(DOMException::FATAL_ERROR_DURING_PARSING);
	ParseBuff = new char[strlen(ParseStr)+1];
	if(!ParseBuff)
	   	{DBGONLY(UpnpPrintf(UPNP_CRITICAL,DOM,__FILE__,__LINE__,"Insuffecient memory\n");)}
	strcpy(ParseBuff, ParseStr);
	CurrPtr=ParseBuff;
	TagVal=false;
	TagName=false;
	inAttrib=false;
	attrFound = true;
	atLeastOneAttrFound=false;

    membuffer_init(&tokBuf);
    tokBuf.size_inc = 50;

    membuffer_init(&lastElem);
    lastElem.size_inc = 50;
    setLastElem("");
}

Parser::~Parser()
{
    membuffer_destroy(&tokBuf);
    membuffer_destroy(&lastElem);
	delete ParseBuff;
}

char* Parser::getLastElem() {
    if (lastElem.buf) return lastElem.buf;
    return "";
}

int Parser::getLastElemLength() {
    return lastElem.length;
}

int Parser::setLastElem(const char*s) {
    if (*s == '\0') {
        membuffer_assign(&lastElem,NULL,0);
	return 0;
    }
    if (membuffer_assign_str(&lastElem,s)!=0) return -1;
    return 0;
}

char* Parser::getTokBuf() {
    if (tokBuf.buf) return tokBuf.buf;
    else return "";
}

int Parser::getTokBufLength() {
    return tokBuf.length;
}

int Parser::clearTokBuf() {
    membuffer_assign(&tokBuf,NULL,0);
    return 0;
}

int Parser::appendTokBuf(const char *s) {
    if (*s == '\0') return 0;
    if (membuffer_append_str(&tokBuf,s)!=0) return -1;
    return 0;
}

int Parser::appendTokBuf(char c) {
    if (membuffer_append(&tokBuf,&c,1)!=0) return -1;
    return 0;
}

void Parser::IgnoreWhiteSpaces()
{
	while(char_match (*CurrPtr, WHITESPACE))
		getNextToken();
}

//Rewinds Current Ptr by n bytes
void Parser::rewindCurrentPtr(int n)
{
	CurrPtr=CurrPtr - n;
}

//Finds the Next Match
//Finds the next character in strSearch that contains strMatch.
char * Parser::findNextMatch (char *strSearch, const char *strMatch)
{
	char *strIndex;		// Current character match position

	if ((strSearch == NULL) || (strMatch == NULL))
		return NULL;

	strIndex = strSearch;

	while (!(char_match (*strIndex, strMatch)) && (*strIndex != '\0'))
		strIndex++;

	return strIndex;
}

int get_char(char *src, int*clen)
{
    char *pnum;
    int count;
    int sum;
    char c;

    *clen=0;
    if (!src || !clen)
        return -1; 
    if (*src!='&'){
        if (*src>0 &&isxmlch(*src)){
            *clen=1; return *src; }
        int i, len;
        i=toint(src,&len);
        if (!isxmlch(i))return -1;
        *clen=len;
        return i;
    }
    if (!strncasecmp(src,QUOT,strlen(QUOT))){
        *clen=strlen(QUOT);  return '"'; }
    if (!strncasecmp(src,LT,strlen(LT))){
        *clen=strlen(LT); return '<';  }
    if (!strncasecmp(src,GT,strlen(GT))){
        *clen=strlen(GT); return '>'; }
    if (!strncasecmp(src,APOS,strlen(APOS))){
        *clen=strlen(APOS); return '\''; }
    if (!strncasecmp(src,AMP,strlen(AMP))){
        *clen=strlen(AMP); return '&'; }
    // Read in escape characters of type &#xnn where nn is a hexadecimal value
    if (!strncasecmp(src,ESC_HEX,strlen(ESC_HEX))){
        count=0; pnum=src+strlen(ESC_HEX); sum=0;
        while (char_match(pnum[count],HEX_NUMBERS)){
            c=pnum[count];
            if (c<='9') sum = sum * 16+(c-'0');
            else if(c<='F')
                sum = sum*16+(c-'A'+10);
            else
                sum = sum*16+(c-'a'+10);
            count++;
        }
        if (count<=0||pnum[count]!=';'||!isxmlch(sum)) return -1;
        *clen=strlen(ESC_HEX)+count+1;
        return sum;
    }
    // Read in escape characters of type &#nn where nn is a decimal value
    if (!strncasecmp(src,ESC_DEC,strlen(ESC_DEC))){
        count=0; pnum=src+strlen(ESC_DEC); sum=0;
        while (char_match(pnum[count],DEC_NUMBERS))
            sum=sum*10+(pnum[count++]-'0');
        if (count<=0||pnum[count]!=';'||!isxmlch(sum)) return -1;
        *clen=strlen(ESC_DEC)+count+1;
        return sum;
    }

    return -1;

}

bool Parser::copy_token(char *src, int len)
{
    int i,c, cl;
    char *psrc, *pend;
    utf8char uch;
    
    if (!src||len <= 0) return false;
    psrc=src;
    pend=src+len;
    while (psrc<pend){
        
        if ((c=get_char(psrc, &cl))<=0) 
        { 
          //*dest=0; 
          //clearTokBuf();
          return false;
        }
        if (cl==1){
            //*dest++=c;
            appendTokBuf(c);
            psrc++;
            }
        else {
            //i=toutf8(c, dest);
            i=toutf8(c, uch);
            //if (i<0) {*dest=0; return false;}
            if (i<0) { return false;}
            //dest+=i;
            appendTokBuf(uch);
            psrc+=cl;
            }
    }
    //*dest=0;
    if (psrc>pend) return false;
    return true; // success
}


// returns true on success; false on error
bool copy_token(char *dest, char *src, int len)
{
    int i,c, cl;
    char *psrc, *pend;
    if (!dest||!src||len <= 0) return false;
    psrc=src;
    pend=src+len;
    while (psrc<pend){
        
        if ((c=get_char(psrc, &cl))<=0) 
        { *dest=0; return false;}
        if (cl==1){
            *dest++=c;
            psrc++;
            }
        else {
            i=toutf8(c, dest);
            if (i<0) {*dest=0; return false;}
            dest+=i;
            psrc+=cl;
            }
    }
    *dest=0;
    if (psrc>pend) return false;
    return true; // success
}

//Skip String
//Skips all characters in the fragment string that are contained within
//strSkipChars until some other character is found.  Useful for skipping
//over whitespace.

long Parser::skipString (char **pstrFragment, const char *strSkipChars)
{
	if (!pstrFragment || !strSkipChars)
		return -1;

	while ((**pstrFragment != '\0') && (char_match (**pstrFragment, strSkipChars)))
	{
		(*pstrFragment)++;
	}

	return 0; //success
}

//Skip Until String
//Skips all characters in the string until it finds the skip key.
//Then it skips the skip key and returns.
long Parser::skipUntilString (char **pstrSource, const char *strSkipKey)
{
	if (!pstrSource || !strSkipKey)
		return -1;

	while (**pstrSource && strncmp (*pstrSource, strSkipKey, strlen(strSkipKey)))
		(*pstrSource)++;

	*pstrSource = *pstrSource + strlen (strSkipKey);

	return 0; //success
}


//This will return the string of the next token in the TokenBuff
int Parser::getNextToken()
{
	int TokenLength=0;
    int temp, tlen;

    clearTokBuf();

	// Check for white space
	if(*CurrPtr=='\0')
	{
		//TokenBuff[0]='\0';
		return 0;
	}

	// Attribute value logic must come first, since all text untokenized until end-quote
	if (inAttrib && (!char_match (*CurrPtr, QUOTE)))
	{
		char *strEndQuote = findNextMatch (CurrPtr, QUOTE);
		if (strEndQuote == NULL)
		{
			TokenLength = 1;
			//*TokenBuff = '\0';
			 // return a single space for whitespace
			//if (!copy_token (TokenBuff, CurrPtr, TokenLength))
              if (!copy_token (CurrPtr, TokenLength))
                return 1;
			return 0; // serious problem - no matching end-quote found for attribute
		}

		TokenLength = strEndQuote - CurrPtr; // BUGBUG: conversion issue if using more than simple strings
		//if (!copy_token (TokenBuff, CurrPtr, TokenLength))
        if (!copy_token (CurrPtr, TokenLength))
            return 1;
		CurrPtr = CurrPtr+TokenLength;
		return 0; // must return now, so it doesn't go into name processing
	}
	
	if (char_match (*CurrPtr, WHITESPACE))
	{
		TokenLength = 1;
		//if (!copy_token (TokenBuff, " ", TokenLength)) // return a single space for whitespace
        if (!copy_token (" ", TokenLength)) // return a single space for whitespace
            return 1;
		CurrPtr = CurrPtr+TokenLength;
		return 0;
	}
	// Skip <? .. ?> , <! .. >, <!-- .. -->
	while (!strncmp (CurrPtr, BEGIN_COMMENT, strlen(BEGIN_COMMENT))  // <!--
			|| !strncmp (CurrPtr, BEGIN_PROCESSING, strlen(BEGIN_PROCESSING)) // <?
			|| !strncmp (CurrPtr, BEGIN_DOCTYPE, strlen(BEGIN_DOCTYPE)))  // <!
	{
		if (!strncmp (CurrPtr, BEGIN_COMMENT, strlen(BEGIN_COMMENT)))
			skipUntilString (&CurrPtr, END_COMMENT);
		else if (!strncmp (CurrPtr, BEGIN_PROCESSING, strlen(BEGIN_PROCESSING)))
			skipUntilString (&CurrPtr, END_PROCESSING);
		else
			skipUntilString (&CurrPtr, GREATERTHAN);			

		skipString (&CurrPtr, WHITESPACE);
		TagVal=false;
	}

	// Check for start tags
	if (char_match (*CurrPtr, LESSTHAN))
	{
        temp = toint(CurrPtr+1, &tlen);
        if (temp == '/')
            TokenLength = 2; // token is '</' end tag
        else if(isnamech(temp,false))
            TokenLength=1; // Begin tag found, so return '<' token
        else{
            //strcpy(TokenBuff,"\0");

            return 1;   //error
            }
        TagVal=false;
	}

		// Check for opening/closing attribute value quotation mark
	if (char_match (*CurrPtr, QUOTE) && !TagVal)
	{
		// Quote found, so return it as token
		TokenLength = strlen(QUOTE);
	}

	// Check for '=' token
	if (char_match (*CurrPtr, EQUALS) && !TagVal)
	{
		// Equals found, so return it as a token
		TokenLength = strlen(EQUALS);
	}
	// Check for '/>' token
	if (char_match (*CurrPtr, SLASH))
	{
		if (char_match (*(CurrPtr + 1), GREATERTHAN))
		{
			// token '/>' found
			TokenLength = 2;
			TagVal=true;
		}
		//Content may begin with a /
    	else if (TagVal)
    	{
    		TagVal=false;
    		CurrPtr=SavePtr+1;//SavePtr whould not have have already moved.
    		char *pEndContent = CurrPtr;

    		// Read content until a < is found that is not a comment <!--
    		bool bReadContent = true;

    		while (bReadContent)
    		{
    			while (!char_match (*pEndContent, LESSTHAN) && *pEndContent)
    				pEndContent++;

    			if (!strncmp (pEndContent, BEGIN_COMMENT, strlen (BEGIN_COMMENT)))
    				skipUntilString (&pEndContent, END_COMMENT);
    			else
    				bReadContent = false;

    			if (!(*pEndContent))
    				bReadContent = false;
    		}
    		TokenLength = pEndContent - CurrPtr;
    	}
	}
	// Check for '>' token
	else if (char_match (*CurrPtr, GREATERTHAN))
	{
		// Equals found, so return it as a token
		TokenLength = strlen(GREATERTHAN);
		SavePtr=CurrPtr; // Saving this ptr for not ignoring the leading and trailing spaces.
		TagVal=true;
	}

	// Check for Content e.g.  <tag>content</tag>
	else if (TagVal)
	{
		TagVal=false;
		CurrPtr=SavePtr+1;//SavePtr whould not have have already moved.
		char *pEndContent = CurrPtr;

		// Read content until a < is found that is not a comment <!--
		bool bReadContent = true;

		while (bReadContent)
		{
			while (!char_match (*pEndContent, LESSTHAN) && *pEndContent)
				pEndContent++;

			if (!strncmp (pEndContent, BEGIN_COMMENT, strlen (BEGIN_COMMENT)))
				skipUntilString (&pEndContent, END_COMMENT);
			else
				bReadContent = false;

			if (!(*pEndContent))
				bReadContent = false;
		}
		TokenLength = pEndContent - CurrPtr;
	}

    // Check for name tokens
    else if (isnamech(toint(CurrPtr,&tlen),false)){
        // Name found, so find out how long it is
        int iIndex=tlen;
        while (isnamech(toint(CurrPtr+iIndex,&tlen),true))
            iIndex+=tlen;
        TokenLength=iIndex;
    };

	// Copy the token to the return string
    if (TokenLength > 0){
		//if (!copy_token (TokenBuff, CurrPtr, TokenLength))
        if (!copy_token (CurrPtr, TokenLength))
            return 1;
    }
    else if (*CurrPtr == '\0') {
        TokenLength = 0;
        clearTokBuf();
        return 0;
    }
	else
	{
		// return the unrecognized token for error information
		TokenLength = 1;
        //copy_token (TokenBuff, CurrPtr, TokenLength);
        appendTokBuf(*CurrPtr);
        return 1;
	}
	CurrPtr = CurrPtr+TokenLength;
	return 0;
}

//Will return a handle to the tree structure
//The tree structure indicates to which parent the node belongs.
int Parser::getNextNode(NODE_TYPE &NodeType, char **NodeName, char **NodeValue, bool &IsEnd,  bool IgnoreWhiteSpace)
{
	while(*CurrPtr!='\0')
	{
		if(IgnoreWhiteSpace)
			IgnoreWhiteSpaces();
		if(getNextToken()!=0)
		{
			*NodeValue=NULL;
			*NodeName=NULL;
			NodeType=INVALID_NODE;
			IsEnd=false;
			return 1;
		}
		//if(!strcmp(TokenBuff, LESSTHAN))
        if(!strcmp(getTokBuf(), LESSTHAN))
		{
			if(IgnoreWhiteSpace)
				IgnoreWhiteSpaces();
			TagName=true;
			if(getNextToken()!=0)
			{
				*NodeValue=NULL;
				*NodeName=NULL;
				NodeType=INVALID_NODE;
				IsEnd=false;
				return 1;
			}
			TagName=false;
			//if(TokenBuff==NULL)
			//	throw DOMException(DOMException::FATAL_ERROR_DURING_PARSING);
			//if(*TokenBuff=='\0')
			//	throw DOMException(DOMException::FATAL_ERROR_DURING_PARSING);
            if (getTokBufLength() == 0)
                throw DOMException(DOMException::FATAL_ERROR_DURING_PARSING);

			//*NodeName=(char *)malloc(strlen(TokenBuff)+1);
            *NodeName=(char *)malloc(getTokBufLength()+1);
			//strcpy(*NodeName,TokenBuff);
            strcpy(*NodeName,getTokBuf());
			//strcpy(LastElement,TokenBuff);
            setLastElem(getTokBuf());
			*NodeValue=NULL;
			NodeType=ELEMENT_NODE;
			attrFound=true;
			atLeastOneAttrFound=false;
			IsEnd=false;
		}
		//else if(!strcmp(TokenBuff, GREATERTHAN))
        else if(!strcmp(getTokBuf(), GREATERTHAN))
		{
			attrFound=false;
			if(atLeastOneAttrFound)//forget the greater than
			{
				atLeastOneAttrFound=false;
				continue;
			}
			else
				return 0;
		}
		//else if(!strcmp(TokenBuff, ENDTAG))
        else if(!strcmp(getTokBuf(), ENDTAG))
		{
			if(IgnoreWhiteSpace)
				IgnoreWhiteSpaces();
			if(getNextToken()!=0)
			{
				*NodeValue=NULL;
				*NodeName=NULL;
				NodeType=INVALID_NODE;
				IsEnd=false;
				return 1;
			}
			//if(TokenBuff==NULL)
			//	throw DOMException(DOMException::FATAL_ERROR_DURING_PARSING);
			//if(*TokenBuff=='\0')
			//	throw DOMException(DOMException::FATAL_ERROR_DURING_PARSING);
            if (getTokBufLength() == 0)
                throw DOMException(DOMException::FATAL_ERROR_DURING_PARSING);
			//*NodeName=(char *)malloc(strlen(TokenBuff)+1);
            *NodeName=(char *)malloc(getTokBufLength()+1);
			//strcpy(*NodeName,TokenBuff);
            strcpy(*NodeName,getTokBuf());
			*NodeValue=NULL;
			NodeType=ELEMENT_NODE;
			IsEnd=true;
		}
		//else if(!strcmp(TokenBuff, COMPLETETAG))
        else if(!strcmp(getTokBuf(), COMPLETETAG))
		{
			if(NodeType==ELEMENT_NODE || (NodeType==ATTRIBUTE_NODE))
			{
				IsEnd=false;
				//rewindCurrentPtr(strlen(TokenBuff));
                rewindCurrentPtr(getTokBufLength());
				return 0;
			}

			//store the last element tag and return back as end element node
			//*NodeName=(char *)malloc(strlen(LastElement)+1);
            *NodeName=(char *)malloc(getLastElemLength()+1);
			//strcpy(*NodeName,LastElement);
            strcpy(*NodeName,getLastElem());

			*NodeValue=NULL;
			NodeType=ELEMENT_NODE;
			attrFound=true;
			atLeastOneAttrFound=false;
			IsEnd=true;
			return 0;
		}
		//else if(TokenBuff[0] == '\0')
        else if (getTokBufLength() == 0)
		{
			IsEnd=false;
			continue;
		}
		else if(!attrFound)
		{
            //if (TokenBuff != NULL)
			//{
			//	if(*TokenBuff!='\0')
			//	{
			//		*NodeValue=(char *)malloc(strlen(TokenBuff)+1);
			//		strcpy(*NodeValue,TokenBuff);
			//	}
    		//}
            if (getTokBufLength() != 0){
                *NodeValue=(char *)malloc(getTokBufLength()+1);
                strcpy(*NodeValue,getTokBuf());
            }
			*NodeName=(char *)malloc(strlen("#text")+1);
			strcpy(*NodeName,"#text");
			NodeType=TEXT_NODE;
			IsEnd=false;
			return 0;
		}
		else
		{
			//if(TokenBuff==NULL)
			//	throw DOMException(DOMException::FATAL_ERROR_DURING_PARSING);
			//if(*TokenBuff=='\0')
			//	throw DOMException(DOMException::FATAL_ERROR_DURING_PARSING);
            if (getTokBufLength() == 0)
                throw DOMException(DOMException::FATAL_ERROR_DURING_PARSING);
			if(NodeType==ELEMENT_NODE)
			{
				//rewindCurrentPtr(strlen(TokenBuff)+1);
                rewindCurrentPtr(getTokBufLength()+1);
				return 0;
			}
			//*NodeName=(char *)malloc(strlen(TokenBuff)+1);
            *NodeName=(char *)malloc(getTokBufLength()+1);
			//strcpy(*NodeName,TokenBuff);
            strcpy(*NodeName,getTokBuf());
			if(IgnoreWhiteSpace) 
				IgnoreWhiteSpaces(); 
			//gets rid of equals
			if(getNextToken()!=0)
			{
				*NodeValue=NULL;
				*NodeName=NULL;
				NodeType=INVALID_NODE;
				IsEnd=false;
				return 1;
			}
			if(IgnoreWhiteSpace)
				IgnoreWhiteSpaces(); 
			if(getNextToken()!=0)
			{
				*NodeValue=NULL;
				*NodeName=NULL;
				NodeType=INVALID_NODE;
				IsEnd=false;
				return 1;
			}
			//gets rid of beginning quotes			
			inAttrib=true;
			if(getNextToken()!=0)
			{
				*NodeValue=NULL;
				*NodeName=NULL;
				NodeType=INVALID_NODE;
				IsEnd=false;
				return 1;
			}
			inAttrib=false;
			//if(TokenBuff==NULL)
			//	throw DOMException(DOMException::FATAL_ERROR_DURING_PARSING);
			//if(*TokenBuff=='\0')
			//	throw DOMException(DOMException::FATAL_ERROR_DURING_PARSING);
            if (getTokBufLength() == 0)
                throw DOMException(DOMException::FATAL_ERROR_DURING_PARSING);
			//*NodeValue=(char *)malloc(strlen(TokenBuff)+1);
            *NodeValue=(char *)malloc(getTokBufLength()+1);
			//strcpy(*NodeValue,TokenBuff);
            strcpy(*NodeValue,getTokBuf());
			// gets rid of ending quotes
			if(getNextToken()!=0)
			{
				*NodeValue=NULL;
				*NodeName=NULL;
				NodeType=INVALID_NODE;
				IsEnd=false;
				return 1;
			}
			NodeType=ATTRIBUTE_NODE;
			IsEnd=false;
			atLeastOneAttrFound=true;
			return 0;
		}
	}
	return 0;
}
#endif			
		
		






