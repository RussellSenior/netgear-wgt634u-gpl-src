/* 
   Unix SMB/Netbios implementation.
   Version 1.9.
   Parameter loading functions
   Copyright (C) Karl Auer 1993,1994

   Largely re-written by Andrew Tridgell, September 1994
   
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
*/

/*
 *  Load parameters.
 *
 *  This module provides suitable callback functions for the params
 *  module. It builds the internal table of service details which is
 *  then used by the rest of the server.
 *
 * To add a parameter:
 *
 * 1) add it to the global or service structure definition
 * 2) add it to the parm_table
 * 3) add it to the list of available functions (eg: using FN_GLOBAL_STRING())
 * 4) If it's a global then initialise it in init_globals. If a local
 *    (ie. service) parameter then initialise it in the sDefault structure
 *  
 *
 * Notes:
 *   The configuration file is processed sequentially for speed. It is NOT
 *   accessed randomly as happens in 'real' Windows. For this reason, there
 *   is a fair bit of sequence-dependent code here - ie., code which assumes
 *   that certain things happen before others. In particular, the code which
 *   happens at the boundary between sections is delicately poised, so be
 *   careful!
 *
 */

#include "includes.h"

BOOL bLoaded = False;

extern int DEBUGLEVEL;
extern pstring user_socket_options;
extern pstring myname;

#ifndef GLOBAL_NAME
#define GLOBAL_NAME "global"
#endif

#ifndef PRINTCAP_NAME
#ifdef AIX
#define PRINTCAP_NAME "/etc/qconfig"
#else
#define PRINTCAP_NAME "/etc/printcap"
#endif
#endif

#ifndef PRINTERS_NAME
#define PRINTERS_NAME "printers"
#endif

#ifndef HOMES_NAME
#define HOMES_NAME "homes"
#endif

/* some helpful bits */
#define pSERVICE(i) ServicePtrs[i]
#define iSERVICE(i) (*pSERVICE(i))
#define LP_SNUM_OK(iService) (((iService) >= 0) && ((iService) < iNumServices) && iSERVICE(iService).valid)
#define VALID(i) iSERVICE(i).valid

/* these are the types of parameter we have */
typedef enum
{
  P_BOOL,P_BOOLREV,P_CHAR,P_INTEGER,P_OCTAL,
  P_STRING,P_USTRING,P_GSTRING,P_UGSTRING
} parm_type;

typedef enum
{
  P_LOCAL,P_GLOBAL,P_NONE
} parm_class;

int keepalive=0;
extern BOOL use_getwd_cache;

extern int extra_time_offset;
#ifdef KANJI
extern int coding_system;
#endif

/* 
 * This structure describes global (ie., server-wide) parameters.
 */
typedef struct
{
  char *szPrintcapname;
  char *szLockDir;
  char *szRootdir;
  char *szDefaultService;
  char *szDfree;
  char *szMsgCommand;
  char *szHostsEquiv;
  char *szServerString;
  char *szAutoServices;
  char *szPasswdProgram;
  char *szPasswdChat;
  char *szLogFile;
  char *szConfigFile;
  char *szSMBPasswdFile;
  char *szPasswordServer;
  char *szSocketOptions;
  char *szValidChars;
  char *szWorkGroup;
  char *szDomainController;
  char *szUsernameMap;
  char *szCharacterSet;
  char *szLogonScript;
  char *szSmbrun;
  char *szWINSserver;
  char *szInterfaces;
  char *szRemoteAnnounce;
  char *szSocketAddress;
  int max_log_size;
  int mangled_stack;
  int max_xmit;
  int max_mux;
  int max_packet;
  int pwordlevel;
  int deadtime;
  int maxprotocol;
  int security;
  int printing;
  int maxdisksize;
  int lpqcachetime;
  int syslog;
  int os_level;
  int max_ttl;
  int ReadSize;
  BOOL bWINSsupport;
  BOOL bWINSproxy;
  BOOL bPreferredMaster;
  BOOL bDomainMaster;
  BOOL bDomainLogons;
  BOOL bEncryptPasswords;
  BOOL bStripDot;
  BOOL bNullPasswords;
  BOOL bLoadPrinters;
  BOOL bUseRhosts;
  BOOL bReadRaw;
  BOOL bWriteRaw;
  BOOL bReadPrediction;
  BOOL bReadbmpx;
  BOOL bSyslogOnly;
  BOOL bBrowseList;
} global;

static global Globals;



/* 
 * This structure describes a single service. 
 */
typedef struct
{
  BOOL valid;
  char *szService;
  char *szPath;
  char *szUsername;
  char *szGuestaccount;
  char *szInvalidUsers;
  char *szValidUsers;
  char *szAdminUsers;
  char *szCopy;
  char *szInclude;
  char *szPreExec;
  char *szPostExec;
  char *szRootPreExec;
  char *szRootPostExec;
  char *szPrintcommand;
  char *szLpqcommand;
  char *szLprmcommand;
  char *szLppausecommand;
  char *szLpresumecommand;
  char *szPrintername;
  char *szPrinterDriver;
  char *szDontdescend;
  char *szHostsallow;
  char *szHostsdeny;
  char *szMagicScript;
  char *szMagicOutput;
  char *szMangledMap;
  char *comment;
  char *force_user;
  char *force_group;
  char *readlist;
  char *writelist;
  char *volume;
  int  iMinPrintSpace;
  int  iCreate_mode;
  int  iMaxConnections;
  int  iDefaultCase;
  BOOL bAlternatePerm;
  BOOL bRevalidate;
  BOOL bCaseSensitive;
  BOOL bCasePreserve;
  BOOL bShortCasePreserve;
  BOOL bCaseMangle;
  BOOL status;
  BOOL bHideDotFiles;
  BOOL bBrowseable;
  BOOL bAvailable;
  BOOL bRead_only;
  BOOL bNo_set_dir;
  BOOL bGuest_only;
  BOOL bGuest_ok;
  BOOL bPrint_ok;
  BOOL bPostscript;
  BOOL bMap_system;
  BOOL bMap_hidden;
  BOOL bMap_archive;
  BOOL bLocking;
  BOOL bStrictLocking;
  BOOL bShareModes;
  BOOL bOnlyUser;
  BOOL bMangledNames;
  BOOL bWidelinks;
  BOOL bSyncAlways;
  char magic_char;
  BOOL *copymap;
  BOOL bDeleteReadonly;
  char dummy[3]; /* for alignment */
} service;


/* This is a default service used to prime a services structure */
static service sDefault = 
{
  True,   /* valid */
  NULL,    /* szService */
  NULL,    /* szPath */
  NULL,    /* szUsername */
  NULL,    /* szGuestAccount */
  NULL,    /* szInvalidUsers */
  NULL,    /* szValidUsers */
  NULL,    /* szAdminUsers */
  NULL,    /* szCopy */
  NULL,    /* szInclude */
  NULL,    /* szPreExec */
  NULL,    /* szPostExec */
  NULL,    /* szRootPreExec */
  NULL,    /* szRootPostExec */
  NULL,    /* szPrintcommand */
  NULL,    /* szLpqcommand */
  NULL,    /* szLprmcommand */
  NULL,    /* szLppausecommand */
  NULL,    /* szLpresumecommand */
  NULL,    /* szPrintername */
  NULL,    /* szPrinterDriver */
  NULL,    /* szDontdescend */
  NULL,    /* szHostsallow */
  NULL,    /* szHostsdeny */
  NULL,    /* szMagicScript */
  NULL,    /* szMagicOutput */
  NULL,    /* szMangledMap */
  NULL,    /* comment */
  NULL,    /* force user */
  NULL,    /* force group */
  NULL,    /* readlist */
  NULL,    /* writelist */
  NULL,    /* volume */
  0,       /* iMinPrintSpace */
  0755,    /* iCreate_mode */
  0,       /* iMaxConnections */
  CASE_LOWER, /* iDefaultCase */
  False,   /* bAlternatePerm */
  False,   /* revalidate */
  False,   /* case sensitive */
  False,   /* case preserve */
  False,   /* short case preserve */
  False,  /* case mangle */
  True,  /* status */
  True,  /* bHideDotFiles */
  True,  /* bBrowseable */
  True,  /* bAvailable */
  True,  /* bRead_only */
  True,  /* bNo_set_dir */
  False, /* bGuest_only */
  False, /* bGuest_ok */
  False, /* bPrint_ok */
  False, /* bPostscript */
  False, /* bMap_system */
  False, /* bMap_hidden */
  True,  /* bMap_archive */
  True,  /* bLocking */
  False,  /* bStrictLocking */
  True,  /* bShareModes */
  False, /* bOnlyUser */
  True,  /* bMangledNames */
  True,  /* bWidelinks */
  False, /* bSyncAlways */
  '~',   /* magic char */
  NULL,  /* copymap */
  False, /* bDeleteReadonly */
  ""     /* dummy */
};



/* local variables */
static service **ServicePtrs = NULL;
static int iNumServices = 0;
static int iServiceIndex = 0;
static BOOL bInGlobalSection = True;
static BOOL bGlobalOnly = False;


#define NUMPARAMETERS (sizeof(parm_table) / sizeof(struct parm_struct))

