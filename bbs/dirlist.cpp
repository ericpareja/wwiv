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
// Implementation
//
//
//


/** Displays the available file areas for the current user. */
void dirlist( int mode )
{
    bool next   = false;
    int oc      = sess->GetCurrentConferenceFileArea();
    int os      = udir[sess->GetCurrentFileArea()].subnum;
    int tally   = 0;
    int nd      = 0;
    int sn      = sess->GetCurrentConferenceFileArea();
    int en      = sess->GetCurrentConferenceFileArea();
	bool done   = false;

    do
    {
        bool is     = false;
        bool abort  = false;
        int p       = 1;
        int i       = sn;
        char *ss    = NULL;

        while ( i <= en && uconfdir[i].confnum != -1 && !abort )
        {
            int i1 = 0;
            while ( i1 < sess->num_dirs && udir[i1].subnum != -1 && !abort )
            {
                char s[ 255 ];
                int firstp = 0;
                if ( p && mode == 0 )
                {
                    p = 0;
                    firstp = i1;
                    ClearScreen();
                    if ( uconfdir[1].confnum != -1 && okconf( &sess->thisuser ) )
                    {
                        sprintf( s, " [ %s %c ] [ %s ] ", "Conference",
                                dirconfs[uconfdir[i].confnum].designator,
                                stripcolors( reinterpret_cast<char*>( dirconfs[uconfdir[i].confnum].name ) ) );
                    }
                    else
                    {
                        sprintf( s, " [ %s File Areas ] ", syscfg.systemname );
                    }
                    DisplayLiteBar( s );
                    DisplayHorizontalBar( 78, 7 );
                    sess->bout << "|#2 Dir Qscan?     Directory Name                          Total Files\r\n";
                    DisplayHorizontalBar( 78, 7 );
                }
                ++nd;
                int nDirectoryNumber = udir[i1].subnum;
                if ( nDirectoryNumber == 0 )
                {
                    is = true;
                }
                std::string scanme = "|12No ";
                if ( qsc_n[ nDirectoryNumber / 32 ] & ( 1L << ( nDirectoryNumber % 32 ) ) )
                {
                    scanme = "|10Yes";
                }
                dliscan1( nDirectoryNumber );
                if ( udir[sess->GetCurrentFileArea()].subnum == udir[i1].subnum )
                {
                    sprintf( s, " |#9%3s |#9� |12%3s |#9�|B1|15 %-40.40s |#9� |#9%4ld|B0",
                        udir[i1].keys, scanme.c_str(), directories[ nDirectoryNumber ].name, sess->numf );
                }
                else
                {
                    sprintf( s, " |#9%3s |#9� |12%3s |#9� %s%-40.40s |#9� |#9%4ld",
                            udir[i1].keys, scanme.c_str(),
                            (((mode == 1) && (directories[udir[i1].subnum].mask & mask_cdrom)) ? "|#9" : "|#1"),
                            directories[ nDirectoryNumber ].name, sess->numf );
                }
                if ( okansi() )
                {
                    osan( s, &abort, &next );
                }
                else
                {
                    osan( stripcolors( s ), &abort, &next );
                }
                tally += sess->numf;
                int lastp = i1++;
                nl();
                if ( lines_listed >= sess->screenlinest - 2 && mode == 0 )
                {
                    p = 1;
                    lines_listed = 0;
                    DisplayHorizontalBar( 78, 7 );
                    bprintf( "|#1Select |#9[|#2%d-%d, [Enter]=Next Page, Q=Quit|#9]|#0 : ", is ? firstp : firstp + 1, lastp );
                    ss = mmkey( 1, true );
                    if ( isdigit( ss[0] ) )
                    {
                        for ( int i3 = 0; i3 < sess->num_dirs; i3++ )
                        {
                            if ( wwiv::stringUtils::IsEquals(udir[i3].keys, ss) )
                            {
                                sess->SetCurrentFileArea( i3 );
                                os      = udir[sess->GetCurrentFileArea()].subnum;
                                done    = true;
                                abort   = true;
                            }
                        }
                    }
                    else
                    {
                        switch ( ss[0] )
                        {
                        case 'Q':
                            if ( okconf( &sess->thisuser) )
                            {
                                setuconf(CONF_DIRS, oc, os);
                            }
                            done    = true;
                            abort   = true;
                            break;
                        default:
                            BackLine();
                            break;
                        }
                    }
                }
            }
            if (nd)
            {
                i++;
            }
            if ( !okconf( &sess->thisuser ) )
            {
                break;
            }
        }
        if ( i == 0 )
        {
            pla("None.", &abort);
            nl();
        }
        if ( !abort && mode == 0 )
        {
            p = 1;
            DisplayHorizontalBar( 78, 7 );
            if ( okconf( &sess->thisuser ) )
            {
                if (uconfdir[1].confnum != -1)
                {
                    bprintf("|#1Select |#9[|#2%d-%d, J=Join Conference, ?=List Again, Q=Quit|#9]|#0 : ", is ? 0 : 1, is ? nd - 1 : nd);
                }
                else
                {
                    bprintf("|#1Select |#9[|#2%d-%d, ?=List Again, Q=Quit|#9]|#0 : ", is ? 0 : 1, is ? nd - 1 : nd);
                }
            }
            else
            {
                bprintf("|#1Select |#9[|#2%d-%d, ?=List Again, Q=Quit|#9]|#0 : ", is ? 0 : 1, is ? nd - 1 : nd);
            }
            ss = mmkey( 0, true );
            if ( wwiv::stringUtils::IsEquals( ss, "" ) ||
                 wwiv::stringUtils::IsEquals( ss, "Q" ) ||
                 wwiv::stringUtils::IsEquals( ss, "\r" ) )
            {
                if ( okconf( &sess->thisuser ) )
                {
                    setuconf(CONF_DIRS, oc, os);
                }
                done = true;
            }
            if ( wwiv::stringUtils::IsEquals( ss, "J" ) )
            {
                if ( okconf( &sess->thisuser ) )
                {
                    jump_conf(CONF_DIRS);
                }
                sn = en = oc = sess->GetCurrentConferenceFileArea();
                nd = i = 0;
                is = false;
            }
            if ( isdigit( ss[0] ) )
            {
                for ( int i3 = 0; i3 < sess->num_dirs; i3++ )
                {
                    if ( wwiv::stringUtils::IsEquals( udir[i3].keys, ss ) )
                    {
                        sess->SetCurrentFileArea( i3 );
                        os = udir[sess->GetCurrentFileArea()].subnum;
                        done = true;
                    }
                }
            }
            nd = 0;
        }
        else
        {
            if ( okconf( &sess->thisuser ) )
            {
                setuconf(CONF_DIRS, oc, os);
            }
            done = true;
        }
    } while ( !hangup && !done );
}


