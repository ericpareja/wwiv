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


int t_now()
{
    time_t t = time( NULL );
    struct tm * pTm = localtime( &t );
    return ( pTm->tm_hour * 60 ) + pTm->tm_min;
}


char *ttc(int d)
{
    static char ch[7];

    int h = d / 60;
    int m = d - (h * 60);
    sprintf( ch, "%02d:%02d", h, m );
    return ch;
}

void sort_events()
{
    // keeping events sorted in time order makes things easier.
    for ( int i = 0; i < (sess->num_events - 1); i++ )
    {
        int z = i;
        for ( int j = ( i + 1 ); j < sess->num_events; j++ )
        {
            if ( events[j].time < events[z].time )
            {
                z = j;
            }
        }
        if ( z != i )
        {
            eventsrec e;
            e = events[i];
            events[i] = events[z];
            events[z] = e;
        }
    }
}

void init_events()
{
    if ( events )
	{
        BbsFreeMemory( events );
		events = NULL;
	}
    events = static_cast<eventsrec *>( BbsAllocWithComment(MAX_EVENT * sizeof(eventsrec), "external events") );
    WWIV_ASSERT( events != NULL );

    WFile file( syscfg.datadir, EVENTS_DAT );
    if ( file.Open( WFile::modeBinary | WFile::modeReadOnly ) )
	{
        sess->num_events = file.GetLength() / sizeof( eventsrec );
        file.Read( events, sess->num_events * sizeof( eventsrec ) );
        get_next_forced_event();
    }
	else
	{
        sess->num_events = 0;
	}
}


void get_next_forced_event()
{
    syscfg.executetime = 0;
    time_event = 0.0;
    int first = -1;
    int tl = t_now();
    int day = dow() + 1;
    if ( day == 7 )
    {
        day = 0;
    }
    for ( int i = 0; i < sess->num_events; i++ ) 
    {
        if ( ( events[i].instance == app->GetInstanceNumber() || events[i].instance == 0 ) &&
             events[i].status & EVENT_FORCED )
        {
            if ( first < 0 && events[i].time < tl && ( ( events[i].days & ( 1 << day ) ) > 0 ) )
            {
                first = events[i].time;
            }
            if ( ( events[i].status & EVENT_RUNTODAY ) == 0 && ( events[i].days & ( 1 << dow() ) ) > 0 && !syscfg.executetime ) 
            {
                time_event = static_cast<double>( events[i].time ) * SECONDS_PER_MINUTE_FLOAT;
                syscfg.executetime = events[i].time;
                if ( !syscfg.executetime )
                {
                    ++syscfg.executetime;
                }
            }
        }
    }
    if ( first >= 0 && !syscfg.executetime )
	{ 
        // all of todays events are
        time_event = static_cast<double>( first ) * SECONDS_PER_MINUTE_FLOAT;   // complete, set next forced
        syscfg.executetime = static_cast<unsigned short>( first );              // event to first one
        if ( !syscfg.executetime )                                              // scheduled for tomorrow
		{
            ++syscfg.executetime;
		}
    }
}


void cleanup_events()
{
    if ( !sess->num_events )
    {
        return;
    }

    // since the date has changed, make sure all events for yesterday have been
    // run, then clear all status to "not run" note in this case all events end up
    // running on the same node, but this is preferable to not running at all
    int day = dow() - 1;
    if ( day < 0 )
    {
        day = 6;
    }

    int i;
    for ( i = 0; i < sess->num_events; i++ )
    {
        if (((events[i].status & EVENT_RUNTODAY) == 0) &&
            ((events[i].days & (1 << day)) > 0))
        {
            run_event( i );
        }
    }
    for ( i = 0; i < sess->num_events; i++ )
    {
        events[i].status &= ~EVENT_RUNTODAY;
    }

    WFile eventsFile( syscfg.datadir, EVENTS_DAT );
    eventsFile.Open( WFile::modeReadWrite|WFile::modeBinary, WFile::shareUnknown, WFile::permReadWrite );
    eventsFile.Write( events, sess->num_events * sizeof( eventsrec ) );
    eventsFile.Close();
}


