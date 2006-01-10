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


void SetupThreadRecordsBeforeScan();
void HandleScanReadPrompt( int &nMessageNumber, int &nScanOptionType, int *nextsub, bool &bTitleScan, bool &done, bool &quit, int &val );
void GetScanReadPrompts(  int nMessageNumber, char *pszReadPrompt, char *szSubNamePrompt );
void HandleScanReadAutoReply( int &nMessageNumber, const char *pszUserInput, int &nScanOptionType );
void HandleScanReadFind( int &nMessageNumber, int &nScanOptionType );
void HandleListTitles( int &nMessageNumber, int &nScanOptionType );
void HandleMessageDownload( int nMessageNumber );
void HandleMessageMove( int &nMessageNumber );
void HandleMessageLoad();
void HandleMessageReply( int &nMessageNumber );
void HandleMessageDelete( int &nMessageNumber );
void HandleMessageExtract( int &nMessageNumber );
void HandleMessageHelp();
void HandleListReplies( int nMessageNumber );


static char s_szFindString[21];


void scan( int nMessageNumber, int nScanOptionType, int *nextsub, bool bTitleScan )
{
	int j, k;

	irt[0] = '\0';
	irt_name[0] = '\0';

	int val = 0;
	bool realexpress = express;
	iscan( sess->GetCurrentMessageArea() );
	if ( sess->GetCurrentReadMessageArea() < 0 )
	{
		nl();
		sess->bout << "No subs available.\r\n\n";
		return;
	}
	if ( sess->IsMessageThreadingEnabled() )
	{
		SetupThreadRecordsBeforeScan();
	}

	bool done = false;
	bool quit = false;
	do
	{
		if ( sess->IsMessageThreadingEnabled() )
		{
			for ( j = 0; j <= sess->GetNumMessagesInCurrentMessageArea(); j++ )
			{
				for ( k = 0; k <= sess->GetNumMessagesInCurrentMessageArea(); k++ )
				{
					if ( wwiv::stringUtils::IsEquals( thread[j].parent_code, thread[k].message_code ) )
					{
						thread[j].parent_num = thread[k].msg_num;
						k = sess->GetNumMessagesInCurrentMessageArea() + 1;
					}
				}
			}
		}
		app->localIO->tleft( true );
		CheckForHangup();
		set_net_num( (xsubs[sess->GetCurrentReadMessageArea()].num_nets) ? xsubs[sess->GetCurrentReadMessageArea()].nets[0].net_num : 0 );
		if ( nScanOptionType != SCAN_OPTION_READ_PROMPT )
		{
			resynch( sess->GetCurrentMessageArea(), &nMessageNumber, NULL );
		}
		write_inst( INST_LOC_SUBS, usub[sess->GetCurrentMessageArea()].subnum, INST_FLAGS_NONE );

		switch ( nScanOptionType )
		{
		case SCAN_OPTION_READ_PROMPT:   // Read Prompt
		    {
				HandleScanReadPrompt( nMessageNumber, nScanOptionType, nextsub, bTitleScan, done, quit, val );
			}
			break;
		case SCAN_OPTION_LIST_TITLES:   // List Titles
			{
				HandleListTitles( nMessageNumber, nScanOptionType );
			}
			break;
		case SCAN_OPTION_READ_MESSAGE: // Read Message
			{
				bool next = false;
				if ( nMessageNumber > 0 && nMessageNumber <= sess->GetNumMessagesInCurrentMessageArea() )
				{
					if ( forcescansub )
					{
						incom = false;
					}
					read_message( nMessageNumber, &next, &val );
					if ( forcescansub )
					{
						incom = true;
					}
				}
				ansic( 0 );
				nl();
				if ( sess->IsMessageThreadingEnabled() )
				{
					if (thread[nMessageNumber].used)
					{
						sess->bout << "|#9Current Message is a reply to Msg #|#2" << thread[nMessageNumber].parent_num << wwiv::endl;
						//temp isn't used anywhere else here, not sure why this code is here.
						//strcpy(temp, thread[nMessageNumber].message_code);
					}
					k = 0;
					for ( j = 0; j <= sess->GetNumMessagesInCurrentMessageArea(); j++ )
					{
						if ( wwiv::stringUtils::IsEquals( thread[j].parent_code, thread[nMessageNumber].message_code ) &&
                             j != nMessageNumber )
						{
							k++;
						}
					}
					if (k)
					{
						sess->bout << "|#9Current Message has |12" << k << "|#9";
						if (k == 1)
						{
							sess->bout << "reply.";
						}
						else
						{
							sess->bout << "replies.";
						}
					}
					sess->bout << wwiv::endl;;
				}
				if ( next )
				{
					++nMessageNumber;
					if ( nMessageNumber > sess->GetNumMessagesInCurrentMessageArea() )
					{
						done = true;
					}
					nScanOptionType = SCAN_OPTION_READ_MESSAGE;
				}
				else
				{
					nScanOptionType = SCAN_OPTION_READ_PROMPT;
				}
				if ( expressabort )
				{
					if ( realexpress )
					{
						done = true;
						quit = true;
						*nextsub = 0;
					}
					else
					{
						expressabort = false;
						express = false;
						nScanOptionType = SCAN_OPTION_READ_PROMPT;
					}
				}
			}
			break;
		}
	} while ( !done && !hangup );
	if (!realexpress)
	{
		express = false;
		expressabort = false;
	}
	if ( ( val & 1 ) && lcs() && !express )
	{
		nl();
		sess->bout << "|#5Validate messages here? ";
		if (noyes())
		{
			open_sub( true );
			for (int i = 1; i <= sess->GetNumMessagesInCurrentMessageArea(); i++)
			{
				postrec* p3 = get_post(i);
				if (p3->status & (status_unvalidated | status_delete))
				{
					p3->status &= (~(status_unvalidated | status_delete));
				}
				write_post(i, p3);
			}
			close_sub();
		}
	}
	if (  (val & 2 ) && lcs() && !express )
	{
		nl();
		sess->bout << "|#5Network validate here? ";
		if (yesno())
		{
			int nNumMsgsSent = 0;
			open_sub( true );
			int nMsgToValidate = 0;
			for (int i = 1; i <= sess->GetNumMessagesInCurrentMessageArea(); i++)
			{
				if (get_post(i)->status & status_pending_net)
				{
					nMsgToValidate++;
				}
			}
			postrec* p3 = static_cast<postrec *>( BbsAllocA( nMsgToValidate * sizeof( postrec ) ) );
			if (p3)
			{
				nMsgToValidate = 0;
				for (int i = 1; i <= sess->GetNumMessagesInCurrentMessageArea(); i++)
				{
					postrec *p4 = get_post(i);
					if (p4->status & status_pending_net)
					{
						p3[nMsgToValidate++] = *p4;
						p4->status &= ~status_pending_net;
						write_post(i, p4);
					}
				}

				close_sub();

				for (int j = 0; j < nMsgToValidate; j++)
				{
					send_net_post(p3 + j, subboards[sess->GetCurrentReadMessageArea()].filename, sess->GetCurrentReadMessageArea());
					nNumMsgsSent++;
				}

				BbsFreeMemory(p3);
			}
			else
			{
				close_sub();
			}

			nl();
			sess->bout << nNumMsgsSent << " messages sent.";
			nl( 2 );
		}
	}
	if ( !quit && !express )
	{
		nl();
		if ( !sess->thisuser.isRestrictionPost() &&
			 ( sess->thisuser.GetNumPostsToday() < getslrec( sess->GetEffectiveSl() ).posts ) &&
			 ( sess->GetEffectiveSl() >= subboards[sess->GetCurrentReadMessageArea()].postsl ) )
		{
			sess->bout << "|#5Post on " << subboards[sess->GetCurrentReadMessageArea()].name << "? ";
			irt[0] = '\0';
			irt_name[0] = '\0';
			grab_quotes(NULL, NULL);
			if ( yesno() )
			{
				post();
			}
		}
	}
	nl();
	if ( thread )
	{
		BbsFreeMemory( thread );
	}
	thread = NULL;
}


