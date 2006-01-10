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
#include <sstream>

#define EMAIL_STORAGE 2

#define NUM_ATTEMPTS_TO_OPEN_EMAIL 5
#define DELAY_BETWEEN_EMAIL_ATTEMPTS 9

#define GAT_NUMBER_ELEMENTS 2048
#define GAT_SECTION_SIZE    4096
#define MSG_BLOCK_SIZE      512


//
// Local function prototypes
//
void SetMessageOriginInfo(int nSystemNumber, int nUserNumber);
WFile * OpenMessageFile( const char *pszMessageAreaFileName );
void set_gat_section( WFile *pMessageFile, int section );
void save_gat( WFile *pMessageFile );

static long gat_section;


/**
 * Sets the global variables pszOutOriginStr and pszOutOriginStr2.
 * Note: This is a private function
 */
void SetMessageOriginInfo(int nSystemNumber, int nUserNumber, char *pszOutOriginStr, char *pszOutOriginStr2 )
{
	char szNetName[81];

	if ( sess->GetMaxNetworkNumber() > 1 )
	{
		sprintf( szNetName, "%s - ", net_networks[sess->GetNetworkNumber()].name );
	}
	else
	{
		szNetName[0] = '\0';
	}

	*pszOutOriginStr    = '\0';
	*pszOutOriginStr2   = '\0';

	if ( wwiv::stringUtils::IsEqualsIgnoreCase( sess->GetNetworkName(), "Internet" ) ||
         nSystemNumber == 32767 )
	{
		strcpy(pszOutOriginStr, "Internet Mail and Newsgroups");
		return;
	}

	if ( nSystemNumber && sess->GetCurrentNetworkType() == net_type_wwivnet )
	{
		net_system_list_rec *csne = next_system(nSystemNumber);
		if ( csne )
		{
            char szNetStatus[12];
			szNetStatus[0] = '\0';
			if (nUserNumber == 1)
			{
				if (csne->other & other_net_coord)
				{
					strcpy(szNetStatus, "{NC}");
				}
				else if (csne->other & other_group_coord)
				{
					sprintf(szNetStatus, "{GC%d}", csne->group);
				}
				else if (csne->other & other_area_coord)
				{
					strcpy(szNetStatus, "{AC}");
				}
			}
            char szFileName[ MAX_PATH ];
			sprintf( szFileName,
					 "%s%s%c%s.%-3u",
					 syscfg.datadir,
					 REGIONS_DIR,
				     WWIV_FILE_SEPERATOR_CHAR,
					 REGIONS_DIR,
					 atoi( csne->phone ) );

            char szDescription[ 81 ];
			if ( WFile::Exists( szFileName ) )
			{
                char szPhonePrefix[ 10 ];
				sprintf( szPhonePrefix, "%c%c%c", csne->phone[4], csne->phone[5], csne->phone[6] );
				describe_town( atoi( csne->phone ), atoi( szPhonePrefix ), szDescription );
			}
			else
			{
				describe_area_code(atoi(csne->phone), szDescription);
			}

			if (szDescription[0])
			{
				sprintf(pszOutOriginStr, "%s%s [%s] %s", szNetName, csne->name, csne->phone, szNetStatus);
				strcpy(pszOutOriginStr2, szDescription);
			}
			else
			{
				sprintf(pszOutOriginStr, "%s%s [%s] %s", szNetName, csne->name, csne->phone, szNetStatus);
				strcpy(pszOutOriginStr2, "Unknown Area");
			}
		}
		else
		{
            sprintf( pszOutOriginStr, "%s%s", szNetName, "Unknown System" );
			strcpy( pszOutOriginStr2, "Unknown Area" );
		}
	}
}

/**
 * Deletes a message
 * This is a public function.
 */
void remove_link( messagerec * pMessageRecord, const char *aux )
{
	switch ( pMessageRecord->storage_type )
	{
    case 0:
    case 1:
		break;
    case 2:
        {
		    WFile *pMessageFile = OpenMessageFile( aux );
			if ( pMessageFile->IsOpen() )
		    {
			    set_gat_section( pMessageFile, static_cast<int> (pMessageRecord->stored_as / GAT_NUMBER_ELEMENTS) );
			    long lCurrentSection = pMessageRecord->stored_as % GAT_NUMBER_ELEMENTS;
			    while ( lCurrentSection > 0 && lCurrentSection < GAT_NUMBER_ELEMENTS )
			    {
				    long lNextSection = static_cast<long>( gat[ lCurrentSection ] );
				    gat[lCurrentSection] = 0;
				    lCurrentSection = lNextSection;
			    }
			    save_gat( pMessageFile );
				pMessageFile->Close();
				delete pMessageFile;
		    }
        }
		break;
    default:
		// illegal storage type
		break;
	}
}

/**
 * Opens the message area file {pszMessageAreaFileName} and returns the file handle.
 * Note: This is a Private method to this module.
 */
WFile * OpenMessageFile( const char *pszMessageAreaFileName )
{
	app->statusMgr->Read();

	std::string strFullPathName;
	wwiv::stringUtils::FormatString( strFullPathName, "%s%s.dat", syscfg.msgsdir, pszMessageAreaFileName );
	WFile *pFileMessage = new WFile( strFullPathName );
	if ( !pFileMessage->Open( WFile::modeReadWrite | WFile::modeBinary ) )
	{
        // Create message area file if it doesn't exist.
		pFileMessage->Open( WFile::modeBinary | WFile::modeCreateFile | WFile::modeReadWrite, WFile::shareUnknown, WFile::permReadWrite );
		for ( int i = 0; i < GAT_NUMBER_ELEMENTS; i++ )
		{
			gat[i] = 0;
		}
		pFileMessage->Write( gat, GAT_SECTION_SIZE );
		strcpy( g_szMessageGatFileName, pFileMessage->GetFullPathName() );
		pFileMessage->SetLength( GAT_SECTION_SIZE + ( 75L * 1024L ) );
		gat_section = 0;
	}
	pFileMessage->Seek( 0L, WFile::seekBegin );
	pFileMessage->Read( gat, GAT_SECTION_SIZE );
	strcpy( g_szMessageGatFileName, pFileMessage->GetFullPathName() );
	gat_section = 0;
	return pFileMessage;
}


#define GATSECLEN ( GAT_SECTION_SIZE + GAT_NUMBER_ELEMENTS * MSG_BLOCK_SIZE )
#ifndef MSG_STARTING
#define MSG_STARTING ( static_cast<long>( gat_section ) * GATSECLEN + GAT_SECTION_SIZE )
#endif	// MSG_STARTING