/* prototypes for the special type handlers */
static BOOL handle_valid_chars(char *pszParmValue, char **ptr);
static BOOL handle_include(char *pszParmValue, char **ptr);
static BOOL handle_copy(char *pszParmValue, char **ptr);
static BOOL handle_protocol(char *pszParmValue,int *val);
static BOOL handle_security(char *pszParmValue,int *val);
static BOOL handle_case(char *pszParmValue,int *val);
static BOOL handle_printing(char *pszParmValue,int *val);
static BOOL handle_character_set(char *pszParmValue,int *val);
#ifdef KANJI
static BOOL handle_coding_system(char *pszParmValue,int *val);
#endif /* KANJI */

struct parm_struct
{
  char *label;
  parm_type type;
  parm_class class;
  void *ptr;
  BOOL (*special)();
} parm_table[] =
{
  {"debuglevel",       P_INTEGER, P_GLOBAL, &DEBUGLEVEL,                NULL},
  {"log level",        P_INTEGER, P_GLOBAL, &DEBUGLEVEL,                NULL},
  {"syslog",           P_INTEGER, P_GLOBAL, &Globals.syslog,            NULL},
  {"syslog only",      P_BOOL,    P_GLOBAL, &Globals.bSyslogOnly,       NULL},
  {"protocol",         P_INTEGER, P_GLOBAL, &Globals.maxprotocol,handle_protocol},
  {"security",         P_INTEGER, P_GLOBAL, &Globals.security,handle_security},
  {"printing",         P_INTEGER, P_GLOBAL, &Globals.printing,handle_printing},
  {"max disk size",    P_INTEGER, P_GLOBAL, &Globals.maxdisksize,       NULL},
  {"lpq cache time",   P_INTEGER, P_GLOBAL, &Globals.lpqcachetime,      NULL},
  {"encrypt passwords",P_BOOL,    P_GLOBAL, &Globals.bEncryptPasswords, NULL},
  {"getwd cache",      P_BOOL,    P_GLOBAL, &use_getwd_cache,           NULL},
  {"read prediction",  P_BOOL,    P_GLOBAL, &Globals.bReadPrediction,   NULL},
  {"read bmpx",        P_BOOL,    P_GLOBAL, &Globals.bReadbmpx,         NULL},
  {"read raw",         P_BOOL,    P_GLOBAL, &Globals.bReadRaw,          NULL},
  {"write raw",        P_BOOL,    P_GLOBAL, &Globals.bWriteRaw,         NULL},
  {"use rhosts",       P_BOOL,    P_GLOBAL, &Globals.bUseRhosts,        NULL},
  {"load printers",    P_BOOL,    P_GLOBAL, &Globals.bLoadPrinters,     NULL},
  {"null passwords",   P_BOOL,    P_GLOBAL, &Globals.bNullPasswords,    NULL},
  {"strip dot",        P_BOOL,    P_GLOBAL, &Globals.bStripDot,         NULL},
  {"interfaces",       P_STRING,  P_GLOBAL, &Globals.szInterfaces,      NULL},
  {"password server",  P_STRING,  P_GLOBAL, &Globals.szPasswordServer,  NULL},
  {"socket options",   P_GSTRING, P_GLOBAL, user_socket_options,        NULL},
  {"netbios name",     P_UGSTRING,P_GLOBAL, myname,                     NULL},
  {"smbrun",           P_STRING,  P_GLOBAL, &Globals.szSmbrun,          NULL},
  {"log file",         P_STRING,  P_GLOBAL, &Globals.szLogFile,         NULL},
  {"config file",      P_STRING,  P_GLOBAL, &Globals.szConfigFile,      NULL},
  {"smb passwd file",  P_STRING,  P_GLOBAL, &Globals.szSMBPasswdFile,   NULL},
  {"hosts equiv",      P_STRING,  P_GLOBAL, &Globals.szHostsEquiv,      NULL},
  {"preload",          P_STRING,  P_GLOBAL, &Globals.szAutoServices,    NULL},
  {"auto services",    P_STRING,  P_GLOBAL, &Globals.szAutoServices,    NULL},
  {"server string",    P_STRING,  P_GLOBAL, &Globals.szServerString,    NULL},
  {"printcap name",    P_STRING,  P_GLOBAL, &Globals.szPrintcapname,    NULL},
  {"printcap",         P_STRING,  P_GLOBAL, &Globals.szPrintcapname,    NULL},
  {"lock dir",         P_STRING,  P_GLOBAL, &Globals.szLockDir,         NULL},
  {"lock directory",   P_STRING,  P_GLOBAL, &Globals.szLockDir,         NULL},
  {"root directory",   P_STRING,  P_GLOBAL, &Globals.szRootdir,         NULL},
  {"root dir",         P_STRING,  P_GLOBAL, &Globals.szRootdir,         NULL},
  {"root",             P_STRING,  P_GLOBAL, &Globals.szRootdir,         NULL},
  {"default service",  P_STRING,  P_GLOBAL, &Globals.szDefaultService,  NULL},
  {"default",          P_STRING,  P_GLOBAL, &Globals.szDefaultService,  NULL},
  {"message command",  P_STRING,  P_GLOBAL, &Globals.szMsgCommand,      NULL},
  {"dfree command",    P_STRING,  P_GLOBAL, &Globals.szDfree,           NULL},
  {"passwd program",   P_STRING,  P_GLOBAL, &Globals.szPasswdProgram,   NULL},
  {"passwd chat",      P_STRING,  P_GLOBAL, &Globals.szPasswdChat,      NULL},
  {"valid chars",      P_STRING,  P_GLOBAL, &Globals.szValidChars,      handle_valid_chars},
  {"workgroup",        P_USTRING, P_GLOBAL, &Globals.szWorkGroup,       NULL},
  {"domain controller",P_STRING,  P_GLOBAL, &Globals.szDomainController,NULL},
  {"username map",     P_STRING,  P_GLOBAL, &Globals.szUsernameMap,     NULL},
  {"character set",    P_STRING,  P_GLOBAL, &Globals.szCharacterSet,    handle_character_set},
  {"logon script",     P_STRING,  P_GLOBAL, &Globals.szLogonScript,     NULL},
  {"remote announce",  P_STRING,  P_GLOBAL, &Globals.szRemoteAnnounce,  NULL},
  {"socket address",   P_STRING,  P_GLOBAL, &Globals.szSocketAddress,   NULL},
  {"max log size",     P_INTEGER, P_GLOBAL, &Globals.max_log_size,      NULL},
  {"mangled stack",    P_INTEGER, P_GLOBAL, &Globals.mangled_stack,     NULL},
  {"max mux",          P_INTEGER, P_GLOBAL, &Globals.max_mux,           NULL},
  {"max xmit",         P_INTEGER, P_GLOBAL, &Globals.max_xmit,          NULL},
  {"max packet",       P_INTEGER, P_GLOBAL, &Globals.max_packet,        NULL},
  {"packet size",      P_INTEGER, P_GLOBAL, &Globals.max_packet,        NULL},
  {"password level",   P_INTEGER, P_GLOBAL, &Globals.pwordlevel,        NULL},
  {"keepalive",        P_INTEGER, P_GLOBAL, &keepalive,                 NULL},
  {"deadtime",         P_INTEGER, P_GLOBAL, &Globals.deadtime,          NULL},
  {"time offset",      P_INTEGER, P_GLOBAL, &extra_time_offset,         NULL},
  {"read size",        P_INTEGER, P_GLOBAL, &Globals.ReadSize,          NULL},
#ifdef KANJI
  {"coding system",    P_INTEGER, P_GLOBAL, &coding_system, handle_coding_system},
#endif /* KANJI */
  {"os level",         P_INTEGER, P_GLOBAL, &Globals.os_level,          NULL},
  {"max ttl",          P_INTEGER, P_GLOBAL, &Globals.max_ttl,           NULL},
  {"wins support",     P_BOOL,    P_GLOBAL, &Globals.bWINSsupport,      NULL},
  {"wins proxy",       P_BOOL,    P_GLOBAL, &Globals.bWINSproxy,        NULL},
  {"wins server",      P_STRING,  P_GLOBAL, &Globals.szWINSserver,      NULL},
  {"preferred master", P_BOOL,    P_GLOBAL, &Globals.bPreferredMaster,  NULL},
  {"prefered master",  P_BOOL,    P_GLOBAL, &Globals.bPreferredMaster,  NULL},
  {"domain master",    P_BOOL,    P_GLOBAL, &Globals.bDomainMaster,     NULL},
  {"domain logons",    P_BOOL,    P_GLOBAL, &Globals.bDomainLogons,     NULL},
  {"browse list",      P_BOOL,    P_GLOBAL, &Globals.bBrowseList,       NULL},

  {"-valid",           P_BOOL,    P_LOCAL,  &sDefault.valid,            NULL},
  {"comment",          P_STRING,  P_LOCAL,  &sDefault.comment,          NULL},
  {"copy",             P_STRING,  P_LOCAL,  &sDefault.szCopy,    handle_copy},
  {"include",          P_STRING,  P_LOCAL,  &sDefault.szInclude, handle_include},
  {"exec",             P_STRING,  P_LOCAL,  &sDefault.szPreExec,        NULL},
  {"preexec",          P_STRING,  P_LOCAL,  &sDefault.szPreExec,        NULL},
  {"postexec",         P_STRING,  P_LOCAL,  &sDefault.szPostExec,       NULL},
  {"root preexec",     P_STRING,  P_LOCAL,  &sDefault.szRootPreExec,    NULL},
  {"root postexec",    P_STRING,  P_LOCAL,  &sDefault.szRootPostExec,   NULL},
  {"alternate permissions",P_BOOL,P_LOCAL,  &sDefault.bAlternatePerm,   NULL},
  {"revalidate",       P_BOOL,    P_LOCAL,  &sDefault.bRevalidate,      NULL},
  {"default case",     P_INTEGER, P_LOCAL,  &sDefault.iDefaultCase,   handle_case},
  {"case sensitive",   P_BOOL,    P_LOCAL,  &sDefault.bCaseSensitive,   NULL},
  {"casesignames",     P_BOOL,    P_LOCAL,  &sDefault.bCaseSensitive,   NULL},
  {"preserve case",    P_BOOL,    P_LOCAL,  &sDefault.bCasePreserve,    NULL},
  {"short preserve case",P_BOOL,  P_LOCAL,  &sDefault.bShortCasePreserve,NULL},
  {"mangle case",      P_BOOL,    P_LOCAL,  &sDefault.bCaseMangle,      NULL},
  {"mangling char",    P_CHAR,    P_LOCAL,  &sDefault.magic_char,       NULL},
  {"browseable",       P_BOOL,    P_LOCAL,  &sDefault.bBrowseable,      NULL},
  {"browsable",        P_BOOL,    P_LOCAL,  &sDefault.bBrowseable,      NULL},
  {"available",        P_BOOL,    P_LOCAL,  &sDefault.bAvailable,       NULL},
  {"path",             P_STRING,  P_LOCAL,  &sDefault.szPath,           NULL},
  {"directory",        P_STRING,  P_LOCAL,  &sDefault.szPath,           NULL},
  {"username",         P_STRING,  P_LOCAL,  &sDefault.szUsername,       NULL},
  {"user",             P_STRING,  P_LOCAL,  &sDefault.szUsername,       NULL},
  {"users",            P_STRING,  P_LOCAL,  &sDefault.szUsername,       NULL},
  {"guest account",    P_STRING,  P_LOCAL,  &sDefault.szGuestaccount,   NULL},
  {"invalid users",    P_STRING,  P_LOCAL,  &sDefault.szInvalidUsers,   NULL},
  {"valid users",      P_STRING,  P_LOCAL,  &sDefault.szValidUsers,     NULL},
  {"admin users",      P_STRING,  P_LOCAL,  &sDefault.szAdminUsers,     NULL},
  {"read list",        P_STRING,  P_LOCAL,  &sDefault.readlist,         NULL},
  {"write list",       P_STRING,  P_LOCAL,  &sDefault.writelist,        NULL},
  {"volume",           P_STRING,  P_LOCAL,  &sDefault.volume,           NULL},
  {"force user",       P_STRING,  P_LOCAL,  &sDefault.force_user,       NULL},
  {"force group",      P_STRING,  P_LOCAL,  &sDefault.force_group,      NULL},
  {"group",            P_STRING,  P_LOCAL,  &sDefault.force_group,      NULL},
  {"read only",        P_BOOL,    P_LOCAL,  &sDefault.bRead_only,       NULL},
  {"write ok",         P_BOOLREV, P_LOCAL,  &sDefault.bRead_only,       NULL},
  {"writeable",        P_BOOLREV, P_LOCAL,  &sDefault.bRead_only,       NULL},
  {"writable",         P_BOOLREV, P_LOCAL,  &sDefault.bRead_only,       NULL},
  {"max connections",  P_INTEGER, P_LOCAL,  &sDefault.iMaxConnections,  NULL},
  {"min print space",  P_INTEGER, P_LOCAL,  &sDefault.iMinPrintSpace,   NULL},
  {"create mask",      P_OCTAL,   P_LOCAL,  &sDefault.iCreate_mode,     NULL},
  {"create mode",      P_OCTAL,   P_LOCAL,  &sDefault.iCreate_mode,     NULL},
  {"set directory",    P_BOOLREV, P_LOCAL,  &sDefault.bNo_set_dir,      NULL},
  {"status",           P_BOOL,    P_LOCAL,  &sDefault.status,           NULL},
  {"hide dot files",   P_BOOL,    P_LOCAL,  &sDefault.bHideDotFiles,    NULL},
  {"guest only",       P_BOOL,    P_LOCAL,  &sDefault.bGuest_only,      NULL},
  {"only guest",       P_BOOL,    P_LOCAL,  &sDefault.bGuest_only,      NULL},
  {"guest ok",         P_BOOL,    P_LOCAL,  &sDefault.bGuest_ok,        NULL},
  {"public",           P_BOOL,    P_LOCAL,  &sDefault.bGuest_ok,        NULL},
  {"print ok",         P_BOOL,    P_LOCAL,  &sDefault.bPrint_ok,        NULL},
  {"printable",        P_BOOL,    P_LOCAL,  &sDefault.bPrint_ok,        NULL},
  {"postscript",       P_BOOL,    P_LOCAL,  &sDefault.bPostscript,      NULL},
  {"map system",       P_BOOL,    P_LOCAL,  &sDefault.bMap_system,      NULL},
  {"map hidden",       P_BOOL,    P_LOCAL,  &sDefault.bMap_hidden,      NULL},
  {"map archive",      P_BOOL,    P_LOCAL,  &sDefault.bMap_archive,     NULL},
  {"locking",          P_BOOL,    P_LOCAL,  &sDefault.bLocking,         NULL},
  {"strict locking",   P_BOOL,    P_LOCAL,  &sDefault.bStrictLocking,   NULL},
  {"share modes",      P_BOOL,    P_LOCAL,  &sDefault.bShareModes,      NULL},
  {"only user",        P_BOOL,    P_LOCAL,  &sDefault.bOnlyUser,        NULL},
  {"wide links",       P_BOOL,    P_LOCAL,  &sDefault.bWidelinks,       NULL},
  {"sync always",      P_BOOL,    P_LOCAL,  &sDefault.bSyncAlways,      NULL},
  {"mangled names",    P_BOOL,    P_LOCAL,  &sDefault.bMangledNames,    NULL},
  {"print command",    P_STRING,  P_LOCAL,  &sDefault.szPrintcommand,   NULL},
  {"lpq command",      P_STRING,  P_LOCAL,  &sDefault.szLpqcommand,     NULL},
  {"lprm command",     P_STRING,  P_LOCAL,  &sDefault.szLprmcommand,    NULL},
  {"lppause command",  P_STRING,  P_LOCAL,  &sDefault.szLppausecommand, NULL},
  {"lpresume command", P_STRING,  P_LOCAL,  &sDefault.szLpresumecommand,NULL},
  {"printer",          P_STRING,  P_LOCAL,  &sDefault.szPrintername,    NULL},
  {"printer name",     P_STRING,  P_LOCAL,  &sDefault.szPrintername,    NULL},
  {"printer driver",   P_STRING,  P_LOCAL,  &sDefault.szPrinterDriver,  NULL},
  {"hosts allow",      P_STRING,  P_LOCAL,  &sDefault.szHostsallow,     NULL},
  {"allow hosts",      P_STRING,  P_LOCAL,  &sDefault.szHostsallow,     NULL},
  {"hosts deny",       P_STRING,  P_LOCAL,  &sDefault.szHostsdeny,      NULL},
  {"deny hosts",       P_STRING,  P_LOCAL,  &sDefault.szHostsdeny,      NULL},
  {"dont descend",     P_STRING,  P_LOCAL,  &sDefault.szDontdescend,    NULL},
  {"magic script",     P_STRING,  P_LOCAL,  &sDefault.szMagicScript,    NULL},
  {"magic output",     P_STRING,  P_LOCAL,  &sDefault.szMagicOutput,    NULL},
  {"mangled map",      P_STRING,  P_LOCAL,  &sDefault.szMangledMap,     NULL},
  {"delete readonly",  P_BOOL,    P_LOCAL,  &sDefault.bDeleteReadonly,  NULL},

  {NULL,               P_BOOL,    P_NONE,   NULL,                       NULL}
};



