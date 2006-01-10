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
#include "WStringUtils.h"

static int netw;
int last_time_c;


void rename_pend( const char *pszDirectory, const char *pszFileName )
{
	char s[ MAX_PATH ], s1[ MAX_PATH ], s2[ MAX_PATH ];

	sprintf( s, "%s%s", pszDirectory, pszFileName );

	if ( atoi( pszFileName + 1 ) )
    {
		strcpy(s2, "p1-");
    }
	else
    {
		strcpy( s2, "p0-" );
    }

	for ( int i = 0; i < 1000; i++ )
    {
		sprintf( s1, "%s%s%u.net", pszDirectory, s2, i );
		if ( !WFile::Rename( s, s1 ) || errno != EACCES )
        {
			break;
        }
	}
}


int checkup2( const time_t tFileTime, const char *pszFileName )
{
    WFile file( sess->GetNetworkDataDirectory(), pszFileName );

	if ( file.Open( WFile::modeReadOnly ) )
	{
        time_t tNewFileTime = file.GetFileTime();
        file.Close();
        return ( tNewFileTime > ( tFileTime + 2 ) ) ? 1 : 0;
	}
	return 1;
}


int check_bbsdata()
{
	char s[MAX_PATH];
	int ok = 0, ok2 = 0;

	sprintf(s, "%s%s", sess->GetNetworkDataDirectory(), CONNECT_UPD);
	if ((ok = WFile::Exists(s)) == 0)
    {
		sprintf(s, "%s%s", sess->GetNetworkDataDirectory(), BBSLIST_UPD);
        ok = WFile::Exists(s) ? 1 : 0;
	}
	if (ok && status.net_edit_stuff)
    {
		holdphone( true );
		sprintf( s, "NETEDIT .%ld /U", sess->GetNetworkNumber() );
		ExecuteExternalProgram(s, EFLAG_NETPROG);
	}
    else
    {
        WFile bbsdataNet( sess->GetNetworkDataDirectory(), BBSDATA_NET );
		if ( bbsdataNet.Open( WFile::modeReadOnly ) )
		{
            time_t tFileTime = bbsdataNet.GetFileTime();
            bbsdataNet.Close();
			ok = checkup2(tFileTime, BBSDATA_NET) || checkup2(tFileTime, CONNECT_NET);
			ok2 = checkup2(tFileTime, CALLOUT_NET);
		}
        else
        {
			ok = ok2 = 1;
        }
	}
	sprintf(s, "%s%s", sess->GetNetworkDataDirectory(), BBSDATA_NET);
	if (!WFile::Exists(s))
    {
		ok = ok2 = 0;
    }
	sprintf(s, "%s%s", sess->GetNetworkDataDirectory(), CONNECT_NET);
	if (!WFile::Exists(s))
    {
		ok = ok2 = 0;
    }
	sprintf(s, "%s%s", sess->GetNetworkDataDirectory(), CALLOUT_NET);
	if (!WFile::Exists(s))
    {
		ok = ok2 = 0;
    }
	if (ok || ok2)
    {
		sprintf( s, "network3 %s .%ld", (ok ? " Y" : ""), sess->GetNetworkNumber() );
		holdphone( true );

		ExecuteExternalProgram(s, EFLAG_NETPROG);
		app->statusMgr->Lock();
		++status.filechange[filechange_net];
		app->statusMgr->Write();

		zap_call_out_list();
		zap_contacts();
		zap_bbs_list();
		if ( ok >= 0 )
        {
			return 1;
        }
	}
	return 0;
}


void cleanup_net()
{
	char cl1[81], cl2[81], *ss1;
	int i1, i2;

	if ( cleanup_net1() && app->HasConfigFlag( OP_FLAGS_NET_CALLOUT ) )
    {
		if ( sess->wfc_status == 0 )                    // WFC addition
        {
			app->localIO->LocalCls();
        }

		i1 = i2 = 0;
		if ( ini_init( WWIV_INI, INI_TAG, NULL ) )
        {
			if ( ( ss1 = ini_get( "NET_CLEANUP_CMD1", -1, NULL ) ) != NULL )
            {
				if ( ss1 )
                {
					strcpy( cl1, ss1 );
					i1 = 1;
				}
			}
			if ( ( ss1 = ini_get( "NET_CLEANUP_CMD2", -1, NULL ) ) != NULL )
            {
				if ( ss1 )
                {
					strcpy( cl2, ss1 );
					i2 = 1;
				}
			}
			ini_done();
		}
		if ( i1 )
        {
			ExecuteExternalProgram( cl1, app->GetSpawnOptions( SPWANOPT_NET_CMD1 ) );
			cleanup_net1();
		}
		if ( i2 )
        {
			ExecuteExternalProgram( cl2, app->GetSpawnOptions( SPWANOPT_NET_CMD2 ) );
			cleanup_net1();
		}
	}

	holdphone( false );
}


int cleanup_net1()
{
    char s[81], cl[81];
    int ok, ok2, nl = 0, anynew = 0, i = 0;
    bool abort;

	app->SetCleanNetNeeded( false );

    if ( net_networks[0].sysnum == 0 && sess->GetMaxNetworkNumber() == 1 )
    {
        return 0;
    }

    int any = 1;

    while ( any && ( nl++ < 10 ) )
    {
        any = 0;

        for ( int nNetNumber = 0; nNetNumber < sess->GetMaxNetworkNumber(); nNetNumber++ )
        {
            set_net_num( nNetNumber );

            if ( !net_sysnum )
            {
                continue;
            }

            app->localIO->set_protect( 0 );

            ok2 = 1;
            abort = false;
            while ( ok2 && !abort )
            {
                ok2 = 0;
                ok = 0;
                WFindFile fnd;
                sprintf( s, "%s""p*.%3.3d", sess->GetNetworkDataDirectory(), app->GetInstanceNumber() );
                bool bFound = fnd.open( s, 0 );
                while ( bFound )
                {
                    ok = 1;
                    ++i;
                    rename_pend( sess->GetNetworkDataDirectory(), fnd.GetFileName() );
                    anynew = 1;
                    bFound = fnd.next();
                }

                if ( app->GetInstanceNumber() == 1 )
                {
                    if ( !ok )
                    {
                        sprintf( s, "%s""p*.net", sess->GetNetworkDataDirectory() );
                        WFindFile fnd;
                        ok = fnd.open( s, 0 );
                    }
                    if ( ok )
                    {
#ifndef _UNIX
                        if ( sess->wfc_status == 0 )
                        {
                            // WFC addition
                            app->localIO->LocalCls();
                        }
                        else
                        {
                            wfc_cls();
                        }
#endif
                        holdphone( true );
                        ++i;
                        hangup = false;
                        sess->using_modem = 0;
                        sprintf( cl, "network1 .%ld", sess->GetNetworkNumber() );
                        if ( ExecuteExternalProgram( cl, EFLAG_NETPROG ) < 0 )
                        {
                            abort = true;
                        }
                        else
                        {
                            any = 1;
                        }
                        ok2 = 1;
                    }
                    sprintf( s, "%s%s", sess->GetNetworkDataDirectory(), LOCAL_NET );
                    if ( WFile::Exists( s ) )
                    {
#ifndef _UNIX
                        if ( sess->wfc_status == 0 )
                        {
                            // WFC addition
                            app->localIO->LocalCls();
                        }
                        else
                        {
                            wfc_cls();
                        }
#endif
                        holdphone( true );
                        ++i;
                        any = 1;
                        ok = 1;
                        hangup = false;
                        sess->using_modem = 0;
                        sprintf( cl, "network2 .%ld", sess->GetNetworkNumber() );
                        if ( ExecuteExternalProgram( cl, EFLAG_NETPROG ) < 0 )
                        {
                            abort = true;
                        }
                        else
                        {
                            any = 1;
                        }
                        ok2 = 1;
                        app->statusMgr->Read();
                        sess->SetCurrentReadMessageArea( -1 );
                        sess->ReadCurrentUser( 1 );
                        fwaiting = sess->thisuser.GetNumMailWaiting();
                    }
                    if ( check_bbsdata() )
                    {
                        ok2 = 1;
                    }
                    if ( ok2 )
                    {
                        app->localIO->LocalCls();
                        zap_contacts();
                        ++i;
                    }
                }
            }
        }
    }
    if ( anynew && ( app->GetInstanceNumber() != 1 ) )
    {
        send_inst_cleannet();
    }
    return i;
}


