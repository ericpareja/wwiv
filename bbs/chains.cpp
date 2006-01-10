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


//////////////////////////////////////////////////////////////////////////////
//
//
// module private functions
//
//

void show_chains(int *mapp, int *map);


//////////////////////////////////////////////////////////////////////////////
//
// Implementation
//
//
//



//////////////////////////////////////////////////////////////////////////////
//
// Displays the list of chains to a user
//

void show_chains(int *mapp, int *map)
{
    char szBuffer[ 255 ];

    ansic( 0 );
    ClearScreen();
    nl();
    bool abort = false;
    bool next = false;
    if ( app->HasConfigFlag( OP_FLAGS_CHAIN_REG ) && chains_reg )
    {
        sprintf( szBuffer, "|#5  Num |#1%-42.42s|#2%-22.22s|#1%-5.5s", "Description", "Sponsored by", "Usage" );
        pla( szBuffer, &abort );

        if ( okansi() )
        {
            sprintf( szBuffer, "|#%d %s", FRAME_COLOR, "�������������������������������������������������������������������������ͻ" );
        }
        else
        {
            sprintf( szBuffer, " +---+-----------------------------------------+---------------------+-----+" );
        }
        pla( szBuffer, &abort );
        for ( int i = 0; i < *mapp && !abort && !hangup; i++ )
        {
            WUser user;
            strcat( szBuffer, ". " );
            if ( okansi() )
            {
                app->userManager->ReadUser( &user, chains_reg[map[i]].regby[0] );
                sprintf( szBuffer, " |#%d�|10%3d|#%d�|#1%-41s|#%d�|%2.2d%-21s|#%d�|#1%5d|#%d�",
						 FRAME_COLOR,
						 i + 1,
						 FRAME_COLOR,
						 chains[map[i]].description,
						 FRAME_COLOR,
						 (chains_reg[map[i]].regby[0]) ? 14 : 13,
						 (chains_reg[map[i]].regby[0]) ? user.GetName() : "Available",
						 FRAME_COLOR,
						 chains_reg[map[i]].usage,
						 FRAME_COLOR );
                pla( szBuffer, &abort );
                if ( chains_reg[map[i]].regby[0] != 0 )
                {
                    for ( int i1 = 1; i1 < 5 && !abort; i1++ )
                    {
                        if ( chains_reg[map[i]].regby[i1] != 0 )
                        {
                            app->userManager->ReadUser( &user, chains_reg[map[i]].regby[i1] );
                            sprintf( szBuffer, " |#%d�   �%-41s�|#2%-21s|#%�%5.5s�",
                                     FRAME_COLOR, " ", user.GetName(), FRAME_COLOR, " " );
                            pla( szBuffer, &abort );
                        }
                    }
                }
            }
            else
            {
                app->userManager->ReadUser( &user, chains_reg[map[i]].regby[0] );
                sprintf( szBuffer, " |%3d|%-41.41s|%-21.21s|%5d|",
						 i + 1, chains[map[i]].description,
						 ( chains_reg[map[i]].regby[0] ) ? user.GetName() : "Available",
						 chains_reg[map[i]].usage );
                pla( szBuffer, &abort );
                if ( chains_reg[map[i]].regby[0] != 0 )
                {
                    for ( int i1 = 1; i1 < 5; i1++ )
                    {
                        if ( chains_reg[map[i]].regby[i1] != 0 )
                        {
                            app->userManager->ReadUser( &user, chains_reg[map[i]].regby[i1] );
                            sprintf( szBuffer, " |   |                                         |%-21.21s|     |",
									 (chains_reg[map[i]].regby[i1]) ? user.GetName() : "Available" );
                            pla( szBuffer, &abort );
                        }
                    }
                }
            }
        }
        if ( okansi() )
        {
            sprintf( szBuffer, "|#%d %s", FRAME_COLOR, "�������������������������������������������������������������������������ͼ" );
        }
        else
        {
            sprintf( szBuffer, " +---+-----------------------------------------+---------------------+-----+" );
        }
        pla( szBuffer, &abort );

    }
    else
    {
        sprintf( szBuffer, " [ %s Online Programs ] ", syscfg.systemname );
        DisplayLiteBar( szBuffer );
        sess->bout << "|#7���������������������������������������������������������������������������Ļ\r\n";
        for ( int i = 0; i < *mapp && !abort && !hangup; i++ )
        {
            sprintf( szBuffer, "|#7�|#2%2d|#7� |#1%-33.33s|#7�", i + 1, chains[map[i]].description );
            osan( szBuffer, &abort, &next );
            i++;
			if ( !abort && !hangup )
			{
				char szBuffer[ 255 ];
				if ( i >= *mapp )
				{
					sprintf( szBuffer, "  |#7�                                  |#7�" );
				}
				else
				{
					sprintf( szBuffer, "|#2%2d|#7� |#1%-33.33s|#7�", i + 1,chains[map[i]].description );
				}
				pla( szBuffer, &abort );
			}
        }
        sess->bout << "|#7���������������������������������������������������������������������������ļ\r\n\n";
    }
}


