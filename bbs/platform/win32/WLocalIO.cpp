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

#ifndef NOT_BBS
#include "wwiv.h"
#endif
#include "WStringUtils.h"

extern int oldy = 0;


const int WLocalIO::cursorNone      = 0;
const int WLocalIO::cursorNormal    = 1;
const int WLocalIO::cursorSolid     = 2;

const int WLocalIO::topdataNone     = 0;
const int WLocalIO::topdataSystem   = 1;
const int WLocalIO::topdataUser     = 2;

//
// local functions
//

bool HasKeyBeenPressed();
unsigned char GetKeyboardChar();

/*
 * Sets screen attribute at screen pos x,y to attribute contained in a.
 */

void WLocalIO::set_attr_xy(int x, int y, int a)
{
	COORD loc = {0};
	DWORD cb = {0};

	loc.X = static_cast<short>( x );
	loc.Y = static_cast<short>( y );

	WriteConsoleOutputAttribute( m_hConOut, reinterpret_cast< LPWORD >( &a ), 1, loc, &cb );
}




WLocalIO::WLocalIO()
{
    m_nWfcStatus = 0;
	ExtendedKeyWaiting = 0;
	wx = 0;

    m_hConOut = GetStdHandle( STD_OUTPUT_HANDLE );
    m_hConIn  = GetStdHandle( STD_INPUT_HANDLE );
    if ( m_hConOut == INVALID_HANDLE_VALUE )
    {
        std::cout << "\n\nCan't get console handle!.\n\n";
        abort();
    }
    GetConsoleScreenBufferInfo( m_hConOut,&m_consoleBufferInfo );
	m_originalConsoleSize = m_consoleBufferInfo.dwSize;
	SMALL_RECT rect = m_consoleBufferInfo.srWindow;
	COORD bufSize;
	bufSize.X = static_cast<short>( rect.Right - rect.Left + 1 );
	bufSize.Y = static_cast<short>( rect.Bottom - rect.Top + 1 );
	bufSize.X = static_cast<short>( std::min<SHORT>( bufSize.X, 80 ) );
	bufSize.Y = static_cast<short>( std::min<SHORT>( bufSize.Y, 25 ) );
    SetConsoleWindowInfo( m_hConOut, TRUE, &rect );
	SetConsoleScreenBufferSize( m_hConOut, bufSize );

    m_cursorPosition.X = m_consoleBufferInfo.dwCursorPosition.X;
    m_cursorPosition.Y = m_consoleBufferInfo.dwCursorPosition.Y;

	// Have to reset this info, otherwise bad things happen.
    GetConsoleScreenBufferInfo( m_hConOut,&m_consoleBufferInfo );
}


WLocalIO::~WLocalIO()
{
	SetConsoleScreenBufferSize( m_hConOut, m_originalConsoleSize );
}


void WLocalIO::set_global_handle(bool bOpenFile, bool bOnlyUpdateVariable )
{
    if ( x_only )
	{
        return;
	}

    if ( bOpenFile )
    {
		if ( !fileGlobalCap.IsOpen() )
        {
			char szFileName[MAX_PATH];
			sprintf(szFileName, "%sglobal-%d.txt", syscfg.gfilesdir, app->GetInstanceNumber() );
			fileGlobalCap.SetName( szFileName );

			bool bOpen = fileGlobalCap.Open( WFile::modeBinary | WFile::modeAppend | WFile::modeCreateFile | WFile::modeReadWrite, WFile::shareUnknown, WFile::permReadWrite );
            global_ptr = 0;
            global_buf = static_cast<char *>( BbsAllocA( GLOBAL_SIZE ) );
            if ( !bOpen || !global_buf )
            {
                if (global_buf)
                {
                    BbsFreeMemory(global_buf);
                    global_buf = NULL;
                }
            }
        }
    }
    else
    {
		if ( fileGlobalCap.IsOpen() && !bOnlyUpdateVariable )
        {
			fileGlobalCap.Write( global_buf, global_ptr );
			fileGlobalCap.Close();
            if (global_buf)
            {
                BbsFreeMemory(global_buf);
                global_buf = NULL;
            }
        }
    }
}


void WLocalIO::global_char(char ch)
{

	if ( global_buf && fileGlobalCap.IsOpen() )
    {
        global_buf[global_ptr++] = ch;
        if (global_ptr == GLOBAL_SIZE)
        {
			fileGlobalCap.Write( global_buf, global_ptr );
            global_ptr = 0;
        }
    }
}

void WLocalIO::set_x_only(int tf, const char *pszFileName, int ovwr)
{
    static bool nOldGlobalHandle;

    if (x_only)
    {
        if (!tf)
        {
			if ( fileGlobalCap.IsOpen() )
            {
				fileGlobalCap.Write( global_buf, global_ptr );
				fileGlobalCap.Close();
                if (global_buf)
                {
                    BbsFreeMemory(global_buf);
                    global_buf = NULL;
                }
            }
            x_only = false;
			set_global_handle( ( nOldGlobalHandle ) ? true : false );
            nOldGlobalHandle = false;
            express = expressabort = false;
        }
    }
    else
    {
        if (tf)
        {
			nOldGlobalHandle = fileGlobalCap.IsOpen();
            set_global_handle( false );
            x_only = true;
            wx = 0;
    		char szTempFileName[MAX_PATH];
            sprintf(szTempFileName, "%s%s", syscfgovr.tempdir, pszFileName);
			fileGlobalCap.SetName( szTempFileName );

            if (ovwr)
            {
				fileGlobalCap.Open( WFile::modeBinary|WFile::modeText|WFile::modeCreateFile|WFile::modeReadWrite, WFile::shareUnknown, WFile::permReadWrite );
            }
            else
            {
				fileGlobalCap.Open( WFile::modeBinary|WFile::modeCreateFile|WFile::modeAppend|WFile::modeReadWrite, WFile::shareUnknown, WFile::permReadWrite );
            }
            global_ptr = 0;
            express = true;
            expressabort = false;
            global_buf = static_cast<char *>( BbsAllocA(GLOBAL_SIZE) );
			if ( !fileGlobalCap.IsOpen() || !global_buf )
            {
                if (global_buf)
                {
                    BbsFreeMemory(global_buf);
                    global_buf = NULL;
                }
                set_x_only(0, NULL, 0);
            }
        }
    }
    timelastchar1 = timer1();
}


