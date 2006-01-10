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
// private functions
//
int  comparedl(uploadsrec * x, uploadsrec * y, int type);
void quicksort(int l, int r, int type);
bool upload_file(const char *pszFileName, int nDirectoryNum, const char *pszDescription);
long db_index( WFile &fileAllow, const char *pszFileName);
void l_config_nscan();
void config_nscan();


void move_file()
{
	char s1[81], s2[81];
	int d1 = 0, d2 = 0;
	uploadsrec u, u1;

	bool ok = false;
	nl( 2 );
	sess->bout << "|#2Filename to move: ";
    char szFileMask[ MAX_PATH ];
	input(szFileMask, 12);
	if (strchr(szFileMask, '.') == NULL)
	{
		strcat(szFileMask, ".*");
	}
	align(szFileMask);
	dliscan();
	int nCurRecNum = recno(szFileMask);
	if (nCurRecNum < 0)
	{
		sess->bout << "\r\nFile not found.\r\n";
		return;
	}
	bool done = false;

	tmp_disable_conf( true );

	while ( !hangup && nCurRecNum > 0 && !done )
	{
		int nCurrentPos = nCurRecNum;
		WFile fileDownload( g_szDownloadFileName );
		fileDownload.Open( WFile::modeBinary | WFile::modeReadOnly );
		FileAreaSetRecord( fileDownload, nCurRecNum );
		fileDownload.Read( &u, sizeof( uploadsrec ) );
		fileDownload.Close();
		nl();
		printfileinfo(&u, udir[sess->GetCurrentFileArea()].subnum);
		nl();
		sess->bout << "|#5Move this (Y/N/Q)? ";
		char ch = ynq();
		if (ch == 'Q')
		{
			done = true;
		}
		else if (ch == 'Y')
		{
			sprintf(s1, "%s%s", directories[udir[sess->GetCurrentFileArea()].subnum].path, u.filename);
            char *ss = NULL;
			do
			{
				nl( 2 );
				sess->bout << "|#2To which directory? ";
				ss = mmkey( 1 );
				if (ss[0] == '?')
				{
					dirlist( 1 );
					dliscan();
				}
			} while ((!hangup) && (ss[0] == '?'));
			d1 = -1;
			if (ss[0])
			{
				for (int i1 = 0; (i1 < sess->num_dirs) && (udir[i1].subnum != -1); i1++)
				{
					if ( wwiv::stringUtils::IsEquals( udir[i1].keys, ss ) )
					{
						d1 = i1;
					}
				}
			}
			if (d1 != -1)
			{
				ok = true;
				d1 = udir[d1].subnum;
				dliscan1(d1);
				if (recno(u.filename) > 0)
				{
					ok = false;
					sess->bout << "\r\nFilename already in use in that directory.\r\n";
				}
				if (sess->numf >= directories[d1].maxfiles)
				{
					ok = false;
					sess->bout << "\r\nToo many files in that directory.\r\n";
				}
				if (freek1(directories[d1].path) < ((double) (u.numbytes / 1024L) + 3))
				{
					ok = false;
					sess->bout << "\r\nNot enough disk space to move it.\r\n";
				}
				dliscan();
			}
			else
			{
				ok = false;
			}
		}
		else
		{
			ok = false;
		}
		if (ok && !done)
		{
			sess->bout << "|#5Reset upload time for file? ";
			if (yesno())
			{
				time((long *) &u.daten);
			}
			--nCurrentPos;
			fileDownload.Open( WFile::modeBinary|WFile::modeCreateFile|WFile::modeReadWrite, WFile::shareUnknown, WFile::permReadWrite );
			for (int i1 = nCurRecNum; i1 < sess->numf; i1++)
			{
				FileAreaSetRecord( fileDownload, i1 + 1 );
				fileDownload.Read( &u1, sizeof( uploadsrec ) );
				FileAreaSetRecord( fileDownload, i1 );
				fileDownload.Write( &u1, sizeof( uploadsrec ) );
			}
			--sess->numf;
			FileAreaSetRecord( fileDownload, 0 );
			fileDownload.Read( &u1, sizeof( uploadsrec ) );
			u1.numbytes = sess->numf;
			FileAreaSetRecord( fileDownload, 0 );
			fileDownload.Write( &u1, sizeof( uploadsrec ) );
			fileDownload.Close();
			char* ss = read_extended_description(u.filename);
			if (ss)
			{
				delete_extended_description(u.filename);
			}

			sprintf(s2, "%s%s", directories[d1].path, u.filename);
			dliscan1(d1);
			fileDownload.Open( WFile::modeBinary|WFile::modeCreateFile|WFile::modeReadWrite, WFile::shareUnknown, WFile::permReadWrite );
			for (int i = sess->numf; i >= 1; i--)
			{
				FileAreaSetRecord( fileDownload, i );
				fileDownload.Read( &u1, sizeof( uploadsrec ) );
				FileAreaSetRecord( fileDownload, i + 1 );
				fileDownload.Write( &u1, sizeof( uploadsrec ) );
			}
			FileAreaSetRecord( fileDownload, 1 );
			fileDownload.Write( &u, sizeof( uploadsrec ) );
			++sess->numf;
			FileAreaSetRecord( fileDownload, 0 );
			fileDownload.Read( &u1, sizeof( uploadsrec ) );
			u1.numbytes = sess->numf;
			if (u.daten > u1.daten)
			{
				u1.daten = u.daten;
				sess->m_DirectoryDateCache[d1] = u.daten;
			}
			FileAreaSetRecord( fileDownload, 0 );
			fileDownload.Write( &u1, sizeof( uploadsrec ) );
			fileDownload.Close();
			if (ss)
			{
				add_extended_description(u.filename, ss);
				BbsFreeMemory(ss);
			}
            StringRemoveWhitespace( s1 );
            StringRemoveWhitespace( s2 );
			if ( !wwiv::stringUtils::IsEquals(s1, s2) && WFile::Exists( s1 ) )
			{
				d2 = 0;
				if ((s1[1] != ':') && (s2[1] != ':'))
				{
					d2 = 1;
				}
				if ((s1[1] == ':') && (s2[1] == ':') && (s1[0] == s2[0]))
				{
					d2 = 1;
				}
				if (d2)
				{
					WFile::Rename(s1, s2);
					if (WFile::Exists(s2))
					{
						WFile::Remove(s1);
					}
					else
					{
						copyfile(s1, s2, false);
						WFile::Remove(s1);
					}
				}
				else
				{
					copyfile(s1, s2, false);
					WFile::Remove(s1);
				}
			}
			sess->bout << "\r\nFile moved.\r\n";
		}
		dliscan();
		nCurRecNum = nrecno(szFileMask, nCurrentPos);
  }

  tmp_disable_conf( false );
}


