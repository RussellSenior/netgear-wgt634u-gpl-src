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

/* Read configuration file(s), and manage server/configuration
 * structures.
 * $Id: dirtree.c,v 1.53 2002/05/21 20:47:22 castaglia Exp $
 */

/* History:
 * 5/1/97 0.99.0pl2
 *  Used to be named "config.c", renamed to dirtree.c (directive
 *  tree) so as not to conflict with GNU autoconf's top-level config.h.
 */

#include "conf.h"

#include <sys/stat.h>
#include <stdarg.h>

#ifdef HAVE_ARPA_INET_H
# include <arpa/inet.h>
#endif

xaset_t *servers = NULL;
server_rec *main_server = NULL;
int tcpBackLog = TUNABLE_DEFAULT_BACKLOG;
int SocketBindTight = FALSE;
char ServerType = SERVER_STANDALONE;
int ServerMaxInstances = 0;
int ServerUseReverseDNS = TRUE;
int TimeoutLogin = TUNABLE_TIMEOUTLOGIN;
int TimeoutIdle = TUNABLE_TIMEOUTIDLE;
int TimeoutNoXfer = TUNABLE_TIMEOUTNOXFER;
int TimeoutStalled = TUNABLE_TIMEOUTSTALLED;
char MultilineRFC2228 = 0;

/* from src/pool.c */
extern pool *global_config_pool;

/* Used by find_config_* */
xaset_t *find_config_top = NULL;

static void _mergedown(xaset_t*,int);

/* Used by get_param_int_next & get_param_ptr_next as "placeholders" */
static config_rec *_last_param_int = NULL;
static config_rec *_last_param_ptr = NULL;
static int _kludge_disable_umask = 0;

/* Used only while reading configuration files */

struct {
  pool *tpool;
  array_header *sstack,*cstack;
  server_rec **curserver;
  config_rec **curconfig;
} conf;

/* Imported this function from modules/mod_ls.c -- it belongs more with the
 * dir_* functions here, rather than the ls_* functions there.
 */

/* Return true if dir is ".", "./", "../", or "..".
 */
int is_dotdir(const char *dir) {
  if(!strcmp(dir, ".")  ||
     !strcmp(dir, "./") ||
     !strcmp(dir, "..") ||
     !strcmp(dir, "../"))
    return TRUE;

  return FALSE;
}

/* Determine whether the given path is to be treated as a "hidden" file.
 * Returns TRUE if so, FALSE otherwise.
 */
int dir_check_hidden(const char *path) {
  char *filename_start = NULL;

  /* NULL is never a hidden file
   */
  if (!path)
    return FALSE;

  /* if path is "." or "..", it's _not_ a hidden file
   */
  if (!strcmp(path, ".") || !strcmp(path, ".."))
    return FALSE;

  /* if the given path matches the current working directory, treat it
   * (the path) as if it was ".", and return FALSE.
   */
  if (!strcmp(path, fs_getcwd()))
    return FALSE;

  /* find the start of the filename portion by looking for the last
   * slash in the path.  If found, and the next character is a ".", it's
   * a hidden file.  If not found, and the first character of path is a
   * ".", it's a hidden file.  Otherwise, it's not a hidden file.
   */
  filename_start = rindex(path, '/');

  if (filename_start) {

    /* check to see if this pointer is pointing at the last character in 
     * the string.  If so, path can't be a hidden file, at least by nature
     * of it's name.  This _shouldn't_ happen, though, as intervening
     * functions strip off trailing slashes.  However, just to be thorough
     * cautious...
     */
    if (strlen(filename_start) < 1)
      return FALSE;

    else
      filename_start++;

    if (*filename_start == '.') {
      return TRUE;

    } else {

      /* no-op -- for now */
    }

  } else if (*path == '.')
    return TRUE;

  /* return FALSE by default */
  return FALSE;	
}

void kludge_disable_umask(void)
{
  _kludge_disable_umask = 1;
}

void kludge_enable_umask(void)
{
  _kludge_disable_umask = 0;
}

char *get_word(char **cp)
{
  char *ret,*dst;
  char quote_mode = 0;

  if(!cp || !*cp || !**cp)
    return NULL;

  while(**cp && isspace((UCHAR)**cp)) (*cp)++;

  if(!**cp)
    return NULL;

  ret = dst = *cp;
  
  if(**cp == '\"') {
    quote_mode++;
    (*cp)++;
  }

  while(**cp && (quote_mode ? (**cp != '\"') : !isspace((UCHAR)**cp))) {
    if(**cp == '\\' && quote_mode) {
      /* escaped char */
      if(*((*cp)+1))
        *dst = *(++(*cp));
    }

    *dst++ = **cp;
    ++(*cp);
  }

  if(**cp) (*cp)++;
  *dst = '\0';

  return ret;
}

cmd_rec *get_config_cmd(pool *ppool, FILE *fp, int *line)
{
  char buf[1024] = {'\0'}, *cp, *wrd;
  cmd_rec *newcmd;
  pool *newpool;
  array_header *tarr;
  int i;
  
  while(fgets(buf, sizeof(buf) - 1, fp)) {
    if(line != NULL)
      (*line)++;
    
    i = strlen(buf);
    if(i && buf[i - 1] == '\n')
      buf[i - 1] = '\0';
    
    for(cp = buf; *cp && isspace((UCHAR)*cp); cp++) ;

    if(*cp == '#' || !*cp)		/* Comment or blank line */
      continue;

    /* Build a new pool for the command structure and array */
    newpool = make_sub_pool(ppool);
    newcmd = (cmd_rec*)pcalloc(newpool,sizeof(cmd_rec));
    newcmd->pool = newpool;
    tarr = make_array(newpool,4,sizeof(char**));

    /* Add each word to the array */
    while((wrd = get_word(&cp)) != NULL) {
      char *tmp = pstrdup(newpool, wrd);
      
      *((char**)push_array(tarr)) = tmp; /* pstrdup(newpool,wrd); */
      newcmd->argc++;
    }

    *((char**)push_array(tarr)) = NULL;
    
    /* The array header's job is done, we can forget about it and
     * it will get purged when the command's pool is cleared
     */

    newcmd->argv = (char**)tarr->elts;

    /* Perform a fixup on configuration directives so that:
     * -argv[0]--  -argv[1]-- ----argv[2]-----
     * <Option     /etc/adir  /etc/anotherdir>
     *   .. becomes ..
     * -argv[0]--  -argv[1]-  ----argv[2]----
     * <Option>    /etc/adir  /etc/anotherdir
     */

    if(newcmd->argc && *(newcmd->argv[0]) == '<') {
      char *cp = newcmd->argv[newcmd->argc-1];

      if(*(cp + strlen(cp)-1) == '>' && newcmd->argc > 1) {
        if(!strcmp(cp,">")) {
          newcmd->argv[newcmd->argc-1] = NULL;
          newcmd->argc--;
        } else
          *(cp + strlen(cp)-1) = '\0';

        cp = newcmd->argv[0];
        if(*(cp + strlen(cp)-1) != '>')
          newcmd->argv[0] = pstrcat(newcmd->pool,cp,">",NULL);
      }
    }
        
    return newcmd;
  }

  return NULL;
}

void init_dyn_stacks(pool *p,config_rec *top)
{
  conf.sstack = make_array(p,1,sizeof(server_rec*));
  conf.curserver = (server_rec**)push_array(conf.sstack);
  *conf.curserver = main_server;
  conf.cstack = make_array(p,3,sizeof(config_rec*));
  conf.curconfig = (config_rec**)push_array(conf.cstack);
  *conf.curconfig = NULL;
  conf.curconfig = (config_rec**)push_array(conf.cstack);
  *conf.curconfig = top;
}

void init_conf_stacks(void)
{
  pool *pool = make_sub_pool(permanent_pool);

  conf.tpool = pool;
  conf.sstack = make_array(pool,1,sizeof(server_rec*));
  conf.curserver = (server_rec**)push_array(conf.sstack);
  *conf.curserver = main_server;
  conf.cstack = make_array(pool,10,sizeof(config_rec*));
  conf.curconfig = (config_rec**)push_array(conf.cstack);
  *conf.curconfig = NULL;
}

void free_dyn_stacks(void)
{
  memset(&conf, 0, sizeof(conf));
}

void free_conf_stacks(void)
{
  destroy_pool(conf.tpool);
  memset(&conf, 0, sizeof(conf));
}

/* Used by modules to start/end configuration sections */

server_rec *start_new_server(const char *addr)
{
  server_rec *s;
  pool *p;

  p = make_sub_pool(permanent_pool);

  s = (server_rec*)pcalloc(p,sizeof(server_rec));
  s->pool = p;
  s->config_type = CONF_VIRTUAL;
  
  /* Have to make sure it ends up on the end of the chain,
   * otherwise main_server becomes useless.
   */

  xaset_insert_end(servers,(xasetmember_t*)s);
  s->set = servers;
  if(addr)
    s->ServerAddress = pstrdup(s->pool,addr);

  /* default server port */
  s->ServerPort = inet_getservport(s->pool, "ftp", "tcp");

  conf.curserver = (server_rec**)push_array(conf.sstack);
  *conf.curserver = s;
  return s;
}

