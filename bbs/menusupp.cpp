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


void UnQScan()
{
    nl();
    sess->bout << "|#9Mark messages as unread on [C]urrent sub or [A]ll subs (A/C/Q)? ";
    char ch = onek( "QAC\r" );
    switch ( ch )
    {
    case 'Q':
    case RETURN:
        break;
    case 'A':
        {
            for ( int i = 0; i < sess->GetMaxNumberMessageAreas(); i++ )
            {
                qsc_p[i] = 0;
            }
			sess->bout << "\r\nQ-Scan pointers reset.\r\n\n";
        }
        break;
    case 'C':
        {
            nl();
            qsc_p[usub[sess->GetCurrentMessageArea()].subnum] = 0;
            sess->bout << "Messages on " << subboards[usub[sess->GetCurrentMessageArea()].subnum].name << " marked as unread.\r\n";
        }
        break;
    }
}


void DirList()
{
    dirlist( 0 );
}


void UpSubConf()
{
    if ( okconf( &sess->thisuser ) )
    {
        if ((sess->GetCurrentConferenceMessageArea() < subconfnum - 1) && (uconfsub[sess->GetCurrentConferenceMessageArea() + 1].confnum >= 0))
        {
            sess->SetCurrentConferenceMessageArea( sess->GetCurrentConferenceMessageArea() + 1 );
        }
        else
        {
            sess->SetCurrentConferenceMessageArea( 0 );
        }
        setuconf( CONF_SUBS, sess->GetCurrentConferenceMessageArea(), -1 );
    }
}


void DownSubConf()
{
    if ( okconf( &sess->thisuser ) )
    {
        if ( sess->GetCurrentConferenceMessageArea() > 0 )
        {
            sess->SetCurrentConferenceMessageArea( sess->GetCurrentConferenceMessageArea() - 1 );
        }
        else
        {
            while ( uconfsub[sess->GetCurrentConferenceMessageArea() + 1].confnum >= 0 && sess->GetCurrentConferenceMessageArea() < subconfnum - 1 )
            {
                sess->SetCurrentConferenceMessageArea( sess->GetCurrentConferenceMessageArea() + 1 );
            }
        }
        setuconf( CONF_SUBS, sess->GetCurrentConferenceMessageArea(), -1 );
    }
}


void DownSub()
{
    if ( sess->GetCurrentMessageArea() > 0 )
    {
        sess->SetCurrentMessageArea( sess->GetCurrentMessageArea() - 1 );
    }
    else
    {
        while ( usub[sess->GetCurrentMessageArea() + 1].subnum >= 0 && 
                sess->GetCurrentMessageArea() < sess->num_subs - 1 )
        {
            sess->SetCurrentMessageArea( sess->GetCurrentMessageArea() + 1 );
        }
    }
}


void UpSub()
{
    if ( sess->GetCurrentMessageArea() < sess->num_subs - 1 && 
         usub[sess->GetCurrentMessageArea() + 1].subnum >= 0 )
    {
        sess->SetCurrentMessageArea( sess->GetCurrentMessageArea() + 1 );
    }
    else
    {
        sess->SetCurrentMessageArea( 0 );
    }
}


void ValidateUser()
{
    nl( 2 );
	sess->bout << "|#9Enter user name or number:\r\n:";
    std::string userName;
    input( userName, 30, true );
    int nUserNum = finduser1( userName.c_str() );
    if ( nUserNum > 0 )
    {
        sysoplogf( "@ Validated user #%d", nUserNum );
        valuser( nUserNum );
    }
    else
    {
        sess->bout << "Unknown user.\r\n";
    }
}


void Chains()
{
    if (GuestCheck())
    {
        write_inst(INST_LOC_CHAINS, 0, INST_FLAGS_NONE);
        play_sdf( CHAINS_NOEXT, false );
        printfile( CHAINS_NOEXT );
        sess->SetMMKeyArea( WSession::mmkeyChains );
        while ( sess->GetMMKeyArea() == WSession::mmkeyChains && !hangup )
        {
            do_chains();
        }
    }
}