/***************************************************************************
Initialise the global parameter structure.
***************************************************************************/
static void init_globals(void)
{
  static BOOL done_init = False;
  pstring s;

  if (!done_init)
    {
      int i;
      bzero((void *)&Globals,sizeof(Globals));

      for (i = 0; parm_table[i].label; i++) 
	if ((parm_table[i].type == P_STRING ||
	     parm_table[i].type == P_USTRING) && 
	    parm_table[i].ptr)
	  string_init(parm_table[i].ptr,"");

      string_set(&sDefault.szGuestaccount, GUEST_ACCOUNT);

      done_init = True;
    }


  DEBUG(3,("Initialising global parameters\n"));

#ifdef SMB_PASSWD_FILE
  string_set(&Globals.szSMBPasswdFile, SMB_PASSWD_FILE);
#endif
  string_set(&Globals.szPasswdChat,"*old*password* %o\\n *new*password* %n\\n *new*password* %n\\n *changed*");
  string_set(&Globals.szWorkGroup, WORKGROUP);
#ifdef SMB_PASSWD
  string_set(&Globals.szPasswdProgram, SMB_PASSWD);
#else
  string_set(&Globals.szPasswdProgram, "/bin/passwd");
#endif
  string_set(&Globals.szPrintcapname, PRINTCAP_NAME);
  string_set(&Globals.szLockDir, LOCKDIR);
  string_set(&Globals.szRootdir, "/");
  string_set(&Globals.szSmbrun, SMBRUN);
  string_set(&Globals.szSocketAddress, "0.0.0.0");
  sprintf(s,"WGT634U");
//  sprintf(s,"Samba %s",VERSION);
  string_set(&Globals.szServerString,s);
  Globals.bLoadPrinters = True;
  Globals.bUseRhosts = False;
  Globals.max_packet = 65535;
  Globals.mangled_stack = 50;
  Globals.max_xmit = Globals.max_packet;
  Globals.max_mux = 2;
  Globals.lpqcachetime = 10;
  Globals.pwordlevel = 0;
  Globals.deadtime = 0;
  Globals.max_log_size = 5000;
  Globals.maxprotocol = PROTOCOL_NT1;
  Globals.security = SEC_SHARE;
  Globals.bEncryptPasswords = False;
  Globals.printing = DEFAULT_PRINTING;
  Globals.bReadRaw = True;
  Globals.bWriteRaw = True;
  Globals.bReadPrediction = False;
  Globals.bReadbmpx = True;
  Globals.bNullPasswords = False;
  Globals.bStripDot = False;
  Globals.syslog = 1;
  Globals.bSyslogOnly = False;
  Globals.os_level = 0;
  Globals.max_ttl = 60*60*4; /* 2 hours default */
  Globals.bPreferredMaster = True;
  Globals.bDomainMaster = False;
  Globals.bDomainLogons = False;
  Globals.bBrowseList = True;
  Globals.bWINSsupport = True;
  Globals.bWINSproxy = False;
  Globals.ReadSize = 16*1024;

#ifdef KANJI
  coding_system = interpret_coding_system (KANJI, SJIS_CODE);
#endif /* KANJI */

}