void check_event()
{
    int i;

    int tl = t_now();
    for ( i = 0; i < sess->num_events && !do_event; i++ )
    {
        if (((events[i].status & EVENT_RUNTODAY) == 0) && (events[i].time <= tl) &&
            ((events[i].days & (1 << dow())) > 0) &&
            ((events[i].instance == app->GetInstanceNumber()) ||
            (events[i].instance == 0)))
        {
            // make sure the event hasn't already been executed on another node,then mark it as run
            WFile eventsFile( syscfg.datadir, EVENTS_DAT );
            eventsFile.Open( WFile::modeReadWrite|WFile::modeBinary, WFile::shareUnknown, WFile::permReadWrite );
            eventsFile.Seek( i * sizeof( eventsrec ), WFile::seekBegin );
            eventsFile.Read( &events[i], sizeof( eventsrec ) );

            if ((events[i].status & EVENT_RUNTODAY) == 0)
            {
                events[i].status |= EVENT_RUNTODAY;
                eventsFile.Seek( i * sizeof( eventsrec ), WFile::seekBegin );
                eventsFile.Write( &events[i], sizeof( eventsrec ) );
                do_event = i + 1;
            }
            eventsFile.Close();
        }
    }
}

void run_event( int evnt )
{
    int exitlevel;

    write_inst(INST_LOC_EVENT, 0, INST_FLAGS_NONE);
#ifndef _UNIX
    app->localIO->SetCursor( WLocalIO::cursorNormal );
#endif
    ClearScreen();
    sess->bout << "\r\nNow running external event.\r\n\n";
    if (events[evnt].status & EVENT_HOLD)
    {
        holdphone( true );
    }
    if (events[evnt].status & EVENT_EXIT)
	{
        exitlevel = static_cast<int>( events[evnt].cmd[0] );
        close_strfiles();
        if ( ok_modem_stuff && app->comm != NULL )
        {
            app->comm->close();
        }
        exit( exitlevel );
    }
    ExecuteExternalProgram( events[evnt].cmd, EFLAG_NONE );
    do_event = 0;
    get_next_forced_event();
    holdphone( false );
#ifndef _UNIX
    wfc_cls();
#endif
}

void show_events()
{
    char s[121], s1[81], daystr[8];

    ClearScreen();
    bool abort = false;
    char y = "Yes"[0];
    char n = "No"[0];
    pla("|#1                                         Hold   Force   Run", &abort);
    pla("|#1Evnt Time  Command                 Node  Phone  Event  Today  Shrink", &abort);
    pla("|#7=============================================================================", &abort);
    for (int i = 0; (i < sess->num_events) && (!abort); i++)
    {
        if (events[i].status & EVENT_EXIT)
        {
            sprintf(s1, "Exit Level = %d", events[i].cmd[0]);
        }
        else
        {
            strcpy(s1, events[i].cmd);
        }
        strcpy(daystr, "SMTWTFS");
        for (int j = 0; j <= 6; j++)
        {
            if ((events[i].days & (1 << j))==0)
            {
                daystr[j] = ' ';
            }
        }
        sprintf(s, " %2d  %-5.5s %-23.23s  %2d     %1c      %1c      %1c      %1c     %s",
                    i, ttc(events[i].time), s1, events[i].instance,
                    events[i].status & EVENT_HOLD ? y : n,
                    events[i].status & EVENT_FORCED ? y : n,
                    events[i].status & EVENT_RUNTODAY ? y : n,
                    events[i].status & EVENT_SHRINK ? y : n,
                    daystr);
        pla(s, &abort);
    }
}


