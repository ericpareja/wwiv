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
// Allows local-only editing of some of the user data in a shadowized
// window.
//


void OnlineUserEditor()
{
#if !defined (_UNIX)
	char sl[4], dsl[4], exempt[4], sysopsub[4], ar[17], dar[17], restrict[17], rst[17], uk[8], dk[8], up[6], down[6], posts[6], banktime[6], gold[10], ass[6], logons[6];
	int cp, i, rc = ABORTED;

	app->localIO->pr_Wait( 1 );
	app->localIO->savescreen(&screensave);
	curatr = sess->GetUserEditorColor();
	int wx = 5;
	int wy = 3;
	app->localIO->MakeLocalWindow(wx, wy-2, 70, 16 + 2);
    char szBar[ 255 ];
	sprintf( szBar, "�%s�", charstr( 70 - wx + 3, '�' ) );
	app->localIO->LocalXYPrintf( wx, wy, szBar );
	app->localIO->LocalXYPrintf( wx, wy + 4, szBar );
	app->localIO->LocalXYPrintf( wx, wy + 7, szBar );
	app->localIO->LocalXYPrintf( wx, wy + 11, szBar );
	app->localIO->LocalXYPrintf( wx, wy + 13, szBar);
    sprintf( sl, "%u", sess->thisuser.GetSl() );
	sprintf( dsl, "%u", sess->thisuser.GetDsl() );
	sprintf( exempt, "%u", sess->thisuser.GetExempt() );
	if (*qsc > 999)
	{
		*qsc = 999;
	}
	sprintf( sysopsub, "%lu", *qsc );
    sprintf( uk, "%lu", sess->thisuser.GetUploadK() );
	sprintf( dk, "%lu", sess->thisuser.GetDownloadK() );
    sprintf( up, "%u", sess->thisuser.GetFilesUploaded() );
    sprintf( down, "%u", sess->thisuser.GetFilesDownloaded() );
    sprintf( posts, "%u", sess->thisuser.GetNumMessagesPosted() );
    sprintf( banktime, "%u", sess->thisuser.GetTimeBankMinutes() );
    sprintf( logons, "%u", sess->thisuser.GetNumLogons() );
    sprintf( ass, "%u", sess->thisuser.GetAssPoints() );

	gcvt( sess->thisuser.GetGold(), 5, gold );
	strcpy( rst, restrict_string );
	for (i = 0; i <= 15; i++)
	{
		if ( sess->thisuser.hasArFlag(1 << i))
		{
			ar[i] = ( char ) ( 'A' + i );
		}
		else
		{
			ar[i] = SPACE;
		}
		if (sess->thisuser.hasDarFlag(1 << i))
		{
			dar[i] = ( char ) ( 'A' + i );
		}
		else
		{
			dar[i] = SPACE;
		}
		if ( sess->thisuser.hasRestrictionFlag ( 1 << i ) )
		{
			restrict[i] = rst[i];
		}
		else
		{
			restrict[i] = SPACE;
		}
	}
	dar[16]	= '\0';
	ar[16]	= '\0';
	restrict[16] = '\0';
	cp = 0;
	bool done = false;

    // heading
    char szLocalName[ 255 ];
    sprintf( szLocalName, "[%s]", sess->thisuser.GetUserNameAndNumber( sess->usernum ) );
	app->localIO->LocalXYAPrintf( wx + 1, wy - 1, 31, " %-29.29s%s ", "WWIV User Editor", StringJustify( szLocalName, 37, SPACE, JUSTIFY_RIGHT ) );

    app->localIO->LocalXYAPrintf( wx + 2,  wy + 1, 3,   "Security Level(SL): %s", sl );
	app->localIO->LocalXYAPrintf( wx + 36, wy + 1, 3,   "  Message AR: %s", ar );
	app->localIO->LocalXYAPrintf( wx + 2,  wy + 2, 3,   "DL Sec. Level(DSL): %s", dsl );
	app->localIO->LocalXYAPrintf( wx + 36, wy + 2, 3,   " Download AR: %s", dar );
	app->localIO->LocalXYAPrintf( wx + 2,  wy + 3, 3,   "   User Exemptions: %s", exempt );
	app->localIO->LocalXYAPrintf( wx + 36, wy + 3, 3,   "Restrictions: %s", restrict );
	app->localIO->LocalXYAPrintf( wx + 2,  wy + 5, 3,   "         Sysop Sub: %s", sysopsub );
	app->localIO->LocalXYAPrintf( wx + 36, wy + 5, 3,   "   Time Bank: %s", banktime );
	app->localIO->LocalXYAPrintf( wx + 2,  wy + 6, 3,   "        Ass Points: %s", ass );
	app->localIO->LocalXYAPrintf( wx + 36, wy + 6, 3,   " Gold Points: %s", gold );
	app->localIO->LocalXYAPrintf( wx + 2,  wy + 8, 3,   "       KB Uploaded: %s", uk );
	app->localIO->LocalXYAPrintf( wx + 35, wy + 8, 3,   "KB Downloaded: %s", dk );
	app->localIO->LocalXYAPrintf( wx + 2,  wy + 9, 3,   "    Files Uploaded: %s", up );
	app->localIO->LocalXYAPrintf( wx + 32, wy + 9, 3,   "Files Downloaded: %s", down );
	app->localIO->LocalXYAPrintf( wx + 2,  wy + 10, 3,  "   Messages Posted: %s", posts );
	app->localIO->LocalXYAPrintf( wx + 32, wy + 10, 3,  "Number of Logons: %s", logons );
	app->localIO->LocalXYAPrintf( wx + 2,  wy + 12, 3,  "Note: %s", sess->thisuser.GetNote() );
	app->localIO->LocalXYAPrintf( wx + 1, wy + 14, 31,  "    (ENTER) Next Field   (UP-ARROW) Previous Field    (ESC) Exit    ");
    curatr = 3;
	while ( !done )
	{
		switch (cp)
		{
		case 0:
			app->localIO->LocalGotoXY(wx + 22, wy + 1);
			app->localIO->LocalEditLine(sl, 3, NUM_ONLY, &rc, "");
			sess->thisuser.SetSl( atoi( sl ) );
            sprintf( sl, "%d", sess->thisuser.GetSl() );
			app->localIO->LocalPrintf( "%-3s", sl );
			break;
		case 1:
			app->localIO->LocalGotoXY( wx + 50, wy + 1 );
			app->localIO->LocalEditLine( ar, 16, SET, &rc, "ABCDEFGHIJKLMNOP " );
			sess->thisuser.SetAr( 0 );
			for (i = 0; i <= 15; i++)
			{
				if (ar[i] != SPACE)
				{
                    sess->thisuser.SetArFlag(1 << i);
				}
			}
			break;
		case 2:
			app->localIO->LocalGotoXY(wx + 22, wy + 2);
			app->localIO->LocalEditLine(dsl, 3, NUM_ONLY, &rc, "");
			sess->thisuser.SetDsl( atoi( dsl ) );
            sprintf( dsl, "%d", sess->thisuser.GetDsl() );
			app->localIO->LocalPrintf( "%-3s", dsl );
			break;
		case 3:
			app->localIO->LocalGotoXY(wx + 50, wy + 2);
			app->localIO->LocalEditLine(dar, 16, SET, &rc, "ABCDEFGHIJKLMNOP ");
			sess->thisuser.SetDar( 0 );
			for (i = 0; i <= 15; i++)
			{
				if (dar[i] != SPACE)
				{
                    sess->thisuser.SetDarFlag( 1 << i );
				}
			}
			break;
		case 4:
			app->localIO->LocalGotoXY(wx + 22, wy + 3);
			app->localIO->LocalEditLine(exempt, 3, NUM_ONLY, &rc, "");
			sess->thisuser.SetExempt( atoi( exempt ) );
            sprintf( exempt, "%u", sess->thisuser.GetExempt() );
			app->localIO->LocalPrintf( "%-3s", exempt );
			break;
		case 5:
			app->localIO->LocalGotoXY(wx + 50, wy + 3);
			app->localIO->LocalEditLine(restrict, 16, SET, &rc, rst);
			sess->thisuser.SetRestriction( 0 );
			for (i = 0; i <= 15; i++)
			{
				if (restrict[i] != SPACE)
				{
					sess->thisuser.setRestrictionFlag( 1 << i );
				}
			}
			break;
		case 6:
			app->localIO->LocalGotoXY(wx + 22, wy + 5);
			app->localIO->LocalEditLine(sysopsub, 3, NUM_ONLY, &rc, "");
			*qsc = atoi( sysopsub );
            sprintf( sysopsub, "%lu", *qsc );
			app->localIO->LocalPrintf( "%-3s", sysopsub );
			break;
		case 7:
			app->localIO->LocalGotoXY(wx + 50, wy + 5);
			app->localIO->LocalEditLine(banktime, 5, NUM_ONLY, &rc, "");
			sess->thisuser.SetTimeBankMinutes( atoi( banktime ) );
            sprintf( banktime, "%u", sess->thisuser.GetTimeBankMinutes() );
			app->localIO->LocalPrintf( "%-5s", banktime );
			break;
		case 8:
			app->localIO->LocalGotoXY(wx + 22, wy + 6);
			app->localIO->LocalEditLine(ass, 5, NUM_ONLY, &rc, "");
			sess->thisuser.SetAssPoints( atoi( ass ) );
            sprintf( ass, "%u", sess->thisuser.GetAssPoints() );
			app->localIO->LocalPrintf( "%-5s", ass );
			break;
		case 9:
			app->localIO->LocalGotoXY(wx + 50, wy + 6);
			app->localIO->LocalEditLine( gold, 5, NUM_ONLY, &rc, "" );
			sess->thisuser.SetGold( static_cast<float>( atof( gold ) ) );
			gcvt( sess->thisuser.GetGold(), 5, gold );
			app->localIO->LocalPrintf( "%-5s", gold );
			break;
		case 10:
			app->localIO->LocalGotoXY(wx + 22, wy + 8);
			app->localIO->LocalEditLine(uk, 7, NUM_ONLY, &rc, "");
			sess->thisuser.SetUploadK( atol( uk ) );
            sprintf( uk, "%lu", sess->thisuser.GetUploadK() );
			app->localIO->LocalPrintf( "%-7s", uk );
			break;
		case 11:
			app->localIO->LocalGotoXY(wx + 50, wy + 8);
			app->localIO->LocalEditLine(dk, 7, NUM_ONLY, &rc, "");
			sess->thisuser.SetDownloadK( atol( dk ) );
            sprintf( dk, "%lu", sess->thisuser.GetDownloadK() );
            app->localIO->LocalPrintf( "%-7s", dk );
			break;
		case 12:
			app->localIO->LocalGotoXY(wx + 22, wy + 9);
			app->localIO->LocalEditLine(up, 5, NUM_ONLY, &rc, "");
			sess->thisuser.SetFilesUploaded( atoi( up ) );
            sprintf( up, "%u", sess->thisuser.GetFilesUploaded() );
			app->localIO->LocalPrintf( "%-5s", up );
			break;
		case 13:
			app->localIO->LocalGotoXY( wx + 50, wy + 9 );
			app->localIO->LocalEditLine( down, 5, NUM_ONLY, &rc, "" );
			sess->thisuser.SetFilesDownloaded( atoi( down ) );
            sprintf( down, "%u", sess->thisuser.GetFilesDownloaded() );
			app->localIO->LocalPrintf( "%-5s", down );
			break;
		case 14:
			app->localIO->LocalGotoXY(wx + 22, wy + 10);
			app->localIO->LocalEditLine(posts, 5, NUM_ONLY, &rc, "");
			sess->thisuser.SetNumMessagesPosted( atoi( posts ) );
            sprintf( posts, "%u", sess->thisuser.GetNumMessagesPosted() );
			app->localIO->LocalPrintf( "%-5s", posts );
			break;
		case 15:
			app->localIO->LocalGotoXY(wx + 50, wy + 10);
			app->localIO->LocalEditLine(logons, 5, NUM_ONLY, &rc, "");
            sess->thisuser.SetNumLogons( atoi( logons ) );
            sprintf( logons, "%u", sess->thisuser.GetNumLogons() );
			app->localIO->LocalPrintf( "%-5s", logons );
			break;
		case 16:
            {
                char szNote[ 81 ];
			    app->localIO->LocalGotoXY( wx + 8, wy + 12 );
                strcpy( szNote, sess->thisuser.GetNote() );
			    app->localIO->LocalEditLine( szNote, 60, ALL, &rc, "" );
			    StringTrimEnd( szNote );
                sess->thisuser.SetNote( szNote );
            }
			break;
    }
    switch (rc)
	{
	case ABORTED:
        done = true;
        break;
	case DONE:
        done = true;
        break;
	case NEXT:
        cp = (cp + 1) % 17;
        break;
	case PREV:
        cp--;
        if ( cp < 0 )
		{
			cp = 16;
		}
        break;
    }
  }
  app->localIO->restorescreen(&screensave);
  sess->ResetEffectiveSl();
  changedsl();
  app->localIO->pr_Wait( 0 );
#endif // !defined (_UNIX)
}