int comparedl(uploadsrec * x, uploadsrec * y, int type)
{
	switch ( type )
	{
	case 0:
        return wwiv::stringUtils::StringCompare( x->filename, y->filename );
    case 1:
		if ( x->daten < y->daten )
		{
			return -1;
		}
		return (x->daten > y->daten) ? 1 : 0;
    case 2:
		if ( x->daten < y->daten )
		{
			return 1;
		}
		return ( ( x->daten > y->daten ) ? -1 : 0 );
	}
	return 0;
}


void quicksort(int l, int r, int type)
{
	uploadsrec a, a2, x;

	register int i = l;
	register int j = r;
	WFile fileDownload( g_szDownloadFileName );
	fileDownload.Open( WFile::modeBinary|WFile::modeCreateFile|WFile::modeReadWrite, WFile::shareUnknown, WFile::permReadWrite );

	FileAreaSetRecord( fileDownload, ((l + r) / 2) );
	fileDownload.Read( &x, sizeof( uploadsrec ) );
	do
	{
		FileAreaSetRecord( fileDownload, i );
		fileDownload.Read( &a, sizeof( uploadsrec ) );
		while (comparedl(&a, &x, type) < 0)
		{
			FileAreaSetRecord( fileDownload, ++i );
			fileDownload.Read( &a, sizeof( uploadsrec ) );
		}
		FileAreaSetRecord( fileDownload, j );
		fileDownload.Read( &a2, sizeof( uploadsrec ) );
		while (comparedl(&a2, &x, type) > 0)
		{
			FileAreaSetRecord( fileDownload, --j );
			fileDownload.Read( &a2, sizeof( uploadsrec ) );
		}
		if (i <= j)
		{
			if (i != j)
			{
				FileAreaSetRecord( fileDownload, i );
				fileDownload.Write( &a2, sizeof( uploadsrec ) );
				FileAreaSetRecord( fileDownload, j );
				fileDownload.Write( &a, sizeof( uploadsrec ) );
			}
			i++;
			j--;
		}
	} while ( i < j );
	fileDownload.Close();
	if (l < j)
	{
		quicksort(l, j, type);
	}
	if (i < r)
	{
		quicksort(i, r, type);
	}
}


void sortdir( int nDirectoryNum, int type )
{
	dliscan1( nDirectoryNum );
	if (sess->numf > 1)
	{
		quicksort( 1, sess->numf, type );
	}
}


void sort_all(int type)
{
	tmp_disable_conf( true );
	for ( int i = 0; ( i < sess->num_dirs ) && ( udir[i].subnum != -1 ) &&
		( !app->localIO->LocalKeyPressed() ); i++ )
	{
		sess->bout << "\r\n|#1Sorting " << directories[udir[i].subnum].name << wwiv::endl;
		sortdir( i, type );
	}
	tmp_disable_conf( false );
}


void rename_file()
{
	char s[81], s1[81], s2[81], *ss, s3[81];
	uploadsrec u;

	nl( 2 );
	sess->bout << "|#2File to rename: ";
	input(s, 12);
	if (s[0] == 0)
	{
		return;
	}
	if (strchr(s, '.') == NULL)
	{
		strcat(s, ".*");
	}
	align(s);
	dliscan();
	nl();
	strcpy(s3, s);
	int nRecNum = recno(s);
	while ( nRecNum > 0 && !hangup )
	{
		WFile fileDownload( g_szDownloadFileName );
		fileDownload.Open( WFile::modeBinary | WFile::modeReadOnly );
		int nCurRecNum = nRecNum;
		FileAreaSetRecord( fileDownload, nRecNum );
		fileDownload.Read( &u, sizeof( uploadsrec ) );
		fileDownload.Close();
		nl();
		printfileinfo(&u, udir[sess->GetCurrentFileArea()].subnum);
		nl();
		sess->bout << "|#5Change info for this file (Y/N/Q)? ";
		char ch = ynq();
		if (ch == 'Q')
		{
			break;
		}
		else if (ch == 'N')
		{
			nRecNum = nrecno(s3, nCurRecNum);
			continue;
		}
		nl();
		sess->bout << "|#2New filename? ";
		input(s, 12);
		if (!okfn(s))
		{
			s[0] = 0;
		}
		if (s[0])
		{
			align( s );
			if ( !wwiv::stringUtils::IsEquals( s, "        .   " ) )
			{
				strcpy(s1, directories[udir[sess->GetCurrentFileArea()].subnum].path);
				strcpy(s2, s1);
				strcat(s1, s);
                StringRemoveWhitespace( s1 );
				if (WFile::Exists(s1))
				{
					sess->bout << "Filename already in use; not changed.\r\n";
				}
				else
				{
					strcat(s2, u.filename);
                    StringRemoveWhitespace( s2 );
					WFile::Rename(s2, s1);
					if (WFile::Exists(s1))
					{
						ss = read_extended_description(u.filename);
						if (ss)
						{
							delete_extended_description(u.filename);
							add_extended_description(s, ss);
							BbsFreeMemory(ss);
						}
						strcpy(u.filename, s);
					}
					else
					{
						sess->bout << "Bad filename.\r\n";
					}
				}
			}
		}
		sess->bout << "\r\nNew description:\r\n|#2: ";
		inputl(s, 58);
		if (s[0])
		{
			strcpy(u.description, s);
		}
		ss = read_extended_description(u.filename);
		nl( 2 );
		sess->bout << "|#5Modify extended description? ";
		if (yesno())
		{
			nl();
			if (ss)
			{
				sess->bout << "|#5Delete it? ";
				if (yesno())
				{
					BbsFreeMemory(ss);
					delete_extended_description(u.filename);
					u.mask &= ~mask_extended;
				}
				else
				{
					u.mask |= mask_extended;
					modify_extended_description(&ss,
						directories[udir[sess->GetCurrentFileArea()].subnum].name, u.filename);
					if (ss)
					{
						delete_extended_description(u.filename);
						add_extended_description(u.filename, ss);
						BbsFreeMemory(ss);
					}
				}
			}
			else
			{
				modify_extended_description(&ss,
					directories[udir[sess->GetCurrentFileArea()].subnum].name, u.filename);
				if (ss)
				{
					add_extended_description(u.filename, ss);
					BbsFreeMemory(ss);
					u.mask |= mask_extended;
				}
				else
				{
					u.mask &= ~mask_extended;
				}
			}
		}
		else if (ss)
		{
			BbsFreeMemory(ss);
			u.mask |= mask_extended;
		}
		else
		{
			u.mask &= ~mask_extended;
		}
		fileDownload.Open( WFile::modeBinary|WFile::modeCreateFile|WFile::modeReadWrite, WFile::shareUnknown, WFile::permReadWrite );
		FileAreaSetRecord( fileDownload, nRecNum );
		fileDownload.Write( &u, sizeof( uploadsrec ) );
		fileDownload.Close();
		nRecNum = nrecno(s3, nCurRecNum);
    }
}


