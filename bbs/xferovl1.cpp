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


// How far to indent extended descriptions
#define INDENTION 24


// the archive type to use
#define ARC_NUMBER 0


extern int foundany;
const unsigned char *invalid_chars =
    (unsigned char *)"ڿ��ĳô��ɻȼͺ̹��ոԾͳƵ��ַӽĺǶ�����װ�������";


void modify_extended_description(char **sss, const char *dest, const char *title)
{
	char s[255], s1[255];
	int i, i2;

	bool ii	= ( *sss ) ? true : false;
	int i4	= 0;
	do
	{
		if ( ii )
		{
			nl();
			if ( okfsed() && app->HasConfigFlag( OP_FLAGS_FSED_EXT_DESC ) )
			{
				sess->bout << "|#5Modify the extended description? ";
			}
			else
			{
				sess->bout << "|#5Enter a new extended description? ";
			}
			if ( !yesno() )
			{
				return;
			}
		}
		else
		{
			nl();
			sess->bout << "|#5Enter an extended description? ";
			if (!yesno())
			{
				return;
			}
		}
		if ( okfsed() && app->HasConfigFlag( OP_FLAGS_FSED_EXT_DESC ) )
		{
			sprintf(s, "%sEXTENDED.DSC", syscfgovr.tempdir);
			if ( *sss )
			{
				WFile fileExtDesc( s );
				fileExtDesc.Open( WFile::modeBinary|WFile::modeCreateFile|WFile::modeReadWrite, WFile::shareUnknown, WFile::permReadWrite );
				fileExtDesc.Write( *sss, strlen( *sss ) );
				fileExtDesc.Close();
				BbsFreeMemory( *sss );
				*sss = NULL;
			}
			else
			{
				WFile::Remove( s );
			}
			int nSavedScreenChars = sess->thisuser.GetScreenChars();
			if (sess->thisuser.GetScreenChars() > (76 - INDENTION))
			{
				sess->thisuser.SetScreenChars( 76 - INDENTION );
			}
			bool bEditOK = external_edit( "extended.dsc", syscfgovr.tempdir,
                                          sess->thisuser.GetDefaultEditor() - 1,
				                          sess->max_extend_lines, dest, title,
                                          MSGED_FLAG_NO_TAGLINE );
			sess->thisuser.SetScreenChars( nSavedScreenChars );
			if ( bEditOK )
			{
				if ( ( *sss = static_cast<char *>( BbsAllocA( 10240 ) ) ) == NULL )
				{
					return;
				}
				WFile fileExtDesc( s );
				fileExtDesc.Open( WFile::modeBinary | WFile::modeReadWrite );
				fileExtDesc.Read( *sss, fileExtDesc.GetLength() );
				( *sss )[ fileExtDesc.GetLength() ] = 0;
				fileExtDesc.Close();
			}
			for (int i3 = strlen(*sss) - 1; i3 >= 0; i3--)
			{
				if ((*sss)[i3] == 1)
				{
					(*sss)[i3] = ' ';
				}
			}
		}
		else
		{
			if (*sss)
			{
				BbsFreeMemory(*sss);
			}
			if ( ( *sss = static_cast<char *>( BbsAllocA( 10240 ) ) ) == NULL )
			{
				return;
			}
			*sss[0] = 0;
			i = 1;
			nl();
			sess->bout << "Enter up to  " << sess->max_extend_lines << " lines, "
                       << 78 - INDENTION << " chars each.\r\n";
			nl();
			s[0] = '\0';
			int nSavedScreenSize = sess->thisuser.GetScreenChars();
			if (sess->thisuser.GetScreenChars() > (76 - INDENTION))
			{
				sess->thisuser.SetScreenChars( 76 - INDENTION );
			}
			do
			{
				sess->bout << "|#2" << i << ": |#0";
				s1[0] = 0;
				bool bAllowPrevious = ( i4 > 0 ) ? true : false;
				while ( inli( s1, s, 90, true, bAllowPrevious ) )
				{
					if (i > 1)
					{
						--i;
					}
                    sprintf( s1, "%d:", i );
					sess->bout << "|#2" << s1;
					i2 = 0;
					i4 -= 2;
					do
					{
						s[i2] = *(sss[0] + i4 - 1);
						++i2;
						--i4;
					} while ((*(sss[0] + i4) != 10) && (i4 != 0));
					if (i4)
					{
						++i4;
					}
					*(sss[0] + i4) = 0;
					s[i2] = 0;
					strrev(s);
					if (strlen(s) > static_cast<unsigned int>( sess->thisuser.GetScreenChars() - 1 ) )
					{
						s[ sess->thisuser.GetScreenChars() - 2 ] = '\0';
					}
				}
				i2 = strlen(s1);
				if (i2 && (s1[i2 - 1] == 1))
				{
					s1[i2 - 1] = '\0';
				}
				if (s1[0])
				{
					strcat(s1, "\r\n");
					strcat(*sss, s1);
					i4 += strlen(s1);
				}
			} while ( ( i++ < sess->max_extend_lines ) && ( s1[0] ) );
			sess->thisuser.SetScreenChars( nSavedScreenSize );
			if ( *sss[0] == '\0' )
			{
				BbsFreeMemory( *sss );
				*sss = NULL;
			}
		}
		sess->bout << "|#5Is this what you want? ";
		i = !yesno();
		if (i)
		{
			BbsFreeMemory(*sss);
			*sss = NULL;
		}
	} while (i);
}


