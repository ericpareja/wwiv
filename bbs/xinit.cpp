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

// Additional INI file function and structure
#include "ini.h"
#include "xinitini.h"


// %%TODO: Turn this warning back on once the rest of the code is cleaned up
#if defined( _MSC_VER )
#pragma warning( push )
#pragma warning( disable : 4244 )
#endif


//
// Local function prototypes
//

/*
unsigned short WBbsApp::str2spawnopt(char *s);
unsigned short WBbsApp::str2restrict(char *s);
unsigned char WBbsApp::stryn2tf(char *s);
void WBbsApp::read_nextern();
void WBbsApp::read_arcs();
void WBbsApp::read_editors();
void WBbsApp::read_nintern();
void WBbsApp::read_networks();
bool WBbsApp::read_names();
void WBbsApp::read_voting();
bool WBbsApp::read_dirs();
void WBbsApp::read_chains();
bool WBbsApp::read_language();
bool WBbsApp::read_modem();
void WBbsApp::read_gfile();
bool WBbsApp::make_abs_path(char *pszDirectory);
void WBbsApp::check_phonenum();
void WBbsApp::create_phone_file();
*/

#ifdef _UNIX
#define XINIT_PRINTF(x)
#else
#define XINIT_PRINTF(x) std::cout << (x)
#endif // _UNIX

#define INI_STRFILE 7


// Turns a string into a bitmapped unsigned short flag for use with
// ExecuteExternalProgram calls.
unsigned short WBbsApp::str2spawnopt(char *s)
{
    char ts[128];

    unsigned short return_val = 0;
    strcpy( ts, (const char*) s );
    strupr( ts );

    if (strstr(ts, "ABORT") != NULL)
    {
        return_val |= EFLAG_ABORT;
    }
    if (strstr(ts, "INTERNAL") != NULL)
    {
        return_val |= EFLAG_INTERNAL;
    }
    if (strstr(ts, "NOHUP") != NULL)
    {
        return_val |= EFLAG_NOHUP;
    }
    if (strstr(ts, "COMIO") != NULL)
    {
        return_val |= EFLAG_COMIO;
    }
    if (strstr(ts, "FOSSIL") != NULL)
    {
        return_val |= EFLAG_FOSSIL; // RF20020929
    }
    if (strstr(ts, "NOPAUSE") != NULL)
    {
        return_val |= EFLAG_NOPAUSE;
    }
    if (strstr(ts, "NETPROG") != NULL)
    {
        return_val |= EFLAG_NETPROG;
    }
    if (strstr(ts, "TOPSCREEN") != NULL)
    {
        return_val |= EFLAG_TOPSCREEN;
    }

    return return_val;
}


// Takes string s and creates restrict val
unsigned short WBbsApp::str2restrict(char *s)
{
    char *rs = restrict_string;
    char s1[81];

    strcpy(s1, s);
    strupr(s1);
    int r = 0;
    for ( int i = strlen(rs) - 1; i >= 0; i-- )
    {
        if ( strchr( s1, rs[i] ) )
        {
            r |= ( 1 << i );
        }
    }

    return static_cast< short >( r );
}


// begin callback addition

unsigned char WBbsApp::stryn2tf(char *s)
{
	char ch = wwiv::UpperCase<char>( *s );

	char yesChar = *( YesNoString( true ) );
	if ( ch == yesChar || ch == 1 || ch == '1' || ch == 'T' )
	{
		return 1;
	}
	return 0;
}


// end callback addition


#define OFFOF(x) ( reinterpret_cast<long>( &sess->thisuser.data.x ) - reinterpret_cast<long>( &sess->thisuser.data ) )


// Reads WWIV.INI info from [WWIV] subsection, overrides some config.dat
// settings (as appropriate), updates config.dat with those values. Also
// tries to read settings from [WWIV-<instnum>] subsection - this overrides
// those in [WWIV] subsection.


static unsigned char nucol[] = {7, 11, 14, 5, 31, 2, 12, 9, 6, 3};
static unsigned char nucolbw[] = {7, 15, 15, 15, 112, 15, 15, 7, 7, 7};


struct eventinfo_t
{
    char *name;
    unsigned short eflags;
};


static eventinfo_t eventinfo[] =
{
    {"TIMED",         0},
    {"NEWUSER",       0},
    {"BEGINDAY",      0},
    {"LOGON",         EFLAG_COMIO | EFLAG_INTERNAL},
    {"ULCHK",         EFLAG_NOHUP},
    {"FSED",          EFLAG_COMIO},
    {"PROT_SINGLE",   0},
    {"PROT_BATCH",    EFLAG_TOPSCREEN},
    {"CHAT",          0},
    {"ARCH_E",        EFLAG_COMIO | EFLAG_INTERNAL},
    {"ARCH_L",        EFLAG_COMIO | EFLAG_INTERNAL | EFLAG_ABORT},
    {"ARCH_A",        EFLAG_COMIO | EFLAG_INTERNAL},
    {"ARCH_D",        EFLAG_COMIO | EFLAG_INTERNAL},
    {"ARCH_K",        EFLAG_COMIO | EFLAG_INTERNAL},
    {"ARCH_T",        EFLAG_COMIO | EFLAG_INTERNAL},
    {"NET_CMD1",      EFLAG_INTERNAL},
    {"NET_CMD2",      EFLAG_NETPROG | EFLAG_INTERNAL},
    {"LOGOFF",        EFLAG_COMIO | EFLAG_INTERNAL},
    {"V_SCAN",        EFLAG_NOPAUSE},
    {"NETWORK",       0},
};


static const char *get_key_str( int n )
{
	return INI_OPTIONS_ARRAY[ n ];
}


#define INI_INIT_A(n,i,f,s) \
{if (((ss=ini_get(get_key_str(n), i, s))!=NULL) && (atoi(ss)>0)) \
{sess->f[i] = atoi(ss);}}
#define INI_INIT(n,f) \
{if (((ss=ini_get(get_key_str(n), -1, NULL))!=NULL) && (atoi(ss)>0)) \
sess->f = atoi(ss);}
#define INI_INIT_N(n,f) \
{if (((ss=ini_get(get_key_str(n), -1, NULL))!=NULL)) \
sess->f = atoi(ss);}
#define INI_GET_ASV(s, f, func, d) \
{if ((ss=ini_get(get_key_str(INI_STR_SIMPLE_ASV), -1, s))!=NULL) \
    sess->asv.f = func (ss); \
    else \
sess->asv.f = d;}
#define INI_GET_ADVANCED_ASV(s, f, func, d) \
{if ((ss=ini_get(get_key_str(INI_STR_ADVANCED_ASV), -1, s))!=NULL) \
    sess->advasv.f = func (ss); \
    else \
sess->advasv.f = d;}
#define INI_GET_CALLBACK(s, f, func, d) \
{if ((ss=ini_get(get_key_str(INI_STR_CALLBACK), -1, s))!=NULL) \
    sess->cbv.f = func (ss); \
    else \
sess->cbv.f = d;}
#define INI_INIT_TF(n,f) { if (((ss=ini_get(get_key_str(n), -1, NULL))!=NULL)) sess->f = stryn2tf(ss); }

#define NEL(s) (sizeof(s) / sizeof((s)[0]))


