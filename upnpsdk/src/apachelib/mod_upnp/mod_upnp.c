// mod_upnp.c - 
//
// ***********************************************************
// Copyright © 2002 America Online, Inc.  All Rights Reserved.
//
// -----------------------------------------------------------
// Initials		Date							Change
// -----------------------------------------------------------
// JJD			1/02						Initial Creation	
//
// -----------------------------------------------------------
#include "httpd.h"
#include "http_config.h"
#include "http_core.h"
#include "http_log.h"
#include "http_main.h"
#include "http_protocol.h"
#include "util_script.h"

#include <stdio.h>

// entry points to UPnP library
#include "miniserver.h"
#include "mod_upnp.h"

#if DEBUG
#define DBGONLY(x) x
#else
#define DBGONLY(x)
#endif

// happily stolen from alloc.c
struct table {
	array_header a;
};

void HandleRequest(request_rec *);

typedef struct upnpcfg {
    int cmode;            /* Environment to which record applies (directory,
                                 * server, or combination).
                                 */
    int local;                  /* Boolean: "Example" directive declared here? */
    int congenital;             /* Boolean: did we inherit an "Example"? */
    char *trace;                /* Pointer to trace string. */
	// not too fancy yet, only sort determine by port
    unsigned short port;
	char *location;
	// setable
	char *webRoot;
} upnpcfg;

#define PRINT_UPNPCFG(x) \
	printf("--- UPnP CFG STRUCT ---\n\tport: %i\n",x->port);\
	if(x->location!=NULL)\
		printf("\tlocation: %s\n",x->location);\
	else\
		printf("\tlocation: %i\n",x->location);\
	if(x->webRoot!=NULL)\
		printf("\twebRoot: %s\n",x->webRoot);\
	else\
		printf("\twebRoot: %i\n",x->webRoot);\
	printf("------------------------\n");
	

static const char *trace = NULL;
static table *static_calls_made = NULL;

static pool *upnp_pool = NULL;
static pool *upnp_subpool = NULL;

module MODULE_VAR_EXPORT upnp_module;

static upnpcfg *our_dconfig(request_rec *r)
{
    return (upnpcfg *) ap_get_module_config(r->per_dir_config, &upnp_module);
}

static upnpcfg *our_sconfig(server_rec *s)
{
	return (upnpcfg *)ap_get_module_config(s->module_config,&upnp_module);
}

/*
 * This routine sets up some module-wide cells if they haven't been already.
 */
static void setup_module_cells()
{
    /*
     * If we haven't already allocated our module-private pool, do so now.
     */
    if (upnp_pool == NULL) {
        upnp_pool = ap_make_sub_pool(NULL);
    };
    /*
     * Likewise for the table of routine/environment pairs we visit outside of
     * request context.
     */
    if (static_calls_made == NULL) {
        static_calls_made = ap_make_table(upnp_pool, 16);
    };
}


#define TRACE_NOTE "upnp-trace"