bool valid_desc(const unsigned char *pszDescription)
{
    // I don't think this function is really doing what it should
    // be doing, but am not sure what it should be doing instead.
	unsigned int i = 0;

	do
	{
		if (pszDescription[i] > 64 && pszDescription[i] < 123)
		{
			return true;
		}
		i++;
	} while ( i < strlen( reinterpret_cast<char*>( const_cast<unsigned char*>( pszDescription ) ) ) );
	return false;
}


bool get_file_idz(uploadsrec * u, int dn)
{
	char *b, *ss, cmd[MAX_PATH], s[81];
	int i;
	bool ok = false;

	if ( app->HasConfigFlag( OP_FLAGS_READ_CD_IDZ) && (directories[dn].mask & mask_cdrom))
	{
		return false;
	}
	sprintf(s, "%s%s", directories[dn].path, stripfn(u->filename));
	filedate( s, u->actualdate );
	ss = strchr(stripfn(u->filename), '.');
	if (ss == NULL)
	{
		return false;
	}
	++ss;
	for (i = 0; i < MAX_ARCS; i++)
	{
		if (!ok)
		{
			ok = wwiv::stringUtils::IsEqualsIgnoreCase( ss, arcs[i].extension );
		}
	}
	if (!ok)
	{
		return false;
	}

	WFile::Remove( syscfgovr.tempdir, FILE_ID_DIZ );
	WFile::Remove( syscfgovr.tempdir, DESC_SDI );

	WWIV_ChangeDirTo(directories[dn].path);
	WWIV_GetDir(s, true);
	strcat(s, stripfn(u->filename));
	app->CdHome();
	get_arc_cmd(cmd, s, 1, "FILE_ID.DIZ DESC.SDI");
	WWIV_ChangeDirTo(syscfgovr.tempdir);
	ExecuteExternalProgram(cmd, EFLAG_TOPSCREEN | EFLAG_NOHUP);
	app->CdHome();
	sprintf(s, "%s%s", syscfgovr.tempdir, FILE_ID_DIZ);
	if (!WFile::Exists(s))
	{
		sprintf(s, "%s%s", syscfgovr.tempdir, DESC_SDI);
	}
	if (WFile::Exists(s))
	{
		nl();
		sess->bout << "|#9Reading in |#2" << stripfn( s ) << "|#9 as extended description...";
		ss = read_extended_description(u->filename);
		if (ss)
		{
			BbsFreeMemory(ss);
			delete_extended_description(u->filename);
		}
		if ( ( b = static_cast<char *>( BbsAllocA( sess->max_extend_lines * 256 + 1 ) ) ) == NULL )
		{
			return false;
		}
		WFile file( s );
		file.Open( WFile::modeBinary | WFile::modeReadOnly );
		if ( file.GetLength() < ( sess->max_extend_lines * 256 ) )
		{
			long lFileLen = file.GetLength();
			file.Read( b, lFileLen );
			b[ lFileLen ] = 0;
		}
		else
		{
			file.Read( b, sess->max_extend_lines * 256 );
			b[ sess->max_extend_lines * 256 ] = 0;
		}
		file.Close();
		if ( app->HasConfigFlag( OP_FLAGS_IDZ_DESC ) )
		{
			ss = strtok(b, "\n");
			if (ss)
			{
				for (i = 0; i < wwiv::stringUtils::GetStringLength(ss); i++)
				{
					if ((strchr( reinterpret_cast<char*>( const_cast<unsigned char*>( invalid_chars ) ), ss[i]) != NULL) && (ss[i] != CZ))
					{
						ss[i] = '\x20';
					}
				}
				if (!valid_desc((unsigned char *) ss))
				{
					do
					{
						ss = strtok(NULL, "\n");
					} while (!valid_desc((unsigned char *) ss));
				}
			}
			if (ss[strlen(ss) - 1] == '\r')
			{
				ss[strlen(ss) - 1] = '\0';
			}
			sprintf(u->description, "%.55s", ss);
			ss = strtok(NULL, "");
		}
		else
		{
			ss = b;
		}
		if (ss)
		{
			for (i = strlen(ss) - 1; i > 0; i--)
			{
				if ( ss[i] == CZ || ss[i] == 12 )
				{
					ss[i] = '\x20';
				}
			}
			add_extended_description(u->filename, ss);
			u->mask |= mask_extended;
		}
		BbsFreeMemory(b);
		sess->bout << "Done!\r\n";
	}
	WFile::Remove( syscfgovr.tempdir, FILE_ID_DIZ );
	WFile::Remove( syscfgovr.tempdir, DESC_SDI );
	return true;
}


int read_idz_all()
{
	int count = 0;

	tmp_disable_conf( true );
	tmp_disable_pause( true );
	app->localIO->set_protect( 0 );
	for (int i = 0; (i < sess->num_dirs) && (udir[i].subnum != -1) &&
		(!app->localIO->LocalKeyPressed()); i++)
	{
		count += read_idz(0, i);
	}
	tmp_disable_conf( false );
	tmp_disable_pause( false );
	app->localIO->UpdateTopScreen();
	return count;
}


