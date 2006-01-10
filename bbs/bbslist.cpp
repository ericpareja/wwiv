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
// $Header$

#include "wwiv.h"
#include "WStringUtils.h"

//
// Local function prototypes
//

char ShowBBSListMenuAndGetChoice();
bool IsBBSPhoneNumberUnique( const char *pszPhoneNumber );
bool IsBBSPhoneNumberValid( const char *pszPhoneNumber );
void AddBBSListLine( const char *pszBbsListLine );
void AddBBSListEntryImpl();
void AddBBSListEntry();
void DeleteBBSListEntry();



char ShowBBSListMenuAndGetChoice()
{
    nl();
    if ( so() )
    {
        sess->bout << "|#9(|#2Q|#9=|#1Quit|#9) [|#2BBS list|#9]: (|#1R|#9)ead, (|#1A|#9)dd, (|#1D|#9)elete, (|#1N|#9)et : ";
        return onek("QRNAD");
    }
    else
    {
        sess->bout << "|#9(|#2Q|#9=|#1Quit|#9) [|#2BBS list|#9] (|#1R|#9)ead, (|#1A|#9)dd, (|#1N|#9)et : ";
        return onek("QRNA");
    }
}


bool IsBBSPhoneNumberUnique( const char *pszPhoneNumber )
{
    bool ok = true;
    WFile file( syscfg.gfilesdir, BBSLIST_MSG );
    if ( file.Open( WFile::modeReadOnly | WFile::modeBinary ) )
    {
        file.Seek( 0L, WFile::seekBegin );
        long lBbsListLength = file.GetLength();
        char *ss = static_cast<char *>( BbsAllocA( lBbsListLength + 500L ) );
        if ( ss == NULL )
        {
            file.Close();
            return true;
        }
        file.Read( ss, lBbsListLength );
        long lBbsListPos = 0L;
        while ( lBbsListPos < lBbsListLength && ok )
        {
            char szBbsListLine[ 255 ];
            int i = 0;
            char ch = '\0';
            do
            {
                ch = ss[ lBbsListPos++ ];
                szBbsListLine[i] = ch;
                if ( ch == '\r' )
                {
                    szBbsListLine[i] = '\0';
                }
                ++i;
            } while ( ch != '\n' && i < 120 && lBbsListPos < lBbsListLength );
            if ( strstr( szBbsListLine, pszPhoneNumber ) != NULL )
            {
                ok = false;
            }
            if ( strncmp( szBbsListLine, pszPhoneNumber, 12 ) == 0 )
            {
                ok = false;
            }
        }
        BbsFreeMemory( ss );
        ss = NULL;
        file.Close();
    }
    return ok;
}


bool IsBBSPhoneNumberValid( const char *pszPhoneNumber )
{
    if ( !pszPhoneNumber || !*pszPhoneNumber )
    {
        return false;
    }
    if ( pszPhoneNumber[3] != '-' || pszPhoneNumber[7] != '-' )
    {
        return false;
    }
    for ( int nPhoneNumIter = 0; nPhoneNumIter < 12; nPhoneNumIter++ )
    {
        if ( strchr("0123456789-", pszPhoneNumber[nPhoneNumIter]) == 0 )
        {
            return false;
        }
    }
    if ( strlen( pszPhoneNumber ) != 12 )
    {
        return false;
    }
    return true;
}


void AddBBSListLine( const char* pszBbsListLine )
{
    WFile file( syscfg.gfilesdir, BBSLIST_MSG );
    bool bOpen = file.Open( WFile::modeReadWrite | WFile::modeCreateFile | WFile::modeBinary, WFile::shareUnknown, WFile::permReadWrite );
    if ( bOpen && file.GetLength() > 0 )
    {
        file.Seek( -1L, WFile::seekEnd );
        char chLastChar = 0;
        file.Read( &chLastChar, 1 );
        if ( chLastChar == CZ )
        {
            // If last char is a EOF, skip it.
            file.Seek( -1L, WFile::seekEnd );
        }
    }
    // we cast away const'ness however WFile::Write doesn't modify the parameter
    file.Write( const_cast<char*>( pszBbsListLine ), strlen( pszBbsListLine ) );
    file.Close();
}


