// p2013dim
// Pixel 2013 backlight command
// Paul Alfille 2021
// github.com/alfille/pixel2013

// MIT license

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

#define BackDir "/sys/class/backlight/intel_backlight/"
#define KeyDir "/sys/class/leds/chromeos::kbd_backlight/"

#define ControlCur "brightness"
#define ControlMax "max_brightness"

char * progname = "program" ; // should be redirected

void help( void )
{
    fprintf( stderr, "%s -- set the screen or keyboard brightness level for the Google Pixel Chromebook (2013)\n" , progname ) ;
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

FILE * file_open( char * dir, char * name, char * mode )
{
	char Path[1024] ;
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

long value_read( char * dir, char * name )
{
    FILE * file = file_open( dir, name, "r" ) ;
    char buffer[64] ;
    char * str = fgets( buffer, sizeof(buffer), file ) ;

    fclose( file ) ;
    if ( str == buffer ) {
		return strtol( buffer, NULL, 10 ) ;
    }
    perror("Trouble reading file" ) ;
    exit(1) ;
}
    
int main( int argc, char **argv )
{

    progname = argv[0] ; // point to invoked program name
    
    // Arguments
    int c ;
	char * endptr ;
	long bright = -1. ;
	long max_bright ;
	char * Dir = BackDir ;
	
    while ( (c = getopt( argc, argv, ":b:k:h" )) != -1 ) {
		switch ( c ) {
			case 'h':
				help() ;
				return 0 ;
			case 'b':
				Dir = BackDir ;
				bright = strtol( optarg, &endptr, 10 ) ;
				if ( (optarg == endptr) || (bright<0.) || (bright > 100.) ) {
					help() ;
					fprintf( stderr, "\nInvalid numeric argument: %s\n", optarg );
					exit(1) ;
				}
				break ;
			case 'k':
				Dir = KeyDir ;
				bright = strtol( optarg, &endptr, 10 ) ;
				if ( (optarg == endptr) || (bright<0.) || (bright > 100.) ) {
					help() ;
					fprintf( stderr, "\nInvalid numeric argument: %s\n", optarg );
					exit(1) ;
				}
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
		bright = strtol( argv[optind], &endptr, 10 ) ;
		if ( (argv[optind] == endptr) || (bright<0.) || (bright>100.) ) {
			help() ;
			fprintf( stderr, "\nInvalid numeric argument: %s\n", argv[optind] );
			exit(1) ;
		}
	}

	// max
	max_bright = value_read( Dir, ControlMax ) ;
	
	if ( bright < 0. ) {
		long cur = value_read( Dir, ControlCur ) ;
		printf( "%3.0f\n",100.*cur/max_bright ) ;
		return 0 ;
	}
    
    // write brightness to control file
    FILE * sys = file_open( Dir, ControlCur, "a" ) ;
    if ( fprintf( sys, "%ld\n", (long int) (bright*max_bright*.01) ) < 1 ) {
		perror("Trouble writing to file" ) ;
    }
    fclose( sys ) ;
    return 0 ;
}
