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


void send_inst_msg(inst_msg_header *ih, const char *msg)
{
    char szFileName[MAX_PATH];

    sprintf(szFileName, "%sTMSG%3.3u.%3.3d", syscfg.datadir, app->GetInstanceNumber(), ih->dest_inst);
    WFile file( szFileName );
    if ( file.Open( WFile::modeBinary | WFile::modeReadWrite | WFile::modeCreateFile ) )
    {
        file.Seek( 0L, WFile::seekEnd );
        if ( ih->msg_size > 0 && !msg )
        {
            ih->msg_size = 0;
        }
        file.Write( ih, sizeof( inst_msg_header ) );
        if (ih->msg_size > 0)
        {
            file.Write( const_cast<char*>( msg ), ih->msg_size );
        }
        file.Close();

        for (int i = 0; i < 1000; i++)
        {
            char szMsgFileName[ MAX_PATH ];
            sprintf(szMsgFileName, "%sMSG%5.5d.%3.3d", syscfg.datadir, i, ih->dest_inst);
            if (!WFile::Rename(szFileName, szMsgFileName) || (errno != EACCES))
            {
                break;
            }
        }
    }
}


#define LAST(s) s[strlen(s)-1]

void send_inst_str1( int m, int whichinst, const char *pszSendString )
{
    inst_msg_header ih;
    char szTempSendString[ 1024 ];

    sprintf( szTempSendString, "%s\r\n", pszSendString );
    ih.main = static_cast<unsigned short>( m );
    ih.minor = 0;
    ih.from_inst = static_cast<unsigned short>( app->GetInstanceNumber() );
    ih.from_user = static_cast<unsigned short>( sess->usernum );
    ih.msg_size = strlen( szTempSendString ) + 1;
    ih.dest_inst = static_cast<unsigned short>( whichinst );
    time((long *) &ih.daten);

    send_inst_msg( &ih, szTempSendString );
}


void send_inst_str(int whichinst, const char *pszSendString)
{
    send_inst_str1(INST_MSG_STRING, whichinst, pszSendString);
}


void send_inst_sysstr(int whichinst, const char *pszSendString)
{
    send_inst_str1(INST_MSG_SYSMSG, whichinst, pszSendString);
}


void send_inst_shutdown(int whichinst)
{
    inst_msg_header ih;

    ih.main = INST_MSG_SHUTDOWN;
    ih.minor = 0;
    ih.from_inst = static_cast<unsigned short>( app->GetInstanceNumber() );
    ih.from_user = static_cast<unsigned short>( sess->usernum );
    ih.msg_size = 0;
    ih.dest_inst = static_cast<unsigned short>( whichinst );
    time((long *) &ih.daten);

    send_inst_msg(&ih, NULL);
}


void send_inst_cleannet()
{
    inst_msg_header ih;
    instancerec ir;

    if (app->GetInstanceNumber() == 1)
    {
        return;
    }

    get_inst_info(1, &ir);
    if (ir.loc == INST_LOC_WFC)
    {
        ih.main = INST_MSG_CLEANNET;
        ih.minor = 0;
        ih.from_inst = static_cast<unsigned short>( app->GetInstanceNumber() );
        ih.from_user = 1;
        ih.msg_size = 0;
        ih.dest_inst = 1;
        time((long *) &ih.daten);

        send_inst_msg(&ih, NULL);
    }
}


/*
 * "Broadcasts" a message to all online instances.
 */
void _broadcast(char *pszSendString)
{
    instancerec ir;

    int ni = num_instances();
    for (int i = 1; i <= ni; i++)
    {
        if (i == app->GetInstanceNumber())
        {
            continue;
        }
        if (get_inst_info(i, &ir))
        {
            if (inst_available(&ir))
            {
                send_inst_str(i, pszSendString);
            }
        }
    }
}

void broadcast( const char *fmt, ... )
{
    va_list ap;
    char szBuffer[2048];

    va_start(ap, fmt);
    vsnprintf(szBuffer, 2048, fmt, ap);
    va_end(ap);
    _broadcast( szBuffer );
}


/*
 * Handles one inter-instance message, based on type, returns inter-instance
 * main type of the "packet".
 */