void AddBBSListEntryImpl()
{
	sess->bout << "\r\nPlease enter phone number:\r\n ###-###-####\r\n:";
    std::string bbsPhoneNumber;
    input( bbsPhoneNumber, 12, true );
    if ( IsBBSPhoneNumberValid( bbsPhoneNumber.c_str() ) )
    {
        if ( IsBBSPhoneNumberUnique( bbsPhoneNumber.c_str() ) )
        {
            std::string bbsName, bbsSpeed, bbsType;
            sess->bout << "|13This number can be added! It is not yet in BBS list.\r\n\n\n"
                       << "|#7Enter the BBS name and comments about it (incl. V.32/HST) :\r\n:";
            inputl( bbsName, 50, true );
            sess->bout << "\r\n|#7Enter maximum speed of the BBS:\r\n"
				       << "|#7(|#1example: 14.4,28.8, 33.6, 56k|#7)\r\n:";
            input( bbsSpeed, 4, true );
			sess->bout << "\r\n|#7Enter BBS type (ie, |#1WWIV|#7):\r\n:";
            input( bbsType, 4, true );

            char szBbsListLine[ 255 ];
            sprintf( szBbsListLine, "%12s  %-50s  [%4s] (%4s)\r\n",
                     bbsPhoneNumber.c_str(), bbsName.c_str(), bbsSpeed.c_str(), bbsType.c_str() );
            nl( 2 );
            sess->bout << szBbsListLine;
            nl( 2 );
            sess->bout << "|10Is this information correct? ";
            if ( yesno() )
            {
                AddBBSListLine( szBbsListLine );
                sess->bout << "\r\n|13This entry was added to BBS list.\r\n";
            }
			nl();
        }
        else
        {
            sess->bout << "|12Sorry, It's already in the BBS list.\r\n\n\n";
        }
    }
    else
    {
        sess->bout << "\r\n|12 Error: Please enter number in correct format.\r\n\n";
    }
}


void AddBBSListEntry()
{
    if ( sess->GetEffectiveSl() <= 10 )
    {
        sess->bout << "\r\n\nYou must be a validated user to add to the BBS list.\r\n\n";
    }
    else if ( sess->thisuser.isRestrictionAutomessage() )
    {
        sess->bout << "\r\n\nYou can not add to the BBS list.\r\n\n\n";
    }
    else
    {
        AddBBSListEntryImpl();
    }

}


void DeleteBBSListEntry()
{
    sess->bout << "\r\n|#7Please enter phone number in the following format:\r\n";
	sess->bout << "|#1 ###-###-####\r\n:";
    std::string bbsPhoneNumber;
    input( bbsPhoneNumber, 12, true );
    if ( bbsPhoneNumber[3] != '-' || bbsPhoneNumber[7] != '-' )
    {
        CLEAR_STRING( bbsPhoneNumber );
    }
    if ( bbsPhoneNumber.length() == 12 )
    {
        bool ok = false;
        char szTempFileName[ MAX_PATH ];
        char szFileName[ MAX_PATH ];
        sprintf(szFileName, "%s%s", syscfg.gfilesdir, BBSLIST_MSG);
        sprintf(szTempFileName, "%s%s", syscfg.gfilesdir, BBSLIST_TMP);
        FILE* fi = fopen(szFileName, "r");
        if (fi)
        {
            FILE* fo = fopen(szTempFileName, "w");
            if (fo)
            {
                char szLine[ 255 ];
                while (fgets(szLine, sizeof(szLine), fi))
                {
                    if ( strstr( szLine, bbsPhoneNumber.c_str() ) )
                    {
                        ok = true;
                    }
                    else
                    {
                        fprintf(fo, "%s", szLine);
                    }
                }
                fclose(fo);
            }
            fclose(fi);
        }
        nl();
        if (ok)
        {
            WFile::Remove(szFileName);
            WFile::Rename(szTempFileName, szFileName);
            sess->bout << "|#7* |#1Number removed.\r\n";
        }
        else
        {
            WFile::Remove(szTempFileName);
            sess->bout << "|12Error: Couldn't find that in the bbslist file.\r\n";
		}
    }
    else
    {
        sess->bout << "\r\n|12Error: Please enter number in correct format.\r\n\n";
    }
}


void BBSList()
{
    bool done = false;
    do
    {
        char chInput = ShowBBSListMenuAndGetChoice();
        switch ( chInput )
        {
        case 'Q':
            done = true;
            break;
        case 'R':
            printfile( BBSLIST_MSG, true, true );
            break;
        case 'N':
            print_net_listing( false );
            break;
        case 'A':
            AddBBSListEntry();
            break;
        case 'D':
            DeleteBBSListEntry();
            break;
        }
    } while ( !done && !hangup );
}