void TimeBank()
{
    if (GuestCheck())
    {
        write_inst(INST_LOC_BANK, 0, INST_FLAGS_NONE);
        time_bank();
    }
}


void AutoMessage()
{
    write_inst(INST_LOC_AMSG, 0, INST_FLAGS_NONE);
    do_automessage();
}



void Defaults(MenuInstanceData * MenuData)
{
    if (GuestCheck())
    {
        write_inst(INST_LOC_DEFAULTS, 0, INST_FLAGS_NONE);
        if (printfile(DEFAULTS_NOEXT))
        {
            pausescr();
        }
        defaults(MenuData);
    }
}


void SendEMail()
{
    send_email();
}


void FeedBack()
{
    write_inst(INST_LOC_FEEDBACK, 0, INST_FLAGS_NONE);
    feedback( false );
}


void Bulletins()
{
    write_inst(INST_LOC_GFILES, 0, INST_FLAGS_NONE);
    printfile(GFILES_NOEXT);
    gfiles();
}


void SystemInfo()
{
    WWIVVersion();

    if ( printfile( LOGON_NOEXT ) )
	{
		// Only display the pause if the file is not empty and contains information
		pausescr();
	}

    if ( printfile( SYSTEM_NOEXT ) )
	{
		pausescr();
	}
}


void JumpSubConf()
{
    if ( okconf( &sess->thisuser ) )
    {
        jump_conf(CONF_SUBS);
    }
}

void KillEMail()
{
    if (GuestCheck())
    {
        write_inst(INST_LOC_KILLEMAIL, 0, INST_FLAGS_NONE);
        kill_old_email();
    }
}


void LastCallers()
{
    if (status.callstoday > 0)
    {
		if ( app->HasConfigFlag( OP_FLAGS_SHOW_CITY_ST ) &&
			 ( syscfg.sysconfig & sysconfig_extended_info ) )
        {
            sess->bout << "|#2Number Name/Handle               Time  Date  City            ST Cty Modem    ##\r\n";
        }
        else
        {
            sess->bout << "|#2Number Name/Handle               Language   Time  Date  Speed                ##\r\n";
        }
        int i = okansi() ? 205 : '=';
		sess->bout << "|#7" << charstr( 79, i ) << wwiv::endl;
    }
    printfile( USER_LOG );
}


void ReadEMail()
{
    readmail( 0 );
}


void NewMessageScan()
{
    if ( okconf( &sess->thisuser ) )
    {
        nl();
        sess->bout << "|#5New message scan in all conferences? ";
        if (noyes())
        {
            NewMsgsAllConfs();
            return;
        }
    }
    write_inst(INST_LOC_SUBS, 65535, INST_FLAGS_NONE);
    express = false;
    expressabort = false;
    newline = false;
    preload_subs();
    nscan();
    newline = true;
}