void do_callout( int sn )
{
	char s[81], s1[81], town[5];

	long l;
	time( &l );
	int i = -1;
	int i2 = -1;
	if ( !net_networks[sess->GetNetworkNumber()].con )
    {
		read_call_out_list();
    }
    int i1;
	for ( i1 = 0; i1 < net_networks[sess->GetNetworkNumber()].num_con; i1++ )
    {
		if ( net_networks[sess->GetNetworkNumber()].con[i1].sysnum == sn )
        {
			i = i1;
        }
    }
	if ( !net_networks[sess->GetNetworkNumber()].ncn )
    {
		read_contacts();
    }
	for ( i1 = 0; i1 < net_networks[sess->GetNetworkNumber()].num_ncn; i1++ )
    {
		if ( net_networks[sess->GetNetworkNumber()].ncn[i1].systemnumber == sn )
        {
			i2 = i1;
        }
    }

	if ( i != -1 )
    {
	    net_system_list_rec *csne = next_system( net_networks[sess->GetNetworkNumber()].con[i].sysnum );
		if ( csne )
        {
			sprintf( s, "network /N%u /A%s /P%s /S%u /T%ld",
				     sn, ( net_networks[sess->GetNetworkNumber()].con[i].options & options_sendback ) ? "1" : "0",
				     csne->phone, ( NULL != modem_i ) ? modem_i->defl.com_speed : 0, l );
            // if modem_i is null, we are on a telnet node, and PPP project, so we don't care...
			if ( net_networks[sess->GetNetworkNumber()].con[i].macnum )
            {
				sprintf( s1, " /M%d", static_cast<int>( net_networks[sess->GetNetworkNumber()].con[i].macnum ) );
				strcat( s, s1 );
			}
			sprintf( s1, " .%ld", sess->GetNetworkNumber() );
			strcat( s, s1 );
			if ( strncmp( csne->phone, "000", 3 ) )
            {
#ifndef _UNUX
				run_exp();
#endif
				sess->bout << "|#7Calling out to: |#2" << csne->name << " - ";
				if ( sess->GetMaxNetworkNumber() > 1 )
                {
					sess->bout << sess->GetNetworkName() << " ";
                }
				sess->bout << "@" << sn << wwiv::endl;
                char szRegionsFileName[ MAX_PATH ];
				sprintf( szRegionsFileName, "%s%s%c%s.%-3u", syscfg.datadir,
						 REGIONS_DAT, WWIV_FILE_SEPERATOR_CHAR, REGIONS_DAT, atoi( csne->phone ) );
				if ( WFile::Exists( szRegionsFileName ) )
                {
					sprintf( town, "%c%c%c", csne->phone[4], csne->phone[5], csne->phone[6] );
					describe_town( atoi( csne->phone ), atoi( town ), s1 );
				}
                else
                {
					describe_area_code( atoi( csne->phone ), s1 );
                }
				sess->bout << "|#7Sys located in: |#2" << s1 << wwiv::endl;
				if ( i2 != -1 && net_networks[sess->GetNetworkNumber()].ncn[i2].bytes_waiting )
                {
					sess->bout << "|#7Amount pending: |#2" <<
						bytes_to_k( net_networks[sess->GetNetworkNumber()].ncn[i2].bytes_waiting ) <<
						"k" << wwiv::endl;
				}
				else
				{
					nl();
				}
				sess->bout << "|#7Commandline is: |#2" << s << wwiv::endl;
				ansic( 7 );
				sess->bout << charstr( 80, 205 );
#ifndef _UNIX
				holdphone( true );
				Wait( 2.5 );
#endif
				ExecuteExternalProgram( s, EFLAG_NETPROG );
				zap_contacts();
				app->statusMgr->Read();
				last_time_c = l;
				global_xx = false;
				cleanup_net();
#ifndef _UNUX
				run_exp();
#endif
				send_inst_cleannet();
#ifndef _UNUX
				holdphone( false );
				imodem( false );
#endif
			}
		}
	}
}


bool ok_to_call(int i)
{
	net_call_out_rec *con = &( net_networks[sess->GetNetworkNumber()].con[i] );

    bool ok = ( ( con->options & options_no_call ) == 0 ) ? true : false;
	if ( con->options & options_receive_only )
    {
		ok = false;
    }
	int nDow = dow();

	time_t t;
	time( &t );
	struct tm * pTm = localtime( &t );

	if ( con->options & options_ATT_night )
    {
		if ( nDow != 0 && nDow != 6 )
        {
			if ( pTm->tm_hour < 23 && pTm->tm_hour >= 7 )
            {
				ok = false;
            }
		}
		if ( nDow == 0 )
        {
			if ( pTm->tm_hour < 23 && pTm->tm_hour >= 16 )
            {
				ok = false;
            }
		}
	}
	char l = con->min_hr;
	char h = con->max_hr;
	if ( l > -1 && h > -1 && h != l )
    {
		if ( h == 0 || h == 24 )
        {
			if ( pTm->tm_hour < l )
            {
				ok = false;
            }
			else if ( pTm->tm_hour == l && pTm->tm_min < 12 )
            {
				ok = false;
            }
			else if ( pTm->tm_hour == 23 && pTm->tm_min > 30 )
            {
				ok = false;
            }
		}
        else if ( l == 0 || l == 24 )
        {
			if ( pTm->tm_hour >= h )
            {
				ok = false;
            }
			else if ( pTm->tm_hour == ( h - 1 ) && pTm->tm_min > 30 )
            {
				ok = false;
            }
			else if ( pTm->tm_hour == 0 && pTm->tm_min < 12 )
            {
				ok = false;
            }
		}
        else if ( h > l )
        {
			if ( pTm->tm_hour < l || pTm->tm_hour >= h )
            {
				ok = false;
            }
			else if ( pTm->tm_hour == l && pTm->tm_min < 12 )
            {
				ok = false;
            }
			else if ( pTm->tm_hour == ( h - 1 ) && pTm->tm_min > 30 )
            {
				ok = false;
            }
		}
        else
        {
			if ( pTm->tm_hour >= h && pTm->tm_hour < l )
            {
				ok = false;
            }
			else if ( pTm->tm_hour == l && pTm->tm_min < 12 )
            {
				ok = false;
            }
			else if ( pTm->tm_hour == ( h - 1 ) && pTm->tm_min > 30 )
            {
				ok = false;
            }
		}
	}
	return ok;
}