void SetupThreadRecordsBeforeScan()
{
	if (thread)
	{
		BbsFreeMemory(thread);
	}
	thread = NULL;
	thread = static_cast<threadrec *>( BbsAllocWithComment( ( sess->GetNumMessagesInCurrentMessageArea() + 1 ) * sizeof( threadrec ), "Thread Record" ) );
	WWIV_ASSERT(thread != NULL);

	for (unsigned short tempnum = 1; tempnum <= sess->GetNumMessagesInCurrentMessageArea(); tempnum++) // was 0.
	{
		strcpy(thread[tempnum].parent_code, "");
		strcpy(thread[tempnum].message_code, "");
		thread[tempnum].msg_num = tempnum;
		thread[tempnum].parent_num = 0;
		thread[tempnum].used = 0;

		postrec *pPostRec1 = get_post(tempnum);
		long lFileLen;
		char *b = readfile(&(pPostRec1->msg), (subboards[sess->GetCurrentReadMessageArea()].filename), &lFileLen);
		for (long l1 = 0; l1 < lFileLen; l1++)
		{
			if ( b[l1] == 4 && b[l1 + 1] == '0' && b[l1 + 2] == 'P' )
			{
				l1 += 4;
				strcpy(thread[tempnum].message_code, "");
				char szTemp[ 81 ];
				szTemp[0] = '\0';
				while ((b[l1] != '\r') && (l1 < lFileLen))
				{
					sprintf(szTemp, "%c", b[l1]);
					strcat( thread[tempnum].message_code, szTemp );
					l1++;
				}
				l1 = lFileLen;
				thread[tempnum].msg_num = tempnum;
			}
		}
		for (long l2 = 0; l2 < lFileLen; l2++)
		{
			if ((b[l2] == 4) && (b[l2 + 1] == '0') && (b[l2 + 2] == 'W'))
			{
				l2 += 4;
				strcpy(thread[tempnum].parent_code, "");
				char szTemp[ 81 ];
				szTemp[0] = '\0';
				while ((b[l2] != '\r') && (l2 < lFileLen))
				{
					sprintf(szTemp, "%c", b[l2]);
					strcat(thread[tempnum].parent_code, szTemp);
					l2++;
				}
				l2 = lFileLen;
				thread[tempnum].msg_num = tempnum;
				thread[tempnum].used = 1;
			}
		}
		BbsFreeMemory( b );
	}
}