int read_idz(int mode, int tempdir)
{
	char s[81], s1[255];
	int i, count = 0;
	bool abort = false, next = false;
	uploadsrec u;

	if (mode)
	{
		tmp_disable_pause( true );
		app->localIO->set_protect( 0 );
		dliscan();
		file_mask(s);
	}
	else
	{
		sprintf(s, "*.*");
		align(s);
		dliscan1(udir[tempdir].subnum);
	}
	bprintf( "|#9Checking for external description files in |#2%-25.25s #%s...\r\n",
		directories[udir[tempdir].subnum].name,
		udir[tempdir].keys);
	WFile fileDownload( g_szDownloadFileName );
	fileDownload.Open( WFile::modeBinary|WFile::modeCreateFile|WFile::modeReadWrite, WFile::shareUnknown, WFile::permReadWrite );
	for (i = 1; (i <= sess->numf) && (!hangup) && !abort; i++)
	{
		FileAreaSetRecord( fileDownload, i );
		fileDownload.Read( &u, sizeof( uploadsrec ) );
		if ((compare(s, u.filename)) &&
			(strstr(u.filename, ".COM") == NULL) &&
			(strstr(u.filename, ".EXE") == NULL))
		{
			WWIV_ChangeDirTo(directories[udir[tempdir].subnum].path);
			WWIV_GetDir(s1, true);
			strcat(s1, stripfn(u.filename));
			app->CdHome();
			if (WFile::Exists(s1))
			{
				if (get_file_idz(&u, udir[tempdir].subnum))
				{
					count++;
				}
				FileAreaSetRecord( fileDownload, i );
				fileDownload.Write( &u, sizeof( uploadsrec ) );
			}
		}
		checka(&abort, &next);
	}
	fileDownload.Close();
	if (mode)
	{
		app->localIO->UpdateTopScreen();
		tmp_disable_pause( false );
	}
	return count;
}


void tag_it()
{
	int i, i2, i3, i4;
	bool bad;
    double t = 0.0;
	char s[255], s1[255], s2[81], s3[400];
	long fs = 0;

	if (sess->numbatch >= sess->max_batch)
	{
		sess->bout << "|#6No room left in batch queue.";
		getkey();
		return;
	}
    sess->bout << "|#2Which file(s) (1-" << sess->tagptr << ", *=All, 0=Quit)? ";
	input( s3, 30, true );
	if (s3[0] == '*')
	{
		s3[0] = '\0';
		for (i2 = 0; i2 < sess->tagptr && i2 < 78; i2++)
		{
			sprintf(s2, "%d ", i2 + 1);
			strcat(s3, s2);
			if (strlen(s3) > sizeof(s3) - 10)
			{
				break;
			}
		}
		sess->bout << "\r\n|#2Tagging: |#4" << s3 << wwiv::endl;
	}
	for (i2 = 0; i2 < wwiv::stringUtils::GetStringLength(s3); i2++)
	{
		sprintf(s1, "%s", s3 + i2);
		i4 = 0;
		bad = false;
		for (i3 = 0; i3 < wwiv::stringUtils::GetStringLength(s1); i3++)
		{
			if ((s1[i3] == ' ') || (s1[i3] == ',') || (s1[i3] == ';'))
			{
				s1[i3] = 0;
				i4 = 1;
			}
			else
			{
				if (i4 == 0)
				{
					i2++;
				}
			}
		}
		i = atoi(s1);
		if (i == 0)
		{
			break;
		}
		i--;
		if ((s1[0]) && (i >= 0) && (i < sess->tagptr))
		{
			if (check_batch_queue(filelist[i].u.filename))
			{
				sess->bout << "|#6" << filelist[i].u.filename << " is already in the batch queue.\r\n";
				bad = true;
			}
			if (sess->numbatch >= sess->max_batch)
			{
				sess->bout << "|#6Batch file limit of " << sess->max_batch << " has been reached.\r\n";
				bad = true;
			}
			if ( ( syscfg.req_ratio > 0.0001 ) && ( ratio() < syscfg.req_ratio ) &&
                 !sess->thisuser.isExemptRatio() && !bad )
			{
				bprintf( "|#2Your up/download ratio is %-5.3f.  You need a ratio of %-5.3f to download.\r\n", ratio(), syscfg.req_ratio );
				bad = true;
			}
			if ( !bad )
			{
				sprintf( s, "%s%s", directories[filelist[i].directory].path,
					     stripfn( filelist[i].u.filename ) );
				if (filelist[i].dir_mask & mask_cdrom)
				{
					sprintf( s2, "%s%s", directories[filelist[i].directory].path,
						     stripfn(filelist[i].u.filename));
					sprintf(s, "%s%s", syscfgovr.tempdir, stripfn(filelist[i].u.filename));
					if (!WFile::Exists(s))
					{
						copyfile(s2, s, true);
					}
				}
				WFile fp( s );
				if ( !fp.Open( WFile::modeBinary | WFile::modeReadOnly ) )
				{
					sess->bout << "|#6The file " << stripfn(filelist[i].u.filename ) << " is not there.\r\n";
					bad = true;
				}
				else
				{
					fs = fp.GetLength();
					fp.Close();
				}
			}
			if ( !bad )
			{
				t = 12.656 / static_cast<double>( modem_speed ) * static_cast<double> ( fs );
				if ( nsl() <= ( batchtime + t ) )
				{
					sess->bout << "|#6Not enough time left in queue for " << filelist[i].u.filename << ".\r\n";
					bad = true;
				}
			}
			if ( !bad )
			{
				batchtime += static_cast<float>( t );
				strcpy(batch[sess->numbatch].filename, filelist[i].u.filename);
				batch[sess->numbatch].dir = filelist[i].directory;
				batch[sess->numbatch].time = (float) t;
				batch[sess->numbatch].sending = 1;
				batch[sess->numbatch].len = fs;
				sess->numbatch++;
				++sess->numbatchdl;
				sess->bout << "|#1" << filelist[i].u.filename << " added to batch queue.\r\n";
			}
		}
		else
		{
			sess->bout << "|#6Bad file number " << i + 1 << wwiv::endl;
		}
		lines_listed = 0;
  }
}


