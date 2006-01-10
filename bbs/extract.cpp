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


void compress_file(char *pszFileName, char *pszDirectoryName);


//////////////////////////////////////////////////////////////////////////////
//
// Implementation
//
//


//
// Compresses file *pszFileName to directory *pszDirectoryName.
//


void compress_file( char *pszFileName, char *pszDirectoryName )
{
    char szFullFileName[255], szBaseFileName[81], s3[81], s4[81], s5[81];

	WWIV_ASSERT( pszFileName );
	WWIV_ASSERT( pszDirectoryName );

	sess->bout << "|#2Now compressing " << pszFileName << wwiv::endl;
    if ( strchr( pszFileName, '.' ) == NULL )
    {
        strcat( pszFileName, ".msg" );
    }
    strcpy( szFullFileName, pszFileName );
    strcpy( szBaseFileName, pszFileName );
    char *ss = strchr( szBaseFileName, '.' );
    if ( ss )
    {
        ss[1] = '\0';
    }
    strcat( szBaseFileName, arcs[0].extension );
    strcpy( s5, arcs[0].arca );
    strcpy( s3, pszDirectoryName );
    strcat( s3, szBaseFileName );
    strcpy( s4, pszDirectoryName );
    strcat( s4, szFullFileName );
    stuff_in( szFullFileName, s5, s3, s4, "", "", "" );
    ExecuteExternalProgram( szFullFileName, app->GetSpawnOptions( SPWANOPT_ARCH_A ) );
    WFile::Remove( s4 );
    app->localIO->UpdateTopScreen();
}


//
// Allows extracting a message into a file area, directly.
//