#define WEIGHT 30.0

void fixup_long( long *f, long l )
{
	if ( *f > l )
    {
		*f = l;
    }

	if ( *f + ( SECONDS_PER_DAY * 30L ) < l )
    {
		*f = l - SECONDS_PER_DAY * 30L;
    }
}


void free_vars( float **weight, int **try1 )
{
	if ( weight || try1 )
	{
		for ( int nNetNumber = 0; nNetNumber < sess->GetMaxNetworkNumber(); nNetNumber++ )
		{
			if ( try1 && try1[nNetNumber] )
			{
				BbsFreeMemory( try1[nNetNumber] );
			}
			if ( weight && weight[nNetNumber] )
			{
				BbsFreeMemory( weight[nNetNumber] );
			}
		}
		if ( try1 )
		{
			BbsFreeMemory( try1 );
		}
		if ( weight )
		{
			BbsFreeMemory( weight );
		}
	}
}

void attempt_callout()
{
    int **try1, i, i1, i2, num_call_sys, num_ncn;
    float **weight, fl, fl1;
    long l, l1;
    net_call_out_rec *con;
    net_contact_rec *ncn;

    app->statusMgr->Read();

    bool net_only = true;

    if ( syscfg.netlowtime != syscfg.nethightime )
    {
        if ( syscfg.nethightime > syscfg.netlowtime )
        {
            if ( ( timer() <= ( syscfg.netlowtime * SECONDS_PER_MINUTE_FLOAT ) ) || ( timer() >= ( syscfg.nethightime * 60.0 ) ) )
            {
                net_only = false;
            }
        }
        else
        {
            if ( ( timer() <= ( syscfg.netlowtime * SECONDS_PER_MINUTE_FLOAT ) ) && ( timer() >= ( syscfg.nethightime * 60.0 ) ) )
            {
                net_only = false;
            }
        }
    }
    else
    {
        net_only = false;
    }
    time( &l );
    if ( last_time_c > l )
    {
        last_time_c = 0L;
    }
    if ( labs(last_time_c - l ) < 120 )
    {
        return;
    }
    if ( last_time_c == 0L )
    {
        last_time_c = l;
        return;
    }
    if ( ( try1 = static_cast<int **>( BbsAllocA( sizeof(int *) * sess->GetMaxNetworkNumber() ) ) ) == NULL )
    {
        return;
    }
    if ( ( weight = static_cast<float **>( BbsAllocA( sizeof( float * ) * sess->GetMaxNetworkNumber() ) ) ) == NULL )
    {
        BbsFreeMemory(try1);
        return;
    }
    memset( try1, 0, sizeof(int *) * sess->GetMaxNetworkNumber() );
    memset( weight, 0, sizeof(float *) * sess->GetMaxNetworkNumber() );

    float fl2 = 0.0;
    int any = 0;

    for ( int nNetNumber = 0; nNetNumber < sess->GetMaxNetworkNumber(); nNetNumber++ )
    {
        set_net_num( nNetNumber );
        if ( !net_sysnum )
        {
            continue;
        }

        // if (!net_networks[sess->GetNetworkNumber()].con)
        read_call_out_list();
        // if (!net_networks[sess->GetNetworkNumber()].ncn)
        read_contacts();

        con = net_networks[sess->GetNetworkNumber()].con;
        ncn = net_networks[sess->GetNetworkNumber()].ncn;
        num_call_sys = net_networks[sess->GetNetworkNumber()].num_con;
        num_ncn = net_networks[sess->GetNetworkNumber()].num_ncn;

        try1[nNetNumber] = static_cast<int *>( BbsAllocA( sizeof( int ) * num_call_sys ) );
        if ( !try1[nNetNumber] )
        {
            break;
        }
        weight[nNetNumber] = static_cast<float *>( BbsAllocA( sizeof( float ) * num_call_sys ) );
        if ( !weight[nNetNumber] )
        {
            break;
        }

        for ( i = 0; i < num_call_sys; i++ )
        {
            try1[nNetNumber][i] = 0;
            bool ok = ok_to_call( i );
            i2 = -1;
            for ( i1 = 0; i1 < num_ncn; i1++ )
            {
                if ( ncn[i1].systemnumber == con[i].sysnum )
                {
                    i2 = i1;
                }
            }
            if ( ok && ( i2 != -1 ) )
            {
                if ( ncn[i2].bytes_waiting == 0L )
                {
                    if ( con[i].call_anyway )
                    {
                        if ( con[i].call_anyway > 0 )
                        {
                            l1 = ncn[i2].lastcontact + SECONDS_PER_HOUR * con[i].call_anyway;
                        }
                        else
                        {
                            l1 = ncn[i2].lastcontact + SECONDS_PER_HOUR / (-con[i].call_anyway);
                        }
                        if ( l < l1 )
                        {
                            ok = false;
                        }
                    }
                    else
                    {
                        ok = false;
                    }
                }
                if ( con[i].options & options_once_per_day )
                {
                    if ( labs( l - ncn[i2].lastcontactsent ) <
                        ( 20L * SECONDS_PER_HOUR / con[i].times_per_day ) )
                    {
                        ok = false;
                    }
                }
                if ( ok )
                {
                    if ( ( bytes_to_k(ncn[i2].bytes_waiting ) < con[i].min_k )
                        && ( labs( l - ncn[i2].lastcontact ) < SECONDS_PER_DAY ) )
                    {
                        ok = false;
                    }
                }
                fixup_long( ( long * ) &( ncn[i2].lastcontactsent ), l );
                fixup_long( ( long * ) &( ncn[i2].lasttry ), l );
                if ( ok )
                {
                    if ( ncn[i2].bytes_waiting == 0L )
                    {
                        fl = 5.0 * WEIGHT;
                    }
                    else
                    {
                        fl = (float) 1024.0 / ((float) ncn[i2].bytes_waiting) * (float) WEIGHT * (float) 60.0;
                    }
                    fl1 = (float) (l - ncn[i2].lasttry);
                    if ( ( fl < fl1 ) || net_only )
                    {
                        try1[nNetNumber][i] = 1;
                        fl1 = fl1 / fl;
                        if (fl1 < 1.0)
                        {
                            fl1 = 1.0;
                        }
                        if (fl1 > 5.0)
                        {
                            fl1 = 5.0;
                        }
                        weight[nNetNumber][i] = fl1;
                        fl2 += fl1;
                        any++;
                    }
                }
            }
        }
    }

    if (any)
    {
        fl = static_cast<float>( fl2 ) * static_cast<float>( rand() ) / static_cast<float>( 32767.0 );
        fl1 = 0.0;
        for (int nNetNumber = 0; nNetNumber < sess->GetMaxNetworkNumber(); nNetNumber++)
        {
            set_net_num(nNetNumber);
            if (!net_sysnum)
            {
                continue;
            }

            i1 = -1;
            for (i = 0; (i < net_networks[sess->GetNetworkNumber()].num_con); i++)
            {
                if (try1[nNetNumber][i])
                {
                    fl1 += weight[nNetNumber][i];
                    if (fl1 >= fl)
                    {
                        i1 = i;
                        break;
                    }
                }
            }
            if (i1 != -1)
            {
                free_vars(weight, try1);
                weight = NULL;
                try1 = NULL;
                do_callout(net_networks[sess->GetNetworkNumber()].con[i1].sysnum);
                time(&l);
                last_time_c = l;
                break;
            }
        }
    }
    free_vars(weight, try1);

    set_net_num( 0 );
}