void tag_files()
{
	int i, i1, i2;
	char s[255], s1[255], s2[81], ch;
    bool had = false;
	double d;

	if ((lines_listed == 0) || (sess->tagging == 0) || (g_num_listed == 0))
	{
		return;
	}
	bool abort = false;
	if ( x_only || sess->tagging == 2 )
	{
		sess->tagptr = 0;
		return;
	}
	app->localIO->tleft( true );
	if (hangup)
	{
		return;
	}
	if ( sess->thisuser.isUseNoTagging() )
	{
		if ( sess->thisuser.hasPause() )
		{
			pausescr();
		}
        ansic( sess->thisuser.isUseExtraColor() ? FRAME_COLOR : 0 );
		if ( okansi() )
		{
			sess->bout << "\r" << "������������������������������������������������������������������������������" << wwiv::endl;
		}
		else
		{
			sess->bout << "\r" << "------------+-----+-----------------------------------------------------------" << wwiv::endl;
		}
		sess->tagptr = 0;
		return;
	}
	lines_listed = 0;
	ansic( sess->thisuser.isUseExtraColor() ? FRAME_COLOR : 0 );
	if ( okansi() )
	{
		sess->bout << "\r������������������������������������������������������������������������������\r\n";
	}
	else
	{
		sess->bout << "\r--+------------+-----+----+---------------------------------------------------\r\n";
	}

	bool done = false;
	while ( !done && !hangup )
	{
		lines_listed = 0;
		ch = fancy_prompt("File Tagging", "CDEMNQRTV?");
		lines_listed = 0;
		switch (ch)
		{
		case '?':
			i = sess->tagging;
			sess->tagging = 0;
			printfile( TTAGGING_NOEXT );
			pausescr();
			sess->tagging = i;
			relist();
			break;
		case 'C':
		case SPACE:
		case RETURN:
			lines_listed = 0;
			sess->tagptr = 0;
			sess->titled = 2;
			ClearScreen();
			done = true;
			break;
		case 'D':
			batchdl( 1 );
			sess->tagging = 0;
			if ( !had )
			{
				nl();
				pausescr();
				ClearScreen();
			}
			done = true;
			break;
		case 'E':
			lines_listed = 0;
			i1 = sess->tagging;
			sess->tagging = 0;
			sess->bout << "|#9Which file (1-" << sess->tagptr << ")? ";
			input( s, 2, true );
			i = atoi( s ) - 1;
			if ( s[0] && i >= 0 && i < sess->tagptr )
			{
				d = XFER_TIME( filelist[i].u.numbytes );
				nl();
				for ( i2 = 0; i2 < sess->num_dirs; i2++ )
				{
					if ( udir[i2].subnum == filelist[i].directory )
					{
						break;
					}
				}
				if ( i2 < sess->num_dirs )
				{
					sess->bout << "|#1Directory  : |#2#" << udir[i2].keys << ", " << directories[filelist[i].directory].name << wwiv::endl;
				}
				else
				{
					sess->bout << "|#1Directory  : |#2#" << "??" << ", " << directories[filelist[i].directory].name << wwiv::endl;
				}
				sess->bout << "|#1Filename   : |#2" << filelist[i].u.filename << wwiv::endl;
				sess->bout << "|#1Description: |#2" << filelist[i].u.description << wwiv::endl;
				if ( filelist[i].u.mask & mask_extended )
				{
					strcpy( s1, g_szExtDescrFileName );
					sprintf( g_szExtDescrFileName, "%s%s.ext", syscfg.datadir, directories[filelist[i].directory].filename );
					zap_ed_info();
					sess->bout << "|#1Ext. Desc. : |#2";
					print_extended(filelist[i].u.filename, &abort, sess->max_extend_lines, 2);
					zap_ed_info();
					strcpy(g_szExtDescrFileName, s1);
				}
				sess->bout << "|#1File size  : |#2" << bytes_to_k(filelist[i].u.numbytes) << wwiv::endl;
				sess->bout << "|#1Apprx. time: |#2" << ctim( d ) << wwiv::endl;
				sess->bout << "|#1Uploaded on: |#2" << filelist[i].u.date << wwiv::endl;
				sess->bout << "|#1Uploaded by: |#2" << filelist[i].u.upby << wwiv::endl;
				sess->bout << "|#1Times D/L'd: |#2" << filelist[i].u.numdloads << wwiv::endl;
				if (directories[filelist[i].directory].mask & mask_cdrom)
				{
					nl();
					sess->bout << "|13CD ROM DRIVE\r\n";
				}
				else
				{
					sprintf( s, "|#7%s%s", directories[filelist[i].directory].path, filelist[i].u.filename );
					if ( !WFile::Exists( s ) )
					{
						nl();
						sess->bout << "|12-=>FILE NOT THERE<=-\r\n";
					}
				}
				nl();
				pausescr();
				relist();

			}
			sess->tagging = i1;
			break;
		case 'N':
			sess->tagging = 2;
			done = true;
			break;
		case 'M':
			if ( dcs() )
			{
				i = sess->tagging;
				sess->tagging = 0;
				move_file_t();
				sess->tagging = i;
				if ( g_num_listed == 0 )
				{
					done = true;
					return;
				}
				relist();
			}
			break;
		case 'Q':
			sess->tagging   = 0;
			sess->titled    = 0;
			sess->tagptr    = 0;
			lines_listed    = 0;
			done = true;
			return;
		case 'R':
			relist();
			break;
		case 'T':
			tag_it();
			break;
		case 'V':
			sess->bout << "|#2Which file (1-|#2" << sess->tagptr << ")? ";
			input( s, 2, true );
			i = atoi(s) - 1;
			if ((s[0]) && (i >= 0) && (i < sess->tagptr))
			{
				sprintf(s1, "%s%s", directories[filelist[i].directory].path,
					stripfn(filelist[i].u.filename));
				if (directories[filelist[i].directory].mask & mask_cdrom)
				{
					sprintf(s2, "%s%s", directories[filelist[i].directory].path,
						stripfn(filelist[i].u.filename));
					sprintf(s1, "%s%s", syscfgovr.tempdir,
						stripfn(filelist[i].u.filename));
					if (!WFile::Exists(s1))
					{
						copyfile(s2, s1, true);
					}
				}
				if (!WFile::Exists(s1))
				{
					sess->bout << "|#6File not there.\r\n";
					pausescr();
					break;
				}
				get_arc_cmd(s, s1, 0, "");
				if (!okfn(stripfn(filelist[i].u.filename)))
				{
					s[0] = 0;
				}
				if (s[0] != 0)
				{
					nl();
					sess->tagging = 0;
					ExecuteExternalProgram(s, app->GetSpawnOptions( SPWANOPT_ARCH_L ) );
					nl();
					pausescr();
					sess->tagging = 1;
					app->localIO->UpdateTopScreen();
					ClearScreen();
					relist();
				}
				else
				{
					sess->bout << "|#6Unknown archive.\r\n";
					pausescr();
					break;
				}
			}
			break;
		default:
			ClearScreen();
			done = true;
			break;
    }
  }
  sess->tagptr = 0;
  lines_listed = 0;
}