/***************************************************************************
check if a string is initialised and if not then initialise it
***************************************************************************/
static void string_initial(char **s,char *v)
{
  if (!*s || !**s)
    string_init(s,v);
}


/***************************************************************************
Initialise the sDefault parameter structure.
***************************************************************************/
static void init_locals(void)
{
  /* choose defaults depending on the type of printing */
  switch (Globals.printing)
    {
    case PRINT_BSD:
    case PRINT_AIX:
    case PRINT_LPRNG:
    case PRINT_PLP:
      string_initial(&sDefault.szLpqcommand,"lpq -P%p");
      string_initial(&sDefault.szLprmcommand,"lprm -P%p %j");
      string_initial(&sDefault.szPrintcommand,"lpr -r -P%p %s");
      break;

    case PRINT_SYSV:
    case PRINT_HPUX:
      string_initial(&sDefault.szLpqcommand,"lpstat -o%p");
      string_initial(&sDefault.szLprmcommand,"cancel %p-%j");
      string_initial(&sDefault.szPrintcommand,"lp -c -d%p %s; rm %s");
#ifdef SVR4
      string_initial(&sDefault.szLppausecommand,"lp -i %p-%j -H hold");
      string_initial(&sDefault.szLpresumecommand,"lp -i %p-%j -H resume");
#endif
      break;

    case PRINT_QNX:
      string_initial(&sDefault.szLpqcommand,"lpq -P%p");
      string_initial(&sDefault.szLprmcommand,"lprm -P%p %j");
      string_initial(&sDefault.szPrintcommand,"lp -r -P%p %s");
      break;

      
    }
}


/******************************************************************* a
convenience routine to grab string parameters into a rotating buffer,
and run standard_sub_basic on them. The buffers can be written to by
callers without affecting the source string.
********************************************************************/
char *lp_string(char *s)
{
  static char *bufs[10];
  static int buflen[10];
  static int next = -1;  
  char *ret;
  int i;
  int len = s?strlen(s):0;

  if (next == -1) {
    /* initialisation */
    for (i=0;i<10;i++) {
      bufs[i] = NULL;
      buflen[i] = 0;
    }
    next = 0;
  }

  len = MAX(len+100,sizeof(pstring)); /* the +100 is for some
					 substitution room */

  if (buflen[next] != len) {
    buflen[next] = len;
    if (bufs[next]) free(bufs[next]);
    bufs[next] = (char *)malloc(len);
    if (!bufs[next]) {
      DEBUG(0,("out of memory in lp_string()"));
      exit(1);
    }
  } 

  ret = &bufs[next][0];
  next = (next+1)%10;

  if (!s) 
    *ret = 0;
  else
    StrCpy(ret,s);

  standard_sub_basic(ret);
  return(ret);
}


/*
   In this section all the functions that are used to access the 
   parameters from the rest of the program are defined 
*/

#define FN_GLOBAL_STRING(fn_name,ptr) \
 char *fn_name(void) {return(lp_string(*(char **)(ptr) ? *(char **)(ptr) : ""));}
#define FN_GLOBAL_BOOL(fn_name,ptr) \
 BOOL fn_name(void) {return(*(BOOL *)(ptr));}
#define FN_GLOBAL_CHAR(fn_name,ptr) \
 char fn_name(void) {return(*(char *)(ptr));}
#define FN_GLOBAL_INTEGER(fn_name,ptr) \
 int fn_name(void) {return(*(int *)(ptr));}

#define FN_LOCAL_STRING(fn_name,val) \
 char *fn_name(int i) {return(lp_string((LP_SNUM_OK(i)&&pSERVICE(i)->val)?pSERVICE(i)->val : sDefault.val));}
#define FN_LOCAL_BOOL(fn_name,val) \
 BOOL fn_name(int i) {return(LP_SNUM_OK(i)? pSERVICE(i)->val : sDefault.val);}
#define FN_LOCAL_CHAR(fn_name,val) \
 char fn_name(int i) {return(LP_SNUM_OK(i)? pSERVICE(i)->val : sDefault.val);}
#define FN_LOCAL_INTEGER(fn_name,val) \
 int fn_name(int i) {return(LP_SNUM_OK(i)? pSERVICE(i)->val : sDefault.val);}

FN_GLOBAL_STRING(lp_logfile,&Globals.szLogFile)
FN_GLOBAL_STRING(lp_smbrun,&Globals.szSmbrun)
FN_GLOBAL_STRING(lp_configfile,&Globals.szConfigFile)
FN_GLOBAL_STRING(lp_smb_passwd_file,&Globals.szSMBPasswdFile)
FN_GLOBAL_STRING(lp_serverstring,&Globals.szServerString)
FN_GLOBAL_STRING(lp_printcapname,&Globals.szPrintcapname)
FN_GLOBAL_STRING(lp_lockdir,&Globals.szLockDir)
FN_GLOBAL_STRING(lp_rootdir,&Globals.szRootdir)
FN_GLOBAL_STRING(lp_defaultservice,&Globals.szDefaultService)
FN_GLOBAL_STRING(lp_msg_command,&Globals.szMsgCommand)
FN_GLOBAL_STRING(lp_dfree_command,&Globals.szDfree)
FN_GLOBAL_STRING(lp_hosts_equiv,&Globals.szHostsEquiv)
FN_GLOBAL_STRING(lp_auto_services,&Globals.szAutoServices)
FN_GLOBAL_STRING(lp_passwd_program,&Globals.szPasswdProgram)
FN_GLOBAL_STRING(lp_passwd_chat,&Globals.szPasswdChat)
FN_GLOBAL_STRING(lp_passwordserver,&Globals.szPasswordServer)
FN_GLOBAL_STRING(lp_workgroup,&Globals.szWorkGroup)
FN_GLOBAL_STRING(lp_domain_controller,&Globals.szDomainController)
FN_GLOBAL_STRING(lp_username_map,&Globals.szUsernameMap)
FN_GLOBAL_STRING(lp_character_set,&Globals.szCharacterSet) 
FN_GLOBAL_STRING(lp_logon_script,&Globals.szLogonScript) 
FN_GLOBAL_STRING(lp_remote_announce,&Globals.szRemoteAnnounce) 
FN_GLOBAL_STRING(lp_wins_server,&Globals.szWINSserver)
FN_GLOBAL_STRING(lp_interfaces,&Globals.szInterfaces)
FN_GLOBAL_STRING(lp_socket_address,&Globals.szSocketAddress)

FN_GLOBAL_BOOL(lp_wins_support,&Globals.bWINSsupport)
FN_GLOBAL_BOOL(lp_wins_proxy,&Globals.bWINSproxy)
FN_GLOBAL_BOOL(lp_domain_master,&Globals.bDomainMaster)
FN_GLOBAL_BOOL(lp_domain_logons,&Globals.bDomainLogons)
FN_GLOBAL_BOOL(lp_preferred_master,&Globals.bPreferredMaster)
FN_GLOBAL_BOOL(lp_load_printers,&Globals.bLoadPrinters)
FN_GLOBAL_BOOL(lp_use_rhosts,&Globals.bUseRhosts)
FN_GLOBAL_BOOL(lp_getwdcache,&use_getwd_cache)
FN_GLOBAL_BOOL(lp_readprediction,&Globals.bReadPrediction)
FN_GLOBAL_BOOL(lp_readbmpx,&Globals.bReadbmpx)
FN_GLOBAL_BOOL(lp_readraw,&Globals.bReadRaw)
FN_GLOBAL_BOOL(lp_writeraw,&Globals.bWriteRaw)
FN_GLOBAL_BOOL(lp_null_passwords,&Globals.bNullPasswords)
FN_GLOBAL_BOOL(lp_strip_dot,&Globals.bStripDot)
FN_GLOBAL_BOOL(lp_encrypted_passwords,&Globals.bEncryptPasswords)
FN_GLOBAL_BOOL(lp_syslog_only,&Globals.bSyslogOnly)
FN_GLOBAL_BOOL(lp_browse_list,&Globals.bBrowseList)