void HandleScanReadPrompt( int &nMessageNumber, int &nScanOptionType, int *nextsub, bool &bTitleScan, bool &done, bool &quit, int &val  )
{
	bool bFollowThread = false;
	char szReadPrompt[ 255 ];
	char szSubNamePrompt[81];
	sess->threadID = "";
	resetnsp();
	GetScanReadPrompts( nMessageNumber, szReadPrompt, szSubNamePrompt );
	nl();
	char szUserInput[ 81 ];
	if (express)
	{
		szReadPrompt[0]		= '\0';
		szSubNamePrompt[0]	= '\0';
		nl( 2 );
	}
	else
	{
		sess->bout << szReadPrompt;
		input( szUserInput, 5, true );
		resynch( sess->GetCurrentMessageArea(), &nMessageNumber, NULL );
		while (szUserInput[0] == 32)
		{
			char szTempBuffer[ 255 ];
			strcpy(szTempBuffer, &(szUserInput[1]));
			strcpy(szUserInput, szTempBuffer);
		}
	}
	if ( bTitleScan && szUserInput[0] == 0 && nMessageNumber < sess->GetNumMessagesInCurrentMessageArea() )
	{
		nScanOptionType = SCAN_OPTION_LIST_TITLES;
		szUserInput[0] = 'T';
		szUserInput[1] = '\0';
	}
	else
	{
		bTitleScan = false;
		nScanOptionType = SCAN_OPTION_READ_PROMPT;
	}
	int nUserInput = atoi(szUserInput);
	if (szUserInput[0] == 0)
	{
		bFollowThread = false;
		nUserInput = nMessageNumber + 1;
		if (nUserInput >= sess->GetNumMessagesInCurrentMessageArea() + 1)
		{
			done = true;
		}
	}

	if ( nUserInput != 0 && nUserInput <= sess->GetNumMessagesInCurrentMessageArea() && nUserInput >= 1 )
	{
		nScanOptionType = SCAN_OPTION_READ_MESSAGE;
		nMessageNumber = nUserInput;
	}
	else if (szUserInput[1] == 0)
	{
		if (forcescansub)
		{
			return;
		}
		switch (szUserInput[0])
		{
        case '$':   // Last message in area.
            {
                nScanOptionType = SCAN_OPTION_READ_MESSAGE;
                nMessageNumber = sess->GetNumMessagesInCurrentMessageArea();
            }
            break;
		case 'F':   // Find addition
			{
				HandleScanReadFind( nMessageNumber, nScanOptionType );
			}
			break;
			// End of find addition

		case 'Q':
			quit = true;
			done = true;
			*nextsub = 0;
			break;
		case 'B':
			if (*nextsub != 0)
			{
				nl();
				sess->bout << "|#5Remove this sub from your Q-Scan? ";
				if (yesno())
				{
					qsc_q[usub[sess->GetCurrentMessageArea()].subnum / 32] ^= (1L << (usub[sess->GetCurrentMessageArea()].subnum % 32));
				}
				else
				{
					nl();
					sess->bout << "|#9Mark messages in "<< subboards[usub[sess->GetCurrentMessageArea()].subnum].name << " as read? ";
					if (yesno())
					{
						qsc_p[usub[sess->GetCurrentMessageArea()].subnum] = status.qscanptr - 1L;
					}
				}

				*nextsub = 1;
				done = true;
				quit = true;
			}
			break;
		case 'T':
			bTitleScan = true;
			nScanOptionType = SCAN_OPTION_LIST_TITLES;
			break;
		case 'R':
			nScanOptionType = SCAN_OPTION_READ_MESSAGE;
			break;
		case '@':
			strcpy(irt_sub, subboards[usub[sess->GetCurrentMessageArea()].subnum].name);
		case 'O':
		case 'A':
			{
				HandleScanReadAutoReply( nMessageNumber, szUserInput, nScanOptionType );
			}
			break;
		case 'P':
			irt[0]          = '\0';
			irt_name[0]     = '\0';
			sess->threadID = "";
		case 'W':
			HandleMessageReply( nMessageNumber );
			break;
		case 'Y':
			HandleMessageDownload( nMessageNumber );
			break;
		case '?':
			HandleMessageHelp();
			break;
		case '-':
			if ( nMessageNumber > 1 && ( nMessageNumber - 1 < sess->GetNumMessagesInCurrentMessageArea() ) )
			{
				--nMessageNumber;
				nScanOptionType = SCAN_OPTION_READ_MESSAGE;
			}
			break;
		case 'C':
			express = true;
			break;
		case '*':
			HandleListReplies( nMessageNumber );
			break;
		case '[':
			if ( sess->IsMessageThreadingEnabled() )
			{
				if (forcescansub)
				{
					printfile(MUSTREAD_NOEXT);
				}
				else
				{
					nMessageNumber = thread[nMessageNumber].parent_num;
					nScanOptionType = SCAN_OPTION_READ_MESSAGE;
				}
			}
			break;
		case ']':
			if ( sess->IsMessageThreadingEnabled() )
			{
				if (forcescansub)
				{
					printfile(MUSTREAD_NOEXT);
				}
				else
				{
					for ( int j = nMessageNumber; j <= sess->GetNumMessagesInCurrentMessageArea(); j++ )
					{
						if ( wwiv::stringUtils::IsEquals( thread[j].parent_code, thread[nMessageNumber].message_code ) &&
                             j != nMessageNumber )
						{
							nMessageNumber = j;
							j = sess->GetNumMessagesInCurrentMessageArea();
							nScanOptionType = SCAN_OPTION_READ_MESSAGE;
						}
					}
				}
			}
			break;
		case '>':
			if ( sess->IsMessageThreadingEnabled() )
			{
				if (forcescansub)
				{
					printfile(MUSTREAD_NOEXT);
				}
				else
				{
					int original = 0;
					if (!bFollowThread)
					{
						bFollowThread = true;
						original = nMessageNumber;
					}
					int j = 0;
					for (j = nMessageNumber; j <= sess->GetNumMessagesInCurrentMessageArea(); j++)
					{
						if ( wwiv::stringUtils::IsEquals( thread[j].parent_code, thread[original].message_code ) &&
                             j != nMessageNumber )
						{
							nMessageNumber = j;
							break;
						}
					}
					if (j >= sess->GetNumMessagesInCurrentMessageArea())
					{
						nMessageNumber = original;
						bFollowThread = false;
					}
				}
			}
			nScanOptionType = SCAN_OPTION_READ_MESSAGE;
			break;

		case 'V':
			if ( cs() && ( get_post( nMessageNumber )->ownersys == 0 ) && ( nMessageNumber > 0 ) && ( nMessageNumber <= sess->GetNumMessagesInCurrentMessageArea() ) )
			{
				valuser( get_post( nMessageNumber )->owneruser );
			}
			else if ( cs() && ( nMessageNumber > 0 ) && ( nMessageNumber <= sess->GetNumMessagesInCurrentMessageArea() ) )
			{
				nl();
				sess->bout << "|12Post from another system.\r\n\n";
			}
			break;
		case 'N':
			if ((lcs()) && (nMessageNumber > 0) && (nMessageNumber <= sess->GetNumMessagesInCurrentMessageArea()))
			{
				open_sub( true );
				resynch(sess->GetCurrentMessageArea(), &nMessageNumber, NULL);
				postrec *p3 = get_post(nMessageNumber);
				p3->status ^= status_no_delete;
				write_post(nMessageNumber, p3);
				close_sub();
				nl();
				if (p3->status & status_no_delete)
				{
					sess->bout << "|#9Message will |12NOT|#9 be auto-purged.\r\n";
				}
				else
				{
					sess->bout << "|#9Message |#2CAN|#9 now be auto-purged.\r\n";
				}
				nl();
			}
			break;
		case 'X':
			if ((lcs()) && (nMessageNumber > 0) && (nMessageNumber <= sess->GetNumMessagesInCurrentMessageArea()) &&
				(subboards[sess->GetCurrentReadMessageArea()].anony & anony_val_net) &&
				(xsubs[sess->GetCurrentReadMessageArea()].num_nets))
			{
				open_sub( true );
				resynch(sess->GetCurrentMessageArea(), &nMessageNumber, NULL);
				postrec *p3 = get_post(nMessageNumber);
				p3->status ^= status_pending_net;
				write_post(nMessageNumber, p3);
				close_sub();
				nl();
				if (p3->status & status_pending_net)
				{
					val |= 2;
					sess->bout << "|#9Will be sent out on net now.\r\n";
				}
				else
				{
					sess->bout << "|#9Not set for net pending now.\r\n";
				}
				nl();
			}
			break;
		case 'U':
			if ((lcs()) && (nMessageNumber > 0) && (nMessageNumber <= sess->GetNumMessagesInCurrentMessageArea()))
			{
				open_sub( true );
				resynch(sess->GetCurrentMessageArea(), &nMessageNumber, NULL);
				postrec *p3 = get_post(nMessageNumber);
				p3->anony = 0;
				write_post(nMessageNumber, p3);
				close_sub();
				nl();
				sess->bout << "|#9Message is not anonymous now.\r\n";
			}
			break;
		case 'D':
			HandleMessageDelete( nMessageNumber );
			break;
		case 'E':
			HandleMessageExtract( nMessageNumber );
			break;
		case 'M':
			HandleMessageMove( nMessageNumber );
			break;
		case 'L':
			HandleMessageLoad();
			break;
		}
	}
	else if ( wwiv::stringUtils::IsEquals( szUserInput, "ClearScreen" ) )
	{
		bputch('\x0c');
	}
}