void WLocalIO::LocalGotoXY(int x, int y)
// This, obviously, moves the cursor to the location specified, offset from
// the protected dispaly at the top of the screen.  Note: this function
// is 0 based, so (0,0) is the upper left hand corner.
{
	x = std::max<int>( x, 0 );
    x = std::min<int>( x, 79 );
    y = std::max<int>( y, 0 );
    y += sess->topline;
	y = std::min<int>( y, sess->screenbottom );

    if (x_only)
    {
        wx = x;
        return;
    }
    m_cursorPosition.X = static_cast< short > ( x );
    m_cursorPosition.Y = static_cast< short > ( y );
    SetConsoleCursorPosition(m_hConOut,m_cursorPosition);
}




int WLocalIO::WhereX()
/* This function returns the current X cursor position, as the number of
* characters from the left hand side of the screen.  An X position of zero
* means the cursor is at the left-most position
*/
{
    if (x_only)
    {
        return( wx );
    }

    CONSOLE_SCREEN_BUFFER_INFO m_consoleBufferInfo;
    GetConsoleScreenBufferInfo(m_hConOut,&m_consoleBufferInfo);

    m_cursorPosition.X = m_consoleBufferInfo.dwCursorPosition.X;
    m_cursorPosition.Y = m_consoleBufferInfo.dwCursorPosition.Y;

    return m_cursorPosition.X;
}



int WLocalIO::WhereY()
/* This function returns the Y cursor position, as the line number from
* the top of the logical window.  The offset due to the protected top
* of the screen display is taken into account.  A WhereY() of zero means
* the cursor is at the top-most position it can be at.
*/
{
    CONSOLE_SCREEN_BUFFER_INFO m_consoleBufferInfo;

    GetConsoleScreenBufferInfo(m_hConOut,&m_consoleBufferInfo);

    m_cursorPosition.X = m_consoleBufferInfo.dwCursorPosition.X;
    m_cursorPosition.Y = m_consoleBufferInfo.dwCursorPosition.Y;

    return m_cursorPosition.Y - sess->topline;
}



void WLocalIO::LocalLf()
/* This function performs a linefeed to the screen (but not remotely) by
* either moving the cursor down one line, or scrolling the logical screen
* up one line.
*/
{
    SMALL_RECT scrollRect;
    COORD dest;
    CHAR_INFO fill;

    if (m_cursorPosition.Y >= sess->screenbottom)
    {
        dest.X = 0;
        dest.Y = static_cast< short > ( sess->topline );
        scrollRect.Top = static_cast< short > ( sess->topline + 1 );
        scrollRect.Bottom = static_cast< short > ( sess->screenbottom );
        scrollRect.Left = 0;
        scrollRect.Right = 79;
        fill.Attributes = static_cast< short > ( curatr );
        fill.Char.AsciiChar = ' ';

        ScrollConsoleScreenBuffer(m_hConOut, &scrollRect, NULL, dest, &fill);
    }
    else
    {
        m_cursorPosition.Y++;
        SetConsoleCursorPosition(m_hConOut,m_cursorPosition);
    }
}



void WLocalIO::LocalCr()
/* This short function returns the local cursor to the left-most position
* on the screen.
*/
{
    m_cursorPosition.X = 0;
    SetConsoleCursorPosition(m_hConOut,m_cursorPosition);
}

void WLocalIO::LocalCls()
/* This clears the local logical screen */
{
    int nOldCurrentAttribute = curatr;
    curatr = 0x07;
    // TODO Debugging hack - REMOVE THIS!!
    SMALL_RECT scrollRect;
    COORD dest;
    CHAR_INFO fill;

    dest.X = 32767;
    dest.Y = 0;
    scrollRect.Top = static_cast< short > ( sess->topline );
    scrollRect.Bottom = static_cast< short > ( sess->screenbottom );
    scrollRect.Left = 0;
    scrollRect.Right = 79;
    fill.Attributes = static_cast< short > ( curatr );
    fill.Char.AsciiChar = ' ';

    ScrollConsoleScreenBuffer(m_hConOut, &scrollRect, NULL, dest, &fill);

    LocalGotoXY(0, 0);
    lines_listed = 0;
    curatr = nOldCurrentAttribute;
}



void WLocalIO::LocalBackspace()
/* This function moves the cursor one position to the left, or if the cursor
* is currently at its left-most position, the cursor is moved to the end of
* the previous line, except if it is on the top line, in which case nothing
* happens.
*/
{
    if (m_cursorPosition.X >= 0)
    {
        m_cursorPosition.X--;
    }
    else if ( m_cursorPosition.Y != sess->topline )
    {
        m_cursorPosition.Y--;
        m_cursorPosition.X = 79;
    }
    SetConsoleCursorPosition( m_hConOut,m_cursorPosition );
}



void WLocalIO::LocalPutchRaw(unsigned char ch)
/* This function outputs one character to the screen, then updates the
* cursor position accordingly, scolling the screen if necessary.  Not that
* this function performs no commands such as a C/R or L/F.  If a value of
* 8, 7, 13, 10, 12 (BACKSPACE, BEEP, C/R, L/F, TOF), or any other command-
* type characters are passed, the appropriate corresponding "graphics"
* symbol will be output to the screen as a normal character.
*/
{
    DWORD cb;

    SetConsoleTextAttribute( m_hConOut, static_cast< short > ( curatr ) );
    WriteConsole( m_hConOut, &ch, 1, &cb,NULL );

    if (m_cursorPosition.X <= 79)
    {
        m_cursorPosition.X++;
        return;
    }

    // Need to scroll the screen up one.
    m_cursorPosition.X = 0;
    if ( m_cursorPosition.Y == sess->screenbottom )
    {
        COORD dest;
        SMALL_RECT MoveRect;
        CHAR_INFO fill;

        // rushfan scrolling fix (was no +1)
        MoveRect.Top    = static_cast< short > ( sess->topline + 1 );
        MoveRect.Bottom = static_cast< short > ( sess->screenbottom );
        MoveRect.Left   = 0;
        MoveRect.Right  = 79;

        fill.Attributes = static_cast< short > ( curatr );
        fill.Char.AsciiChar = ' ';

        dest.X = 0;
		// rushfan scrolling fix (was -1)
        dest.Y = static_cast< short > ( sess->topline );

        ScrollConsoleScreenBuffer(m_hConOut,&MoveRect,&MoveRect,dest,&fill);
    }
    else
    {
        m_cursorPosition.Y++;
    }
}




void WLocalIO::LocalPutch(unsigned char ch)
/* This function outputs one character to the local screen.  C/R, L/F, TOF,
* BS, and BELL are interpreted as commands instead of characters.
*/
{
    if (x_only)
    {
        if (ch > 31)
        {
            wx = (wx + 1) % 80;
        }
        else if ( ch == RETURN || ch == CL )
        {
            wx = 0;
        }
        else if (ch == BACKSPACE)
        {
            if (wx)
            {
                wx--;
            }
        }
        return;
    }

    if (ch > 31)
    {
        LocalPutchRaw(ch);
    }
    else if (ch == CM)
    {
        LocalCr();
    }
    else if (ch == CJ)
    {
        LocalLf();
    }
    else if (ch == CL)
    {
        LocalCls();
    }
    else if (ch == BACKSPACE)
    {
        LocalBackspace();
    }
    else if ( ch == CG )
    {
        if ( !outcom )
        {
            // TODO Make the bell sound configurable.
			WWIV_Sound(500, 4);
        }
    }
}


