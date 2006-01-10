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


void showchains();
void modify_chain( int nCurrentChainNumber );
void insert_chain( int nCurrentChainNumber );
void delete_chain( int nCurrentChainNumber );
void chaindata( int nCurrentChainNumber, char *pszData );


void chaindata( int nCurrentChainNumber, char *pszData )
{
    chainfilerec c = chains[ nCurrentChainNumber ];
    char chAr = SPACE;

    if (c.ar != 0)
    {
        for (int i = 0; i < 16; i++)
        {
            if ((1 << i) & c.ar)
            {
                chAr = static_cast< char >( 'A' + i );
            }
        }
    }

    char chAnsiReq = ( c.ansir & ansir_ansi ) ? 'Y' : 'N';
    sprintf( pszData, 
             "|#2%2d |#1%-28.28s  |#2%-30.30s |#9%-3d    %1c  %1c",
             nCurrentChainNumber, 
             stripcolors( c.description ),
             c.filename, 
             c.sl, 
             chAnsiReq, 
             chAr );
}


void showchains()
{
    char szChainData[255];

    ClearScreen();
    bool abort = false;
    pla("|#2NN Description                   Path Name                      SL  ANSI AR", &abort);
    pla("|#7== ----------------------------  ============================== --- ==== --", &abort);
    for (int nChainNum = 0; nChainNum < sess->GetNumberOfChains() && !abort; nChainNum++)
    {
        chaindata( nChainNum, szChainData );
        pla( szChainData, &abort );
    }
}


void ShowChainCommandLineHelp()
{
    sess->bout << "|#2  Macro   Value\r\n";
    sess->bout << "|#7 ======== =======================================\r\n";
    sess->bout << "|#1   %     |#9 A single \'%\' Character\r\n";
    sess->bout << "|#1   %1    |#9 CHAIN.TXT full pathname (legacy parameter)\r\n";
    sess->bout << "|#1   %A    |#9 CALLINFO.BBS full pathname \r\n";
    sess->bout << "|#1   %C    |#9 CHAIN.TXT full pathname \r\n";
    sess->bout << "|#1   %D    |#9 DORIFOx.DEF full pathname \r\n";
    sess->bout << "|#1   %E    |#9 DOOR32.SYS full pathname \r\n";
    sess->bout << "|#1   %K    |#9 GFiles Comment File For Archives\r\n";
    sess->bout << "|#1   %M    |#9 Modem Baud Rate\r\n";
    sess->bout << "|#1   %N    |#9 Node (Instance) number\r\n";
    sess->bout << "|#1   %O    |#9 PCBOARD.SYS full pathname\r\n";
    sess->bout << "|#1   %P    |#9 ComPort Number\r\n";
    sess->bout << "|#1   %R    |#9 DOOR.SYS Full Pathname\r\n";
    sess->bout << "|#1   %S    |#9 Com Port Baud Rate\r\n";
    sess->bout << "|#1   %T    |#9 Minutes Remaining\r\n";
    nl();
}