int add_batch(char *pszDescription, const char *pszFileName, int dn, long fs)
{
	unsigned char ch;
	char s1[81], s2[81];
	int i;

	if (find_batch_queue(pszFileName) > -1)
	{
		return 0;
	}

	double t = 0.0;
	if (modem_speed)
	{
		t = (12.656) / ((double) (modem_speed)) * ((double) (fs));
	}

	if (nsl() <= (batchtime + t))
	{
		sess->bout << "|#6 Insufficient time remaining... press any key.";
		getkey();
	}
	else
	{
		if (dn == -1)
		{
			return 0;
		}
		else
		{
			for (i = 0; i < wwiv::stringUtils::GetStringLength(pszDescription); i++)
			{
				if (pszDescription[i] == RETURN)
				{
					pszDescription[i] = SPACE;
				}
			}
			BackLine();
			bprintf(" |#6? |#1%s %3luK |#5%-43.43s |#7[|#2Y/N/Q|#7] |#0", pszFileName,
				bytes_to_k(fs), stripcolors(pszDescription));
			ch = onek_ncr("QYN\r");
			BackLine();
			if (wwiv::UpperCase<char>(ch) == 'Y')
			{
				if (directories[dn].mask & mask_cdrom)
				{
					sprintf(s2, "%s%s", directories[dn].path, pszFileName);
					sprintf(s1, "%s%s", syscfgovr.tempdir, pszFileName);
					if (!WFile::Exists(s1))
					{
						if (!copyfile(s2, s1, true))
						{
							sess->bout << "|#6 file unavailable... press any key.";
							getkey();
						}
						BackLine();
						ClearEOL();
					}
				}
				else
				{
					sprintf(s2, "%s%s", directories[dn].path, pszFileName);
					StringRemoveWhitespace(s2);
					if ((!WFile::Exists(s2)) && (!so()))
					{
						sess->bout << "\r";
						ClearEOL();
						sess->bout << "|#6 file unavailable... press any key.";
						getkey();
						sess->bout << "\r";
						ClearEOL();
						return 0;
					}
				}
				batchtime += static_cast<float>( t );
				strcpy(batch[sess->numbatch].filename, pszFileName);
				batch[sess->numbatch].dir = static_cast<short>( dn );
				batch[sess->numbatch].time = static_cast<float>( t );
				batch[sess->numbatch].sending = 1;
				batch[sess->numbatch].len = fs;
				sess->bout << "\r";
				bprintf("|#2%3d |#1%s |#2%-7ld |#1%s  |#2%s\r\n",
					sess->numbatch + 1, batch[sess->numbatch].filename, batch[sess->numbatch].len, ctim(batch[sess->numbatch].time),
					directories[batch[sess->numbatch].dir].name);
				sess->numbatch++;
				++sess->numbatchdl;
				sess->bout << "\r";
				sess->bout << "|#5    Continue search? ";
				ch = onek_ncr("YN\r");
				if (wwiv::UpperCase<char>(ch) == 'N')
				{
					return -3;
				}
				else
				{
					return 1;
				}
			}
			else if ( ch == 'Q' )
			{
				BackLine();
				return -3;
			}
			else
			{
				BackLine();
			}
		}
	}
	return 0;
}