FN_GLOBAL_INTEGER(lp_os_level,&Globals.os_level)
FN_GLOBAL_INTEGER(lp_max_ttl,&Globals.max_ttl)
FN_GLOBAL_INTEGER(lp_max_log_size,&Globals.max_log_size)
FN_GLOBAL_INTEGER(lp_mangledstack,&Globals.mangled_stack)
FN_GLOBAL_INTEGER(lp_maxxmit,&Globals.max_xmit)
FN_GLOBAL_INTEGER(lp_maxmux,&Globals.max_mux)
FN_GLOBAL_INTEGER(lp_maxpacket,&Globals.max_packet)
FN_GLOBAL_INTEGER(lp_keepalive,&keepalive)
FN_GLOBAL_INTEGER(lp_passwordlevel,&Globals.pwordlevel)
FN_GLOBAL_INTEGER(lp_readsize,&Globals.ReadSize)
FN_GLOBAL_INTEGER(lp_deadtime,&Globals.deadtime)
FN_GLOBAL_INTEGER(lp_maxprotocol,&Globals.maxprotocol)
FN_GLOBAL_INTEGER(lp_security,&Globals.security)
FN_GLOBAL_INTEGER(lp_printing,&Globals.printing)
FN_GLOBAL_INTEGER(lp_maxdisksize,&Globals.maxdisksize)
FN_GLOBAL_INTEGER(lp_lpqcachetime,&Globals.lpqcachetime)
FN_GLOBAL_INTEGER(lp_syslog,&Globals.syslog)

FN_LOCAL_STRING(lp_preexec,szPreExec)
FN_LOCAL_STRING(lp_postexec,szPostExec)
FN_LOCAL_STRING(lp_rootpreexec,szRootPreExec)
FN_LOCAL_STRING(lp_rootpostexec,szRootPostExec)
FN_LOCAL_STRING(lp_servicename,szService)
FN_LOCAL_STRING(lp_pathname,szPath)
FN_LOCAL_STRING(lp_dontdescend,szDontdescend)
FN_LOCAL_STRING(lp_username,szUsername)
FN_LOCAL_STRING(lp_guestaccount,szGuestaccount)
FN_LOCAL_STRING(lp_invalid_users,szInvalidUsers)
FN_LOCAL_STRING(lp_valid_users,szValidUsers)
FN_LOCAL_STRING(lp_admin_users,szAdminUsers)
FN_LOCAL_STRING(lp_printcommand,szPrintcommand)
FN_LOCAL_STRING(lp_lpqcommand,szLpqcommand)
FN_LOCAL_STRING(lp_lprmcommand,szLprmcommand)
FN_LOCAL_STRING(lp_lppausecommand,szLppausecommand)
FN_LOCAL_STRING(lp_lpresumecommand,szLpresumecommand)
FN_LOCAL_STRING(lp_printername,szPrintername)
FN_LOCAL_STRING(lp_printerdriver,szPrinterDriver)
FN_LOCAL_STRING(lp_hostsallow,szHostsallow)
FN_LOCAL_STRING(lp_hostsdeny,szHostsdeny)
FN_LOCAL_STRING(lp_magicscript,szMagicScript)
FN_LOCAL_STRING(lp_magicoutput,szMagicOutput)
FN_LOCAL_STRING(lp_comment,comment)
FN_LOCAL_STRING(lp_force_user,force_user)
FN_LOCAL_STRING(lp_force_group,force_group)
FN_LOCAL_STRING(lp_readlist,readlist)
FN_LOCAL_STRING(lp_writelist,writelist)
FN_LOCAL_STRING(lp_volume,volume)
FN_LOCAL_STRING(lp_mangled_map,szMangledMap)

FN_LOCAL_BOOL(lp_alternate_permissions,bAlternatePerm)
FN_LOCAL_BOOL(lp_revalidate,bRevalidate)
FN_LOCAL_BOOL(lp_casesensitive,bCaseSensitive)
FN_LOCAL_BOOL(lp_preservecase,bCasePreserve)
FN_LOCAL_BOOL(lp_shortpreservecase,bShortCasePreserve)
FN_LOCAL_BOOL(lp_casemangle,bCaseMangle)
FN_LOCAL_BOOL(lp_status,status)
FN_LOCAL_BOOL(lp_hide_dot_files,bHideDotFiles)
FN_LOCAL_BOOL(lp_browseable,bBrowseable)
FN_LOCAL_BOOL(lp_readonly,bRead_only)
FN_LOCAL_BOOL(lp_no_set_dir,bNo_set_dir)
FN_LOCAL_BOOL(lp_guest_ok,bGuest_ok)
FN_LOCAL_BOOL(lp_guest_only,bGuest_only)
FN_LOCAL_BOOL(lp_print_ok,bPrint_ok)
FN_LOCAL_BOOL(lp_postscript,bPostscript)
FN_LOCAL_BOOL(lp_map_hidden,bMap_hidden)
FN_LOCAL_BOOL(lp_map_archive,bMap_archive)
FN_LOCAL_BOOL(lp_locking,bLocking)
FN_LOCAL_BOOL(lp_strict_locking,bStrictLocking)
FN_LOCAL_BOOL(lp_share_modes,bShareModes)
FN_LOCAL_BOOL(lp_onlyuser,bOnlyUser)
FN_LOCAL_BOOL(lp_manglednames,bMangledNames)
FN_LOCAL_BOOL(lp_widelinks,bWidelinks)
FN_LOCAL_BOOL(lp_syncalways,bSyncAlways)
FN_LOCAL_BOOL(lp_map_system,bMap_system)
FN_LOCAL_BOOL(lp_delete_readonly,bDeleteReadonly)

FN_LOCAL_INTEGER(lp_create_mode,iCreate_mode)
FN_LOCAL_INTEGER(lp_max_connections,iMaxConnections)
FN_LOCAL_INTEGER(lp_defaultcase,iDefaultCase)
FN_LOCAL_INTEGER(lp_minprintspace,iMinPrintSpace)

FN_LOCAL_CHAR(lp_magicchar,magic_char)



/* local prototypes */
static int    strwicmp( char *psz1, char *psz2 );
static int    map_parameter( char *pszParmName);
static BOOL   set_boolean( BOOL *pb, char *pszParmValue );
static int    getservicebyname(char *pszServiceName, service *pserviceDest);
static void   copy_service( service *pserviceDest, 
                            service *pserviceSource,
                            BOOL *pcopymapDest );
static BOOL   service_ok(int iService);
static BOOL   do_parameter(char *pszParmName, char *pszParmValue);
static BOOL   do_section(char *pszSectionName);
static void   dump_globals(void);
static void   dump_a_service(service *pService);
static void init_copymap(service *pservice);


/***************************************************************************
initialise a service to the defaults
***************************************************************************/
static void init_service(service *pservice)
{
  bzero((char *)pservice,sizeof(service));
  copy_service(pservice,&sDefault,NULL);
}


/***************************************************************************
free the dynamically allocated parts of a service struct
***************************************************************************/
static void free_service(service *pservice)
{
  int i;
  if (!pservice)
     return;

  for (i=0;parm_table[i].label;i++)
    if ((parm_table[i].type == P_STRING ||
	 parm_table[i].type == P_STRING) &&
	parm_table[i].class == P_LOCAL)
      string_free((char **)(((char *)pservice) + PTR_DIFF(parm_table[i].ptr,&sDefault)));
}

/***************************************************************************
add a new service to the services array initialising it with the given 
service
***************************************************************************/
static int add_a_service(service *pservice, char *name)
{
  int i;
  service tservice;
  int num_to_alloc = iNumServices+1;

  tservice = *pservice;

  /* it might already exist */
  if (name) 
    {
      i = getservicebyname(name,NULL);
      if (i >= 0)
	return(i);
    }

  /* find an invalid one */
  for (i=0;i<iNumServices;i++)
    if (!pSERVICE(i)->valid)
      break;

  /* if not, then create one */
  if (i == iNumServices)
    {
      ServicePtrs = (service **)Realloc(ServicePtrs,sizeof(service *)*num_to_alloc);
      if (ServicePtrs)
	pSERVICE(iNumServices) = (service *)malloc(sizeof(service));

      if (!ServicePtrs || !pSERVICE(iNumServices))
	return(-1);

      iNumServices++;
    }
  else
    free_service(pSERVICE(i));

  pSERVICE(i)->valid = True;

  init_service(pSERVICE(i));
  copy_service(pSERVICE(i),&tservice,NULL);
  if (name)
    string_set(&iSERVICE(i).szService,name);  

  return(i);
}