static void trace_add(server_rec *s, request_rec *r, upnpcfg *mconfig,
                      const char *note)
{
#if 0
    const char *sofar;
    char *addon;
    char *where;
    pool *p;
    const char *trace_copy;

    /*
     * Make sure our pools and tables are set up - we need 'em.
     */
    setup_module_cells();
    /*
     * Now, if we're in request-context, we use the request pool.
     */
    if (r != NULL) {
        p = r->pool;
        if ((trace_copy = ap_table_get(r->notes, TRACE_NOTE)) == NULL) {
            trace_copy = "";
        }
    }
    else {
        /*
         * We're not in request context, so the trace gets attached to our
         * module-wide pool.  We do the create/destroy every time we're called
         * in non-request context; this avoids leaking memory in some of
         * the subsequent calls that allocate memory only once (such as the
         * key formation below).
         *
         * Make a new sub-pool and copy any existing trace to it.  Point the
         * trace cell at the copied value.
         */
        p = ap_make_sub_pool(upnp_pool);
        if (trace != NULL) {
            trace = ap_pstrdup(p, trace);
        }
        /*
         * Now, if we have a sub-pool from before, nuke it and replace with
         * the one we just allocated.
         */
        if (upnp_subpool != NULL) {
            ap_destroy_pool(upnp_subpool);
        }
        upnp_subpool = p;
        trace_copy = trace;
    }
    /*
     * If we weren't passed a configuration record, we can't figure out to
     * what location this call applies.  This only happens for co-routines
     * that don't operate in a particular directory or server context.  If we
     * got a valid record, extract the location (directory or server) to which
     * it applies.
     */
    where = (mconfig != NULL) ? mconfig->loc : "nowhere";
    where = (where != NULL) ? where : "";
    /*
     * Now, if we're not in request context, see if we've been called with
     * this particular combination before.  The table is allocated in the
     * module's private pool, which doesn't get destroyed.
     */
    if (r == NULL) {
        char *key;

        key = ap_pstrcat(p, note, ":", where, NULL);
        if (ap_table_get(static_calls_made, key) != NULL) {
            /*
             * Been here, done this.
             */
            return;
        }
        else {
            /*
             * First time for this combination of routine and environment -
             * log it so we don't do it again.
             */
            ap_table_set(static_calls_made, key, "been here");
        }
    }
    addon = ap_pstrcat(p, "   <LI>\n", "    <DL>\n", "     <DT><SAMP>",
                    note, "</SAMP>\n", "     </DT>\n", "     <DD><SAMP>[",
                    where, "]</SAMP>\n", "     </DD>\n", "    </DL>\n",
                    "   </LI>\n", NULL);
    sofar = (trace_copy == NULL) ? "" : trace_copy;
    trace_copy = ap_pstrcat(p, sofar, addon, NULL);
    if (r != NULL) {
        ap_table_set(r->notes, TRACE_NOTE, trace_copy);
    }
    else {
        trace = trace_copy;
    }
    /*
     * You *could* change the following if you wanted to see the calling
     * sequence reported in the server's error_log, but beware - almost all of
     * these co-routines are called for every single request, and the impact
     * on the size (and readability) of the error_log is considerable.
     */
#define EXAMPLE_LOG_EACH 0
#if EXAMPLE_LOG_EACH
    if (s != NULL) {
        ap_log_error(APLOG_MARK, APLOG_DEBUG, s, "mod_example: %s", note);
    }
#endif
#endif
}

/* 
 * Command handler for the NO_ARGS "Example" directive.  All we do is mark the
 * call in the trace log, and flag the applicability of the directive to the
 * current location in that location's configuration record.
 */
static const char *cmd_devicedir(cmd_parms *cmd, void *dummy, char *deviceDir)
{
   // upnpcfg *cfg = (upnpcfg *) dummy;
	server_rec *s=cmd->server;
    DBGONLY(ap_log_error(APLOG_MARK, APLOG_DEBUG,cmd->server, "mod_example: HERE cmd: %s",deviceDir);)
	upnpcfg *cfg=our_sconfig(s);
	cfg->webRoot=ap_pstrdup(cmd->pool,deviceDir);
	//PRINT_UPNPCFG(cfg);

    return NULL;
}

/*--------------------------------------------------------------------------*/
/*                                                                          */
/* Now we declare our content handlers, which are invoked when the server   */
/* encounters a document which our module is supposed to have a chance to   */
/* see.  (See mod_mime's SetHandler and AddHandler directives, and the      */
/* mod_info and mod_status examples, for more details.)                     */
/*                                                                          */
/* Since content handlers are dumping data directly into the connexion      */
/* (using the r*() routines, such as rputs() and rprintf()) without         */
/* intervention by other parts of the server, they need to make             */
/* sure any accumulated HTTP headers are sent first.  This is done by       */
/* calling send_http_header().  Otherwise, no header will be sent at all,   */
/* and the output sent to the client will actually be HTTP-uncompliant.     */
/*--------------------------------------------------------------------------*/
/* 
 * Sample content handler.  All this does is display the call list that has
 * been built up so far.
 *
 * The return value instructs the caller concerning what happened and what to
 * do next:
 *  OK ("we did our thing")
 *  DECLINED ("this isn't something with which we want to get involved")
 *  HTTP_mumble ("an error status should be reported")
 */