void extract_mod(const char *b, long len, long daten)
{
	char s1[81], s2[81],                  // reusable strings
		szFileName[MAX_PATH],                    // mod filename
		strip_cmd[MAX_PATH],                  // Strip command
		compressed_fn[MAX_PATH],               // compressed filename
		eof = CZ,                        // EOF indicator
		ch,                              // switch key
		*ss1,                            // mmKey
		dir_path[MAX_PATH],                    // path to directory
		idz_fn[MAX_PATH],                      // file_idz.diz path\filename
		temp_irt[81],                    // temp description
		temp[81],                        // temp string
		szDescription[81],                        // Moved here from global scope since only 2 functions use it and this one calls the other
		desc1[81],                       // description option 1
		desc2[81],                       // description option 2
		desc3[81],                       // description option 3
		author[31];                      // author name

	int  i2, i3, i4, i5, i6, i7 = 0,	 // misc int vars
		mod_dir,                         // directory number
		start = 0;                       // loop control

	WWIV_ASSERT( b );

	author[0] = '\0';
	tmp_disable_conf( true );

	// get directory number to extract to
	do
	{
		nl();
		sess->bout << "|#2Which dir? ";
		ss1 = mmkey( 1 );
		if ( ss1[0] == '?' )
		{
			dirlist( 0 );
		}
	} while ( !hangup && ss1[0] == '?' );

	mod_dir = -1;
	for ( int i1 = 0; i1 < sess->num_dirs && udir[i1].subnum != -1; i1++ )
	{
		if ( wwiv::stringUtils::IsEquals( udir[i1].keys, ss1 ) )
		{
			mod_dir = i1;
		}
	}

	bool exists = false;
	bool quit = false;

	if ( mod_dir == -1 )
	{
		goto go_away;
	}

	strcpy( s1, directories[udir[mod_dir].subnum].path );
	do
	{
		if ( irt )
		{
			sess->bout << "|#2Press |#7[|#9Enter|#7]|#2 for |#1" << StringRemoveChar(irt, '.') << ".mod.\r\n";
		}
		sess->bout << "|#2Save under what filename? ";
		input( s2, 12 );
		if ( !s2[0] )
		{
			if ( irt )
			{
				strcpy( s2, StringRemoveChar( irt, '.' ) );
			}
			else
			{
				goto go_away;
			}
		}
		if ( strchr( s2, '.' ) == NULL )
		{
			strcat( s2, ".mod" );
		}
		sprintf( szFileName, "%s%s", s1, s2 );
		if ( WFile::Exists( szFileName ) )
		{
			exists = true;
			sprintf( szFileName, "%s already exists.", s2 );
			nl();
			sess->bout << szFileName;
			nl( 2 );
		}
		if ( exists )
		{
			sess->bout << "|#2Which (N:ew Name, Q:uit): ";
			ch = onek( "QN" );
			switch( ch )
			{
			case 'Q':
				quit = true;
				break;
			case 'N':
				s2[0] = '\0';
				break;
			}
			nl();
		}
	} while ( !hangup && s2[0] == '\0' && !quit );

	if ( !quit && !hangup )
	{
		WFile file( szFileName );
		file.Open( WFile::modeBinary|WFile::modeCreateFile|WFile::modeReadWrite, WFile::shareUnknown, WFile::permReadWrite );
		file.Seek( 0L, WFile::seekEnd );
		file.Write( const_cast<char*>( b ), len );
		file.Write( &eof, 1 );
		file.Close();
		sess->bout << "Message written toL " << szFileName << wwiv::endl;
		sprintf( strip_cmd, "STRIPNET.EXE %s", szFileName );
		ExecuteExternalProgram( strip_cmd, EFLAG_ABORT | EFLAG_TOPSCREEN );
		compress_file( s2, s1 );
		nl( 2 );
		sess->bout << "|#2//UPLOAD the file? ";
		if ( noyes() )
		{
			sprintf( compressed_fn,"%s.%s", StringRemoveChar(s2, '.'), arcs[0].extension );
			sess->bout << "|#2Now //UPLOAD'ing the file...";
			strcpy( szDescription, stripcolors( irt ) );
            strcpy( author, stripcolors( StringRemoveChar( net_email_name, '#' ) ) );

			if ( author[0] == '`' )
			{
				i3 = 0;
				temp[0] = '\0';
				i4 = strlen( author );

				for ( i2 = 2; i2 < i4; i2++ )
				{
					temp[i3] = irt_name[i2];
					i3++;
				}
				sprintf( author, "%s", StringRemoveChar( temp, '`' ) );
			}

			i6 = strlen( irt );
			for ( i5 = 0; i5 < i6+1; i5++ )
			{
				if ( !start )
				{
					if ( irt[i5] == ' ' )
					{
						strcpy( "", temp_irt );
						start = 1;
						i7 = 0;
					}
					if ( start )
					{
						temp_irt[i7] = irt[i5];
						i7++;
						if ( irt[i5] == ':' || ( ( irt[i5] == '-' || irt[i5] == ' ' ) && i7 < 2 ) )
						{
							i7 = 0;
						}
					}
				}
			}

			nl();
			strcpy( desc2, temp_irt );
			strcpy( desc1, author );
			strcat( desc1, ": " );
			strcat( desc1, desc2 );
			strcpy( desc3, irt );
			desc1[58] = '\0';
			desc2[58] = '\0';
			desc3[58] = '\0';
			bputch( CL );
			sess->bout << "|#7Available Descriptions For |#1" << s2 << " |#7by |#1" << author << "|#7.";
			nl( 2 );
			sess->bout << "|#21) |#1" << desc1 << wwiv::endl;
			sess->bout << "|#22) |#1" <<  desc2 << wwiv::endl;
			sess->bout << "|#23) |#1" << desc3 << wwiv::endl;
			sess->bout << "|#2E) |#1Enter your own description\r\n";
			sess->bout << "|#7Q) |#1Quit\r\n";
			nl( 2 );
			sess->bout << "|#2Press [ENTER] for 1 or enter your selection:";
			ch = onek( "QE123\r" );
			switch( ch )
			{
			case 'Q':
				goto go_away;
			case 'E':
				sess->bout << "Input the description:\r\n\n";
				Input1( szDescription, desc1, 58, true, MIXED );
				break;
			case '\r':
			case '1':
				strcpy( szDescription, stripcolors( desc1 ) );
				break;
			case '2':
				strcpy( szDescription, stripcolors( desc2 ) );
				break;
			case '3':
				strcpy( szDescription, stripcolors( desc3 ) );
				break;
			}
			szDescription[58] = '\0';
			nl( 2 );
			sess->bout << "|#9Add a |#1FILE_ID.DIZ|#9 to archive? ";
			if ( noyes() )
            {
				sprintf( idz_fn, "%s%s", syscfgovr.tempdir, FILE_ID_DIZ );
				sprintf( dir_path,"%s%s", directories[udir[mod_dir].subnum].path, StringRemoveChar( s2, '.' ) );
				FILE *idz = fopen( idz_fn, "w" );
				fprintf( idz, "%.58s\n", szDescription );
				fprintf( idz, "Copyright (c) %s, %s\n", W_DateString(daten,"Y", ""), author );
				fprintf( idz, "Distribution is LIMITED by the WWIV Source\n" );
				fprintf( idz, "Code EULA.  Email WSS at 1@50 or wss@wwiv.com\n" );
				fprintf( idz, "for a copy of the EULA or more information.\n" );
				fclose( idz );
				add_arc( dir_path, idz_fn, 0 );
			}
			quit = upload_mod( mod_dir, compressed_fn, szDescription );
		}
	}
go_away:
	tmp_disable_conf( false );
}


