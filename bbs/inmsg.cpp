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

//
// Local function prototypes
//
void AddLineToMessageBuffer( char *pszMessageBuffer, const char *pszLineToAdd, long *plBufferLength );
bool GetMessageToName( const char *aux );
void ReplaceString( char *pszResult, char *pszOld, char *pszNew );


bool InternalMessageEditor( char *lin, int maxli, int &curli, int &setanon, char *pszTitle );
void GetMessageTitle( char *pszTitle, bool force_title );
bool ExternalMessageEditor( int maxli, int &setanon, char *pszTitle, const char *pszDestination, int flags );
void UpdateMessageBufferTheadsInfo( char *pszMessageBuffer, long *plBufferLength, const char *aux );
void UpdateMessageBufferInReplyToInfo( char *pszMessageBuffer, long *plBufferLength, const char *aux );
void UpdateMessageBufferTagLine( char *pszMessageBuffer, long *plBufferLength, const char *aux );
void UpdateMessageBufferQuotesCtrlLines( char *pszMessageBuffer, long *plBufferLength );
void GetMessageAnonStatus( bool &real_name, int *anony, int setanon );


#define LEN 161


void inmsg(messagerec * pMessageRecord, char *pszTitle, int *anony, bool needtitle, const char *aux, int fsed, const char *pszDestination, int flags, bool force_title )
{
	char *lin = NULL, *b = NULL;

    int oiia = setiia( 0 );

	if ( fsed != INMSG_NOFSED && !okfsed() )
	{
		fsed = INMSG_NOFSED;
	}

    char szExtEdFileName[MAX_PATH];
    sprintf(szExtEdFileName, "%s%s", syscfgovr.tempdir, INPUT_MSG);
	if (fsed)
	{
		fsed = INMSG_FSED;
	}
	if (use_workspace)
	{
		if (!WFile::Exists(szExtEdFileName))
		{
			use_workspace = false;
		}
		else
		{
			fsed = INMSG_FSED_WORKSPACE;
		}
	}
	int setanon = 0;
	bool bSaveMessage = false;
	int curli = 0;

    int maxli = GetMaxMessageLinesAllowed();
	if (!fsed)
	{
		if ( ( lin = static_cast<char *>( BbsAllocA( ( maxli + 10 ) * LEN ) ) ) == NULL )
		{
			pMessageRecord->stored_as = 0xffffffff;
			setiia(oiia);
			return;
		}
		for (int i = 0; i < maxli; i++)
		{
			lin[i * LEN] = '\0';
		}
	}
	nl();

	if ( irt_name[0] == '\0')
	{
		if ( GetMessageToName( aux ) )
		{
			nl();
		}
	}

    GetMessageTitle( pszTitle, force_title );
	if ( pszTitle[0] == '\0' && needtitle )
	{
		sess->bout << "|12Aborted.\r\n";
		pMessageRecord->stored_as = 0xffffffff;
		if (!fsed)
		{
			BbsFreeMemory( lin );
		}
		setiia(oiia);
		return;
	}

	if ( fsed == INMSG_NOFSED ) // Use Internal Message Editor
    {
        bSaveMessage = InternalMessageEditor( lin, maxli, curli, setanon, pszTitle );
    }
    else if ( fsed == INMSG_FSED )   // Use Full Screen Editor
	{
        bSaveMessage = ExternalMessageEditor( maxli, setanon, pszTitle, pszDestination, flags );
	}
	else if ( fsed == INMSG_FSED_WORKSPACE )   // "auto-send mail message"
	{
		bSaveMessage = WFile::Exists(szExtEdFileName);
		if ( bSaveMessage && !sess->IsNewMailWatiting() )
		{
			sess->bout << "Reading in file...\r\n";
		}
		use_workspace = false;
	}

	if ( bSaveMessage )
	{
    	long lMaxMessageSize = 0;
    	bool real_name = false;
        GetMessageAnonStatus( real_name, anony, setanon );
		BackLine();
		if ( !sess->IsNewMailWatiting() )
		{
			SpinPuts( "Saving...", 2 );
		}
		WFile fileExtEd( szExtEdFileName );
		if ( fsed )
		{
			fileExtEd.Open( WFile::modeBinary | WFile::modeReadOnly );
			long lExternalEditorFileSize = fileExtEd.GetLength();
			lMaxMessageSize  = std::max<long>( lExternalEditorFileSize, 30000 );
		}
		else
		{
			for (int i5 = 0; i5 < curli; i5++)
			{
				lMaxMessageSize  += ( strlen(&(lin[i5 * LEN])) + 2 );
			}
		}
		lMaxMessageSize  += 1024;
		if ( ( b = static_cast<char *>( BbsAllocA( lMaxMessageSize ) ) ) == NULL )
		{
			if ( fileExtEd.IsOpen() )
            {
				fileExtEd.Close();
            }
			BbsFreeMemory(lin);
			sess->bout << "Out of memory.\r\n";
			pMessageRecord->stored_as = 0xffffffff;
			setiia(oiia);
			return;
		}
		long lCurrentMessageSize = 0;

        // Add author name
		if (real_name)
		{
			AddLineToMessageBuffer( b, sess->thisuser.GetRealName(), &lCurrentMessageSize );
		}
		else
		{
			if ( sess->IsNewMailWatiting() )
			{
                char szSysopName[ 255 ];
				sprintf( szSysopName, "%s #1", syscfg.sysopname );
				AddLineToMessageBuffer( b, szSysopName, &lCurrentMessageSize );
			}
			else
			{
				AddLineToMessageBuffer( b, sess->thisuser.GetUserNameNumberAndSystem( sess->usernum, net_sysnum ), &lCurrentMessageSize );
			}
		}

        // Add date to message body
        long lTime;
		time( &lTime );
        char szTime[ 255 ];
        strcpy( szTime, asctime( localtime( &lTime ) ) );
        szTime[ strlen( szTime ) - 1 ] = '\0';
		AddLineToMessageBuffer(b, szTime, &lCurrentMessageSize);

        UpdateMessageBufferQuotesCtrlLines( b, &lCurrentMessageSize );

		if ( sess->IsMessageThreadingEnabled() )
        {
            UpdateMessageBufferTheadsInfo( b, &lCurrentMessageSize, aux );
		}
		if (irt[0])
		{
            UpdateMessageBufferInReplyToInfo( b, &lCurrentMessageSize, aux );
		}
		if (fsed)
		{
            // Read the file produced by the external editor and add it to 'b'
			long lFileSize = fileExtEd.GetLength();
			fileExtEd.Read( (&(b[lCurrentMessageSize])), lFileSize );
			lCurrentMessageSize += lFileSize;
			fileExtEd.Close();
		}
		else
		{
            // iterate through the lines in "char *lin" and append them to 'b'
			for (int i5 = 0; i5 < curli; i5++)
			{
				AddLineToMessageBuffer(b, &(lin[i5 * LEN]), &lCurrentMessageSize);
			}
		}

		if ( app->HasConfigFlag( OP_FLAGS_MSG_TAG ) )
		{
            UpdateMessageBufferTagLine( b, &lCurrentMessageSize, aux );
		}
		if (b[lCurrentMessageSize - 1] != CZ)
        {
			b[lCurrentMessageSize++] = CZ;
        }
		savefile(b, lCurrentMessageSize, pMessageRecord, aux);
		if (fsed)
        {
			WFile::Remove(szExtEdFileName);
        }
	}
	else
	{
		if (fsed)
		{
			WFile::Remove(szExtEdFileName);
		}
		sess->bout << "|12Aborted.\r\n";
		pMessageRecord->stored_as = 0xffffffff;
	}
	if (!fsed)
	{
		BbsFreeMemory( lin );
	}
	charbufferpointer = 0;
	charbuffer[0] = '\0';
	setiia(oiia);
	grab_quotes(NULL, NULL);
}