bool upload_file( const char *pszFileName, int nDirectoryNum, const char *pszDescription )
{
	uploadsrec u, u1;

	directoryrec d = directories[nDirectoryNum];
	char szTempFileName[ 255 ];
	strcpy( szTempFileName, pszFileName );
	align( szTempFileName );
	strcpy( u.filename, szTempFileName );
	u.ownerusr = static_cast< unsigned short >( sess->usernum );
	u.ownersys = 0;
	u.numdloads = 0;
	u.filetype = 0;
	u.mask = 0;
	if ( !( d.mask & mask_cdrom ) && !check_ul_event( nDirectoryNum, &u ) )
	{
		sess->bout << pszFileName << " was deleted by upload event.\r\n";
	}
	else
	{
        char szUnalignedFileName[ MAX_PATH ];
        strcpy( szUnalignedFileName, szTempFileName );
        unalign( szUnalignedFileName );

		char szFullPathName[ MAX_PATH ];
		sprintf( szFullPathName, "%s%s", d.path, szUnalignedFileName );

		WFile fileUpload( szFullPathName );
		if ( !fileUpload.Open( WFile::modeBinary | WFile::modeReadOnly ) )
		{
			if ( pszDescription && ( *pszDescription ) )
			{
                sess->bout << "ERR: " << pszFileName << ":" << pszDescription << wwiv::endl;
			}
			else
			{
                sess->bout << "|#1" << pszFileName << " does not exist.";
			}
			return true;
		}
		long lFileSize = fileUpload.GetLength();
		u.numbytes = lFileSize;
		fileUpload.Close();
        strcpy( u.upby, sess->thisuser.GetUserNameAndNumber( sess->usernum ) );
		strcpy( u.date, date() );
		filedate( szFullPathName, u.actualdate );
		if ( d.mask & mask_PD )
		{
			d.mask = mask_PD;
		}
        nl();

        char szTempDisplayFileName[ MAX_PATH ];
        strcpy( szTempDisplayFileName, u.filename );
		sess->bout << "|#9File name   : |#2" << StringRemoveWhitespace( szTempDisplayFileName ) << wwiv::endl;
		sess->bout << "|#9File size   : |#2" << bytes_to_k( u.numbytes ) << wwiv::endl;
		if ( pszDescription && *pszDescription )
		{
			strncpy( u.description, pszDescription, 58 );
			u.description[58] = '\0';
			sess->bout << "|#1 Description: " << u.description << wwiv::endl;
		}
		else
		{
            sess->bout << "|#9Enter a description for this file.\r\n|#7: ";
			inputl( u.description, 58, true );
		}
        nl();
		if ( u.description[0] == 0 )
		{
			return false;
		}
		get_file_idz( &u, nDirectoryNum );
        sess->thisuser.SetFilesUploaded( sess->thisuser.GetFilesUploaded() + 1 );
		if ( !( d.mask & mask_cdrom ) )
		{
			modify_database( u.filename, true );
		}
		sess->thisuser.SetUploadK( sess->thisuser.GetUploadK() + bytes_to_k( lFileSize ) );
		time_t tCurrentTime;
		time( &tCurrentTime );
		u.daten = static_cast<unsigned long>( tCurrentTime );
		WFile fileDownload( g_szDownloadFileName );
		fileDownload.Open( WFile::modeBinary|WFile::modeCreateFile|WFile::modeReadWrite, WFile::shareUnknown, WFile::permReadWrite );
		for ( int i = sess->numf; i >= 1; i-- )
		{
			FileAreaSetRecord( fileDownload, i );
			fileDownload.Read( &u1, sizeof( uploadsrec ) );
			FileAreaSetRecord( fileDownload, i + 1 );
			fileDownload.Write( &u1, sizeof( uploadsrec ) );
		}
		FileAreaSetRecord( fileDownload, 1 );
		fileDownload.Write( &u, sizeof( uploadsrec ) );
		++sess->numf;
		FileAreaSetRecord( fileDownload, 0 );
		fileDownload.Read( &u1, sizeof( uploadsrec ) );
		u1.numbytes = sess->numf;
		u1.daten = static_cast<unsigned long>( tCurrentTime );
		sess->m_DirectoryDateCache[nDirectoryNum] = static_cast<unsigned long>( tCurrentTime );
		FileAreaSetRecord( fileDownload, 0 );
		fileDownload.Write( &u1, sizeof( uploadsrec ) );
		fileDownload.Close();
		app->statusMgr->Lock();
		++status.uptoday;
		++status.filechange[filechange_upload];
		app->statusMgr->Write();
		sysoplogf( "+ \"%s\" uploaded on %s", u.filename, d.name);
		app->localIO->UpdateTopScreen();
	}
	return true;
}


