/*
 * Copyright (c) 1998 Regents of The University of Michigan.
 * All Rights Reserved.  See COPYRIGHT.
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/param.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <string.h>
#include <syslog.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

#include <snet.h>

#include "command.h"
#include "cparse.h"

#define CPATH "/var/cosign"
#define IP_SZ 254
#define USER_SZ 30
#define REALM_SZ 254

extern char	*version;

struct command {
    char	*c_name;
    int		(*c_func) ___P(( SNET *, int, char *[] ));
};

static int	f_noop ___P(( SNET *, int, char *[] ));
static int	f_quit ___P(( SNET *, int, char *[] ));
static int	f_help ___P(( SNET *, int, char *[] ));
static int	f_login ___P(( SNET *, int, char *[] ));
static int	f_logout ___P(( SNET *, int, char *[] ));



    int
f_quit( snet, ac, av )
    SNET			*snet;
    int				ac;
    char			*av[];
{
    snet_writef( snet, "%d Service closing transmission channel\r\n", 221 );
    exit( 0 );
}


    int
f_noop( snet, ac, av )
    SNET			*snet;
    int				ac;
    char			*av[];
{
    snet_writef( snet, "%d cosign v%s\r\n", 250, version );
    return( 0 );
}

    int
f_help( sn, ac, av )
    SNET        *sn;
    int         ac;
    char        *av[];
{
    snet_writef( sn, "%d Vaild commands are HELP, NOOP, and QUIT\r\n", 203 );
    return( 0 );
}
    int
f_login( sn, ac, av )
    SNET        *sn;
    int         ac;
    char        *av[];
{
    char		path[ MAXPATHLEN ], tmppath[ MAXPATHLEN ];
    FILE		*tmpfile;
    struct timeval	tv;
    int			fd;
    extern int		errno;

    /* login login_cookie ip principal realm [tgt] */

    if ( ac != 5 ) {
	snet_writef( sn, "%d LOGIN Syntax error\r\n", 500 );
	return( 1 );
    }

    if ( snprintf( path, MAXPATHLEN, "%s/%s", CPATH, av[ 1 ] ) >= MAXPATHLEN ) {
	snet_writef( sn, "%d LOGIN Syntax error: Cookie too long\r\n", 501 );
	return( 1 );
    }

    if ( gettimeofday( &tv, NULL ) != 0 ){
	syslog( LOG_ERR, "f_login: gettimeofday: %m" );
	snet_writef( sn, "%d LOGIN Fatal Error: Sorry!\r\n", 502 );
	return( -1 );
    }

    if ( snprintf( tmppath, MAXPATHLEN, "%s/%x%x.%i", CPATH,
	    tv.tv_sec, tv.tv_usec, (int)getpid()) >= MAXPATHLEN ) {
	syslog( LOG_ERR, "f_login: tmppath too long" );
	snet_writef( sn, "%d LOGIN Fatal Error: Sorry!\r\n", 502 );
	return( -1 );
    }

    if (( fd = open( tmppath, O_CREAT|O_EXCL|O_WRONLY, 0644 )) < 0 ) {
	syslog( LOG_ERR, "f_login: open: %m" );
	snet_writef( sn, "%d LOGIN Fatal Error: Sorry!\r\n", 502 );
	return( -1 );
    }

    if (( tmpfile = fdopen( fd, "w" )) == NULL ) {
	/* close */
	if ( unlink( tmppath ) != 0 ) {
	    syslog( LOG_ERR, "f_login: unlink: %m" );
	}
	syslog( LOG_ERR, "f_login: fdopen: %m" );
	snet_writef( sn, "%d LOGIN Fatal Error: Sorry!\r\n", 502 );
	return( -1 );
    }

    syslog( LOG_INFO, "f_login: tmpfile: %s", tmppath );

    fprintf( tmpfile, "v0\n" );
    fprintf( tmpfile, "s1\n" );	 /* 1 is logged in, 0 is logged out */
    if ( strlen( av[ 2 ] ) >= IP_SZ ) {
	goto file_err;
    }
    fprintf( tmpfile, "i%s\n", av[ 2 ] );
    if ( strlen( av[ 3 ] ) >= USER_SZ ) {
	goto file_err;
    }
    fprintf( tmpfile, "p%s\n", av[ 3 ] );
    if ( strlen( av[ 4 ] ) >= REALM_SZ ) {
	goto file_err;
    }
    fprintf( tmpfile, "r%s\n", av[ 4 ] );
    fprintf( tmpfile, "t%lu\n", tv.tv_sec );

    if ( fclose ( tmpfile ) != 0 ) {
	if ( unlink( tmppath ) != 0 ) {
	    syslog( LOG_ERR, "f_login: unlink: %m" );
	}
	syslog( LOG_ERR, "f_login: fclose: %m" );
	snet_writef( sn, "%d LOGIN Fatal Error: Sorry!\r\n", 502 );
	return( -1 );
    }

    if ( link( tmppath, path ) != 0 ) {
	if ( unlink( tmppath ) != 0 ) {
	    syslog( LOG_ERR, "f_login: unlink: %m" );
	}
	if( errno == EEXIST ) {
	    syslog( LOG_ERR, "f_login: file already exists: %s", path );
	    snet_writef( sn,
		    "%d LOGIN error: Cookie already exists\r\n", 400 );
	    return( 1 );
	}
	syslog( LOG_ERR, "f_login: link: %m" );
	snet_writef( sn, "%d LOGIN Fatal Error: Sorry!\r\n", 502 );
	return( -1 );
    }

    if ( unlink( tmppath ) != 0 ) {
	syslog( LOG_ERR, "f_login: unlink: %m" );
    }
    snet_writef( sn, "%d LOGIN successful: Cookie Stored \r\n", 200 );

    return( 0 );