void GetScanReadPrompts( int nMessageNumber, char *pszReadPrompt, char *pszSubNamePrompt )
{
    // TODO remove pszSubNamePrompt is decided to go to 1 line format.
	char szLocalNetworkName[81];
	if (forcescansub)
	{
		if ( nMessageNumber < sess->GetNumMessagesInCurrentMessageArea() )
		{
			strcpy(pszReadPrompt, "|#1Press |#7[|#2ENTER|#7]|#1 to go to the next message...");
		}
		else
		{
			sprintf(pszReadPrompt, "|#1Press |#7[|#2ENTER|#7]|#1 to continue...");
		}
	}
	else
	{
		if ( xsubs[sess->GetCurrentReadMessageArea()].num_nets )
		{
			sprintf( szLocalNetworkName, "%s", sess->GetNetworkName() );
		}
		else
		{
			set_net_num( 0 );
			sprintf(szLocalNetworkName, "%s", "Local");
		}
		sprintf(pszSubNamePrompt, "|#7[|#1%s|#7] [|#2%s|#7]", szLocalNetworkName, subboards[sess->GetCurrentReadMessageArea()].name);
		char szTemp[81];
		if ( sess->GetNumMessagesInCurrentMessageArea()> nMessageNumber )
		{
			sprintf( szTemp, "%d", nMessageNumber + 1 );
		}
		else
		{
			sprintf(szTemp, "\bExit Sub");
		}
        //sprintf(pszReadPrompt, "|#7(|#1Read |#21-%lu|#1, |#7[|#2ENTER|#7]|#1 = #|#2%s|#7) :|#0 ", sess->GetNumMessagesInCurrentMessageArea(), szTemp);
        sprintf(pszReadPrompt, "%s |#7(|#1Read |#2%lu |#1of |#2%lu|#1|#7) : ", pszSubNamePrompt, nMessageNumber, sess->GetNumMessagesInCurrentMessageArea() );
	}
}