bool maybe_upload( const char *pszFileName, int nDirectoryNum, const char *pszDescription )
{
	char s[81], ch, s1[81];
	bool abort = false;
	bool ok = true;
	uploadsrec u;

	strcpy(s, pszFileName);
	align(s);
	int i = recno(s);

	if (i == -1)
	{
		if ( app->HasConfigFlag( OP_FLAGS_FAST_SEARCH ) && ( !is_uploadable(pszFileName) && dcs() ) )
        {
            bprintf("|#2%-12s: ", pszFileName);
			sess->bout << "|#5In filename database - add anyway? ";
			ch = ynq();
			if (ch == *str_quit)
			{
				return false;
			}
			else
			{
				if (ch == *(YesNoString( false )))
				{
					sess->bout << "|#5Delete it? ";
					if (yesno())
                    {
						sprintf(s1, "%s%s", directories[nDirectoryNum].path, pszFileName);
						WFile::Remove(s1);
						nl();
						return true;
					}
					else
					{
						return true;
					}
				}
			}
		}
		if (!upload_file(s, udir[nDirectoryNum].subnum, pszDescription))
		{
			ok = false;
		}
	}
	else
	{
		WFile fileDownload( g_szDownloadFileName );
		fileDownload.Open( WFile::modeBinary | WFile::modeReadOnly );
		FileAreaSetRecord( fileDownload, i );
		fileDownload.Read( &u, sizeof( uploadsrec ) );
		fileDownload.Close();
		int ocd = sess->GetCurrentFileArea();
		sess->SetCurrentFileArea( nDirectoryNum );
		printinfo( &u, &abort );
		sess->SetCurrentFileArea( ocd );
		if ( abort )
		{
			ok = false;
		}
	}
	return ok;
}


/* This assumes the file holds listings of files, one per line, to be
 * uploaded.  The first word (delimited by space/tab) must be the filename.
 * after the filename are optional tab/space separated words (such as file
 * size or date/time).  After the optional words is the description, which
 * is from that position to the end of the line.  the "type" parameter gives
 * the number of optional words between the filename and description.
 * the optional words (size, date/time) are ignored completely.
 */
void upload_files(const char *pszFileName, int nDirectoryNum, int type)
{
	char s[255], *fn1 = NULL, *pszDescription = NULL, last_fn[81], *ext = NULL;
	bool abort = false, next = false;
    int ok1, i;
	bool ok = true;
	uploadsrec u;

	last_fn[0] = 0;
	dliscan1(udir[nDirectoryNum].subnum);

	FILE* f = fsh_open(const_cast<char*>(pszFileName), "r");
	if (!f)
	{
		sprintf(s, "%s%s", directories[udir[nDirectoryNum].subnum].path, pszFileName);
		f = fsh_open(s, "r");
	}
	if (!f)
	{
		sess->bout << pszFileName << ": not found.\r\n";
	}
	else
	{
		while (ok && fgets(s, 250, f))
		{
			if (s[0] < SPACE)
			{
				continue;
			}
			else if (s[0] == SPACE)
			{
				if (last_fn[0])
				{
					if (!ext)
					{
						ext = static_cast<char *>( BbsAllocA( 4096L ) );
						*ext = 0;
					}
					for (pszDescription = s; (*pszDescription == ' ') || (*pszDescription == '\t'); pszDescription++);
					if (*pszDescription == '|')
					{
						do
						{
							pszDescription++;
						} while ((*pszDescription == ' ') || (*pszDescription == '\t'));
					}
					fn1 = strchr(pszDescription, '\n');
					if (fn1)
					{
						*fn1 = 0;
					}
					strcat(ext, pszDescription);
					strcat(ext, "\r\n");
				}
			}
			else
			{
				ok1 = 0;
				fn1 = strtok(s, " \t\n");
				if (fn1)
				{
					ok1 = 1;
					for (i = 0; ok1 && (i < type); i++)
					{
						if (strtok(NULL, " \t\n") == NULL)
						{
							ok1 = 0;
						}
					}
					if (ok1)
					{
						pszDescription = strtok(NULL, "\n");
						if (!pszDescription)
						{
							ok1 = 0;
						}
					}
				}
				if (ok1)
				{
					if (last_fn[0] && ext && *ext)
					{
						WFile fileDownload( g_szDownloadFileName );
						fileDownload.Open( WFile::modeBinary|WFile::modeCreateFile|WFile::modeReadWrite, WFile::shareUnknown, WFile::permReadWrite );
						FileAreaSetRecord( fileDownload, 1 );
						fileDownload.Read( &u, sizeof( uploadsrec ) );
						if ( wwiv::stringUtils::IsEquals( last_fn, u.filename ) )
						{
							modify_database(u.filename, true);
							add_extended_description(last_fn, ext);
							u.mask |= mask_extended;
							FileAreaSetRecord( fileDownload, 1 );
							fileDownload.Write( &u, sizeof( uploadsrec ) );
						}
						fileDownload.Close();
						*ext = 0;
					}
					while ( *pszDescription == ' ' || *pszDescription == '\t' )
					{
						++pszDescription;
					}
					ok = maybe_upload( fn1, nDirectoryNum, pszDescription );
					checka(&abort, &next);
					if (abort)
					{
						ok = false;
					}
					if (ok)
					{
						strcpy(last_fn, fn1);
						align(last_fn);
						if (ext)
						{
							*ext = 0;
						}
					}
				}
			}
		}
		fsh_close(f);
		if (ok && last_fn[0] && ext && *ext)
		{
			WFile fileDownload( g_szDownloadFileName );
			fileDownload.Open( WFile::modeBinary|WFile::modeCreateFile|WFile::modeReadWrite, WFile::shareUnknown, WFile::permReadWrite );
			FileAreaSetRecord( fileDownload, 1 );
			fileDownload.Read( &u, sizeof( uploadsrec ) );
			if ( wwiv::stringUtils::IsEquals( last_fn, u.filename ) )
			{
				modify_database(u.filename, true);
				add_extended_description(last_fn, ext);
				u.mask |= mask_extended;
				FileAreaSetRecord( fileDownload, 1 );
				fileDownload.Write( &u, sizeof( uploadsrec ) );
			}
			fileDownload.Close();
		}
  }

  if ( ext )
  {
	  BbsFreeMemory( ext );
  }
}