void print_pending_list()
{
	int i = 0,
        i1 = 0,
        i2 = 0,
        i3 = 0,
        num_ncn = 0,
        num_call_sys = 0,
        h = 0,
        m = 0,
        se = 0;
	int adjust = 0, lines = 0;
	char s[255], s1[81], s2[81], s3[81], s4[81], s5[81];
	long l, l1;
	net_call_out_rec *con;
	net_contact_rec *ncn;
	time_t t;
	time(&t);
	struct tm * pTm = localtime(&t);

	long ss = sess->thisuser.GetStatus();

	int nDow = dow();

	if ( net_networks[0].sysnum == 0 && sess->GetMaxNetworkNumber() == 1 )
    {
		return;
    }

	time(&l);

	nl( 2 );
	sess->bout << "                           |#3�> |#9Network Status |#3<�\r\n";
	nl();

	sess->bout << "|#7���������������������������������������������������������������������������ͻ\r\n";
	sess->bout << "|#7� |#1Ok? |#7� |#1Network  |#7� |#1 Node |#7�  |#1 Sent  |#7�|#1Received |#7�|#1Ready |#7�|#1Fails|#7�  |#1Elapsed  |#7�|#1/HrWt|#7�\r\n";
	sess->bout << "|#7���������������������������������������������������������������������������͹\r\n";

	int nNetNumber;
	for (nNetNumber = 0; nNetNumber < sess->GetMaxNetworkNumber(); nNetNumber++)
    {
		set_net_num(nNetNumber);

		if (!net_sysnum)
        {
			continue;
        }

		if (!net_networks[sess->GetNetworkNumber()].con)
        {
			read_call_out_list();
        }

		if (!net_networks[sess->GetNetworkNumber()].ncn)
        {
			read_contacts();
        }

		con = net_networks[sess->GetNetworkNumber()].con;
		ncn = net_networks[sess->GetNetworkNumber()].ncn;
		num_call_sys = net_networks[sess->GetNetworkNumber()].num_con;
		num_ncn = net_networks[sess->GetNetworkNumber()].num_ncn;

		for (i1 = 0; i1 < num_call_sys; i1++)
        {
			i2 = -1;

			for (i = 0; i < num_ncn; i++)
            {
				if ((con[i1].options & options_hide_pend))
                {
					continue;
                }
				if (con[i1].sysnum == ncn[i].systemnumber)
                {
					i2 = i;
					break;
				}
			}
			if (i2 != -1)
            {
				if (ok_to_call(i1))
                {
					strcpy(s2, "|#5Yes");
                }
				else
                {
					strcpy(s2, "|#3---");
                }

				if (ncn[i2].lastcontactsent)
				{
					l1 = l - ncn[i2].lastcontactsent;
					se = l1 % 60;
					l1 = (l1 - se) / 60;
					m = l1 % 60;
					h = l1 / 60;
					sprintf(s1, "|#2%02d:%02d:%02d", h, m, se);
				}
				else
				{
					strcpy(s1, "   |#6NEVER!    ");
				}

				sprintf( s3, "%ld""k", ( ( ncn[i2].bytes_sent ) + 1023 ) / 1024 );
                sprintf( s4, "%ld""k", ( ( ncn[i2].bytes_received ) + 1023 ) / 1024 );
                sprintf( s5, "%ld""k", ( ( ncn[i2].bytes_waiting ) + 1023 ) / 1024 );

				if (con[i1].options & options_ATT_night)
				{
					if ((nDow != 0) && (nDow != 6))
					{
						if (!((pTm->tm_hour < 23) && (pTm->tm_hour >= 7)))
						{
							adjust = (7 - pTm->tm_hour);
							if (pTm->tm_hour == 23)
							{
								adjust = 8;
							}
						}
					}
				}
				if (m >= 30)
                {
					h++;
                }
				i3 = ((con[i1].call_x_days * 24) - h - adjust);
				if (m < 31)
                {
					h--;
                }
				if (i3 < 0)
                {
					i3 = 0;
                }

				bprintf( "|#7� %-3s |#7� |#2%-8.8s |#7� |#2%5u |#7�|#2%8s |#7�|#2%8s |#7�|#2%5s |#7�|#2%4d |#7�|#2%13.13s |#7�|#2%4d |#7�\r\n",
					     s2,
					     sess->GetNetworkName(),
					     ncn[i2].systemnumber,
					     s3,
					     s4,
					     s5,
					     ncn[i2].numfails,
					     s1,
					     i3 );
				if ( !sess->thisuser.hasPause() && ( ( lines++ ) == 20 ) )
				{
					pausescr();
					lines = 0;
				}
			}
		}
	}

	for (nNetNumber = 0; nNetNumber < sess->GetMaxNetworkNumber(); nNetNumber++)
	{
		set_net_num(nNetNumber);

		if (!net_sysnum)
		{
			continue;
		}

		sprintf(s, "%s%s", sess->GetNetworkDataDirectory(), DEAD_NET);
		int hFileDeadNet = open(s, O_RDONLY | O_BINARY);
		if ( hFileDeadNet > 0 )
        {
			l = filelength( hFileDeadNet );
			close( hFileDeadNet );
            sprintf( s3, "%ld""k", ( l + 1023 ) / 1024 );
			bprintf( "|#7� |#3--- |#7� |#2%-8s |#7� |#6DEAD! |#7� |#2------- |#7� |#2------- |#7�|#2%5s |#7�|#2 --- |#7� |#2--------- |#7�|#2 --- |#7�\r\n", sess->GetNetworkName(), s3);
		}
	}

	for (nNetNumber = 0; nNetNumber < sess->GetMaxNetworkNumber(); nNetNumber++)
	{
		set_net_num(nNetNumber);

		if (!net_sysnum)
        {
			continue;
        }

		sprintf(s, "%s%s", sess->GetNetworkDataDirectory(), CHECK_NET);
		int hFileCheckNet = open(s, O_RDONLY | O_BINARY);
		if ( hFileCheckNet > 0 )
		{
			l = filelength( hFileCheckNet );
			close( hFileCheckNet );
            sprintf( s3, "%ld""k", ( l + 1023 ) / 1024 );
			strcat(s3, "k");
			bprintf( "|#7� |#3--- |#7� |#2%-8s |#7� |#6CHECK |#7� |#2------- |#7� |#2------- |#7�|#2%5s |#7�|#2 --- |#7� |#2--------- |#7�|#2 --- |#7�\r\n", sess->GetNetworkName(), s3);
		}
	}

	sess->bout << "|#7���������������������������������������������������������������������������ͼ\r\n";
	nl();
	sess->thisuser.SetStatus( ss );
	if ( !sess->IsUserOnline() && lines_listed )
	{
		pausescr();
	}
}