static ini_flags_rec sysinfo_flags[] =
{
    {INI_STR_FORCE_FBACK, 0, OP_FLAGS_FORCE_NEWUSER_FEEDBACK},
    {INI_STR_CHECK_DUP_PHONES, 0, OP_FLAGS_CHECK_DUPE_PHONENUM},
    {INI_STR_HANGUP_DUP_PHONES, 0, OP_FLAGS_HANGUP_DUPE_PHONENUM},
    {INI_STR_USE_SIMPLE_ASV, 0, OP_FLAGS_SIMPLE_ASV},
    {INI_STR_POSTTIME_COMPENS, 0, OP_FLAGS_POSTTIME_COMPENSATE},
    {INI_STR_SHOW_HIER, 0, OP_FLAGS_SHOW_HIER},
    {INI_STR_IDZ_DESC, 0, OP_FLAGS_IDZ_DESC},
    {INI_STR_SETLDATE, 0, OP_FLAGS_SETLDATE},
    {INI_STR_SLASH_SZ, 0, OP_FLAGS_SLASH_SZ},
    {INI_STR_READ_CD_IDZ, 0, OP_FLAGS_READ_CD_IDZ},
    {INI_STR_FSED_EXT_DESC, 0, OP_FLAGS_FSED_EXT_DESC},
    {INI_STR_FAST_TAG_RELIST, 0, OP_FLAGS_FAST_TAG_RELIST},
    {INI_STR_MAIL_PROMPT, 0, OP_FLAGS_MAIL_PROMPT},
    {INI_STR_SHOW_CITY_ST, 0, OP_FLAGS_SHOW_CITY_ST},
    {INI_STR_NO_EASY_DL, 0, OP_FLAGS_NO_EASY_DL},
    {INI_STR_NEW_EXTRACT, 0, OP_FLAGS_NEW_EXTRACT},
    {INI_STR_FAST_SEARCH, 0, OP_FLAGS_FAST_SEARCH},
    {INI_STR_NET_CALLOUT, 0, OP_FLAGS_NET_CALLOUT},
    {INI_STR_WFC_SCREEN, 0, OP_FLAGS_WFC_SCREEN},
    {INI_STR_FIDO_PROCESS, 0, OP_FLAGS_FIDO_PROCESS},
    {INI_STR_USER_REGISTRATION, 0, OP_FLAGS_USER_REGISTRATION},
    {INI_STR_MSG_TAG, 0, OP_FLAGS_MSG_TAG},
    {INI_STR_CHAIN_REG, 0, OP_FLAGS_CHAIN_REG},
    {INI_STR_CAN_SAVE_SSM, 0, OP_FLAGS_CAN_SAVE_SSM},
    {INI_STR_EXTRA_COLOR, 0, OP_FLAGS_EXTRA_COLOR},
    {INI_STR_THREAD_SUBS, 0, OP_FLAGS_THREAD_SUBS},
    {INI_STR_USE_CALLBACK, 0, OP_FLAGS_CALLBACK},
    {INI_STR_USE_VOICE_VAL, 0, OP_FLAGS_VOICE_VAL},
    {INI_STR_USE_ADVANCED_ASV, 0, OP_FLAGS_ADV_ASV},
    {INI_STR_USE_FORCE_SCAN, 0, OP_FLAGS_USE_FORCESCAN},
    {INI_STR_NEWUSER_MIN, 0, OP_FLAGS_NEWUSER_MIN},

};

static ini_flags_rec sysconfig_flags[] =
{
    {INI_STR_LOCAL_SYSOP, 1, sysconfig_no_local},
    {INI_STR_2WAY_CHAT, 0, sysconfig_2_way},
    {INI_STR_OFF_HOOK, 0, sysconfig_off_hook},
    {INI_STR_TITLEBAR, 0, sysconfig_titlebar},
    {INI_STR_LOG_DOWNLOADS, 0, sysconfig_log_dl},
    {INI_STR_CLOSE_XFER, 0, sysconfig_no_xfer},
    {INI_STR_ALL_UL_TO_SYSOP, 0, sysconfig_all_sysop},
    {INI_STR_BEEP_CHAT, 1, sysconfig_no_beep},
    {INI_STR_TWO_COLOR_CHAT, 0, sysconfig_two_color},
    {INI_STR_ALLOW_ALIASES, 1, sysconfig_no_alias},
    {INI_STR_USE_LIST, 0, sysconfig_list},
    {INI_STR_EXTENDED_USERINFO, 0, sysconfig_extended_info},
    {INI_STR_FREE_PHONE, 0, sysconfig_free_phone},
    {INI_STR_ENABLE_PIPES, 0, sysconfig_enable_pipes},
    {INI_STR_ENABLE_MCI, 0, sysconfig_enable_mci},
};