/***************************************************************************
add a new home service, with the specified home directory, defaults coming 
from service ifrom
***************************************************************************/
BOOL lp_add_home(char *pszHomename, int iDefaultService, char *pszHomedir)
{
  int i = add_a_service(pSERVICE(iDefaultService),pszHomename);

  if (i < 0)
    return(False);

  if (!(*(iSERVICE(i).szPath)) || strequal(iSERVICE(i).szPath,lp_pathname(-1)))
    string_set(&iSERVICE(i).szPath,pszHomedir);
  if (!(*(iSERVICE(i).comment)))
    {
      pstring comment;
      sprintf(comment,"Home directory of %s",pszHomename);
      string_set(&iSERVICE(i).comment,comment);
    }
  iSERVICE(i).bAvailable = sDefault.bAvailable;
  iSERVICE(i).bBrowseable = sDefault.bBrowseable;

  DEBUG(3,("adding home directory %s at %s\n", pszHomename, pszHomedir));

  return(True);
}

/***************************************************************************
add a new service, based on an old one
***************************************************************************/
int lp_add_service(char *pszService, int iDefaultService)
{
  return(add_a_service(pSERVICE(iDefaultService),pszService));
}


/***************************************************************************
add the IPC service
***************************************************************************/
static BOOL lp_add_ipc(void)
{
  pstring comment;
  int i = add_a_service(&sDefault,"IPC$");

  if (i < 0)
    return(False);

  sprintf(comment,"IPC Service (%s)",lp_serverstring());

  string_set(&iSERVICE(i).szPath,"/tmp");
  string_set(&iSERVICE(i).szUsername,"");
  string_set(&iSERVICE(i).comment,comment);
  iSERVICE(i).status = False;
  iSERVICE(i).iMaxConnections = 0;
  iSERVICE(i).bAvailable = True;
  iSERVICE(i).bRead_only = True;
  iSERVICE(i).bGuest_only = False;
  iSERVICE(i).bGuest_ok = True;
  iSERVICE(i).bPrint_ok = False;
  iSERVICE(i).bBrowseable = sDefault.bBrowseable;

  DEBUG(3,("adding IPC service\n"));

  return(True);
}


/***************************************************************************
add a new printer service, with defaults coming from service iFrom
***************************************************************************/
BOOL lp_add_printer(char *pszPrintername, int iDefaultService)
{
  char *comment = "From Printcap";
  int i = add_a_service(pSERVICE(iDefaultService),pszPrintername);
  
  if (i < 0)
    return(False);
  
  /* note that we do NOT default the availability flag to True - */
  /* we take it from the default service passed. This allows all */
  /* dynamic printers to be disabled by disabling the [printers] */
  /* entry (if/when the 'available' keyword is implemented!).    */
  
  /* the printer name is set to the service name. */
  string_set(&iSERVICE(i).szPrintername,pszPrintername);
  string_set(&iSERVICE(i).comment,comment);
  iSERVICE(i).bBrowseable = sDefault.bBrowseable;
  
  DEBUG(3,("adding printer service %s\n",pszPrintername));
  
  return(True);
}


/***************************************************************************
Do a case-insensitive, whitespace-ignoring string compare.
***************************************************************************/
static int strwicmp(char *psz1, char *psz2)
{
   /* if BOTH strings are NULL, return TRUE, if ONE is NULL return */
   /* appropriate value. */
   if (psz1 == psz2)
      return (0);
   else
      if (psz1 == NULL)
         return (-1);
      else
          if (psz2 == NULL)
              return (1);

   /* sync the strings on first non-whitespace */
   while (1)
   {
      while (isspace(*psz1))
         psz1++;
      while (isspace(*psz2))
         psz2++;
      if (toupper(*psz1) != toupper(*psz2) || *psz1 == '\0' || *psz2 == '\0')
         break;
      psz1++;
      psz2++;
   }
   return (*psz1 - *psz2);
}

/***************************************************************************
Map a parameter's string representation to something we can use. 
Returns False if the parameter string is not recognised, else TRUE.
***************************************************************************/
static int map_parameter(char *pszParmName)
{
   int iIndex;

   if (*pszParmName == '-')
     return(-1);

   for (iIndex = 0; parm_table[iIndex].label; iIndex++) 
      if (strwicmp(parm_table[iIndex].label, pszParmName) == 0)
         return(iIndex);

   DEBUG(0,( "Unknown parameter encountered: \"%s\"\n", pszParmName));
   return(-1);
}


/***************************************************************************
Set a boolean variable from the text value stored in the passed string.
Returns True in success, False if the passed string does not correctly 
represent a boolean.
***************************************************************************/
static BOOL set_boolean(BOOL *pb, char *pszParmValue)
{
   BOOL bRetval;

   bRetval = True;
   if (strwicmp(pszParmValue, "yes") == 0 ||
       strwicmp(pszParmValue, "true") == 0 ||
       strwicmp(pszParmValue, "1") == 0)
      *pb = True;
   else
      if (strwicmp(pszParmValue, "no") == 0 ||
          strwicmp(pszParmValue, "False") == 0 ||
          strwicmp(pszParmValue, "0") == 0)
         *pb = False;
      else
      {
         DEBUG(0,( "Badly formed boolean in configuration file: \"%s\".\n",
               pszParmValue));
         bRetval = False;
      }
   return (bRetval);
}

/***************************************************************************
Find a service by name. Otherwise works like get_service.
***************************************************************************/
static int getservicebyname(char *pszServiceName, service *pserviceDest)
{
   int iService;

   for (iService = iNumServices - 1; iService >= 0; iService--)
      if (VALID(iService) &&
	  strwicmp(iSERVICE(iService).szService, pszServiceName) == 0) 
      {
         if (pserviceDest != NULL)
	   copy_service(pserviceDest, pSERVICE(iService), NULL);
         break;
      }

   return (iService);
}



/***************************************************************************
Copy a service structure to another

If pcopymapDest is NULL then copy all fields
***************************************************************************/
static void copy_service(service *pserviceDest, 
                         service *pserviceSource,
                         BOOL *pcopymapDest)
{
  int i;
  BOOL bcopyall = (pcopymapDest == NULL);

  for (i=0;parm_table[i].label;i++)
    if (parm_table[i].ptr && parm_table[i].class == P_LOCAL && 
	(bcopyall || pcopymapDest[i]))
      {
	void *def_ptr = parm_table[i].ptr;
	void *src_ptr = 
	  ((char *)pserviceSource) + PTR_DIFF(def_ptr,&sDefault);
	void *dest_ptr = 
	  ((char *)pserviceDest) + PTR_DIFF(def_ptr,&sDefault);

	switch (parm_table[i].type)
	  {
	  case P_BOOL:
	  case P_BOOLREV:
	    *(BOOL *)dest_ptr = *(BOOL *)src_ptr;
	    break;

	  case P_INTEGER:
	  case P_OCTAL:
	    *(int *)dest_ptr = *(int *)src_ptr;
	    break;

	  case P_CHAR:
	    *(char *)dest_ptr = *(char *)src_ptr;
	    break;

	  case P_STRING:
	    string_set(dest_ptr,*(char **)src_ptr);
	    break;

	  case P_USTRING:
	    string_set(dest_ptr,*(char **)src_ptr);
	    strupper(*(char **)dest_ptr);
	    break;
	  default:
	    break;
	  }
      }

  if (bcopyall)
    {
      init_copymap(pserviceDest);
      if (pserviceSource->copymap)
	memcpy((void *)pserviceDest->copymap,
	       (void *)pserviceSource->copymap,sizeof(BOOL)*NUMPARAMETERS);
    }
}

/***************************************************************************
Check a service for consistency. Return False if the service is in any way
incomplete or faulty, else True.
***************************************************************************/
static BOOL service_ok(int iService)
{
   BOOL bRetval;

   bRetval = True;
   if (iSERVICE(iService).szService[0] == '\0')
   {
      DEBUG(0,( "The following message indicates an internal error:\n"));
      DEBUG(0,( "No service name in service entry.\n"));
      bRetval = False;
   }

   /* The [printers] entry MUST be printable. I'm all for flexibility, but */
   /* I can't see why you'd want a non-printable printer service...        */
   if (strwicmp(iSERVICE(iService).szService,PRINTERS_NAME) == 0)
      if (!iSERVICE(iService).bPrint_ok)
      {
         DEBUG(0,( "WARNING: [%s] service MUST be printable!\n",
               iSERVICE(iService).szService));
	 iSERVICE(iService).bPrint_ok = True;
      }

   if (iSERVICE(iService).szPath[0] == '\0' &&
       strwicmp(iSERVICE(iService).szService,HOMES_NAME) != 0)
   {
      DEBUG(0,("No path in service %s - using /tmp\n",iSERVICE(iService).szService));
      string_set(&iSERVICE(iService).szPath,"/tmp");      
   }

   /* If a service is flagged unavailable, log the fact at level 0. */
   if (!iSERVICE(iService).bAvailable) 
      DEBUG(1,( "NOTE: Service %s is flagged unavailable.\n",
            iSERVICE(iService).szService));

   return (bRetval);
}

static struct file_lists {
  struct file_lists *next;
  char *name;
  time_t modtime;
} *file_lists = NULL;