void AddLineToMessageBuffer( char *pszMessageBuffer, const char *pszLineToAdd, long *plBufferLength )
{
	strcpy( &( pszMessageBuffer[ *plBufferLength ] ), pszLineToAdd );
	*plBufferLength += strlen( pszLineToAdd );
	strcpy( &( pszMessageBuffer[ *plBufferLength ] ), "\r\n" );
	*plBufferLength += 2;
}


void ReplaceString(char *pszResult, char *pszOld, char *pszNew)
{
	if (strlen(pszResult) - strlen(pszOld) + strlen(pszNew) >= LEN)
	{
		return;
	}
	char* ss = strstr(pszResult, pszOld);
	if (ss == NULL)
	{
		return;
	}
	ss[0] = '\0';

	char szTempResult[LEN];
	sprintf(szTempResult, "%s%s%s", pszResult, pszNew, ss + strlen(pszOld));
	strcpy(pszResult, szTempResult);
}


bool GetMessageToName( const char *aux )
{
	// If sess->GetCurrentReadMessageArea() is -1, then it hasn't been set by reading a sub,
	// also, if we are using e-mail, this is definately NOT a FidoNet
	// post so there's no reason in wasting everyone's time in the loop...
    WWIV_ASSERT( aux );
    if ( sess->GetCurrentReadMessageArea() == -1 ||
         wwiv::stringUtils::IsEqualsIgnoreCase( aux, "email" ) )
	{
		return 0;
	}

	bool bHasAddress = false;
    bool newlsave = newline;

	if (xsubs[sess->GetCurrentReadMessageArea()].num_nets)
	{
		for ( int i = 0; i < xsubs[sess->GetCurrentReadMessageArea()].num_nets; i++ )
		{
        	xtrasubsnetrec *xnp = &xsubs[sess->GetCurrentReadMessageArea()].nets[i];
			if ( net_networks[xnp->net_num].type == net_type_fidonet &&
                 !wwiv::stringUtils::IsEqualsIgnoreCase( aux, "email" ) )
			{
				bHasAddress = true;
				sess->bout << "|#1Fidonet addressee, |#7[|#2Enter|#7]|#1 for ALL |#0: ";
				newline = false;
                std::string toName;
				input1( toName, 40, MIXED, false, true );
				newline = newlsave;
                if ( toName.empty() )
				{
					strcpy(irt_name, "ALL");
                    sess->bout << "|#4All\r\n";
					ansic( 0 );
				}
				else
				{
                    strcpy( irt_name, toName.c_str() );
				}
				strcpy( irt, "�" );
			}
		}
	}
	return bHasAddress;
}