void gate_msg(net_header_rec * nh, char *pszMessageText, int nNetNumber, const char *pszAuthorName, unsigned short int *pList, int nFromNetworkNumber )
{
	char newname[256], qn[200], on[200];
	char nm[205];
	int i;

	if (strlen(pszMessageText) < 80)
    {
		char *pszOriginalText = pszMessageText;
		pszMessageText += strlen( pszOriginalText ) + 1;
		unsigned short ntl = static_cast<unsigned short>( nh->length - strlen( pszOriginalText ) - 1 );
		char *ss = strchr(pszMessageText, '\r');
		if (ss && (ss - pszMessageText < 200) && (ss - pszMessageText < ntl))
        {
			strncpy(nm, pszMessageText, ss - pszMessageText);
			nm[ss - pszMessageText] = 0;
			ss++;
			if (*ss == '\n')
            {
				ss++;
            }
			nh->length -= (ss - pszMessageText);
			ntl = ntl - static_cast<unsigned short>( ss - pszMessageText );
			pszMessageText = ss;

			qn[0] = on[0] = '\0';

			if ( nFromNetworkNumber == 65535 || nh->fromsys == net_networks[nFromNetworkNumber].sysnum )
            {

				strcpy(newname, nm);
				ss = strrchr(newname, '@');
				if (ss)
                {
					sprintf(ss + 1, "%u", net_networks[nNetNumber].sysnum);
					ss = strrchr(nm, '@');
					if (ss)
                    {
						++ss;
						while ((*ss >= '0') && (*ss <= '9'))
                        {
							++ss;
                        }
						strcat(newname, ss);
					}
					strcat(newname, "\r\n");
					nh->fromsys = net_networks[nNetNumber].sysnum;
				}
			}
            else
            {
				if ((nm[0] == '`') && (nm[1] == '`'))
                {
					for (i = strlen(nm) - 2; i > 0; i--)
                    {
						if ((nm[i] == '`') && (nm[i + 1] == '`'))
                        {
							break;
						}
					}
					if (i > 0)
                    {
						i += 2;
						strncpy(qn, nm, i);
						qn[i] = ' ';
						qn[i + 1] = 0;
					}
				}
                else
                {
					i = 0;
                }
				if (qn[0] == 0)
                {
					ss = strrchr(nm, '#');
					if (ss)
                    {
						if ((ss[1] >= '0') && (ss[1] <= '9'))
                        {
							*ss = 0;
							ss--;
							while ((ss > nm) && (*ss == ' '))
                            {
								*ss = 0;
								ss--;
							}
						}
					}
					if (nm[0])
                    {
						if (nh->fromuser)
                        {
							sprintf(qn, "``%s`` ", nm);
                        }
						else
                        {
							strcpy(on, nm);
                        }
					}
				}
				if ((on[0] == 0) && (nh->fromuser == 0))
                {
					strcpy(on, nm + i);
				}
				if ( net_networks[nFromNetworkNumber].sysnum == 1 && on[0] &&
                     wwiv::stringUtils::IsEqualsIgnoreCase( net_networks[nFromNetworkNumber].name, "Internet" ) )
                {
					sprintf(newname, "%s%s", qn, on);
				}
				else
				{
					if (on[0])
                    {
						sprintf(newname, "%s%s@%u.%s\r\n", qn, on, nh->fromsys,
						        net_networks[nFromNetworkNumber].name);
                    }
					else
                    {
						sprintf(newname, "%s#%u@%u.%s\r\n", qn, nh->fromuser, nh->fromsys,
						        net_networks[nFromNetworkNumber].name);
                    }
				}
				nh->fromsys = net_networks[nNetNumber].sysnum;
				nh->fromuser = 0;
			}


			nh->length += strlen(newname);
			if ((nh->main_type == main_type_email_name) ||
				(nh->main_type == main_type_new_post))
            {
				nh->length += strlen( pszAuthorName ) + 1;
            }
            char szPacketFileName[ MAX_PATH ];
			sprintf( szPacketFileName, "%sP1%s", net_networks[nNetNumber].dir, app->GetNetworkExtension() );
            WFile packetFile( szPacketFileName );
            if ( packetFile.Open( WFile::modeReadWrite | WFile::modeBinary | WFile::modeCreateFile, WFile::shareUnknown, WFile::permReadWrite ) )
            {
                packetFile.Seek( 0L, WFile::seekEnd );
				if (!pList)
                {
					nh->list_len = 0;
                }
				if (nh->list_len)
                {
					nh->tosys = 0;
                }
                packetFile.Write( nh, sizeof( net_header_rec ) );
				if (nh->list_len)
                {
                    packetFile.Write( pList, 2 * ( nh->list_len ) );
                }
				if ((nh->main_type == main_type_email_name) || (nh->main_type == main_type_new_post))
                {
                    packetFile.Write( const_cast<char*>( pszAuthorName ), strlen( pszAuthorName ) + 1);
                }
                packetFile.Write( pszOriginalText, strlen( pszOriginalText ) + 1 );
                packetFile.Write( newname, strlen( newname ) );
                packetFile.Write( pszMessageText, ntl );
                packetFile.Close();
			}
        }
    }
}

// begin callout additions