void GoodBye()
{
    char szFileName[ MAX_PATH ];
    int cycle;
    int ch;

    if (sess->numbatchdl != 0)
    {
        nl();
        sess->bout << "|#2Download files in your batch queue (|#1Y/n|#2)? ";
        if (noyes())
		{
            batchdl( 1 );
		}
    }
    sprintf(szFileName, "%s%s", sess->pszLanguageDir, LOGOFF_MAT);
    if (!WFile::Exists(szFileName))
    {
        sprintf(szFileName, "%s%s", syscfg.gfilesdir, LOGOFF_MAT);
    }
    if (WFile::Exists(szFileName))
    {
        cycle = 0;
        do
        {
            ClearScreen();
            printfile( szFileName );
            ch = onek( "QFTO", true );
            switch (ch)
            {
            case 'Q':
                cycle = 1;
                break;
            case 'F':
                write_inst(INST_LOC_FEEDBACK, 0, INST_FLAGS_ONLINE);
                feedback( false );
                app->localIO->UpdateTopScreen();
                break;
            case 'T':
                write_inst(INST_LOC_BANK, 0, INST_FLAGS_ONLINE);
                time_bank();
                break;
            case 'O':
                cycle = 1;
                write_inst( INST_LOC_LOGOFF, 0, INST_FLAGS_NONE );
                ClearScreen();
				sess->bout <<  "Time on   = " << ctim( timer() - timeon ) << wwiv::endl;
                tmp_disable_pause( true );
                printfile( LOGOFF_NOEXT );
                tmp_disable_pause( false );
                sess->thisuser.SetLastSubNum( sess->GetCurrentMessageArea() );
                sess->thisuser.SetLastDirNum( sess->GetCurrentFileArea() );
                if ( okconf( &sess->thisuser ) )
                {
                    sess->thisuser.SetLastSubConf( sess->GetCurrentConferenceMessageArea() );
                    sess->thisuser.SetLastDirConf( sess->GetCurrentConferenceFileArea() );
                }
                hangup = true;
                break;
            }
        } while ( cycle == 0 );
    }
    else
    {
        nl( 2 );
        sess->bout << "|#5Log Off? ";
        if (yesno())
        {
            write_inst(INST_LOC_LOGOFF, 0, INST_FLAGS_NONE);
            ClearScreen();
			sess->bout << "Time on   = " << ctim( timer() - timeon ) << wwiv::endl;
            printfile(LOGOFF_NOEXT);
            sess->thisuser.SetLastSubNum( sess->GetCurrentMessageArea() );
            sess->thisuser.SetLastDirNum( sess->GetCurrentFileArea() );
            if ( okconf( &sess->thisuser ) )
            {
                sess->thisuser.SetLastSubConf( sess->GetCurrentConferenceMessageArea() );
                sess->thisuser.SetLastDirConf( sess->GetCurrentConferenceFileArea() );
            }
            hangup = true;
        }
    }
}


void WWIV_PostMessage()
{
    irt[0] = 0;
    irt_name[0] = 0;
    grab_quotes(NULL, NULL);
    if (usub[0].subnum != -1)
    {
        post();
    }
}


void ScanSub()
{
    if (usub[0].subnum != -1)
    {
        write_inst(INST_LOC_SUBS, usub[sess->GetCurrentMessageArea()].subnum, INST_FLAGS_NONE);
        int i = 0;
        express = expressabort = false;
        qscan( sess->GetCurrentMessageArea(), &i );
    }
}


void RemovePost()
{
    if (usub[0].subnum != -1)
    {
        write_inst(INST_LOC_SUBS, usub[sess->GetCurrentMessageArea()].subnum, INST_FLAGS_NONE);
        remove_post();
    }
}


void TitleScan()
{
    if (usub[0].subnum != -1)
    {
        write_inst(INST_LOC_SUBS, usub[sess->GetCurrentMessageArea()].subnum, INST_FLAGS_NONE);
        express = false;
        expressabort = false;
        ScanMessageTitles();
    }
}


void ListUsers()
{
    list_users( LIST_USERS_MESSAGE_AREA );
}


void Vote()
{
    if ( GuestCheck() )
    {
        write_inst( INST_LOC_VOTE, 0, INST_FLAGS_NONE );
        vote();
    }
}


void ToggleExpert()
{
    sess->thisuser.toggleStatusFlag( WUser::expert );
}


void ExpressScan()
{
    express = true;
    expressabort = false;
    tmp_disable_pause( true );
    newline = false;
    preload_subs();
    nscan();
    newline = true;
    express = false;
    expressabort = false;
    tmp_disable_pause( false );
}


void WWIVVersion()
{
    nl();
    ClearScreen();
	sess->bout << "|#9WWIV Bulletin Board System " << wwiv_version << " " << beta_version << wwiv::endl;
    sess->bout << "|#9Copyright (C) 1998-2004, WWIV Software Services.\r\n";
    sess->bout << "|#9All Rights Reserved.\r\n\r\n";
	sess->bout << "|#9Compile Time  : |#2" << wwiv_date << wwiv::endl;
    sess->bout << "|#9SysOp Name:    |#2%s" << syscfg.sysopname << wwiv::endl;
    nl( 3 );
    pausescr();
}


void InstanceEdit()
{
    instance_edit();
}


void JumpEdit()
{
    write_inst(INST_LOC_CONFEDIT, 0, INST_FLAGS_NONE);
    edit_confs();
}