int try_to_download(const char *pszFileMask, int dn)
{
	int rtn;
    bool abort, next;
	bool ok = false;
	uploadsrec u;
	char s1[81], s3[81];

	dliscan1(dn);
	int i = recno( pszFileMask );
	if (i <= 0)
	{
		abort = next = false;
		checka(&abort, &next);
		return ( (abort) ? -1 : 0 );
	}
	ok = true;
	foundany = 1;
	do
	{
		app->localIO->tleft( true );
		WFile fileDownload( g_szDownloadFileName );
		fileDownload.Open( WFile::modeBinary | WFile::modeReadOnly );
		FileAreaSetRecord( fileDownload, i );
		fileDownload.Read( &u, sizeof( uploadsrec ) );
		fileDownload.Close();

		bool ok2 = false;
		if ( strncmp(u.filename, "WWIV4", 5) == 0 && !app->HasConfigFlag( OP_FLAGS_NO_EASY_DL ) )
		{
			ok2 = true;
		}

		if ( !ok2 && (!(u.mask & mask_no_ratio)) && ( !ratio_ok() ) )
		{
			return -2;
		}

		write_inst(INST_LOC_DOWNLOAD, udir[sess->GetCurrentFileArea()].subnum, INST_FLAGS_ONLINE);
		sprintf(s1, "%s%s", directories[dn].path, u.filename);
		sprintf(s3, "%-40.40s", u.description);
		abort = false;
		rtn = add_batch(s3, u.filename, dn, u.numbytes);
		s3[0] = 0;

		if ( abort || rtn == -3 )
		{
			ok = false;
		}
		else
		{
			i = nrecno( pszFileMask, i );
		}
	} while ( i > 0 && ok && !hangup );
	returning = true;
	if (rtn == -2)
	{
		return -2;
	}
	if ( abort || rtn == -3 )
	{
		return -1;
	}
	else
	{
		return 1;
	}
}


void download()
{
    char ch, s[81], s1[81], buff[81];
    int i = 0, color = 0, count;
    bool ok = true;
    int dn, ip, rtn = 0, useconf;
    bool done = false;

    returning = false;
    useconf = 0;

    ClearScreen();
    sprintf(s, " [ %s Batch Downloads ] ", syscfg.systemname);
    DisplayLiteBar(s);
    nl();
    do
    {
        if (!i)
        {
            sess->bout << "|#2Enter files, one per line, wildcards okay.  [Space] aborts a search.\r\n";
            nl();
            sess->bout << "|#1 #  File Name    Size    Time      Directory\r\n";
            sess->bout << "|#7��� ������������ ������� ��������� ����������������������������������������\r\n";
        }
        if (i < sess->numbatch)
        {
            if ( !returning && batch[i].sending )
            {
                bprintf("|#2%3d |#1%s |#2%-7ld |#1%s  |#2%s\r\n", i + 1, batch[i].filename,
                    batch[i].len, ctim(batch[i].time), directories[batch[i].dir].name);
            }
        }
        else
        {
            do
            {
                count = 0;
                ok = true;
                BackLine();
                bprintf("|#2%3d ", sess->numbatch + 1);
                ansic( 1 );
                bool onl = newline;
                newline = 0;
                input1(s, 12, UPPER, false);
                newline = onl;
                if ((s[0]) && (s[0] != ' '))
                {
                    if (strchr(s, '.') == NULL)
                    {
                        strcat(s, ".*");
                    }
                    align(s);
                    rtn = try_to_download(s, udir[sess->GetCurrentFileArea()].subnum);
                    if (rtn == 0)
                    {
                        if ( uconfdir[1].confnum != -10 && okconf( &sess->thisuser ) )
                        {
                            BackLine();
                            sess->bout << "|#5Search all conferences? ";
                            ch = onek_ncr("YN\r");
                            if ( ch == '\r' || wwiv::UpperCase<char>( ch ) == 'Y' )
                            {
                                tmp_disable_conf( true );
                                useconf = 1;
                            }
                        }
                        BackLine();
                        sprintf(s1, "%3ld %s", sess->numbatch + 1, s);
                        ansic( 1 );
                        sess->bout << s1;
                        foundany = dn = 0;
                        while ((dn < sess->num_dirs) && (udir[dn].subnum != -1))
                        {
                            count++;
                            if (!x_only)
                            {
								sess->bout << "|#" << color;
                                if (count == NUM_DOTS)
                                {
                                    sess->bout << "\r";
                                    ansic(color);
                                    sess->bout << s1;
                                    color++;
                                    count = 0;
                                    if (color == 4)
                                    {
                                        color++;
                                    }
                                    if (color == 10)
                                    {
                                        color = 0;
                                    }
                                }
                            }
                            rtn = try_to_download(s, udir[dn].subnum);
                            if (rtn < 0)
                            {
                                break;
                            }
                            else
                            {
                                dn++;
                            }
                        }
                        if (useconf)
                        {
                            tmp_disable_conf( false );
                        }
                        if (!foundany)
                        {
                            sess->bout << "|#6 File not found... press any key.";
                            getkey();
                            BackLine();
                            ok = false;
                        }
                    }
                }
                else
                {
                    BackLine();
                    done = true;
                }
            } while (!ok && !hangup);
        }
        i++;
        if (rtn == -2)
        {
            rtn = 0;
            i = 0;
        }
    } while (!done && !hangup && (i <= sess->numbatch));

    if (!sess->numbatchdl)
    {
        return;
    }

    nl();
    if (!ratio_ok())
    {
        sess->bout << "\r\nSorry, your ratio is too low.\r\n\n";
        done = true;
        return;
    }
    nl();
	sess->bout << "|#1Files in Batch Queue   : |#2" << sess->numbatch << wwiv::endl;
	sess->bout << "|#1Estimated Download Time: |#2" << ctim2( batchtime, buff ) << wwiv::endl;
    nl();
    rtn = batchdl( 3 );
    if (rtn)
    {
        return;
    }
    nl();
    if (!sess->numbatchdl)
    {
        return;
    }
    sess->bout << "|#5Hang up after transfer? ";
    bool had = yesno();
    ip = get_protocol(xf_down_batch);
    if (ip > 0)
    {
        switch ( ip )
        {
        case WWIV_INTERNAL_PROT_YMODEM:
            {
                // %%TODO: This needs to be updated for handling internal zmodem
                if (over_intern && (over_intern[2].othr & othr_override_internal) &&
                    (over_intern[2].sendbatchfn[0]))
                {
                    dszbatchdl(had, over_intern[2].sendbatchfn, prot_name(WWIV_INTERNAL_PROT_YMODEM));
                }
                else
                {
                    ymbatchdl(had);
                }
            } break;
        case WWIV_INTERNAL_PROT_ZMODEM:
            {
                zmbatchdl( had );
            }
        default:
            {
                dszbatchdl(had, externs[ip - WWIV_NUM_INTERNAL_PROTOCOLS].sendbatchfn, externs[ip - WWIV_NUM_INTERNAL_PROTOCOLS].description);
            }
        }
        if (!had)
        {
            nl();
            bprintf("Your ratio is now: %-6.3f\r\n", ratio());
        }
    }
}