// returns false on abort
bool uploadall(int nDirectoryNum)
{
	dliscan1(udir[nDirectoryNum].subnum);

    char szDefaultFileSpec[ MAX_PATH ];
	strcpy(szDefaultFileSpec, "*.*");

    char szPathName[ MAX_PATH ];
	sprintf( szPathName, "%s%s", directories[udir[nDirectoryNum].subnum].path, szDefaultFileSpec );
	int maxf = directories[udir[nDirectoryNum].subnum].maxfiles;

    WFindFile fnd;
	bool bFound = fnd.open(szPathName, 0);
	bool ok = true;
	bool abort = false;
	while ( bFound && !hangup && (sess->numf < maxf) && ok && !abort )
	{
        const char *pszCurrentFile = fnd.GetFileName();
        if ( pszCurrentFile &&
             *pszCurrentFile &&
             !wwiv::stringUtils::IsEquals( pszCurrentFile, "." ) &&
             !wwiv::stringUtils::IsEquals( pszCurrentFile, ".." ) )
        {
		    ok = maybe_upload(pszCurrentFile, nDirectoryNum, NULL);
        }
		bFound = fnd.next();
        bool next;
		checka(&abort, &next);
	}
	if ( !ok || abort )
	{
		sess->bout << "|12Aborted.\r\n";
        ok = false;
	}
	if (sess->numf >= maxf)
	{
		sess->bout << "directory full.\r\n";
	}
	return ok;
}


void relist()
{
	char s[85], s1[40], s2[81];
	int i, i1;
    bool next, abort = 0;
	int tcd = -1, otag, tcdi;

	if (!filelist)
	{
		return;
	}
	ClearScreen();
	lines_listed = 0;
	otag = sess->tagging;
	sess->tagging = 0;
	if ( app->HasConfigFlag( OP_FLAGS_FAST_TAG_RELIST ) )
	{
		ansic( sess->thisuser.isUseExtraColor() ? FRAME_COLOR : 0 );
		if ( okansi() )
		{
			sess->bout << "������������������������������������������������������������������������������\r\n";
		}
		else
		{
			sess->bout << "--+------------+-----+----+----------------------------------------------------\r\n";
		}
	}
	for (i = 0; i < sess->tagptr; i++)
	{
		if ( !app->HasConfigFlag( OP_FLAGS_FAST_TAG_RELIST ) )
		{
			if (tcd != filelist[i].directory)
			{
				ansic( sess->thisuser.isUseExtraColor() ? FRAME_COLOR : 0 );
				if (tcd != -1)
				{
					if ( okansi() )
					{
						sess->bout << "\r" << "������������������������������������������������������������������������������" << wwiv::endl;
					}
					else
					{
						sess->bout << "\r" << "--+------------+-----+----+---------------------------------------------------" << wwiv::endl;
					}
				}
				tcd = filelist[i].directory;
				tcdi = -1;
				for (i1 = 0; i1 < sess->num_dirs; i1++)
				{
					if (udir[i1].subnum == tcd)
					{
						tcdi = i1;
						break;
					}
				}
				if ( sess->thisuser.isUseExtraColor() )
				{
					ansic( 2 );
				}
				if (tcdi == -1)
				{
					sess->bout << directories[tcd].name << "." << wwiv::endl;
				}
				else
				{
					sess->bout << directories[tcd].name << " - #" << udir[tcdi].keys << ".\r\n";
				}
				ansic( sess->thisuser.isUseExtraColor() ? FRAME_COLOR : 0 );
				if ( okansi() )
				{
					sess->bout << "������������������������������������������������������������������������������" << wwiv::endl;
				}
				else
				{
					sess->bout << "--+------------+-----+----+---------------------------------------------------" << wwiv::endl;
				}
			}
		}
		sprintf(s, "%c%d%2d%c%d%c",
			0x03,
			check_batch_queue(filelist[i].u.filename) ? 6 : 0,
			i + 1,
			0x03,
			sess->thisuser.isUseExtraColor() ? FRAME_COLOR : 0,
			okansi() ? '�' : '|');
		osan(s, &abort, &next);
		if ( sess->thisuser.isUseExtraColor() )
		{
			ansic( 1 );
		}
		strncpy(s, filelist[i].u.filename, 8);
		s[8] = 0;
		osan(s, &abort, &next);
		strncpy(s, &((filelist[i].u.filename)[8]), 4);
		s[4] = 0;
		if ( sess->thisuser.isUseExtraColor() )
		{
			ansic( 1 );
		}
		osan(s, &abort, &next);
		ansic( sess->thisuser.isUseExtraColor() ? FRAME_COLOR : 0 );
		osan((okansi() ? "�" : ":"), &abort, &next);

        sprintf( s1, "%ld""k", bytes_to_k( filelist[i].u.numbytes ) );
		if ( !app->HasConfigFlag( OP_FLAGS_FAST_TAG_RELIST ) )
		{
			if (!(directories[tcd].mask & mask_cdrom))
			{
                sprintf( s2, "%s%s", directories[tcd].path, filelist[i].u.filename );
                StringRemoveWhitespace( s2 );
				if (!WFile::Exists(s2))
				{
					strcpy(s1, "N/A");
				}
			}
		}
		for (i1 = 0; i1 < 5 - wwiv::stringUtils::GetStringLength(s1); i1++)
		{
			s[i1] = SPACE;
		}
		s[i1] = 0;
		strcat(s, s1);
		if ( sess->thisuser.isUseExtraColor() )
		{
			ansic( 2 );
		}
		osan(s, &abort, &next);

		ansic( sess->thisuser.isUseExtraColor() ? FRAME_COLOR : 0 );
		osan((okansi() ? "�" : "|"), &abort, &next);
		sprintf(s1, "%d", filelist[i].u.numdloads);

		for (i1 = 0; i1 < 4 - wwiv::stringUtils::GetStringLength(s1); i1++)
		{
			s[i1] = SPACE;
		}
		s[i1] = 0;
		strcat(s, s1);
		if ( sess->thisuser.isUseExtraColor() )
		{
			ansic( 2 );
		}
		osan(s, &abort, &next);

		ansic( sess->thisuser.isUseExtraColor() ? FRAME_COLOR : 0 );
		osan((okansi() ? "�" : "|"), &abort, &next);
		sprintf(s, "%c%d%s",
			    0x03,
			    (filelist[i].u.mask & mask_extended) ? 1 : 2,
			    filelist[i].u.description);
		plal(s, sess->thisuser.GetScreenChars() - 28, &abort);
  }
  ansic( sess->thisuser.isUseExtraColor() ? FRAME_COLOR : 0 );
  if ( okansi() )
  {
	  sess->bout << "\r" << "������������������������������������������������������������������������������" << wwiv::endl;
  }
  else
  {
	  sess->bout << "\r" << "--+------------+-----+----+---------------------------------------------------" << wwiv::endl;
  }
  sess->tagging = otag;
  lines_listed = 0;
}