bool WBbsApp::ReadINIFile()
{
    char *ss;

    // can't allow user to change these on-the-fly
    unsigned short omb = sess->max_batch;
    unsigned short omc = sess->max_chains;
    unsigned short omg = sess->max_gfilesec;

    // Setup default sess-> data

    for (int i = 0; i < 10; i++)
    {
        sess->newuser_colors[i] = nucol[i];
        sess->newuser_bwcolors[i] = nucolbw[i];
    }

    sess->SetTopScreenColor( 31 );
    sess->SetUserEditorColor( 31 );
    sess->SetEditLineColor( 112 );
    sess->SetChatNameSelectionColor( 95 );
    sess->SetMessageColor( 2 );
    sess->max_batch = 50;
    sess->max_extend_lines = 10;
    sess->max_chains = 50;
    sess->max_gfilesec = 32;
    sess->mail_who_field_len = 35;
    sess->SetBeginDayNodeNumber( 1 );
    sess->SetUseInternalZmodem( true );
	sess->SetExecUseWaitForInputIdle( true );
    sess->SetExecWaitForInputTimeout( 2000 );
	sess->SetExecChildProcessWaitTime( 500 );
	sess->SetExecLogSyncFoss( true );
	sess->SetNewScanAtLogin( false );

    for ( unsigned int i1 = 0; i1 < NEL( eventinfo ); i1++ )
    {
        app->spawn_opts[ i1 ] = eventinfo[ i1 ].eflags;
    }

    // put in default WBbsApp::flags
    app->SetConfigFlags( OP_FLAGS_FIDO_PROCESS );

    if ( ok_modem_stuff )
    {
        app->SetConfigFlag( OP_FLAGS_NET_CALLOUT );
    }
    else
    {
        app->ClearConfigFlag( OP_FLAGS_NET_CALLOUT );
    }

    // initialize ini communication
    char szInstanceName[81];
    sprintf( szInstanceName, "WWIV-%u", GetInstanceNumber() );
    if ( ini_init( WWIV_INI, szInstanceName, INI_TAG ) )
    {
        ///////////////////////////////////////////////////////////////////////////////
        // DO NOT DO ANYTHING HERE WHICH ALLOCATES MEMORY
        // the ini_init has allocated a lot of memory, and it will be freed
        // by ini_done.  If you need to read anything here which will cause memory
        // to be allocated, only store the size of the array here, and allocate it
        // after ini_done.  Or, if necessary, call ini_done(), allocate the memory,
        // then call ini_init again, and continue processing.
        ///////////////////////////////////////////////////////////////////////////////

        //
        // found something
        //

        // pull out event flags
        for ( unsigned int i = 0; i < NEL( app->spawn_opts ); i++ )
        {
            if ( (unsigned int)i < NEL( eventinfo ) )
            {
                if ( ( ss = ini_get( get_key_str( INI_STR_SPAWNOPT ), i, eventinfo[i].name ) ) != NULL )
                {
                    app->spawn_opts[i] = str2spawnopt( ss );
                }
            }
            else
            {
                if ( ( ss = ini_get( get_key_str( INI_STR_SPAWNOPT ), i, eventinfo[i].name ) ) != NULL )
                {
                    app->spawn_opts[i] = str2spawnopt( ss );
                }
            }
        }

        // pull out newuser colors
        for ( int i1 = 0; i1 < 10; i1++ )
        {
            INI_INIT_A( INI_STR_NUCOLOR, i1, newuser_colors, NULL );
            INI_INIT_A( INI_STR_NUCOLORBW, i1, newuser_bwcolors, NULL );
        }

        // pull out sysop-side colors
        INI_INIT( INI_STR_TOPCOLOR, m_nTopScreenColor );
        INI_INIT( INI_STR_F1COLOR, m_nUserEditorColor );
        INI_INIT( INI_STR_EDITLINECOLOR, m_nEditLineColor );
        INI_INIT( INI_STR_CHATSELCOLOR, m_nChatNameSelectionColor );

        // pull out sizing options
        INI_INIT( INI_STR_MAX_BATCH, max_batch );
        INI_INIT( INI_STR_MAX_EXTEND_LINES, max_extend_lines );
        INI_INIT( INI_STR_MAX_CHAINS, max_chains );
        INI_INIT( INI_STR_MAX_GFILESEC, max_gfilesec );

        // pull out strings
        //    INI_INIT_STR(INI_STR_TERMINAL_CMD, terminal);
        //    INI_INIT_STR(INI_STR_EXECUTE_CMD, executestr);
        //    INI_INIT_STR(INI_STR_UPLOAD_CMD, upload_c);
        //    INI_INIT_STR(INI_STR_BEGINDAY_CMD, beginday_c);
        //    INI_INIT_STR(INI_STR_NEWUSER_CMD, newuser_c);
        //    INI_INIT_STR(INI_STR_LOGON_CMD, logon_c);
        //    INI_INIT_STR(INI_STR_LOGOFF_CMD, logoff_c);

        INI_INIT_N( INI_STR_FORCE_SCAN_SUBNUM, m_nForcedReadSubNumber );
        INI_INIT_TF( INI_STR_INTERNALZMODEM, m_bInternalZmodem );
		INI_INIT_TF( INI_STR_NEW_SCAN_AT_LOGIN, m_bNewScanAtLogin );

		INI_INIT_TF( INI_STR_EXEC_LOG_SYNCFOSS, m_nExecLogSyncFoss );
		INI_INIT_TF( INI_STR_EXEC_USE_WAIT_FOR_IDLE, m_nExecUseWaitForInputIdle );
		INI_INIT_N( INI_STR_EXEC_CHILD_WAIT_TIME, m_nExecChildProcessWaitTime );
		INI_INIT_N( INI_STR_EXEC_WAIT_FOR_IDLE_TIME, m_nExecUseWaitForInputTimeout );


        INI_INIT( INI_STR_BEGINDAYNODENUMBER, m_nBeginDayNodeNumber );


        // pull out sysinfo_flags
        app->SetConfigFlags( ini_flags( 'Y', get_key_str, sysinfo_flags, NEL(sysinfo_flags), app->GetConfigFlags() ) );

        // allow override of WSession::m_nMessageColor
        INI_INIT( INI_STR_MSG_COLOR, m_nMessageColor );

        // get asv values
        if ( app->HasConfigFlag( OP_FLAGS_SIMPLE_ASV ) )
        {
            INI_GET_ASV("SL", sl, atoi, syscfg.autoval[9].sl);
            INI_GET_ASV("DSL", dsl, atoi, syscfg.autoval[9].dsl);
            INI_GET_ASV("EXEMPT", exempt, atoi, 0);
            INI_GET_ASV("AR", ar, str_to_arword, syscfg.autoval[9].ar);
            INI_GET_ASV("DAR", dar, str_to_arword, syscfg.autoval[9].dar);
            INI_GET_ASV("RESTRICT", restrict, str2restrict, 0);
        }
        if ( app->HasConfigFlag( OP_FLAGS_ADV_ASV ) )
        {
            INI_GET_ADVANCED_ASV("REG_WWIV", reg_wwiv, atoi, 1);
            INI_GET_ADVANCED_ASV("NONREG_WWIV", nonreg_wwiv, atoi, 1);
            INI_GET_ADVANCED_ASV("NON_WWIV", non_wwiv, atoi, 1);
            INI_GET_ADVANCED_ASV("COSYSOP", cosysop, atoi, 1);
        }


        // get callback values
        if (app->HasConfigFlag( OP_FLAGS_CALLBACK ) )
        {
            INI_GET_CALLBACK("SL", sl, atoi, syscfg.autoval[2].sl);
            INI_GET_CALLBACK("DSL", dsl, atoi, syscfg.autoval[2].dsl);
            INI_GET_CALLBACK("EXEMPT", exempt, atoi, 0);
            INI_GET_CALLBACK("AR", ar, str_to_arword, syscfg.autoval[2].ar);
            INI_GET_CALLBACK("DAR", dar, str_to_arword, syscfg.autoval[2].dar);
            INI_GET_CALLBACK("RESTRICT", restrict, str2restrict, 0);
            INI_GET_CALLBACK("FORCED", forced, stryn2tf, 0);
            INI_GET_CALLBACK("LONG_DISTANCE", longdistance, stryn2tf, 0);
            INI_GET_CALLBACK("REPEAT", repeat, atoi, 0);
        }


        // sysconfig flags
        syscfg.sysconfig = static_cast<unsigned short>( ini_flags( 'Y', get_key_str, sysconfig_flags,
            NEL( sysconfig_flags ), syscfg.sysconfig ) );

        // misc stuff
        if ( ( ( ss = ini_get( get_key_str( INI_STR_MAIL_WHO_LEN ), -1, NULL ) ) != NULL ) &&
            ( atoi(ss) > 0 || ss[0] == '0' ) )
        {
            sess->mail_who_field_len = atoi( ss );
        }
        if ((ss = ini_get(get_key_str(INI_STR_RATIO), -1, NULL)) != NULL)
        {
            syscfg.req_ratio = static_cast<float>( atof( ss ) );
        }

        if ( ( ss = ini_get( get_key_str( INI_STR_ATTACH_DIR ), -1, NULL ) ) != NULL )
        {
            strcpy( g_szAttachmentDirectory, ss );
            if ( g_szAttachmentDirectory[ strlen( g_szAttachmentDirectory ) - 1 ] != WWIV_FILE_SEPERATOR_CHAR )
            {
                strcat( g_szAttachmentDirectory, WWIV_FILE_SEPERATOR_STRING );
            }
        }
        else
        {
            sprintf( g_szAttachmentDirectory, "%s%s%c", GetHomeDir(), ATTACH_DIR, WWIV_FILE_SEPERATOR_CHAR );
        }

        INI_INIT_N( INI_STR_SCREEN_SAVER_TIME, screen_saver_time );

        // now done
        ini_done();
    }

	sess->max_extend_lines    = std::min<unsigned short>( sess->max_extend_lines, 99 );
    sess->max_batch           = std::min<unsigned short>( sess->max_batch , 999 );
    sess->max_chains          = std::min<unsigned short>( sess->max_chains, 999 );
    sess->max_gfilesec        = std::min<unsigned short>( sess->max_gfilesec, 999 );

    if ( omb )
	{
        sess->max_batch = omb;
	}
    if ( omc )
	{
        sess->max_chains = omc;
	}
    if ( omg )
	{
       sess->max_gfilesec = omg;
	}

    set_strings_fn( INI_STRFILE, NULL, NULL, 0 );

    // Success if here
    return true;
}