static int upnp_handler(request_rec *r)
{

    upnpcfg *dcfg;

    dcfg = our_dconfig(r);
    trace_add(r->server, r, dcfg, "upnp_handler()");
    DBGONLY(ap_log_error(APLOG_MARK, APLOG_DEBUG,r->server, "mod_example: HERE HANDLER");)

    r->content_type = "text/html";

    ap_soft_timeout("send upnp call trace", r);
    //ap_send_http_header(r);// sending out later
#ifdef CHARSET_EBCDIC
    /* Server-generated response, converted */
    ap_bsetflag(r->connection->client, B_EBCDIC2ASCII, r->ebcdic.conv_out = 1);
#endif

    /*
     * If we're only supposed to send header information (HEAD request), we're
     * already there.
     *
    if (r->header_only) {
        ap_kill_timeout(r);
        return OK;
    }*/

	HandleRequest(r);

    /*
     * We're all done, so cancel the timeout we set.  Since this is probably
     * the end of the request we *could* assume this would be done during
     * post-processing - but it's possible that another handler might be
     * called and inherit our outstanding timer.  Not good; to each its own.
     */
    ap_kill_timeout(r);
    /*
     * We did what we wanted to do, so tell the rest of the server we
     * succeeded.
     */
    DBGONLY(ap_log_error(APLOG_MARK, APLOG_DEBUG,r->server, "mod_example: END HANDLER");)
    return OK;
}

int upnp_write(int rec, const char *bytes, size_t len, int timeout /* not used */)
{
	char *p=NULL;
	char *lineBegin=NULL;
	char *endOfLine=NULL;
	char *freeMe=NULL;
	char statusNum[4];
	int nWritten=0;
	int sendHeader=1;
	request_rec *r=(request_rec *)rec;
	
	DBGONLY(ap_log_error(APLOG_MARK, APLOG_DEBUG,r->server, "mod_example: %s",bytes);)

	lineBegin=(char *)calloc(len+1,sizeof(char));
	freeMe=lineBegin;
	strncpy(lineBegin,bytes,len);
	if(!strncmp(lineBegin,"HTTP/",5)){
		// steal the status
		while(*lineBegin!=' '){lineBegin++;}
		lineBegin++;
		strncpy(statusNum,lineBegin,3);

		r->status=atoi(statusNum);

		lineBegin=strstr(lineBegin,CRLF);
		lineBegin+=2;
		while((p=strchr(lineBegin,':'))!=NULL){
			if((endOfLine=strstr(p,CRLF))!=NULL){
				*p='\0';
				++p;
				//dont forget the space
				while(*p==' '){ ++p; }
				
				*endOfLine='\0';
				endOfLine+=2;// go beyond CRLF
				DBGONLY(ap_log_error(APLOG_MARK,APLOG_DEBUG,r->server,"header table set: %s : %s\n",lineBegin,p);)

				if(!strcasecmp(lineBegin,"content-type")){
    				//r->content_type = strdup(p);
    				r->content_type = ap_pstrdup(r->pool,p);
				}else if((endOfLine-2)==p){// empty variable
					ap_table_set(r->headers_out,lineBegin,"");
				}else{
					ap_table_set(r->headers_out,lineBegin,p);
				}

				if(endOfLine==strstr(endOfLine,CRLF)){
					//if CRLF is next, we're done
					lineBegin=endOfLine+2;
					break;
				}
			}
			lineBegin=endOfLine;//move on to next	
		}
		len=strlen(lineBegin);//recalculate
	}else{
		sendHeader=0;
	}

	if(sendHeader){
		DBGONLY(ap_log_error(APLOG_MARK, APLOG_DEBUG,r->server, "sending header\n");)
		ap_send_http_header(r);
	}
	
	DBGONLY(ap_log_error(APLOG_MARK, APLOG_DEBUG,r->server, "what we're sending (%i):\n%s",len,lineBegin);)

	nWritten=ap_rputs(lineBegin,r);

	if(freeMe)
		free(freeMe);

	return nWritten;
}