bool InternalMessageEditor( char *lin, int maxli, int &curli, int &setanon, char *pszTitle )
{
    bool abort, next;
    char s[ 255 ];
    char s1[ 255 ];

    nl( 2 );
    sess->bout << "|#9Enter message now, you can use |#2" << maxli << "|#9 lines.\r\n";
    sess->bout << "|#9Colors: ^P-0\003""11\003""22\003""33\003""44\003""55\003""66\003""77\003""88\003""99\003""AA\003""BB\003""CC\003""DD\003""EE\003""FF\003""GG\003""HH\003""II\003""JJ\003""KK\003""LL\003""MM\003""NN\003""OO\003""PP\003""QQ\003""RR\003""SS\003""""\003""0";
    sess->bout << "\003""TT\003""UU\003""VV\003""WW\003""XX\003""YY\003""ZZ\003""aa\003""bb\003""cc\003""dd\003""ee\003""ff\003""gg\003""hh\003""ii\003""jj\003""kk\003""ll\003""mm\003""nn\003""oo\003""pp\003""qq\003""rr\003""ss\003""tt\003""uu\003""vv\003""ww\003""xx\003""yy\003""zz\r\n";
    nl();
    sess->bout << "|#1Enter |#2/Q|#1 to quote previous message, |#2/HELP|#1 for other editor commands.\r\n";
    strcpy(s, "[---=----=----=----=----=----=----=----]----=----=----=----=----=----=----=----]");
    if ( sess->thisuser.GetScreenChars() < 80 )
    {
        s[ sess->thisuser.GetScreenChars() ] = '\0';
    }
    ansic( 7 );
    sess->bout << s;
    nl();

    bool bCheckMessageSize = true;
    bool bSaveMessage = false;
	bool done = false;
    char szRollOverLine[ 81 ];
    szRollOverLine[ 0 ] = '\0';
    while (!done && !hangup)
    {
        bool bAllowPrevious = ( curli > 0 ) ? true : false;
        while ( inli( s, szRollOverLine, 160, true, bAllowPrevious ) )
        {
            --curli;
            strcpy(szRollOverLine, &(lin[(curli) * LEN]));
            if (wwiv::stringUtils::GetStringLength(szRollOverLine) > sess->thisuser.GetScreenChars() - 1)
            {
                szRollOverLine[ sess->thisuser.GetScreenChars() - 2 ] = '\0';
            }
        }
        if (hangup)
        {
            done = true;
        }
        bCheckMessageSize = true;
        if (s[0] == '/')
        {
            if ( ( wwiv::stringUtils::IsEqualsIgnoreCase( s, "/HELP" ) ) ||
                 ( wwiv::stringUtils::IsEqualsIgnoreCase( s, "/H" ) ) ||
                 ( wwiv::stringUtils::IsEqualsIgnoreCase( s, "/?" ) ) )
            {
                bCheckMessageSize = false;
                printfile(EDITOR_NOEXT);
            }
            else if ( wwiv::stringUtils::IsEqualsIgnoreCase( s,"/QUOTE" ) ||
                      wwiv::stringUtils::IsEqualsIgnoreCase( s,"/Q" ) )
            {
                bCheckMessageSize = false;
                if (quotes_ind != NULL)
                {
                    get_quote( 0 );
                }
            }
            else if ( wwiv::stringUtils::IsEqualsIgnoreCase( s, "/LI" ) )
            {
                bCheckMessageSize = false;
                if ( okansi() )
                {
                    next = false;
                }
                else
                {
                    sess->bout << "|#5With line numbers? ";
                    next = yesno();
                }
                abort = false;
                for (int i = 0; (i < curli) && (!abort); i++)
                {
                    if (next)
                    {
						sess->bout << i + 1 << ":" << wwiv::endl;
                    }
                    strcpy(s1, &(lin[i * LEN]));
                    int i3 = strlen(s1);
                    if (s1[i3 - 1] == 1)
                    {
                        s1[i3 - 1] = '\0';
                    }
                    if (s1[0] == 2)
                    {
                        strcpy(s1, &(s1[1]));
                        int i5 = 0;
                        int i4 = 0;
                        for (i4 = 0; i4 < wwiv::stringUtils::GetStringLength(s1); i4++)
                        {
                            if ((s1[i4] == 8) || (s1[i4] == 3))
                            {
                                --i5;
                            }
                            else
                            {
                                ++i5;
                            }
                        }
                        for (i4 = 0; (i4 < (sess->thisuser.GetScreenChars() - i5) / 2) && (!abort); i4++)
                        {
                            osan(" ", &abort, &next);
                        }
                    }
                    pla(s1, &abort);
                }
                if ( !okansi() || next )
                {
                    nl();
                    sess->bout << "Continue...\r\n";
                }
            }
            else if ( wwiv::stringUtils::IsEqualsIgnoreCase( s, "/ES" ) ||
                      wwiv::stringUtils::IsEqualsIgnoreCase( s, "/S" ) )
            {
                bSaveMessage = true;
                done = true;
                bCheckMessageSize = false;
            }
            else if ( wwiv::stringUtils::IsEqualsIgnoreCase( s, "/ESY" ) ||
                      wwiv::stringUtils::IsEqualsIgnoreCase( s, "/SY" )  )
            {
                bSaveMessage = true;
                done = true;
                bCheckMessageSize = false;
                setanon = 1;
            }
            else if ( wwiv::stringUtils::IsEqualsIgnoreCase( s, "/ESN" ) ||
                      wwiv::stringUtils::IsEqualsIgnoreCase( s, "/SN" ) )
            {
                bSaveMessage = true;
                done = true;
                bCheckMessageSize = false;
                setanon = -1;
            }
            else if ( wwiv::stringUtils::IsEqualsIgnoreCase( s, "/ABT" ) ||
                      wwiv::stringUtils::IsEqualsIgnoreCase( s, "/A"  ) )
            {
                done = true;
                bCheckMessageSize = false;
            }
            else if ( wwiv::stringUtils::IsEqualsIgnoreCase( s, "/CLR" ) )
            {
                bCheckMessageSize = false;
                curli = 0;
                sess->bout << "Message cleared... Start over...\r\n\n";
            }
            else if ( wwiv::stringUtils::IsEqualsIgnoreCase( s, "/RL" ) )
            {
                bCheckMessageSize = false;
                if (curli)
                {
                    --curli;
                    sess->bout << "Replace:\r\n";
                }
                else
                {
                    sess->bout << "Nothing to replace.\r\n";
                }
            }
            else if ( wwiv::stringUtils::IsEqualsIgnoreCase( s, "/TI" ) )
            {
                bCheckMessageSize = false;
                if ( okansi() )
                {
                    sess->bout << "|#1Subj|#7: |#2" ;
                    inputl( pszTitle, 60, true );
                }
                else
                {
                    sess->bout << "       (---=----=----=----=----=----=----=----=----=----=----=----)\r\n";
                    sess->bout << "|#1Subj|#7: |#2";
                    inputl(pszTitle, 60);
                }
                sess->bout << "Continue...\r\n\n";
            }
            strcpy( s1, s );
            s1[3] = '\0';
            if ( wwiv::stringUtils::IsEqualsIgnoreCase( s1, "/C:" ) )
            {
                s1[0] = 2;
                strcpy( ( &s1[1] ), &( s[3] ) );
                strcpy( s, s1 );
            }
            else if ( wwiv::stringUtils::IsEqualsIgnoreCase( s1, "/SU" ) &&
                      s[3] == '/' && curli > 0 )
            {
                strcpy( s1, &( s[4] ) );
                char *ss = strstr( s1, "/" );
                if ( ss )
                {
                    char *ss1 = &(ss[1]);
                    ss[0] = '\0';
                    ReplaceString(&(lin[(curli - 1) * LEN]), s1, ss1);
                    sess->bout << "Last line:\r\n" << &(lin[(curli - 1) * LEN]) << "\r\nContinue...\r\n";
                }
                bCheckMessageSize = false;
            }
        }

        if ( bCheckMessageSize )
        {
            strcpy( &( lin[ ( curli++ ) * LEN ] ), s );
            if ( curli == ( maxli + 1 ) )
            {
                sess->bout << "\r\n-= No more lines, last line lost =-\r\n/S to save\r\n\n";
                --curli;
            }
            else if (curli == maxli)
            {
                sess->bout << "-= Message limit reached, /S to save =-\r\n";
            }
            else if ((curli + 5) == maxli)
            {
                sess->bout << "-= 5 lines left =-\r\n";
            }
        }
    }
    if (curli == 0)
    {
        bSaveMessage = false;
    }
    return bSaveMessage;
}


void GetMessageTitle( char *pszTitle, bool force_title )
{
    if ( okansi() )
    {
        if ( !sess->IsNewMailWatiting() )
        {
            sess->bout << "|#2Title: ";
            mpl( 60 );
        }
        if ( irt[0] != '�' && irt[0] )
        {
            char s1[ 255 ];
            char ch = '\0';
            StringTrim( irt );
            if ( strnicmp( stripcolors( irt ), "re:", 3 ) != 0 )
            {
                if ( sess->IsNewMailWatiting() )
                {
                    sprintf(s1, "%s", irt);
                    irt[0] = '\0';
                }
                else
                {
                    sprintf(s1, "Re: %s", irt);
                }
            }
            else
            {
                sprintf( s1, "%s", irt );
            }
            s1[60] = '\0';
            if ( !sess->IsNewMailWatiting() && !force_title )
            {
                sess->bout << s1;
                ch = getkey();
                if (ch == 10)
                {
                    ch = getkey();
                }
            }
            else
            {
                strcpy( pszTitle, s1 );
                ch = RETURN;
            }
            force_title = false;
            if ( ch != RETURN )
            {
                sess->bout << "\r";
                if ( ch == SPACE || ch == ESC )
                {
                    ch = '\0';
                }
                sess->bout << "|#2Title: ";
                mpl( 60 );
                char szRollOverLine[ 81 ];
                sprintf( szRollOverLine, "%c", ch );
                inli( s1, szRollOverLine, 60, true, false );
                sprintf( pszTitle, "%s", s1 );
            }
            else
            {
                nl();
                strcpy( pszTitle,s1 );
            }
        }
        else
        {
            inputl( pszTitle, 60 );
        }
    }
    else
    {
        if ( sess->IsNewMailWatiting() || force_title )
        {
            strcpy( pszTitle, irt );
        }
        else
        {
            sess->bout << "       (---=----=----=----=----=----=----=----=----=----=----=----)\r\n";
            sess->bout << "Title: ";
            inputl( pszTitle, 60 );
        }
    }
}


bool ExternalMessageEditor( int maxli, int &setanon, char *pszTitle, const char *pszDestination, int flags )
{
    char fn1[MAX_PATH], fn2[MAX_PATH];
    sprintf(fn1, "%s%s", syscfgovr.tempdir, FEDIT_INF );
    sprintf(fn2, "%s%s", syscfgovr.tempdir, RESULT_ED );
    WFile::SetFilePermissions( fn1, WFile::permReadWrite );
    WFile::Remove(fn1);
    WFile::SetFilePermissions( fn2, WFile::permReadWrite );
    WFile::Remove(fn2);
    fedit_data_rec fedit_data;
    fedit_data.tlen = 60;
    strcpy( fedit_data.ttl, pszTitle );
    fedit_data.anon = 0;
    FILE *result = fsh_open(fn1, "wb");
    if (result)
    {
        fsh_write(&fedit_data, sizeof(fedit_data), 1, result);
        fsh_close(result);
    }
    bool bSaveMessage = external_edit( INPUT_MSG, syscfgovr.tempdir, sess->thisuser.GetDefaultEditor() - 1,
                                       maxli, pszDestination, pszTitle, flags );
    if ( bSaveMessage )
    {
        if ( ( result = fsh_open( fn2, "rt" ) ) != NULL )
        {
            char szAnonString[ 81 ];
            if ( fgets( szAnonString, 80, result ) )
            {
                char *ss = strchr( szAnonString, '\n' );
                if ( ss )
                {
                    *ss = 0;
                }
                setanon = atoi(szAnonString);
                if (fgets(pszTitle, 80, result))
                {
                    ss = strchr(pszTitle, '\n');
                    if (ss)
                    {
                        *ss = 0;
                    }
                }
            }
            fsh_close( result );
        }
        else if ( ( result = fsh_open( fn1, "rb" ) ) != NULL )
        {
            if ( fsh_read( &fedit_data, sizeof( fedit_data ), 1, result ) == 1 )
            {
                strcpy( pszTitle, fedit_data.ttl );
                setanon = fedit_data.anon;
            }
            fsh_close( result );
        }
    }
    WFile::Remove( fn1 );
    WFile::Remove( fn2 );
    return bSaveMessage;
}


void UpdateMessageBufferTheadsInfo( char *pszMessageBuffer, long *plBufferLength, const char *aux )
{
    if ( !wwiv::stringUtils::IsEqualsIgnoreCase( aux, "email" ) )
    {
        long msgid = time( NULL );
        long targcrc = crc32buf( pszMessageBuffer, strlen( pszMessageBuffer ) );
        char szBuffer[ 255 ];
        sprintf( szBuffer, "0P %lX-%lX", targcrc, msgid );
        AddLineToMessageBuffer( pszMessageBuffer, szBuffer, plBufferLength );
        if ( thread )
        {
            thread [sess->GetNumMessagesInCurrentMessageArea() + 1 ].msg_num = static_cast< unsigned short> ( sess->GetNumMessagesInCurrentMessageArea() + 1 );
            strcpy( thread[sess->GetNumMessagesInCurrentMessageArea() + 1].message_code, &szBuffer[4] );
        }
		if ( sess->threadID.length() > 0 )
        {
			sprintf( szBuffer, "0W %s", sess->threadID.c_str() );
            AddLineToMessageBuffer(pszMessageBuffer, szBuffer, plBufferLength);
            if ( thread )
            {
                strcpy( thread[sess->GetNumMessagesInCurrentMessageArea() + 1].parent_code, &szBuffer[4] );
                thread[ sess->GetNumMessagesInCurrentMessageArea() + 1 ].used = 1;
            }
        }
    }
}


void UpdateMessageBufferInReplyToInfo( char *pszMessageBuffer, long *plBufferLength, const char *aux )
{
    char szBuffer[ 255 ];
    if ( irt_name[0] &&
         !wwiv::stringUtils::IsEqualsIgnoreCase(aux, "email") &&
         xsubs[sess->GetCurrentReadMessageArea()].num_nets)
    {
        for (int i = 0; i < xsubs[sess->GetCurrentReadMessageArea()].num_nets; i++)
        {
	        xtrasubsnetrec *xnp = &xsubs[sess->GetCurrentReadMessageArea()].nets[i];
            if (net_networks[xnp->net_num].type == net_type_fidonet)
            {
                sprintf(szBuffer, "0FidoAddr: %s", irt_name);
                AddLineToMessageBuffer(pszMessageBuffer, szBuffer, plBufferLength);
                break;
            }
        }
    }
    if ((strnicmp("internet", sess->GetNetworkName(), 8) == 0) ||
        (strnicmp("filenet", sess->GetNetworkName(), 7) == 0))
    {
		if (sess->usenetReferencesLine.length() > 0 )
        {
			sprintf(szBuffer, "%c0RReferences: %s", CD, sess->usenetReferencesLine.c_str() );
            AddLineToMessageBuffer(pszMessageBuffer, szBuffer, plBufferLength);
            sess->usenetReferencesLine = "";
        }
    }
    if (irt[0] != '"')
    {
        sprintf(szBuffer, "RE: %s", irt);
        AddLineToMessageBuffer(pszMessageBuffer, szBuffer, plBufferLength);
        if (irt_sub[0])
        {
            sprintf(szBuffer, "ON: %s", irt_sub);
            AddLineToMessageBuffer(pszMessageBuffer, szBuffer, plBufferLength);
        }
    }
    else
    {
        irt_sub[0] = '\0';
    }

    if ( irt_name[0] &&
         !wwiv::stringUtils::IsEqualsIgnoreCase( aux, "email" ) )
    {
        sprintf(szBuffer, "BY: %s", irt_name);
        AddLineToMessageBuffer(pszMessageBuffer, szBuffer, plBufferLength);
    }
    AddLineToMessageBuffer(pszMessageBuffer, "", plBufferLength);
}


void UpdateMessageBufferTagLine( char *pszMessageBuffer, long *plBufferLength, const char *aux )
{
    char szMultiMail[] = "Multi-Mail";
    if ( xsubs[sess->GetCurrentReadMessageArea()].num_nets &&
         !wwiv::stringUtils::IsEqualsIgnoreCase( aux, "email" ) &&
         (! ( subboards[sess->GetCurrentReadMessageArea()].anony & anony_no_tag ) ) &&
         !wwiv::stringUtils::IsEquals( strupr( irt ), strupr( szMultiMail ) ) )
    {
        char szFileName[ MAX_PATH ];
        for (int i = 0; i < xsubs[sess->GetCurrentReadMessageArea()].num_nets; i++)
        {
            xtrasubsnetrec *xnp = &xsubs[sess->GetCurrentReadMessageArea()].nets[i];
            char *nd = net_networks[xnp->net_num].dir;
            sprintf(szFileName, "%s%s.tag", nd, xnp->stype);
            if (WFile::Exists(szFileName))
            {
                break;
            }
            sprintf(szFileName, "%s%s", nd, GENERAL_TAG);
            if (WFile::Exists(szFileName))
            {
                break;
            }
            sprintf(szFileName, "%s%s.tag", syscfg.datadir, xnp->stype);
            if (WFile::Exists(szFileName))
            {
                break;
            }
            sprintf(szFileName, "%s%s", syscfg.datadir, GENERAL_TAG);
            if (WFile::Exists(szFileName))
            {
                break;
            }
        }
        FILE *result = fsh_open(szFileName, "rb");
        if (result)
        {
            int j = 0;
            while (!feof(result))
            {
                char s[ 181 ];
                s[0] = '\0';
                char s1[ 181 ];
                s1[0] = '\0';
                fgets(s, 180, result);
                if (strlen(s) > 1)
                {
                    if (s[strlen(s) - 2] == RETURN)
                    {
                        s[strlen(s) - 2] = '\0';
                    }
                }
                if ( s[0] != CD )
                {
                    sprintf( s1, "%c%c%s", CD, j + '2', s );
                }
                else
                {
                    strncpy(s1, s, sizeof(s1));
                }
                if (!j)
                {
                    AddLineToMessageBuffer(pszMessageBuffer, "1", plBufferLength);
                }
                AddLineToMessageBuffer( pszMessageBuffer, s1, plBufferLength);
                if (j < 7)
                {
                    j++;
                }
            }
            fsh_close(result);
        }
    }
}


void UpdateMessageBufferQuotesCtrlLines( char *pszMessageBuffer, long *plBufferLength )
{
    char szQuotesFileName[ MAX_PATH ];
    sprintf( szQuotesFileName, "%s%s", syscfgovr.tempdir, QUOTES_TXT );
    FILE *q_fp = fsh_open( szQuotesFileName, "rt" );
    char q_txt[ 255 ];
    if (q_fp)
    {
        while (fgets(q_txt, sizeof(q_txt) - 1, q_fp))
        {
            char *ss1 = strchr(q_txt, '\n');
            if (ss1)
            {
                *ss1 = '\0';
            }
            if (strncmp(q_txt, "\004""0U", 3) == 0)
            {
                AddLineToMessageBuffer( pszMessageBuffer, q_txt, plBufferLength );
            }
        }
        fsh_close(q_fp);
    }

    // was q_txt but that could be bad, I think this was broken in 4.3x
    char szMsgInfFileName[ MAX_PATH ];
    sprintf( szMsgInfFileName, "%smsginf", syscfgovr.tempdir );
    copyfile( szQuotesFileName, szMsgInfFileName, false );

}

void GetMessageAnonStatus( bool &real_name, int *anony, int setanon )
{
    // Changed *anony to anony
    switch (*anony)
    {
    case 0:
        *anony = 0;
        break;
    case anony_enable_anony:
        if (setanon)
        {
            if (setanon == 1)
            {
                *anony = anony_sender;
            }
            else
            {
                *anony = 0;
            }
        }
        else
        {
            sess->bout << "|#5Anonymous? ";
            if (yesno())
            {
                *anony = anony_sender;
            }
            else
            {
                *anony = 0;
            }
        }
        break;
    case anony_enable_dear_abby:
        {
            nl();
			sess->bout << "1. " << sess->thisuser.GetUserNameAndNumber( sess->usernum ) << wwiv::endl;
            sess->bout << "2. Abby\r\n";
            sess->bout << "3. Problemed Person\r\n\n";
            sess->bout << "|#5Which? ";
            char chx = onek( "\r123" );
            switch ( chx )
            {
            case '\r':
            case '1':
                *anony = 0;
                break;
            case '2':
                *anony = anony_sender_da;
                break;
            case '3':
                *anony = anony_sender_pp;
            }
        }
        break;
    case anony_force_anony:
        *anony = anony_sender;
        break;
    case anony_real_name:
        real_name = true;
        *anony = 0;
        break;
    }
}