void print_call(int sn, int nNetNumber, int i2)
{
	static int color, got_color = 0;

	char s[100], s1[100];
	long l, l2;

	time(&l);

	set_net_num(nNetNumber);
	read_call_out_list();
	read_contacts();

    net_contact_rec *ncn = net_networks[sess->GetNetworkNumber()].ncn;
	net_system_list_rec *csne = next_system(sn);

	if (!got_color)
    {
		got_color = 1;
		color = 30;

		if (ini_init(WWIV_INI, INI_TAG, NULL))
        {
            char *ss = ini_get("CALLOUT_COLOR_TEXT", -1, NULL);
			if ( ss != NULL )
            {
				color = atoi( ss );
			}
			ini_done();
		}
	}
	curatr = color;
	sprintf(s1, "%ldk", bytes_to_k(ncn[i2].bytes_waiting));
	app->localIO->LocalXYAPrintf( 58, 17, color, "%-10.16s", s1 );

	sprintf(s1, "%ldk", bytes_to_k(ncn[i2].bytes_received));
	app->localIO->LocalXYAPrintf( 23, 17, color, "%-10.16s", s1 );

    sprintf(s1, "%ldk", bytes_to_k(ncn[i2].bytes_sent));
	app->localIO->LocalXYAPrintf( 23, 18, color, "%-10.16s", s1 );

	if (ncn[i2].firstcontact)
    {
		sprintf(s1, "%ld:", (l - ncn[i2].firstcontact) / SECONDS_PER_HOUR);
		l2 = (((l - ncn[i2].firstcontact) % SECONDS_PER_HOUR) / 60);
        sprintf( s, "%d", l2 );
		if ( l2 < 10 )
        {
			strcat(s1, "0");
			strcat(s1, s);
		}
        else
        {
			strcat(s1, s);
        }
		strcat(s1, " Hrs");
	}
    else
    {
		strcpy(s1, "NEVER");
    }
	app->localIO->LocalXYAPrintf( 23, 16, color, "%-17.16s", s1 );

	if (ncn[i2].lastcontactsent)
    {
		sprintf(s1, "%ld:", (l - ncn[i2].lastcontactsent) / SECONDS_PER_HOUR);
		l2 = (((l - ncn[i2].lastcontactsent) % SECONDS_PER_HOUR) / 60);
		sprintf( s, "%d", l2 );
		if (l2 < 10)
        {
			strcat(s1, "0");
			strcat(s1, s);
		}
        else
        {
			strcat(s1, s);
        }
		strcat(s1, " Hrs");
	}
    else
    {
		strcpy(s1, "NEVER");
    }
	app->localIO->LocalXYAPrintf( 58, 16, color, "%-17.16s", s1 );

	if (ncn[i2].lasttry)
    {
		sprintf(s1, "%ld:", (l - ncn[i2].lasttry) / SECONDS_PER_HOUR);
		l2 = (((l - ncn[i2].lasttry) % SECONDS_PER_HOUR) / 60);
		sprintf( s, "%d", l2 );
		if (l2 < 10)
        {
			strcat(s1, "0");
			strcat(s1, s);
		}
        else
        {
			strcat(s1, s);
        }
		strcat(s1, " Hrs");
	}
    else
    {
		strcpy(s1, "NEVER");
    }
	app->localIO->LocalXYAPrintf( 58, 15, color, "%-17.16s", s1 );
	app->localIO->LocalXYAPrintf( 23, 15, color, "%-16u", ncn[i2].numcontacts );
	app->localIO->LocalXYAPrintf( 41, 3, color, "%-30.30s", csne->name );
	app->localIO->LocalXYAPrintf( 23, 19, color, "%-17.17s", csne->phone );
	sprintf( s1, "%u Bps", csne->speed );
	app->localIO->LocalXYAPrintf( 58, 18, color, "%-10.16s", s1 );
	describe_area_code(atoi(csne->phone), s1);
	app->localIO->LocalXYAPrintf( 58, 19, color, "%-17.17s", stripcolors( s1 ) );
	app->localIO->LocalXYAPrintf( 14, 3, color, "%-11.16s", sess->GetNetworkName() );
}

void fill_call(int color, int row, int netmax, int *nodenum)
{
	int i, x = 0, y = 0;
	char s1[6];

	curatr = color;
	for (i = row * 10; (i < ((row + 6) * 10)); i++)
    {
		if (x > 69)
        {
			x = 0;
			y++;
		}
		if (i < netmax)
        {
			sprintf(s1, "%-5u", nodenum[i]);
        }
		else
        {
			strcpy(s1, "     ");
        }
		app->localIO->LocalXYPuts( 6 + x, 5 + y, s1 );
		x += 7;
	}
}

#define MAX_CONNECTS 2000