void set_gat_section( WFile *pMessageFile, int section )
{
	if ( gat_section != section )
	{
		long lFileSize = pMessageFile->GetLength();
		long lSectionPos = static_cast<long>( section ) * GATSECLEN;
		if ( lFileSize < lSectionPos )
		{
			pMessageFile->SetLength( lSectionPos );
			lFileSize = lSectionPos;
		}
		pMessageFile->Seek( lSectionPos, WFile::seekBegin );
		if ( lFileSize < ( lSectionPos + GAT_SECTION_SIZE ) )
		{
			for ( int i = 0; i < GAT_NUMBER_ELEMENTS; i++ )
			{
				gat[ i ] = 0;
			}
			pMessageFile->Write( gat, GAT_SECTION_SIZE );
		}
		else
		{
			pMessageFile->Read( gat, GAT_SECTION_SIZE );
		}
		gat_section = section;
	}
}


void save_gat( WFile *pMessageFile )
{
	long lSectionPos = static_cast<long>( gat_section ) * GATSECLEN;
	pMessageFile->Seek( lSectionPos, WFile::seekBegin );
	pMessageFile->Write( gat, GAT_SECTION_SIZE );
	app->statusMgr->Lock();
	status.filechange[ filechange_posts ]++;
	app->statusMgr->Write();
}


void savefile( char *b, long lMessageLength, messagerec * pMessageRecord, const char *aux )
{
    WWIV_ASSERT( pMessageRecord );
    switch ( pMessageRecord->storage_type )
	{
    case 0:
    case 1:
		break;
    case 2:
        {
            int gati[128];
            WFile *pMessageFile = OpenMessageFile( aux );
			if ( pMessageFile->IsOpen() )
            {
                for ( int section = 0; section < 1024; section++ )
                {
                    set_gat_section( pMessageFile, section );
                    int gatp = 0;
                    int i5 = static_cast<int>( ( lMessageLength + 511L ) / MSG_BLOCK_SIZE );
                    int i4 = 1;
                    while ( ( gatp < i5 ) && ( i4 < GAT_NUMBER_ELEMENTS ) )
                    {
                        if ( gat[ i4 ] == 0 )
                        {
                            gati[ gatp++ ] = i4;
                        }
                        ++i4;
                    }
                    if ( gatp >= i5 )
                    {
                        gati[ gatp ] = -1;
                        for ( i4 = 0; i4 < i5; i4++ )
                        {
							pMessageFile->Seek( MSG_STARTING + MSG_BLOCK_SIZE * static_cast<long>( gati[i4] ), WFile::seekBegin );
							pMessageFile->Write( (&b[i4 * MSG_BLOCK_SIZE]), MSG_BLOCK_SIZE );
                            gat[gati[i4]] = static_cast< unsigned short >( gati[i4 + 1] );
                        }
                        save_gat( pMessageFile );
                        break;
                    }
                }
				pMessageFile->Close();
				delete pMessageFile;
            }
            pMessageRecord->stored_as = static_cast<long>( gati[0] ) + static_cast<long>( gat_section ) * GAT_NUMBER_ELEMENTS;
        }
        break;
    default:
        {
            bprintf( "WWIV:ERROR:msgbase.cpp: Save - storage_type=%u!\r\n", pMessageRecord->storage_type );
            WWIV_ASSERT( false );
        }
		break;
	}
	BbsFreeMemory( b );
}


char *readfile( messagerec * pMessageRecord, const char *aux, long *plMessageLength )
{
    char *b =  NULL;

    *plMessageLength = 0L;
    switch (pMessageRecord->storage_type)
    {
    case 0:
    case 1:
        break;
    case 2:
        {
			WFile * pMessageFile = OpenMessageFile(aux);
            set_gat_section( pMessageFile, static_cast< int >( pMessageRecord->stored_as / GAT_NUMBER_ELEMENTS ) );
            int lCurrentSection = pMessageRecord->stored_as % GAT_NUMBER_ELEMENTS;
            long lMessageLength = 0;
            while ( lCurrentSection > 0 && lCurrentSection < GAT_NUMBER_ELEMENTS )
            {
                lMessageLength += MSG_BLOCK_SIZE;
                lCurrentSection = gat[ lCurrentSection ];
            }
            if ( lMessageLength == 0 )
            {
                sess->bout << "\r\nNo message found.\r\n\n";
				pMessageFile->Close();
				delete pMessageFile;
                return NULL;
            }
            if ( ( b = static_cast<char *>( BbsAllocA( lMessageLength + 512 ) ) ) == NULL )	// was +3
            {
				pMessageFile->Close();
				delete pMessageFile;
                return NULL;
            }
            lCurrentSection = pMessageRecord->stored_as % GAT_NUMBER_ELEMENTS;
            long lMessageBytesRead = 0;
            while ( lCurrentSection > 0 && lCurrentSection < GAT_NUMBER_ELEMENTS )
            {
				pMessageFile->Seek( MSG_STARTING + MSG_BLOCK_SIZE * static_cast< long >( lCurrentSection ), WFile::seekBegin );
				lMessageBytesRead += static_cast<long>( pMessageFile->Read( &( b[lMessageBytesRead] ), MSG_BLOCK_SIZE ) );
                lCurrentSection = gat[ lCurrentSection ];
            }
			pMessageFile->Close();
			delete pMessageFile;
            long lRealMessageLength = lMessageBytesRead - MSG_BLOCK_SIZE;
            while ( ( lRealMessageLength < lMessageBytesRead ) && ( b[lRealMessageLength] != CZ ) )
            {
                ++lRealMessageLength;
            }
            *plMessageLength = lRealMessageLength;
            b[ lRealMessageLength + 1 ] = '\0';
        }
        break;
    default:
        // illegal storage type
        *plMessageLength = 0L;
        b = NULL;
        break;
    }
    return b;
}


void LoadFileIntoWorkspace( const char *pszFileName, bool bNoEditAllowed )
{
	WFile fileOrig( pszFileName );
	if ( !fileOrig.Open( WFile::modeBinary | WFile::modeReadOnly ) )
	{
		sess->bout << "\r\nFile not found.\r\n\n";
		return;
	}

	long lOrigSize = fileOrig.GetLength();
	char* b = static_cast<char*>( BbsAllocA( lOrigSize + 1024 ) );
	if (b == NULL)
	{
		fileOrig.Close();
		return;
	}
	fileOrig.Read( b, lOrigSize );
	fileOrig.Close();
	if (b[lOrigSize - 1] != CZ)
	{
		b[lOrigSize++] = CZ;
	}

	WFile fileOut( syscfgovr.tempdir, INPUT_MSG );
	fileOut.Open( WFile::modeBinary | WFile::modeCreateFile | WFile::modeReadWrite, WFile::shareUnknown, WFile::permReadWrite );
	fileOut.Write( b, lOrigSize );
	fileOut.Close();
	BbsFreeMemory(b);

	use_workspace = ( bNoEditAllowed || !okfsed() ) ? true : false;

	if ( !sess->IsNewMailWatiting() )
	{
		sess->bout << "\r\nFile loaded into workspace.\r\n\n";
		if (!use_workspace)
		{
			sess->bout << "Editing will be allowed.\r\n";
		}
	}
}