void edit_database()
/*
 * Allows user to add or remove ALLOW.DAT entries.
 */
{
	char ch, s[20];
	bool done = false;

	if ( !app->HasConfigFlag( OP_FLAGS_FAST_SEARCH ) )
	{
		return;
	}

	do
	{
		nl();
		sess->bout << "|#2A|#7)|#9 Add to ALLOW.DAT\r\n";
		sess->bout << "|#2R|#7)|#9 Remove from ALLOW.DAT\r\n";
		sess->bout << "|#2Q|#7)|#9 Quit\r\n";
		nl();
		sess->bout << "|#7Select: ";
		ch = onek("QAR");
		switch (ch)
		{
		case 'A':
			nl();
			sess->bout << "|#2Filename: ";
			input( s, 12, true );
			if (s[0])
			{
				modify_database(s, true);
			}
			break;
		case 'R':
			nl();
			sess->bout << "|#2Filename: ";
			input( s, 12, true );
			if (s[0])
			{
				modify_database(s, false);
			}
			break;
		case 'Q':
			done = true;
			break;
		}
	} while ( !hangup && !done );
}


/*
 * Checks the database file to see if filename is there.  If so, the record
 * number plus one is returned.  If not, the negative of record number
 * that it would be minus one is returned.  The plus one is there so that there
 * is no record #0 ambiguity.
 *
 */



long db_index(WFile &fileAllow, const char *pszFileName)
{
	char cfn[18], tfn[81], tfn1[81];
	int i = 0;
	long hirec, lorec, currec, ocurrec = -1;

	strcpy(tfn1, pszFileName);
	align(tfn1);
	strcpy(tfn, stripfn(tfn1));

	hirec = fileAllow.GetLength() / 13;
	lorec = 0;

	if (hirec == 0)
	{
		return -1;
	}

	for ( ;; )
	{
		currec = ( hirec + lorec ) / 2;
		if (currec == ocurrec)
		{
			if (i < 0)
			{
				return (-(currec + 2));
			}
			else
			{
				return (-(currec + 1));
			}
		}
		ocurrec = currec;
		fileAllow.Seek( currec * 13, WFile::seekBegin );
		fileAllow.Read( &cfn, 13 );
        i = wwiv::stringUtils::StringCompare( cfn, tfn );

		// found
		if (i == 0)
		{
			return (currec + 1);
		}
		else
		{
			if (i < 0)
			{
				lorec = currec;
			}
			else
			{
				hirec = currec;
			}
		}
	}
}

/*
 * Adds or deletes a single entry to/from filename database.
 */

#define ALLOW_BUFSIZE 61440

void modify_database(const char *pszFileName, bool add)
{
	char tfn[MAX_PATH], tfn1[MAX_PATH];
	unsigned int nb;
	long l, l1, cp;

	if ( !app->HasConfigFlag( OP_FLAGS_FAST_SEARCH ) )
	{
		return;
	}

	WFile fileAllow( syscfg.datadir, ALLOW_DAT );
	if ( !fileAllow.Open( WFile::modeBinary | WFile::modeCreateFile | WFile::modeReadWrite, WFile::shareUnknown, WFile::permReadWrite ) )
	{
		return;
	}

	long rec = db_index( fileAllow, pszFileName );

	if ( ( rec < 0 && !add ) || ( rec > 0 && add ) )
	{
		fileAllow.Close();
		return;
	}
	char * bfr = static_cast<char *>( BbsAllocA( ALLOW_BUFSIZE ) );
	if (!bfr)
	{
		fileAllow.Close();
		return;
	}
	long len = fileAllow.GetLength();

	if (add)
	{
		cp = (-1 - rec) * 13;
		l1 = len;

		do
		{
			l = l1 - cp;
			if ( l < ALLOW_BUFSIZE )
			{
				nb = ( unsigned int ) l;
			}
			else
			{
				nb = ALLOW_BUFSIZE;
			}
			if (nb)
			{
				fileAllow.Seek( l1 - static_cast<long>( nb ), WFile::seekBegin );
				fileAllow.Read( bfr, nb );
				fileAllow.Seek( l1 - static_cast<long>( nb ) + 13, WFile::seekBegin );
				fileAllow.Write( bfr, nb );
				l1 -= nb;
			}
		} while ( nb == ALLOW_BUFSIZE );

		// put in the new value
		strcpy(tfn1, pszFileName);
		align(tfn1);
		strncpy(tfn, stripfn(tfn1), 13);
		fileAllow.Seek( cp, WFile::seekBegin );
		fileAllow.Write( tfn, 13 );
	}
	else
	{
		cp = rec * 13;

		do
		{
			l = len - cp;
			if (l < ALLOW_BUFSIZE)
			{
				nb = (unsigned int) l;
			}
			else
			{
				nb = ALLOW_BUFSIZE;
			}
			if (nb)
			{
				fileAllow.Seek(  cp, WFile::seekBegin );
				fileAllow.Read( bfr, nb );
				fileAllow.Seek( cp - 13, WFile::seekBegin );
				fileAllow.Write( bfr, nb );
				cp += nb;
			}
		} while (nb == ALLOW_BUFSIZE);

		// truncate the file
		fileAllow.SetLength( len - 13 );
	}

	BbsFreeMemory( bfr );
	fileAllow.Close();
}