/*******************************************************************
keep a linked list of all config files so we know when one has changed 
it's date and needs to be reloaded
********************************************************************/
static void add_to_file_list(char *fname)
{
  struct file_lists *f=file_lists;

  while (f) {
    if (f->name && !strcmp(f->name,fname)) break;
    f = f->next;
  }

  if (!f) {
    f = (struct file_lists *)malloc(sizeof(file_lists[0]));
    if (!f) return;
    f->next = file_lists;
    f->name = strdup(fname);
    if (!f->name) {
      free(f);
      return;
    }
    file_lists = f;
  }

  {
    pstring n2;
    strcpy(n2,fname);
    standard_sub_basic(n2);
    f->modtime = file_modtime(n2);
  }

}

/*******************************************************************
check if a config file has changed date
********************************************************************/
BOOL lp_file_list_changed(void)
{
  struct file_lists *f = file_lists;
  while (f) {
    pstring n2;
    strcpy(n2,f->name);
    standard_sub_basic(n2);
    if (f->modtime != file_modtime(n2)) return(True);
    f = f->next;   
  }
  return(False);
}

#ifdef KANJI
/***************************************************************************
  handle the interpretation of the coding system parameter
  *************************************************************************/
static BOOL handle_coding_system(char *pszParmValue,int *val)
{
  *val = interpret_coding_system(pszParmValue,*val);
  return(True);
}
#endif /* KANJI */

/***************************************************************************
handle the interpretation of the character set system parameter
***************************************************************************/
static BOOL handle_character_set(char *pszParmValue,int *val)
{
  string_set(&Globals.szCharacterSet,pszParmValue);
  *val = interpret_character_set(pszParmValue,*val);
  return(True);
}


/***************************************************************************
handle the interpretation of the protocol parameter
***************************************************************************/
static BOOL handle_protocol(char *pszParmValue,int *val)
{
  *val = interpret_protocol(pszParmValue,*val);
  return(True);
}

/***************************************************************************
handle the interpretation of the security parameter
***************************************************************************/
static BOOL handle_security(char *pszParmValue,int *val)
{
  *val = interpret_security(pszParmValue,*val);
  return(True);
}

/***************************************************************************
handle the interpretation of the default case
***************************************************************************/
static BOOL handle_case(char *pszParmValue,int *val)
{
  if (strequal(pszParmValue,"LOWER"))
    *val = CASE_LOWER;
  else if (strequal(pszParmValue,"UPPER"))
    *val = CASE_UPPER;
  return(True);
}

/***************************************************************************
handle the interpretation of the printing system
***************************************************************************/
static BOOL handle_printing(char *pszParmValue,int *val)
{
  if (strequal(pszParmValue,"sysv"))
    *val = PRINT_SYSV;
  else if (strequal(pszParmValue,"aix"))
    *val = PRINT_AIX;
  else if (strequal(pszParmValue,"hpux"))
    *val = PRINT_HPUX;
  else if (strequal(pszParmValue,"bsd"))
    *val = PRINT_BSD;
  else if (strequal(pszParmValue,"qnx"))
    *val = PRINT_QNX;
  else if (strequal(pszParmValue,"plp"))
    *val = PRINT_PLP;
  else if (strequal(pszParmValue,"lprng"))
    *val = PRINT_LPRNG;
  return(True);
}

/***************************************************************************
handle the valid chars lines
***************************************************************************/
static BOOL handle_valid_chars(char *pszParmValue,char **ptr)
{ 
  string_set(ptr,pszParmValue);

  add_char_string(pszParmValue);
  return(True);
}


/***************************************************************************
handle the include operation
***************************************************************************/
static BOOL handle_include(char *pszParmValue,char **ptr)
{ 
  pstring fname;
  strcpy(fname,pszParmValue);

  add_to_file_list(fname);

  standard_sub_basic(fname);

  string_set(ptr,fname);

  if (file_exist(fname,NULL))
    return(pm_process(fname, do_section, do_parameter));      

  DEBUG(2,("Can't find include file %s\n",fname));

  return(False);
}


/***************************************************************************
handle the interpretation of the copy parameter
***************************************************************************/
static BOOL handle_copy(char *pszParmValue,char **ptr)
{
   BOOL bRetval;
   int iTemp;
   service serviceTemp;

   string_set(ptr,pszParmValue);

   init_service(&serviceTemp);

   bRetval = False;
   
   DEBUG(3,("Copying service from service %s\n",pszParmValue));

   if ((iTemp = getservicebyname(pszParmValue, &serviceTemp)) >= 0)
     {
       if (iTemp == iServiceIndex)
	 {
	   DEBUG(0,("Can't copy service %s - unable to copy self!\n",
		    pszParmValue));
	 }
       else
	 {
	   copy_service(pSERVICE(iServiceIndex), 
			&serviceTemp,
			iSERVICE(iServiceIndex).copymap);
	   bRetval = True;
	 }
     }
   else
     {
       DEBUG(0,( "Unable to copy service - source not found: %s\n",
		pszParmValue));
       bRetval = False;
     }

   free_service(&serviceTemp);
   return (bRetval);
}


/***************************************************************************
initialise a copymap
***************************************************************************/
static void init_copymap(service *pservice)
{
  int i;
  if (pservice->copymap) free(pservice->copymap);
  pservice->copymap = (BOOL *)malloc(sizeof(BOOL)*NUMPARAMETERS);
  if (!pservice->copymap)
    DEBUG(0,("Couldn't allocate copymap!! (size %d)\n",NUMPARAMETERS));

  for (i=0;i<NUMPARAMETERS;i++)
    pservice->copymap[i] = True;
}


/***************************************************************************
Process a parameter.
***************************************************************************/
static BOOL do_parameter(char *pszParmName, char *pszParmValue)
{
   int parmnum;
   void *parm_ptr=NULL; /* where we are going to store the result */
   void *def_ptr=NULL;

   if (!bInGlobalSection && bGlobalOnly) return(True);

   DEBUG(3,("doing parameter %s = %s\n",pszParmName,pszParmValue));
   
   parmnum = map_parameter(pszParmName);

   if (parmnum < 0)
     {
       DEBUG(0,( "Ignoring unknown parameter \"%s\"\n", pszParmName));
       return(True);
     }

   def_ptr = parm_table[parmnum].ptr;

   /* we might point at a service, the default service or a global */
   if (bInGlobalSection)
     parm_ptr = def_ptr;
   else
     {
       if (parm_table[parmnum].class == P_GLOBAL)
	 {
	   DEBUG(0,( "Global parameter %s found in service section!\n",pszParmName));
	   return(True);
	 }
       parm_ptr = ((char *)pSERVICE(iServiceIndex)) + PTR_DIFF(def_ptr,&sDefault);
     }

   if (!bInGlobalSection)
     {
       int i;
       if (!iSERVICE(iServiceIndex).copymap)
	 init_copymap(pSERVICE(iServiceIndex));
       
       /* this handles the aliases - set the copymap for other entries with
	  the same data pointer */
       for (i=0;parm_table[i].label;i++)
	 if (parm_table[i].ptr == parm_table[parmnum].ptr)
	   iSERVICE(iServiceIndex).copymap[i] = False;
     }

   /* if it is a special case then go ahead */
   if (parm_table[parmnum].special)
     {
       parm_table[parmnum].special(pszParmValue,parm_ptr);
       return(True);
     }

   /* now switch on the type of variable it is */
   switch (parm_table[parmnum].type)
     {
     case P_BOOL:
       set_boolean(parm_ptr,pszParmValue);
       break;

     case P_BOOLREV:
       set_boolean(parm_ptr,pszParmValue);
       *(BOOL *)parm_ptr = ! *(BOOL *)parm_ptr;
       break;

     case P_INTEGER:
       *(int *)parm_ptr = atoi(pszParmValue);
       break;

     case P_CHAR:
       *(char *)parm_ptr = *pszParmValue;
       break;

     case P_OCTAL:
       sscanf(pszParmValue,"%o",(int *)parm_ptr);
       break;

     case P_STRING:
       string_set(parm_ptr,pszParmValue);
       break;

     case P_USTRING:
       string_set(parm_ptr,pszParmValue);
       strupper(*(char **)parm_ptr);
       break;

     case P_GSTRING:
       strcpy((char *)parm_ptr,pszParmValue);
       break;

     case P_UGSTRING:
       strcpy((char *)parm_ptr,pszParmValue);
       strupper((char *)parm_ptr);
       break;
     }

   return(True);
}

/***************************************************************************
print a parameter of the specified type
***************************************************************************/
static void print_parameter(parm_type type,void *ptr)
{
  switch (type)
    {
    case P_BOOL:
      printf("%s",BOOLSTR(*(BOOL *)ptr));
      break;
      
    case P_BOOLREV:
      printf("%s",BOOLSTR(! *(BOOL *)ptr));
      break;
      
    case P_INTEGER:
      printf("%d",*(int *)ptr);
      break;
      
    case P_CHAR:
      printf("%c",*(char *)ptr);
      break;
      
    case P_OCTAL:
      printf("0%o",*(int *)ptr);
      break;
      
    case P_GSTRING:
    case P_UGSTRING:
      if ((char *)ptr)
	printf("%s",(char *)ptr);
      break;

    case P_STRING:
    case P_USTRING:
      if (*(char **)ptr)
	printf("%s",*(char **)ptr);
      break;
    }
}