server_rec *end_new_server(void)
{
  if(!*conf.curserver)
    return NULL;

  if(conf.curserver == (server_rec**)conf.sstack->elts)
    return NULL; /* Disallow underflows */

  
  conf.curserver--;
  conf.sstack->nelts--;


  return *conf.curserver;
}

/* Starts a sub-configuration */
  
config_rec *start_sub_config(const char *name) {
  config_rec *c = NULL, *parent = *conf.curconfig;
  pool *c_pool = NULL, *parent_pool = NULL;
  xaset_t **set = NULL;

  if (parent) {
    parent_pool = parent->pool;
    set = &parent->subset;

  } else {
    parent_pool = (*conf.curserver)->pool;
    set = &(*conf.curserver)->conf;
  }

  /* allocate a sub-pool for this config_rec.  Note: special
   * exception for <Global> configs -- the parent pool is global_config_pool
   * (a pool just for this context), not the pool of the parent server.  This
   * keeps <Global> config recs from being freed prematurely, and helps to
   * avoid memory leaks.
   */
  if (!strcmp(name, "<Global>")) {
    if (!global_config_pool)
      global_config_pool = make_named_sub_pool(permanent_pool,
        "<Global> configs");
    parent_pool = global_config_pool;
  }

  c_pool = make_sub_pool(parent_pool);
  c = (config_rec *) pcalloc(c_pool, sizeof(config_rec));

  if (!*set)
    *set = xaset_create(c_pool, NULL);

  xaset_insert(*set, (xasetmember_t*)c);
  
  c->pool = c_pool;
  c->set = *set;
  c->parent = parent;

  if (name)
    c->name = pstrdup(c->pool, name);

  if (parent && (parent->config_type == CONF_DYNDIR))
    c->flags |= CF_DYNAMIC;

  /* Now insert another level onto the stack */
  if(!*conf.curconfig)
    *conf.curconfig = c;
  else {
    conf.curconfig = (config_rec**)push_array(conf.cstack);
    *conf.curconfig = c;
  }

  return c;
}

/* Pop one level off the stack */
config_rec *end_sub_config(void)
{
  if(conf.curconfig == (config_rec**)conf.cstack->elts) {
    if(*conf.curconfig)
      *conf.curconfig = NULL;
    return NULL;
  }

  conf.curconfig--;
  conf.cstack->nelts--;

  return *conf.curconfig;
}

/* Adds a config_rec to the specified set */
config_rec *add_config_set(xaset_t **set,const char *name)
{
  pool *conf_pool = NULL, *set_pool = NULL;
  config_rec *c,*parent = NULL;

  if(!*set) {

    /* allocate a subpool from permanent_pool for the set
     */
    set_pool = make_sub_pool(permanent_pool);
    *set = xaset_create(set_pool,NULL);
    (*set)->mempool = set_pool;
    
    /* now, make a subpool for the config_rec to be allocated
     */
    conf_pool = make_sub_pool(set_pool);

  } else {

    /* find the parent set for the config_rec to be allocated
     */
    if((*set)->xas_list)
      parent = ((config_rec*)((*set)->xas_list))->parent;

    /* allocate a subpool for the config_rec from the parent's pool
     */
    conf_pool = make_sub_pool((*set)->mempool);
  }

  c = (config_rec *) pcalloc(conf_pool, sizeof(config_rec));
  
  c->pool = conf_pool;
  c->set = *set;
  c->parent = parent;
  if(name)
    c->name = pstrdup(conf_pool, name);
  xaset_insert_end(*set,(xasetmember_t*)c);
  return c;
}

/* Adds a config_rec on the current "level" */
config_rec *add_config(const char *name) {
  server_rec *s = *conf.curserver;
  config_rec *parent = NULL, *c = *conf.curconfig;
  pool *p = NULL;
  xaset_t **set = NULL;

  if (c) {
    parent = c;
    p = c->pool;
    set = &c->subset;

  } else {
    parent = NULL;
    
    if(!s->conf || !s->conf->xas_list)
      p = make_sub_pool(s->pool);
    else
      p = ((config_rec*)s->conf->xas_list)->pool;

    set = &s->conf;
  }

  if (!*set)
    *set = xaset_create(p, NULL);

  c = add_config_set(set, name);
  c->parent = parent;

  return c;
}

array_header *parse_group_expression(pool *p, int *argc, char **argv)
{
  array_header *acl = NULL;
  int cnt = *argc;
  char *s,*ent;

  if(cnt) {
    acl = make_array(p,cnt,(sizeof(char*)));
    while(cnt-- && *(++argv)) {
      s = pstrdup(p,*argv);
      while((ent = get_token(&s,",")) != NULL)
        if(*ent)
          *((char**)push_array(acl)) = ent;
    }

    *argc = acl->nelts;
  } else
    *argc = 0;

  return acl;
}

array_header *parse_user_expression(pool *p, int *argc, char **argv)
{
  return parse_group_expression(p,argc,argv);
}

/* boolean "group-expression" matching, returns 1 if the expression
 * matches
 */

int group_expression(char **expr)
{
  int cnt,found;
  char *grp;

  for(; *expr; expr++) {
    grp = *expr;
    found = FALSE;

    if(*grp == '!') {
      found = !found;
      grp++;
    }

    if(session.group && strcmp(session.group,grp) == 0)
      found = !found;
    else if (session.groups)
      for(cnt = session.groups->nelts-1; cnt >= 0; cnt--)
        if(strcmp(*(((char**)session.groups->elts)+cnt),grp) == 0) {
          found = !found; break;
        }

    if(!found) {
      expr = NULL;
      break;
    } 
  }

  if(expr)
    return TRUE;

  return FALSE;
}
  
/* boolean "user-expression" matching, returns 1 if the expression
 * matches
 */

int user_expression(char **expr)
{
  int found;
  char *user;

  for(; *expr; expr++) {
    user = *expr;
    found = FALSE;

    if(*user == '!') {
      found = !found;
      user++;
    }

    if(strcmp(session.user,user) == 0)
      found = !found;

    if(!found) {
      expr = NULL;
      break;
    }
  }

  if(expr)
    return TRUE;

  return FALSE;
}

/* Per-directory configuration */

static int _strmatch(register char *s1, register char *s2)
{
  register int len = 0;

  while(*s1 && *s2 && *s1++ == *s2++)
    len++;

  return len;
}

