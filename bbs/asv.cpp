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

// Local prototypes
int  printasv(char *pszFileName, int num, bool abort);


void asv()
{
    int i = 0;
    int inode = 0;
    char ch, s[41], s1[81], ph1[12], ph[12], sysname[35], snode[6];
    net_system_list_rec *csne;
    long *reg_num, reg_num1;
    unsigned char i2, ok = 0, reg = 0, valfile = 0;
    messagerec msg;
    int nAllowAnon = 0;

    nl();
    sess->bout << "|#5Are you the SysOp of a BBS? ";
    if (yesno())
    {
        printasv(ASV_HLP, 1, false);
        nl();
        sess->bout << "|#5Select |#7[|#21-4,Q|#7]|#0 : ";
        ch = onek("Q1234");
        nl();
        switch (ch)
        {
        case '1':
            sess->bout << "|#5Select a network you are in [Q=Quit].";
            nl( 2 );
            for ( i = 0; i < sess->GetMaxNetworkNumber(); i++ )
            {
                if (net_networks[i].sysnum)
                {
                    sess->bout << " |#3" << i + 1 << "|#1.  |#1" << net_networks[i].name << wwiv::endl;
                }
            }
            nl();
            sess->bout << "|#1:";
            input( s, 2, true );
            i = atoi(s);
            if ( i < 1 || i > sess->GetMaxNetworkNumber() )
            {
                nl();
                sess->bout << "|12Aborted!";
                break;
            }
            set_net_num(i - 1);

            do
            {
                nl();
                sess->bout << "|#5Enter your node number [Q=Quit].\r\n|#1: |#4@";
                input( snode, 5, true );
                inode = atoi(snode);
                csne = next_system(inode);
                if ((!csne) && (inode > 0))
                {
                    nl();
                    sess->bout << "|#6Unknown System!\r\n";
                }
            } while ((!csne) && (wwiv::UpperCase<char>(snode[0]) != 'Q'));
            if (wwiv::UpperCase<char>(snode[0] == 'Q'))
            {
                nl();
                sess->bout << "|12Aborted!";
                break;
            }
            strcpy(ph, csne->phone);
            strcpy(sysname, csne->name);

            ph1[0] = 0;
            if ( sess->thisuser.GetDataPhoneNumber()[0] &&
                 !wwiv::stringUtils::IsEquals( sess->thisuser.GetDataPhoneNumber(), "999-999-9999" ) )
            {
                nl();
                sess->bout << "|#9Is |#2" << sess->thisuser.GetDataPhoneNumber() << "|#9 the number of your BBS? ";
                if (yesno())
                {
                    strcpy( ph1, sess->thisuser.GetDataPhoneNumber() );
                }
                nl();
            }
            if (!ph1[0])
            {
                nl();
                i = 3;
                do
                {
                    sess->bout << "|#5Enter your BBS phone number.\r\n";
                    sess->bout << "|#3 ###-###-####\r\n|#1:";
                    mpl(12);
                    for (i2 = 0; i2 < 12; i2++)
                    {
                        ansic( 4 );
                        if ( i2 == 3 || i2 == 7 )
                        {
                            ph1[i2] = 45;
                            sess->bout << "|#4-";
                        }
                        else
                        {
                            ph1[i2] = onek_ncr("0123456789\r");
                            if (ph1[i2] == 8)
                            {
                                if (!(i2 == 0))
                                {
                                    bputch(' ');
                                    BackSpace();
                                    i2--;
                                    if ((i2 == 3) || (i2 == 7))
                                    {
                                        i2--;
                                        BackSpace();
                                    }
                                }
                                else
                                {
                                    bputch(' ');
                                }
                                ph1[i2] = '\0';
                                i2--;
                            }
                        }
                    }
                    ph1[12] = '\0';
                    nl();
                    ok = valid_phone(ph1);
                    if (!ok)
                    {
                        nl();
                        sess->bout << "|#6Improper Format!\r\n";
                    }
                } while ( !ok && ( --i > 0 ) );

                if ( i == 0 && !ok )
                {
                    valfile = 5;
                    break;
                }
            }
            if (compare(ph, ph1))
            {
                reg_num = next_system_reg( inode );
                if ( *reg_num == 0 )
                {
                    strcpy(s, sysname);
                    strcpy(s1, (strstr(strupr(s), "SERVER")));
                    if ( wwiv::stringUtils::IsEquals( s1, "SERVER" ) )
                    {
                        nl();
						sess->bout << "|#5Is " << sysname << " a server in " << sess->GetNetworkName() << "? ";
                        if ( noyes() )
                        {
                            sysoplog( "* Claims to run a network server." );
                            reg = 2;
                            valfile = 6;
                        }
                    }
                }
                else if ((*reg_num > 0) && (*reg_num <= 99999))
                {
                    nl();
                    sess->bout << "|#5Have you registered WWIV? ";
                    if (noyes())
                    {
                        i = 2;
                        do
                        {
                            nl();
                            sess->bout << "|#5Enter your registration number: ";
                            input( s, 5, true );
                            reg_num1 = atol(s);
                            if (*reg_num == reg_num1)
                            {
                                reg = 1;
                            }
                            else
                            {
                                reg = 0;
                                nl();
                                sess->bout << "|#6Incorrect!\r\n";
                            }
                        } while ((--i > 0) && (!reg));
                    }
                    if (!reg)
                    {
                        valfile = 2;
                        break;
                    }
                }
                sprintf(s, "%s 1@%d (%s)", sess->GetNetworkName(), inode, sysname);
                s[40] = '\0';
                sess->thisuser.SetNote( s );
                sess->thisuser.SetName( s );
                properize(s);
                ssm(1, 0, "%s validated as %s 1@%d on %s.", s, sess->GetNetworkName(), inode, fulldate());
                sprintf(s1, "* Validated as %s 1@%d", sess->GetNetworkName(), inode);
                sysoplog(s1);
                sysoplog(s);
                sess->thisuser.setStatusFlag( WUser::expert );
                sess->thisuser.SetExempt( 0 );
                sess->thisuser.SetForwardSystemNumber( inode );
                sess->thisuser.SetHomeSystemNumber( inode );
                sess->thisuser.SetForwardUserNumber( 1 );
                sess->thisuser.SetHomeUserNumber( 1 );
                sess->thisuser.SetForwardNetNumber( sess->GetNetworkNumber() );
                sess->thisuser.SetHomeNetNumber( sess->GetNetworkNumber() );
                nl();
                if (reg != 2)
                {
                    if (reg)
                    {
                        set_autoval(sess->advasv.reg_wwiv);
                        sess->thisuser.SetWWIVRegNumber( *reg_num );
                        valfile = 7;
                    }
                    else
                    {
                        set_autoval(sess->advasv.nonreg_wwiv);
                        valfile = 8;
                    }
                }
                else
                {
                    set_autoval(sess->advasv.reg_wwiv);
                }
                sprintf( irt, "%s %s SysOp Auto Validation", syscfg.systemname, sess->GetNetworkName() );
                if ( strlen( irt ) > 60 )
                {
                    irt[60] = '\0';
                }
                sprintf( s1, "%s%s", syscfg.gfilesdir, FORMASV_MSG );
                if ( WFile::Exists( s1 ) )
                {
                    LoadFileIntoWorkspace( s1, true );
                    msg.storage_type = 2;
                    sess->SetNewMailWaiting( true );
                    sprintf( net_email_name, "%s #1@%u", syscfg.sysopname, net_sysnum );
                    inmsg( &msg, irt, &nAllowAnon, false, "email", INMSG_NOFSED, snode, MSGED_FLAG_NONE, true );
                    sendout_email( irt, &msg, 0, 1, inode, 0, 1, net_sysnum, 1, sess->GetNetworkNumber() );
                }
                irt[0] = '\0';
                sess->SetNewMailWaiting( false );
            }
            else
            {
                valfile = 2;
                break;
            }
            break;

        case '2':
            nl();
            do
            {
                sess->bout <<  "|#3Enter your BBS name, phone and modem speed.\r\n\n|#1:";
                inputl( s, 28, true );
            } while ( !s[0] );
            properize( s );
			sprintf( s1, "WWIV SysOp: %s", s );
            sess->thisuser.SetNote( s1 );
            strcpy( s, sess->thisuser.GetName() );
            properize( s );
            ssm( 1, 0, "%s validated as a WWIV SysOp on %s.", s, fulldate() );
            sysoplog( "* Validated as a WWIV SysOp" );
            sess->thisuser.setStatusFlag( WUser::expert );
            set_autoval( sess->advasv.nonreg_wwiv );
            nl();
            valfile = 9;
            break;
        case '3':
            nl();
            do
            {
                sess->bout << "|#3Enter your BBS name, phone, software and modem speed.\r\n\n|#1:";
                inputl( s, 33, true );
            } while ( !s[0] );
            properize( s );
            sprintf( s1, "SysOp: %s", s );
            sess->thisuser.SetNote( s1 );
            strcpy( s, sess->thisuser.GetName() );
            properize( s );
            ssm( 1, 0, "%s validated as a Non-WWIV SysOp on %s.", s, fulldate() );
            sysoplog( "* Validation of a Non-WWIV SysOp" );
            sess->thisuser.SetExempt( 0 );
            set_autoval( sess->advasv.non_wwiv );
            nl();
            valfile = 10;
            break;
        case '4':
            nl();
            do
            {
                sess->bout << "|#3Enter the BBS name, phone, software and modem speed.\r\n\n|#1:";
                inputl( s, 30, true );
            }
            while ( !s[0] );
            properize( s );

            sprintf( s1, "Co-SysOp: %s", s );
            sess->thisuser.SetNote( s1 );
            sprintf( s1, "* Co-SysOp of %s", sess->thisuser.GetNote() );
            sysoplog( s1 );
            set_autoval( sess->advasv.cosysop );
        case 'Q':
            break;
        }
        if ( valfile )
        {
            printasv( ASV_HLP, valfile, false );
            pausescr();
        }
    }
}