void BoardEdit()
{
    write_inst(INST_LOC_BOARDEDIT, 0, INST_FLAGS_NONE);
    sysoplog("@ Ran Board Edit");
    boardedit();
}


void ChainEdit()
{
    write_inst(INST_LOC_CHAINEDIT, 0, INST_FLAGS_NONE);
    sysoplog("@ Ran Chain Edit");
    chainedit();
}


void ToggleChat()
{
    nl( 2 );
    bool bOldAvail = sysop2();
    ToggleScrollLockKey();
    bool bNewAvail = sysop2();
    if ( bOldAvail != bNewAvail )
    {
        sess->bout << ( ( bNewAvail ) ? "|10Sysop now available\r\n" : "|13Sysop now unavailable\r\n" );
        sysoplog("@ Changed sysop available status");
    }
    else
    {
        sess->bout << "|12Unable to toggle Sysop availability (hours restriction)\r\n";
    }
    app->localIO->UpdateTopScreen();
}


void ChangeUser()
{
    write_inst(INST_LOC_CHUSER, 0, INST_FLAGS_NONE);
    chuser();
}


void CallOut()
{
	force_callout( 2 );
}


void Debug()
{
    sess->SetGlobalDebugLevel( sess->GetGlobalDebugLevel() + 1 );
    if ( sess->GetGlobalDebugLevel() > 4 )
    {
        sess->SetGlobalDebugLevel( 0 );
    }
	sess->bout << "|10New Debug Level: " << sess->GetGlobalDebugLevel() << wwiv::endl;
}


void DirEdit()
{
    write_inst(INST_LOC_DIREDIT, 0, INST_FLAGS_NONE);
    sysoplog("@ Ran Directory Edit");
    dlboardedit();
}


void EventEdit()
{
    write_inst(INST_LOC_EVENTEDIT, 0, INST_FLAGS_NONE);
    sysoplog("- Ran Event Editor");
    eventedit();
}


void LoadTextFile()
{
    nl();
    sess->bout << "|#9Enter Filename: ";
    std::string fileName;
    input( fileName, 50, true );
    if ( !fileName.empty() )
    {
        nl();
        sess->bout << "|#5Allow editing? ";
        if ( yesno() )
        {
            nl();
            LoadFileIntoWorkspace( fileName.c_str(), false );
        }
        else
        {
            nl();
            LoadFileIntoWorkspace( fileName.c_str(), true );
        }
    }
}


void EditText()
{
    write_inst( INST_LOC_TEDIT, 0, INST_FLAGS_NONE );
    nl();
    sess->bout << "|#7Enter Filespec: ";
    std::string fileName;
    input( fileName, 50 );
    if ( !fileName.empty() )
    {
        external_edit( fileName.c_str(), "", sess->thisuser.GetDefaultEditor() - 1, 500, ".", fileName.c_str(), MSGED_FLAG_NO_TAGLINE );
    }
}


void EditBulletins()
{
    write_inst( INST_LOC_GFILEEDIT, 0, INST_FLAGS_NONE);
    sysoplog( "@ Ran Gfile Edit" );
    gfileedit();
}


void ReadAllMail()
{
    write_inst( INST_LOC_MAILR, 0, INST_FLAGS_NONE );
    sysoplog( "@ Read mail" );
    mailr();
}


void RebootComputer()
{
    WWIV_RebootComputer();
}


void ReloadMenus()
{
    write_inst( INST_LOC_RELOAD, 0, INST_FLAGS_NONE );
    read_new_stuff();
}


void ResetFiles()
{
    write_inst( INST_LOC_RESETF, 0, INST_FLAGS_NONE );
    reset_files();
}


void ResetQscan()
{
    sess->bout << "|#5Reset all QScan/NScan pointers? ";
    if ( yesno() )
    {
        write_inst(INST_LOC_RESETQSCAN, 0, INST_FLAGS_NONE);
        for ( int i = 0; i <= number_userrecs(); i++ )
        {
            read_qscn( i, qsc, true );
            memset( qsc_p, 0, syscfg.qscn_len - 4 * ( 1 + ( ( sess->GetMaxNumberFileAreas() + 31 ) / 32 ) + ( ( sess->GetMaxNumberMessageAreas() + 31 ) / 32 ) ) );
            write_qscn( i, qsc, true );
        }
        read_qscn( 1, qsc, false );
        close_qscn();
    }
}