bool WBbsApp::ReadConfig()
{
    configrec full_syscfg;

    WFile configFile( CONFIG_DAT );
	int nFileMode = WFile::modeReadOnly | WFile::modeBinary;
    if ( !configFile.Open( nFileMode ) )
    {
        std::cout << CONFIG_DAT << " NOT FOUND.\r\n";
        return false;
    }
    configFile.Read( &full_syscfg, sizeof( configrec ) );
    configFile.Close();

    WFile configOvrFile( CONFIG_OVR );
    bool bIsConfigObvOpen = configOvrFile.Open( WFile::modeBinary | WFile::modeReadOnly );
    if (  bIsConfigObvOpen &&
         configOvrFile.GetLength() < static_cast<long>( GetInstanceNumber() * sizeof( configoverrec ) ) )
    {
        configOvrFile.Close();
        std::cout << "Not enough instances configured.\r\n";
        AbortBBS();
    }
    if ( !bIsConfigObvOpen )
    {
        // slap in the defaults, although this is not used anymore
        for ( int i = 0; i < 4; i++ )
        {
            syscfgovr.com_ISR[i + 1]	= full_syscfg.com_ISR[i + 1];
            syscfgovr.com_base[i + 1]	= full_syscfg.com_base[i + 1];
            syscfgovr.com_ISR[i + 5]	= full_syscfg.com_ISR[i + 1];
            syscfgovr.com_base[i + 5]	= full_syscfg.com_base[i + 1];
        }

        syscfgovr.primaryport = full_syscfg.primaryport;
        strcpy( syscfgovr.modem_type, full_syscfg.modem_type );
        strcpy( syscfgovr.tempdir, full_syscfg.tempdir );
        strcpy( syscfgovr.batchdir, full_syscfg.batchdir );
    }
    else
    {
        long lCurNodeOffset = ( GetInstanceNumber() - 1) * sizeof(configoverrec);
        configOvrFile.Seek( lCurNodeOffset, WFile::seekBegin );
        configOvrFile.Read( &syscfgovr, sizeof( configoverrec ) );
        configOvrFile.Close();
    }
    make_abs_path(full_syscfg.gfilesdir);
    make_abs_path(full_syscfg.datadir);
    make_abs_path(full_syscfg.msgsdir);
    make_abs_path(full_syscfg.dloadsdir);
    make_abs_path(full_syscfg.menudir);

    make_abs_path(syscfgovr.tempdir);
    strncpy(full_syscfg.tempdir, syscfgovr.tempdir, sizeof(full_syscfg.tempdir));

    make_abs_path(syscfgovr.batchdir);
    strncpy(full_syscfg.batchdir, syscfgovr.batchdir, sizeof(full_syscfg.batchdir));


    // update user info data
    int userreclen = sizeof(userrec);
    int waitingoffset = OFFOF(waiting);
    int inactoffset = OFFOF(inact);
    int sysstatusoffset = OFFOF(sysstatus);
    int fuoffset = OFFOF(forwardusr);
    int fsoffset = OFFOF(forwardsys);
    int fnoffset = OFFOF(net_num);

    if ( userreclen != full_syscfg.userreclen       ||
         waitingoffset != full_syscfg.waitingoffset    ||
         inactoffset != full_syscfg.inactoffset      ||
         sysstatusoffset != full_syscfg.sysstatusoffset  ||
         fuoffset != full_syscfg.fuoffset         ||
         fsoffset != full_syscfg.fsoffset         ||
         fnoffset != full_syscfg.fnoffset )
    {
        full_syscfg.userreclen      = userreclen;
        full_syscfg.waitingoffset   = waitingoffset;
        full_syscfg.inactoffset     = inactoffset;
        full_syscfg.sysstatusoffset = sysstatusoffset;
        full_syscfg.fuoffset        = fuoffset;
        full_syscfg.fsoffset        = fsoffset;
        full_syscfg.fnoffset        = fnoffset;
    }

    WFile configFile2( CONFIG_DAT );
    if ( configFile2.Open( WFile::modeReadWrite | WFile::modeBinary ) )
    {
        configFile2.Write( &full_syscfg, sizeof( configrec ) );
        configFile2.Close();
    }
    WFile ovrFile2( CONFIG_OVR );
    if ( !ovrFile2.Open( WFile::modeBinary | WFile::modeReadWrite ) ||
          ovrFile2.GetLength() < static_cast<long>( GetInstanceNumber() * sizeof( configoverrec ) ) )
    {
        ovrFile2.Close();
    }
    else
    {
        ovrFile2.Seek( ( GetInstanceNumber() - 1 ) * sizeof( configoverrec ), WFile::seekBegin );
        ovrFile2.Write( &syscfgovr, sizeof( configoverrec ) );
        ovrFile2.Close();
    }

    syscfg.newuserpw = strdup(full_syscfg.newuserpw);
    syscfg.systempw = strdup(full_syscfg.systempw);

    syscfg.msgsdir = strdup(full_syscfg.msgsdir);
    syscfg.gfilesdir = strdup(full_syscfg.gfilesdir);
    syscfg.datadir = strdup(full_syscfg.datadir);
    syscfg.dloadsdir = strdup(full_syscfg.dloadsdir);
    syscfg.batchdir = strdup(full_syscfg.batchdir);
    syscfg.menudir = strdup(full_syscfg.menudir);
    syscfg.terminal = strdup(full_syscfg.terminal);

    syscfg.systemname = strdup(full_syscfg.systemname);
    syscfg.systemphone = strdup(full_syscfg.systemphone);
    syscfg.sysopname = strdup(full_syscfg.sysopname);
    syscfg.executestr = strdup(full_syscfg.executestr);

    syscfg.beginday_c = strdup(full_syscfg.beginday_c);
    syscfg.logon_c = strdup(full_syscfg.logon_c);
    syscfg.logoff_c = strdup(full_syscfg.logoff_c);
    syscfg.newuser_c = strdup(full_syscfg.newuser_c);
    syscfg.upload_c = strdup(full_syscfg.upload_c);
    syscfg.v_scan_c = strdup(full_syscfg.v_scan_c);
    syscfg.dszbatchdl = strdup(full_syscfg.dszbatchdl);
    syscfg.dial_prefix = strdup(full_syscfg.dial_prefix);

    syscfg.newusersl = full_syscfg.newusersl;
    syscfg.newuserdsl = full_syscfg.newuserdsl;
    syscfg.maxwaiting = full_syscfg.maxwaiting;
    syscfg.newuploads = full_syscfg.newuploads;
    syscfg.closedsystem = full_syscfg.closedsystem;

    syscfg.systemnumber = full_syscfg.systemnumber;
    syscfg.maxusers = full_syscfg.maxusers;
    syscfg.newuser_restrict = full_syscfg.newuser_restrict;
    syscfg.sysconfig = full_syscfg.sysconfig;
    syscfg.sysoplowtime = full_syscfg.sysoplowtime;
    syscfg.sysophightime = full_syscfg.sysophightime;
    syscfg.executetime = full_syscfg.executetime;
    syscfg.netlowtime = full_syscfg.netlowtime;
    syscfg.nethightime = full_syscfg.nethightime;
    syscfg.max_subs = full_syscfg.max_subs;
    syscfg.max_dirs = full_syscfg.max_dirs;
    syscfg.qscn_len = full_syscfg.qscn_len;
    syscfg.userreclen = full_syscfg.userreclen;

    syscfg.post_call_ratio = full_syscfg.post_call_ratio;
    syscfg.req_ratio = full_syscfg.req_ratio;
    syscfg.newusergold = full_syscfg.newusergold;

    syscfg.autoval[0] = full_syscfg.autoval[0];
    syscfg.autoval[1] = full_syscfg.autoval[1];
    syscfg.autoval[2] = full_syscfg.autoval[2];
    syscfg.autoval[3] = full_syscfg.autoval[3];
    syscfg.autoval[4] = full_syscfg.autoval[4];
    syscfg.autoval[5] = full_syscfg.autoval[5];
    syscfg.autoval[6] = full_syscfg.autoval[6];
    syscfg.autoval[7] = full_syscfg.autoval[7];
    syscfg.autoval[8] = full_syscfg.autoval[8];
    syscfg.autoval[9] = full_syscfg.autoval[9];

    syscfg.wwiv_reg_number = full_syscfg.wwiv_reg_number;
    syscfg.regcode = full_syscfg.regcode;
    syscfg.sysconfig1 = full_syscfg.sysconfig1;
    syscfg.rrd = full_syscfg.rrd;

    return true;
}