void WLocalIO::LocalPuts(const char *s)
// This (obviously) outputs a string TO THE SCREEN ONLY
{
    while ( *s )
    {
        LocalPutch( *s++ );
    }
}


void WLocalIO::LocalXYPuts( int x, int y, const char *pszText )
{
    app->localIO->LocalGotoXY( x, y );
    app->localIO->LocalFastPuts( pszText );
}


void WLocalIO::LocalFastPuts( const char *pszText )
// This RAPIDLY outputs ONE LINE to the screen only and is not exactly stable.
{
    DWORD cb = 0;
    int len = strlen( pszText );

    SetConsoleTextAttribute( m_hConOut, static_cast< short > ( curatr ) );
    WriteConsole( m_hConOut, pszText, len, &cb, NULL );
    m_cursorPosition.X = m_cursorPosition.X + static_cast< short >( cb );
}


int  WLocalIO::LocalPrintf( const char *pszFormattedText, ... )
{
    va_list ap;
    char szBuffer[ 1024 ];

    va_start( ap, pszFormattedText );
    int nNumWritten = vsnprintf( szBuffer, 1024, pszFormattedText, ap );
    va_end( ap );
    app->localIO->LocalFastPuts( szBuffer );
    return nNumWritten;
}


int  WLocalIO::LocalXYPrintf( int x, int y, const char *pszFormattedText, ... )
{
    va_list ap;
    char szBuffer[ 1024 ];

    va_start( ap, pszFormattedText );
    int nNumWritten = vsnprintf( szBuffer, 1024, pszFormattedText, ap );
    va_end( ap );
    app->localIO->LocalXYPuts( x, y, szBuffer );
    return nNumWritten;
}


int  WLocalIO::LocalXYAPrintf( int x, int y, int nAttribute, const char *pszFormattedText, ... )
{
    va_list ap;
    char szBuffer[ 1024 ];

    va_start( ap, pszFormattedText );
    int nNumWritten = vsnprintf( szBuffer, 1024, pszFormattedText, ap );
    va_end( ap );
    setc( nAttribute );
    app->localIO->LocalXYPuts( x, y, szBuffer );
    return nNumWritten;
}


void WLocalIO::pr_Wait(int i1)
{
    int i, i2, i3;
    char *ss = "-=[WAIT]=-";
    i2 = i3 = strlen(ss);
    for (i = 0; i < i3; i++)
    {
        if ((ss[i] == 3) && (i2 > 1))
        {
            i2 -= 2;
        }
    }

    if (i1)
    {
        if ( okansi() )
        {
            i = curatr;
            setc( sess->thisuser.hasColor() ? sess->thisuser.GetColor( 3 ) : sess->thisuser.GetBWColor( 3 ) );
            sess->bout << ss;
            sess->bout << "\x1b[" << i2 << "D";
            setc( static_cast< unsigned char > ( i ) );
        }
        else
        {
            sess->bout << ss;
        }
    }
    else
    {
        if ( okansi() )
        {
            for (i = 0; i < i2; i++)
            {
                bputch(' ');
            }
            sess->bout << "\x1b[" << i2 << "D";
        }
        else
        {
            for (i = 0; i < i2; i++)
            {
                BackSpace();
            }
        }
    }
}



void WLocalIO::set_protect(int l) //JZ Set_Protect Fix
// set_protect sets the number of lines protected at the top of the screen.
{
	if ( l != sess->topline )
	{
        COORD coord;
		coord.X = 0;
		coord.Y = static_cast< short > ( l );

        if (l > sess->topline)
        {
            if ( ( WhereY() + sess->topline - l ) < 0 )
            {
	            CHAR_INFO lpFill;
	            SMALL_RECT scrnl;

				scrnl.Top = static_cast< short > ( sess->topline );
				scrnl.Left = 0;
				scrnl.Bottom = static_cast< short > ( sess->screenbottom );
				scrnl.Right = 79; //%%TODO - JZ Make the console size user defined

				lpFill.Char.AsciiChar = ' ';
				lpFill.Attributes = 0;

				coord.X = 0;
				coord.Y = static_cast< short > ( l );
				ScrollConsoleScreenBuffer(m_hConOut, &scrnl, NULL, coord, &lpFill);
                LocalGotoXY( WhereX(), WhereY() + l - sess->topline );
            }
            oldy += (sess->topline - l);
        }
        else
        {
        	DWORD written;
			FillConsoleOutputAttribute(m_hConOut,0,(sess->topline - l) * 80,coord,&written);
            oldy += (sess->topline - l);
        }
    }
    sess->topline = l;
	sess->screenlinest = ( sess->using_modem ) ? sess->thisuser.GetScreenLines() : defscreenbottom + 1 - sess->topline;
}


void WLocalIO::savescreen(screentype * pScreenType)
{
    COORD topleft;
    CONSOLE_SCREEN_BUFFER_INFO bufinfo;
    SMALL_RECT region;

    GetConsoleScreenBufferInfo(m_hConOut,&bufinfo);
    topleft.Y = topleft.X = region.Top = region.Left = 0;
    region.Bottom = static_cast< short > ( bufinfo.dwSize.Y - 1 );
    region.Right  = static_cast< short > ( bufinfo.dwSize.X - 1 );

    if (!pScreenType->scrn1)
    {
        pScreenType->scrn1= static_cast< CHAR_INFO *> ( bbsmalloc((bufinfo.dwSize.X*bufinfo.dwSize.Y)*sizeof(CHAR_INFO)) );
    }

    if (pScreenType->scrn1)
    {
        ReadConsoleOutput(m_hConOut,(CHAR_INFO *)pScreenType->scrn1,bufinfo.dwSize,topleft,&region);
    }

    pScreenType->x1 = static_cast< short > ( WhereX() );
    pScreenType->y1 = static_cast< short > ( WhereY() );
    pScreenType->topline1 = static_cast< short > ( sess->topline );
    pScreenType->curatr1 = static_cast< short > ( curatr );
}


/*
 * restorescreen restores a screen previously saved with savescreen
 */
