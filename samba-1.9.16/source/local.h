/* local definitions for file server */
#ifndef _LOCAL_H
#define _LOCAL_H

/* This defines the section name in the configuration file that will contain */
/* global parameters - that is, parameters relating to the whole server, not */
/* just services. This name is then reserved, and may not be used as a       */
/* a service name. It will default to "global" if not defined here.          */
#define GLOBAL_NAME "global"
#define GLOBAL_NAME2 "globals"

/* This defines the section name in the configuration file that will
   refer to the special "homes" service */
#define HOMES_NAME "homes"

/* This defines the section name in the configuration file that will
   refer to the special "printers" service */
#define PRINTERS_NAME "printers"

/* This defines the name of the printcap file. It is MOST UNLIKELY that
   this will change BUT! Specifying a file with the format of a printcap
   file but containing only a subset of the printers actually in your real 
   printcap file is a quick-n-dirty way to allow dynamic access to a subset
   of available printers.
*/
#define PRINTCAP_NAME "/etc/printcap"

/* set these to define the limits of the server. NOTE These are on a
   per-client basis. Thus any one machine can't connect to more than
   MAX_CONNECTIONS services, but any number of machines may connect at
   one time. */
#define MAX_CONNECTIONS 127
#define MAX_OPEN_FILES 100

/* the max number of connections that the smbstatus program will show */
#define MAXSTATUS 1000

/* max number of directories open at once */
/* note that with the new directory code this no longer requires a
   file handle per directory, but large numbers do use more memory */
#define MAXDIR 64

#define WORDMAX 0xFFFF


/* separators for lists */
#define LIST_SEP " \t,;:\n\r"

#ifndef LOCKDIR
#define LOCKDIR "/tmp/samba"
#endif

/* this is where browse lists are kept in the lock dir */
#define SERVER_LIST "browse.dat"

/* shall guest entries in printer queues get changed to user entries,
   so they can be deleted using the windows print manager? */
#define LPQ_GUEST_TO_USER

/* shall filenames with illegal chars in them get mangled in long
   filename listings? */
#define MANGLE_LONG_FILENAMES 

/* define this if you want to stop spoofing with .. and soft links
   NOTE: This also slows down the server considerably */
#define REDUCE_PATHS

/* the size of the directory cache */
#define DIRCACHESIZE 20

/* what type of filesystem do we want this to show up as in a NT file
   manager window? */
#define FSTYPE_STRING "Samba"

/* do you want smbd to send a 1 byte packet to nmbd to trigger it to start 
   when smbd starts? */
#ifndef PRIME_NMBD
#define PRIME_NMBD 1
#endif

/* do you want session setups at user level security with a invalid
   password to be rejected or allowed in as guest? WinNT rejects them
   but it can be a pain as it means "net view" needs to use a password 

   You have 3 choices:

   GUEST_SESSSETUP = 0 means session setups with an invalid password
   are rejected.

   GUEST_SESSSETUP = 1 means session setups with an invalid password
   are rejected, unless the username does not exist, in which case it
   is treated as a guest login

   GUEST_SESSSETUP = 2 means session setups with an invalid password
   are treated as a guest login

   Note that GUEST_SESSSETUP only has an effect in user or server
   level security.
   */
#ifndef GUEST_SESSSETUP
#define GUEST_SESSSETUP 0
#endif

/* the default pager to use for the client "more" command. Users can
   override this with the PAGER environment variable */
#ifndef PAGER
#define PAGER "more"
#endif

/* the size of the uid cache used to reduce valid user checks */
#define UID_CACHE_SIZE 4

/* the following control timings of various actions. Don't change 
   them unless you know what you are doing. These are all in seconds */
#define DEFAULT_SMBD_TIMEOUT (60*60*24*7)
#define SMBD_RELOAD_CHECK (10)
#define SHARE_MODES_CHECK (10)
#define SHARE_MODES_CLEAN (300)
#define IDLE_CLOSED_TIMEOUT (60)
#define DPTR_IDLE_TIMEOUT (120)
#define SMBD_SELECT_LOOP (10)
#define NMBD_SELECT_LOOP (2)
#define BROWSE_INTERVAL (60)
#define REGISTRATION_INTERVAL (10*60)
#define NMBD_INETD_TIMEOUT (120)
#define NMBD_MAX_TTL (24*60*60)
#define LPQ_LOCK_TIMEOUT (5)

/* the following are in milliseconds */
#define LOCK_RETRY_TIMEOUT (100)

/* do you want to dump core (carefully!) when an internal error is
   encountered? Samba will be careful to make the core file only
   accessible to root */
#define DUMP_CORE 1

/* what is the longest significant password available on your system? 
 Knowing this speeds up password searches a lot */
#ifndef PASSWORD_LENGTH
#define PASSWORD_LENGTH 8
#endif

#define SMB_ALIGNMENT 1


/* shall we support browse requests via a FIFO to nmbd? */
#define ENABLE_FIFO 1

/* keep the password server open, this uses up a aocket, but is needed
   by many apps */
#define KEEP_PASSWORD_SERVER_OPEN 1

/* how long to wait for a socket connect to happen */
#define LONG_CONNECT_TIMEOUT 30
#define SHORT_CONNECT_TIMEOUT 5

#endif