void MemoryStatus()
{
    app->statusMgr->Read();
    nl();
    sess->bout << "Qscanptr        : " << status.qscanptr << wwiv::endl;
}


void PackMessages()
{
    nl();
    sess->bout << "|#5Pack all subs? ";
    if ( yesno() )
    {
        pack_all_subs( false );
    }
    else
    {
        pack_sub( usub[sess->GetCurrentMessageArea()].subnum );
    }
}


void InitVotes()
{
    write_inst( INST_LOC_VOTE, 0, INST_FLAGS_NONE );
    sysoplog( "@ Ran Ivotes" );
    ivotes();
}


void ReadLog()
{
    char szLogFileName[ 255 ];

    slname( date(), szLogFileName );
    print_local_file( szLogFileName, "" );
}


void ReadNetLog()
{
    print_local_file( "NET.LOG", "" );
}


void PrintPending()
{
    print_pending_list();
}


void PrintStatus()
{
    prstatus( false );
}


void TextEdit()
{
    write_inst( INST_LOC_TEDIT, 0, INST_FLAGS_NONE );
    sysoplog( "@ Ran Text Edit" );
    text_edit();
}


void UserEdit()
{
    write_inst( INST_LOC_UEDIT, 0, INST_FLAGS_NONE );
    sysoplog( "@ Ran User Edit" );
    uedit( sess->usernum, UEDIT_NONE );
}


void VotePrint()
{
    write_inst( INST_LOC_VOTEPRINT, 0, INST_FLAGS_NONE );
    voteprint();
}


void YesturdaysLog()
{
    app->statusMgr->Read();
    print_local_file( status.log1, "" );
}


void ZLog()
{
    zlog();
}


void ViewNetDataLog()
{
	bool done = false;

    while ( !done && !hangup )
    {
        nl();
        sess->bout << "|#9Which NETDAT log (0-2,Q)? ";
        char ch = onek( "Q012" );
        switch ( ch )
        {
        case 'Q':
            done = true;
            break;
        case '0':
            print_local_file( "netdat0.log", "" );
            break;
        case '1':
            print_local_file( "netdat1.log", "" );
            break;
        case '2':
            print_local_file( "netdat2.log", "" );
            break;
        }
    }
}


void UploadPost()
{
    upload_post();
}


void NetListing()
{
    print_net_listing( false );
}


void WhoIsOnline()
{
    multi_instance();
	nl();
	pausescr();
}


void NewMsgsAllConfs()
{
    bool ac = false;

    write_inst(INST_LOC_SUBS, usub[sess->GetCurrentMessageArea()].subnum, INST_FLAGS_NONE);
    express = false;
    expressabort = false;
    newline = false;
    if ( uconfsub[1].confnum != -1 && okconf( &sess->thisuser ) )
    {
        ac = true;
        tmp_disable_conf( true );
    }
    preload_subs();
    nscan();
    newline = true;
    if ( ac == true )
    {
        tmp_disable_conf( false );
    }
}


void MultiEmail()
{
    slash_e();
}

void InternetEmail()
{
    send_inet_email();
}


void NewMsgScanFromHere()
{
    newline = false;
    preload_subs();
    nscan( sess->GetCurrentMessageArea() );
    newline = true;
}


void ValidateScan()
{
    newline = false;
    valscan();
    newline = true;
}


void ChatRoom()
{
    char szCommandLine[ MAX_PATH ];

    write_inst( INST_LOC_CHATROOM, 0, INST_FLAGS_NONE );
    if ( WFile::Exists( "WWIVCHAT.EXE" ) )
    {
        sprintf( szCommandLine, "WWIVCHAT.EXE %s", create_chain_file() );
        ExecuteExternalProgram( szCommandLine, app->GetSpawnOptions( SPWANOPT_CHAT ) );
    }
    else
    {
        chat_room();
    }
}