void WLocalIO::restorescreen(screentype * pScreenType)
{
    if (pScreenType->scrn1)
    {
        // COORD size;
        COORD topleft;
        CONSOLE_SCREEN_BUFFER_INFO bufinfo;
        SMALL_RECT region;

        GetConsoleScreenBufferInfo(m_hConOut,&bufinfo);
        topleft.Y = topleft.X = region.Top = region.Left = 0;
        region.Bottom = static_cast< short > ( bufinfo.dwSize.Y - 1 );
        region.Right  = static_cast< short > ( bufinfo.dwSize.X - 1 );

        WriteConsoleOutput(m_hConOut,pScreenType->scrn1,bufinfo.dwSize,topleft,&region);
        BbsFreeMemory(pScreenType->scrn1);
        pScreenType->scrn1 = NULL;
    }
    sess->topline = pScreenType->topline1;
    curatr = pScreenType->curatr1;
    LocalGotoXY(pScreenType->x1, pScreenType->y1);
}


void WLocalIO::ExecuteTemporaryCommand( const char *pszCommand )
{
    pr_Wait( 1 );
    savescreen( &screensave );
    int i = sess->topline;
    sess->topline = 0;
    curatr = 0x07;
    LocalCls();
    ExecuteExternalProgram( pszCommand, EFLAG_TOPSCREEN );
    restorescreen( &screensave );
    sess->topline = i;
    pr_Wait( 0 );
}


char xlate[] =
{
    'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', 0, 0, 0, 0,
    'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', 0, 0, 0, 0, 0,
    'Z', 'X', 'C', 'V', 'B', 'N', 'M',
};


char WLocalIO::scan_to_char(unsigned char ch)
{
    return ( ch >= 16 && ch <= 50 ) ? xlate[ch - 16] : '\x00';
}


void WLocalIO::alt_key(unsigned char ch)
{
    char ch1 = scan_to_char(ch);
    if (ch1)
    {
        char szCommand[ MAX_PATH ];
        memset( szCommand, 0, sizeof( szCommand ) );
        WFile macroFile( syscfg.datadir, MACROS_TXT );
        if ( macroFile.Open( WFile::modeReadOnly | WFile::modeBinary ) )
        {
            int l = macroFile.GetLength();
            char* ss = static_cast<char *>( BbsAllocA(l + 10) );
            if (ss)
            {
                macroFile.Read( ss, l );
                macroFile.Close();

                ss[l] = 0;
                char* ss1 = strtok(ss, "\r\n");
                while (ss1)
                {
                    if (upcase(*ss1) == ch1)
                    {
                        strtok(ss1, " \t");
                        ss1 = strtok(NULL, "\r\n");
                        if (ss1 && (strlen(ss1) < 128))
                        {
                            strcpy(szCommand, ss1);
                        }
                        ss1 = NULL;
                    }
                    else
                    {
                        ss1 = strtok(NULL, "\r\n");
                    }
                }
                BbsFreeMemory(ss);
            }

            if (szCommand[0])
            {
                if (szCommand[0] == '@')
                {
                    if (okmacro && okskey && (!charbufferpointer) && (szCommand[1]))
                    {
                        for (l = strlen(szCommand) - 1; l >= 0; l--)
                        {
                            if (szCommand[l] == '{')
                            {
                                szCommand[l] = '\r';
                            }
                            strcpy(charbuffer, szCommand);
                            charbufferpointer = 1;
                        }
                    }
                }
                else if (m_nWfcStatus == 1)
                {
                    holdphone( true );
                    ExecuteTemporaryCommand(szCommand);
                    cleanup_net();
                    holdphone( false );
                }

            }
        }
    }
}


/*
 * skey handles all f-keys and the like hit FROM THE KEYBOARD ONLY
 */
void WLocalIO::skey(char ch)
{
    int i, i1;

    if ( (syscfg.sysconfig & sysconfig_no_local) == 0 )
    {
        if (okskey)
        {
            if ((ch >= 104) && (ch <= 113))
            {
                set_autoval(ch - 104);
            }
            else
            {
                switch ((unsigned char) ch)
                {
                case 59:                          /* F1 */
                    OnlineUserEditor();
                    break;
                case 84:                          /* Shift-F1 */
					set_global_handle( ( fileGlobalCap.IsOpen() ) ? false : true );
                    UpdateTopScreen();
                    break;
                case 94:                          /* Ctrl-F1 */
                    if ( sess->bbsshutdown )
                    {
                        sess->bbsshutdown = 0;
                    }
                    else
                    {
                        shut_down( 1 );
                    }
                    break;
                case 60:                          /* F2 */
                    sess->topdata++;
                    if ( sess->topdata > WLocalIO::topdataUser )
                    {
                        sess->topdata = WLocalIO::topdataNone;
                    }
                    UpdateTopScreen();
                    break;
                case 61:                          /* F3 */
                    if ( sess->using_modem )
                    {
                        incom = !incom;
                        dump();
                        tleft( false );
                    }
                    break;
                case 62:                          /* F4 */
                    chatcall = false;
                    UpdateTopScreen();
                    break;
                case 63:                          /* F5 */
                    hangup = true;
                    app->comm->dtr( false );
                    break;
                case 88:                          /* Shift-F5 */
                    i1 = (rand() % 20) + 10;
                    for (i = 0; i < i1; i++)
                    {
                        bputch( static_cast< unsigned char > ( rand() % 256 ) );
                    }
                    hangup = true;
                    app->comm->dtr( false );
                    break;
                case 98:                          /* Ctrl-F5 */
                    sess->bout << "\r\nCall back later when you are there.\r\n\n";
                    hangup = true;
                    app->comm->dtr( false );
                    break;
                case 64:                          /* F6 */
                    ToggleSysopAlert();
                    tleft( false );
                    break;
                case 65:                          /* F7 */
                    sess->thisuser.SetExtraTime( sess->thisuser.GetExtraTime() -
                                                 static_cast<float>( 5.0 * SECONDS_PER_MINUTE_FLOAT ) );
                    tleft( false );
                    break;
                case 66:                          /* F8 */
                    sess->thisuser.SetExtraTime( sess->thisuser.GetExtraTime() +
                                                 static_cast<float>( 5.0 * SECONDS_PER_MINUTE_FLOAT ) );
                    tleft( false );
                    break;
                case 67:                          /* F9 */
                    if ( sess->thisuser.GetSl() != 255 )
                    {
                        if ( sess->GetEffectiveSl() != 255)
                        {
                            sess->SetEffectiveSl( 255 );
                        }
                        else
                        {
                            sess->ResetEffectiveSl();
                        }
                        changedsl();
                        tleft( false );
                    }
                    break;
                case 68:                          /* F10 */
                    if (chatting == 0)
                    {
                        if (syscfg.sysconfig & sysconfig_2_way)
                        {
                            chat1("", true);
                        }
                        else
                        {
                            chat1("", false);
                        }
                    }
                    else
                    {
                        chatting = 0;
                    }
                    break;
/* No need on a multi-tasking OS
                case 93:                          // Shift-F10
                    ExecuteTemporaryCommand(getenv("COMSPEC"));
                    break;
*/
                case 103:                         /* Ctrl-F10 */
                    if (chatting == 0)
                    {
                        chat1("", false);
                    }
                    else
                    {
                        chatting = 0;
                    }
                    break;
                case 71:                          /* HOME */
                    if (chatting == 1)
                    {
                        chat_file = !chat_file;
                    }
                    break;
                default:
                    alt_key( static_cast<unsigned char>( ch ) );
                    break;
                }
            }
        }
        else
        {
            alt_key((unsigned char) ch);
        }
    }
}


