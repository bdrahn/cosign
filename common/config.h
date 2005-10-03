/*
 * Copyright (c) 2004 Regents of The University of Michigan.
 * All Rights Reserved.  See LICENSE.
 */

#define NOTAUTH 0 
#define CGI	1
#define SERVICE	2
#define DAEMON	3

#define AL_TICKET	(1<<0)
#define AL_PROXY	(1<<1)

#define SL_REAUTH	(1<<0)

struct certlist {
    char		*cl_issuer;
    char		*cl_subject;
    char		*cl_login;
    char		*cl_realm;
    char		*cl_type;
    struct certlist	*cl_next;
};

struct servicelist {
    char		*sl_cookie;
    int			sl_flag;
    struct servicelist	*sl_next;
};

struct authlist {
    char		*al_hostname;
    int			al_key;
    int			al_flag;
    struct proxies	*al_proxies;
    struct authlist	*al_next;
};

struct proxies {
    char		*pr_hostname;
    char		*pr_cookie;
    struct proxies	*pr_next;
};

struct cosigncfg {
    char 		*cc_key;
    char 		**cc_value;
    unsigned int 	cc_numval;
    struct cosigncfg 	*cc_next;
};

#define COSIGNDBKEY		"cosigndb"
#define COSIGNCADIRKEY		"cosigncadir"
#define COSIGNCERTKEY		"cosigncert"
#define	COSIGNKEYKEY		"cosignkey"
#define COSIGNTICKKEY		"cosignticketcache"
#define COSIGNDTICKKEY		"cosigndticketcache"
#define COSIGNTMPLDIRKEY	"cosigntmpldir"
#define COSIGNHOSTKEY		"cosignhost"
#define COSIGNKEYTABKEY		"cosignkeytab"
#define	COSIGNLOGOUTURLKEY	"cosignlogouturl"
#define COSIGNTIMEOUTKEY	"cosignnettimeout"
#define COSIGNPORTKEY		"cosignport"
#define COSIGNLOOPURLKEY	"cosignloopurl"
#define COSIGNX509TKTSKEY       "cosignx509krbtkts"

#ifdef SQL_FRIEND
#define MYSQLDBKEY	"mysqldb"
#define MYSQLUSERKEY	"mysqluser"
#define MYSQLPASSWDKEY	"mysqlpasswd"
#endif

int		cosign_ssl( char *, char *, char *, SSL_CTX ** );
struct authlist	*authlist_find( char * );
struct servicelist	*service_find( char * );
int		cosign_config( char * );
char		*cosign_config_get( char * );
char		**cosign_config_get_all( char *, int * );
int 		x509_translate( char *, char *, char **, char ** );