/***************************************************************************
check if two parameters are equal
***************************************************************************/
static BOOL equal_parameter(parm_type type,void *ptr1,void *ptr2)
{
  switch (type)
    {
    case P_BOOL:
    case P_BOOLREV:
      return(*((BOOL *)ptr1) == *((BOOL *)ptr2));

    case P_INTEGER:
    case P_OCTAL:
      return(*((int *)ptr1) == *((int *)ptr2));
      
    case P_CHAR:
      return(*((char *)ptr1) == *((char *)ptr2));

    case P_GSTRING:
    case P_UGSTRING:
      {
	char *p1 = (char *)ptr1, *p2 = (char *)ptr2;
	if (p1 && !*p1) p1 = NULL;
	if (p2 && !*p2) p2 = NULL;
	return(p1==p2 || strequal(p1,p2));
      }
    case P_STRING:
    case P_USTRING:
      {
	char *p1 = *(char **)ptr1, *p2 = *(char **)ptr2;
	if (p1 && !*p1) p1 = NULL;
	if (p2 && !*p2) p2 = NULL;
	return(p1==p2 || strequal(p1,p2));
      }
    }
  return(False);
}

/***************************************************************************
Process a new section (service). At this stage all sections are services.
Later we'll have special sections that permit server parameters to be set.
Returns True on success, False on failure.
***************************************************************************/
static BOOL do_section(char *pszSectionName)
{
   BOOL bRetval;
   BOOL isglobal = ((strwicmp(pszSectionName, GLOBAL_NAME) == 0) || 
		    (strwicmp(pszSectionName, GLOBAL_NAME2) == 0));
   bRetval = False;

   /* if we were in a global section then do the local inits */
   if (bInGlobalSection && !isglobal)
     init_locals();

   /* if we've just struck a global section, note the fact. */
   bInGlobalSection = isglobal;   

   /* check for multiple global sections */
   if (bInGlobalSection)
   {
     DEBUG(3,( "Processing section \"[%s]\"\n", pszSectionName));
     return(True);
   }

   if (!bInGlobalSection && bGlobalOnly) return(True);

   /* if we have a current service, tidy it up before moving on */
   bRetval = True;

   if (iServiceIndex >= 0)
     bRetval = service_ok(iServiceIndex);

   /* if all is still well, move to the next record in the services array */
   if (bRetval)
     {
       /* We put this here to avoid an odd message order if messages are */
       /* issued by the post-processing of a previous section. */
       DEBUG(2,( "Processing section \"[%s]\"\n", pszSectionName));

       if ((iServiceIndex=add_a_service(&sDefault,pszSectionName)) < 0)
	 {
	   DEBUG(0,("Failed to add a new service\n"));
	   return(False);
	 }
     }

   return (bRetval);
}

/***************************************************************************
Display the contents of the global structure.
***************************************************************************/
static void dump_globals(void)
{
  int i;
  printf("Global parameters:\n");

  for (i=0;parm_table[i].label;i++)
    if (parm_table[i].class == P_GLOBAL &&
	parm_table[i].ptr &&
	(i == 0 || (parm_table[i].ptr != parm_table[i-1].ptr)))
      {
	printf("\t%s: ",parm_table[i].label);
	print_parameter(parm_table[i].type,parm_table[i].ptr);
	printf("\n");
      }
}

/***************************************************************************
Display the contents of a single services record.
***************************************************************************/
static void dump_a_service(service *pService)
{
  int i;
  if (pService == &sDefault)
    printf("\nDefault service parameters:\n");
  else
    printf("\nService parameters [%s]:\n",pService->szService);

  for (i=0;parm_table[i].label;i++)
    if (parm_table[i].class == P_LOCAL &&
	parm_table[i].ptr && 
	(*parm_table[i].label != '-') &&
	(i == 0 || (parm_table[i].ptr != parm_table[i-1].ptr)))
      {
	int pdiff = PTR_DIFF(parm_table[i].ptr,&sDefault);

	if (pService == &sDefault || !equal_parameter(parm_table[i].type,
						      ((char *)pService) + pdiff,
						      ((char *)&sDefault) + pdiff))
	  {
	    printf("\t%s: ",parm_table[i].label);
	    print_parameter(parm_table[i].type,
			    ((char *)pService) + pdiff);
	    printf("\n");
	  }
      }
}

#if 0
/***************************************************************************
Display the contents of a single copy structure.
***************************************************************************/
static void dump_copy_map(BOOL *pcopymap)
{
  int i;
  if (!pcopymap) return;

  printf("\n\tNon-Copied parameters:\n");

  for (i=0;parm_table[i].label;i++)
    if (parm_table[i].class == P_LOCAL &&
	parm_table[i].ptr && !pcopymap[i] &&
	(i == 0 || (parm_table[i].ptr != parm_table[i-1].ptr)))
      {
	printf("\t\t%s\n",parm_table[i].label);
      }
}
#endif

/***************************************************************************
Return TRUE if the passed service number is within range.
***************************************************************************/
BOOL lp_snum_ok(int iService)
{
   return (LP_SNUM_OK(iService) && iSERVICE(iService).bAvailable);
}


/***************************************************************************
auto-load some homes and printer services
***************************************************************************/
static void lp_add_auto_services(char *str)
{
  char *s;
  char *p;
  int homes = lp_servicenumber(HOMES_NAME);
  int printers = lp_servicenumber(PRINTERS_NAME);

  if (!str)
    return;

  s = strdup(str);
  if (!s) return;

  for (p=strtok(s,LIST_SEP);p;p=strtok(NULL,LIST_SEP))
    {
      char *home = get_home_dir(p);

      if (lp_servicenumber(p) >= 0) continue;

      if (home && homes >= 0)
	{
	  lp_add_home(p,homes,home);
	  continue;
	}

      if (printers >= 0 && pcap_printername_ok(p,NULL))
	lp_add_printer(p,printers);
    }
  free(s);
}

/***************************************************************************
auto-load one printer
***************************************************************************/
static void lp_add_one_printer(char *name,char *comment)
{
  int printers = lp_servicenumber(PRINTERS_NAME);
  int i;

  if (lp_servicenumber(name) < 0)
    {
      lp_add_printer(name,printers);
      if ((i=lp_servicenumber(name)) >= 0)
	string_set(&iSERVICE(i).comment,comment);
    }      
}


/***************************************************************************
auto-load printer services
***************************************************************************/
static void lp_add_all_printers(void)
{
  int printers = lp_servicenumber(PRINTERS_NAME);

  if (printers < 0) return;

  pcap_printer_fn(lp_add_one_printer);
}

/***************************************************************************
have we loaded a services file yet?
***************************************************************************/
BOOL lp_loaded(void)
{
  return(bLoaded);
}

/***************************************************************************
unload unused services
***************************************************************************/
void lp_killunused(BOOL (*snumused)(int ))
{
  int i;
  for (i=0;i<iNumServices;i++)
    if (VALID(i) && !snumused(i))
      {
	iSERVICE(i).valid = False;
	free_service(pSERVICE(i));
      }
}

/***************************************************************************
Load the services array from the services file. Return True on success, 
False on failure.
***************************************************************************/
BOOL lp_load(char *pszFname,BOOL global_only)
{
  pstring n2;
  BOOL bRetval;
  
  add_to_file_list(pszFname);

  bRetval = False;

  bInGlobalSection = True;
  bGlobalOnly = global_only;
  
  init_globals();
  
  strcpy(n2,pszFname);
  standard_sub_basic(n2);

  /* We get sections first, so have to start 'behind' to make up */
  iServiceIndex = -1;
  bRetval = pm_process(n2, do_section, do_parameter);
  
  /* finish up the last section */
  DEBUG(3,("pm_process() returned %s\n", BOOLSTR(bRetval)));
  if (bRetval)
    if (iServiceIndex >= 0)
      bRetval = service_ok(iServiceIndex);	   

  lp_add_auto_services(lp_auto_services());
  if (lp_load_printers())
    lp_add_all_printers();

  lp_add_ipc();

  bLoaded = True;

  return (bRetval);
}


/***************************************************************************
return the max number of services
***************************************************************************/
int lp_numservices(void)
{
  return(iNumServices);
}

/***************************************************************************
Display the contents of the services array in human-readable form.
***************************************************************************/
void lp_dump(void)
{
   int iService;

   dump_globals();
   
   dump_a_service(&sDefault);

   for (iService = 0; iService < iNumServices; iService++)
   {
     if (VALID(iService))
       {
	 if (iSERVICE(iService).szService[0] == '\0')
	   break;
	 dump_a_service(pSERVICE(iService));
       }
   }
}

/***************************************************************************
Return the number of the service with the given name, or -1 if it doesn't
exist. Note that this is a DIFFERENT ANIMAL from the internal function
getservicebyname()! This works ONLY if all services have been loaded, and
does not copy the found service.
***************************************************************************/
int lp_servicenumber(char *pszServiceName)
{
   int iService;

   for (iService = iNumServices - 1; iService >= 0; iService--)
      if (VALID(iService) &&
	  strwicmp(iSERVICE(iService).szService, pszServiceName) == 0) 
         break;

   if (iService < 0)
     DEBUG(7,("lp_servicenumber: couldn't find %s\n",pszServiceName));
   
   return (iService);
}

/*******************************************************************
  a useful volume label function
  ******************************************************************/
char *volume_label(int snum)
{
  char *ret = lp_volume(snum);
  if (!*ret) return(lp_servicename(snum));
  return(ret);
}