// returns true on success (i.e. the message gets forwarded)
bool ForwardMessage( int *pUserNumber, int *pSystemNumber )
{
	if (*pSystemNumber)
	{
		return false;
	}

	WUser userRecord;
    app->userManager->ReadUser( &userRecord, *pUserNumber );
    if ( userRecord.isUserDeleted() )
	{
		return false;
	}
    if ( userRecord.GetForwardUserNumber() == 0 && userRecord.GetForwardSystemNumber() == 0 &&
          userRecord.GetForwardSystemNumber() != 32767 )
	{
		return false;
	}
    if ( userRecord.GetForwardSystemNumber() != 0 )
	{
		if ( userRecord.GetForwardUserNumber() < 32767 )
		{
			int nNetworkNumber = sess->GetNetworkNumber();
			set_net_num( userRecord.GetForwardNetNumber() );
			if ( !valid_system( userRecord.GetForwardSystemNumber() ) )
			{
				set_net_num( nNetworkNumber );
				return false;
			}
			if ( !userRecord.GetForwardUserNumber() )
			{
				read_inet_addr(net_email_name, *pUserNumber);
				if (!check_inet_addr(net_email_name))
				{
					return false;
				}
			}
			*pUserNumber = userRecord.GetForwardUserNumber();
			*pSystemNumber = userRecord.GetForwardSystemNumber();
			return true;
		}
		else
		{
			read_inet_addr(net_email_name, *pUserNumber);
			*pUserNumber = 0;
			*pSystemNumber = 0;
			return false;
		}
	}
	int nCurrentUser = userRecord.GetForwardUserNumber();
	if ( nCurrentUser == -1 )
	{
		sess->bout << "Mailbox Closed.\r\n";
		if (so())
		{
			sess->bout << "(Forcing)\r\n";
		}
		else
		{
			*pUserNumber = 0;
			*pSystemNumber = 0;
		}
		return false;
	}
    char *ss = static_cast<char*> ( BbsAllocA( static_cast<long>( syscfg.maxusers ) + 300L) );
	if (ss == NULL)
	{
		return false;
	}
	for (int i = 0; i < syscfg.maxusers + 300; i++)
	{
		ss[i] = '\0';
	}
	ss[*pUserNumber] = 1;
    app->userManager->ReadUser( &userRecord, nCurrentUser );
    while ( userRecord.GetForwardUserNumber() || userRecord.GetForwardSystemNumber() )
	{
		if ( userRecord.GetForwardSystemNumber() )
		{
			if ( !valid_system( userRecord.GetForwardSystemNumber() ) )
			{
				return false;
			}
			*pUserNumber = userRecord.GetForwardUserNumber();
			*pSystemNumber = userRecord.GetForwardSystemNumber();
			BbsFreeMemory(ss);
			set_net_num( userRecord.GetForwardNetNumber() );
			return true;
		}
		if ( ss[ nCurrentUser ] )
		{
			BbsFreeMemory(ss);
			return false;
		}
		ss[ nCurrentUser ] = 1;
		if ( userRecord.GetForwardUserNumber() == 65535 )
		{
			sess->bout << "Mailbox Closed.\r\n";
			if (so())
			{
				sess->bout << "(Forcing)\r\n";
				*pUserNumber = nCurrentUser;
				*pSystemNumber = 0;
			}
			else
			{
				*pUserNumber = 0;
				*pSystemNumber = 0;
			}
			BbsFreeMemory(ss);
			return false;
		}
		nCurrentUser = userRecord.GetForwardUserNumber() ;
        app->userManager->ReadUser( &userRecord, nCurrentUser );
	}
	BbsFreeMemory( ss );
	*pSystemNumber = 0;
	*pUserNumber = nCurrentUser;
	return true;
}


WFile *OpenEmailFile( bool bAllowWrite )
{
	WFile *file = new WFile( syscfg.datadir, EMAIL_DAT );

	for ( int nAttempNum = 0; nAttempNum < NUM_ATTEMPTS_TO_OPEN_EMAIL; nAttempNum++ )
	{
		if ( bAllowWrite )
		{
			file->Open( WFile::modeBinary|WFile::modeCreateFile|WFile::modeReadWrite, WFile::shareUnknown, WFile::permReadWrite );
		}
		else
		{
			file->Open( WFile::modeBinary | WFile::modeReadOnly );
		}
		if ( file->IsOpen() )
		{
			break;
		}
		wait1( DELAY_BETWEEN_EMAIL_ATTEMPTS );
	}

	return file;
}