void select_event_days(int evnt)
{
    int i;
    char ch, daystr[8], days[8];

    nl();
    strcpy(days, "SMTWTFS");
    for (i = 0; i <= 6; i++)
	{
        if (events[evnt].days & (1 << i))
		{
            daystr[i] = days[i];
		}
        else
		{
            daystr[i] = ' ';
		}
	}
    daystr[8] = '\0';
    sess->bout << "Enter number to toggle day of the week, 'Q' to quit.\r\n\n";
    sess->bout << "                   1234567\r\n";
    sess->bout << "Days to run event: ";
    do
	{
        sess->bout << daystr;
        ch = onek_ncr("1234567Q");
        if ((ch >= '1') && (ch <= '7'))
		{
            i = ch - '1';
            events[evnt].days ^= (1 << i);
            if (events[evnt].days & (1 << i))
			{
                daystr[i] = days[i];
			}
            else
			{
                daystr[i] = ' ';
			}
            sess->bout << "\b\b\b\b\b\b\b";
        }
    } while ( ch != 'Q' && !hangup );
}


void modify_event( int evnt )
{
    char s[81], s1[81], ch;
    int j;

    bool ok     = true;
    bool done   = false;
    int i       = evnt;
    do
    {
        ClearScreen();
		sess->bout << "A) Event Time......: " << ttc(events[i].time) << wwiv::endl;
        if (events[i].status & EVENT_EXIT)
        {
            sprintf(s1, "Exit BBS with DOS Errorlevel %d", events[i].cmd[0]);
        }
        else
        {
            sprintf(s1, events[i].cmd );
        }
		sess->bout << "B) Event Command...: " << s1 << wwiv::endl;
		sess->bout << "C) Phone Off Hook?.: " << ( ( events[i].status & EVENT_HOLD ) ? "Yes" : "No" ) << wwiv::endl;
		sess->bout << "D) Already Run?....: " << ( ( events[i].status & EVENT_RUNTODAY ) ? "Yes" : "No" ) << wwiv::endl;
		sess->bout << "E) Shrink?.........: " << ( ( events[i].status & EVENT_SHRINK ) ? "Yes" : "No" ) << wwiv::endl;
		sess->bout << "F) Force User Off?.: " << ( ( events[i].status & EVENT_FORCED ) ? "Yes" : "No" ) << wwiv::endl;
        strcpy( s1, "SMTWTFS" );
        for ( j = 0; j <= 6; j++ )
        {
            if ( ( events[i].days & ( 1 << j ) ) == 0 )
            {
                s1[j] = ' ';
            }
        }
		sess->bout << "G) Days to Execute.: " << s1 << wwiv::endl;
		sess->bout << "H) Node (0=Any)....: " << events[i].instance << wwiv::endl;
        nl();
		sess->bout << "|#5Which? |#7[|#1A-H,[,],Q=Quit|#7] |#0: ";
        ch = onek( "QABCDEFGH[]" );
        switch ( ch )
        {
        case 'Q':
            done = true;
            break;
        case ']':
            i++;
            if ( i >= sess->num_events )
            {
                i = 0;
            }
            break;
        case '[':
            i--;
            if ( i < 0 )
            {
                i = sess->num_events - 1;
            }
            break;
        case 'A':
            nl();
			sess->bout << "|#2Enter event times in 24 hour format. i.e. 00:01 or 15:20\r\n";
			sess->bout << "|#2Event time? ";
            ok = true;
            j = 0;
            do
            {
                if ( j == 2 )
                {
                    s[j++] = ':';
                    bputch( ':' );
                }
                else
                {
                    switch ( j )
                    {
                    case 0:
                        ch = onek_ncr( "012\r" );
                        break;
                    case 3:
                        ch = onek_ncr( "012345\b" );
                        break;
                    case 5:
                        ch = onek_ncr( "\b\r" );
                        break;
                    case 1:
                        if ( s[0] == '2' )
                        {
                            ch = onek_ncr( "0123\b" );
                            break;
                        }
                    default: ch = onek_ncr( "0123456789\b" );
                        break;
                    }
                    if ( hangup )
                    {
                        ok = false;
                        s[0] = '\0';
                        break;
                    }
                    switch ( ch )
                    {
                    case '\r':
                        switch ( j )
                        {
                        case 0:
                            ok = false;
                            break;
                        case 5:
                            s[5] = '\0';
                            break;
                        default:
                            ch = 0;
                            break;
                        }
                        break;
                        case '\b':
                            sess->bout << " \b";
                            --j;
                            if ( j == 2 )
                            {
                                BackSpace();
                                --j;
                            }
                            break;
                        default:
                            s[j++] = ch;
                            break;
                    }
                }
            } while ( ch != '\r' && !hangup );
            if ( ok )
            {
                events[i].time = static_cast<short>( ( 60 * atoi(s) ) + atoi(&(s[3]) ) );
            }
            break;
        case 'B':
            nl();
			sess->bout << "|#2Exit BBS for event? ";
            if ( yesno() )
            {
                events[i].status |= EVENT_EXIT;
				sess->bout << "|#2DOS ERRORLEVEL on exit? ";
                input( s, 3 );
                j = atoi( s );
                if ( s[0] != 0 && j >= 0 && j < 256 )
                {
                    events[i].cmd[0] = static_cast<char>( j );
                }
            }
            else
            {
                events[i].status &= ~EVENT_EXIT;
                sess->bout << "|#2Commandline to run? ";
                input( s, 80 );
                if ( s[0] != '\0' )
                {
                    strcpy( events[i].cmd, s );
                }
            }
            break;
        case 'C':
            events[i].status ^= EVENT_HOLD;
            break;
        case 'D':
            events[i].status ^= EVENT_RUNTODAY;
            break;
        case 'E':
            events[i].status ^= EVENT_SHRINK;
            break;
        case 'F':
            events[i].status ^= EVENT_FORCED;
            break;
        case 'G':
            nl();
			sess->bout << "|#2Run event every day? ";
            if ( noyes( ))
            {
                events[i].days = 127;
            }
            else
            {
                select_event_days(i);
            }
            break;
        case 'H':
            nl();
            sess->bout << "|#2Run event on which node (0=any)? ";
            input( s, 3 );
            j = atoi( s );
            if ( s[0] != '\0' && j >= 0 && j < 1000 )
            {
                events[i].instance = static_cast<short>( j );
            }
            break;
    }
  } while ( !done && !hangup );
}