bool upload_mod( int dn, const char *pszFileName, const char *pszDescription )
/* Passes a specific filename to the upload function */
{
	char s[81], s1[81];

	WWIV_ASSERT( pszFileName );

	dliscan1( udir[dn].subnum );
	nl( 2 );
	strcpy( s, pszFileName );
	strcpy( s1, directories[udir[dn].subnum].path );
	int maxf = directories[udir[dn].subnum].maxfiles;
	strcat( s1, s );
	WFindFile fnd;
	bool bDone = fnd.open( s1, 0 );
	bool ok = false;
	if ( !bDone )
	{
		ok = maybe_upload( fnd.GetFileName(), dn, pszDescription );
	}
	if ( ok )
	{
		sess->bout << "Uploaded " << pszFileName << "....\r\n";
	}
	if ( !ok )
	{
		sess->bout << "|12Aborted.\r\n";
	}
	if ( sess->numf >= maxf )
	{
		sess->bout << "directory full.\r\n";
	}
	return false;
}


void extract_out (char *b, long len, const char *title, long daten )
{
	// TODO Fix platform specific path issues...

	WWIV_ASSERT(b);
	char s1[81], s2[81], s3[81], ch = 26, ch1, s4[81];

	if ( app->HasConfigFlag( OP_FLAGS_NEW_EXTRACT ) )
	{
		printfile(MEXTRACT_NOEXT);
		bool done = false;
		bool uued = false;
		do
		{
			uued = false;
			s1[0] = 0;
			s2[0] = 0;
			s3[0] = 0;
			done = true;
			sess->bout << "|#5Which (1-4,Q,?): ";
			ch1 = onek("Q1234?");
			switch (ch1)
			{
			case '1':
				extract_mod(b, len, daten);
				break;
			case '2':
				strcpy(s2, syscfg.gfilesdir);
				break;
			case '3':
				strcpy(s2, syscfg.datadir);
				break;
			case '4':
				strcpy(s2, syscfgovr.tempdir);
				break;
			case '?':
				printfile(MEXTRACT_NOEXT);
				done = false;
				break;
			}
		} while ( !done && !hangup );

		if (s2[0])
		{
			do
			{
				sess->bout << "|#2Save under what filename? ";
				input(s1, 50);
				if (s1[0])
				{
					if ((strchr(s1, ':')) || (strchr(s1, '\\')))
					{
						strcpy(s3, s1);
					}
					else
					{
						sprintf(s3, "%s%s", s2, s1);
					}

					strcpy(s4, s2);

					if (strstr(s3, ".UUE") != NULL)
					{
						sess->bout << "|#1UUEncoded File.  Save Output File As? ";

						input(s1, 30);
						if (strchr(s1, '.') == NULL)
						{
							strcat(s1, ".MOD");
						}

						strcat(s4, s1);
						uued = true;

						if (WFile::Exists(s4))
						{
							sess->bout << s1 << s4 << " already exists!\r\n";
							uued = false;
						}
					}

					if (WFile::Exists(s3))
					{
						sess->bout << "\r\nFilename already in use.\r\n\n";
						sess->bout << "|#0O|#1)verwrite, |#0A|#1)ppend, |#0N|#1)ew name, |#0Q|#1)uit? |#0";
						ch1 = onek("QOAN");
						switch (ch1)
						{
						case 'Q':
							s3[0] = 1;
							s1[0] = 0;
							break;
						case 'N':
							s3[0] = 0;
							break;
						case 'A':
							break;
						case 'O':
							WFile::Remove(s3);
							break;
						}
						nl();
					}
				}
				else
				{
					s3[0] = 1;
				}
			} while ( !hangup && s3[0] == '\0' );

			if ( s3[0] && !hangup )
			{
				if ( s3[0] != '\x01' )
				{
					WFile file( s3 );
					if ( !file.Open( WFile::modeBinary|WFile::modeCreateFile|WFile::modeReadWrite, WFile::shareUnknown, WFile::permReadWrite ) )
					{
						sess->bout << "|12Could not open file for writing.\r\n";
					}
					else
					{
						if ( file.GetLength() > 0 )
						{
							file.Seek( -1L, WFile::seekEnd );
							file.Read( &ch1, 1 );
							if (ch1 == CZ)
							{
								file.Seek( -1L, WFile::seekEnd );
							}
						}
						file.Write( const_cast<char*>( title ), strlen( title ) );
						file.Write( (void *)"\r\n", 2 );
						file.Write( b, len );
						file.Write( &ch, 1 );
						file.Close();
						sess->bout <<  "|#9Message written to|#0: |#2" << s3 << wwiv::endl;
						if ( uued == true )
						{
							uudecode( s3, s4 );
						}
					}
				}
			}
		}
	}
	else
	{
		do
		{
			sess->bout << "|#2Save under what filename? ";
			input( s1, 50 );
			if ( s1[0] )
			{
				if ( strchr( s1, ':' ) || strchr( s1, '\\' ) )
				{
					strcpy( s2, s1 );
				}
				else
				{
					sprintf( s2, "%s%s", syscfg.gfilesdir, s1 );
				}
				if ( WFile::Exists( s2 ) )
				{
					sess->bout << "\r\nFilename already in use.\r\n\n";
					sess->bout << "|#0O|#1)verwrite, |#0A|#1)ppend, |#0N|#1)ew name, |#0Q|#1)uit? |#0";
					ch1 = onek( "QOAN" );
					switch ( ch1 )
					{
					case 'Q':
						s2[0] = '\0';
						s1[0] = '\0';
						break;
					case 'N':
						s1[0] = '\0';
						break;
					case 'A':
						break;
					case 'O':
						WFile::Remove( s2 );
						break;
					}
					nl();
				}
			}
			else
			{
				s2[0] = '\0';
			}
		} while ( !hangup && s2[0] != 0 && s1[0] == 0 );

		if ( s1[0] && !hangup )
		{
			WFile file( s2 );
			if ( !file.Open( WFile::modeBinary|WFile::modeCreateFile|WFile::modeReadWrite, WFile::shareUnknown, WFile::permReadWrite ) )
			{
				sess->bout << "|12Could not open file for writing.\r\n";
			}
			else
			{
				if ( file.GetLength() > 0 )
				{
					file.Seek( -1L, WFile::seekEnd );
					file.Read( &ch1, 1 );
					if ( ch1 == CZ )
					{
						file.Seek( -1L, WFile::seekEnd );
					}
				}
				file.Write( const_cast<char*>( title ), strlen( title ) );
				file.Write( (void *)"\r\n", 2 );
				file.Write( b, len );
				file.Write( &ch, 1 );
				file.Close();
				sess->bout <<  "|#9Message written to|#0: |#2" << s2 << wwiv::endl;
			}
		}
	}
	BbsFreeMemory( b );
}