bool WBbsApp::SaveConfig()
{
    WFile configFile( CONFIG_DAT );
    if ( configFile.Open( WFile::modeBinary | WFile::modeReadWrite ) )
    {
        configrec full_syscfg;
        configFile.Read( &full_syscfg, sizeof( configrec ) );
        strcpy(full_syscfg.newuserpw, syscfg.newuserpw);
        strcpy(full_syscfg.systempw, syscfg.systempw);

        strcpy(full_syscfg.msgsdir, syscfg.msgsdir);
        strcpy(full_syscfg.gfilesdir, syscfg.gfilesdir);
        strcpy(full_syscfg.datadir, syscfg.datadir);
        strcpy(full_syscfg.dloadsdir, syscfg.dloadsdir);
        strcpy(full_syscfg.batchdir, syscfg.batchdir);
        strcpy(full_syscfg.menudir, syscfg.menudir);
        strcpy(full_syscfg.terminal, syscfg.terminal);

        strcpy(full_syscfg.systemname, syscfg.systemname);
        strcpy(full_syscfg.systemphone, syscfg.systemphone);
        strcpy(full_syscfg.sysopname, syscfg.sysopname);
        strcpy(full_syscfg.executestr, syscfg.executestr);

        strcpy(full_syscfg.beginday_c, syscfg.beginday_c);
        strcpy(full_syscfg.logon_c, syscfg.logon_c);
        strcpy(full_syscfg.logoff_c, syscfg.logoff_c);
        strcpy(full_syscfg.newuser_c, syscfg.newuser_c);
        strcpy(full_syscfg.upload_c, syscfg.upload_c);
        strcpy(full_syscfg.v_scan_c, syscfg.v_scan_c);
        strcpy(full_syscfg.dszbatchdl, syscfg.dszbatchdl);
        strcpy(full_syscfg.dial_prefix, syscfg.dial_prefix);

        full_syscfg.newusersl = syscfg.newusersl;
        full_syscfg.newuserdsl = syscfg.newuserdsl;
        full_syscfg.maxwaiting = syscfg.maxwaiting;
        full_syscfg.newuploads = syscfg.newuploads;
        full_syscfg.closedsystem = syscfg.closedsystem;

        full_syscfg.systemnumber = syscfg.systemnumber;
        full_syscfg.maxusers = syscfg.maxusers;
        full_syscfg.newuser_restrict = syscfg.newuser_restrict;
        full_syscfg.sysconfig = syscfg.sysconfig;
        full_syscfg.sysoplowtime = syscfg.sysoplowtime;
        full_syscfg.sysophightime = syscfg.sysophightime;
        full_syscfg.executetime = syscfg.executetime;
        full_syscfg.netlowtime = syscfg.netlowtime;
        full_syscfg.nethightime = syscfg.nethightime;
        full_syscfg.max_subs = syscfg.max_subs;
        full_syscfg.max_dirs = syscfg.max_dirs;
        full_syscfg.qscn_len = syscfg.qscn_len;
        full_syscfg.userreclen = syscfg.userreclen;

        full_syscfg.post_call_ratio = syscfg.post_call_ratio;
        full_syscfg.req_ratio = syscfg.req_ratio;
        full_syscfg.newusergold = syscfg.newusergold;

        full_syscfg.autoval[0] = syscfg.autoval[0];
        full_syscfg.autoval[1] = syscfg.autoval[1];
        full_syscfg.autoval[2] = syscfg.autoval[2];
        full_syscfg.autoval[3] = syscfg.autoval[3];
        full_syscfg.autoval[4] = syscfg.autoval[4];
        full_syscfg.autoval[5] = syscfg.autoval[5];
        full_syscfg.autoval[6] = syscfg.autoval[6];
        full_syscfg.autoval[7] = syscfg.autoval[7];
        full_syscfg.autoval[8] = syscfg.autoval[8];
        full_syscfg.autoval[9] = syscfg.autoval[9];

        for (int i = 0; i < 4; i++)
        {
            strcpy(full_syscfg.arcs[i].extension, arcs[i].extension);
            strcpy(full_syscfg.arcs[i].arca, arcs[i].arca);
            strcpy(full_syscfg.arcs[i].arce, arcs[i].arce);
            strcpy(full_syscfg.arcs[i].arcl, arcs[i].arcl);
        }

        full_syscfg.wwiv_reg_number = syscfg.wwiv_reg_number;
        full_syscfg.sysconfig1 = syscfg.sysconfig1;
        full_syscfg.rrd = syscfg.rrd;

        configFile.Seek( 0, WFile::seekBegin );
        configFile.Write( &full_syscfg, sizeof( configrec ) );
        return false;
    }
    return true;
}


void WBbsApp::read_nextern()
{
    sess->SetNumberOfExternalProtocols( 0 );
    if ( externs )
    {
        BbsFreeMemory(externs);
        externs = NULL;
    }

    WFile externalFile( syscfg.datadir, NEXTERN_DAT );
    if ( externalFile.Open( WFile::modeBinary | WFile::modeReadOnly ) )
    {
        unsigned long l = externalFile.GetLength();
        if (l > 15 * sizeof(newexternalrec))
        {
            l = 15 * sizeof(newexternalrec);
        }
        externs = static_cast<newexternalrec *>( BbsAllocWithComment(l + 10, "external protocols") );
		WWIV_ASSERT(externs != NULL);
        sess->SetNumberOfExternalProtocols( externalFile.Read( externs, l ) / sizeof( newexternalrec ) );
    }
}


void WBbsApp::read_arcs()
{
    if ( arcs )
    {
        BbsFreeMemory( arcs );
        arcs = NULL;
    }

    WFile archiverFile( syscfg.datadir, ARCHIVER_DAT );
    if ( archiverFile.Open( WFile::modeBinary | WFile::modeReadOnly ) )
    {
        unsigned long l = archiverFile.GetLength();
        if ( l > MAX_ARCS * sizeof( arcrec ) )
        {
            l = MAX_ARCS * sizeof( arcrec );
        }
        arcs = static_cast<arcrec *>( BbsAllocWithComment( l, "archivers" ) );
		WWIV_ASSERT( arcs != NULL );
    }
}


void WBbsApp::read_editors()
{
    if ( editors )
    {
        BbsFreeMemory( editors );
        editors = NULL;
    }
    sess->SetNumberOfEditors( 0 );

    WFile file( syscfg.datadir, EDITORS_DAT );
    if ( file.Open( WFile::modeBinary | WFile::modeReadOnly ) )
    {
        unsigned long l = file.GetLength();
        if (l > 10 * sizeof(editorrec))
        {
            l = 10 * sizeof(editorrec);
        }
        editors = static_cast<editorrec *>( BbsAllocWithComment(l + 10, "external editors") );
		WWIV_ASSERT( editors != NULL );
        sess->SetNumberOfEditors( ( file.Read( editors, l ) ) / sizeof( editorrec ) );
    }
}


void WBbsApp::read_nintern()
{
    if (over_intern)
    {
        BbsFreeMemory(over_intern);
        over_intern = NULL;
    }
    WFile file( syscfg.datadir, NINTERN_DAT );
    if ( file.Open( WFile::modeBinary | WFile::modeReadOnly ) )
    {
        over_intern = static_cast<newexternalrec *>( BbsAllocWithComment( 3 * sizeof( newexternalrec ), "internal protocol overrides" ) );
		WWIV_ASSERT(over_intern != NULL);

        file.Read( over_intern, 3 * sizeof( newexternalrec ) );
    }
}


bool WBbsApp::read_subs()
{
    if ( subboards )
    {
        BbsFreeMemory( subboards );
    }
    subboards = NULL;
    sess->SetMaxNumberMessageAreas( syscfg.max_subs );
    subboards = static_cast< subboardrec * >( BbsAllocWithComment( sess->GetMaxNumberMessageAreas() * sizeof( subboardrec ), "subboards" ) );
	WWIV_ASSERT( subboards != NULL );

    WFile file( syscfg.datadir, SUBS_DAT );
    if ( !file.Open( WFile::modeBinary | WFile::modeReadOnly ) )
    {
        std::cout << file.GetName() << " NOT FOUND.\r\n";
        return false;
    }
    sess->num_subs = ( file.Read( subboards, ( sess->GetMaxNumberMessageAreas() * sizeof( subboardrec ) ) ) ) / sizeof( subboardrec );
    return ( read_subs_xtr( sess->GetMaxNumberMessageAreas(), sess->num_subs, subboards ) );
}


void WBbsApp::read_networks()
{
    sess->internetEmailName = "";
    sess->internetEmailDomain = "";
    sess->internetPopDomain = "";
    sess->SetInternetUseRealNames( false );

    FILE *fp = fsh_open( "NET.INI", "rt" );
    if ( fp )
    {
        while ( !feof( fp ) )
        {
            char szBuffer[ 255 ];
            fgets(szBuffer, 80, fp);
            szBuffer[strlen(szBuffer) - 1] = 0;
            StringRemoveWhitespace(szBuffer);
            if ( !strnicmp( szBuffer, "DOMAIN=", 7 ) && sess->internetEmailDomain.empty() )
            {
                sess->internetEmailDomain = &(szBuffer[7]);
            }
            else if ((!strnicmp(szBuffer, "POPNAME=", 8)) && sess->internetEmailName.empty() )
            {
                sess->internetEmailName = &( szBuffer[8] );
            }
            else if (!strnicmp(szBuffer, "FWDDOM=", 7))
            {
                sess->internetEmailDomain = &(szBuffer[7]);
            }
            else if (!strnicmp(szBuffer, "FWDNAME=", 8))
            {
                sess->internetEmailName = &(szBuffer[8]);
            }
            else if (!strnicmp(szBuffer, "POPDOMAIN=", 10))
            {
                sess->internetPopDomain = &( szBuffer[10] );
            }
            else if ((!strnicmp(szBuffer, "REALNAME=", 9)) &&
                ((szBuffer[9] == 'Y') || (szBuffer[9] == 'y')))
            {
                sess->SetInternetUseRealNames( true );
            }
        }
        fsh_close(fp);
    }
    if (net_networks)
    {
        BbsFreeMemory(net_networks);
    }
    net_networks = NULL;
    WFile file( syscfg.datadir, NETWORKS_DAT );
    if ( file.Open( WFile::modeBinary | WFile::modeReadOnly ) )
    {
        sess->SetMaxNetworkNumber( file.GetLength() / sizeof( net_networks_rec ) );
        if ( sess->GetMaxNetworkNumber() )
        {
            net_networks = static_cast<net_networks_rec *>( BbsAllocWithComment( sess->GetMaxNetworkNumber() * sizeof( net_networks_rec ), "networks.dat" ) );
			WWIV_ASSERT(net_networks != NULL);

            file.Read( net_networks, sess->GetMaxNetworkNumber() * sizeof( net_networks_rec ) );
        }
        file.Close();
        for (int i = 0; i < sess->GetMaxNetworkNumber(); i++)
        {
            char* ss = strchr(net_networks[i].name, ' ');
            if (ss)
            {
                *ss = 0;
            }
        }
    }
    if ( !net_networks )
    {
        net_networks = static_cast<net_networks_rec *>( BbsAllocWithComment( sizeof( net_networks_rec ), "networks.dat" ) );
		WWIV_ASSERT(net_networks != NULL);
        sess->SetMaxNetworkNumber( 1 );
        strcpy(net_networks->name, "WWIVnet");
        strcpy(net_networks->dir, syscfg.datadir);
        net_networks->sysnum = syscfg.systemnumber;
    }
}