void modify_chain( int nCurrentChainNumber )
{
    chainregrec r;
    char s[255], s1[255], ch, ch2;
    char szChainNum[81];
    int i, nUserNumber;
    memset( &r, 0, sizeof( chainregrec ) );

    chainfilerec c = chains[ nCurrentChainNumber ];
    if ( app->HasConfigFlag( OP_FLAGS_CHAIN_REG ) )
    {
        r = chains_reg[ nCurrentChainNumber ];
    }
    bool done = false;
    do
    {
        ClearScreen();
        sprintf( szChainNum, "|B1|15Editing Chain # %d", nCurrentChainNumber );
        bprintf( "%-85s", szChainNum );
        nl( 2 );
        ansic( 0 );
        sess->bout << "|#9A) Description  : |#2" << c.description << wwiv::endl;
        sess->bout << "|#9B) Filename     : |#2" << c.filename << wwiv::endl;
        sess->bout << "|#9C) SL           : |#2" << static_cast<int>( c.sl ) << wwiv::endl;
        strcpy(s, "None.");
        if (c.ar != 0)
        {
            for (i = 0; i < 16; i++)
            {
                if ((1 << i) & c.ar)
                {
                    s[0] = static_cast< char >( 'A' + i );
                }
            }
            s[1] = 0;
        }
        sess->bout << "|#9D) AR           : |#2" << s << wwiv::endl;
        sess->bout << "|#9E) ANSI         : |#2" << ( ( c.ansir & ansir_ansi ) ? "|12Required" : "|#1Optional" ) << wwiv::endl;
        sess->bout << "|#9F) DOS Interrupt: |#2" << ( ( c.ansir & ansir_no_DOS ) ? "NOT Used" : "Used" ) << wwiv::endl;
        sess->bout << "|#9G) Win32 FOSSIL : |#2" << YesNoString( ( c.ansir & ansir_emulate_fossil ) ? true : false ) << wwiv::endl;
        //sess->bout << "|#9I) Disable pause: |#2%s\r\n", YesNoString( ( c.ansir & ansir_no_pause ) ) << wwiv::endl;
        sess->bout << "|#9J) Local only   : |#2" << YesNoString( ( c.ansir & ansir_local_only ) ? true : false ) << wwiv::endl;
        sess->bout << "|#9K) Multi user   : |#2" << YesNoString( ( c.ansir & ansir_multi_user ) ? true : false ) << wwiv::endl;
        if ( app->HasConfigFlag( OP_FLAGS_CHAIN_REG ) )
        {
            WUser regUser;
            if ( r.regby[0] )
            {
                app->userManager->ReadUser( &regUser, r.regby[0] );
            }
            sess->bout << "|#9L) Registered by: |#2" << ( ( r.regby[0] ) ? regUser.GetName() : "AVAILABLE" ) << wwiv::endl;
            for (i = 1; i < 5; i++)
            {
                if ( r.regby[i] != 0 )
                {
                    app->userManager->ReadUser( &regUser, r.regby[i] );
                    sess->bout << charstr(18, ' ') << regUser.GetName() << wwiv::endl;
                }
            }
            sess->bout << "|#9M) Usage        : |#2" << r.usage << wwiv::endl;
            if ( r.maxage == 0 && r.minage == 0 )
            {
                r.maxage = 255;
            }
            sess->bout << "|#9N) Age limit    : |#2" << static_cast<int>( r.minage ) << " - " << static_cast<int>( r.maxage ) << wwiv::endl;
            nl();
            sess->bout << "|#7(|#2Q|#7=|#1Quit|#7) Which (|#1A|#7-|#1N|#7,|#1R|#7,|#1[|#7,|#1]|#7) : ";
            ch = onek( "QABCDEFGJKLMN[]", true );   // removed i
        }
        else
        {
            nl();
            sess->bout << "|#9Which (A-K,R,[,],Q) ? ";
            ch = onek( "QABCDEFGJK[]", true ); // removed i
        }
        switch (ch)
        {
        case 'Q':
            done = true;
            break;
        case '[':
            chains[ nCurrentChainNumber ] = c;
            if ( app->HasConfigFlag( OP_FLAGS_CHAIN_REG ) )
            {
                chains_reg[ nCurrentChainNumber ] = r;
            }
            if ( --nCurrentChainNumber < 0 )
            {
                nCurrentChainNumber = sess->GetNumberOfChains() - 1;
                sess->SetNumberOfChains( nCurrentChainNumber );
            }
            c = chains[ nCurrentChainNumber ];
            if ( app->HasConfigFlag( OP_FLAGS_CHAIN_REG ) )
            {
                r = chains_reg[ nCurrentChainNumber ];
            }
            break;
        case ']':
            chains[ nCurrentChainNumber ] = c;
            if ( app->HasConfigFlag( OP_FLAGS_CHAIN_REG ) )
            {
                chains_reg[ nCurrentChainNumber ] = r;
            }
            if ( ++nCurrentChainNumber >= sess->GetNumberOfChains() )
            {
                nCurrentChainNumber = 0;
            }
            c = chains[ nCurrentChainNumber ];
            if ( app->HasConfigFlag( OP_FLAGS_CHAIN_REG ) )
            {
                r = chains_reg[ nCurrentChainNumber ];
            }
            break;
        case 'A':
            nl();
            sess->bout << "|#7New Description? ";
            Input1( s, c.description, 40, true, MIXED );
            if (s[0])
            {
                strcpy(c.description, s);
            }
            break;
        case 'B':
            ClearScreen();
            ShowChainCommandLineHelp();
            sess->bout << "\r\n|#9Enter Command Line.\r\n|#7:";
            Input1( s, c.filename, 79, true, MIXED );
            if (s[0] != 0)
            {
                strcpy(c.filename, s);
            }
            break;
        case 'C':
            nl();
            sess->bout << "|#7New SL? ";
            input( s, 3, true );
            i = atoi(s);
            if ((i >= 0) && (i < 256) && (s[0]))
            {
                c.sl = static_cast< unsigned char > ( i );
            }
            break;
        case 'D':
            nl();
            sess->bout << "|#7New AR (<SPC>=None) ? ";
            ch2 = onek(" ABCDEFGHIJKLMNOP");
            if (ch2 == SPACE)
            {
                c.ar = 0;
            }
            else
            {
                c.ar = static_cast< unsigned short >( 1 << ( ch2 - 'A' ) );
            }
            break;
        case 'E':
            nl();
            sess->bout << "|#5Require ANSI? ";
            if (yesno())
            {
                c.ansir |= ansir_ansi;
            }
            else
            {
                c.ansir &= ~ansir_ansi;
            }
            break;
            case 'F':
            nl();
            sess->bout << "|#5Have BBS intercept DOS calls? ";
            if (noyes())
            {
                c.ansir &= ~ansir_no_DOS;
            }
            else
            {
                c.ansir |= ansir_no_DOS;
            }
            break;
        case 'G':
            nl();
            sess->bout << "|#5Under Windows Use Emulated FOSSIL Support? ";
            if (noyes())
            {
                c.ansir |= ansir_emulate_fossil;
            }
            else
            {
                c.ansir &= ~ansir_emulate_fossil;
            }
            break;
/*
        case 'I':
            nl();
            sess->bout << "|#5Disable screen pause in program? ";
            if (yesno())
            {
                c.ansir |= ansir_no_pause;
            }
            else
            {
                c.ansir &= ~ansir_no_pause;
            }
            break;
*/
        case 'J':
            nl();
            sess->bout << "|#5Allow program to be run locally only? ";
            if (yesno())
            {
                c.ansir |= ansir_local_only;
            }
            else
            {
                c.ansir &= ~ansir_local_only;
            }
            break;
        case 'K':
            nl();
            sess->bout << "|#5Chain is multi-user? ";
            if (yesno())
            {
                c.ansir |= ansir_multi_user;
            }
            else
            {
                c.ansir &= ~ansir_multi_user;
            }
            break;
        case 'L':
            if ( !app->HasConfigFlag( OP_FLAGS_CHAIN_REG ) )
            {
                break;
            }
            for (i = 0; i < 5; i++)
            {
                nl();
                sess->bout << "|#9(Q=Quit, 0=None) User name/number: : ";
                input( s1, 30, true );
                if ( s1[0] != 'Q' && s1[0] != 'q' )
                {
                    if (s1[0] == '0')
                    {
                        r.regby[i] = 0;
                    }
                    else
                    {
                        nUserNumber = finduser1(s1);
                        if (nUserNumber > 0)
                        {
                            WUser regUser;
                            app->userManager->ReadUser( &regUser, nUserNumber );
                            r.regby[i] = static_cast< short > ( nUserNumber );
                            nl();
                            sess->bout << "|#1Registered by       |#2" << nUserNumber << " " << ( ( r.regby[i] ) ? regUser.GetName() : "AVAILABLE" );
                        }
                    }
                }
                else
                {
                    break;
                }
            }
            break;
        case 'M':
            r.usage = 0;
            nl();
            sess->bout << "|#5Times Run : ";
            input(s, 3);
            if (s[0] != 0)
            {
                r.usage = static_cast< short >( atoi(s) );
            }
            break;
        case 'N':
            nl();
            sess->bout << "|#5New minimum age? ";
            input(s, 3);
            if (s[0])
            {
                if ((atoi(s) > 255) || (atoi(s) < 0))
                {
                    s[0] = 0;
                }
                else
                {
                    r.minage = wwiv::stringUtils::StringToUnsignedChar(s);
                }
                sess->bout << "|#5New maximum age? ";
                input(s, 3);
                if (s[0])
                {
                    if (atoi(s) < r.minage)
                    {
                        break;
                    }
                    if (atoi(s) > 255)
                    {
                        r.maxage = 255;
                    }
                    else
                    {
                        r.maxage = wwiv::stringUtils::StringToUnsignedChar(s);
                    }
                }
            }
            break;
        }
    } while ( !done && !hangup );
    chains[ nCurrentChainNumber ] = c;
    if ( app->HasConfigFlag( OP_FLAGS_CHAIN_REG ) )
    {
        chains_reg[ nCurrentChainNumber ] = r;
    }
}