static const char * pszTopScrItems[] =
{
    "Comm Disabled",
    "Temp Sysop",
    "Capture",
    "Alert",
    "�������",
    "Available",
    "�����������",
    "%s chatting with %s"
};


void WLocalIO::tleft(bool bCheckForTimeOut)
{
    static char sbuf[200];
    static char *ss[8];

    if (!sbuf[0])
    {
        ss[0] = sbuf;
        for (int i = 0; i < 7; i++)
        {
            strcpy(ss[i], pszTopScrItems[i]);
            ss[i + 1] = ss[i] + strlen(ss[i]) + 1;
        }
    }
    int cx = WhereX();
    int cy = WhereY();
    int ctl = sess->topline;
    int cc = curatr;
    curatr = sess->GetTopScreenColor();
    sess->topline = 0;
    double nsln = nsl();
    int nLineNumber = (chatcall && (sess->topdata == WLocalIO::topdataUser)) ? 5 : 4;


    if (sess->topdata)
    {
        if ((sess->using_modem) && (!incom))
        {
            LocalXYPuts( 1, nLineNumber, ss[0] );
			for ( std::string::size_type i = 19; i < sess->GetCurrentSpeed().length(); i++ )
            {
                LocalPutch( static_cast< unsigned char > ( '�' ) );
            }
        }
        else
        {
            LocalXYPuts( 1, nLineNumber, sess->GetCurrentSpeed().c_str() );
            for (int i = WhereX(); i < 23; i++)
            {
                LocalPutch( static_cast< unsigned char > ( '�' ) );
            }
        }

        if ((sess->thisuser.GetSl() != 255) && ( sess->GetEffectiveSl() == 255))
        {
            LocalXYPuts( 23, nLineNumber, ss[1] );
        }
		if ( fileGlobalCap.IsOpen() )
        {
            LocalXYPuts( 40, nLineNumber, ss[2] );
        }
        if (GetSysopAlert())
        {
            LocalXYPuts( 54, nLineNumber, ss[3] );
        }
        else
        {
            LocalXYPuts( 54, nLineNumber, ss[4] );
        }

        if (sysop1())
        {
            LocalXYPuts( 64, nLineNumber, ss[5] );
        }
        else
        {
            LocalXYPuts( 64, nLineNumber, ss[6] );
        }
    }
    switch (sess->topdata)
    {
    case WLocalIO::topdataSystem:
        if ( sess->IsUserOnline() )
        {
            LocalXYPrintf( 18, 3, "T-%6.2f", nsln / SECONDS_PER_MINUTE_FLOAT );
        }
        break;
    case WLocalIO::topdataUser:
        {
			if ( sess->IsUserOnline() )
            {
                LocalXYPrintf( 18, 3, "T-%6.2f", nsln / SECONDS_PER_MINUTE_FLOAT );
            }
            else
            {
                LocalXYPrintf( 18, 3, sess->thisuser.GetPassword() );
            }
        }
        break;
    }
    sess->topline = ctl;
    curatr = cc;
    LocalGotoXY( cx, cy );
    if ( bCheckForTimeOut && sess->IsUserOnline() )
    {
        if ( nsln == 0.0 )
        {
            sess->bout << "\r\nTime expired.\r\n\n";
            hangup = true;
        }
    }
}