char fancy_prompt( const char *pszPrompt, const char *pszAcceptChars )
{
	char s1[81], s2[81], s3[81];
	char ch = 0;

	app->localIO->tleft( true );
	sprintf(s1, "\r|#2%s (|#1%s|#2)? |#0", pszPrompt, pszAcceptChars );
	sprintf(s2, "%s (%s)? ", pszPrompt, pszAcceptChars );
	int i1 = strlen(s2);
	sprintf(s3, "%s%s", pszAcceptChars, " \r");
	app->localIO->tleft( true );
	if ( okansi() )
	{
		sess->bout << s1;
		ch = onek1(s3);
		sess->bout << "\x1b[" << i1 << "D";
		for (int i = 0; i < i1; i++)
		{
			bputch(' ');
		}
		sess->bout << "\x1b[" << i1 << "D";
	}
	else
	{
		sess->bout << s2;
		ch = onek1(s3);
		for (int i = 0; i < i1; i++)
		{
			BackSpace();
		}
	}
	return ch;
}


void endlist(int mode)
{
	if (sess->tagging != 0)
	{
		if (g_num_listed)
		{
			if ( sess->tagging == 1 && !sess->thisuser.isUseNoTagging() && filelist )
			{
				tag_files();
				return;
			}
			else
			{
				ansic( sess->thisuser.isUseExtraColor() ? FRAME_COLOR : 0 );
				if ( sess->titled != 2 && sess->tagging == 1 && !sess->thisuser.isUseNoTagging() )
				{
					if ( okansi() )
					{
						sess->bout << "\r������������������������������������������������������������������������������\r\n";
					}
					else
					{
						sess->bout << "\r--+------------+-----+----+---------------------------------------------------\r\n";
					}
				}
				else
				{
					if ( okansi() )
					{
						sess->bout << "\r������������������������������������������������������������������������������\r\n";
					}
					else
					{
						sess->bout << "\r------------+-----+-----------------------------------------------------------\r\n";
					}
				}
			}
			switch (mode)
			{
			case 1:
				sess->bout << "\r|#9Files listed: |#2 " << g_num_listed;
				break;
			case 2:
				sess->bout << "\r|#9Files listed: |#2 " << g_num_listed;
				break;
			}
		}
		else
		{
			switch (mode)
			{
			case 1:
				sess->bout << "\r|13No matching files found.\r\n\n";
				break;
			case 2:
				sess->bout << "\r|#1No new files found.\r\n\n";
				break;
			}
		}
	}
}


void SetNewFileScanDate()
{
	char ag[10];
	int m, dd, y;
	bool ok = true;

	nl();
	struct tm *pTm = localtime(&nscandate);

	bprintf("|#9Current limiting date: |#2%02d/%02d/%02d\r\n", pTm->tm_mon + 1, pTm->tm_mday, (pTm->tm_year % 100));
	nl();
	sess->bout << "|#9Enter new limiting date in the following format: \r\n";
	sess->bout << "|#1 MM/DD/YY\r\n|#7:";
	mpl( 8 );
	int i = 0;
    char ch = 0;
	do
	{
		if ( i == 2 || i == 5 )
		{
			ag[i++] = '/';
			bputch('/');
		}
		else
		{
			switch (i)
			{
			case 0:
				ch = onek_ncr("01\r");
				break;
			case 3:
				ch = onek_ncr("0123\b");
				break;
			case 8:
				ch = onek_ncr("\b\r");
				break;
			default:
				ch = onek_ncr("0123456789\b");
				break;
			}
			if (hangup)
			{
				ok = false;
				ag[0] = 0;
				break;
			}
			switch (ch)
			{
			case '\r':
				switch (i)
				{
				case 0:
					ok = false;
					break;
				case 8:
					ag[8] = 0;
					break;
				default:
					ch = 0;
					break;
				}
				break;
			case BACKSPACE:
					sess->bout << " \b";
					--i;
					if ( ( i == 2 ) || ( i == 5 ) )
					{
						BackSpace();
						--i;
					}
					break;
				default:
					ag[i++] = ch;
					break;
			}
		}
	} while ( ch != '\r' && !hangup );

	nl();
	if (ok)
	{
		m = atoi(ag);
		dd = atoi(&(ag[3]));
		y = atoi(&(ag[6])) + 1900;
		if (y < 1920)
		{
			y += 100;
		}
		struct tm newTime;
		if ((((m == 2) || (m == 9) || (m == 4) || (m == 6) || (m == 11)) && (dd >= 31)) ||
			((m == 2) && (((y % 4 != 0) && (dd == 29)) || (dd == 30))) ||
			(dd > 31) || ((m == 0) || (y == 0) || (dd == 0)) ||
			((m > 12) || (dd > 31)))
		{
			nl();
            bprintf("|#6%02d/%02d/%02d is invalid... date not changed!\r\n",m,dd,(y % 100));
			nl();
		}
		else
		{
			// Rushfan - Note, this needs a better fix, this whole routine should be replaced.
			newTime.tm_min	= 0;
			newTime.tm_hour	= 1;
			newTime.tm_sec	= 0;
			newTime.tm_year	= y - 1900;
			newTime.tm_mday	= dd;
			newTime.tm_mon	= m - 1;
		}
		nl();
		nscandate = mktime( &newTime );

		// Display the new nscan date
		struct tm *pNewTime = localtime(&nscandate);
		bprintf("|#9New Limiting Date: |#2%02d/%02d/%04d\r\n", pNewTime->tm_mon + 1, pNewTime->tm_mday, (pNewTime->tm_year +1900));

		// Hack to make sure the date covers everythig since we had to increment the hour by one
		// to show the right date on some versions of MSVC
		nscandate -= SECONDS_PER_HOUR;
	}
	else
	{
		nl();
	}
}