/**
 * This function prints out a string, with a user-specifiable delay between
 * each character, and a user-definable pause after the entire string has
 * been printed, then it backspaces the string. The color is also definable.
 * The parameters are as follows:
 * <p>
 * <em>Note: ANSI is not required.</em>
 * <p>
 * Example:
 * <p>
 * BackPrint("This is an example.",3,20,500);
 *
 * @param pszText  The string to print
 * @param nColorCode The color of the string
 * @param nCharDelay Delay between each character, in milliseconds
 * @param nStringDelay Delay between completion of string and backspacing
 */
void BackPrint( const char *pszText, int nColorCode, int nCharDelay, int nStringDelay )
{
	WWIV_ASSERT( pszText );

	bool oecho = echo;
	echo = true;
	int nLength = strlen( pszText );
	ansic( nColorCode );
	WWIV_Delay( nCharDelay );
	int nPos = 0;
	while ( pszText[nPos]  && !hangup )
	{
		bputch( pszText[nPos] );
		nPos++;
		WWIV_Delay( nCharDelay );
	}

	WWIV_Delay( nStringDelay );
	for ( int i = 0; i < nLength && !hangup; i++ )
	{
		BackSpace();
		WWIV_Delay( 5 );
	}
	echo = oecho;
}


/**
 * This function will reposition the cursor i spaces to the left, or if the
 * cursor is on the left side of the screen already then it will not move.
 * If the user has no ANSI then nothing happens.
 * @param nNumberOfChars Number of characters to move to the left
 */