void sendout_email(char *pszTitle, messagerec * pMessageRec, int anony, int nUserNumber, int nSystemNumber, int an, int nFromUser, int nFromSystem, int nForwardedCode, int nFromNetworkNumber )
{
	mailrec m, messageRecord;
	net_header_rec nh;
	int i;
	char *b, *b1;

	strcpy(m.title, pszTitle);
	m.msg = *pMessageRec;
	m.anony = static_cast< unsigned char >( anony );
	if ( nFromSystem == net_sysnum )
	{
		m.fromsys = 0;
	}
	else
	{
		m.fromsys = static_cast< unsigned short >( nFromSystem );
	}
	m.fromuser	= static_cast< unsigned short >( nFromUser );
	m.tosys		= static_cast< unsigned short >( nSystemNumber );
	m.touser	= static_cast< unsigned short >( nUserNumber );
	m.status	= 0;
	time((long *) &(m.daten));

	if (m.fromsys && sess->GetMaxNetworkNumber() > 1 )
	{
		m.status |= status_new_net;
		m.title[79] = '\0';
		m.title[80] = static_cast< unsigned char >( nFromNetworkNumber );
	}

	if (nSystemNumber == 0)
	{
		WFile *pFileEmail = OpenEmailFile( true );
		WWIV_ASSERT( pFileEmail );
		if ( !pFileEmail->IsOpen() )
		{
			return;
		}
		int nEmailFileLen = static_cast< int >( pFileEmail->GetLength() / sizeof( mailrec ) );
		if ( nEmailFileLen == 0 )
		{
			i = 0;
		}
		else
		{
			i = nEmailFileLen - 1;
			pFileEmail->Seek( i * sizeof( mailrec ), WFile::seekBegin );
			pFileEmail->Read( &messageRecord, sizeof( mailrec ) );
			while ( i > 0 && messageRecord.tosys == 0 && messageRecord.touser == 0 )
			{
				--i;
				pFileEmail->Seek( i * sizeof( mailrec ), WFile::seekBegin );
				int i1 = pFileEmail->Read( &messageRecord, sizeof( mailrec ) );
				if (i1 == -1)
				{
					sess->bout << "|12DIDN'T READ WRITE!\r\n";
				}
			}
			if ( messageRecord.tosys || messageRecord.touser )
			{
				++i;
			}
		}

		pFileEmail->Seek( i * sizeof( mailrec ), WFile::seekBegin );
		int nBytesWritten = pFileEmail->Write( &m, sizeof( mailrec ) );
		pFileEmail->Close();
		delete pFileEmail;
		if (nBytesWritten == -1)
		{
			sess->bout << "|12DIDN'T SAVE RIGHT!\r\n";
		}
	}
	else
	{
    	long lEmailFileLen;
		if ((b = readfile(&(m.msg), "email", &lEmailFileLen)) == NULL)
		{
			return;
		}
		if ( nForwardedCode == 2 )
		{
			remove_link(&(m.msg), "email");
		}
		nh.tosys	= static_cast< unsigned short >( nSystemNumber );
		nh.touser	= static_cast< unsigned short >( nUserNumber );
		if ( nFromSystem > 0 )
		{
			nh.fromsys = static_cast< unsigned short >( nFromSystem );
		}
		else
		{
			nh.fromsys = net_sysnum;
		}
		nh.fromuser = static_cast< unsigned short >( nFromUser );
		nh.main_type = main_type_email;
		nh.minor_type = 0;
		nh.list_len = 0;
		nh.daten = m.daten;
		nh.method = 0;
		if ( ( b1 = static_cast<char*>( BbsAllocA( lEmailFileLen + 768 ) ) ) == NULL )
		{
			BbsFreeMemory( b );
			return;
		}
		i = 0;
		if ( nUserNumber == 0 && nFromNetworkNumber == sess->GetNetworkNumber() )
		{
			nh.main_type = main_type_email_name;
			strcpy(&(b1[i]), net_email_name);
			i += strlen(net_email_name) + 1;
		}
		strcpy(&(b1[i]), m.title);
		i += strlen(m.title) + 1;
		memmove(&(b1[i]), b,  lEmailFileLen );
		nh.length = lEmailFileLen + i;
		if (nh.length > 32760)
		{
			bprintf("Message truncated by %lu bytes for the network.", nh.length - 32760L);
			nh.length = 32760;
		}
		if ( nFromNetworkNumber != sess->GetNetworkNumber() )
		{
			gate_msg( &nh, b1, sess->GetNetworkNumber(), net_email_name, NULL, nFromNetworkNumber );
		}
		else
		{
            char szNetFileName[MAX_PATH];
			if ( nForwardedCode )
			{
                sprintf( szNetFileName, "%sp1%s", sess->GetNetworkDataDirectory(), app->GetNetworkExtension() );
			}
			else
			{
				sprintf( szNetFileName, "%sp0%s", sess->GetNetworkDataDirectory(), app->GetNetworkExtension() );
			}
			WFile fileNetworkPacket( szNetFileName );
			fileNetworkPacket.Open( WFile::modeBinary | WFile::modeCreateFile | WFile::modeReadWrite, WFile::shareUnknown, WFile::permReadWrite );
			fileNetworkPacket.Seek( 0L, WFile::seekBegin );
			fileNetworkPacket.Write( &nh, sizeof( net_header_rec ) );
			fileNetworkPacket.Write(  b1, nh.length );
			fileNetworkPacket.Close();
		}
		BbsFreeMemory(b);
		BbsFreeMemory(b1);
	}
	std::string logMessage = "Mail sent to ";
	if (nSystemNumber == 0)
	{
    	WUser userRecord;
        app->userManager->ReadUser( &userRecord, nUserNumber );
        userRecord.SetNumMailWaiting( userRecord.GetNumMailWaiting() + 1 );
        app->userManager->WriteUser( &userRecord, nUserNumber );
		if (nUserNumber == 1)
		{
			++fwaiting;
		}
		if ( user_online( nUserNumber, &i ) )
		{
			send_inst_sysstr( i, "You just received email." );
		}
		if ( an )
		{
            logMessage += userRecord.GetUserNameAndNumber( nUserNumber );
			sysoplog( logMessage.c_str() );
		}
		else
		{
			std::string tempLogMessage = logMessage;
            tempLogMessage += userRecord.GetUserNameAndNumber( nUserNumber );
			sysoplog( tempLogMessage.c_str() );
			logMessage += ">UNKNOWN<";
		}
		if ( nSystemNumber == 0 && sess->GetEffectiveSl() > syscfg.newusersl && userRecord.GetForwardSystemNumber() == 0 && !sess->IsNewMailWatiting() )
		{
			sess->bout << "|#5Attach a file to this message? ";
			if (yesno())
			{
				attach_file( 1 );
			}
		}
	}
	else
	{
		std::string logMessagePart;
		if ( ( nSystemNumber == 1 &&
               wwiv::stringUtils::IsEqualsIgnoreCase( sess->GetNetworkName(), "Internet" ) ) ||
               nSystemNumber == 32767 )
		{
			logMessagePart = net_email_name;
		}
		else
		{
			if ( sess->GetMaxNetworkNumber() > 1 )
			{
				if ( nUserNumber == 0 )
				{
					wwiv::stringUtils::FormatString( logMessagePart, "%s @%u.%s", net_email_name, nSystemNumber, sess->GetNetworkName() );
				}
				else
				{
					wwiv::stringUtils::FormatString( logMessagePart, "#%u @%u.%s", nUserNumber, nSystemNumber, sess->GetNetworkName() );
				}
			}
			else
			{
				if ( nUserNumber == 0 )
				{
					wwiv::stringUtils::FormatString( logMessagePart, "%s @%u", net_email_name, nSystemNumber );
				}
				else
				{
					wwiv::stringUtils::FormatString( logMessagePart, "#%u @%u", nUserNumber, nSystemNumber );
				}
			}
		}
		logMessage += logMessagePart;
		sysoplog( logMessage.c_str() );
	}

	app->statusMgr->Lock();
	if ( nUserNumber == 1 && nSystemNumber == 0 )
	{
		++status.fbacktoday;
        sess->thisuser.SetNumFeedbackSent( sess->thisuser.GetNumFeedbackSent() + 1 );
        sess->thisuser.SetNumFeedbackSentToday( sess->thisuser.GetNumFeedbackSentToday() + 1 );
		++fsenttoday;
	}
	else
	{
		++status.emailtoday;
        sess->thisuser.SetNumEmailSentToday( sess->thisuser.GetNumEmailSentToday() + 1 );
		if (nSystemNumber == 0)
		{
            sess->thisuser.SetNumEmailSent( sess->thisuser.GetNumEmailSent() + 1 );
		}
		else
		{
            sess->thisuser.SetNumNetEmailSent( sess->thisuser.GetNumNetEmailSent() + 1 );
		}
	}
	app->statusMgr->Write();
	if ( !sess->IsNewMailWatiting() )
	{
        ansic( 3 );
		sess->bout << logMessage;
		nl();
	}
}