/*
 * Returns 1 if file not found in filename database.
 */

bool is_uploadable(const char *pszFileName)
{
	if ( !app->HasConfigFlag( OP_FLAGS_FAST_SEARCH ) )
	{
		return true;
	}

	WFile fileAllow( syscfg.datadir, ALLOW_DAT );
	if ( !fileAllow.Open( WFile::modeBinary | WFile::modeReadOnly ) )
	{
		return true;
	}
	long rc = db_index( fileAllow, pszFileName );
	fileAllow.Close();
	return (rc < 0) ? true : false;
}

void l_config_nscan()
{
	int i, i1;
	char s[81], s2[81];

	bool abort = false;
	nl();
	sess->bout << "|#9Directories to new-scan marked with '|#2*|#9'\r\n\n";
	for (i = 0; (i < sess->num_dirs) && (udir[i].subnum != -1) && (!abort); i++)
	{
		i1 = udir[i].subnum;
		if (qsc_n[i1 / 32] & (1L << (i1 % 32)))
		{
			strcpy(s, "* ");
		}
		else
		{
			strcpy(s, "  ");
		}
		sprintf(s2, "%s%s. %s", s, udir[i].keys, directories[i1].name);
		pla(s2, &abort);
	}
	nl( 2 );
}

void config_nscan()
{
	char *s, s1[MAX_CONFERENCES + 2], s2[120], ch;
	int i, i1, oc, os;
    bool abort = false;
	bool done, done1;

	if( okansi() )
	{									// ZU - SCONFIG
		config_scan_plus(NSCAN);		// ZU - SCONFIG
		return;							// ZU - SCONFIG
	}									// ZU - SCONFIG

	done = done1 = false;
	oc = sess->GetCurrentConferenceFileArea();
	os = udir[sess->GetCurrentFileArea()].subnum;

	do
	{
		if ( okconf( &sess->thisuser ) && uconfdir[1].confnum != -1 )
		{
			abort = false;
			strcpy(s1, " ");
			nl();
			sess->bout << "Select Conference: \r\n\n";
			i = 0;
			while ((i < dirconfnum) && (uconfdir[i].confnum != -1) && (!abort))
			{
				sprintf( s2, "%c) %s", dirconfs[uconfdir[i].confnum].designator,
					     stripcolors( reinterpret_cast<char*>( dirconfs[uconfdir[i].confnum].name ) ) );
				pla(s2, &abort);
				s1[i + 1] = dirconfs[uconfdir[i].confnum].designator;
				s1[i + 2] = 0;
				i++;
			}
			nl();
			sess->bout << " Select [" << &s1[1] << ", <space> to quit]: ";
			ch = onek(s1);
		}
		else
		{
			ch = '-';
		}
		switch (ch)
		{
		case ' ':
			done1 = true;
			break;
		default:
			if ( okconf( &sess->thisuser ) && dirconfnum > 1 )
			{
				i = 0;
				while ( ( ch != dirconfs[uconfdir[i].confnum].designator ) && (i < static_cast<int>( dirconfnum ) ) )
				{
					i++;
				}
				if ( i >= static_cast<int>( dirconfnum ) )
				{
					break;
				}

				setuconf(CONF_DIRS, i, -1);
			}
			l_config_nscan();
			done = false;
			do
			{
				nl();
				sess->bout << "|#9Enter directory number (|#1C=Clr All, Q=Quit, S=Set All|#9): |#0";
				s = mmkey( 1 );
				if (s[0])
				{
					for (i = 0; i < sess->num_dirs; i++)
					{
						i1 = udir[i].subnum;
						if ( wwiv::stringUtils::IsEquals( udir[i].keys, s ) )
						{
							qsc_n[i1 / 32] ^= 1L << (i1 % 32);
						}
						if ( wwiv::stringUtils::IsEquals( s, "S" ) )
						{
							qsc_n[i1 / 32] |= 1L << (i1 % 32);
						}
						if ( wwiv::stringUtils::IsEquals( s, "C" ) )
						{
							qsc_n[i1 / 32] &= ~(1L << (i1 % 32));
						}
					}
					if ( wwiv::stringUtils::IsEquals( s, "Q" ) )
					{
						done = true;
					}
					if ( wwiv::stringUtils::IsEquals( s, "?" ) )
					{
						l_config_nscan();
					}
				}
			} while ( !done && !hangup );
			break;
		}
		if ( !okconf( &sess->thisuser ) || uconfdir[1].confnum == -1 )
        {
			done1 = true;
        }
	} while ( !done1 && !hangup );

	if ( okconf( &sess->thisuser ) )
	{
		setuconf( CONF_DIRS, oc, os );
	}
}