void WLocalIO::UpdateTopScreenImpl()
{
    char i;
    char sl[82], ar[17], dar[17], restrict[17], rst[17], lo[90];

    int lll = lines_listed;

    if ( so() && !incom )
    {
        sess->topdata = WLocalIO::topdataNone;
    }

    if ( syscfg.sysconfig & sysconfig_titlebar )
    {
        // Only set the titlebar if the user wanted it that way.
        char szConsoleTitle[ 255 ];
        sprintf( szConsoleTitle, "WWIV Node %d (User: %s)", app->GetInstanceNumber(), sess->thisuser.GetUserNameAndNumber( sess->usernum ) );
        ::SetConsoleTitle( szConsoleTitle );
    }

    switch ( sess->topdata )
    {
    case WLocalIO::topdataNone:
        set_protect( 0 );
        break;
    case WLocalIO::topdataSystem:
        set_protect( 5 );
        break;
    case WLocalIO::topdataUser:
        if ( chatcall )
        {
            set_protect( 6 );
        }
        else
        {
            if ( sess->topline == 6 )
            {
                set_protect( 0 );
            }
            set_protect( 5 );
        }
        break;
    }
    int cx = WhereX();
    int cy = WhereY();
    int nOldTopLine = sess->topline;
    int cc = curatr;
    curatr = sess->GetTopScreenColor();
    sess->topline = 0;
    for ( i = 0; i < 80; i++ )
    {
        sl[i] = '\xCD';
    }
    sl[80] = '\0';

    switch (sess->topdata)
    {
    case WLocalIO::topdataNone:
        break;
    case WLocalIO::topdataSystem:
        {
            app->statusMgr->Read();
            LocalXYPrintf( 0, 0, "%-50s  Activity for %8s:      ", syscfg.systemname, status.date1 );

            LocalXYPrintf( 0, 1, "Users: %4u       Total Calls: %5lu      Calls Today: %4u    Posted      :%3u ",
                           status.users, status.callernum1, status.callstoday, status.localposts );

            LocalXYPrintf( 0, 2, "%-36s      %-4u min   /  %2u%%    E-mail sent :%3u ",
                           sess->thisuser.GetUserNameAndNumber( sess->usernum ),
                           status.activetoday,
                           static_cast<int>( 10 * status.activetoday / 144 ),
                           status.emailtoday );

            LocalXYPrintf( 0, 3, "SL=%3u   DL=%3u               FW=%3u      Uploaded:%2u files    Feedback    :%3u ",
                           sess->thisuser.GetSl(), sess->thisuser.GetDsl(),
                           fwaiting, status.uptoday, status.fbacktoday );
        }
        break;
    case WLocalIO::topdataUser:
        {
            strcpy(rst, restrict_string);
            for (i = 0; i <= 15; i++)
            {
                if ( sess->thisuser.hasArFlag( 1 << i ) )
                {
                    ar[i] = static_cast< char > ('A' + i );
                }
                else
                {
                    ar[i] = SPACE;
                }
                if ( sess->thisuser.hasDarFlag( 1 << i ) )
                {
                    dar[i] = static_cast< char > ( 'A' + i );
                }
                else
                {
                    dar[i] = SPACE;
                }
                if ( sess->thisuser.hasRestrictionFlag( 1 << i ) )
                {
                    restrict[i] = rst[i];
                }
                else
                {
                    restrict[i] = SPACE;
                }
            }
            dar[16] = '\0';
            ar[16] = '\0';
            restrict[16] = '\0';
            if ( !wwiv::stringUtils::IsEquals( sess->thisuser.GetLastOn(), date() ) )
            {
                strcpy( lo, sess->thisuser.GetLastOn() );
            }
            else
            {
                sprintf( lo, "Today:%2d", sess->thisuser.GetTimesOnToday() );
            }

            LocalXYAPrintf( 0, 0, curatr, "%-35s W=%3u UL=%4u/%6lu SL=%3u LO=%5u PO=%4u",
                            sess->thisuser.GetUserNameAndNumber( sess->usernum ),
                            sess->thisuser.GetNumMailWaiting(),
                            sess->thisuser.GetFilesUploaded(),
                            sess->thisuser.GetUploadK(),
                            sess->thisuser.GetSl(),
                            sess->thisuser.GetNumLogons(),
                            sess->thisuser.GetNumMessagesPosted() );

            char szCallSignOrRegNum[ 41 ];
            if ( sess->thisuser.GetWWIVRegNumber() )
            {
                sprintf( szCallSignOrRegNum, "%lu", sess->thisuser.GetWWIVRegNumber() );
            }
            else
            {
                strcpy( szCallSignOrRegNum, sess->thisuser.GetCallsign() );
            }
            LocalXYPrintf(  0, 1, "%-20s %12s  %-6s DL=%4u/%6lu DL=%3u TO=%5.0lu ES=%4u",
                            sess->thisuser.GetRealName(),
                            sess->thisuser.GetVoicePhoneNumber(),
                            szCallSignOrRegNum,
                            sess->thisuser.GetFilesDownloaded(),
                            sess->thisuser.GetDownloadK(),
                            sess->thisuser.GetDsl(),
                            static_cast<long>( ( sess->thisuser.GetTimeOn() + timer() - timeon ) / SECONDS_PER_MINUTE_FLOAT ),
                            sess->thisuser.GetNumEmailSent() + sess->thisuser.GetNumNetEmailSent() );

            LocalXYPrintf( 0, 2, "ARs=%-16s/%-16s R=%-16s EX=%3u %-8s FS=%4u",
                ar, dar, restrict, sess->thisuser.GetExempt(),
                lo, sess->thisuser.GetNumFeedbackSent() );

            LocalXYPrintf( 0, 3, "%-40.40s %c %2u %-16.16s           FW= %3u",
                            sess->thisuser.GetNote(),
                            sess->thisuser.GetGender(),
                            sess->thisuser.GetAge(),
                            ctypes( sess->thisuser.GetComputerType() ), fwaiting );

            if (chatcall)
            {
                LocalXYPuts( 0, 4, m_szChatReason );
            }
        }
        break;
    default:
        break;
    }
    if ( nOldTopLine != 0 )
    {
        LocalXYPuts( 0, nOldTopLine - 1, sl );
    }
    sess->topline = nOldTopLine;
    LocalGotoXY( cx, cy );
    curatr = cc;
    tleft( false );

    lines_listed = lll;
}


/****************************************************************************/


/**
 * IsLocalKeyPressed - returns whether or not a key been pressed at the local console.
 *
 * @return true if a key has been pressed at the local console, false otherwise
 */
bool WLocalIO::LocalKeyPressed()
{
    return (x_only ? 0 : (HasKeyBeenPressed() || ExtendedKeyWaiting)) ? true : false;
}

/****************************************************************************/
/*
* returns the ASCII code of the next character waiting in the
* keyboard buffer.  If there are no characters waiting in the
* keyboard buffer, then it waits for one.
*
* A value of 0 is returned for all extended keys (such as F1,
* Alt-X, etc.).  The function must be called again upon receiving
* a value of 0 to obtain the value of the extended key pressed.
*/
unsigned char WLocalIO::getchd()
{
    if (ExtendedKeyWaiting)
    {
        ExtendedKeyWaiting = 0;
        return GetKeyboardChar();
    }
    unsigned char rc = GetKeyboardChar();
    if ( rc == 0 || rc == 0xe0 )
    {
        ExtendedKeyWaiting = 1;
    }
    return rc;
}


/****************************************************************************/
/*
* returns the ASCII code of the next character waiting in the
* keyboard buffer.  If there are no characters waiting in the
* keyboard buffer, then it returns immediately with a value
* of 255.
*
* A value of 0 is returned for all extended keys (such as F1,
* Alt-X, etc.).  The function must be called again upon receiving
* a value of 0 to obtain the value of the extended key pressed.
*/
unsigned char WLocalIO::getchd1()
{
    if ( !( HasKeyBeenPressed() || ExtendedKeyWaiting ) )
    {
        return 255;
    }
    if (ExtendedKeyWaiting)
    {
        ExtendedKeyWaiting = 0;
        return GetKeyboardChar();
    }
    unsigned char rc = GetKeyboardChar();
    if ( rc == 0 || rc == 0xe0 )
    {
        ExtendedKeyWaiting = 1;
    }
    return rc;
}


void WLocalIO::SaveCurrentLine(char *cl, char *atr, char *xl, char *cc)
{
    *cc = (char) curatr;
    strcpy(xl, endofline);
    {
        WORD Attr[80];
        CONSOLE_SCREEN_BUFFER_INFO ConInfo;
        GetConsoleScreenBufferInfo(m_hConOut,&ConInfo);

        int len = ConInfo.dwCursorPosition.X;
        ConInfo.dwCursorPosition.X = 0;
        DWORD cb;
        ReadConsoleOutputCharacter(m_hConOut,cl,len,ConInfo.dwCursorPosition,&cb);
        ReadConsoleOutputAttribute(m_hConOut,Attr,len,ConInfo.dwCursorPosition,&cb);

        for (int i = 0; i < len; i++)
        {
            atr[i] = (char) Attr[i]; // atr is 8bit char, Attr is 16bit
        }
    }
    cl[app->localIO->WhereX()]	= 0;
    atr[app->localIO->WhereX()] = 0;
}