void removefilesnotthere(int dn, int *autodel)
{
	char ch = 0;
	char s[MAX_PATH];
	char s1[MAX_PATH];
	uploadsrec u;

	dliscan1(dn);
	strcpy(s, "*.*");
	align(s);
	int i = recno(s);
	bool abort = false;
	while ( !hangup && i > 0 && !abort )
	{
		WFile fileDownload( g_szDownloadFileName );
		fileDownload.Open( WFile::modeBinary | WFile::modeReadOnly );
		FileAreaSetRecord( fileDownload, i );
		fileDownload.Read( &u, sizeof( uploadsrec ) );
		fileDownload.Close();
		sprintf(s1, "%s%s", directories[dn].path, u.filename);
        StringRemoveWhitespace( s1 );
		if (!WFile::Exists(s1))
		{
			StringTrim(u.description);
			sprintf(s1, "|#2%s :|#1 %-40.40s", u.filename, u.description);
			if (!*autodel)
			{
				BackLine();
				sess->bout << s1;
				nl();
				sess->bout << "|#5Remove Entry (Yes/No/Quit/All) : ";
				ch = onek_ncr("QYNA");
			}
			else
			{
				nl();
                sess->bout << "|#1Removing entry " << s1;
				ch = 'Y';
			}
			if ( ch == 'Y' || ch == 'A' )
			{
				if (ch == 'A')
				{
					sess->bout << "ll";
					*autodel = 1;
				}
				if (u.mask & mask_extended)
				{
					delete_extended_description(u.filename);
				}
				sysoplogf("-%s Removed from %s", u.filename, directories[dn].name);
				fileDownload.Open( WFile::modeBinary|WFile::modeCreateFile|WFile::modeReadWrite, WFile::shareUnknown, WFile::permReadWrite );
				for (int i1 = i; i1 < sess->numf; i1++)
				{
					FileAreaSetRecord( fileDownload, i1 + 1 );
					fileDownload.Read( &u, sizeof( uploadsrec ) );
					FileAreaSetRecord( fileDownload, i1 );
					fileDownload.Write( &u, sizeof( uploadsrec ) );
				}
				--i;
				--sess->numf;
				FileAreaSetRecord( fileDownload, 0 );
				fileDownload.Read( &u, sizeof( uploadsrec ) );
				u.numbytes = sess->numf;
				FileAreaSetRecord( fileDownload, 0 );
				fileDownload.Write( &u, sizeof( uploadsrec ) );
				fileDownload.Close();
			}
			else if (ch == 'Q')
			{
				abort = true;
			}
		}
		i = nrecno(s, i);
        bool next = true;
		checka(&abort, &next);
        if (!next)
        {
            i = 0;
        }
	}
}


void removenotthere()
{
	if (!so())
	{
		return;
	}

	tmp_disable_conf( true );
	tmp_disable_pause( true );
	int autodel = 0;
	nl();
    sess->bout << "|#5Remove N/A files in all directories? ";
	if (yesno())
	{
		for (int i = 0; ((i < sess->num_dirs) && (udir[i].subnum != -1) &&
			(!app->localIO->LocalKeyPressed())); i++)
		{
			nl();
            sess->bout << "|#1Removing N/A|#0 in " << directories[udir[i].subnum].name;
			nl( 2 );
			removefilesnotthere(udir[i].subnum, &autodel);
		}
	}
	else
	{
		nl();
		sess->bout << "Removing N/A|#0 in " << directories[udir[sess->GetCurrentFileArea()].subnum].name;
		nl( 2 );
		removefilesnotthere(udir[sess->GetCurrentFileArea()].subnum, &autodel);
	}
	tmp_disable_conf( false );
	tmp_disable_pause( false );
	app->localIO->UpdateTopScreen();
}


int find_batch_queue( const char *pszFileName )
{
	for (int i = 0; i < sess->numbatch; i++)
	{
		if ( wwiv::stringUtils::IsEquals( pszFileName, batch[i].filename ) )
		{
			return i;
		}
	}

	return -1;
}


// Removes a file off the batch queue specified by pszFileNam,e
void remove_batch( const char *pszFileName )
{
	int batchNum = find_batch_queue( pszFileName );
	if ( batchNum > -1 )
	{
		delbatch( batchNum );
	}
}