void xfer_defaults()
{
	char s[81], ch;
	int i;
	bool done = false;

	do
	{
		ClearScreen();
		sess->bout <<  "|#7[|#21|#7]|#1 Set New-Scan Directories.\r\n";
		sess->bout <<  "|#7[|#22|#7]|#1 Set Default Protocol.\r\n";
		sess->bout <<  "|#7[|#23|#7]|#1 New-Scan Transfer after Message Base (" <<
                 YesNoString( sess->thisuser.isNewScanFiles() ) << ").\r\n";
		sess->bout << "|#7[|#24|#7]|#1 Number of lines of extended description to print [" << sess->thisuser.GetNumExtended() << " line(s)].\r\n";
		sess->bout << "|#7[|#25|#7]|#1 File sess->tagging (";
		if ( sess->thisuser.isUseNoTagging() )
		{
			sess->bout << "Disabled)\r\n";
		}
		else
		{
			sess->bout << "Enabled";
		}
		if ( !sess->thisuser.isUseListPlus() && !sess->thisuser.isUseNoTagging() )
		{
			sess->bout << " - ListPlus!)\r\n";
		}
		else if ( !sess->thisuser.isUseNoTagging() )
		{
			sess->bout << " - Internal Tagging)\r\n";
		}
		sess->bout << "|#7[|#2Q|#7]|#1 Quit.\r\n\n";
		sess->bout << "|#5Which? ";
		ch = onek("Q12345");
		switch (ch)
		{
		case 'Q':
			done = true;
			break;
		case '1':
			config_nscan();
			break;
		case '2':
			nl( 2 );
			sess->bout << "|#9Enter your default protocol, |#20|#9 for none.\r\n\n";
			i = get_protocol(xf_down);
			if (i >= 0)
			{
				sess->thisuser.SetDefaultProtocol( i );
			}
			break;
		case '3':
            sess->thisuser.toggleStatusFlag( WUser::nscanFileSystem );
			break;
		case '4':
			nl( 2 );
			sess->bout << "|#9How many lines of an extended description\r\n";
			sess->bout << "|#9do you want to see when listing files (|#20-" << sess->max_extend_lines << "|#7)\r\n";
			sess->bout << "|#9Current # lines: |#2" << sess->thisuser.GetNumExtended() << wwiv::endl;
            sess->bout << "|#7: ";
			input(s, 3);
			if (s[0])
			{
				i = atoi(s);
				if ((i >= 0) && (i <= sess->max_extend_lines))
				{
					sess->thisuser.SetNumExtended( i );
				}
			}
			break;
		case '5':
            if ( sess->thisuser.isUseNoTagging() )
			{
                sess->thisuser.clearStatusFlag( WUser::noTag );
				check_listplus();
			}
			else
			{
                sess->thisuser.setStatusFlag( WUser::noTag );
			}
			break;
		}
	} while ( !done && !hangup );
}



void finddescription()
{
	uploadsrec u;
	int i, i1, i2, pts, count, color, ac = 0;
	char s[81], s1[81];

	if (ok_listplus())
	{
		listfiles_plus(SEARCH_ALL);
		return;
	}

	nl();
	if ( uconfdir[1].confnum != -1 && okconf( &sess->thisuser ) )
	{
		if (!x_only)
		{
			sess->bout << "|#5All conferences? ";
			ac = yesno();
		}
		else
		{
			ac = 1;
		}
		if (ac)
		{
			tmp_disable_conf( true );
		}
	}
	sess->bout << "\r\nFind description -\r\n\n";
	sess->bout << "Enter string to search for in file description:\r\n:";
	input(s1, 58);
	if (s1[0] == 0)
	{
		tmp_disable_conf( false );
		return;
	}
	int ocd = sess->GetCurrentFileArea();
	bool abort = false;
    bool next = true;
	g_num_listed = 0;
	count = 0;
	color = 3;
	if (!x_only)
	{
		sess->bout << "\r|#2Searching ";
	}
	lines_listed = 0;
	for (i = 0; (i < sess->num_dirs) && (!abort) && (!hangup) && (sess->tagging != 0)
		&& (udir[i].subnum != -1); i++)
	{
		i1 = udir[i].subnum;
		pts = 0;
		sess->titled = 1;
		if (qsc_n[i1 / 32] & (1L << (i1 % 32)))
		{
			pts = 1;
		}
		pts = 1;
		// remove pts=1 to search only marked directories
		if ((pts) && (!abort) && (sess->tagging != 0))
		{
			if (!x_only)
			{
				count++;
				sess->bout << static_cast<char>( 3 ) << color << ".";
				if (count == NUM_DOTS)
				{
					sess->bout << "\r|#2Searching ";
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
			sess->SetCurrentFileArea( i );
			dliscan();
			WFile fileDownload( g_szDownloadFileName );
			fileDownload.Open( WFile::modeBinary | WFile::modeReadOnly );
			for (i1 = 1; (i1 <= sess->numf) && (!abort) && (!hangup) && (sess->tagging != 0); i1++)
			{
				FileAreaSetRecord( fileDownload, i1 );
				fileDownload.Read( &u, sizeof( uploadsrec ) );
				strcpy( s, u.description );
				for ( i2 = 0; i2 < wwiv::stringUtils::GetStringLength(s); i2++ )
				{
					s[i2] = upcase(s[i2]);
				}
				if (strstr(s, s1) != NULL)
				{
					fileDownload.Close();
					printinfo(&u, &abort);
					fileDownload.Open( WFile::modeBinary | WFile::modeReadOnly );
				}
				else if ( bkbhit() )
				{
					checka( &abort, &next );
				}
			}
			fileDownload.Close();
		}
	}
	if (ac)
	{
		tmp_disable_conf( false );
	}
	sess->SetCurrentFileArea( ocd );
	endlist( 1 );
}


void arc_l()
{
	char szFileSpec[MAX_PATH];
	uploadsrec u;

	nl();
	sess->bout << "|#2File for listing: ";
	input(szFileSpec, 12);
	if (strchr(szFileSpec, '.') == NULL)
	{
		strcat(szFileSpec, ".*");
	}
	if (!okfn(szFileSpec))
	{
		szFileSpec[0] = 0;
	}
	align(szFileSpec);
	dliscan();
	bool abort = false;
	bool next = false;
	int nRecordNum = recno(szFileSpec);
	do
	{
		if ( nRecordNum > 0 )
		{
			WFile fileDownload( g_szDownloadFileName );
			fileDownload.Open( WFile::modeBinary|WFile::modeReadOnly );
			FileAreaSetRecord( fileDownload, nRecordNum );
			fileDownload.Read( &u, sizeof( uploadsrec ) );
			fileDownload.Close();
			int i1 = list_arc_out(stripfn(u.filename), directories[udir[sess->GetCurrentFileArea()].subnum].path);
			if (i1)
			{
				abort = true;
			}
			checka(&abort, &next);
			nRecordNum = nrecno( szFileSpec, nRecordNum );
		}
	} while ( nRecordNum > 0 && !hangup && !abort );
}