bool ok_to_mail( int nUserNumber, int nSystemNumber, bool bForceit )
{
	if ( nSystemNumber != 0 && net_sysnum == 0 )
	{
		sess->bout << "\r\nSorry, this system is not a part of WWIVnet.\r\n\n";
		return false;
	}
	if ( nSystemNumber == 0 )
	{
		if ( nUserNumber == 0 )
		{
			return false;
		}
		WUser userRecord;
        app->userManager->ReadUser( &userRecord, nUserNumber );
		if ( ( userRecord.GetSl() == 255 && userRecord.GetNumMailWaiting() > (syscfg.maxwaiting * 5) ) ||
			 ( userRecord.GetSl() != 255 && userRecord.GetNumMailWaiting() > syscfg.maxwaiting ) ||
			 userRecord.GetNumMailWaiting() > 200 )
		{
			if ( !bForceit )
			{
				sess->bout << "\r\nMailbox full.\r\n";
				return false;
			}
		}
        if ( userRecord.isUserDeleted() )
		{
			sess->bout << "\r\nDeleted user.\r\n\n";
			return false;
		}
	}
	else
	{
		if ( !valid_system( nSystemNumber ) )
		{
			sess->bout << "\r\nUnknown system number.\r\n\n";
			return false;
		}
		if ( sess->thisuser.isRestrictionNet() )
		{
			sess->bout << "\r\nYou can't send mail off the system.\r\n";
			return false;
		}
	}
	if ( !sess->IsNewMailWatiting() && !bForceit )
	{
		if ( ( ( nUserNumber == 1 && nSystemNumber == 0 &&
            ( fsenttoday >= 5 || sess->thisuser.GetNumFeedbackSentToday() >= 10 ) ) ||
			( ( nUserNumber != 1 || nSystemNumber != 0 ) &&
            ( sess->thisuser.GetNumEmailSentToday() >= getslrec( sess->GetEffectiveSl() ).emails ) ) ) && !cs() )
		{
			sess->bout << "\r\nToo much mail sent today.\r\n\n";
			return false;
		}
        if ( sess->thisuser.isRestrictionEmail() && nUserNumber != 1 )
		{
			sess->bout << "\r\nYou can't send mail.\r\n\n";
			return false;
		}
	}
	return true;
}


void email( int nUserNumber, int nSystemNumber, bool forceit, int anony, bool force_title, bool bAllowFSED )
{
	int an;
	int nNumUsers = 0;
	messagerec messageRecord;
	char szDestination[81], szTitle[81];
	WUser userRecord;
	net_system_list_rec *csne = NULL;
	struct
	{
		int nUserNumber, nSystemNumber, net_num;
		char net_name[20], net_email_name[40];
	} carbon_copy[20];

	bool cc = false, bcc = false;

	if ( freek1( syscfg.msgsdir ) < 10.0 )
	{
		sess->bout << "\r\nSorry, not enough disk space left.\r\n\n";
		return;
	}
	nl();
	if ( ForwardMessage( &nUserNumber, &nSystemNumber ) )
	{
		if ( nSystemNumber == 32767 )
		{
			read_inet_addr( szDestination, nUserNumber );
		}
		sess->bout << "\r\nMail Forwarded.\r\n\n";
		if ( nUserNumber == 0 && nSystemNumber == 0 )
		{
			sess->bout << "Forwarded to unknown user.\r\n";
			return;
		}
	}
	if ( !nUserNumber && !nSystemNumber )
	{
		return;
	}
	if ( !ok_to_mail( nUserNumber, nSystemNumber, forceit ) )
	{
		return;
	}
	if ( nSystemNumber )
	{
		csne = next_system( nSystemNumber );
	}
	if ( getslrec( sess->GetEffectiveSl() ).ability & ability_read_email_anony )
	{
		an = 1;
	}
	else if ( anony & ( anony_sender | anony_sender_da | anony_sender_pp ) )
	{
		an = 0;
	}
	else
	{
		an = 1;
	}
	if ( nSystemNumber == 0 )
	{
		set_net_num( 0 );
		if ( an )
		{
            app->userManager->ReadUser( &userRecord, nUserNumber );
            strcpy( szDestination, userRecord.GetUserNameAndNumber( nUserNumber ) );
		}
		else
		{
			strcpy( szDestination, ">UNKNOWN<" );
		}
	}
	else
	{
		if ( ( nSystemNumber == 1 && nUserNumber == 0 &&
               wwiv::stringUtils::IsEqualsIgnoreCase( sess->GetNetworkName(), "Internet" ) ) ||
               nSystemNumber == 32767 )
		{
			strcpy( szDestination, net_email_name );
		}
		else
		{
			if ( sess->GetMaxNetworkNumber() > 1 )
			{
				if ( nUserNumber == 0 )
				{
					sprintf( szDestination, "%s @%u.%s", net_email_name, nSystemNumber, sess->GetNetworkName() );
				}
				else
				{
					sprintf( szDestination, "#%u @%u.%s", nUserNumber, nSystemNumber, sess->GetNetworkName() );
				}
			}
			else
			{
				if ( nUserNumber == 0 )
				{
					sprintf( szDestination, "%s @%u", net_email_name, nSystemNumber );
				}
				else
				{
					sprintf( szDestination, "#%u @%u", nUserNumber, nSystemNumber );
				}
			}
		}
	}
	if ( !sess->IsNewMailWatiting() )
	{
		sess->bout << "|#9E-mailing |#2";
	}
	sess->bout << szDestination;
	nl();
    int i = ( getslrec( sess->GetEffectiveSl() ).ability & ability_email_anony ) ? anony_enable_anony : 0;

    if ( anony & ( anony_receiver_pp | anony_receiver_da ) )
	{
		i = anony_enable_dear_abby;
	}
	if ( anony & anony_receiver )
	{
		i = anony_enable_anony;
	}
    if ( i == anony_enable_anony && sess->thisuser.isRestrictionAnonymous() )
	{
		i = 0;
	}
	if ( nSystemNumber != 0 )
	{
		if ( nSystemNumber != 32767 )
		{
			i = 0;
			anony = 0;
			nl();
            WWIV_ASSERT( csne );
			sess->bout << "|#9Name of system: |#2" << csne->name << wwiv::endl;
			sess->bout << "|#9Number of hops: |#2" << csne->numhops << wwiv::endl;
			nl();
		}
	}
	write_inst(INST_LOC_EMAIL, (nSystemNumber == 0) ? nUserNumber : 0, INST_FLAGS_NONE);

	messageRecord.storage_type = EMAIL_STORAGE;
    int nUseFSED = ( bAllowFSED ) ? INMSG_FSED : INMSG_NOFSED;
	inmsg( &messageRecord, szTitle, &i, !forceit, "email", nUseFSED, szDestination, MSGED_FLAG_NONE, force_title );
	if ( messageRecord.stored_as == 0xffffffff )
	{
		return;
	}
	if ( anony & anony_sender )
	{
		i |= anony_receiver;
	}
	if ( anony & anony_sender_da )
	{
		i |= anony_receiver_da;
	}
	if ( anony & anony_sender_pp )
	{
		i |= anony_receiver_pp;
	}

	if ( sess->IsCarbonCopyEnabled() )
	{
		if ( !sess->IsNewMailWatiting() )
		{
			nl();
			sess->bout << "|#9Copy this mail to others? ";
			nNumUsers = 0;
			if ( yesno() )
			{
				bool done = false;
				carbon_copy[nNumUsers].nUserNumber = nUserNumber;
				carbon_copy[nNumUsers].nSystemNumber = nSystemNumber;
				strcpy(carbon_copy[nNumUsers].net_name, sess->GetNetworkName() );
				strcpy(carbon_copy[nNumUsers].net_email_name, net_email_name);
				carbon_copy[nNumUsers].net_num = sess->GetNetworkNumber();
				nNumUsers++;
				do
				{
                    std::string emailAddress;
					sess->bout << "|#9Enter Address (blank to end) : ";
					input( emailAddress, 75 );
                    if ( emailAddress.empty() )
					{
						done = true;
						break;
					}
                	int tu, ts;
                    parse_email_info( emailAddress.c_str(), &tu, &ts );
					if ( tu || ts )
					{
						carbon_copy[nNumUsers].nUserNumber = tu;
						carbon_copy[nNumUsers].nSystemNumber = ts;
						strcpy(carbon_copy[nNumUsers].net_name, sess->GetNetworkName() );
						strcpy(carbon_copy[nNumUsers].net_email_name, net_email_name);
						carbon_copy[nNumUsers].net_num = sess->GetNetworkNumber();
						nNumUsers++;
						cc = true;
					}
					if ( nNumUsers == 20 )
					{
						sess->bout << "|#6Maximum number of addresses reached!";
						done = true;
					}
				} while ( !done );
				if ( cc )
				{
					sess->bout << "|#9Show Recipients in message? ";
					bcc = !yesno();
				}
			}
		}

		if ( cc && !bcc )
		{
			int listed = 0;
            std::string s1 = "\003""6Carbon Copy: \003""1";
			lineadd(&messageRecord, "\003""7----", "email");
			for (int j = 0; j < nNumUsers; j++)
			{
				if ( carbon_copy[j].nSystemNumber == 0 )
				{
					set_net_num( 0 );
                    app->userManager->ReadUser( &userRecord, carbon_copy[j].nUserNumber );
                    strcpy( szDestination, userRecord.GetUserNameAndNumber( carbon_copy[j].nUserNumber ) );
				}
				else
				{
					if ( carbon_copy[j].nSystemNumber == 1 &&
                         carbon_copy[j].nUserNumber == 0 &&
                         wwiv::stringUtils::IsEqualsIgnoreCase( carbon_copy[j].net_name, "Internet" ) )
					{
						strcpy( szDestination, carbon_copy[j].net_email_name );
					}
					else
					{
						set_net_num( carbon_copy[j].net_num );
						if ( sess->GetMaxNetworkNumber() > 1 )
						{
							if ( carbon_copy[j].nUserNumber == 0 )
							{
								sprintf( szDestination, "%s@%u.%s", carbon_copy[j].net_email_name, carbon_copy[j].nSystemNumber, carbon_copy[j].net_name );
							}
							else
							{
								sprintf( szDestination, "#%u@%u.%s", carbon_copy[j].nUserNumber, carbon_copy[j].nSystemNumber, carbon_copy[j].net_name );
							}
						}
						else
						{
							if ( carbon_copy[j].nUserNumber == 0 )
							{
								sprintf( szDestination, "%s@%u", carbon_copy[j].net_email_name, carbon_copy[j].nSystemNumber );
							}
							else
							{
								sprintf( szDestination, "#%u@%u", carbon_copy[j].nUserNumber, carbon_copy[j].nSystemNumber );
							}
						}
					}
				}
				if ( j == 0 )
				{
                    wwiv::stringUtils::FormatString( s1, "\003""6Original To: \003""1%s", szDestination );
                    lineadd( &messageRecord, s1.c_str(), "email" );
					s1 = "\003""6Carbon Copy: \003""1";
				}
				else
				{
                    if ( s1.length() + strlen( szDestination ) < 77 )
					{
						s1 += szDestination;
						if ( j + 1 < nNumUsers )
						{
							s1 += ", ";
						}
						else
						{
							s1 += "  ";
						}
						listed = 0;
					}
					else
					{
                        lineadd( &messageRecord, s1.c_str(), "email" );
						s1 += "\003""1             ";
						j--;
						listed = 1;
					}
				}
			}
			if ( !listed )
			{
                lineadd( &messageRecord, s1.c_str(), "email" );
			}
		}
	}
	if ( cc )
	{
		for ( int nCounter = 0; nCounter < nNumUsers; nCounter++ )
		{
			set_net_num( carbon_copy[nCounter].net_num );
			sendout_email( szTitle, &messageRecord, i, carbon_copy[nCounter].nUserNumber, carbon_copy[nCounter].nSystemNumber, an, sess->usernum, net_sysnum, 0, carbon_copy[nCounter].net_num );
		}
	}
	else
	{
		sendout_email( szTitle, &messageRecord, i, nUserNumber, nSystemNumber, an, sess->usernum, net_sysnum, 0, sess->GetNetworkNumber() );
	}
}