file_err:
    if ( unlink( tmppath ) != 0 ) {
	syslog( LOG_ERR, "f_login: unlink: %m" );
    }
    syslog( LOG_ERR, "f_login: bad file format" );
    snet_writef( sn, "%d LOGIN Syntax Error: Bad File Format\r\n", 503 );
    return( 1 );
}

    int
f_logout( sn, ac, av )
    SNET        *sn;
    int         ac;
    char        *av[];
{
    struct cinfo	ci;
    int			fd;
    char		path[ MAXPATHLEN ];

    /*LOGOUT login_cookie ip */

    if ( ac != 3 ) {
	snet_writef( sn, "%d LOGOUT Syntax error\r\n", 510 );
	return( 1 );
    }

    /* do we care if there are /s ? */
    /* strchr for / */
    if ( snprintf( path, MAXPATHLEN, "%s/%s", CPATH, av[ 1 ] ) >= MAXPATHLEN ) {
	snet_writef( sn, "%d LOGOUT Syntax error: Cookie too long\r\n", 511 );
	return( 1 );
    }

    if ( read_a_cookie( path, &ci ) != 0 ) {
	syslog( LOG_ERR, "f_logout: read_a_cookie: XXX" );
	snet_writef( sn, "%d LOGOUT error: Sorry\r\n", 512 );
	return( 1 );
    }

    if( strcmp( av[ 2 ], ci.ci_ipaddr ) != 0 ) {
	syslog( LOG_INFO, "%s in cookie %s does not match %s",
		ci.ci_ipaddr, path, av[ 2 ] );
	snet_writef( sn,
		"%d IP address given does not match cookie\r\n", 410 );
	return( 1 );
    }

    if ( ci.ci_state == 0 ) {
	syslog( LOG_ERR, "f_logout: %s already logged out", path );
	snet_writef( sn, "%d LOGOUT error: Already logged out\r\n", 410 );
	return( 1 );
    }

    if (( fd = open( path, O_WRONLY, 0644 )) < 0 ) {
	syslog( LOG_ERR, "f_logout: %s: %m" );
	snet_writef( sn, "%d LOGOUT error: Sorry!\r\n", 522 );
	return( -1 );
    }

    if ( lseek( fd, 4, SEEK_SET ) == -1 ) {
	(void)close( fd );
	syslog( LOG_ERR, "f_logout: %s: %m", path );
	snet_writef( sn, "%d LOGOUT error: Sorry!\r\n", 522 );
	return( -1 );
    }

    if ( write( fd, "0", 1 ) == -1 ) {
	(void)close( fd );
	syslog( LOG_ERR, "f_logout: %s: %m", path );
	snet_writef( sn, "%d LOGOUT error: Sorry!\r\n", 522);
	return( -1 );
    }

    (void)close( fd );

    snet_writef( sn, "%d LOGOUT successful: cookie no longer valid\r\n", 210 );
    return( 0 );

}


struct command	commands[] = {
    { "NOOP",		f_noop },
    { "QUIT",		f_quit },
    { "HELP",		f_help },
    { "LOGIN",		f_login },
    { "LOGOUT",		f_logout },
};
int		ncommands = sizeof( commands ) / sizeof( commands[ 0 ] );

    int
command( fd )
    int			fd;
{
    SNET				*snet;
    int					ac, i;
    char				**av, *line;
    struct timeval			tv;

    srandom( (unsigned)getpid());

    if (( snet = snet_attach( fd, 1024 * 1024 )) == NULL ) {
	syslog( LOG_ERR, "snet_attach: %m" );
	/* We *could* use write(2) to report an error before we exit here */
	exit( 1 );
    }

    snet_writef( snet, "%d COokie SIGNer ready\r\n", 220 );

    tv.tv_sec = 60 * 10;	/* 10 minutes, should get this from config */
    tv.tv_usec = 0;
    while (( line = snet_getline( snet, &tv )) != NULL ) {
	tv.tv_sec = 60 * 10;
	tv.tv_usec = 0;

	if (( ac = argcargv( line, &av )) < 0 ) {
	    syslog( LOG_ERR, "argcargv: %m" );
	    break;
	}

	if ( ac == 0 ) {
	    snet_writef( snet, "%d Command unrecognized\r\n", 501 );
	    continue;
	}

	for ( i = 0; i < ncommands; i++ ) {
	    if ( strcasecmp( av[ 0 ], commands[ i ].c_name ) == 0 ) {
		break;
	    }
	}
	if ( i >= ncommands ) {
	    snet_writef( snet, "%d Command %s unregcognized\r\n",
		    500, av[ 0 ] );
	    continue;
	}

	if ( (*(commands[ i ].c_func))( snet, ac, av ) < 0 ) {
	    break;
	}
    }

    snet_writef( snet,
	    "421 Service not available, closing transmission channel\r\n" );

    if ( line == NULL ) {
	syslog( LOG_ERR, "snet_getline: %m" );
    }

    exit( 1 );

}