int handle_inst_msg(inst_msg_header * ih, const char *msg)
{
    unsigned short i;
    char xl[81], cl[81], atr[81], cc;

    if ( !ih || ( ih->msg_size > 0 && msg == NULL ) )
    {
        return -1;
    }

    switch (ih->main)
    {
    case INST_MSG_STRING:
    case INST_MSG_SYSMSG:
        if ( ih->msg_size > 0 && sess->IsUserOnline() && !hangup )
        {
            app->localIO->SaveCurrentLine( cl, atr, xl, &cc );
            nl( 2 );
            if ( in_chatroom )
            {
                i = 0;
                while ( i < ih->msg_size )
                {
                    bputch( msg[ i++ ] );
                }
                nl();
                RestoreCurrentLine( cl, atr, xl, &cc );
                return( ih->main );
            }
            if ( ih->main == INST_MSG_STRING )
            {
                WUser user;
                app->userManager->ReadUser( &user, ih->from_user );
                bprintf( "|#1%.12s (%d)|#0> |#2", user.GetUserNameAndNumber( ih->from_user ), ih->from_inst );
            }
            else
            {
                sess->bout << "|#6[SYSTEM ANNOUNCEMENT] |#7> |#2";
            }
            i = 0;
            while (i < ih->msg_size)
            {
                bputch(msg[i++]);
            }
            nl( 2 );
            RestoreCurrentLine(cl, atr, xl, &cc);
        }
        break;
    case INST_MSG_CLEANNET:
		app->SetCleanNetNeeded( true );
        break;
        // Handle this one in process_inst_msgs
    case INST_MSG_SHUTDOWN:
    default:
        break;
    }
    return ih->main;
}


void process_inst_msgs()
{
    if ( x_only || !inst_msg_waiting() )
    {
        return;
    }
    last_iia = timer1();

    int oiia = setiia( 0 );
    char* m = NULL;
    char szFindFileName[MAX_PATH];
    inst_msg_header ih;
    WFindFile fnd;

    sprintf(szFindFileName, "%sMSG*.%3.3u", syscfg.datadir, app->GetInstanceNumber());
    bool bDone = fnd.open(szFindFileName, 0);
    while ((bDone) && (!hangup))
    {
        WFile file( syscfg.datadir, fnd.GetFileName() );
        if ( !file.Open( WFile::modeBinary | WFile::modeReadOnly ) )
        {
            continue;
        }
        long lFileSize = file.GetLength();
        long lFilePos = 0L;
        while (lFilePos < lFileSize)
        {
            m = NULL;
            file.Read( &ih, sizeof( inst_msg_header ) );
            lFilePos += sizeof(inst_msg_header);
            if (ih.msg_size > 0)
            {
                m = static_cast<char*>( BbsAllocA( ih.msg_size ) );
                WWIV_ASSERT( m );
                if (m == NULL)
                {
                    break;
                }
                file.Read( m, ih.msg_size );
                lFilePos += ih.msg_size;
            }
            int hi = handle_inst_msg(&ih, m);
            if (m)
            {
                BbsFreeMemory(m);
                m = NULL;
            }
            if ( hi == INST_MSG_SHUTDOWN )
            {
                if ( sess->IsUserOnline() )
                {
                    tmp_disable_pause( true );
                    nl( 2 );
                    printfile( OFFLINE_NOEXT );
                    if ( sess->IsUserOnline() )
                    {
                        if (sess->usernum == 1)
                        {
                            fwaiting = sess->thisuser.GetNumMailWaiting();
                        }
                        sess->WriteCurrentUser( sess->usernum );
                        write_qscn(sess->usernum, qsc, false);
                    }
                    tmp_disable_pause( false );
                }
                file.Close();
                file.Delete();
                sess->topline = 0;
                app->localIO->LocalCls();
                hangup = true;
                hang_it_up();
                holdphone( false );
                wait1(18);
                app->QuitBBS();
            }
        }
        file.Close();
        file.Delete();
        bDone = fnd.next();
    }
    setiia(oiia);
}


// Gets instancerec for specified instance, returns in ir.
bool get_inst_info(int nInstanceNum, instancerec * ir)
{
    if (!ir)
    {
        return false;
    }

    memset(ir, 0, sizeof(instancerec));

    WFile instFile( syscfg.datadir, INSTANCE_DAT );
    if ( !instFile.Open( WFile::modeBinary | WFile::modeReadOnly ) )
    {
        return false;
    }
    int i = static_cast<int>( instFile.GetLength() / sizeof( instancerec ) );
    if (i < (nInstanceNum + 1))
    {
        instFile.Close();
        return false;
    }
    instFile.Seek( static_cast<long> (nInstanceNum * sizeof(instancerec) ), WFile::seekBegin );
    instFile.Read( ir, sizeof( instancerec ) );
    instFile.Close();
    return true;
}


/*
 * Returns 1 if inst_no has user online that can receive msgs, else 0
 */
bool inst_available(instancerec * ir)
{
    if (!ir)
    {
        return false;
    }

    return ((ir->flags & INST_FLAGS_ONLINE) && (ir->flags & INST_FLAGS_MSG_AVAIL)) ? true : false;
}


/*
 * Returns 1 if inst_no has user online in chat, else 0
 */