/**
 * LocalGetChar - gets character entered at local console.
 *                <B>Note: This is a blocking function call.</B>
 *
 * @return int value of key entered
 */
int  WLocalIO::LocalGetChar()
{
	return GetKeyboardChar();
}


void WLocalIO::MakeLocalWindow(int x, int y, int xlen, int ylen)
{
    // Make sure that we are within the range of {(0,0), (80,sess->screenbottom)}
	xlen = std::min<int>( xlen, 80 );
    if (ylen > (sess->screenbottom + 1 - sess->topline))
    {
        ylen = (sess->screenbottom + 1 - sess->topline);
    }
    if ((x + xlen) > 80)
    {
        x = 80 - xlen;
    }
    if ((y + ylen) > sess->screenbottom + 1)
    {
        y = sess->screenbottom + 1 - ylen;
    }

    int xx = WhereX();
    int yy = WhereY();

    // we expect to be offset by sess->topline
    y += sess->topline;

    // large enough to hold 80x50
    CHAR_INFO ci[4000];

    // pos is the position within the buffer
    COORD pos = {0, 0};
    COORD size;
    SMALL_RECT rect;

    // rect is the area on screen the buffer is to be drawn
    rect.Top    = static_cast< short > ( y );
    rect.Left   = static_cast< short > ( x );
    rect.Right  = static_cast< short > ( xlen + x - 1 );
    rect.Bottom = static_cast< short > ( ylen + y - 1 );

    // size of the buffer to use (lower right hand coordinate)
    size.X      = static_cast< short > ( xlen );
    size.Y      = static_cast< short > ( ylen );

    // our current position within the CHAR_INFO buffer
    int nCiPtr  = 0;

    //
    // Loop through Y, each time looping through X adding the right character
    //

    for (int yloop = 0; yloop<size.Y; yloop++)
    {
        for (int xloop=0; xloop<size.X; xloop++)
        {
            ci[nCiPtr].Attributes = static_cast< short > ( curatr );
            if ((yloop==0) || (yloop==size.Y-1))
            {
                ci[nCiPtr].Char.AsciiChar   = '\xC4';      // top and bottom
            }
            else
            {
                if ((xloop==0) || (xloop==size.X-1))
                {
                    ci[nCiPtr].Char.AsciiChar   = '\xB3';  // right and left sides
                }
                else
                {
                    ci[nCiPtr].Char.AsciiChar   = '\x20';   // nothing... Just filler (space)
                }
            }
            nCiPtr++;
        }
    }

    //
    // sum of the lengths of the previous lines (+0) is the start of next line
    //

    ci[0].Char.AsciiChar                    = '\xDA';      // upper left
    ci[xlen-1].Char.AsciiChar               = '\xBF';      // upper right

    ci[xlen*(ylen-1)].Char.AsciiChar        = '\xC0';      // lower left
    ci[xlen*(ylen-1)+xlen-1].Char.AsciiChar = '\xD9';      // lower right

    //
    // Send it all to the screen with 1 WIN32 API call (Windows 95's console mode API support
    // is MUCH slower than NT/Win2k, therefore it is MUCH faster to render the buffer off
    // screen and then write it with one fell swoop.
    //

    WriteConsoleOutput(m_hConOut, ci, size, pos, &rect);

    //
    // Draw shadow around boxed window
    //
    for (int i = 0; i < xlen; i++)
    {
        set_attr_xy(x + 1 + i, y + ylen, 0x08);
    }

    for (int i1 = 0; i1 < ylen; i1++)
    {
        set_attr_xy(x + xlen, y + 1 + i1, 0x08);
    }

    LocalGotoXY(xx, yy);

}


void WLocalIO::SetCursor(UINT cursorStyle)
{
	CONSOLE_CURSOR_INFO cursInfo;

	switch (cursorStyle)
	{
		case WLocalIO::cursorNone:
		{
			cursInfo.dwSize = 20;
			cursInfo.bVisible = false;
		    SetConsoleCursorInfo(m_hConOut, &cursInfo);
		}
		break;
		case WLocalIO::cursorSolid:
		{
			cursInfo.dwSize = 100;
		    cursInfo.bVisible = true;
		    SetConsoleCursorInfo(m_hConOut, &cursInfo);
		}
		break;
		case WLocalIO::cursorNormal:
		default:
		{
			cursInfo.dwSize = 20;
		    cursInfo.bVisible = true;
			SetConsoleCursorInfo(m_hConOut, &cursInfo);
		}
	}
}


void WLocalIO::LocalClrEol()
{
	CONSOLE_SCREEN_BUFFER_INFO ConInfo;
	DWORD cb;
	int len = 80 - app->localIO->WhereX();

	GetConsoleScreenBufferInfo(m_hConOut,&ConInfo);
	FillConsoleOutputCharacter(m_hConOut, ' ', len, ConInfo.dwCursorPosition, &cb);
	FillConsoleOutputAttribute(m_hConOut, (WORD) curatr, len, ConInfo.dwCursorPosition, &cb);
}



void WLocalIO::LocalWriteScreenBuffer(const char *pszBuffer)
{
    CHAR_INFO ci[2000];

    COORD pos       = { 0, 0};
    COORD size      = { 80, 25 };
    SMALL_RECT rect = { 0, 0, 79, 24 };

	for(int i=0; i<2000; i++)
	{
        ci[i].Char.AsciiChar = (char) *(pszBuffer + ((i*2)+0));
        ci[i].Attributes     = (unsigned char) *(pszBuffer + ((i*2)+1));
	}
    WriteConsoleOutput(m_hConOut, ci, size, pos, &rect);
}


int WLocalIO::GetDefaultScreenBottom()
{
	return (m_consoleBufferInfo.dwSize.Y - 1);
}