int ansicallout()
{
    static int callout_ansi, color1, color2, color3, color4, got_info = 0;

    char ch = 0, *ss;
    int i, i1, nNetNumber, netnum = 0, x = 0, y = 0, pos = 0, sn = 0;
    int num_ncn, num_call_sys, rownum = 0, *nodenum, *netpos, *ipos;
    net_contact_rec *ncn;
    net_call_out_rec *con;
#ifndef _UNIX
    app->localIO->SetCursor( WLocalIO::cursorNone );
    holdphone( true );
#endif
    if (!got_info)
    {
        got_info = 1;
        callout_ansi = 0;
        color1 = 31;
        color2 = 59;
        color3 = 7;
        color4 = 30;
        if (ini_init(WWIV_INI, INI_TAG, NULL))
        {
            if ((ss = ini_get("CALLOUT_ANSI", -1, NULL)) != NULL)
            {
				if ( wwiv::UpperCase<char>(ss[0]) == 'Y' )
                {
                    callout_ansi = 1;
                }
            }
            if ((ss = ini_get("CALLOUT_COLOR", -1, NULL)) != NULL)
            {
                color1 = atoi(ss);
            }
            if ((ss = ini_get("CALLOUT_HIGHLIGHT", -1, NULL)) != NULL)
            {
                color2 = atoi(ss);
            }
            if ((ss = ini_get("CALLOUT_NORMAL", -1, NULL)) != NULL)
            {
                color3 = atoi(ss);
            }
            if ((ss = ini_get("CALLOUT_COLOR_TEXT", -1, NULL)) != NULL)
            {
                color4 = atoi(ss);
            }
            ini_done();
        }
    }

    if (callout_ansi)
    {
        nodenum = static_cast<int *>( BbsAllocA(MAX_CONNECTS * 2 ) );
        netpos = static_cast<int *>( BbsAllocA(MAX_CONNECTS * 2 ) );
        ipos = static_cast<int *>( BbsAllocA(MAX_CONNECTS * 2 ) );
        for (nNetNumber = 0; nNetNumber < sess->GetMaxNetworkNumber(); nNetNumber++)
        {
            set_net_num(nNetNumber);
            read_call_out_list();
            read_contacts();

            con = net_networks[sess->GetNetworkNumber()].con;
            ncn = net_networks[sess->GetNetworkNumber()].ncn;
            num_call_sys = net_networks[sess->GetNetworkNumber()].num_con;
            num_ncn = net_networks[sess->GetNetworkNumber()].num_ncn;

            for (i1 = 0; i1 < num_call_sys; i1++)
            {
                for (i = 0; i < num_ncn; i++)
                {
                    if ((!(con[i1].options & options_hide_pend)) &&
                        (con[i1].sysnum == ncn[i].systemnumber) &&
                        (valid_system(con[i1].sysnum)))
                    {
                        ipos[netnum] = i;
                        netpos[netnum] = nNetNumber;
                        nodenum[netnum++] = ncn[i].systemnumber;
                        break;
                    }
                }
            }
            if (netnum > MAX_CONNECTS)
            {
                break;
            }
        }

        app->localIO->LocalCls();
        curatr = color1;
        app->localIO->MakeLocalWindow( 3, 2, 73, 10 );
        app->localIO->LocalXYAPrintf( 3, 4, color1, "�%s�", charstr( 71, '�' ) );
        app->localIO->MakeLocalWindow( 3, 14, 73, 7 );
        app->localIO->LocalXYAPrintf( 5, 3,   color3, "NetWork:" );
        app->localIO->LocalXYAPrintf( 31, 3,  color3, "BBS Name:" );
        app->localIO->LocalXYAPrintf( 5, 15,  color3, "Contact Number  :" );
        app->localIO->LocalXYAPrintf( 5, 16,  color3, "First Contact   :" );
        app->localIO->LocalXYAPrintf( 5, 17,  color3, "Bytes Received  :" );
        app->localIO->LocalXYAPrintf( 5, 18,  color3, "Bytes Sent      :" );
        app->localIO->LocalXYAPrintf( 5, 19,  color3, "Phone Number    :" );
        app->localIO->LocalXYAPrintf( 40, 15, color3, "Last Try Contact:" );
        app->localIO->LocalXYAPrintf( 40, 16, color3, "Last Contact    :" );
        app->localIO->LocalXYAPrintf( 40, 17, color3, "Bytes Waiting   :" );
        app->localIO->LocalXYAPrintf( 40, 18, color3, "Max Speed       :" );
        app->localIO->LocalXYAPrintf( 40, 19, color3, "System Location :" );

        fill_call( color4, rownum, netnum, nodenum );
        curatr = color2;
        x = 0;
        y = 0;
        app->localIO->LocalXYAPrintf( 6, 5, color2, "%-5u", nodenum[pos]);
        print_call( nodenum[pos], netpos[pos], ipos[pos] );

		bool done = false;
        do
        {
            ch = wwiv::UpperCase<char>( static_cast<char>( app->localIO->LocalGetChar() ) );
            switch (ch)
            {
            case ' ':
            case RETURN:
                sn = nodenum[pos];
                done = true;
                break;
            case 'Q':
            case ESC:
                sn = 0;
                done = true;
                break;
            case -32: // (224) I don't know MS's CRT returns this on arrow keys....
            case 0:
                ch = wwiv::UpperCase<char>( static_cast<char>( app->localIO->LocalGetChar() ) );
                switch (ch)
                {
                case RARROW:                        // right arrow
                    if ((pos < netnum - 1) && (x < 63))
                    {
                        app->localIO->LocalXYAPrintf( 6 + x, 5 + y, color4, "%-5u", nodenum[pos] );
                        pos++;
                        x += 7;
                        app->localIO->LocalXYAPrintf(6 + x, 5 + y, color2, "%-5u", nodenum[pos]);
                        print_call(nodenum[pos], netpos[pos], ipos[pos]);
                    }
                    break;
                case LARROW:                        // left arrow
                    if (x > 0)
                    {
                        curatr = color4;
                        app->localIO->LocalXYAPrintf( 6 + x, 5 + y, color4, "%-5u", nodenum[pos] );
                        pos--;
                        x -= 7;
                        curatr = color2;
                        app->localIO->LocalXYAPrintf( 6 + x, 5 + y, color2, "%-5u", nodenum[pos] );
                        print_call(nodenum[pos], netpos[pos], ipos[pos]);
                    }
                    break;
                case UPARROW:                        // up arrow
                    if (y > 0)
                    {
                        app->localIO->LocalXYAPrintf( 6 + x, 5 + y, color4, "%-5u", nodenum[pos] );
                        pos -= 10;
                        y--;
                        app->localIO->LocalXYAPrintf( 6 + x, 5 + y, color2, "%-5u", nodenum[pos] );
                        print_call(nodenum[pos], netpos[pos], ipos[pos]);
                    }
                    else if (rownum > 0)
                    {
                        pos -= 10;
                        rownum--;
                        fill_call(color4, rownum, netnum, nodenum);
                        app->localIO->LocalXYAPrintf( 6 + x, 5 + y, color2, "%-5u", nodenum[pos] );
                        print_call(nodenum[pos], netpos[pos], ipos[pos]);
                    }
                    break;
                case DNARROW:                        // down arrow
                    if ((y < 5) && (pos + 10 < netnum))
                    {
                        app->localIO->LocalXYAPrintf( 6 + x, 5 + y, color4, "%-5u", nodenum[pos] );
                        pos += 10;
                        y++;
                    }
                    else if ((rownum + 6) * 10 < netnum)
                    {
                        rownum++;
                        fill_call(color4, rownum, netnum, nodenum);
                        if (pos + 10 < netnum)
					    {
                            pos += 10;
					    }
                        else
					    {
                            --y;
					    }
                    }
                    curatr = color2;
                    app->localIO->LocalXYAPrintf( 6 + x, 5 + y, color2, "%-5u", nodenum[pos] );
                    print_call(nodenum[pos], netpos[pos], ipos[pos]);
                    break;
                case HOME:                        // home
                    if (pos > 0)
                    {
                        x = 0;
                        y = 0;
                        pos = 0;
                        rownum = 0;
                        fill_call(color4, rownum, netnum, nodenum);
                        app->localIO->LocalXYAPrintf( 6, 5, color2, "%-5u", nodenum[pos] );
                        print_call(nodenum[pos], netpos[pos], ipos[pos]);
                    }
                case PAGEUP:                        // page up
                    if (y > 0)
                    {
                        app->localIO->LocalXYAPrintf( 6 + x, 5 + y, color4, "%-5u", nodenum[pos] );
                        pos -= 10 * y;
                        y = 0;
                        app->localIO->LocalXYAPrintf( 6 + x, 5 + y, color2, "%-5u", nodenum[pos] );
                        print_call(nodenum[pos], netpos[pos], ipos[pos]);
                    }
                    else
                    {
                        if (rownum > 5)
                        {
                            pos -= 60;
                            rownum -= 6;
                        }
                        else
                        {
                            pos -= 10 * rownum;
                            rownum = 0;
                        }
                        fill_call(color4, rownum, netnum, nodenum);
                        app->localIO->LocalXYAPrintf( 6 + x, 5 + y, color2, "%-5u", nodenum[pos] );
                        print_call(nodenum[pos], netpos[pos], ipos[pos]);
                    }
                    break;
                case PAGEDN:                        // page down
                    if (y < 5)
                    {
                        app->localIO->LocalXYAPrintf( 6 + x, 5 + y, color4, "%-5u", nodenum[pos] );
                        pos += 10 * (5 - y);
                        y = 5;
                        if (pos >= netnum)
                        {
                            pos -= 10;
                            --y;
                        }
                        app->localIO->LocalXYAPrintf( 6 + x, 5 + y, color2, "%-5u", nodenum[pos] );
                        print_call(nodenum[pos], netpos[pos], ipos[pos]);
                    }
                    else if ( ( rownum + 6 ) * 10 < netnum )
                    {
                        for ( i1 = 0; i1 < 6; i1++ )
                        {
                            if ( ( rownum + 6 ) * 10 < netnum )
                            {
                                rownum++;
                                pos += 10;
                            }
                        }
                        fill_call(color4, rownum, netnum, nodenum);
                        if ( pos >= netnum )
					    {
                            pos -= 10;
                            --y;
                        }
                        app->localIO->LocalXYAPrintf( 6 + x, 5 + y, color2, "%-5u", nodenum[pos] );
                        print_call( nodenum[pos], netpos[pos], ipos[pos] );
                    }
                    break;
                }
            }
        } while ( !done );
        app->localIO->SetCursor( WLocalIO::cursorNormal );
        curatr = color3;
        app->localIO->LocalCls();
        netw = ( netpos[pos] );
        BbsFreeMemory( nodenum );
        BbsFreeMemory( netpos );
        BbsFreeMemory( ipos );
    }
    else
    {
        nl();
        sess->bout << "|#2Which system: ";
        char szSystemNumber[ 11 ];
        input( szSystemNumber, 5, true );
        sn = atoi( szSystemNumber );
    }
#ifndef _UNIX
    holdphone( false );
    app->localIO->SetCursor( WLocalIO::cursorNormal );
#endif
    std::cout << "System: " << sn << std::endl;
    return sn;
}