void insert_event()
{
    strcpy(events[sess->num_events].cmd, "**New Event**");
    events[sess->num_events].time = 0;
    events[sess->num_events].status = 0;
    events[sess->num_events].instance = 0;
    events[sess->num_events].days = 127;                // Default to all 7 days
    modify_event(sess->num_events);
    ++sess->num_events;
}


void delete_event(int n)
{
    for ( int i = n; i < sess->num_events; i++ )
	{
        events[i] = events[i + 1];
	}
    --sess->num_events;
}


void eventedit()
{
    char s[81];

    if ( !ValidateSysopPassword() )
    {
        return;
    }
    bool done = false;
    do
	{
		char ch = 0;
        show_events();
        nl();
        sess->bout << "|#9Events: |#1I|#9nsert, |#1D|#9elete, |#1M|#9odify, e|#1X|#9ecute, |#1S|#2ystem Events|#9, |#1Q|#9uit :";
        if ( so() )
		{
            ch = onek( "QDIMS?X" );
		}
        else
		{
            ch = onek( "QDIM?" );
		}
        switch ( ch )
		{
        case '?':
            show_events();
            break;
        case 'Q':
            done = true;
            break;
        case 'X':
			{
				nl();
				sess->bout << "|#2Run which Event? ";
				input( s, 2 );
				int nEventNum = atoi( s );
				if ( s[0] != '\0' && nEventNum >= 0 && nEventNum < sess->num_events )
				{
					run_event( nEventNum );
				}
			}
            break;
        case 'M':
			{
				nl();
				sess->bout << "|#2Modify which Event? ";
				input( s, 2 );
				int nEventNum = atoi( s );
				if ( s[0] != '\0' && nEventNum >= 0 && nEventNum < sess->num_events )
				{
					modify_event( nEventNum );
				}
			}
            break;
        case 'I':
            if ( sess->num_events < MAX_EVENT )
			{
                insert_event();
			}
            else
			{
                sess->bout << "\r\n|#6Can't add any more events!\r\n\n";
                pausescr();
            }
            break;
        case 'D':
            if ( sess->num_events )
            {
                nl();
                sess->bout << "|#2Delete which Event? ";
                input( s, 2 );
                int nEventNum = atoi( s );
                if ( s[0] && nEventNum >= 0 && nEventNum < sess->num_events )
                {
                    nl();
                    if ( events[nEventNum].status & EVENT_EXIT )
                    {
                        sprintf( s, "Exit Level = %d", events[nEventNum].cmd[0] );
                    }
                    else
                    {
                        strcpy( s, events[nEventNum].cmd );
                    }
                    sess->bout << "|#5Delete " << s << "?";
                    if ( yesno() )
                    {
                        delete_event( nEventNum );
                    }
                }
            }
            else
            {
                sess->bout << "\r\n|#6No events to delete!\r\n\n";
                pausescr();
            }
            break;
        case 'S':
			{
				bool bSysEventsDone = false;

				do
				{
					app->localIO->LocalCls();
                    std::string title = "|B1|15System Events Configuration";
                    bprintf( "%-85s", title.c_str() );
					ansic ( 0 );
					nl( 2 );
					sess->bout << "|#91) Terminal Program     : |#2" << syscfg.terminal     << wwiv::endl;
					sess->bout << "|#93) Begin Day Event      : |#2" << syscfg.beginday_c   << wwiv::endl;
					sess->bout << "|#94) Logon Event          : |#2" << syscfg.logon_c      << wwiv::endl;
					sess->bout << "|#95) Logoff Event         : |#2" << syscfg.logoff_c     << wwiv::endl;
					sess->bout << "|#96) Newuser Event        : |#2" << syscfg.newuser_c    << wwiv::endl;
					sess->bout << "|#97) Upload  Event        : |#2" << syscfg.upload_c     << wwiv::endl;
					sess->bout << "|#98) Virus Scanner CmdLine: |#2" << syscfg.v_scan_c     << wwiv::endl;
					sess->bout << "|#9Q) Quit\r\n";
					nl();
					sess->bout << "|#7(|#2Q|#7=|#1Quit|#7, |#2?|#7=|#1Help|#7) Which? (|#11|#7-|#18|#7) :";
					ch = onek( "Q1345678?" );
					app->localIO->LocalGotoXY( 26, ch - 47 );
					switch( ch )
					{
					case '1':
						Input1( syscfg.terminal, syscfg.terminal, 21, true, UPPER );
						break;
					case '3':
						Input1( syscfg.beginday_c, syscfg.beginday_c, 51, true, UPPER );
						break;
					case '4':
						Input1( syscfg.logon_c, syscfg.logon_c, 51, true, UPPER );
						break;
					case '5':
						Input1( syscfg.logoff_c, syscfg.logoff_c, 51, true, UPPER );
						break;
					case '6':
						Input1( syscfg.newuser_c,syscfg.newuser_c, 51, true, UPPER );
						break;
					case '7':
						Input1( syscfg.upload_c, syscfg.upload_c, 51, true, UPPER );
						break;
					case '8':
						Input1( syscfg.v_scan_c, syscfg.v_scan_c, 51, true, UPPER );
						break;
					case '?':
						app->localIO->LocalCls();
						printfile( CMDPARAM_NOEXT );
						pausescr();
						break;
					case 'Q':
						bSysEventsDone = true;
						app->SaveConfig();
						break;
					}
				} while( !bSysEventsDone );
            }
            break;
        }
    } while ( !done && !hangup );
	sort_events();

	WFile eventsFile( syscfg.datadir, EVENTS_DAT );
	if ( sess->num_events )
	{
		// %%TODO: Shouldn't a mode create be in here somewhere too?
		eventsFile.Open( WFile::modeReadWrite | WFile::modeBinary, WFile::shareUnknown, WFile::permReadWrite );
		eventsFile.Write( events, sess->num_events * sizeof( eventsrec ) );
		eventsFile.Close();
	}
	else
	{
		// %%TODO: Fix this, it looks like this is a bug...
		WFile::Remove( s );
	}
}