void HandleScanReadAutoReply( int &nMessageNumber, const char *pszUserInput, int &nScanOptionType )
{
	if ( !lcs() && get_post( nMessageNumber )->status & (status_unvalidated | status_delete) )
	{
		return;
	}
	if ( get_post( nMessageNumber )->ownersys && !get_post( nMessageNumber )->owneruser )
	{
		grab_user_name(&(get_post( nMessageNumber )->msg), subboards[sess->GetCurrentReadMessageArea()].filename);
	}
	grab_quotes( &(get_post( nMessageNumber )->msg), subboards[sess->GetCurrentReadMessageArea()].filename );

    if ( okfsed() && sess->thisuser.isUseAutoQuote() && nMessageNumber > 0 &&
         nMessageNumber <= sess->GetNumMessagesInCurrentMessageArea() && pszUserInput[0] != 'O' )
	{
		long lMessageLen;
		char *b=readfile(&( get_post( nMessageNumber )->msg ),
			(subboards[sess->GetCurrentReadMessageArea()].filename),&lMessageLen);
		if (pszUserInput[0] == '@')
		{
			auto_quote(b, lMessageLen, 1, get_post( nMessageNumber )->daten);
		}
		else
		{
			auto_quote(b, lMessageLen, 3, get_post( nMessageNumber )->daten);
		}
	}

	if (get_post( nMessageNumber )->status & status_post_new_net)
	{
		set_net_num( get_post( nMessageNumber )->title[80] );
		if (get_post( nMessageNumber )->title[80] == -1)
		{
			sess->bout << "|12Deleted network.\r\n";
			return;
		}
	}

	if ( pszUserInput[0] == 'O' && ( so() || lcs() ) )
	{
		irt_sub[0] = 0;
		show_files("*.FRM", syscfg.gfilesdir);
		sess->bout << "|#2Which form letter: ";
		char szFileName[ MAX_PATH ];
		input( szFileName, 8, true );
		if (!szFileName[0])
		{
			return;
		}
		char szFullPathName[ MAX_PATH ];
		sprintf(szFullPathName, "%s%s.frm", syscfg.gfilesdir, szFileName);
		if (!WFile::Exists(szFullPathName))
		{
			sprintf(szFullPathName, "%sform%s.msg", syscfg.gfilesdir, szFileName);
		}
		if (WFile::Exists(szFullPathName))
		{
			LoadFileIntoWorkspace( szFullPathName, true );
			email(get_post( nMessageNumber )->owneruser, get_post( nMessageNumber )->ownersys, false, get_post( nMessageNumber )->anony);
			grab_quotes(NULL, NULL);
		}
	}
	else if ( pszUserInput[0] == '@' )
	{
		nl();
		sess->bout << "|#21|#7]|#9 Reply to Different Address\r\n";
		sess->bout << "|#22|#7]|#9 Forward Message\r\n";
		nl();
		sess->bout << "|#7(Q=Quit) Select [1,2] : ";
		char chReply = onek( "Q12\r", true );
		switch ( chReply )
		{
		case '\r':
		case 'Q':
			nScanOptionType = SCAN_OPTION_READ_MESSAGE;
			break;
		case '1':
			{
				nl();
				sess->bout << "|#9Enter user name or number:\r\n:";
				char szUserNameOrNumber[ 81 ];
				input( szUserNameOrNumber, 75, true );
				size_t nAtPos = strcspn(szUserNameOrNumber, "@");
				if ( nAtPos != strlen(szUserNameOrNumber) && isalpha( szUserNameOrNumber[ nAtPos + 1 ] ) )
				{
					if (strstr(szUserNameOrNumber, "@32767") == NULL)
					{
						strlwr(szUserNameOrNumber);
						strcat(szUserNameOrNumber, " @32767");
					}
				}
				int nUserNumber, nSystemNumber;
				parse_email_info(szUserNameOrNumber, &nUserNumber, &nSystemNumber);
				if (nUserNumber || nSystemNumber)
				{
					email(nUserNumber, nSystemNumber, false, 0);
				}
				nScanOptionType = SCAN_OPTION_READ_MESSAGE;
			}
			break;
		case '2':
			{
				if ( nMessageNumber > 0 && nMessageNumber <= sess->GetNumMessagesInCurrentMessageArea() )
				{
					long lMessageLen;
					char *b = readfile(&(get_post( nMessageNumber )->msg), (subboards[sess->GetCurrentReadMessageArea()].filename), &lMessageLen);
					char szFileName[ MAX_PATH ];
					strcpy(szFileName, "EXTRACT.TMP");
					if (WFile::Exists(szFileName))
					{
						WFile::Remove(szFileName);
					}
					WFile fileExtract( szFileName );
					if ( !fileExtract.Open( WFile::modeBinary|WFile::modeCreateFile|WFile::modeReadWrite, WFile::shareUnknown, WFile::permReadWrite ) )
					{
						sess->bout << "|12Could not open file for writing.\r\n";
					}
					else
					{
						if ( fileExtract.GetLength() > 0 )
						{
							fileExtract.Seek( -1L, WFile::seekEnd );
							char chLastChar = CZ;
							fileExtract.Read( &chLastChar, 1 );
							if (chLastChar == CZ)
							{
								fileExtract.Seek( -1L, WFile::seekEnd );
							}
						}
						char szBuffer[ 255 ];
						sprintf( szBuffer, "ON: %s", subboards[sess->GetCurrentReadMessageArea()].name );
						fileExtract.Write( szBuffer, strlen( szBuffer ) );
						fileExtract.Write( const_cast<char *>("\r\n\r\n"), 4 );
						fileExtract.Write( get_post( nMessageNumber )->title, strlen(get_post( nMessageNumber )->title) );
						fileExtract.Write( const_cast<char *>("\r\n"), 2 );
						fileExtract.Write( b, lMessageLen );
						fileExtract.Close();
					}
					nl();
					sess->bout << "|#5Allow editing? ";
					if (yesno())
					{
						nl();
						LoadFileIntoWorkspace( szFileName, false );
					}
					else
					{
						nl();
						LoadFileIntoWorkspace( szFileName, true );
					}
					send_email();
					sprintf(szFileName, "%s%s", syscfgovr.tempdir, INPUT_MSG);
					if ( WFile::Exists(szFileName) )
					{
						WFile::Remove(szFileName);
					}
				}
				nScanOptionType = SCAN_OPTION_READ_MESSAGE;
			}
			break;
		}
	}
	else
	{
		if ( lcs() || ( getslrec(sess->GetEffectiveSl()).ability & ability_read_post_anony ) || get_post( nMessageNumber )->anony == 0 )
		{
			email( get_post( nMessageNumber )->owneruser, get_post( nMessageNumber )->ownersys, false, 0 );
		}
		else
		{
			email( get_post( nMessageNumber )->owneruser, get_post( nMessageNumber )->ownersys, false, get_post( nMessageNumber )->anony );
		}
		irt_sub[0] = 0;
		grab_quotes(NULL, NULL);
	}
}