typedef enum HTTP_COMMAND_TYPEtag { CMD_HTTP_GET,
        CMD_SOAP_POST, CMD_SOAP_MPOST,
        CMD_GENA_SUBSCRIBE, CMD_GENA_UNSUBSCRIBE, CMD_GENA_NOTIFY,
        CMD_HTTP_UNKNOWN,
        CMD_HTTP_MALFORMED } HTTP_COMMAND_TYPE;


void HandleRequest(request_rec *rec)
{
	char * getStr = "GET";
	char * postStr = "POST";
	char * mpostStr = "M-POST";
    char * subscribeStr = "SUBSCRIBE";
    char * unsubscribeStr = "UNSUBSCRIBE";
    char * notifyStr = "NOTIFY";
    char * pattern=NULL;
    HTTP_COMMAND_TYPE retCode=CMD_HTTP_UNKNOWN;
	char *body=NULL;
	char *message=NULL;
	const int MAXHEADER=5120;
	char header[MAXHEADER+1];
	int i=0, patLength=0,totalHeader=0;
	char *p, *relativeLoc;
	MiniServerCallback callback=NULL;
	table_entry *elts;
	int clength=0;
//printf("port: %i\n",ap_get_server_port(rec));
	if(rec!=NULL && rec->method!=NULL ){
       	char c = rec->method[0];
        
       	switch (c){
        case 'G':
        pattern = getStr;
        retCode = CMD_HTTP_GET;
        break;
                
      	case 'P':
        pattern = postStr;
        retCode = CMD_SOAP_POST;
        break;
                
       	case 'M':
        pattern = mpostStr;
        retCode = CMD_SOAP_MPOST;
		break;
                
	    case 'S':
        pattern = subscribeStr;
        retCode = CMD_GENA_SUBSCRIBE;
        break;
                
       	case 'U':
        pattern = unsubscribeStr;
        retCode = CMD_GENA_UNSUBSCRIBE;
        break;
                
       	case 'N':
        pattern = notifyStr;
        retCode = CMD_GENA_NOTIFY;
        break;
            
       	default:
        // unknown method
		pattern = NULL;
		retCode = CMD_HTTP_UNKNOWN;
		break;
		}
	} 
       
	if(pattern!=NULL){
    	patLength = strlen( pattern );
        for (i = 1; i < patLength; i++ ){
        	if (rec->method[i] != pattern[i] )
           		retCode=CMD_HTTP_UNKNOWN;
		}	
        DBGONLY(ap_log_error(APLOG_MARK, APLOG_DEBUG,rec->server, "mod_upnp pattern: %s",pattern);)
    }

	switch(retCode){
		case CMD_SOAP_POST:
		case CMD_SOAP_MPOST:
			callback=GetSoapCallback();
       		DBGONLY(ap_log_error(APLOG_MARK, APLOG_DEBUG,rec->server, "mod_upnp soap: %i",callback);)
			break;
		case CMD_GENA_NOTIFY:
		case CMD_GENA_SUBSCRIBE:
		case CMD_GENA_UNSUBSCRIBE:
			callback=GetGenaCallback();
       		DBGONLY(ap_log_error(APLOG_MARK, APLOG_DEBUG,rec->server, "mod_upnp gena: %i",callback);)
			break;
		case CMD_HTTP_GET:
			//send the http get through the new callback
			callback=GetHTTPGetCallback();
       		DBGONLY(ap_log_error(APLOG_MARK, APLOG_DEBUG,rec->server, "mod_upnp http get: %i",callback);)
			break;
		default:
			callback=NULL;
			break;
	}
	// time to get the header... - note: must update upnp so it doesnt want to do the parsing
	// go through headers_in, dump those and add the ':', yuck..

	elts=(table_entry *)rec->headers_in->a.elts;
	bzero(header,MAXHEADER);

	sprintf(header,"%s%s",rec->the_request,CRLF);
	// strip the relative location on the web request
	relativeLoc=((upnpcfg *)our_dconfig(rec))->location;

	if(relativeLoc==NULL)// if not defined, use default, /upnp
		relativeLoc=ap_pstrdup(rec->pool,"/upnp");

	if((retCode==CMD_HTTP_GET) && ((p=strstr(header,relativeLoc))!=NULL)){
		strcpy(&p[0],&p[strlen(relativeLoc)]);
	}
	totalHeader=strlen(header);

	for(i=0;i < rec->headers_in->a.nelts;i++){
		totalHeader+= (strlen(elts[i].key)+strlen(elts[i].val)+7);
		if(totalHeader < MAXHEADER){
			sprintf(header,"%s%s: %s%s",header,elts[i].key,elts[i].val,CRLF);
			if(!strcasecmp(elts[i].key,"content-length"))
				clength=atoi(elts[i].val);
		}else{
			//break;
		}
 //       DBGONLY(ap_log_error(APLOG_MARK, APLOG_DEBUG,rec->server, "mod_upnp: headers appened: %s: %s",elts[i].key,elts[i].val);)
	}
	// concat the empty line btw header and body
	strcat(header,CRLF);

	DBGONLY(ap_log_error(APLOG_MARK, APLOG_DEBUG,rec->server, "mod_upnp: BODY len: %i",clength);)
	// now get the body
	if(clength){
		int totalRead=0;
		const int BUFFSIZE=3;
		char buf[BUFFSIZE+1];
		body=(char *)calloc(clength+5,sizeof(char));
		if(!ap_setup_client_block(rec,REQUEST_CHUNKED_ERROR)){
			if(ap_should_client_block(rec)){
				int getRetCode=0;
				do{
					getRetCode=ap_get_client_block(rec,buf,BUFFSIZE);
					totalRead+=getRetCode;
					if((getRetCode > 0) && (totalRead <= clength)){
 //   					DBGONLY(ap_log_error(APLOG_MARK, APLOG_DEBUG,rec->server,"buf: %s, getRetCode: %i, totalRead: %i, clength:%i\n",buf,getRetCode,totalRead,clength);)
						strncat(body,buf,getRetCode);
					}else{
//    					DBGONLY(ap_log_error(APLOG_MARK, APLOG_DEBUG,rec->server,"getRetCode: %i, totalRead: %i, clength:%i, strlen: %i\n",getRetCode,totalRead,clength,strlen(body));)
						break;
					}
				}while(1);
			}
		}
    	DBGONLY(ap_log_error(APLOG_MARK, APLOG_DEBUG,rec->server, "mod_upnp: BODY: %s",body);)
	}else if(retCode==CMD_SOAP_POST || retCode==CMD_SOAP_MPOST){
		// error must have body for POST
       	DBGONLY(ap_log_error(APLOG_MARK, APLOG_DEBUG,rec->server, "mod_upnp: NO BODY, MUST HAVE BODY!!");)
	}else{
       	DBGONLY(ap_log_error(APLOG_MARK, APLOG_DEBUG,rec->server, "mod_upnp: NO BODY, NO POST");)
	}

	if(body!=NULL){
		message=(char *)calloc(strlen(header)+strlen(body)+1,sizeof(char));
		strcpy(message,header);
		strcat(message,body);
		free(body);
	}else{
		message=(char *)calloc(strlen(header)+1,sizeof(char));
		strcpy(message,header);
	}


	if(callback!=NULL){
		CBParams *params=(CBParams *)calloc(1,sizeof(CBParams));
		params->cmdType=retCode;
		params->rec=(int)rec;
		callback(message,(int)params);
		if(params)
			free(params);
	}else{
       	DBGONLY(ap_log_error(APLOG_MARK, APLOG_DEBUG,rec->server, "mod_upnp: CALLBACK NULL");)
	}

	if(message!=NULL)
		free(message);
}