void force_callout(int dw)
{
    int i, i1, i2;
    bool  abort = false;
    bool ok;
    unsigned int nr = 1, tc = 0;
    long l;
    char ch, s[101], onx[20];
    net_system_list_rec *csne;
    unsigned long lc, cc;

    time(&l);
    int sn = ansicallout();
    if (!sn)
    {
        return;
    }

    odc[0]  = '\0';
    int odci    = 0;
    onx[0]  = 'Q';
    onx[1]  = '\0';
    int onxi = 1;
    int nv = 0;
    char *ss = static_cast<char *>( BbsAllocA( sess->GetMaxNetworkNumber() * 3 ) );
    char *ss1 = ss + sess->GetMaxNetworkNumber();
    char *ss2 = ss1 + sess->GetMaxNetworkNumber();

    for ( int nNetNumber = 0; nNetNumber < sess->GetMaxNetworkNumber(); nNetNumber++ )
    {
        set_net_num( nNetNumber );
        if ( !net_sysnum || net_sysnum == sn )
        {
            continue;
        }

        if ( !net_networks[sess->GetNetworkNumber()].con )
        {
            read_call_out_list();
        }

        i = -1;
        for ( i1 = 0; i1 < net_networks[sess->GetNetworkNumber()].num_con; i1++ )
        {
            if ( net_networks[sess->GetNetworkNumber()].con[i1].sysnum == sn )
            {
                i = i1;
                break;
            }
        }

        if ( i != -1 )
        {
            if ( !net_networks[sess->GetNetworkNumber()].ncn )
            {
                read_contacts();
            }

            i2 = -1;
            for ( i1 = 0; i1 < net_networks[sess->GetNetworkNumber()].num_ncn; i1++ )
            {
                if ( net_networks[sess->GetNetworkNumber()].ncn[i1].systemnumber == sn )
                {
                    i2 = i1;
                    break;
                }
            }

            if (i2 != -1)
            {
                ss[nv] = static_cast< char >( nNetNumber );
                ss1[nv] = static_cast< char >( i );
                ss2[nv++] = static_cast< char >( i2 );
            }
        }
    }

    int nitu = -1;
    if ( nv )
    {
        if ( nv == 1 )
        {
            nitu = 0;
        }
        else
        {
            nl();
            for ( i = 0; i < nv; i++ )
            {
                set_net_num(ss[i]);
                csne = next_system(sn);
                if ( csne )
                {
                    if ( i < 9 )
                    {
                        onx[onxi++] = static_cast< char >( i + '1' );
                        onx[onxi] = 0;
                    }
                    else
                    {
                        odci = ( i + 1 ) / 10;
                        odc[odci - 1] = static_cast< char >( odci + '0' );
                        odc[odci] = 0;
                    }
                    if ( wwiv::stringUtils::IsEqualsIgnoreCase( net_networks[netw].name, sess->GetNetworkName() ) )
                    {
                        nitu = i;
                    }
                }
            }
        }
    }
    if ( nitu != -1 )
    {
        set_net_num( ss[nitu] );
        ok = ok_to_call( ss1[nitu] );

        if ( !ok )
        {
            nl();
            sess->bout <<  "|#5Are you sure? ";
            if ( yesno() )
            {
                ok = true;
            }
        }
        if ( ok )
        {
            if ( net_networks[sess->GetNetworkNumber()].ncn[ss2[nitu]].bytes_waiting == 0L )
            {
                if ( !( net_networks[sess->GetNetworkNumber()].con[ss1[nitu]].options & options_sendback ) )
                {
                    ok = false;
                }
                if ( ok )
                {
                    if ( dw )
                    {
                        nl();
                        sess->bout << "|#2Num Retries : ";
                        input( s, 5, true );
                        nr = atoi( s );
                    }
                    if ( dw == 2 )
                    {
                        if ( sess->IsUserOnline() )
                        {
                            sess->WriteCurrentUser( sess->usernum );
                            write_qscn(sess->usernum, qsc, false);
                            sess->SetUserOnline( false );
                        }
                        hang_it_up();
                        wait1( 90 );
                    }
                    if ( !dw || nr < 1 )
                    {
                        nr = 1;
                    }

                    read_contacts();
                    lc = net_networks[sess->GetNetworkNumber()].ncn[ss2[nitu]].lastcontact;
                    while ((tc < nr) && (!abort))
                    {
                        if ( app->localIO->LocalKeyPressed() )
                        {
                            while ( app->localIO->LocalKeyPressed() )
                            {
                                ch = wwiv::UpperCase<char>(app->localIO->getchd1());
                                if ( !abort )
                                {
                                    abort = ( ch == ESC ) ? true : false;
                                }
                            }
                        }
                        tc++;
                        set_net_num( ss[nitu] );
                        read_contacts();
                        cc = net_networks[sess->GetNetworkNumber()].ncn[ss2[nitu]].lastcontact;
                        if ( abort || cc != lc )
                        {
                            break;
                        }
                        else
                        {
                            app->localIO->LocalCls();
                            sess->bout << "|#9Retries |#0= |#2" << nr << "|#9, Current |#0= |#2" << tc << "|#9, Remaining |#0= |#2" << nr - tc << "|#9. ESC to abort.\r\n";
                            if ( nr == tc )
                            {
                                BbsFreeMemory( ss );
                                ss = NULL;
                            }
                            do_callout( sn );
                        }
                    }
                }
            }
        }
    }
    if (ss)
    {
        BbsFreeMemory(ss);
    }
}

// end callout additions

long *next_system_reg( int ts )
{
	static long reg_num;

	if ( sess->GetNetworkNumber() != -1 )
    {
		read_bbs_list_index();
    }

	if ( csn )
    {
		for ( int i = 0; i < sess->num_sys_list; i++ )
        {
			if ( i == ts )
            {
				if ( i == ( sess->num_sys_list - 1 ) )
                {
					return NULL;
                }
				else
                {
					return ( long* )( &( csn[i] ) );
                }
            }
        }
	}
    else
    {
		for ( int i = 0; i < sess->num_sys_list; i++ )
        {
			if ( csn_index[i] == ts )
            {
                WFile bbsdataReg( sess->GetNetworkDataDirectory(), BBSDATA_REG );
                bbsdataReg.Open( WFile::modeBinary | WFile::modeReadOnly );
                bbsdataReg.Seek( i * sizeof( long ), WFile::seekBegin );
                bbsdataReg.Read( &reg_num, sizeof( long ) );
                bbsdataReg.Close();
				if ( i == sess->num_sys_list - 1 )
                {
					return NULL;
                }
				else
                {
					return &reg_num;
                }
			}
		}
	}
	return NULL;
}


#ifndef _UNUX
void run_exp()
{
	int nOldNetworkNumber = sess->GetNetworkNumber();
	int nFileNetNetworkNumber = getnetnum("FILEnet");
	if ( nFileNetNetworkNumber == -1 )
    {
		return;
    }
	set_net_num( nFileNetNetworkNumber );

	char szExpCommand[MAX_PATH];
    sprintf( szExpCommand, "EXP S32767.NET %s %d %s %s %s", sess->GetNetworkDataDirectory(), net_sysnum, sess->internetEmailName.c_str(), sess->internetEmailDomain.c_str(), sess->GetNetworkName() );
	ExecuteExternalProgram( szExpCommand, EFLAG_NETPROG );

	set_net_num( nOldNetworkNumber );
	app->localIO->LocalCls();
}
#endif