void imail( int nUserNumber, int nSystemNumber )
{
	char szInternetAddr[ 255 ];
	WUser userRecord;

	int fwdu = nUserNumber;
    bool fwdm = false;

	if ( ForwardMessage( &nUserNumber, &nSystemNumber ) )
	{
		sess->bout << "Mail forwarded.\r\n";
		fwdm = true;
	}

	if ( !nUserNumber && !nSystemNumber )
	{
		return;
	}

	if ( fwdm )
	{
		read_inet_addr( szInternetAddr, fwdu );
	}

	int i = 1;
	if ( nSystemNumber == 0 )
	{
        app->userManager->ReadUser( &userRecord, nUserNumber );
        if ( !userRecord.isUserDeleted() )
		{
            sess->bout << "|#5E-mail " << userRecord.GetUserNameAndNumber( nUserNumber ) << "? ";
			if ( !yesno() )
			{
				i = 0;
			}
		}
		else
		{
			i = 0;
		}
	}
	else
	{
		if ( fwdm )
		{
			sess->bout << "|#5E-mail " << szInternetAddr << "? ";
		}
		else
		{
			sess->bout << "|#5E-mail User " << nUserNumber << " @" << nSystemNumber << " ? ";
		}
		if ( !yesno() )
		{
			i = 0;
		}
	}
	grab_quotes( NULL, NULL );
	if ( i )
	{
		email( nUserNumber, nSystemNumber, false, 0 );
	}
}