/* Handlers that are declared as "int" can return the following:            */
/*                                                                          */
/*  OK          Handler accepted the request and did its thing with it.     */
/*  DECLINED    Handler took no action.                                     */
/*  HTTP_mumble Handler looked at request and found it wanting.             */
/*                                                                          */
/* What the server does after calling a module handler depends upon the     */
/* handler's return value.  In all cases, if the handler returns            */
/* DECLINED, the server will continue to the next module with an handler    */
/* for the current phase.  However, if the handler return a non-OK,         */
/* non-DECLINED status, the server aborts the request right there.  If      */
/* the handler returns OK, the server's next action is phase-specific;      */
/* see the individual handler comments below for details.                   */
/*                                                                          */
/*--------------------------------------------------------------------------*/

/*
 * All our module-initialiser does is add its trace to the log.
 */
static void upnp_init(server_rec *s, pool *p)
{
    DBGONLY(ap_log_error(APLOG_MARK, APLOG_DEBUG, s, "upnp_init: HERE");)
    /*
     * Set up any module cells that ought to be initialised.
     */
    setup_module_cells();
   	// initialization for UPnP library callback 
    SetWriteToClient(upnp_write);
}

/* 
 * This function is called during server initialisation when an heavy-weight
 * process (such as a child) is being initialised.  As with the
 * module-initialisation function, any information that needs to be recorded
 * must be in static cells, since there's no configuration record.
 *
 * There is no return value.
 */

