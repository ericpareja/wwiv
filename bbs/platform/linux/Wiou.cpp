/**************************************************************************/
/*                                                                        */
/*                              WWIV Version 5.0x                         */
/*             Copyright (C)1998-2004, WWIV Software Services             */
/*                                                                        */
/*    Licensed  under the  Apache License, Version  2.0 (the "License");  */
/*    you may not use this  file  except in compliance with the License.  */
/*    You may obtain a copy of the License at                             */
/*                                                                        */
/*                http://www.apache.org/licenses/LICENSE-2.0              */
/*                                                                        */
/*    Unless  required  by  applicable  law  or agreed to  in  writing,   */
/*    software  distributed  under  the  License  is  distributed on an   */
/*    "AS IS"  BASIS, WITHOUT  WARRANTIES  OR  CONDITIONS OF ANY  KIND,   */
/*    either  express  or implied.  See  the  License for  the specific   */
/*    language governing permissions and limitations under the License.   */
/*                                                                        */
/**************************************************************************/


#include "wwiv.h"
#include "WComm.h"
#include <termios.h>
#include <sys/poll.h>
#include <sys/ioctl.h>
#include <sys/time.h>

#define TTY "/dev/tty"


void set_terminal( bool initMode )
{
	static struct termios foo;
	static struct termios boo;

	if ( initMode )
	{
		tcgetattr( fileno( stdin ), &foo );
		memcpy( &boo, &foo, sizeof( struct termios ) );
		foo.c_lflag &= ~ICANON;
		tcsetattr( fileno( stdin ), TCSANOW, &foo );
	}
	else
	{
		tcsetattr( fileno( stdin ), TCSANOW, &boo );
	}
}


bool WIOUnix::setup( char parity, int wordlen, int stopbits, unsigned long baud )
{
    return true;
}

unsigned int WIOUnix::open()
{
	return 0;
}

void WIOUnix::close( bool bIsTemporary = false )
{
    bIsTemporary = bIsTemporary;
}

unsigned int WIOUnix::putW( unsigned char ch )
{
	::write( fileno( stdout ), &ch, 1 );
	return 0;
}

unsigned char WIOUnix::getW()
{
	unsigned char ch = 0;
	int dlay = 0;
	struct pollfd p;

	p.fd = fileno( stdin );
	p.events = POLLIN;

	if ( poll( &p, 1, dlay ) )
	{
		if ( p.revents & POLLIN )
		{
			::read( fileno( stdin ), &ch, 1 );
			// Don't ask why this needs to be here...but it does
			if ( ch == SOFTRETURN )
			{
				ch = RETURN;
			}
		}
	}
	return ch;
}


bool WIOUnix::dtr( bool raise )
{
	return true;
}


void WIOUnix::flushOut()
{
}


void WIOUnix::purgeOut()
{
}


void WIOUnix::purgeIn()
{
}


unsigned int WIOUnix::put(unsigned char ch)
{
	return putW( ch );
}


char WIOUnix::peek()
{
	// This is only called by function rpeek_wfconly which is only
	// ever invoked from the WFC
	return 0;
}

unsigned int WIOUnix::read( char *buffer, unsigned int count )
{
	unsigned char ch = 0;
	int dlay = 0;
	struct pollfd p;

	p.fd = fileno( stdin );
	p.events = POLLIN;

	if ( poll( &p, 1, dlay ) )
	{
		if ( p.revents & POLLIN )
		{
			::read( fileno( stdin ), &ch, 1 );
			// Don't ask why this needs to be here...but it does
			if ( ch == SOFTRETURN )
			{
				ch = RETURN;
			}
		}
	}
	return ch;
}


unsigned int WIOUnix::write( const char *buffer, unsigned int count, bool bNoTransation )
{
	::write( fileno( stdout ), buffer, count );
	return 0;
}

bool WIOUnix::carrier()
{
    return ( !hangup && !hungup ) ? true : false;
}


bool WIOUnix::incoming()
{
  	struct pollfd p;

	p.fd = fileno( stdin );
  	p.events = POLLIN;
  	poll( &p, 1, 0 );
  	if ( p.revents & POLLIN )
    {
		return true;
	}
	return false;
}


bool WIOUnix::startup()
{
	if ( tty_open )
	{
		return true;
	}

	if ( ( ttyf = fdopen( ::open( TTY, O_RDWR ), "r" ) ) == NULL )
	{
		ttyf = stdin;
	}
	else
	{
		setbuf( ttyf, NULL );
	}


	int f = fileno( ttyf );

#ifdef linux
	ioctl( f, TCGETS, &ttyb );
	ioctl( f, TCGETS, &ttysav );
	ttyb.c_lflag &= ~( ECHO | ISIG );
	ioctl( f, TCSETS, &ttyb );
#else
	ioctl( f, TIOCGETA, &ttyb );
	ioctl( f, TIOCGETA, &ttysav );
	ttyb.c_lflag &= ~( ECHO | ISIG );
	ioctl( f, TIOCSETA, &ttyb );
#endif

	set_terminal( true );

	tty_open = f;
	return true;
}


bool WIOUnix::shutdown()
{
	if ( !tty_open )
	{
		std::cout << "DBG: close_tty called when the tty is not open!\n";
		return true;
	}

	set_terminal( false );

#ifdef linux
	ioctl( tty_open, TCSETS, &ttysav );
#else
	ioctl( tty_open, TIOCSETA, &ttysav );
#endif

	if ( ttyf != stdin )
	{
		fclose( ttyf );
	}

	tty_open = 0;
	return true;
}


void WIOUnix::StopThreads()
{
}


void WIOUnix::StartThreads()
{
}



