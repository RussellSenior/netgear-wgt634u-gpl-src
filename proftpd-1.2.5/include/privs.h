/*
 * ProFTPD - FTP server daemon
 * Copyright (c) 1997, 1998 Public Flood Software
 * Copyright (C) 1999, 2000 MacGyver aka Habeeb J. Dihu <macgyver@tos.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307, USA.
 *
 * As a special exemption, Public Flood Software/MacGyver aka Habeeb J. Dihu
 * and other respective copyright holders give permission to link this program
 * with OpenSSL, and distribute the resulting executable, without including
 * the source code for OpenSSL in the source distribution.
 */

/* $Id: privs.h,v 1.7 2002/05/09 17:36:00 castaglia Exp $
 */

#ifndef __PRIVS_H
#define __PRIVS_H

/* Macros for manipulating saved, real and effective uid for easy 
 * switching from/to root.
 *
 * Note: In version 1.1.5, all of this changed.  We USED to play games
 * with the saved-uid/gid _and_ setreuid()/setregid(), however this
 * appears to be slightly non-portable (i.e. w/ BSDs).  However, since
 * POSIX.1 saved-uids are pretty much useless without setre* (in the
 * case of root), so we now use basic uid swapping if we have seteuid(),
 * and setreuid() swapping if not.
 */

/* Porters, please put the most reasonable and secure method of
 * doing this in here:
 */

#ifdef __hpux
#define setreuid(x,y) setresuid(x,y,0)
#endif

#if !defined(HAVE_SETEUID)
 
/* Use setreuid() to perform uid swapping.
 */

#define PRIVS_SETUP(u,g)	{ log_debug(DEBUG8, "SETUP PRIVS at %s:%d", \
                                  __FILE__, __LINE__); \
                                  if (getuid()) { \
                                    session.ouid = session.uid = getuid(); \
                                    session.gid = getgid(); \
                                    setgid(session.gid); \
                                    setreuid(session.uid, session.uid); \
                                  } else {  \
                                    session.ouid = getuid(); \
                                    session.uid = (u); \
                                    session.gid = (g); \
                                    setgid(session.gid); \
                                    setreuid(0, session.uid); \
                                  } \
                                }

#define PRIVS_ROOT		{ log_debug(DEBUG8, "ROOT PRIVS at %s:%d", \
				  __FILE__, __LINE__); \
				  if (!session.disable_id_switching) \
				    { setreuid(session.uid, 0); \
				} }

#define PRIVS_USER              { log_debug(DEBUG8, "USER PRIVS %d at %s:%d", \
                                  session.login_uid, __FILE__, __LINE__); \
                                  if(!session.disable_id_switching) \
                                    { setreuid(session.uid,0); \
                                      setreuid(session.uid,session.login_uid); \
                                } }

#define PRIVS_RELINQUISH	{ log_debug(DEBUG8, \
                                  "RELINQUISH PRIVS at %s:%d", \
				  __FILE__, __LINE__); \
				  if(!session.disable_id_switching) \
				    { if (geteuid()!=0) \
					{ setreuid(session.uid,0); } \
					  setreuid(session.uid,session.uid); \
				} }

#define PRIVS_REVOKE		{ log_debug(DEBUG8, "REVOKE PRIVS at %s:%d", \
                                  __FILE__, __LINE__); \
                                  setreuid(0,0); \
				  setgid(session.gid); \
                                  setuid(session.uid); }
#else /* HAVE_SETEUID */

/* Set the saved uid/gid using setuid/seteuid().  setreuid() is
 * no longer used as it is considered obsolete on many systems.
 * gids are also no longer swapped, as they are unnecessary.
 * If run as root, proftpd now normally runs as:
 *   real user            : root
 *   effective user       : <user>
 *   saved user           : root
 *   real/eff/saved group : <group>
 */

#define PRIVS_SETUP(u,g)	{ log_debug(DEBUG8, "SETUP PRIVS at %s:%d", \
                                  __FILE__, __LINE__); \
                                  if (getuid()) { \
                                    session.ouid = session.uid = getuid(); \
                                    session.gid = getgid(); \
                                    setgid(session.gid); \
                                    setuid(session.uid); \
				    seteuid(session.uid); \
                                  } else { \
				    session.ouid = getuid(); \
                                    session.uid = (u); \
                                    session.gid = (g); \
                                    setuid(0); \
                                    setgid((g)); \
                                    seteuid((u)); \
                                } }


/* Switch back to root */

#define PRIVS_ROOT		if(!session.disable_id_switching) \
				{ log_debug(DEBUG8, "ROOT PRIVS at %s:%d", \
                                  __FILE__, __LINE__); \
                                  seteuid(0); }

/* Switch to the login user */
#define PRIVS_USER		if(!session.disable_id_switching) \
				{ if (session.login_uid == 0) { \
				    log_debug(DEBUG1, \
                                    "Use of PRIVS_USER before " \
                                    "session.login_uid set in %s %d", \
                                    __FILE__, __LINE__); \
				  } else {\
                                     log_debug(DEBUG8, \
                                       "USER PRIVS %d at %s:%d", \
                                       session.login_uid, __FILE__, __LINE__); \
				     seteuid(0); \
                                     seteuid(session.login_uid); \
				  } }

/* Relinquish privs granted by PRIVS_ROOT or PRIVS_USER */

#define PRIVS_RELINQUISH	if(!session.disable_id_switching) \
				{ if (geteuid()!=0) { seteuid(0); } \
                                  log_debug(DEBUG8, \
                                    "RELINQUISH PRIVS at %s:%d", \
                                  __FILE__, __LINE__); \
				  seteuid(session.uid); }

/* Revoke all privs */

#define PRIVS_REVOKE		{ log_debug(DEBUG8, "REVOKE PRIVS at %s:%d", \
                                  __FILE__, __LINE__); \
                                  seteuid(0); \
				  setgid(session.gid); \
				  setuid(session.uid); }

#endif /* HAVE_SETEUID */

#endif /* __PRIVS_H */