//////////////////////////////////////////////////////////////////////////////
//
// Executes a "chain", index number nChainNumber.
//


void run_chain( int nChainNumber )
{
    int inst = inst_ok( INST_LOC_CHAINS, nChainNumber + 1 );
    if ( inst != 0 )
    {
        char szMessage[255];
        sprintf( szMessage, "|#2Chain %s is in use on instance %d", chains[nChainNumber].description, inst );
        if ( !( chains[nChainNumber].ansir & ansir_multi_user ) )
        {
            strcat( szMessage, "Try again later.\r\n" );
            sess->bout << szMessage;
            return;
        }
        else
        {
            strcat( szMessage, "Care to join in? " );
            sess->bout << szMessage;
            if ( !yesno() )
            {
                return;
            }
        }
    }
    write_inst( INST_LOC_CHAINS, static_cast< unsigned short >( nChainNumber + 1 ), INST_FLAGS_NONE );
    if ( app->HasConfigFlag( OP_FLAGS_CHAIN_REG ) && chains_reg )
    {
        chains_reg[nChainNumber].usage++;
        WFile regFile( syscfg.datadir, CHAINS_REG );
        if ( regFile.Open( WFile::modeReadWrite|WFile::modeBinary|WFile::modeCreateFile|WFile::modeTruncate, WFile::shareUnknown, WFile::permReadWrite ) )
        {
            regFile.Write( chains_reg, sess->GetNumberOfChains() * sizeof( chainregrec ) );
        }
    }
    char szComSpeed[ 11 ];
    sprintf( szComSpeed, "%d", com_speed );
    if ( com_speed == 1 )
    {
        strcpy( szComSpeed, "115200" );
    }
    char szComPortNum[ 11 ];
    sprintf( szComPortNum, "%d", syscfgovr.primaryport );

    char szModemSpeed[ 11 ];
    sprintf( szModemSpeed, "%d", modem_speed );

    char szChainCmdLine[ MAX_PATH ];
    stuff_in( szChainCmdLine, chains[nChainNumber].filename, create_chain_file(), szComSpeed, szComPortNum, szModemSpeed, "" );

    sysoplogf( "!Ran \"%s\"", chains[nChainNumber].description );
    sess->thisuser.SetNumChainsRun( sess->thisuser.GetNumChainsRun() + 1 );

    unsigned short flags = 0;
    if ( !( chains[nChainNumber].ansir & ansir_no_DOS ) )
    {
        flags |= EFLAG_COMIO;
    }
    if ( chains[nChainNumber].ansir & ansir_no_pause )
    {
        flags |= EFLAG_NOPAUSE;
    }
    if ( chains[nChainNumber].ansir & ansir_emulate_fossil )
    {
        flags |= EFLAG_FOSSIL;
    }

    ExecuteExternalProgram( szChainCmdLine, flags );
    write_inst( INST_LOC_CHAINS, 0, INST_FLAGS_NONE );
    app->localIO->UpdateTopScreen();
}