void insert_chain( int nCurrentChainNumber )
{
    chainfilerec c;
    chainregrec r;

    for ( int i = sess->GetNumberOfChains() - 1; i >= nCurrentChainNumber; i-- )
    {
        chains[i + 1] = chains[i];
        if ( app->HasConfigFlag( OP_FLAGS_CHAIN_REG ) )
        {
            chains_reg[i + 1] = chains_reg[i];
        }
    }
    strcpy(c.description, "** NEW CHAIN **");
    strcpy(c.filename, "REM");
    c.sl = 10;
    c.ar = 0;
    c.ansir = 0;
    chains[ nCurrentChainNumber ] = c;
    c.ansir |= ansir_no_DOS;
    sess->SetNumberOfChains( sess->GetNumberOfChains() + 1 );
    if ( app->HasConfigFlag( OP_FLAGS_CHAIN_REG ) )
    {
        memset(&r, 0, sizeof(r));
        r.maxage = 255;
        chains_reg[ nCurrentChainNumber ] = r;
    }
    modify_chain( nCurrentChainNumber );
}


void delete_chain( int nCurrentChainNumber )
{
    for ( int i = nCurrentChainNumber; i < sess->GetNumberOfChains(); i++ )
    {
        chains[i] = chains[i + 1];
        if ( app->HasConfigFlag( OP_FLAGS_CHAIN_REG ) )
        {
            chains_reg[i] = chains_reg[i + 1];
        }
    }
    sess->SetNumberOfChains( sess->GetNumberOfChains() - 1 );
}