static config_rec *_recur_match_path(pool *p,xaset_t *s, char *path)
{
  char *tmp_path = NULL;
  config_rec *c,*res;

  if(!s)
    return NULL;

  for(c = (config_rec*)s->xas_list; c; c=c->next)
    if(c->config_type == CONF_DIR) {
      tmp_path = c->name;

      if(c->argv[1]) {
        if(*(char*)(c->argv[1]) == '~')
          c->argv[1] = dir_canonical_path(c->pool,(char*)c->argv[1]);

        tmp_path = pdircat(p, (char *) c->argv[1], tmp_path, NULL);
      }

      /* exact path match
       */
      if(!strcmp(tmp_path, path))
        return c;

      if(!strstr(tmp_path,"/*")) {
        if(*tmp_path && *(tmp_path + (strlen(tmp_path)-1)) == '/') {
          *(tmp_path+(strlen(tmp_path)-1)) = '\0';

          if(!strcmp(tmp_path,path))
            return c;
        }
        tmp_path = pdircat(p, tmp_path, "*", NULL);
      }

      /* Temporary measure until we figure what's going on with
       * gnu fnmatch
       *
       * Hmm...wonder what this is, and if it's still an issue.  I love
       * cryptic comments in other people's code. :)
       *
       * - MacGyver
       */

#if 0
      if(pr_fnmatch(tmp_path, path, PR_FNM_PATHNAME) == 0) {
#else
      if(pr_fnmatch(tmp_path, path, 0) == 0) {
#endif
        if(c->subset) {
          res = _recur_match_path(p,c->subset,path);
          if(res)
            return res;
        }
        return c;
      }
    }

  return NULL;
}

config_rec *dir_match_path(pool *p, char *path)
{
  char *tmp;
  config_rec *res = NULL;

  tmp = pstrdup(p,path);
  if(*(tmp+strlen(tmp)-1) == '*')
    *(tmp+strlen(tmp)-1) = '\0';
  if(*(tmp+strlen(tmp)-1) == '/')
    *(tmp+strlen(tmp)-1) = '\0';

  if(session.anon_config) {
    res = _recur_match_path(p,session.anon_config->subset,tmp);
    if(!res) {

      if(session.anon_root && !strncmp(session.anon_root,tmp,
                                       strlen(session.anon_root)))
        return NULL;
    }
  }

  if(!res)
    res = _recur_match_path(p,main_server->conf,tmp);

/*
  if(!res)
    res = ((session.anon_config) ? session.anon_config : (config_rec*)main_server->conf->xas_list);
*/

  return res;
}

int dir_get_param(pool *pp,char *path,char *param)
{
  char *fullpath,*tmp;
  pool *p;
  config_rec *c;

  p = make_sub_pool(pp);

  if(*path != '/')
    fullpath = pstrcat(p,session.cwd,"/",path,NULL);
  else
    fullpath = path;

  if((tmp = dir_realpath(p,fullpath)) != NULL)
    fullpath = tmp;

  if(session.anon_root)
    fullpath = pdircat(p,session.anon_root,fullpath,NULL);

  c = dir_match_path(p,fullpath);

  destroy_pool(p);

  if(c)
    return get_param_int(c->subset,param,FALSE);
  return -1;
}

static int _dir_check_op(pool *p, xaset_t *c, int op, uid_t uid, gid_t gid,
    mode_t mode) {
  int i, res = 1, user_perms = 0;
  uid_t *u = NULL;
  gid_t *g = NULL, *gidp = NULL;

  if(!c)
    return 1;				/* Default is to allow */

  /* attempt to match the UID and GID of the file against that of the
   * current user and groups
   */
  if (uid == session.uid) {

    /* the UID of the file is that of the current user
     */
    user_perms |= (mode & S_IRWXU);

  } else if (gid == session.gid) {

    /* the primary GID of the file is that of the current user
     */
    user_perms |= (mode & S_IRWXG);

  } else {
    int found_gid_match = FALSE;
 
    /* loop through the user's auxiliary groups, checking if these
     * memberships match that of the file
     */
    for (i = session.gids->nelts, gidp = (gid_t *) session.gids->elts;
         i; i--, gidp++) {

      /* matched an auxiliary GID against the file GID
       */
      if (*gidp == gid) {
        found_gid_match = TRUE;
        user_perms |= (mode & S_IRWXG);
        break;
      }
    }

    /* no matching GIDs.  Assume the current user can read, as other,
     * by default.
     */
    if (!found_gid_match)
      user_perms |= (mode & S_IRWXO);
  }

  switch(op) {
  case OP_HIDE:
    u = (uid_t *) get_param_ptr(c, "HideUser", FALSE);

    while (u && *u != (uid_t) -1 && (*u != uid || *u == session.uid))
      u = (uid_t *) get_param_ptr_next("HideUser", FALSE);

    if (u && *u == uid) {
      res = 0;
      break;
    }

    g = (gid_t *) get_param_ptr(c, "HideGroup", FALSE);

    while (g && *g != (gid_t) -1 && (*g != gid || *g == session.gid))
      g = (gid_t *) get_param_ptr_next("HideGroup", FALSE);

    if (g && *g == gid) {
      res = 0;
      break;
    }

    if(get_param_int(c,"HideNoAccess",FALSE) == 1) {

      if(S_ISDIR(mode)) {

        /* check to see if the mode of this directory allows the
         * current user to list its contents
         */
        res = user_perms &= (S_IXUSR|S_IXGRP|S_IXOTH);
     
      } else {

        /* check to see if the mode of this file allows the current
         * user to read it.  The below expression is fairly compact,
         * but achieves its goal, which is:
         *
         * If the file is readable (by user, group, or other)
         *   return > 1 (the user_perms work for this)
         *
         * If the file is unreadable
         *   return 0 (which user_perms will be)
         */
        res = user_perms &= (S_IRUSR|S_IRGRP|S_IROTH);
      }
    }
    break;

  case OP_COMMAND:
    if(get_param_int(c,"AllowAll",FALSE) == 1)
      /* nop */;  

    else if(get_param_int(c,"DenyAll",FALSE) == 1) {
      res = 0;
      errno = EACCES;
    }
    break;
  }

  return res;
}

int dir_check_op_mode(pool *p,char *path,int op,
                       uid_t uid,gid_t gid,mode_t mode)
{
  char *fullpath;
  xaset_t *c;
  config_rec *sc;
  int res;

  if(*path != '/')
    fullpath = pdircat(p,session.cwd,path,NULL);
  else
    fullpath = path;

  if(session.anon_root)
    fullpath = pdircat(p,session.anon_root,fullpath,NULL);
  
  c = CURRENT_CONF;
  sc = _recur_match_path(p,c,fullpath);

  if(sc)
    res = _dir_check_op(p,sc->subset,op,uid,gid,mode);
  else
    res = _dir_check_op(p,c,op,uid,gid,mode);

  return res;  
}

int dir_check_op(pool *pp,char *path,int op)
{
  struct stat sbuf;

  if(fs_stat(path,&sbuf) == -1)
    return 1;

  return dir_check_op_mode(pp,path,op,sbuf.st_uid,sbuf.st_gid,sbuf.st_mode);
}

static int _check_user_access(xaset_t *conf, char *name)
{
  int ret = 0;
  config_rec *c = find_config(conf,CONF_PARAM,name,FALSE);

  while(c) {
    ret = user_expression((char**)c->argv);

    if(ret)
      break;

    c = find_config_next(c,c->next,CONF_PARAM,name,FALSE);
  }

  return ret;
}

static int _check_group_access(xaset_t *conf, char *name)
{
  int	ret = 0;
  config_rec *c = find_config(conf,CONF_PARAM,name,FALSE);

  while(c) {
    ret = group_expression((char**)c->argv);

    if(ret)
      break;

    c = find_config_next(c,c->next,CONF_PARAM,name,FALSE);
  }

  return ret;
}

/* returns 1 if explicit match
 * returns -1 if explicit mismatch (i.e. "NONE")
 * returns 0 if no match
 */

int match_ip(p_in_addr_t *addr, char *name, const char *match)
{
  char buf[1024];
  char *mask,*cp;
  int cidr_mode = 0, cidr_bits;
  p_in_addr_t cidr_addr;
  u_int_32 cidr_mask = 0;

  if(!strcasecmp(match,"ALL"))
    return 1;

  if(!strcasecmp(match,"NONE"))
    return -1;

  memset(buf,0,sizeof(buf));
  mask = buf;

  if(*match == '.') {
    *mask++ = '*';
    *mask = '\0';
    sstrcat(buf, match, sizeof(buf));
  } else if(*(match + strlen(match) - 1) == '.') {
    sstrcat(buf, match, sizeof(buf));
    sstrcat(buf, "*", sizeof(buf));
  } else if((cp = strchr(match,'/')) != NULL) { /* check for CIDR notation */
    /* first portion of CIDR should be dotted quad, second portion
     * is netmask
     */
    sstrncpy(buf, match, (cp-match)+1 <= sizeof(buf) ?
                         (cp-match)+1 :  sizeof(buf));    
    cidr_bits = atoi(cp+1);
    
    if(cidr_bits > 0 && cidr_bits < 33) {
      int shift = 32 - cidr_bits;
      
      cidr_mode = 1;
      while(cidr_bits--)
	cidr_mask = (cidr_mask << 1) | 1;
      cidr_mask = cidr_mask << shift;
#ifdef HAVE_INET_ATON
      if(inet_aton(mask,&cidr_addr) == 0)
	return 0;
#else
      cidr_addr.s_addr = inet_addr(mask);
#endif
      cidr_addr.s_addr &= htonl(cidr_mask);
    } else {
      return 0;
    }
  } else {
    sstrcat(buf, match, sizeof(buf));
  }
  
  if(cidr_mode) {
    if((addr->s_addr & htonl(cidr_mask)) == cidr_addr.s_addr)
      return 1;
  } else {
    if(pr_fnmatch(buf, name, PR_FNM_NOESCAPE | PR_FNM_CASEFOLD) == 0 ||
       pr_fnmatch(buf, inet_ntoa(*addr),
		  PR_FNM_NOESCAPE | PR_FNM_CASEFOLD) == 0)
      return 1;
  }
  
  return 0;
}

/* As of 1.2.0rc3, a '!' character in front of the IP address
 * negates the logic (i.e. doesn't match).
 *
 * Here are our rules for matching an IP/host list:
 *
 * (negate-cond-1 && negate-cond-2 && ... negate-cond-n) &&
 * (cond-1 || cond-2 || ... cond-n)
 *
 * This boils down to the following two rules:
 *
 * 1. ALL negative ('!') conditions must evaluate to
 * logically TRUE.
 *
 * .. and ..
 * 
 * 2. One (or more) normal conditions must evaluate to
 * logically TRUE.
 */

/* Check an ACL for negative (!) rules and make sure all of them evaluate to
 * TRUE.  Default (if none exist) is TRUE.
 */
static int _check_ip_negative(const config_rec *c)
{
  char *arg,**argv;
  int argc;
  
  for(argc = c->argc, argv = (char**)c->argv; argc; argc--, argv++) {
    arg = *argv;
    if(*arg != '!')
      continue;

    arg++;
    switch(match_ip(session.c->remote_ipaddr,session.c->remote_name,arg)) {
      case 1:
        /* This actually means we DIDN'T match, and it's ok to short circuit
         * everything (negative)
         */
        return FALSE;

      case -1:
        /* -1 signifies a NONE match, which isn't valid for negative
         * conditions.
         */
        log_pri(LOG_ERR,"ooops, it looks like !NONE was used in an ACL somehow.");
        return FALSE;

      default:
        /* This means our match is actually true and we can continue */
        break;
    }
  }
 
  /* If we got this far either all conditions were TRUE or there were no
   * conditions.
   */
  
  return TRUE;
}

/* Check an ACL for positive conditions, short-circuiting if ANY of them are
 * TRUE.  Default return is FALSE.
 */
static int _check_ip_positive(const config_rec *c)
{
  char *arg,**argv;
  int argc;

  for(argc = c->argc, argv = (char**)c->argv; argc; argc--, argv++) {
    arg = *argv;
    if(*arg == '!')
      continue;

    switch(match_ip(session.c->remote_ipaddr,session.c->remote_name,arg)) {
      case 1:
        /* Found it! */
        return TRUE;
        
      case -1:
        /* Special value "NONE", meaning nothing can match, so we can
         * short-circuit on this as well.
         */
        return FALSE;
        
      default:
        /* No match, keep trying */
        break;
    }
  }

  /* default return value is FALSE */
  return FALSE;
}

static int _check_ip_access(xaset_t *conf, char *name)
{
  int res = FALSE;
  
  config_rec *c = find_config(conf,CONF_PARAM,name,FALSE);

  while(c) {
    /* If the negative check failed (default is success), short-circuit and
     * return FALSE
     */
    if(_check_ip_negative(c) != TRUE)
      return FALSE;
    
    /* Otherwise, continue on with boolean or check */
    if(_check_ip_positive(c) == TRUE)
      res = TRUE;
    
    /* Continue on, in case there are other acls that need to be checked 
     * (multiple acls are logically OR'd)
     */
    c = find_config_next(c,c->next,CONF_PARAM,name,FALSE);
  }

  return res;
}

/* 1 if allowed, 0 otherwise */

static int _check_limit_allow(config_rec *c)
{
  /* if session.groups is null, this means no authentication
   * attempt has been made, so we simply check for the
   * very existance of an AllowGroup, and assume (for now) it's
   * allowed.  This works because later calls to _check_limit_allow
   * WILL have filled in the group members and we can truely check
   * group membership at that time.  Same goes for AllowUser.
   */

  if(!session.user) {
    if(find_config(c->subset,CONF_PARAM,"AllowUser", FALSE))
      return 1;
  } else if(_check_user_access(c->subset,"AllowUser"))
    return 1;

  if(!session.groups) {
    if(find_config(c->subset,CONF_PARAM,"AllowGroup",FALSE))
      return 1;
  } else if(_check_group_access(c->subset,"AllowGroup"))
    return 1;
  if(_check_ip_access(c->subset,"Allow"))
    return 1;
  if(get_param_int(c->subset,"AllowAll",FALSE) == 1)
    return 1;

  return 0;
}

static int _check_limit_deny(config_rec *c)
{
  if(get_param_int(c->subset,"DenyAll",FALSE) == 1)
    return 1;
  if(session.user && _check_user_access(c->subset,"DenyUser"))
    return 1;
  if(session.groups && _check_group_access(c->subset,"DenyGroup"))
    return 1;
  if(_check_ip_access(c->subset,"Deny"))
    return 1;
  return 0;
}

/* _check_limit returns 1 if allowed, 0 if implicitly allowed,
 * and -1 if implicitly denied and -2 if explicitly denied.
 */
   
static int _check_limit(config_rec *c)
{
  int order;

  if((order = get_param_int(c->subset,"Order",FALSE)) == -1)
    order = ORDER_ALLOWDENY;

  if(order == ORDER_DENYALLOW) {
    /* check deny first */
    if(_check_limit_deny(c))
      return -2;		/* explicit deny */
    if(_check_limit_allow(c))
      return 1;			/* explicit allow */

    return -1;    		/* implicit deny */
  }

  /* check allow first */
  if(_check_limit_allow(c))
    return 1;			/* explicit allow */
  if(_check_limit_deny(c))
    return -2;			/* explicit deny */

  return 0;			/* implicit allow */
}

/* Note: if and == 1, the logic is short circuited so that the first
 * failure results in a FALSE return from the entire function, if and
 * == 0, an ORing operation is assumed and the function will return
 * TRUE if any <limit LOGIN> allows access.
 */

int login_check_limits(xaset_t *conf, int recurse, 
                       int and, int *found)
{
  int res = and;
  int rfound;
  config_rec *c;
  int argc;
  char **argv;

  *found = 0;

  if(!conf || !conf->xas_list)
    return TRUE;			/* default is to allow */

  /* First check top level */
  for(c = (config_rec*)conf->xas_list; c; c=c->next)
    if(c->config_type == CONF_LIMIT) {
      for(argc = c->argc, argv = (char**)c->argv; argc; argc--, argv++)
        if(!strcasecmp("LOGIN",*argv))
          break;

      if(argc) {
        if(and) {
          switch(_check_limit(c)) {
          case 1: res = (res && TRUE); (*found)++; break;
	  case -1:
          case -2: res = (res && FALSE); (*found)++; break;
          }
          if(!res)
            break;
        } else
          switch(_check_limit(c)) {
          case 1: res = TRUE;
	  case -1:
          case -2: (*found)++; break;
          }
      }
    }

  if( ((res && and) || (!res && !and && *found)) && recurse ) {
    for(c = (config_rec*)conf->xas_list; c; c=c->next)
      if(c->config_type == CONF_ANON && c->subset && c->subset->xas_list) {
       if(and) {
         res = (res && login_check_limits(c->subset,recurse,and,&rfound));
         (*found) += rfound;
         if(!res)
           break;
       } else {
         int rres;

         rres = login_check_limits(c->subset,recurse,and,&rfound);
         if(rfound)
           res = (res || rres);
         (*found) += rfound;
         if(res)
           break;
       }
     }
  }

  if(!*found && !and)
    return TRUE;			/* Default is to allow */
  return res;
}

static
int _check_limits(xaset_t *set, char *cmd, int hidden)
{
  /* Check limit directives */
  int res = 1,ignore_hidden = -1;
  config_rec *lc = NULL;
  register int i;

  errno = 0;
  
  if(!set)
    return res;
  
  for(lc = (config_rec*)set->xas_list;
      lc && (res == 1); lc = lc->next) {

    if(lc->config_type == CONF_LIMIT) {
      for(i = 0; i < lc->argc; i++) {
        if(!strcasecmp(cmd,(char*)(lc->argv[i])))
          break;
      }
	  
/*
      log_debug(DEBUG5,"cmd=%s i=%d lc->argc=%d lc->argv[0]=%s\n",
                   cmd, i, lc->argc, (char*)(lc->argv[i]));
*/

      if(i == lc->argc)
        continue;
	  
      /* Found a limit directive associated with the current
       * command.  ignore_hidden defaults to -1, if an explicit
       * IgnoreHidden off is seen, it is set to 0 and the check will not
       * be done again up the chain.  If an explicit IgnoreHidden on is
       * seen, checking short-circuits and we set ENOENT.
       */
     
      if (hidden && ignore_hidden == -1) {
          ignore_hidden = get_param_int(lc->subset,"IgnoreHidden",FALSE);
        if(ignore_hidden == 1) {
          res = 0;
          errno = ENOENT;
          break;
        }
      }

      switch(_check_limit(lc)) {
      case 1:
        res++;
        break;
	    
      case -1:
      case -2:
        res = 0;
        break;
	    
      default:
        continue;
      }
    }
  }
  
  if(!res && !errno)
    errno = EACCES;
  
  return res;
}

int dir_check_limits(config_rec *c, char *cmd, int hidden)
{
  int res = 1;
  
  for(; c && (res == 1); c = c->parent)
    res = _check_limits(c->subset, cmd, hidden);
      
  if(!c && (res == 1))
    /* vhost or main server has been reached without an explicit permit or deny,
     * so try the current server.
     */
    res = _check_limits(main_server->conf, cmd, hidden);

  return res;
}
    
void build_dyn_config(pool *p,char *_path, struct stat *_sbuf, int recurse)
{
  char *fullpath,*path,*dynpath,*cp;
  struct stat sbuf;
  config_rec *d;
  FILE *fp;
  cmd_rec *cmd;
  xaset_t **set = NULL;
  int isfile, line = 0, removed = 0;

  /* Switch through each directory, from "deepest" up looking for
   * new or updated .ftpaccess files
   */

  if(!_path)
    return;

  path = pstrdup(p,_path);

  memcpy(&sbuf,_sbuf,sizeof(sbuf));

  if(S_ISDIR(sbuf.st_mode))
    dynpath = pdircat(p,path,"/.ftpaccess",NULL);
  else
    dynpath = NULL;

  while(path) {
    if(session.anon_root) {
      fullpath = pdircat(p,session.anon_root,path,NULL);

      if(strcmp(fullpath,"/") && *(fullpath + strlen(fullpath) - 1) == '/')
        *(fullpath + strlen(fullpath) - 1) = '\0';
    } else
      fullpath = path;

    if(dynpath)
      isfile = fs_stat(dynpath,&sbuf);
    else
      isfile = -1;

    d = dir_match_path(p,fullpath);

    if(!d && isfile != -1) {
      set = (session.anon_config ? &session.anon_config->subset :
             &main_server->conf);
      d = add_config_set(set,fullpath);
      d->config_type = CONF_DIR;
      d->argc = 1;
      d->argv = pcalloc(d->pool,2*sizeof(void*));
    } else if(d) {
      config_rec *newd,*dnext;

      if(isfile != -1 && strcmp(d->name,fullpath) != 0) {
        set = &d->subset;
        newd = add_config_set(set,fullpath);
        newd->config_type = CONF_DIR;
        newd->argc = 1;
        newd->argv = pcalloc(newd->pool,2*sizeof(void*));
	newd->parent = d;
        d = newd;
      } else if(strcmp(d->name,fullpath) == 0 && 
                (isfile == -1 || sbuf.st_mtime > (time_t)d->argv[0])) {
        set = (d->parent ? &d->parent->subset :
               &main_server->conf);

	if (d->subset && d->subset->xas_list) {
       	  /* remove all old dynamic entries */
          for(newd = (config_rec*)d->subset->xas_list; newd; newd=dnext) {
	    dnext = newd->next;

            if(newd->flags & CF_DYNAMIC) {
              xaset_remove(d->subset,(xasetmember_t*)newd);
              removed++;
            }
          }
	}

        if(d->subset && !d->subset->xas_list) {
          destroy_pool(d->subset->mempool);
          d->subset = NULL;
          d->argv[0] = NULL;

	  /* If the file has been removed and no entries exist in this
           * dynamic entry, remove it completely
           */

          if(isfile == -1)
            xaset_remove(*set,(xasetmember_t*)d);
        }
      }
    }

    if(isfile != -1 && d && sbuf.st_mtime > (time_t)d->argv[0]) {
      /* File has been modified or not loaded yet */
      d->argv[0] = (void*)sbuf.st_mtime;

      fp = fopen(dynpath,"r");
      if(fp) {
        removed = 0;

        init_dyn_stacks(p,d);
        d->config_type = CONF_DYNDIR;

        while((cmd = get_config_cmd(p, fp, &line)) != NULL) {
          if(cmd->argc) {
            conftable *c;
            char found = 0;
            modret_t *mr;

            cmd->server = *conf.curserver;
            cmd->config = *conf.curconfig;
              
            for(c = m_conftable; c->directive; c++) {
              if(!strcasecmp(c->directive, cmd->argv[0])) {
                found++;

                if((mr = call_module(c->m,c->handler,cmd)) != NULL) {
                  if(MODRET_ERRMSG(mr)) {
                    log_pri(LOG_WARNING, "warning: %s",MODRET_ERRMSG(mr));
		  }
                }

		if(MODRET_ISDECLINED(mr))
			found--;

		destroy_pool(cmd->tmp_pool);
              }
            }

            if(!found)
              log_pri(LOG_WARNING,
                "warning: unknown configuration directive '%s' on "
                "line %d of '%s'.", cmd->argv[0], line, dynpath);

          }
 
          destroy_pool(cmd->pool);
        }

	log_debug(DEBUG5,"dynamic configuration added/updated for %s.",
                         fullpath);

        d->config_type = CONF_DIR;
        free_dyn_stacks();

        _mergedown(*set,TRUE);
        fclose(fp);
      }
    }

    if(isfile == -1 && removed && d && set) {
      log_debug(DEBUG5,"dynamic configuration removed for %s.",
                       fullpath);
      _mergedown(*set,FALSE);
    }

    if(!recurse)
      break;

    cp = rindex(path,'/');
    if(cp && strcmp(path,"/") != 0)
      *cp = '\0';
    else
      path = NULL;
    if(path) {
      if(*(path+strlen(path)-1) == '*')
        *(path+strlen(path)-1) = '\0';
      dynpath = pdircat(p,path,"/.ftpaccess",NULL);
    }
  }
}

/* dir_check_full() fully recurses the path passed
 * returns 1 if operation is allowed on current path,
 * or 0 if not.
 */

/* dir_check_full() and dir_check() both take a `hidden' argument which is a
 * pointer to an integer. This is provided so that they can tell the calling
 * function if an entry should be hidden or not.  This is used by mod_ls to
 * determine if a file should be displayed.  Note that in this context, hidden
 * means "hidden by configuration" (HideUser, etc), NOT "hidden because it's a
 * .dotfile".  -jss 2/22/01
 */

int dir_check_full(pool *pp, char *cmd, char *group, char *path, int *hidden)
{
  char *fullpath, *owner;
  config_rec *c;
  struct stat sbuf;
  pool *p;
  mode_t _umask = (mode_t) -1;
  int res = 1, isfile;
  int op_hidden = FALSE;

  p = make_sub_pool(pp);

  /* flood -- this is no longer needed, as all paths passed to 
   * dir_check should have gone through either dir_canonical or
   * dir_real first (depending on if they are supposed to pre-exist

  fullpath = dir_realpath(p,path);

  if(!fullpath)
    fullpath = pdircat(p,session.cwd,path,NULL);
  else 
    path = fullpath;
  */

  fullpath = path;

  if(session.anon_root)
    fullpath = pdircat(p, session.anon_root, fullpath, NULL);

  log_debug(DEBUG5, "in dir_check_full(): path = '%s', fullpath = '%s'.",
            path, fullpath);

  /* Check and build all appropriate dynamic configuration entries */
  if((isfile = fs_stat(path, &sbuf)) == -1)
    memset(&sbuf,0,sizeof(sbuf));
  
  build_dyn_config(p,path,&sbuf,1);
  
  session.dir_config = c = dir_match_path(p,fullpath);

  if(!c && session.anon_config)
    c = session.anon_config;

  if(!_kludge_disable_umask) {
    /* Check for a directory Umask.
     */
    if(S_ISDIR(sbuf.st_mode) ||
		!strcasecmp(cmd, C_MKD) ||
        !strcasecmp(cmd, C_XMKD)) {
      mode_t *dir_umask = (mode_t *) get_param_ptr(CURRENT_CONF, "DirUmask",
        FALSE);

      if (dir_umask == NULL)
        _umask = (mode_t) -1;
      else
        _umask = *dir_umask;
    }
    
    /* It's either a file, or we had no directory Umask.
     */
    if (_umask == (mode_t) -1) {
      mode_t *file_umask = (mode_t *) get_param_ptr(CURRENT_CONF, "Umask",
        FALSE);

      if (file_umask == NULL)
        _umask = (mode_t) 0022;
      else
        _umask = *file_umask;
    }
  }
  
  session.fsuid = (uid_t) -1;
  session.fsgid = (gid_t) -1;

  if((owner = get_param_ptr(CURRENT_CONF,"UserOwner",FALSE)) != NULL) {
    /* attempt chown on all new files */
    struct passwd *pw;

    if((pw = auth_getpwnam(p,owner)) != NULL)
      session.fsuid = pw->pw_uid;
  }
  
  if((owner = get_param_ptr(CURRENT_CONF,"GroupOwner",FALSE)) != NULL) {
    /* attempt chgrp on all new files */
    struct group *gr;

    if((gr = auth_getgrnam(p,owner)) != NULL)
      session.fsgid = gr->gr_gid;
  }
  
  if(isfile != -1) {

    /* Check to see if the current config "hides" the path or not
     */
    
    op_hidden = !_dir_check_op(p,CURRENT_CONF,OP_HIDE,sbuf.st_uid,sbuf.st_gid,
                                 sbuf.st_mode);

    res = _dir_check_op(p,CURRENT_CONF,OP_COMMAND,sbuf.st_uid,sbuf.st_gid,
      sbuf.st_mode);
  }

  if(res) {

    /* Note that dir_check_limits() also handles IgnoreHidden.  If it is set,
     * these return 0 (no access), and also set errno to ENOENT so it looks
     * like the file doesn't exist.
     */
    res = dir_check_limits(c, cmd, op_hidden);

    /* If specifically allowed, res will be > 1 and we don't want to
     * check the command group limit
     */
    if(res == 1 && group)
      res = dir_check_limits(c, group, op_hidden);

    /* if still == 1, no explicit allow so check lowest priority "ALL" group */
    if(res == 1)
      res = dir_check_limits(c, "ALL",op_hidden);
  }

  if (res && _umask != (mode_t) -1)
    log_debug(DEBUG5,"in dir_check_full(): setting umask to %04o (was %04o)",
        (unsigned int)_umask,(unsigned int)umask(_umask));

  destroy_pool(p);

  if(hidden)
    *hidden = op_hidden;

  return res;
}

/* dir_check() checks the current dir configuration against the path,
 * if it matches (partially), a search is done only in the subconfig,
 * otherwise handed off to dir_check_full
 */

int dir_check(pool *pp, char *cmd, char *group, char *path, int *hidden)
{
  char *fullpath, *owner;
  config_rec *c;
  struct stat sbuf;
  pool *p;
  mode_t _umask = (mode_t) -1;
  int res = 1, isfile;
  int op_hidden = FALSE;

  p = make_sub_pool(pp);

  fullpath = path;

  if(session.anon_root)
    fullpath = pdircat(p,session.anon_root,fullpath,NULL);

  c = (session.dir_config ? session.dir_config :
        (session.anon_config ? session.anon_config : NULL));

  if(!c || strncmp(c->name,fullpath,strlen(c->name)) != 0) {
    destroy_pool(p);
    return dir_check_full(pp,cmd,group,path,hidden);
  }

  /* Check and build all appropriate dynamic configuration entries */
  if((isfile = fs_stat(path, &sbuf)) == -1)
    memset(&sbuf,0,sizeof(sbuf));

  build_dyn_config(p, path, &sbuf, 0);

  session.dir_config = c = dir_match_path(p, fullpath);

  if(!c && session.anon_config)
    c = session.anon_config;

  if(!_kludge_disable_umask) {
    /* Check for a directory Umask.
     */
    if(S_ISDIR(sbuf.st_mode) ||
		!strcasecmp(cmd, C_MKD) ||
        !strcasecmp(cmd, C_XMKD)) {
      mode_t *dir_umask = (mode_t *) get_param_ptr(CURRENT_CONF, "DirUmask",
        FALSE);

      if (dir_umask == NULL)
        _umask = (mode_t) -1;
      else
        _umask = *dir_umask;
    }
    
    /* It's either a file, or we had no directory Umask.
     */
    if (_umask == (mode_t) -1) {
      mode_t *file_umask = (mode_t *) get_param_ptr(CURRENT_CONF, "Umask",
        FALSE);

      if (file_umask == NULL)
        _umask = (mode_t) 0022;
      else
        _umask = *file_umask;
    }
  }

  session.fsuid = (uid_t) -1;
  session.fsgid = (gid_t) -1;

  if((owner = get_param_ptr(CURRENT_CONF,"UserOwner",FALSE)) != NULL) {
    /* attempt chown on all new files */
    struct passwd *pw;

    if((pw = auth_getpwnam(p,owner)) != NULL)
      session.fsuid = pw->pw_uid;
  }
  
  if((owner = get_param_ptr(CURRENT_CONF,"GroupOwner",FALSE)) != NULL) {
    /* attempt chgrp on all new files */
    struct group *gr;

    if((gr = auth_getgrnam(p,owner)) != NULL)
      session.fsgid = gr->gr_gid;
  }
  
  if(isfile != -1) {

    /* if not already marked as hidden by its name, check to see if the path
     * is to be hidden by nature of its mode
     */
    op_hidden = !_dir_check_op(p, CURRENT_CONF, OP_HIDE, sbuf.st_uid,
    			         sbuf.st_gid, sbuf.st_mode);

    res = _dir_check_op(p, CURRENT_CONF, OP_COMMAND, sbuf.st_uid, sbuf.st_gid,
			sbuf.st_mode);
  }
  
  if(res) {
    res = dir_check_limits(c, cmd, op_hidden);

    /* If specifically allowed, res will be > 1 and we don't want to
     * check the command group limit
     */

    if(res == 1 && group)
      res = dir_check_limits(c, group, op_hidden);
    
    /* if still == 1, no explicit allow so check lowest priority "ALL" group */
    if(res == 1)
      res = dir_check_limits(c, "ALL", op_hidden);
  }

  if (res && _umask != (mode_t) -1)
    log_debug(DEBUG5,"in dir_check(): setting umask to %04o (was %04o)",
        (unsigned int)_umask,(unsigned int)umask(_umask));

  destroy_pool(p);

  if(hidden)
    *hidden = op_hidden;
  return res;
}

/* dir_check_canon() canonocalizes as much of the path as possible
 * (which may not be all of it, as the target may not yet exist
 * then we hand off to dir_check()
 */

int dir_check_canon(pool *pp, char *cmd, char *group, char *path, int *hidden)
{
  return dir_check(pp,cmd,group,dir_best_path(pp,path),hidden);
}

/*
 * Move all the members (i.e. a "branch") of one config set to
 * a different parent.
 */

static void _reparent_all(config_rec *newparent,xaset_t *set)
{
  config_rec *c,*cnext;

  if(!newparent->subset)
    newparent->subset = xaset_create(newparent->pool,NULL);

  for(c = (config_rec*)set->xas_list; c; c = cnext) {
    cnext = c->next;
    xaset_remove(set,(xasetmember_t*)c);
    xaset_insert(newparent->subset,(xasetmember_t*)c);
    c->set = newparent->subset;
    c->parent = newparent;
  }
}

/* Recursively find the most appropriate place to move a CONF_DIR
 * directive to.
 */

static config_rec *_find_best_dir(xaset_t *set,char *path,int *matchlen)
{
  config_rec *c,*res = NULL,*rres;
  int len,imatchlen,tmatchlen;

  *matchlen = 0;

  if(!set || !set->xas_list)
    return NULL;

  for(c = (config_rec*)set->xas_list; c; c=c->next) {
    if(c->config_type == CONF_DIR) {
      if(!strcmp(c->name,path))
        continue;				/* Don't examine the current */
      len = strlen(c->name);
      while(len > 0 && (*(c->name+len-1) == '*' ||
                        *(c->name+len-1) == '/'))
        len--;

      /*
       * Just a partial match on the pathname does not mean that the longer
       * path is the subdirectory of the other -- they might just be sharing
       * the last path component! 
       * /var/www/.1
       * /var/www/.14
       *            ^ -- not /, not subdir
       * /var/www/.1
       * /var/www/.1/images
       *            ^ -- /, is subdir
       */
      if (strlen(path) > len && path[len] != '/')
          continue;

      if(!strncmp(c->name,path,len) &&
         len < strlen(path)) {
           rres = _find_best_dir(c->subset,path,&imatchlen);
           tmatchlen = _strmatch(path,c->name);
           if(!rres && tmatchlen > *matchlen) {
             res = c;
             *matchlen = tmatchlen;
           } else if(imatchlen > *matchlen) {
             res = rres;
             *matchlen = imatchlen;
           }     
         }
    }
  }

  return res;
}

/* Reorder all the CONF_DIR configuration sections, so that they are
 * in directory tree order
 */

static void _reorder_dirs(xaset_t *set, int mask)
{
  config_rec *c,*cnext,*newparent;
  int tmp,defer = 0;

  if(!set || !set->xas_list)
    return;

  if(!(mask & CF_DEFER))
    defer = 1;

  for(c = (config_rec*)set->xas_list; c; c=cnext) {
    cnext = c->next;

    if(c->config_type == CONF_DIR) {
      if(mask && !(c->flags & mask))
        continue;

      if(defer && (c->flags & CF_DEFER))
        continue;

      /* If <Directory *> is used inside <Anonymous>, move all
       * the directives from '*' into the higher level
       */
      if(!strcmp(c->name,"*") && c->parent &&
         c->parent->config_type == CONF_ANON) {
        if(c->subset)
          _reparent_all(c->parent,c->subset);
        xaset_remove(c->parent->subset,(xasetmember_t*)c);
      } else {
        newparent = _find_best_dir(set,c->name,&tmp);
        if(newparent) {
          if(!newparent->subset)
            newparent->subset = xaset_create(newparent->pool,NULL);

          xaset_remove(c->set,(xasetmember_t*)c);
          xaset_insert(newparent->subset,(xasetmember_t*)c);
          c->set = newparent->subset;
          c->parent = newparent;
        }
      }
    }
  }

  /* Top level is now sorted, now we recursively sort all the sublevels
   */
  for(c = (config_rec*)set->xas_list; c; c=c->next)
    if(c->config_type == CONF_DIR || c->config_type == CONF_ANON)
      _reorder_dirs(c->subset,mask);
}

void debug_dump_config(xaset_t *s,char *indent)
{
  config_rec *c;

  if(!indent)
    indent = "";

  for(c = (config_rec*)s->xas_list; c; c=c->next) {
    log_debug(DEBUG5,"%s%s",indent,c->name);
    if(c->subset)
      debug_dump_config(c->subset,pstrcat(permanent_pool,indent," ",NULL));
  }
}

static void _mergedown(xaset_t *s,int dynamic)
{
  config_rec *c,*dest,*newconf;
  int argc;
  void **argv,**sargv;
  
  if(!s || !s->xas_list)
    return;

  for (c = (config_rec*)s->xas_list; c; c=c->next)
    if ((c->flags & CF_MERGEDOWN) ||
        (c->flags & CF_MERGEDOWN_MULTI))
      for(dest = (config_rec*)s->xas_list; dest; dest=dest->next)
        if(dest->config_type == CONF_ANON ||
           dest->config_type == CONF_DIR) {

          /* If an option of the same name/type is found in the
           * next level down, it overrides, so we don't merge.
           */
          if ((c->flags & CF_MERGEDOWN) &&
              find_config(dest->subset, c->config_type, c->name, FALSE))
            continue;

          if(!dest->subset)
            dest->subset = xaset_create(dest->pool,NULL);

          newconf = add_config_set(&dest->subset,c->name);
          newconf->config_type = c->config_type;
          newconf->flags = c->flags | (dynamic ? CF_DYNAMIC : 0);
          newconf->argc = c->argc;
          newconf->argv = palloc(newconf->pool,(c->argc+1)*sizeof(void*));
          argv = newconf->argv; sargv = c->argv;
          argc = newconf->argc;
          while(argc--)
            *argv++ = *sargv++;
          *argv++ = NULL;
        }
          
  /* Top level merged, recursively merge lower levels */
  for(c = (config_rec*)s->xas_list; c; c=c->next)
    if(c->subset && (c->config_type == CONF_ANON ||
                     c->config_type == CONF_DIR))
      _mergedown(c->subset,dynamic);
}

/* iterate through <Directory> blocks inside of anonymous and
 * resolve each one.
 */

void resolve_anonymous_dirs(xaset_t *clist)
{
  config_rec *c;
  char *realdir;

  if(!clist)
    return;

  for(c = (config_rec*)clist->xas_list; c; c=c->next) {
    if(c->config_type == CONF_DIR) {
      if(c->argv[1]) {
        realdir = dir_best_path(c->pool,c->argv[1]);
        if(realdir)
          c->argv[1] = realdir;
        else {
          realdir = dir_canonical_path(c->pool,c->argv[1]);
          if(realdir)
            c->argv[1] = realdir;
        }
      }

      if(c->subset)
        resolve_anonymous_dirs(c->subset);
    }
  }
}

/* iterate through directory configuration items and resolve
 * ~ references
 */

void resolve_defered_dirs(server_rec *s)
{
  config_rec *c;
  char *realdir;

  if(!s || !s->conf)
    return;

  for(c = (config_rec*)s->conf->xas_list; c; c=c->next) {
    if(c->config_type == CONF_DIR && (c->flags & CF_DEFER)) {
      realdir = dir_best_path(c->pool,c->name);
      if(realdir)
        c->name = realdir;
      else {
        realdir = dir_canonical_path(c->pool,c->name);
        if(realdir)
          c->name = realdir;
      }
    }
  }
}

static
void _copy_recur(xaset_t **set, pool *p, config_rec *c, config_rec *new_parent)
{
  config_rec *newconf;
  int argc;
  void **argv,**sargv;

  if(!*set)
    *set = xaset_create(p,NULL);

  newconf = add_config_set(set,c->name);
  newconf->config_type = c->config_type;
  newconf->flags = c->flags;
  newconf->parent = new_parent;
  newconf->argc = c->argc;
  if(c->argc) {
    newconf->argv = palloc(newconf->pool,(c->argc+1)*sizeof(void*));
    argv = newconf->argv; sargv = c->argv;
    argc = newconf->argc;

    while(argc--)
      *argv++ = *sargv++;
    if(argv)
    *argv++ = NULL;
  }

  if(c->subset)
    for(c = (config_rec*)c->subset->xas_list; c; c=c->next)
      _copy_recur(&newconf->subset,p,c,newconf);
}

static 
void _copy_global_to_all(xaset_t *set)
{
  server_rec *s;
  config_rec *c;

  if(!set || !set->xas_list)
    return;

  for(c = (config_rec*)set->xas_list; c; c=c->next)
    for(s = (server_rec*)servers->xas_list; s; s=s->next)
      _copy_recur(&s->conf,s->pool,c,NULL);
}

void fixup_globals(void)
{
  server_rec *s,*smain;
  config_rec *c,*cnext;

  smain = (server_rec*)servers->xas_list;
  for(s = smain; s; s=s->next) {
    /* loop through each top level directive looking for a CONF_GLOBAL
     * context
     */
    if(!s->conf || !s->conf->xas_list)
      continue;

    for(c = (config_rec*)s->conf->xas_list; c; c=cnext) {
      cnext = c->next;
      if(c->config_type == CONF_GLOBAL) {
        /* copy the contents of the block to all other servers
         * (including this one), then pull the block "out of play".
         */
        if(c->subset && c->subset->xas_list)
          _copy_global_to_all(c->subset);
        xaset_remove(s->conf,(xasetmember_t*)c);
        if(!s->conf->xas_list) {
          destroy_pool(s->conf->mempool);
          s->conf = NULL;
        }
      }
    }
  }
}

void fixup_dirs(server_rec *s, int mask)
{
  if(!s || !s->conf)
    return;

  _reorder_dirs(s->conf,mask);

  /* Merge mergeable configuration items down
   */

  _mergedown(s->conf,FALSE);

/*
  for(c = (config_rec*)s->conf->xas_list; c; c=c->next)
    if(c->config_type == CONF_ANON)
      _reorder_dirs(c->subset);
*/
  log_debug(DEBUG5,"");
  log_debug(DEBUG5,"Config for %s:",s->ServerName);
  debug_dump_config(s->conf,NULL);
}

config_rec *find_config_next(config_rec *prev, config_rec *c, int type,
                             const char *name, int recurse)
{
  config_rec *top = c;

  /* We do two searches (if recursing) so that we find the "deepest"
   * level first.
   */

  if(!c && !prev)
    return NULL;

  if(!prev)
    prev = top;

  if(recurse) {
    do {
      config_rec *res = NULL;

      for(c = top; c; c=c->next) {
        if(c->subset && c->subset->xas_list)
          res = find_config_next(NULL,(config_rec*)c->subset->xas_list,
                                 type,name,recurse+1);
          if(res)
            return res;
        }

      /* If deep recursion yielded no match try the current subset */
      /* NOTE: the string comparison here is specifically case sensitive.
       * The config_rec names are supplied by the modules and intentionally
       * case sensitive (they shouldn't be verbatim from the config file)
       * Do NOT change this to strcasecmp(), no matter how tempted you are
       * to do so, it will break stuff. ;)
       * -jss 4/10/2001
       */
      for(c = top; c; c=c->next)
        if((type == -1 || type == c->config_type) &&
            (!name || !strcmp(name,c->name)))
          return c;
              
      /* Restart the search at the previous level if required */
      if(prev->parent && recurse == 1 &&
         prev->parent->next &&
         prev->parent->set != find_config_top) {
        prev = top = prev->parent->next; c = top;
        continue;
      }

      break;
    } while(1);
  } else {
    for(c = top; c; c=c->next)
      if((type == -1 || type == c->config_type) &&
         (!name || !strcmp(name,c->name)))
        return c;
  }

  return NULL;
}
    
void find_config_set_top(config_rec *c)
{
  if(c && c->parent)
    find_config_top = c->parent->set;
  else
    find_config_top = NULL;
}


config_rec *find_config(xaset_t *set, int type, const char *name, int recurse)
{
  if(!set || !set->xas_list)
    return NULL;

  find_config_set_top((config_rec*)set->xas_list);

  return find_config_next(NULL,(config_rec*)set->xas_list,type,name,recurse);
}

/* These next two functions return the first argument in a
 * CONF_PARAM configuration entry.  If more than one or all
 * parameters are needed, the caller will need to use find_config,
 * and iterate through the argv themselves.
 * _int returns -1 if the config name is not found, _ptr returns
 * NULL.
 */

long get_param_int(xaset_t *set,const char *name,int recurse)
{
  config_rec *c;

  if(!set) {
    _last_param_int = NULL;
    return -1;
  }

  c = find_config(set,CONF_PARAM,name,recurse);

  if(c && c->argc) {
    _last_param_int = c;
    return (long)c->argv[0];
  }

  _last_param_int = NULL;
  return -1;  /* Parameters aren't allowed to contain neg. integers anyway */
}

long get_param_int_next(const char *name,int recurse)
{
  config_rec *c;

  if(!_last_param_int || !_last_param_int->next) {
    _last_param_int = NULL;
    return -1;
  }

  c = find_config_next(_last_param_int,_last_param_int->next,
                       CONF_PARAM,name,recurse);

  if(c && c->argc) {
    _last_param_int = c;
    return (long)c->argv[0];
  }

  _last_param_int = NULL;
  return -1;
}

void *get_param_ptr(xaset_t *set,const char *name,int recurse)
{
  config_rec *c;

  if(!set) {
    _last_param_ptr = NULL;
    return NULL;
  }

  c = find_config(set,CONF_PARAM,name,recurse);

  if(c && c->argc) {
    _last_param_ptr = c;
    return c->argv[0];
  }

  _last_param_ptr = NULL;
  return NULL;
}

void *get_param_ptr_next(const char *name,int recurse)
{
  config_rec *c;

  if(!_last_param_ptr || !_last_param_ptr->next) {
    _last_param_ptr = NULL;
    return NULL;
  }

  c = find_config_next(_last_param_ptr,_last_param_ptr->next,
                       CONF_PARAM,name,recurse);

  if(c && c->argv) {
    _last_param_ptr = c;
    return c->argv[0];
  }

  _last_param_ptr = NULL;
  return NULL;
}

int remove_config(xaset_t *set, const char *name,int recurse)
{
  server_rec *s = (conf.curserver ? *conf.curserver : main_server);
  config_rec *c;
  int found = 0;
  xaset_t *fset;

  while((c = find_config(set,-1,name,recurse)) != NULL) {
    found++;

    fset = c->set;
    xaset_remove(fset,(xasetmember_t*)c);

    /* if the set is empty, and has no more contained members in
     * the xas_list, destroy the set
     */
    if(!fset->xas_list) {

      /* first, set any pointers to the container of the set to NULL
       */
      if (c->parent && c->parent->subset == fset)
        c->parent->subset = NULL;

      else if (s->conf == fset)
        s->conf = NULL;

      /* next, destroy the set's pool, which destroys the set as well
       */
        destroy_pool(fset->mempool);

    } else {

      /* if the set was not empty, destroy only the requested config_rec
       */
      destroy_pool(c->pool); 
    }
  }

  return found;
}
        
config_rec *add_config_param_set(xaset_t **set,const char *name,int num,...)
{
  config_rec *c = add_config_set(set,name);
  void **argv;
  va_list ap;

  if(c) {
    c->config_type = CONF_PARAM;
    c->argc = num;
    c->argv = pcalloc(c->pool,(num+1)*sizeof(void*));

    argv = c->argv;
    va_start(ap,num);

    while(num-- > 0)
      *argv++ = va_arg(ap,void*);


    va_end(ap);
  }

  return c;
}

config_rec *add_config_param_str(const char *name, int num, ...) {
  config_rec *c = add_config(name);
  char *arg = NULL;
  void **argv = NULL;
  va_list ap;

  if (c) {
    c->config_type = CONF_PARAM;
    c->argc = num;
    c->argv = pcalloc(c->pool, (num+1) * sizeof(char*));

    argv = c->argv;
    va_start(ap, num);

    while(num-- > 0) {
      arg = va_arg(ap, char*);
      if (arg)
        *argv++ = pstrdup(c->pool, arg);
      else
        *argv++ = NULL;
    }

    va_end(ap);
  }

  return c;
}

config_rec *add_config_param(const char *name,int num,...)
{
  config_rec *c = add_config(name);
  void **argv;
  va_list ap;

  if(c) {
    c->config_type = CONF_PARAM;
    c->argc = num;
    c->argv = pcalloc(c->pool,(num+1) * sizeof(void*));
    
    argv = c->argv;
    va_start(ap,num);

    while(num-- > 0)
      *argv++ = va_arg(ap,void*);

    va_end(ap);
  }

  return c;
}

int parse_config_file(const char *fname)
{
  FILE *fp;
  cmd_rec *cmd;
  pool *tmp_pool = make_sub_pool(permanent_pool);
  modret_t *mr;
  int line = 0;
 
  fp = pfopen(tmp_pool,fname,"r");

  if(!fp) { destroy_pool(tmp_pool); return -1; }
  
  while((cmd = get_config_cmd(tmp_pool, fp, &line)) != NULL) {
    if(cmd->argc) {
      conftable *c;
      char found = 0;

      cmd->server = *conf.curserver;
      cmd->config = *conf.curconfig;

      for(c = m_conftable; c->directive; c++)
        if(!strcasecmp(c->directive,cmd->argv[0])) {
          ++found;
          if((mr = call_module(c->m,c->handler,cmd)) != NULL) {
            if(MODRET_ISERROR(mr)) {
	            log_pri(LOG_ERR,"Fatal: %s",MODRET_ERRMSG(mr));
	            exit(1);
	    }
          }

	  if(MODRET_ISDECLINED(mr))
	    found--;

          destroy_pool(cmd->tmp_pool);
        }

       if(!found) {
         log_pri(LOG_ERR,"Fatal: unknown configuration directive '%s' on line %d of '%s'.",
                 cmd->argv[0], line, fname);
         exit(1);
       }
    }

    destroy_pool(cmd->pool);
  }

  pfclose(tmp_pool,fp);
  destroy_pool(tmp_pool);
  return 0;
}

/* Go through each server configuration and complain if important
 * information is missing (post reading configuration files).
 * otherwise fill in defaults where applicable
 */

void fixup_servers(void)
{
  config_rec *c;
  server_rec *s;

  fixup_globals();

  s = (server_rec*)servers->xas_list;
  if(s && !s->ServerName)
    s->ServerName = pstrdup(s->pool,"ProFTPD");

  for(; s; s=s->next) {
    if(!s->ServerAddress)
      s->ServerFQDN = s->ServerAddress = inet_gethostname(s->pool);
    else
      s->ServerFQDN = inet_fqdn(s->pool,s->ServerAddress);
    if(!s->ServerFQDN)
      s->ServerFQDN = s->ServerAddress;

    if(!s->ServerAdmin)
      s->ServerAdmin = pstrcat(s->pool,"root@",s->ServerFQDN,NULL);
    if(!s->ServerName) {
      server_rec *m = (server_rec*)servers->xas_list;
      s->ServerName = pstrdup(s->pool,m->ServerName);
    }

    if(!s->tcp_rwin)
      s->tcp_rwin = TUNABLE_DEFAULT_RWIN;
    if(!s->tcp_swin)
      s->tcp_swin = TUNABLE_DEFAULT_SWIN;

    if ((s->ipaddr = inet_getaddr(s->pool, s->ServerAddress)) == NULL) {
      log_pri(LOG_ERR,"Fatal: unable to determine IP address of '%s'.",
        s->ServerAddress);
      exit(1);
    }

    if ((c = find_config(s->conf, CONF_PARAM, "MasqueradeAddress",
        FALSE)) != NULL) {
      log_pri(LOG_INFO, "%s:%d masquerading as %s",
        inet_ascii(s->pool, s->ipaddr), s->ServerPort,
        inet_ascii(s->pool, (p_in_addr_t *) c->argv[0]));
    }

    /* honor the DefaultServer directive only if SocketBindTight is not
     * in effect.
     */
    if (get_param_int(s->conf, "DefaultServer", FALSE) == 1) {
      if (!SocketBindTight)
        s->ipaddr->s_addr = 0;
      else
        log_pri(LOG_NOTICE,
          "SocketBindTight in effect, ignoring DefaultServer");
    }

    fixup_dirs(s,0);
  }

  clear_inet_pool();
}

void init_config(void) {
  pool *pool = make_sub_pool(permanent_pool);

  /* Make sure global_config_pool is destroyed */
  if (global_config_pool) {
    destroy_pool(global_config_pool);
    global_config_pool = NULL;
  }

  servers = xaset_create(pool,NULL);

  pool = make_sub_pool(permanent_pool);
  main_server = (server_rec*) pcalloc(pool,sizeof(server_rec));
  xaset_insert(servers, (xasetmember_t*) main_server);

  main_server->pool = pool;
  main_server->set = servers;

  /* default server port */
  main_server->ServerPort = inet_getservport(main_server->pool, "ftp", "tcp");
}

/* These functions are used by modules to help parse configuration.
 */

unsigned char check_conf(cmd_rec *cmd, int allowed) {
  int ctxt = (cmd->config && cmd->config->config_type != CONF_PARAM ?
     cmd->config->config_type : cmd->server->config_type ?
     cmd->server->config_type : CONF_ROOT);

  if (ctxt & allowed)
    return TRUE;

  /* default */
  return FALSE;
}

char *get_context_name(cmd_rec *cmd) {
  static char cbuf[20];

  if (!cmd->config || cmd->config->config_type == CONF_PARAM) {
    if (cmd->server->config_type == CONF_VIRTUAL)
      return "<VirtualHost>"; 
    else
      return "server config";
  }    

  memset(cbuf,'\0',sizeof(cbuf));
  switch(cmd->config->config_type) {
  case CONF_DIR: return "<Directory>";
  case CONF_ANON: return "<Anonymous>";
  case CONF_LIMIT: return "<Limit>";
  case CONF_DYNDIR: return ".ftpaccess";
  case CONF_GLOBAL: return "<Global>";
  case CONF_USERDATA: return "user data";
  default:
    /* in 1.3/2.0, should dispatch to modules here */
  snprintf(cbuf, sizeof(cbuf), "%d", cmd->config->config_type);
  return cbuf;
  }
}

int get_boolean(cmd_rec *cmd, int av)
{
  char *cp = cmd->argv[av];

  /* Boolean string can be "on","off","yes","no",
   * "true","false","1" or "0"
   */

  if(!strcasecmp(cp,"on"))
    return 1;
  if(!strcasecmp(cp,"off"))
    return 0;
  if(!strcasecmp(cp,"yes"))
    return 1;
  if(!strcasecmp(cp,"no"))
    return 0;
  if(!strcasecmp(cp,"true"))
    return 1;
  if(!strcasecmp(cp,"false"))
    return 0;
  if(!strcasecmp(cp,"1"))
    return 1;
  if(!strcasecmp(cp,"0"))
    return 0;

  return -1;
}

char *get_full_cmd(cmd_rec *cmd)
{
  pool *p = cmd->tmp_pool;
  char *res = "";
  int i;

  if(cmd->arg)
    res = pstrcat(p,cmd->argv[0]," ",cmd->arg,NULL);
  else {
    for(i = 0; i < cmd->argc; i++)
      res = pstrcat(p,res,cmd->argv[i]," ",NULL);

    while(res[strlen(res)-1] == ' ' && *res)
      res[strlen(res)-1] = '\0';
  }

  return res;
}