bool inst_available_chat(instancerec * ir)
{
    if (!ir)
    {
        return false;
    }

    return ( (ir->flags & INST_FLAGS_ONLINE) &&
             (ir->flags & INST_FLAGS_MSG_AVAIL) &&
             (ir->loc == INST_LOC_CHATROOM) ) ? true : false;
}


/*
 * Returns max instance number.
 */
int num_instances()
{
    WFile instFile( syscfg.datadir, INSTANCE_DAT );
    if ( !instFile.Open( WFile::modeReadOnly | WFile::modeBinary, WFile::shareUnknown, WFile::permReadWrite ) )
    {
        return 0;
    }
    int nNumInstances = static_cast<int>( instFile.GetLength() / sizeof( instancerec ) ) - 1;
    instFile.Close();
    return nNumInstances;
}


/*
 * Returns 1 if sess->usernum nUserNumber is online, and returns instance user is on in
 * wi, else returns 0.
 */
bool user_online(int nUserNumber, int *wi)
{
    int ni = num_instances();
    for (int i = 1; i <= ni; i++)
    {
        if (i == app->GetInstanceNumber())
        {
            continue;
        }
        instancerec ir;
        get_inst_info(i, &ir);
        if ( ir.user == nUserNumber && ( ir.flags & INST_FLAGS_ONLINE ) )
        {
            if (wi)
            {
                *wi = i;
            }
            return true;
        }
    }
    if (wi)
    {
        *wi = -1;
    }
    return false;
}


/*
 * Allows sending some types of messages to other instances, or simply
 * viewing the status of the other instances.
 */
void instance_edit()
{
    instancerec ir;

    if ( !ValidateSysopPassword() )
    {
        return;
    }

    int ni = num_instances();

    bool done = false;
    while ( !done && !hangup )
    {
        CheckForHangup();
        nl();
        sess->bout << "|#21|#7)|#1 Multi-Instance Status\r\n";
        sess->bout << "|#22|#7)|#1 Shut Down One Instance\r\n";
        sess->bout << "|#23|#7)|#1 Shut Down ALL Instances\r\n";
        sess->bout << "|#2Q|#7)|#1 Quit\r\n";
        nl();
        sess->bout << "|#1Select: ";
        char ch = onek("Q123");
        switch (ch)
        {
        case '1':
            nl();
            sess->bout << "|#1Instance Status:\r\n";
            multi_instance();
            break;
        case '2':
            {
                nl();
                sess->bout << "|#2Which Instance: ";
                char szInst[ 10 ];
                input( szInst, 3, true );
                if (!szInst[0])
                {
                    break;
                }
                int i = atoi(szInst);
                if ( !i || i > ni )
                {
                    nl();
                    sess->bout << "|#6Instance unavailable.\r\n";
                    break;
                }
                if (i == app->GetInstanceNumber())
                {
                    sess->topline = 0;
                    app->localIO->LocalCls();
                    hangup = true;
                    hang_it_up();
                    holdphone( false );
                    wait1(18);
                    app->QuitBBS();
                    break;
                }
                if (get_inst_info(i, &ir))
                {
                    if (ir.loc != INST_LOC_DOWN)
                    {
                        nl();
						sess->bout << "|#2Shutting down instance " << i << wwiv::endl;
                        send_inst_shutdown(i);
                    }
                    else
                    {
                        sess->bout << "\r\n|#6Instance already shut down.\r\n";
                    }
                }
                else
                {
                    sess->bout << "\r\n|#6Instance unavailable.\r\n";
                }
            }
            break;
        case '3':
            nl();
            sess->bout << "|#5Are you sure? ";
            if (yesno())
            {
                sess->bout << "\r\n|#2Shutting down all instances.\r\n";
                for (int i1 = 1; i1 <= ni; i1++)
                {
                    if (i1 != app->GetInstanceNumber())
                    {
                        if ( get_inst_info(i1, &ir) && ir.loc != INST_LOC_DOWN )
                        {
                            send_inst_shutdown(i1);
                        }
                    }
                }
                sess->topline = 0;
                app->localIO->LocalCls();
                hangup = true;
                hang_it_up();
                holdphone( false );
                wait1(18);
                app->QuitBBS();
            }
            break;
        case 'Q':
        default:
            done = true;
            break;
        }
    }
}



