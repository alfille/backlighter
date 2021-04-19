// brightness
// formerly Pixel 2013 backlight command
// now for all laptops
// Paul Alfille 2021
// github.com/alfille/backlighter

// MIT license

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <dirent.h>

#define BackDir "/sys/class/backlight/"
#define KeyDir "/sys/class/leds/"

#define ControlCur "brightness"
#define ControlMax "max_brightness"
#define ControlType "type"

char * progname = "program" ; // should be redirected

void help( void )
{
    fprintf( stderr, "%s -- set the screen or keyboard brightness level for this laptop\n" , progname ) ;
    fprintf( stderr, "\n") ;
    fprintf( stderr, "Writes to /sys/class -- needs root privileges\n" ) ;
    fprintf( stderr, "by Paul H Alfille 2021\n") ;
    fprintf( stderr, "\n") ;
    fprintf( stderr, "\t%s -b -- show backlight percent\n", progname ) ;
    fprintf( stderr, "\t%s -b 43-- set backlight percent\n", progname ) ;
    fprintf( stderr, "\n") ;
    fprintf( stderr, "\t%s -k -- show keylight percent\n", progname ) ;
    fprintf( stderr, "\t%s -k 43-- set keylight percent\n", progname ) ;
    fprintf( stderr, "\n") ;
    fprintf( stderr, "\t%s -b (screen backlight) is assumed if k or b not specified\n", progname ) ;
    fprintf( stderr, "\n") ;
    fprintf( stderr, "\t%s -h -- this help\n", progname ) ;
}

long value( char * s )
{
    char * endptr ;
    long b = strtol( s, &endptr, 10 ) ;

    if ( s == endptr ) {
        help() ;
        fprintf( stderr, "\nInvalid numeric argument: %s\n", s );
        exit(1) ;
    }
    return b ;
}
    
long percent( long b )
{
    if ( (b<0.) || (b>100.) ) {
        help() ;
        fprintf( stderr, "\nInvalid numeric argument: %ld\n", b );
        exit(1) ;
    }
    return b ;
}
    
int file_exists( char * dir, char * name )
{
    // return 0 if exists
    struct stat sb ;
    char Path[PATH_MAX] ;
    
    strcpy( Path, dir ) ;
    strcat( Path, name ) ;

    if ( stat( Path, &sb )==0 && S_ISREG(sb.st_mode) ) {
        return 0 ;
    }
    return 1 ;
}

char * next_dir( DIR * dir )
{
    struct dirent * next ;

    do {
        next = readdir( dir ) ;
        if ( next == NULL ) {
            return NULL ;
        }
        //printf("non-dir (%d) %s\n",next->d_type,next->d_name ) ;
        if ( next->d_type == DT_LNK ) {
            // For some reason, sys directories are DT_LNK not DT_DIR
            return next->d_name ;
        }
    } while (1) ;
}

FILE * file_open( char * dir, char * name, char * mode )
{
    char Path[PATH_MAX] ;
    FILE * f;
    
    strcpy( Path, dir ) ;
    strcat( Path, name ) ;
    f = fopen( Path, mode ) ;
    if ( f == NULL ) {
        fprintf( stderr, "Cannot open %s: ", Path ) ;
        perror( NULL ) ;
        exit(1) ;
    }
    return f ;
}

void string_read( char * dir, char * name, char * buffer )
{
    FILE * file = file_open( dir, name, "r" ) ;
    char * str = fgets( buffer, sizeof(buffer), file ) ;

    fclose( file ) ;
    if ( str == buffer ) {
        return ;
    }
    perror("Trouble reading file" ) ;
    exit(1) ;
}

long value_read( char * dir, char * name )
{
    char buffer[64] ;
    string_read( dir, name, buffer ) ;
    return value(buffer) ;
}
    
int main( int argc, char **argv )
{
    char FullPath[ PATH_MAX ] ;
    
    // Arguments
    int c ;
    long bright = -1. ;
    long max_bright ;
    char * Dir = BackDir ;

    progname = argv[0] ; // point to invoked program name
    
    while ( (c = getopt( argc, argv, ":b:k:h" )) != -1 ) {
        switch ( c ) {
            case 'h':
                help() ;
                return 0 ;
            case 'b':
                Dir = BackDir ;
                bright = percent(value( optarg )) ;
                break ;
            case 'k':
                Dir = KeyDir ;
                bright = percent(value( optarg )) ;
                break ;
            case ':':
                switch(optopt) {
                    case 'b':
                        Dir = BackDir ;
                        break ;
                    case 'k':
                        Dir = KeyDir ;
                        break ;
                    default:
                        help() ;
                        exit(1);
                }
                break ;
            default:
                help() ;
                return 0 ;
            break ;
        }
    }

    if ( argv[optind] != NULL ) {
        bright = percent(value( argv[optind] )) ;
    }

    DIR * pdir = opendir( Dir ) ;
    if ( pdir == NULL ) {
        fprintf( stderr, "Cannot open directory %s\n", Dir ) ;
        help() ;
        exit(1) ;
    }

    char * trydir ;
    FullPath[0] = '\0' ;
    
    // loop through directories
    while ( (trydir = next_dir( pdir )) != NULL ) {
        char trypath[PATH_MAX] ;
        //printf("Trial directory in %s: %s\n",Dir,trydir ) ;
        if ( Dir==KeyDir && strstr( trydir, "lock" ) ) {
            // hint -- remove keylock and numlock...
            continue ;
        }
            
        strcpy( trypath, Dir ) ; // has terminating /
        strcat( trypath, trydir ) ;
        strcat( trypath, "/" ) ;
        if ( file_exists( trypath, ControlMax ) != 0 ) {
            // must have max backlight file
            continue ;
        }
        if ( file_exists( trypath, ControlCur ) != 0 ) {
            // must have backlight file
            continue ;
        }
        if ( file_exists( trypath, ControlType) == 0 ) {
            // hint, prefer screen backlight "raw" type
            char buffer[64] ;
            string_read( trypath, ControlType, buffer ) ;
            if ( strstr( buffer, "raw" ) ) {
                strcpy( FullPath, trypath ) ;
                break ;
            }
        }
        if ( Dir==KeyDir && strstr( trydir, "light" ) != 0 ) {
            // hint, prefer key lights with "light" in name
            strcpy( FullPath, trypath ) ;
            break ;
        }
        strcpy( FullPath, trypath ) ;                
    }
    closedir( pdir ) ;

    if ( FullPath[0] == '\0' ) {
        fprintf( stderr, "No appropriate directories found in %s\n", Dir ) ;
        help();
        exit(1) ;
    }
    
    // get max value (for scaling)
    max_bright = value_read( FullPath, ControlMax ) ;
    
    if ( bright < 0. ) {
        // report value, not set
        long cur = value_read( FullPath, ControlCur ) ;
        printf( "%3.0f\n",100.*cur/max_bright ) ;
        return 0 ;
    }
    
    // write brightness to control file
    FILE * sys = file_open( FullPath, ControlCur, "a" ) ;
    if ( fprintf( sys, "%ld\n", (long int) (bright*max_bright*.01) ) < 1 ) {
        perror("Trouble writing to file" ) ;
    }
    fclose( sys ) ;
    return 0 ;
}