int printasv( char *pszFileName, int num, bool abort )
{
    char buff[1024], nums[9];
    register unsigned int j;
    int bytes_read;
    int okprint = 0;
    int i1 = 0, found = 0, asv_num = 0;

    int temp = curatr;
    bool asvline = false;
    nl();

    char szFileName[ MAX_PATH ], szFileName1[ MAX_PATH ];
	sprintf( szFileName, "%s%s", syscfg.gfilesdir, pszFileName );
    if ( !strrchr( szFileName, '.' ) )
    {
        if ( sess->thisuser.hasAnsi() )
        {
            if ( sess->thisuser.hasColor() )
            {
				sprintf( szFileName1, "%s%s", szFileName, ".ans");
                if ( WFile::Exists( szFileName1 ) )
                {
                    strcat( szFileName, ".ans" );
                }
            }
            else
            {
				sprintf( szFileName1, "%s%s", szFileName, ".b&w" );
                if ( WFile::Exists( szFileName1 ) )
                {
                    strcat( szFileName, ".b&w" );
                }
            }
        }
    }
    if ( !strrchr( szFileName, '.' ) )
    {
		sprintf( szFileName1, "%s%s", szFileName, ".msg" );
        if ( WFile::Exists( szFileName1 ) )
        {
            strcat( szFileName, ".msg" );
        }
    }
    WFile file_to_prt( szFileName );
    if ( !file_to_prt.Open( WFile::modeBinary | WFile::modeReadOnly ) )
    {
        perror( szFileName );
        if ( curatr != temp )
        {
            curatr = temp;
            setc( curatr );
        }
        return -2;
    }
    while ( ( (bytes_read = file_to_prt.Read( buff, 1024 ) ) > 0 ) && !hangup )
    {
        for (j = 0; j < (unsigned int) bytes_read; j++)
        {
            if ((*(buff + j)) == '~')
            {
                asvline = true;
                j++;
            }
            if ( asvline )
            {
                if ( found == 2 )
                {
                    file_to_prt.Close();
                    if ( curatr != temp )
                    {
                        curatr = temp;
                        setc( curatr );
                    }
                    return 0;
                }
                while ( i1 < 2 && j < static_cast<unsigned int>( bytes_read ) )
                {
                    nums[i1] = (*(buff + (j++)));
                    if ( nums[i1++] == '~' )
                    {
                        i1 = 2;
                        if ( okprint )
                        {
                            bputch( '~' );
                        }
                        asvline = false;
                    }
                    else if ( wwiv::UpperCase<char>(nums[0]) == 'C' )
                    {
                        i1 = 2;
                        if ( okprint && okansi() )
                        {
                            ClearScreen();
                        }
                        asvline = false;
                    }
                    else if ( wwiv::UpperCase<char>(nums[0]) == 'P' )
                    {
                        i1 = 2;
                        if ( okprint )
                        {
                            pausescr();
                        }
                        asvline = false;
                    }
                }
                if ( i1 == 2 )
                {
                    nums[i1] = 0;
                    i1 = 0;
                }
            }
            else
            {
                if ( okprint )
                {
                    bputch( *( buff + j ) );
                }
            }

            if ( asvline && i1 == 0 )
            {
                if ( found )
                {
                    found++;
                }
                asvline = false;
                asv_num = atoi( nums );
                if ( asv_num == num )
                {
                    found = 1;
                    okprint = 1;
                }
                else
                {
                    okprint = 0;
                }
            }
            if ( app->localIO->LocalKeyPressed() )
            {
                switch ( wwiv::UpperCase<char>( getkey() ) )
                {
                case 19:
                case 'P':
                    {
                        getkey();
                    }
                    break;
                case 11:
                case CX:
                case ESC:
                case ' ':
                    {
                        if ( !abort )
                        {
                            break;
                        }
                        file_to_prt.Close();
                        if ( curatr != temp )
                        {
                            curatr = temp;
                            setc( curatr );
                        }
                        return 1;
                    }
                    break;
                }
            }
        }
    }
    file_to_prt.Close();
    if ( curatr != temp )
    {
        curatr = temp;
        setc( curatr );
    }
    if ( !found )
    {
        sess->bout << "\r\n%s\r\nPlease report this to the SysOp in Feedback.\r\n";
        ssm( 1, 0, "Subfile #%d not found in %s.\r\n", num, szFileName );
        return -1;
    }
    return 0;
}


void set_autoval( int n )
{
    auto_val( n, &sess->thisuser );
    sess->ResetEffectiveSl();
    changedsl();
}