void DownloadPosts()
{
    if ( app->HasConfigFlag( OP_FLAGS_SLASH_SZ ) )
    {
        sess->bout << "|#5This could take quite a while.  Are you sure? ";
        if ( yesno() )
        {
            sess->bout << "Please wait...\r\n";
            app->localIO->set_x_only(1, "posts.txt", 0);
            preload_subs();
            nscan();
            app->localIO->set_x_only( 0, NULL, 0 );
            add_arc( "offline", "posts.txt", 0 );
            download_temp_arc( "offline", 0 );
        }
    }
}


void DownloadFileList()
{
    if ( app->HasConfigFlag( OP_FLAGS_SLASH_SZ ) )
    {
        sess->bout << "|#5This could take quite a while.  Are you sure? ";
        if ( yesno() )
        {
            sess->bout << "Please wait...\r\n";
            app->localIO->set_x_only( 1, "files.txt", 1 );
            searchall();
            app->localIO->set_x_only( 0, NULL, 0 );
            add_arc( "temp", "files.txt", 0 );
            download_temp_arc( "temp", 0 );
        }
    }
}


void ClearQScan()
{
    nl();
    sess->bout << "|#5Mark messages as read on [C]urrent sub or [A]ll subs (A/C/Q)? ";
    char ch = onek( "QAC\r" );
    switch ( ch )
    {
    case 'Q':
    case RETURN:
        break;
    case 'A':
    {
        app->statusMgr->Read();
        for ( int i = 0; i < sess->GetMaxNumberMessageAreas(); i++ )
        {
            qsc_p[i] = status.qscanptr - 1L;
        }
        nl();
        sess->bout << "Q-Scan pointers cleared.\r\n";
    }
    break;
    case 'C':
        app->statusMgr->Read();
        nl();
        qsc_p[usub[sess->GetCurrentMessageArea()].subnum] = status.qscanptr - 1L;
        sess->bout << "Messages on " << subboards[usub[sess->GetCurrentMessageArea()].subnum].name << " marked as read.\r\n";
        break;
    }
}


void FastGoodBye()
{
    if ( sess->numbatchdl != 0 )
    {
        nl();
        sess->bout << "|#2Download files in your batch queue (|#1Y/n|#2)? ";
        if ( noyes() )
        {
            batchdl( 1 );
        }
        else
        {
            hangup = true;
        }
    }
    else
    {
        hangup = true;
    }
    sess->thisuser.SetLastSubNum( sess->GetCurrentMessageArea() );
    sess->thisuser.SetLastDirNum( sess->GetCurrentFileArea() );
    if ( okconf( &sess->thisuser ) )
    {
        sess->thisuser.SetLastSubConf( sess->GetCurrentConferenceMessageArea() );
        sess->thisuser.SetLastDirConf( sess->GetCurrentConferenceFileArea() );
    }
}


void NewFilesAllConfs()
{
    nl();
    int ac = 0;
    if ( uconfsub[1].confnum != -1 && okconf( &sess->thisuser ) )
    {
        ac = 1;
        tmp_disable_conf( true );
    }
    g_num_listed = 0;
    sess->tagging = 1;
    sess->titled = 1;
    nscanall();
    sess->tagging = 0;
    if ( ac )
    {
       tmp_disable_conf( false );
    }
}


void ReadIDZ()
{
    nl();
    sess->bout << "|#5Read FILE_ID.DIZ for all directories? ";
    if (yesno())
    {
        read_idz_all();
    }
    else
    {
        read_idz( 1, sess->GetCurrentFileArea() );
    }
}


void RemoveNotThere()
{
    removenotthere();
}


void UploadAllDirs()
{
    nl( 2 );
    bool ok = true;
    for ( int nDirNum = 0; nDirNum < sess->num_dirs && udir[nDirNum].subnum >= 0 && ok && !hangup; nDirNum++ )
    {
        sess->bout << "|#9Now uploading files for: |#2" << directories[udir[nDirNum].subnum].name << wwiv::endl;
        ok = uploadall( nDirNum );
    }
}


void UploadCurDir()
{
    uploadall(sess->GetCurrentFileArea());
}