bool WBbsApp::read_names()
{
    if (smallist)
    {
        BbsFreeMemory((void *) smallist);
    }
    smallist = NULL;
    smallist = static_cast< smalrec * >( BbsAllocWithComment( static_cast<long>( syscfg.maxusers ) * static_cast<long>( sizeof( smalrec ) ),
        "names.lst - try decreasing max users in INIT" ) );
	WWIV_ASSERT(smallist != NULL);

    WFile file( syscfg.datadir, NAMES_LST );
    if ( !file.Open( WFile::modeBinary | WFile::modeReadOnly ) )
    {
        std::cout << file.GetName() << " NOT FOUND.\r\n";
        return false;
    }
    file.Read( smallist, ( sizeof( smalrec ) * status.users ) );
    file.Close();
    app->statusMgr->Read();
    return true;
}


void WBbsApp::read_voting()
{
    for ( int i = 0; i < 20; i++ )
    {
        questused[i] = 0;
    }

    WFile file( syscfg.datadir, VOTING_DAT );
    if ( file.Open( WFile::modeBinary | WFile::modeReadOnly ) )
    {
        int n = static_cast<int>( file.GetLength() / sizeof( votingrec ) ) - 1;
        for (int i = 0; i < n; i++)
        {
            votingrec v;
            file.Seek( static_cast<long>( i ) * sizeof(votingrec), WFile::seekBegin );
            file.Read( &v, sizeof( votingrec ) );
            if (v.numanswers)
            {
                questused[i] = 1;
            }
        }
    }
}


bool WBbsApp::read_dirs()
{
    if ( directories )
    {
        BbsFreeMemory( directories );
    }
    directories = NULL;
    sess->SetMaxNumberFileAreas( syscfg.max_dirs );
    directories = static_cast<directoryrec *>( BbsAllocWithComment(static_cast<long>(sess->GetMaxNumberFileAreas()) * static_cast<long>( sizeof( directoryrec ) ), "directories" ) );
	WWIV_ASSERT(directories != NULL);

    WFile file( syscfg.datadir, DIRS_DAT );
    if ( !file.Open( WFile::modeBinary | WFile::modeReadOnly ) )
    {
        std::cout << file.GetName() << "%s NOT FOUND.\r\n";
        return false;
    }
    sess->num_dirs = file.Read( directories, (sizeof(directoryrec) * sess->GetMaxNumberFileAreas()) ) / sizeof(directoryrec);
    return true;
}


void WBbsApp::read_chains()
{
    if ( chains )
    {
        BbsFreeMemory(chains);
    }
    chains = NULL;
    chains = static_cast<chainfilerec *>( BbsAllocWithComment( sess->max_chains * sizeof( chainfilerec ), "chains" ) );
	WWIV_ASSERT(chains != NULL);
    WFile file( syscfg.datadir, CHAINS_DAT );
    if ( file.Open( WFile::modeBinary | WFile::modeReadOnly ) )
    {
        sess->SetNumberOfChains( file.Read( chains, sess->max_chains * sizeof( chainfilerec ) ) / sizeof( chainfilerec ) );
    }
    file.Close();
    if ( app->HasConfigFlag( OP_FLAGS_CHAIN_REG ) )
    {
        if (chains_reg)
        {
            BbsFreeMemory(chains_reg);
        }
        chains_reg = NULL;
        chains_reg = static_cast<chainregrec *>( BbsAllocWithComment( sess->max_chains * sizeof( chainregrec ),
            "chain registration") );
		WWIV_ASSERT( chains_reg != NULL );

        WFile regFile( syscfg.datadir, CHAINS_REG );
        if ( regFile.Open( WFile::modeBinary | WFile::modeReadOnly ) )
        {
            regFile.Read( chains_reg, sess->max_chains * sizeof( chainregrec ) );
        }
        else
        {
            for ( int i = 0; i < sess->GetNumberOfChains(); i++ )
            {
                for ( unsigned int i1 = 0; i1 < sizeof( chains_reg[i].regby ) / sizeof( chains_reg[i].regby[0] ); i1++ )
                {
                    chains_reg[i].regby[i1] = 0;
                }
                chains_reg[i].usage     = 0;
                chains_reg[i].minage    = 0;
                chains_reg[i].maxage    = 255;
            }
            regFile.Open( WFile::modeReadWrite | WFile::modeBinary | WFile::modeCreateFile, WFile::shareUnknown, WFile::permReadWrite );
            regFile.Write( chains_reg , sizeof( chainregrec ) * sess->GetNumberOfChains() );
        }
        regFile.Close();
    }
}


bool WBbsApp::read_language()
{
    if (languages)
    {
        BbsFreeMemory(languages);
    }
    languages = NULL;
    WFile file( syscfg.datadir, LANGUAGE_DAT );
    if ( file.Open( WFile::modeBinary | WFile::modeReadOnly ) )
    {
        sess->num_languages = file.GetLength() / sizeof(languagerec);
        if (sess->num_languages)
        {
            languages = static_cast<languagerec *>( BbsAllocWithComment(sess->num_languages * sizeof(languagerec), "language.dat") );
			WWIV_ASSERT(languages != NULL);

            file.Read( languages, sess->num_languages * sizeof( languagerec ) );
        }
        file.Close();
    }
    if (!sess->num_languages)
    {
        languages = static_cast<languagerec *>( BbsAllocWithComment(sizeof(languagerec), "language.dat") );
		WWIV_ASSERT(languages != NULL);
        sess->num_languages = 1;
        strcpy(languages->name, "English");
        strncpy(languages->dir, syscfg.gfilesdir, sizeof(languages->dir) - 1);
        strncpy(languages->mdir, syscfg.menudir, sizeof(languages->mdir) - 1);
    }
    sess->SetCurrentLanguageNumber( -1 );
    if ( !set_language( 0 ) )
    {
        std::cout << "You need the default language installed to run the BBS.\r\n";
        return false;
    }
    return true;
}


bool WBbsApp::read_modem()
{
    unsigned long l;

    if (modem_i)
    {
        BbsFreeMemory(modem_i);
    }
    modem_i = NULL;
    if (!ok_modem_stuff)
    {
        return true;
    }
    char szFileName[ MAX_PATH ];
    if (app->GetInstanceNumber() > 1)
    {
        sprintf( szFileName, "%s%s.%3.3d", syscfg.datadir, MODEM_XXX, app->GetInstanceNumber() );
    }
    else
    {
        sprintf( szFileName, "%s%s", syscfg.datadir, MODEM_DAT );
    }
    WFile file( szFileName );
    if ( file.Open( WFile::modeBinary | WFile::modeReadOnly ) )
    {
        l = file.GetLength();
        modem_i = static_cast<modem_info *>( BbsAllocWithComment(l, "modem.dat") );
		WWIV_ASSERT(modem_i != NULL);
        file.Read( modem_i, l );
        return true;
    }
    else
    {
        std::cout << "\r\nRun INIT.EXE to convert modem data.\r\n\n";
        return false;
    }
}