void HandleScanReadFind( int &nMessageNumber, int &nScanOptionType )
{
	bool abort = false;
	char *pszTempFindString = NULL;
	if (!(g_flags & g_flag_made_find_str))
	{
		pszTempFindString = strupr( stripcolors( get_post( nMessageNumber )->title ) );
		strncpy(s_szFindString, pszTempFindString, sizeof(s_szFindString) - 1);
		g_flags |= g_flag_made_find_str;
	}
	else
	{
		pszTempFindString = &s_szFindString[0];
	}
	while (strncmp(pszTempFindString, "RE:", 3) == 0 || *pszTempFindString == ' ')
	{
		if (*pszTempFindString == ' ')
		{
			pszTempFindString++;
		}
		else
		{
			pszTempFindString += 3;
		}
	}
	if (strlen(pszTempFindString) >= 20)
	{
		pszTempFindString[20] = 0;
	}
	strncpy(s_szFindString, pszTempFindString, sizeof(s_szFindString) - 1);
	nl();
	sess->bout << "|#7Find what? (CR=\"" << pszTempFindString << "\")|#1";
	char szFindString[ 81 ];
	input(szFindString, 20);
	if (!*szFindString)
	{
		strncpy(szFindString, pszTempFindString, sizeof(szFindString) - 1);
	}
	else
	{
		strncpy(s_szFindString, szFindString, sizeof(s_szFindString) - 1);
	}
	nl();
	sess->bout << "|#1Backwards or Forwards? ";
	char szBuffer[ 10 ];
	szBuffer[0] = 'Q';
	szBuffer[1] = upcase(*"Backwards");
	szBuffer[2] = upcase(*"Forwards");
	strcpy(&szBuffer[3], "+-");
	char ch = onek( szBuffer );
	if (ch == 'Q')
	{
		return;
	}
	bool fnd = false;
	int nTempMsgNum = nMessageNumber;
	nl();
	sess->bout << "|#1Searching -> |#2";

	// Store search direction and limit
	bool bSearchForwards = true;
	int nMsgNumLimit;
	if (ch == '-' || ch == upcase(*"Backwards"))
	{
		bSearchForwards = false;
		nMsgNumLimit = 1;
	}
	else
	{
		bSearchForwards = true;
		nMsgNumLimit = sess->GetNumMessagesInCurrentMessageArea();
	}

	while ( nTempMsgNum != nMsgNumLimit && !abort && !hangup && !fnd )
	{
		if (bSearchForwards)
		{
			nTempMsgNum++;
		}
		else
		{
			nTempMsgNum--;
		}
		bool next = true;
		checka(&abort, &next);
		if (!(nTempMsgNum % 5))
		{
			bprintf("%5.5d", nTempMsgNum);
			for (int i1 = 0; i1 < 5; i1++)
			{
				sess->bout << "\b";
			}
			if (!(nTempMsgNum % 100))
			{
				app->localIO->tleft( true );
				CheckForHangup();
			}
		}
		long lMessageLen;
		char *b = readfile(&(get_post(nTempMsgNum)->msg), subboards[sess->GetCurrentReadMessageArea()].filename, &lMessageLen);
		if (b)
		{
			b = strupr(b);
			fnd = (strstr(strupr(stripcolors(get_post(nTempMsgNum)->title)), szFindString) || strstr(b, szFindString)) ? true : false;
			BbsFreeMemory(b);
		}
	}
	if (fnd)
	{
		sess->bout << "Found!\r\n";
		nMessageNumber = nTempMsgNum;
		nScanOptionType = SCAN_OPTION_READ_MESSAGE;
	}
	else
	{
		sess->bout << "|#6Not found!\r\n";
		nScanOptionType = SCAN_OPTION_READ_PROMPT;
	}
}


void HandleListTitles( int &nMessageNumber, int &nScanOptionType )
{
	int i = 0;
	bool abort = false;
	if (nMessageNumber >= sess->GetNumMessagesInCurrentMessageArea())
	{
		abort = true;
	}
	else
	{
		nl();
	}
	int nNumTitleLines = std::max<int>( sess->screenlinest - 6, 1 );
	while ( !abort && !hangup && ++i <= nNumTitleLines )
	{
		++nMessageNumber;
		postrec *p3 = get_post(nMessageNumber);

		char szPrompt[ 255 ];
		char szTempBuffer[ 255 ];
		if ( p3->ownersys == 0 && p3->owneruser == sess->usernum )
		{
			sprintf(szTempBuffer, "|#7[|#1%d|#7]", nMessageNumber);
		}
		else if ( p3->ownersys != 0 )
		{
			sprintf(szTempBuffer, "|#7<|#1%d|#7>", nMessageNumber);
		}
		else
		{
			sprintf(szTempBuffer, "|#7(|#1%d|#7)", nMessageNumber);
		}
		for (int i1 = 0; i1 < 7; i1++)
		{
			szPrompt[i1] = SPACE;
		}
		if (p3->qscan > qsc_p[sess->GetCurrentReadMessageArea()])
		{
			szPrompt[0] = '*';
		}
		if (p3->status & (status_pending_net | status_unvalidated))
		{
			szPrompt[0] = '+';
		}
		strcpy(&szPrompt[9 - strlen(stripcolors(szTempBuffer))], szTempBuffer);
		strcat(szPrompt, "|#1 ");
		if ((get_post(nMessageNumber)->status & (status_unvalidated | status_delete)) && (!lcs()))
		{
			strcat(szPrompt, "<<< NOT VALIDATED YET >>>");
		}
		else
		{
			strncat(szPrompt, stripcolors(get_post(nMessageNumber)->title), 60);
		}

		if ( sess->thisuser.GetScreenChars() >= 80 )
		{
			if (strlen(stripcolors(szPrompt)) > 50)
			{
				while (strlen(stripcolors(szPrompt)) > 50)
				{
					szPrompt[strlen(szPrompt) - 1] = 0;
				}
			}
			strcat(szPrompt, charstr(51 - strlen(stripcolors(szPrompt)), ' '));
			if ( okansi() )
			{
				strcat(szPrompt, "|#7�|#1");
			}
			else
			{
				strcat(szPrompt, "|");
			}
			strcat(szPrompt, " ");
			if ((p3->anony & 0x0f) &&
				((getslrec(sess->GetEffectiveSl()).ability & ability_read_post_anony) == 0))
			{
				strcat(szPrompt, ">UNKNOWN<");
			}
			else
			{
				long lMessageLen;
				char *b = readfile(&(p3->msg), (subboards[sess->GetCurrentReadMessageArea()].filename), &lMessageLen);
				if (b)
				{
					strncpy(szTempBuffer, b, sizeof(szTempBuffer) - 1);
					szTempBuffer[sizeof(szTempBuffer) - 1] = 0;
					strcpy(b, stripcolors(szTempBuffer));
					long lTempBufferPos = 0;
					strcpy(szTempBuffer, "");
					while ( b[lTempBufferPos] != RETURN && b[lTempBufferPos] && lTempBufferPos < lMessageLen && lTempBufferPos < ( sess->thisuser.GetScreenChars() - 54 ) )
					{
						szTempBuffer[lTempBufferPos] = b[lTempBufferPos];
						lTempBufferPos++;
					}
					szTempBuffer[lTempBufferPos] = 0;
					strcat(szPrompt, szTempBuffer);
					BbsFreeMemory(b);
				}
			}
		}
		ansic( 2 );
		pla(szPrompt, &abort);
		if (nMessageNumber >= sess->GetNumMessagesInCurrentMessageArea())
		{
			abort = true;
		}
	}
	nScanOptionType = SCAN_OPTION_READ_PROMPT;
}