/*
* Writes BBS location data to instance.dat, so other instances can see
* some info about this instance.
*/
void write_inst( int loc, int subloc, int flags )
{
    static instancerec ti;
    instancerec ir;

    int re_write = 0;
    if (ti.user == 0)
    {
        if (get_inst_info(app->GetInstanceNumber(), &ir))
        {
            ti.user = ir.user;
        }
        else
        {
            ti.user = 1;
        }
        time((long *) &ti.inst_started);
        re_write = 1;
    }

    unsigned short cf = ti.flags & (~(INST_FLAGS_ONLINE | INST_FLAGS_MSG_AVAIL));
    if ( sess->IsUserOnline() )
    {
        cf |= INST_FLAGS_ONLINE;
        if (invis)
        {
            cf |= INST_FLAGS_INVIS;
        }
        if ( !sess->thisuser.isIgnoreNodeMessages() )
        {
            switch (loc)
            {
            case INST_LOC_MAIN:
            case INST_LOC_XFER:
            case INST_LOC_SUBS:
            case INST_LOC_EMAIL:
            case INST_LOC_CHATROOM:
            case INST_LOC_RMAIL:
                if (avail)
                {
                    cf |= INST_FLAGS_MSG_AVAIL;
                }
                break;
            default:
                if ((loc >= INST_LOC_CH1) && (loc <= INST_LOC_CH10) && avail)
                {
                    cf |= INST_FLAGS_MSG_AVAIL;
                }
                break;
            }
        }
        unsigned short ms = (sess->using_modem) ? modem_speed : 0;
        if (ti.modem_speed != ms)
        {
            ti.modem_speed = ms;
            re_write = 1;
        }
    }
    if (flags != INST_FLAGS_NONE)
    {
        if (flags & 0x8000)
        {
            // reset an option
            ti.flags &= flags;
        }
        else
        {
            // set an option
            ti.flags |= flags;
        }
    }
    if ((ti.flags & INST_FLAGS_INVIS) && (!invis))
    {
        cf = 0;
        if (ti.flags & INST_FLAGS_ONLINE)
        {
            cf |= INST_FLAGS_ONLINE;
        }
        if (ti.flags & INST_FLAGS_MSG_AVAIL)
        {
            cf |= INST_FLAGS_MSG_AVAIL;
        }
        re_write = 1;
    }
    if (cf != ti.flags)
    {
        re_write = 1;
        ti.flags = cf;
    }
    if (ti.number != app->GetInstanceNumber())
    {
        re_write = 1;
        ti.number = static_cast<short>( app->GetInstanceNumber() );
    }
    if (loc == INST_LOC_DOWN)
    {
        re_write = 1;
    }
    else
    {
        if ( sess->IsUserOnline() )
        {
            if (ti.user != sess->usernum)
            {
                re_write = 1;
                if ((sess->usernum > 0) && (sess->usernum <= syscfg.maxusers))
                {
                    ti.user = static_cast<short>( sess->usernum );
                }
            }
        }
    }

    if ( ti.subloc != static_cast<unsigned short>( subloc ) )
    {
        re_write = 1;
        ti.subloc = static_cast<unsigned short>( subloc );
    }
    if ( ti.loc != static_cast<unsigned short>( loc ) )
    {
        re_write = 1;
        ti.loc = static_cast<unsigned short>( loc );
    }
    if ((((ti.flags & INST_FLAGS_INVIS) && (!invis)) ||
        ((!(ti.flags & INST_FLAGS_INVIS)) && (invis))) ||
        (((ti.flags & INST_FLAGS_MSG_AVAIL) && (!avail)) ||
        ((!(ti.flags & INST_FLAGS_MSG_AVAIL)) && (avail))) &&
        (ti.loc!=INST_LOC_WFC))
    {
        re_write=1;
    }
    if (re_write)
    {
        time((long *) &ti.last_update);
        WFile instFile( syscfg.datadir, INSTANCE_DAT );
        if ( instFile.Open( WFile::modeReadWrite | WFile::modeBinary | WFile::modeCreateFile, WFile::shareUnknown, WFile::permReadWrite ) )
        {
            instFile.Seek( static_cast<long>(app->GetInstanceNumber() * sizeof(instancerec)), WFile::seekBegin );
            instFile.Write( &ti, sizeof( instancerec ) );
            instFile.Close();
        }
    }
}



/*
* Returns 1 if a message waiting for this instance, 0 otherwise.
*/
bool inst_msg_waiting()
{
    if ( !iia || !echo )
    {
        return false;
    }

    long l = timer1();
    if (labs(l - last_iia) < iia)
    {
        return false;
    }

    char szFileName[81];
    sprintf( szFileName, "%sMSG*.%3.3u", syscfg.datadir, app->GetInstanceNumber() );
    bool bExist = WFile::ExistsWildcard( szFileName );
    if ( !bExist )
    {
        last_iia = l;
    }

    return bExist;
}




// Sets inter-instance availability on/off, for inter-instance messaging.
// retruns the old iia value.
int setiia(int poll_ticks)
{
    int oiia = iia;
    iia = poll_ticks;
    return oiia;
}