/*
 * All our process-initialiser does is add its trace to the log.
 */
static void upnp_child_init(server_rec *s, pool *p)
{
	char *webDir=our_sconfig(s)->webRoot;
    DBGONLY(ap_log_error(APLOG_MARK, APLOG_DEBUG, s, "upnp_child_init: HERE");)
	// initialize the library
int mine=	MiniServerInit(webDir);


    DBGONLY(ap_log_error(APLOG_MARK, APLOG_DEBUG, s, "upnp_child_init: HERE step 2: mine: %i",mine);)
    /*
     * Set up any module cells that ought to be initialised.
     */
    setup_module_cells();

   	// initialization for UPnP library callback 
    SetWriteToClient(upnp_write);
}

/* 
 * This function is called when an heavy-weight process (such as a child) is
 * being run down or destroyed.  As with the child-initialisation function,
 * any information that needs to be recorded must be in static cells, since
 * there's no configuration record.
 *
 * There is no return value.
 */

/*
 * All our process-death routine does is add its trace to the log.
 */
static void upnp_child_exit(server_rec *s, pool *p)
{
    DBGONLY(ap_log_error(APLOG_MARK, APLOG_DEBUG, s, "upnp_child_EXIT: HERE");)
	// UPnP shutdown of server, notify the calling library
	MiniServerDeInit();
}


/*
 * This routine is called after the request has been read but before any other
 * phases have been processed.  This allows us to make decisions based upon
 * the input header fields.
 *
 * The return value is OK, DECLINED, or HTTP_mumble.  If we return OK, no
 * further modules are called for this phase.
 */
static int upnp_post_read_request(request_rec *r)
{

    upnpcfg *cfg;

    cfg = our_dconfig(r);
    /*
     * We don't actually *do* anything here, except note the fact that we were
     * called.
     */
    trace_add(r->server, r, cfg, "example_post_read_request()");
    return DECLINED;
}

/*
 * This routine gives our module an opportunity to translate the URI into an
 * actual filename.  If we don't do anything special, the server's default
 * rules (Alias directives and the like) will continue to be followed.
 *
 * The return value is OK, DECLINED, or HTTP_mumble.  If we return OK, no
 * further modules are called for this phase.
 */
static int upnp_translate_handler(request_rec *r)
{

    upnpcfg *cfg;

    cfg = our_dconfig(r);
    /*
     * We don't actually *do* anything here, except note the fact that we were
     * called.
     */
    trace_add(r->server, r, cfg, "example_translate_handler()");
    return DECLINED;
}