void HandleMessageDownload( int nMessageNumber )
{
	if ( nMessageNumber > 0 && nMessageNumber <= sess->GetNumMessagesInCurrentMessageArea() )
	{
		long lMessageLen;
		char *b = readfile(&(get_post( nMessageNumber )->msg), (subboards[sess->GetCurrentReadMessageArea()].filename), &lMessageLen);
		sess->bout << "|#1Message Download -\r\n\n";
		sess->bout << "|#2Filename to use? ";
		char szFileName[ MAX_PATH ];
		input(szFileName, 12);
		if (!okfn(szFileName))
		{
			return;
		}
		WFile fileTemp( syscfgovr.tempdir, szFileName );
		fileTemp.Delete();
		fileTemp.Open( WFile::modeBinary|WFile::modeCreateFile|WFile::modeReadWrite, WFile::shareUnknown, WFile::permReadWrite );
		fileTemp.Write( b, lMessageLen );
		fileTemp.Close();
		BbsFreeMemory(b);
		bool bFileAbortStatus;
		bool bStatus;
		send_file(fileTemp.GetFullPathName(), &bStatus, &bFileAbortStatus, 0, fileTemp.GetFullPathName(), -1, lMessageLen);
		sess->bout << "|#1Message download... |#2" << ( bStatus ? "successful" : "unsuccessful" );
		if (bStatus)
		{
			sysoplog("Downloaded message");
		}
	}
}

void HandleMessageMove( int &nMessageNumber )
{
	if ((lcs()) && (nMessageNumber > 0) && (nMessageNumber <= sess->GetNumMessagesInCurrentMessageArea()))
	{
		char *ss1 = NULL;
		tmp_disable_conf( true );
		nl();
		do
		{
			sess->bout << "|#5Move to which sub? ";
			ss1 = mmkey( 0 );
			if (ss1[0] == '?')
			{
				old_sublist();
			}
		} while ( !hangup && ss1[0] == '?' );
		int nTempSubNum = -1;
		if (ss1[0] == 0)
		{
			tmp_disable_conf( false );
			return;
		}
		bool ok = false;
		for ( int i1 = 0; ( i1 < sess->num_subs && usub[i1].subnum != -1 && !ok ); i1++ )
		{
			if ( wwiv::stringUtils::IsEquals( usub[i1].keys, ss1 ) )
			{
				nTempSubNum = i1;
				nl();
				sess->bout << "|#9Copying to " << subboards[usub[nTempSubNum].subnum].name << wwiv::endl;
				ok = true;
			}
		}
		if (nTempSubNum != -1)
		{
			if (sess->GetEffectiveSl() < subboards[usub[nTempSubNum].subnum].postsl)
			{
				nl();
				sess->bout << "Sorry, you don't have post access on that sub.\r\n\n";
				nTempSubNum = -1;
			}
		}
		if (nTempSubNum != -1)
		{
			open_sub( true );
			resynch(sess->GetCurrentMessageArea(), &nMessageNumber, NULL);
			postrec p2 = *get_post(nMessageNumber);
			postrec p1 = p2;
			long lMessageLen;
			char *b = readfile(&(p2.msg), (subboards[sess->GetCurrentReadMessageArea()].filename), &lMessageLen);
			nl();
			sess->bout << "|10Delete original post? ";
			if (yesno())
			{
				delete_message(nMessageNumber);
				if (nMessageNumber > 1)
				{
					nMessageNumber--;
				}
			}
			close_sub();
			iscan(nTempSubNum);
			open_sub( true );
			p2.msg.storage_type = (unsigned char) subboards[sess->GetCurrentReadMessageArea()].storage_type;
			savefile(b, lMessageLen, &(p2.msg), (subboards[sess->GetCurrentReadMessageArea()].filename));
			app->statusMgr->Lock();
			p2.qscan = status.qscanptr++;
			app->statusMgr->Write();
			if (sess->GetNumMessagesInCurrentMessageArea() >= subboards[sess->GetCurrentReadMessageArea()].maxmsgs)
			{
				int nTempMsgNum = 1;
				int nMsgToDelete = 0;
				while ( nMsgToDelete == 0 && nTempMsgNum <= sess->GetNumMessagesInCurrentMessageArea() )
				{
					if ((get_post(nTempMsgNum)->status & status_no_delete) == 0)
					{
						nMsgToDelete = nTempMsgNum;
					}
					++nTempMsgNum;
				}
				if (nMsgToDelete == 0)
				{
					nMsgToDelete = 1;
				}
				delete_message( nMsgToDelete );
			}
			if ((!(subboards[sess->GetCurrentReadMessageArea()].anony & anony_val_net)) ||
				(!xsubs[sess->GetCurrentReadMessageArea()].num_nets))
			{
				p2.status &= ~status_pending_net;
			}
			if (xsubs[sess->GetCurrentReadMessageArea()].num_nets)
			{
				p2.status |= status_pending_net;
                sess->thisuser.SetNumNetPosts( sess->thisuser.GetNumNetPosts() + 1 );
				send_net_post(&p2, subboards[sess->GetCurrentReadMessageArea()].filename, sess->GetCurrentReadMessageArea());
			}
			add_post(&p2);
			close_sub();
			tmp_disable_conf( false );
			iscan(sess->GetCurrentMessageArea());
			nl();
			sess->bout << "|#9Message moved.\r\n\n";
			resynch(sess->GetCurrentMessageArea(), &nMessageNumber, &p1);
		}
		else
		{
			tmp_disable_conf( false );
		}
	}
}