void read_message1( messagerec * pMessageRecord, char an, bool readit, bool *next, const char *pszFileName, int nFromSystem, int nFromUser )
{
	char szToName[205], szDate[81], s[205];
	long lMessageTextLength;

    char origin_str[128];
    char origin_str2[81];

    // Moved internally from outside this method
    SetMessageOriginInfo( nFromSystem, nFromUser, origin_str, origin_str2 );

	g_flags &= ~g_flag_ansi_movement;

	char* ss = NULL;
	bool ansi = false;
	*next = false;
	long lCurrentCharPointer = 0;
	bool abort = false;
	switch (pMessageRecord->storage_type)
	{
    case 0:
    case 1:
    case 2:
		{
			ss = readfile(pMessageRecord, pszFileName, &lMessageTextLength);
			if (ss == NULL)
			{
				plan(6, "File not found.", &abort, next);
				nl();
				return;
			}
			int nNamePtr = 0;
			while ( ( ss[nNamePtr] != RETURN) &&
				    ( static_cast<long>( nNamePtr ) < lMessageTextLength ) &&
				    ( nNamePtr < 200 ) )
			{
				szToName[nNamePtr] = ss[nNamePtr++];
			}
			szToName[nNamePtr] = '\0';
			++nNamePtr;
			int nDatePtr = 0;
			if (ss[nNamePtr] == SOFTRETURN )
			{
				++nNamePtr;
			}
			while ( ( ss[nNamePtr + nDatePtr] != RETURN ) &&
				    static_cast<long>( nNamePtr + nDatePtr < lMessageTextLength ) &&
					( nDatePtr < 60 ) )
			{
				szDate[ nDatePtr ] = ss[ ( nDatePtr++ ) + nNamePtr ];
			}
			szDate[nDatePtr] = '\0';
			lCurrentCharPointer = nNamePtr + nDatePtr + 1;
		}
		break;
    default:
		// illegal storage type
		nl();
		sess->bout << "->ILLEGAL STORAGE TYPE<-\r\n\n";
		return;
	}

	irt_name[0] = '\0';
    g_flags |= g_flag_disable_mci;
    switch (an)
    {
	default:
	case 0:
        if (syscfg.sysconfig & sysconfig_enable_mci)
		{
			g_flags &= ~g_flag_disable_mci;
		}
        osan("|#1Name|#7: ", &abort, next);
        plan(sess->GetMessageColor(), szToName, &abort, next);
        strcpy(irt_name, szToName);
        osan("|#1Date|#7: ", &abort, next);
        plan(sess->GetMessageColor(), szDate, &abort, next);
        if (origin_str[0])
		{
			if (szToName[1] == '`')
			{
				osan("|#1Gated From|#7: ", &abort, next);
			}
			else
			{
				osan("|#1From|#7: ", &abort, next);
			}
            plan(sess->GetMessageColor(), origin_str, &abort, next);
        }
        if (origin_str2[0])
		{
			osan("|#1Loc|#7:  ", &abort, next);
            plan(sess->GetMessageColor(), origin_str2, &abort, next);
        }
        break;
	case anony_sender:
        if (readit)
		{
			osan("|#1Name|#7: ", &abort, next);
			std::stringstream toName;
			toName << "<<< " << szToName << " >>>";
			plan(sess->GetMessageColor(), toName.str().c_str(), &abort, next);
			osan("|#1Date|#7: ", &abort, next);
			plan(sess->GetMessageColor(), szDate, &abort, next);
        }
		else
		{
			osan("|#1Name|#7: ", &abort, next);
			plan(sess->GetMessageColor(), ">UNKNOWN<", &abort, next);
			osan("|#1Date|#7: ", &abort, next);
			plan(sess->GetMessageColor(), ">UNKNOWN<", &abort, next);
        }
        break;
	case anony_sender_da:
	case anony_sender_pp:
        if (an == anony_sender_da)
		{
			osan("|#1Name|#7: ", &abort, next);
			plan(sess->GetMessageColor(), "Abby", &abort, next);
        }
		else
		{
			osan("|#1Name|#7: ", &abort, next);
			plan(sess->GetMessageColor(), "Problemed Person", &abort, next);
        }
        if ( readit )
		{
			osan("|#1Name|#7: ", &abort, next);
			plan(sess->GetMessageColor(), szToName, &abort, next);
			osan("|#1Date|#7: ", &abort, next);
			plan(sess->GetMessageColor(), szDate, &abort, next);
        }
		else
		{
			osan("|#1Date|#7: ", &abort, next);
			plan(sess->GetMessageColor(), ">UNKNOWN<", &abort, next);
        }
        break;
    }

	int nNumCharsPtr	= 0;
	int nLineLenPtr		= 0;
	int ctrld			= 0;
    char ch				= 0;
	bool done			= false;
	bool printit		= false;
	bool ctrla			= false;
	bool centre			= false;

	nl();
	while ( !done && !abort && !hangup )
	{
		switch ( pMessageRecord->storage_type )
        {
		case 0:
		case 1:
		case 2:
			ch = ss[lCurrentCharPointer];
			if ( lCurrentCharPointer >= lMessageTextLength )
			{
				ch = CZ;
			}
			break;
		}
		if ( ch == CZ )
		{
			done = true;
		}
		else
		{
			if ( ch != SOFTRETURN )
			{
				if ( ch == RETURN || !ch )
				{
					printit = true;
				}
				else if ( ch == CA )
				{
					ctrla = true;
				}
				else if ( ch == CB )
				{
					centre = true;
				}
				else if ( ch == CD )
				{
					ctrld = 1;
				}
				else if ( ctrld == 1 )
				{
					if ( ch >= '0' && ch <= '9' )
					{
						if ( sess->thisuser.GetOptionalVal() < ( ch - '0'  ) )
						{
							ctrld = 0;
						}
						else
						{
							ctrld = -1;
						}
					}
					else
					{
						ctrld = 0;
					}
				}
				else
				{
					if ( ch == ESC )
					{
						ansi = true;
					}
					if ( g_flags & g_flag_ansi_movement )
					{
						g_flags &= ~g_flag_ansi_movement;
						lines_listed = 0;
						if ( sess->topline && sess->screenbottom == 24 )
						{
							app->localIO->set_protect( 0 );
						}
					}
					s[nNumCharsPtr++] = ch;
					if ( ch == CC || ch == BACKSPACE )
					{
						--nLineLenPtr;
					}
					else
					{
						++nLineLenPtr;
					}
				}

				if ( printit || ansi || nLineLenPtr >= 80 )
				{
					if ( centre && ( ctrld != -1 ) )
					{
						int nSpacesToCenter = ( sess->thisuser.GetScreenChars() - app->localIO->WhereX() - nLineLenPtr ) / 2;
                        osan( charstr( nSpacesToCenter, ' ' ), &abort, next);
					}
					if ( nNumCharsPtr )
					{
						if ( ctrld != -1 )
						{
							if ( ( app->localIO->WhereX() + nLineLenPtr >= sess->thisuser.GetScreenChars() ) && !centre && !ansi )
							{
								nl();
							}
							s[nNumCharsPtr] = '\0';
							osan( s, &abort, next );
							if ( ctrla && s[nNumCharsPtr - 1] != SPACE && !ansi )
							{
								if ( app->localIO->WhereX() < sess->thisuser.GetScreenChars() - 1 )
								{
									bputch( SPACE );
								}
								else
								{
									nl();
								}
								checka( &abort, next );
							}
						}
						nLineLenPtr	 = 0;
						nNumCharsPtr = 0;
					}
					centre = false;
				}
				if ( ch == RETURN )
				{
					if ( ctrla == false )
					{
						if ( ctrld != -1 )
						{
							nl();
							checka( &abort, next );
						}
					}
					else
					{
						ctrla = false;
					}
					if ( printit )
					{
						ctrld = 0;
					}
				}
				printit = false;
			}
			else
			{
				ctrld = 0;
			}
		}
		++lCurrentCharPointer;
	}
	if ( !abort && nNumCharsPtr )
	{
		s[nNumCharsPtr] = '\0';
		sess->bout << s;
		nl();
	}
	ansic( 0 );
	nl();
	if ( express && abort && !*next )
	{
		expressabort = true;
	}
	if ( ss != NULL )
	{
		BbsFreeMemory( ss );
	}
	tmp_disable_pause( false );
	if ( ansi && sess->topdata && sess->IsUserOnline() )
	{
		app->localIO->UpdateTopScreen();
	}
	if ( syscfg.sysconfig & sysconfig_enable_mci )
	{
		g_flags &= ~g_flag_disable_mci;
	}
}