bool HasKeyBeenPressed()
{
    return ( kbhit() ) ? true : false;

#if 0

    PINPUT_RECORD pIRBuf;
    DWORD NumPeeked;
    bool bHasKeyBeenPressed = false;

    DWORD dwNumEvents;  // NumPending
    GetNumberOfConsoleInputEvents( m_hConIn, &dwNumEvents );
    if ( dwNumEvents == 0 )
    {
        return false;
    }

    PINPUT_RECORD pInputRec = ( PINPUT_RECORD ) bbsmalloc( dwNumEvents * sizeof( INPUT_RECORD ) );
    if ( !pInputRec )
    {
        // Really something bad happened here.
        return false;
    }

    DWORD dwNumEventsRead;
    if ( PeekConsoleInput( m_hConIn, pInputRec, dwNumEvents, &dwNumEventsRead ) )
    {
        for( int i=0; i < dwNumEventsRead; i++ )
        {
            if ( pInputRec->EventType == KEY_EVENT &&
                pInputRec->Event.KeyEvent.bKeyDown &&
                ( pInputRec->Event.KeyEvent.uChar.AsciiChar ||
                IsExtendedKeyCode( pInputRec->Event.KeyEvent ) )
            {
                bHasKeyBeenPressed = true;
                break;
            }
            pInputRec++;
        }
    }

        // Scan all of the peeked events to determine if any is a key event
        // which should be recognized.
        for ( ; NumPeeked > 0 ; NumPeeked--, pIRBuf++ )
		{
            if ( (pIRBuf->EventType == KEY_EVENT) &&
                (pIRBuf->Event.KeyEvent.bKeyDown) &&
                ( pIRBuf->Event.KeyEvent.uChar.AsciiChar ||
                _getextendedkeycode( &(pIRBuf->Event.KeyEvent) ) ) )
            {
                // Key event corresponding to an ASCII character or an
                // extended code. In either case, success!
                ret = TRUE;
            }
        }

    BbsFreeMemory( pInputRec );

    return bHasKeyBeenPressed;
#endif

}


unsigned char GetKeyboardChar()
{
    return static_cast< unsigned char >( getch() );
}

void WLocalIO::LocalEditLine( char *s, int len, int status, int *returncode, char *ss )
{
    WWIV_ASSERT(s);
    WWIV_ASSERT(ss);

    int oldatr = curatr;
    int cx = WhereX();
    int cy = WhereY();
    for (int i = strlen(s); i < len; i++)
    {
        s[i] = static_cast<unsigned char>( 176 );
    }
    s[len] = '\0';
    curatr = sess->GetEditLineColor();
    LocalFastPuts( s );
    LocalGotoXY( cx, cy );
    bool done = false;
    int pos = 0;
    bool insert = false;
    do
    {
        unsigned char ch = getchd();
        if ( ch == 0 || ch == 224 )
        {
            ch = getchd();
            switch (ch)
            {
            case F1:
                done = true;
                *returncode = DONE;
                break;
            case HOME:
                pos = 0;
                LocalGotoXY( cx, cy );
                break;
            case END:
                pos = GetEditLineStringLength( s ); // len;
                LocalGotoXY( cx + pos, cy );
                break;
            case RARROW:
                if ( pos < GetEditLineStringLength( s ) )
                {
                    pos++;
                    LocalGotoXY( cx + pos, cy );
                }
                break;
            case LARROW:
                if ( pos > 0 )
                {
                    pos--;
                    LocalGotoXY( cx + pos, cy );
                }
                break;
            case UARROW:
            case CO:
                done = true;
                *returncode = PREV;
                break;
            case DARROW:
                done = true;
                *returncode = NEXT;
                break;
            case INSERT:
                if (status != SET)
                {
                    insert = !insert;
                }
                break;
            case KEY_DELETE:
                if (status != SET)
                {
                    for (int i = pos; i < len; i++)
                    {
                        s[i] = s[i + 1];
                    }
                    s[len - 1] = static_cast<unsigned char>( 176 );
                    LocalXYPuts( cx, cy, s );
                    LocalGotoXY(cx + pos, cy);
                }
                break;
            }
        }
        else
        {
            if (ch > 31)
            {
                if (status == UPPER_ONLY)
                {
                    ch = wwiv::UpperCase<unsigned char>(ch);
                }
                if (status == SET)
                {
                    ch = wwiv::UpperCase<unsigned char>(ch);
                    if (ch != SPACE)
                    {
                        bool bLookingForSpace = true;
                        for (int i = 0; i < len; i++)
                        {
                            if ( ch == ss[i] && bLookingForSpace )
                            {
                                bLookingForSpace = false;
                                pos = i;
                                LocalGotoXY(cx + pos, cy);
                                if (s[pos] == SPACE)
                                {
                                    ch = ss[pos];
                                }
                                else
                                {
                                    ch = SPACE;
                                }
                            }
                        }
                        if ( bLookingForSpace )
                        {
                            ch = ss[pos];
                        }
                    }
                }
                if ((pos < len) && ((status == ALL) || (status == UPPER_ONLY) || (status == SET) ||
                    ((status == NUM_ONLY) && (((ch >= '0') && (ch <= '9')) || (ch == SPACE)))))
                {
                    if (insert)
                    {
                        for (int i = len - 1; i > pos; i--)
                        {
                            s[i] = s[i - 1];
                        }
                        s[pos++] = ch;
                        LocalXYPuts( cx, cy, s );
                        LocalGotoXY( cx + pos, cy );
                    }
                    else
                    {
                        s[pos++] = ch;
                        LocalPutch(ch);
                    }
                }
            }
            else
            {
                // ch is > 32
                switch (ch)
                {
                case RETURN:
                case TAB:
                    done = true;
                    *returncode = NEXT;
                    break;
                case ESC:
                    done = true;
                    *returncode = ABORTED;
                    break;
                case CA:
                    pos = 0;
                    LocalGotoXY( cx, cy );
                    break;
                case CE:
                    pos = GetEditLineStringLength( s ); // len;
                    LocalGotoXY( cx + pos, cy );
                    break;
                case BACKSPACE:
                    if (pos > 0)
                    {
                        if (insert)
                        {
                            for ( int i = pos - 1; i < len; i++ )
                            {
                                s[i] = s[i + 1];
                            }
                            s[len - 1] = static_cast<unsigned char>( 176 );
                            pos--;
                            LocalXYPuts( cx, cy, s );
                            LocalGotoXY(cx + pos, cy);
                        }
                        else
                        {
                            int nStringLen = GetEditLineStringLength( s );
                            pos--;
                            if ( pos == ( nStringLen - 1 ) )
                            {
                                s[pos] = static_cast<unsigned char>( 176 );
                            }
                            else
                            {
                                s[pos] = SPACE;
                            }
                            LocalXYPuts( cx, cy, s );
                            LocalGotoXY(cx + pos, cy);
                        }
                    }
                    break;
                }
            }
        }
    } while ( !done );

    int z = strlen( s );
    while ( z >= 0 && static_cast<unsigned char>( s[z-1] ) == 176 )
    {
        --z;
    }
    s[z] = '\0';

    char szFinishedString[ 260 ];
    sprintf( szFinishedString, "%-255s", s );
    szFinishedString[ len ] = '\0';
    LocalGotoXY( cx, cy );
    curatr=oldatr;
    LocalFastPuts( szFinishedString );
    LocalGotoXY( cx, cy );
}


int WLocalIO::GetEditLineStringLength( const char *pszText )
{
	int i = strlen( pszText );
	while ( i >= 0 && ( /*pszText[i-1] == 32 ||*/ static_cast<unsigned char>( pszText[i-1] ) == 176 ) )
	{
		--i;
	}
	return i;
}