/*
 * This routine is called to determine and/or set the various document type
 * information bits, like Content-type (via r->content_type), language, et
 * cetera.
 *
 * The return value is OK, DECLINED, or HTTP_mumble.  If we return OK, no
 * further modules are given a chance at the request for this phase.
 */
static int upnp_type_checker(request_rec *r)
{

    upnpcfg *cfg;

    cfg = our_dconfig(r);
    /*
     * Log the call, but don't do anything else - and report truthfully that
     * we didn't do anything.
     */
    trace_add(r->server, r, cfg, "example_type_checker()");
    return DECLINED;
}

/*
 * This routine is called to perform any module-specific fixing of header
 * fields, et cetera.  It is invoked just before any content-handler.
 *
 * The return value is OK, DECLINED, or HTTP_mumble.  If we return OK, the
 * server will still call any remaining modules with an handler for this
 * phase.
 */
static int upnp_fixer_upper(request_rec *r)
{

    upnpcfg *cfg;

    cfg = our_dconfig(r);
    /*
     * Log the call and exit.
     */
    trace_add(r->server, r, cfg, "example_fixer_upper()");
    return OK;
}

/*
 * This routine is called to perform any module-specific logging activities
 * over and above the normal server things.
 *
 * The return value is OK, DECLINED, or HTTP_mumble.  If we return OK, any
 * remaining modules with an handler for this phase will still be called.
 */
static int upnp_logger(request_rec *r)
{

    upnpcfg *cfg;

    cfg = our_dconfig(r);
    trace_add(r->server, r, cfg, "upnp_logger()");
    return DECLINED;
}

/*
 * This routine is called to give the module a chance to look at the request
 * headers and take any appropriate specific actions early in the processing
 * sequence.
 *
 * The return value is OK, DECLINED, or HTTP_mumble.  If we return OK, any
 * remaining modules with handlers for this phase will still be called.
 */
static int example_header_parser(request_rec *r)
{

    upnpcfg *cfg;

    cfg = our_dconfig(r);
    trace_add(r->server, r, cfg, "example_header_parser()");
    return DECLINED;
}

/*--------------------------------------------------------------------------*/
/*                                                                          */
/* All of the routines have been declared now.  Here's the list of          */
/* directives specific to our module, and information about where they      */
/* may appear and how the command parser should pass them to us for         */
/* processing.  Note that care must be taken to ensure that there are NO    */
/* collisions of directive names between modules.                           */
/*                                                                          */
/*--------------------------------------------------------------------------*/
/* 
 * List of directives specific to our module.
 */
static const command_rec upnp_cmds[] =
{
    {
        "SetDeviceRootDir",              /* directive name */
        cmd_devicedir,            /* config action routine */
        NULL,                   /* argument to include in call */
        OR_OPTIONS,             /* where available */
        TAKE1,                /* arguments */
        "SetDeviceRootDir: The path to the device description documents",
                                /* directive description */
    },
    {NULL}
};

/* 
 * List of content handlers our module supplies.  Each handler is defined by
 * two parts: a name by which it can be referenced (such as by
 * {Add,Set}Handler), and the actual routine name.  The list is terminated by
 * a NULL block, since it can be of variable length.
 *
 * Note that content-handlers are invoked on a most-specific to least-specific
 * basis; that is, a handler that is declared for "text/plain" will be
 * invoked before one that was declared for "text / *".  Note also that
 * if a content-handler returns anything except DECLINED, no other
 * content-handlers will be called.
 */
static const handler_rec upnp_handlers[] =
{
    {"upnp-handler", upnp_handler},
    {NULL}
};

// per server config
static void *upnp_create_server_config(pool *p, server_rec *s)
{
	server_addr_rec *addrs;
	upnpcfg *cfg=(upnpcfg *)ap_pcalloc(p, sizeof(upnpcfg));

	cfg->port=s->port;
ap_log_error(APLOG_MARK, APLOG_DEBUG, s, "upnp create server: %i",cfg->port);

	return (void *)cfg;
}