void RenameFiles()
{
    rename_file();
}


void MoveFiles()
{
    move_file();
}


void SortDirs()
{
    nl();
    sess->bout << "|#5Sort all dirs? ";
    bool bSortAll = yesno();
    nl();
    sess->bout << "|#5Sort by date? ";

    int nType = 0;
    if ( yesno() )
    {
        nType = 2;
    }

    tmp_disable_pause( true );
    if ( bSortAll )
    {
        sort_all( nType );
    }
    else
    {
        sortdir( udir[sess->GetCurrentFileArea()].subnum, nType );
    }
    tmp_disable_pause( false );
}


void ReverseSort()
{
    nl();
    sess->bout << "|#5Sort all dirs? ";
    bool bSortAll = yesno();
    nl();
    tmp_disable_pause( true );
    if ( bSortAll )
    {
        sort_all( 1 );
    }
    else
    {
        sortdir( udir[sess->GetCurrentFileArea()].subnum, 1 );
    }
    tmp_disable_pause( false );
}


void AllowEdit()
{
    edit_database();
}


void UploadFilesBBS()
{
    char s2[81];

    nl();
    sess->bout << "|#21|#9) PCB, RBBS   - <filename> <size> <date> <description>\r\n";
    sess->bout << "|#22|#9) QBBS format - <filename> <description>\r\n";
    nl();
    sess->bout << "|#Select Format (1,2,Q) : ";
    char ch = onek( "Q12" );
    nl();
    if ( ch != 'Q' )
    {
        int nType = 0;
        sess->bout << "|#9Enter Filename (wildcards allowed).\r\n|#7: ";
		mpl( 77 );
        inputl( s2, 80 );
        switch ( ch )
        {
        case '1':
            nType = 2;
            break;
        case '2':
            nType = 0;
            break;
        default:
            nType = 0;
            break;
        }
        upload_files( s2, sess->GetCurrentFileArea(), nType );
    }
}


void UpDirConf()
{
    if ( okconf( &sess->thisuser ) )
    {
        if ( sess->GetCurrentConferenceFileArea() < dirconfnum - 1 && uconfdir[sess->GetCurrentConferenceFileArea() + 1].confnum >= 0 )
        {
            sess->SetCurrentConferenceFileArea( sess->GetCurrentConferenceFileArea() + 1 );
        }
        else
        {
            sess->SetCurrentConferenceFileArea( 0 );
        }
        setuconf( CONF_DIRS, sess->GetCurrentConferenceFileArea(), -1 );
    }
}


void UpDir()
{
    if ( sess->GetCurrentFileArea() < sess->num_dirs - 1 && udir[sess->GetCurrentFileArea() + 1].subnum >= 0 )
    {
        sess->SetCurrentFileArea( sess->GetCurrentFileArea() + 1 );
    }
    else
    {
        sess->SetCurrentFileArea( 0 );
    }
}


void DownDirConf()
{
    if ( okconf( &sess->thisuser ) )
    {
        if ( sess->GetCurrentConferenceFileArea() > 0 )
        {
            sess->SetCurrentConferenceFileArea( sess->GetCurrentConferenceFileArea() );
        }
        else
        {
            while ( uconfdir[sess->GetCurrentConferenceFileArea() + 1].confnum >= 0 && sess->GetCurrentConferenceFileArea() < dirconfnum - 1 )
            {
                sess->SetCurrentConferenceFileArea( sess->GetCurrentConferenceFileArea() + 1 );
            }
        }
        setuconf( CONF_DIRS, sess->GetCurrentConferenceFileArea(), -1 );
    }
}


void DownDir()
{
    if ( sess->GetCurrentFileArea() > 0 )
    {
        sess->SetCurrentFileArea( sess->GetCurrentFileArea() - 1 );
    }
    else
    {
        while ( udir[sess->GetCurrentFileArea() + 1].subnum >= 0 && 
                sess->GetCurrentFileArea() < sess->num_dirs - 1 )
        {
            sess->SetCurrentFileArea( sess->GetCurrentFileArea() + 1 );
        }
    }
}