void WBbsApp::read_gfile()
{
    if (gfilesec != NULL)
    {
        BbsFreeMemory(gfilesec);
		gfilesec = NULL;
    }
    gfilesec = static_cast<gfiledirrec *>( BbsAllocWithComment(static_cast<long>(sess->max_gfilesec * sizeof(gfiledirrec)), "gfiles") );
	WWIV_ASSERT(gfilesec != NULL);
    WFile file( syscfg.datadir, GFILE_DAT );
    if ( !file.Open( WFile::modeBinary | WFile::modeReadOnly ) )
    {
        sess->num_sec = 0;
    }
    else
    {
        sess->num_sec = file.Read( gfilesec, sess->max_gfilesec * sizeof( gfiledirrec ) ) / sizeof(gfiledirrec);
    }
}


/**
 * Makes a path into an absolute path, returns true if original path altered,
 * else returns false
 */
bool WBbsApp::make_abs_path( char *pszDirectory )
{
    char szOldDirectory[ MAX_PATH ];

#ifdef _UNIX
    if ( strlen( pszDirectory ) < 1 )
#else
    if ( strlen( pszDirectory ) < 3 || pszDirectory[1] != ':' || pszDirectory[2] != WWIV_FILE_SEPERATOR_CHAR )
#endif
    {
        WWIV_GetDir( szOldDirectory, true );
        app->CdHome();
        WWIV_ChangeDirTo( pszDirectory );
        WWIV_GetDir( pszDirectory, true );
        WWIV_ChangeDirTo( szOldDirectory );
        return true;
    }
    return false;
}


void WBbsApp::InitializeBBS()
{
    char *ss, szFileName[MAX_PATH], newprompt[ 255 ];
    int i1 = 0;

	sess->screenbottom = defscreenbottom = localIO->GetDefaultScreenBottom();

	localIO->LocalCls();
#if !defined( _UNIX )
    std::cout << std::endl << wwiv_version << beta_version << ", Copyright (c) 1998-2004, WWIV Software Services.\r\n\n";
	std::cout << "\r\nInitializing BBS...\r\n";
#endif // _UNIX
    sess->SetCurrentReadMessageArea( -1 );
    use_workspace = false;
    chat_file = false;
    localIO->SetSysopAlert( false );
    nsp = 0;
    localIO->set_global_handle( false, true );
    bquote = 0;
    equote = 0;
    sess->SetQuoting( false );
    sess->tagptr = 0;

    sprintf( m_szWWIVEnvironmentVariable, "BBS=%s", wwiv_version );

    time_t t;
    time(&t);
    // Struct tm_year is -= 1900
    struct tm * pTm = localtime(&t);
    if (pTm->tm_year < 88)
    {
        std::cout << "You need to set the date [" << pTm->tm_year << "] & time before running the BBS.\r\n";
        AbortBBS();
    }
    if (!ReadConfig())
    {
        AbortBBS( true );
    }

    strcpy(szFileName, syscfgovr.tempdir);
    int i = strlen(szFileName);
    if (szFileName[0] == 0)
    {
        i1 = 1;
    }
    else
    {
        if (szFileName[i - 1] == WWIV_FILE_SEPERATOR_CHAR)
        {
            szFileName[i - 1] = 0;
        }
        i1 = chdir(szFileName);
    }
    if (i1)
    {
        std::cout << "\r\nYour temp dir isn't valid.\r\n";
        std::cout << "It is now set to: '%" << syscfgovr.tempdir << "'\r\n\n";
        AbortBBS();
    }
    else
    {
        CdHome();
    }

    strcpy(szFileName, syscfgovr.batchdir);
    i = strlen(szFileName);
    if (szFileName[0] == 0)
    {
        i1 = 1;
    }
    else
    {
        if (szFileName[i - 1] == WWIV_FILE_SEPERATOR_CHAR)
        {
            szFileName[i - 1] = 0;
        }
        i1 = chdir(szFileName);
    }
    if (i1)
    {
        std::cout << "\r\nYour batch dir isn't valid.\r\n";
        std::cout << "It is now set to: '" << syscfgovr.tempdir << "'\r\n\n";
        AbortBBS();
    }
    else
    {
        CdHome();
    }

    write_inst(INST_LOC_INIT, 0, INST_FLAGS_NONE);

    // make sure it is the new USERREC structure
    XINIT_PRINTF("* Reading user scan pointers.\r\n");
    sprintf(szFileName, "%s%s", syscfg.datadir, USER_QSC);
    if (!WFile::Exists(szFileName))
    {
     	std::cout << "Could not open file '" << szFileName << "'\r\n";
        std::cout << "You must go into INIT and convert your userlist before running the BBS.\r\n";
        AbortBBS();
    }

#if !defined( _UNIX )
    if (!syscfgovr.primaryport)
    {
        ok_modem_stuff = false;
    }
#endif // _UNIX

    languages = NULL;
    if (!read_language())
    {
        AbortBBS();
    }

    XINIT_PRINTF("* Processing configuration file: WWIV.INI.\r\n");
    if (!ReadINIFile())
    {
        std::cout << "Insufficient memory for system info structure.\r\n";
        AbortBBS();
    }

    net_networks = NULL;
    sess->SetNetworkNumber( 0 );
    read_networks();
    set_net_num( 0 );

    XINIT_PRINTF("* Reading status information.\r\n");
    statusMgr->Get(false, true);

    XINIT_PRINTF("* Reading color information.\r\n");
    sprintf(szFileName, "%s%s", syscfg.datadir, COLOR_DAT);
    if ( !WFile::Exists( szFileName ) )
	{
        buildcolorfile();
	}
    get_colordata();

    status.wwiv_version = wwiv_num_version;
    if (status.callernum != 65535)
    {
        status.callernum1 = static_cast<unsigned long>( status.callernum );
        status.callernum = 65535;
    }
    statusMgr->Write();

    gat = static_cast<unsigned short *>( BbsAllocWithComment(2048 * sizeof(short), "gat") );
	WWIV_ASSERT(gat != NULL);

    XINIT_PRINTF("* Reading Gfiles.\r\n");
    gfilesec = NULL;
    read_gfile();

    smallist = NULL;

    XINIT_PRINTF("* Reading user names.\r\n");
    if (!read_names())
    {
        AbortBBS();
    }

    XINIT_PRINTF("* Reading Message Areas.\r\n");
    subboards = NULL;
    if (!read_subs())
    {
        AbortBBS();
    }

    XINIT_PRINTF("* Reading File Areas.\r\n");
    directories = NULL;
    if (!read_dirs())
    {
        AbortBBS();
    }

    XINIT_PRINTF("* Reading Chains.\r\n");
    sess->SetNumberOfChains( 0 );
    chains = NULL;
    read_chains();

#ifndef _UNIX
    XINIT_PRINTF("* Reading Modem Configuration.\r\n");
    modem_i = NULL;
    if (!read_modem())
    {
        AbortBBS();
    }
#endif
    XINIT_PRINTF("* Reading File Transfer Protocols.\r\n");
    externs = NULL;
    read_nextern();

    over_intern = NULL;
    read_nintern();

    XINIT_PRINTF("* Reading File Archivers.\r\n");
    read_arcs();
    SaveConfig();

    XINIT_PRINTF("* Reading Full Screen Message Editors.\r\n");
    read_editors();

    XINIT_PRINTF("* Parsing WWIV.INI.\r\n");
    if (ini_init(WWIV_INI, INI_TAG, NULL))
    {
        if ((ss = ini_get("THREAD_SUBS", -1, NULL)) != NULL)
        {
            if (wwiv::UpperCase<char>(ss[0] == 'Y'))
            {
                sess->SetMessageThreadingEnabled( true );
            }
        }
    }

    if (ini_init(WWIV_INI, INI_TAG, NULL))
    {
        if ((ss = ini_get("ALLOW_CC_BCC", -1, NULL)) != NULL)
        {
            if (wwiv::UpperCase<char>(ss[0]) == 'Y')
            {
                sess->SetCarbonCopyEnabled( true );
            }
        }
    }
    ini_done();
    strcpy(szFileName, g_szAttachmentDirectory);
    if (WWIV_make_path(szFileName))
    {
        std::cout << "\r\nYour file attachment directory is invalid.\r\n";
        std::cout << "It is now set to: " << g_szAttachmentDirectory << "'\r\n\n";
        AbortBBS();
    }
    CdHome();

    check_phonenum();                         // dupphone addition

    // allocate sub cache
    iscan1(-1, false);

    batch = static_cast<batchrec *>( BbsAllocWithComment(sess->max_batch * sizeof(batchrec), "batch list") );
	WWIV_ASSERT(batch != NULL);

    XINIT_PRINTF("* Reading User Information.\r\n");
    sess->ReadCurrentUser( 1, false );
    fwaiting = ( sess->thisuser.isUserDeleted() ) ? 0 : sess->thisuser.GetNumMailWaiting();

    statusMgr->Read();

    if (syscfg.sysconfig & sysconfig_no_local)
    {
        sess->topdata = WLocalIO::topdataNone;
    }
    else
    {
        sess->topdata = WLocalIO::topdataUser;
    }
    ss = getenv("PROMPT");
    strcpy(newprompt, "PROMPT=WWIV: ");

    if (ss)
    {
        strcat(newprompt, ss);
    }
    else
    {
        strcat(newprompt, "$P$G");
    }
    // put in our environment since passing the xenviron wasn't working
    // with sync emulated fossil
    putenv( newprompt );
    sprintf(g_szDSZLogFileName, "%sWWIVDSZ.%3.3u", GetHomeDir(), GetInstanceNumber());

#if !defined (_UNIX)
    sprintf(szFileName, "DSZLOG=%s", g_szDSZLogFileName);
    int pk = i = i1 = 0;
    ss = getenv("DSZLOG");

    if (!ss)
    {
        putenv( szFileName );
    }
    if (!pk)
    {
        putenv( "PKNOFASTCHAR=Y" );
    }

#endif // defined (_UNIX)

    putenv( m_szWWIVEnvironmentVariable );
    putenv( m_szEnvironVarWwivNetworkNumber );

    XINIT_PRINTF("* Reading Voting Booth Configuration.\r\n");
    read_voting();

    XINIT_PRINTF("* Reading External Events.\r\n");
    init_events();
    last_time = time_event - timer();
    if ( last_time < 0.0 )
    {
        last_time += HOURS_PER_DAY_FLOAT * SECONDS_PER_HOUR_FLOAT;
    }

    XINIT_PRINTF("* Allocating Memory for Message/File Areas.\r\n");
    do_event = 0;
    usub = static_cast<usersubrec *>( BbsAllocWithComment( sess->GetMaxNumberMessageAreas() * sizeof( usersubrec ), "usub" ) );
	WWIV_ASSERT(usub != NULL);
    sess->m_SubDateCache = static_cast<UINT32 *>( BbsAllocWithComment(sess->GetMaxNumberMessageAreas() * sizeof(long), "sess->m_SubDateCache") );
	WWIV_ASSERT(sess->m_SubDateCache != NULL);

    udir = static_cast<usersubrec *>( BbsAllocWithComment(sess->GetMaxNumberFileAreas() * sizeof(usersubrec), "udir") );
	WWIV_ASSERT(udir != NULL);
    sess->m_DirectoryDateCache = static_cast<UINT32 *>( BbsAllocWithComment(sess->GetMaxNumberFileAreas() * sizeof(long), "sess->m_DirectoryDateCache") );
	WWIV_ASSERT(sess->m_DirectoryDateCache != NULL);

    uconfsub = static_cast<userconfrec *>( BbsAllocWithComment(MAX_CONFERENCES * sizeof(userconfrec), "uconfsub") );
	WWIV_ASSERT(uconfsub != NULL);
    uconfdir = static_cast<userconfrec *>( BbsAllocWithComment(MAX_CONFERENCES * sizeof(userconfrec), "uconfdir") );
	WWIV_ASSERT(uconfdir != NULL);

    qsc = static_cast<unsigned long *>( BbsAllocWithComment(syscfg.qscn_len, "quickscan") );
	WWIV_ASSERT(qsc != NULL);
    qsc_n = qsc + 1;
    qsc_q = qsc_n + (sess->GetMaxNumberFileAreas() + 31) / 32;
    qsc_p = qsc_q + (sess->GetMaxNumberMessageAreas() + 31) / 32;

    ss = getenv("WWIV_INSTANCE");
    strcpy( m_szNetworkExtension, ".NET" );
    if (ss)
    {
        i = atoi(ss);
        if (i > 0)
        {
            sprintf( m_szNetworkExtension, ".%3.3d", i );
            // Fix... Set the global instance variable to match this.  When you run WWIV with the -n<instance> parameter
            // it sets the WWIV_INSTANCE environment variable, however it wasn't doing the reverse.
            m_nInstance = i;
        }
    }

    read_bbs_list_index();
    frequent_init();
    if (!m_bUserAlreadyOn)
    {
        tmp_disable_pause( true );
        remove_from_temp( "*.*", syscfgovr.tempdir, true );
        remove_from_temp( "*.*", syscfgovr.batchdir, true );
        tmp_disable_pause( false );
        imodem( true );
        cleanup_net();
    }
    find_devices();

    subconfnum = dirconfnum = 0;

	XINIT_PRINTF("* Reading Conferences.\r\n");
    read_all_conferences();

    if (!m_bUserAlreadyOn)
    {
		sysoplog("", false);
        sysoplogf( "WWIV %s, inst %ld, brought up at %s on %s.", wwiv_version, GetInstanceNumber(), times(), fulldate() );
		sysoplog("", false);
    }
    if (GetInstanceNumber() > 1)
	{
        sprintf(szFileName, "%s.%3.3u", WWIV_NET_NOEXT, GetInstanceNumber());
	}
	else
	{
        strcpy(szFileName, WWIV_NET_DAT);
	}
    WFile::Remove(szFileName);

    srand(time(NULL));

    catsl();

    XINIT_PRINTF("* Saving Instance information.\r\n");
    write_inst(INST_LOC_WFC, 0, INST_FLAGS_NONE);
}