void chainedit()
{
    int i;
    char s[81];

    if (!ValidateSysopPassword())
    {
        return;
    }
    showchains();
    bool done = false;
    do
    {
        nl();
        sess->bout << "|#7Chains: (D)elete, (I)nsert, (M)odify, (Q)uit, ? : ";
        char ch = onek("QDIM?");
        switch (ch) {
        case '?':
            showchains();
            break;
        case 'Q':
            done = true;
            break;
        case 'M':
            nl();
            sess->bout << "|#2Chain number? ";
            input(s, 2);
            i = atoi(s);
            if ( s[0] != '\0' && i >= 0 && i < sess->GetNumberOfChains() )
            {
                modify_chain( i );
            }
            break;
        case 'I':
            if ( sess->GetNumberOfChains() < sess->max_chains )
            {
                nl();
                sess->bout << "|#2Insert before which chain? ";
                input(s, 2);
                i = atoi(s);
                if ( s[0] != '\0' && i >= 0 && i <= sess->GetNumberOfChains() )
                {
                    insert_chain(i);
                }
            }
            break;
        case 'D':
            nl();
            sess->bout << "|#2Delete which chain? ";
            input(s, 2);
            i = atoi(s);
            if ( s[0] != '\0' && i >= 0 && i < sess->GetNumberOfChains() )
            {
                nl();
                sess->bout << "|10Delete " << chains[i].description << "? ";
                if (yesno())
                {
                    delete_chain(i);
                }
            }
            break;
        }
    } while ( !done && !hangup );

    WFile chainsFile( syscfg.datadir, CHAINS_DAT );
    if ( chainsFile.Open( WFile::modeReadWrite|WFile::modeBinary|WFile::modeCreateFile, WFile::shareUnknown, WFile::permReadWrite ) )
    {
        chainsFile.Write( chains, sess->GetNumberOfChains() * sizeof( chainfilerec ) );
        chainsFile.Close();
    }
    if ( app->HasConfigFlag( OP_FLAGS_CHAIN_REG ) )
    {
        WFile regFile( syscfg.datadir, CHAINS_REG );
        if ( regFile.Open( WFile::modeReadWrite|WFile::modeBinary|WFile::modeCreateFile|WFile::modeTruncate, WFile::shareUnknown, WFile::permReadWrite ) )
        {
            regFile.Write( chains_reg, sess->GetNumberOfChains() * sizeof( chainregrec ) );
            regFile.Close();
        }
    }
}