void ListUsersDL()
{
    list_users( LIST_USERS_FILE_AREA );
}


void PrintDSZLog()
{
    if ( WFile::Exists( g_szDSZLogFileName ) )
    {
        print_local_file( g_szDSZLogFileName, "" );
    }
}


void PrintDevices()
{
    print_devices();
}


void ViewArchive()
{
    arc_l();
}


void BatchMenu()
{
    batchdl( 0 );
}


void Download()
{
    play_sdf( DOWNLOAD_NOEXT, false );
    printfile(DOWNLOAD_NOEXT);
    download();
}


void TempExtract()
{
    temp_extract();
}


void FindDescription()
{
    sess->tagging = 1;
    finddescription();
    sess->tagging = 0;
}


void TemporaryStuff()
{
    temporary_stuff();
}


void JumpDirConf()
{
    if ( okconf( &sess->thisuser ) )
    {
        jump_conf(CONF_DIRS);
    }
}

void ConfigFileList()
{
    if (ok_listplus())
    {
        config_file_list();
    }
}


void ListFiles()
{
    sess->tagging = 1;
    listfiles();
    sess->tagging = 0;
}


void NewFileScan()
{
    if ( app->HasConfigFlag( OP_FLAGS_SETLDATE ) )
    {
        SetNewFileScanDate();
    }
    bool abort = false;
    g_num_listed = 0;
    sess->tagging = 1;
    sess->titled = 1;
    nl();
    sess->bout << "|#5Search all directories? ";
    if ( yesno() )
    {
        nscanall();
    }
    else
    {
        nl();
        nscandir( sess->GetCurrentFileArea(), &abort );
        if ( g_num_listed )
        {
            endlist( 2 );
        }
        else
        {
            nl();
            sess->bout << "|#2No new files found.\r\n";
        }
    }
    sess->tagging = 0;
}


void RemoveFiles()
{
    if ( GuestCheck() )
    {
        removefile();
    }
}


void SearchAllFiles()
{
    sess->tagging = 1;
    searchall();
    sess->tagging = 0;
}


void XferDefaults()
{
    if ( GuestCheck() )
    {
        xfer_defaults();
    }
}


void Upload()
{
    play_sdf( UPLOAD_NOEXT, false );
    printfile( UPLOAD_NOEXT );
    if ( sess->thisuser.isRestrictionValidate() || sess->thisuser.isRestrictionUpload() ||
         ( syscfg.sysconfig & sysconfig_all_sysop ) )
    {
        if ( syscfg.newuploads < sess->num_dirs )
        {
            upload( static_cast<int>( syscfg.newuploads ) );
        }
        else
        {
            upload( 0 );
        }
    }
    else
    {
        upload( udir[sess->GetCurrentFileArea()].subnum );
    }
}


void YourInfoDL()
{
    YourInfo();
}


void UploadToSysop()
{
    printfile( ZUPLOAD_NOEXT );
    nl( 2 );
    sess->bout << "Sending file to sysop :-\r\n\n";
    upload( 0 );
}


void ReadAutoMessage()
{
    read_automessage();
}


void GuestApply()
{
    if ( guest_user )
    {
        newuser();
    }
    else
    {
        sess->bout << "You already have an account on here!\r\n\r\n";
    }
}


void AttachFile()
{
    attach_file( 0 );
}


bool GuestCheck()
{
    if ( guest_user )
    {
        sess->bout << "|#6This command is only for registered users.\r\n";
        return false;
    }
    return true;
}


void SetSubNumber( char *pszSubKeys )
{
    for ( int i = 0; (i < sess->num_subs) && (usub[i].subnum != -1); i++ )
    {
        if ( wwiv::stringUtils::IsEquals( usub[i].keys, pszSubKeys ) )
        {
            sess->SetCurrentMessageArea( i );
        }
    }
}


void SetDirNumber(char *pszDirectoryKeys)
{
    for ( int i = 0; i < sess->num_dirs; i++ )
    {
        if ( wwiv::stringUtils::IsEquals( udir[i].keys, pszDirectoryKeys ) )
        {
            sess->SetCurrentFileArea( i );
        }
    }
}