// begin dupphone additions

void WBbsApp::check_phonenum()
{
    WFile phoneFile( syscfg.datadir, PHONENUM_DAT );
    if ( !phoneFile.Exists() )
    {
        create_phone_file();
    }
}


void WBbsApp::create_phone_file()
{
    phonerec p;

    WFile file( syscfg.datadir, USER_LST );
    if ( !file.Open( WFile::modeReadOnly | WFile::modeBinary ) )
    {
        return;
    }
    long l = file.GetLength();
    file.Close();
    int num = static_cast<int>( l / sizeof( userrec ) );

    WFile phoneNumFile( syscfg.datadir, PHONENUM_DAT );
    if ( !phoneNumFile.Open( WFile::modeReadWrite | WFile::modeAppend | WFile::modeBinary | WFile::modeCreateFile,
        WFile::shareUnknown, WFile::permReadWrite ) )
    {
        return;
    }

    for (int i = 1; i <= num; i++)
    {
        WUser user;
        app->userManager->ReadUser( &user, i );
        if ( !user.isUserDeleted() )
        {
            p.usernum = i;
            char szTempVoiceNumber[ 255 ], szTempDataNumber[ 255 ];
            strcpy( szTempVoiceNumber, user.GetVoicePhoneNumber() );
            strcpy( szTempDataNumber, user.GetDataPhoneNumber() );
            if ( szTempVoiceNumber[0] && !strstr( szTempVoiceNumber, "000-" ) )
            {
                strcpy( reinterpret_cast<char*>( p.phone ), szTempVoiceNumber );
                phoneNumFile.Write( &p, sizeof( phonerec ) );
            }
            if ( szTempDataNumber[0] &&
                 !wwiv::stringUtils::IsEquals( szTempVoiceNumber, szTempDataNumber ) &&
                 !strstr( szTempVoiceNumber, "000-" ) )
            {
                strcpy( reinterpret_cast<char*>( p.phone ), szTempDataNumber );
                phoneNumFile.Write( &p, sizeof( phonerec ) );
            }
        }
    }
    phoneNumFile.Close();
}


#if defined( _MSC_VER )
#pragma warning( pop )
#endif // _MSC_VER

// end dupphone additions