static void *upnp_merge_server_config(pool *p,
									  void *oldServerConfig,
									  void *newServerConfig)
{
	upnpcfg *cfg=(upnpcfg *)ap_pcalloc(p,sizeof(upnpcfg));
	upnpcfg *cfgNew=(upnpcfg *)newServerConfig;
	upnpcfg *cfgOld=(upnpcfg *)oldServerConfig;

	// merge the per server info
	cfg->port=cfgNew->port;

	// merge the per directory info
	cfg->location=ap_pstrdup(p,cfgOld->location);
	
 //   ap_log_error(APLOG_MARK, APLOG_DEBUG,NULL , "upnp merge server: %s %i",
//				cfg->location,cfg->port);

	return (void *)cfg;
}

// per directory config
static void *upnp_create_dir_config(pool *p, char *dir)
{
	upnpcfg *cfg=(upnpcfg *)ap_pcalloc(p,sizeof(upnpcfg));
	cfg->location=ap_pstrdup(p,dir);
//	PRINT_UPNPCFG(cfg);
//ap_log_error(APLOG_MARK, APLOG_DEBUG,NULL , "upnp create dir: %i",cfg->location);
	return (void *)cfg;
}

// merge the new directory info, into an existing cfg record (like per server)
static void *upnp_merge_dir_config(pool *p, 
								   void *parentConfig, 
								   void *newConfig)
{
	upnpcfg *cfg=(upnpcfg *)ap_pcalloc(p,sizeof(upnpcfg));
	upnpcfg *cfgNew=(upnpcfg *)newConfig;
	upnpcfg *cfgOld=(upnpcfg *)parentConfig;

	// merge the per directory info
	cfg->location=ap_pstrdup(p,cfgNew->location);

	// merge the per server info
	cfg->port=cfgOld->port;

	// merge webdir
	cfg->webRoot=ap_pstrdup(p,cfgNew->webRoot);

//printf("upnp merge dir: %s %i\n",cfg->location,cfg->port);
 //   ap_log_error(APLOG_MARK, APLOG_DEBUG,NULL , "upnp merge dir: %s %i",
//				cfg->location,cfg->port);

	return (void *)cfg;
}

/* 
 * Module definition for configuration.  If a particular callback is not
 * needed, replace its routine name below with the word NULL.
 *
 * The number in brackets indicates the order in which the routine is called
 * during request processing.  Note that not all routines are necessarily
 * called (such as if a resource doesn't have access restrictions).
 */
module MODULE_VAR_EXPORT upnp_module =
{
    STANDARD_MODULE_STUFF,
    upnp_init,               	/* module initializer */
    upnp_create_dir_config,  		/* per-directory config creator */
    upnp_merge_dir_config,   		/* dir config merger */
    upnp_create_server_config,  		/* server config creator */
    upnp_merge_server_config,   		/* server config merger */
    upnp_cmds,                  /* command table */
    upnp_handlers,           	/* [9] list of handlers */
    upnp_translate_handler,   	/* [2] filename-to-URI translation */
    NULL, 			/* [5] check/validate user_id */
    NULL, 			/* [6] check user_id is valid *here* */
    NULL,		        /* [4] check access by host address */
    upnp_type_checker,       	/* [7] MIME type checker/setter */
    upnp_fixer_upper,        /* [8] fixups */
    upnp_logger,             	/* [10] logger */
#if MODULE_MAGIC_NUMBER >= 19970103
    NULL,      	/* [3] header parser */
#endif
#if MODULE_MAGIC_NUMBER >= 19970719
    upnp_child_init,         	/* process initializer */
#endif
#if MODULE_MAGIC_NUMBER >= 19970728
    upnp_child_exit,         	/* process exit/cleanup */
#endif
#if MODULE_MAGIC_NUMBER >= 19970902
    upnp_post_read_request   	/* [1] post read_request handling */
#endif
};