//////////////////////////////////////////////////////////////////////////////
//
// Main high-level function for chain access and execution.
//


void do_chains()
{
    int *map = static_cast<int*>( BbsAllocA(sess->max_chains * sizeof( int ) ) );
    WWIV_ASSERT( map != NULL );
    if ( !map )
	{
        return;
	}

    app->localIO->tleft( true );
    int mapp = 0;
    memset( odc, 0, sizeof( odc ) );
    for ( int i = 0; i < sess->GetNumberOfChains(); i++ )
    {
        bool ok = true;
        chainfilerec c = chains[i];
        if ( ( c.ansir & ansir_ansi ) && !okansi() )
		{
            ok = false;
		}
        if ( ( c.ansir & ansir_local_only ) && sess->using_modem )
		{
            ok = false;
		}
        if ( c.sl > sess->GetEffectiveSl() )
		{
            ok = false;
		}
        if ( c.ar && !sess->thisuser.hasArFlag( c.ar ) )
		{
            ok = false;
		}
        if ( app->HasConfigFlag( OP_FLAGS_CHAIN_REG ) && chains_reg && ( sess->GetEffectiveSl() < 255 ) )
        {
			chainregrec r = chains_reg[ i ];
            if ( r.maxage )
            {
                if ( r.minage > sess->thisuser.GetAge() || r.maxage < sess->thisuser.GetAge() )
				{
                    ok = false;
				}
            }
        }
        if ( ok )
        {
            map[ mapp++ ] = i;
            if ( mapp < 100 )
            {
                if ( ( mapp % 10 ) == 0 )
				{
                    odc[mapp / 10 - 1] = static_cast< char > ( '0' + ( mapp / 10 ) );
				}
            }
        }
    }
    if ( mapp == 0 )
    {
        sess->bout << "\r\n\n|#5Sorry, no external programs available.\r\n";
        BbsFreeMemory( map );
        sess->SetMMKeyArea( WSession::mmkeyMessageAreas );
        return;
    }
    show_chains( &mapp, map );

    bool done		= false;
    sess->SetMMKeyArea( WSession::mmkeyMessageAreas );
    int start		= 0;
    char *ss		= NULL;

    do
    {
        sess->SetMMKeyArea( WSession::mmkeyChains );
        app->localIO->tleft( true );
        nl();
        sess->bout << "|#7Which chain (1-" << mapp << ", Q=Quit, ?=List): ";

		int nChainNumber = -1;
        if ( mapp < 100 )
        {
            ss = mmkey( 2 );
            nChainNumber = atoi( ss );
        }
        else
        {
			char szChainNumber[ 11 ];
            input( szChainNumber, 3 );
            nChainNumber = atoi( szChainNumber );
        }
        if ( nChainNumber > 0 && nChainNumber <= mapp )
        {
            done = true;
            sess->SetMMKeyArea( WSession::mmkeyChains );
            sess->bout << "\r\n|#6Please wait...\r\n";
            run_chain( map[ nChainNumber - 1 ] );
        }
        else if ( wwiv::stringUtils::IsEquals(ss, "Q") )
        {
            sess->SetMMKeyArea( WSession::mmkeyMessageAreas );
            done = true;
        }
        else if ( wwiv::stringUtils::IsEquals(ss, "?") )
        {
            show_chains(&mapp, map);
        }
        else if ( wwiv::stringUtils::IsEquals(ss, "P") )
        {
            if (start > 0)
            {
                start -= 14;
            }
			start = std::max<int>( start, 0 );
        }
        else if ( wwiv::stringUtils::IsEquals(ss, "N") )
        {
            if (start + 14 < mapp)
            {
                start += 14;
            }
        }
    } while ( !hangup  && !done );

    BbsFreeMemory( map );
}