void HandleMessageLoad()
{
	if (!so())
	{
		return;
	}
	nl();
	sess->bout << "|#2Filename: ";
	char szFileName[ MAX_PATH ];
	input(szFileName, 50);
	if (szFileName[0])
	{
		nl();
		sess->bout << "|#5Allow editing? ";
		if (yesno())
		{
			nl();
			LoadFileIntoWorkspace( szFileName, false );
		}
		else
		{
			nl();
			LoadFileIntoWorkspace( szFileName, true );
		}
	}
}


void HandleMessageReply( int &nMessageNumber )
{
	irt_sub[0] = 0;
	if ((!lcs()) && (get_post( nMessageNumber )->status & (status_unvalidated | status_delete)))
	{
		return;
	}
	postrec p2 = *get_post( nMessageNumber );
	grab_quotes(&(p2.msg), subboards[sess->GetCurrentReadMessageArea()].filename);

	if ( okfsed() && sess->thisuser.isUseAutoQuote() &&
		 nMessageNumber > 0 && nMessageNumber <= sess->GetNumMessagesInCurrentMessageArea() )
	{
		long lMessageLen;
		char *b = readfile( &( get_post( nMessageNumber )->msg),
			               subboards[sess->GetCurrentReadMessageArea()].filename, &lMessageLen );
		auto_quote(b, lMessageLen, 1, get_post( nMessageNumber )->daten);
	}

	if ( irt[0] == '\0' )
	{
		sess->threadID = "";
	}
	post();
	resynch(sess->GetCurrentMessageArea(), &nMessageNumber, &p2);
	grab_quotes(NULL, NULL);
}

void HandleMessageDelete( int &nMessageNumber )
{
	if (lcs())
	{
		if (nMessageNumber)
		{
			open_sub( true );
			resynch(sess->GetCurrentMessageArea(), &nMessageNumber, NULL);
			postrec p2 = *get_post(nMessageNumber);
			delete_message(nMessageNumber);
			close_sub();
			if (p2.ownersys == 0)
			{
				WUser tu;
                app->userManager->ReadUser( &tu, p2.owneruser );
                if ( !tu.isUserDeleted() )
				{
					if (date_to_daten( tu.GetFirstOn() ) < static_cast<signed int>( p2.daten ) )
					{
						nl();
						sess->bout << "|#2Remove how many posts credit? ";
						char szNumCredits[ 10 ];
						input( szNumCredits, 3, true );
						int nNumCredits = 1;
						if (szNumCredits[0])
						{
							nNumCredits = (atoi(szNumCredits));
						}
						nNumCredits = std::min<int>( nNumCredits, tu.GetNumMessagesPosted() );
						if (nNumCredits)
						{
							tu.SetNumMessagesPosted( tu.GetNumMessagesPosted() - static_cast<unsigned short>( nNumCredits ) );
						}
						nl();
						sess->bout << "|#7Post credit removed = " << nNumCredits << wwiv::endl;
						tu.SetNumDeletedPosts( tu.GetNumDeletedPosts() - 1 );
                        app->userManager->WriteUser( &tu, p2.owneruser );
						app->localIO->UpdateTopScreen();
					}
				}
			}
			resynch(sess->GetCurrentMessageArea(), &nMessageNumber, &p2);
		}
	}
}


void HandleMessageExtract( int &nMessageNumber )
{
	if (so())
	{
		if ((nMessageNumber > 0) && (nMessageNumber <= sess->GetNumMessagesInCurrentMessageArea()))
		{
			long lMessageLen;
			char *b = readfile(&(get_post(nMessageNumber)->msg), (subboards[sess->GetCurrentReadMessageArea()].filename), &lMessageLen);
			if (b)
			{
				extract_out( b, lMessageLen, get_post(nMessageNumber)->title, get_post(nMessageNumber)->daten );
			}
		}
	}
}


void HandleMessageHelp()
{
	if (forcescansub)
	{
		printfile(MUSTREAD_NOEXT);
	}
	else
	{
		if (lcs())
		{
			printfile(SMBMAIN_NOEXT);
		}
		else
		{
			printfile(MBMAIN_NOEXT);
		}
	}
}

void HandleListReplies( int nMessageNumber )
{
	if ( sess->IsMessageThreadingEnabled() )
	{
		if (forcescansub)
		{
			printfile(MUSTREAD_NOEXT);
		}
		else
		{
			nl();
			sess->bout << "|#2Current Message has the following replies:\r\n";
			int k = 0;
			for (int j = 0; j <= sess->GetNumMessagesInCurrentMessageArea(); j++)
			{
				if ( wwiv::stringUtils::IsEquals( thread[j].parent_code, thread[nMessageNumber].message_code ) )
				{
					sess->bout << "    |#9Message #|12" << j << ".\r\n";
					k++;
				}
			}
			sess->bout << "|#1 " << k << " total replies.\r\n";
			nl();
			pausescr();
		}
	}
}