void MoveLeft( int nNumberOfChars )
{
	if ( okansi() )
	{
		sess->bout << "\x1b[" << nNumberOfChars << "D";
	}
}


/**
 * Moves the cursor to the end of the line using ANSI sequences.  If the user
 * does not have ansi, this this function does nothing.
 */
void ClearEOL()
{
	if ( okansi() )
	{
		sess->bout << "\x1b[K";
	}
}


/**
 * This function will print out a string, making each character "spin"
 * using the / - \ | sequence. The color is definable and is the
 * second parameter, not the first. If the user does not have ANSI
 * then the string is simply printed normally.
 * @param
 */
void SpinPuts( const char *pszText, int nColorCode )
{
	bool oecho	= echo;
	echo		= true;

	WWIV_ASSERT( pszText );

	if ( okansi() )
	{
		ansic( nColorCode );
	    const int dly = 30;
	    int nPos = 0;
		while ( pszText[nPos] && !hangup )
		{
			WWIV_Delay(dly);
			sess->bout << "/";
			MoveLeft( 1 );
			WWIV_Delay(dly);
			sess->bout << "-";
			MoveLeft( 1 );
			WWIV_Delay(dly);
			sess->bout << "\\";
			MoveLeft( 1 );
			WWIV_Delay(dly);
			sess->bout << "|";
			MoveLeft( 1 );
			WWIV_Delay(dly);
			bputch(pszText[nPos]);
			nPos++;
		}
	}
	else
	{
		sess->bout << pszText;
	}
	echo = oecho;
}