void read_message(int n, bool *next, int *val)
{
	nl();
	bool abort = false;
	*next = false;
	if ( sess->thisuser.isUseClearScreen() )
    {
		ClearScreen();
    }
	if ( forcescansub )
    {
		ClearScreen();
		goxy( 1, 1 );
		sess->bout << "|#4   FORCED SCAN OF SYSOP INFORMATION - YOU MAY NOT ABORT.  PLEASE READ THESE!  |#0\r\n";
	}

	std::string subjectLine;
	bprintf( " |#1Msg|#7: [|#2%u|#7/|#2%lu|#7]|#%d %s\r\n", n, sess->GetNumMessagesInCurrentMessageArea(), sess->GetMessageColor(), subboards[sess->GetCurrentReadMessageArea()].name );
	subjectLine = "|#1Subj|#7: ";
	osan( subjectLine.c_str(), &abort, next );
	ansic( sess->GetMessageColor() );
	postrec p = *get_post( n );
	if ( p.status & ( status_unvalidated | status_delete ) )
    {
		plan( 6, "<<< NOT VALIDATED YET >>>", &abort, next );
		if ( !lcs() )
        {
			return;
        }
		*val |= 1;
		osan( subjectLine.c_str(), &abort, next );
		ansic( sess->GetMessageColor() );
	}
	strncpy( irt, p.title, 60 );
	irt_name[0] = '\0';
	plan( sess->GetMessageColor(), irt, &abort, next );
	if ( ( p.status & status_no_delete ) && lcs() )
    {
		osan( "|#1Info|#7: ", &abort, next );
		plan( sess->GetMessageColor(), "Permanent Message", &abort, next );
	}
	if ( ( p.status & status_pending_net ) && sess->thisuser.GetSl() > syscfg.newusersl )
    {
		osan( "|#1Val|#7:  ", &abort, next );
		plan( sess->GetMessageColor(), "Not Network Validated", &abort, next);
		*val |= 2;
	}
	if ( !abort )
    {
        bool bReadit = ( lcs() || ( getslrec( sess->GetEffectiveSl() ).ability & ability_read_post_anony ) ) ? true : false;
		int nNetNumSaved = sess->GetNetworkNumber();

		if ( p.status & status_post_new_net )
        {
			set_net_num( p.title[80] );
        }
        read_message1( &( p.msg ), static_cast<char>( p.anony & 0x0f ), bReadit, next, (subboards[sess->GetCurrentReadMessageArea()].filename), p.ownersys, p.owneruser );

		if ( nNetNumSaved != sess->GetNetworkNumber() )
        {
			set_net_num( nNetNumSaved );
        }

        sess->thisuser.SetNumMessagesRead( sess->thisuser.GetNumMessagesRead() + 1 );
        sess->SetNumMessagesReadThisLogon( sess->GetNumMessagesReadThisLogon() + 1 );
	}
    else if ( express && !*next )
    {
		expressabort = true;
    }
	if ( p.qscan > qsc_p[sess->GetCurrentReadMessageArea()] )
    {
		qsc_p[sess->GetCurrentReadMessageArea()] = p.qscan;
    }
	if ( p.qscan >= status.qscanptr )
    {
		app->statusMgr->Lock();
		if ( p.qscan >= status.qscanptr )
        {
			status.qscanptr = p.qscan + 1;
        }
		app->statusMgr->Write();
	}
}


void lineadd( messagerec * pMessageRecord, const char *sx, const char *aux )
{
	char szLine[ 255 ];
    sprintf( szLine, "%s\r\n\x1a", sx );

    switch ( pMessageRecord->storage_type )
    {
    case 0:
    case 1:
		break;
    case 2:
        {
            WFile * pMessageFile = OpenMessageFile( aux );
            set_gat_section( pMessageFile, pMessageRecord->stored_as / GAT_NUMBER_ELEMENTS );
            int new1 = 1;
            while ( new1 < GAT_NUMBER_ELEMENTS && gat[new1] != 0 )
            {
                ++new1;
            }
            int i = static_cast<int>( pMessageRecord->stored_as % GAT_NUMBER_ELEMENTS );
            while ( gat[i] != 65535 )
            {
                i = gat[i];
            }
            char *b = NULL;
            if ( ( b = static_cast<char*>( BbsAllocA( GAT_NUMBER_ELEMENTS ) ) ) == NULL )
            {
				pMessageFile->Close();
				delete pMessageFile;
                return;
            }
			pMessageFile->Seek( MSG_STARTING + static_cast<long>( i ) * MSG_BLOCK_SIZE, WFile::seekBegin );
			pMessageFile->Read( b, MSG_BLOCK_SIZE );
            int j = 0;
            while ( j < MSG_BLOCK_SIZE && b[j] != CZ )
            {
                ++j;
            }
            strcpy( &( b[j] ), szLine );
			pMessageFile->Seek( MSG_STARTING + static_cast<long>( i ) * MSG_BLOCK_SIZE, WFile::seekBegin );
			pMessageFile->Write( b, MSG_BLOCK_SIZE );
            if ( ( ( j + strlen( szLine ) ) > MSG_BLOCK_SIZE ) && ( new1 != GAT_NUMBER_ELEMENTS ) )
            {
				pMessageFile->Seek( MSG_STARTING + static_cast<long>( new1 )  * MSG_BLOCK_SIZE, WFile::seekBegin );
				pMessageFile->Write( b + MSG_BLOCK_SIZE, MSG_BLOCK_SIZE );
                gat[new1] = 65535;
                gat[i] = static_cast< unsigned short >( new1 );
                save_gat( pMessageFile );
            }
            BbsFreeMemory( b );
			pMessageFile->Close();
			delete pMessageFile;
        }
		break;
    default:
		// illegal storage type
		break;
	}
}


