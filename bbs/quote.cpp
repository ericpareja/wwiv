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
// Local function prototypes
//
int ste(int i);
char *GetQuoteInitials();


#define LINELEN 79
#define SAVE_IN_MEM
#define PFXCOL 2
#define QUOTECOL 0

#define WRTPFX {fprintf(fpFile,"\x3%c",PFXCOL+48);if (tf==1)\
                cp=fsh_write(pfx,1,pfxlen-1,fpFile);\
                else cp=fsh_write(pfx,1,pfxlen,fpFile);\
                fprintf(fpFile,"\x3%c",cc);}
#define NL {if (!cp) {fprintf(fpFile,"\x3%c",PFXCOL+48);\
            fsh_write(pfx,1,pfxlen,fpFile);} if (ctlc) fsh_write("0",1,1,fpFile);\
            fsh_write("\r\n",1,2,fpFile);cp=ns=ctlc=0;}
#define FLSH {if (ss1) {if (cp && (l3+cp>=linelen)) NL else if (ns)\
              cp+=fsh_write(" ",1,1,fpFile);if (!cp) {if (ctld)\
              fprintf(fpFile,"\x4%c",ctld); WRTPFX; } fsh_write(ss1,1,l2,fpFile);\
              cp+=l3;ss1=NULL;l2=l3=0;ns=1;}}

static int brtnm;

static int quotes_nrm_l;
static int quotes_ind_l;


int ste(int i)
{
	if ( irt_name[i]==32 && irt_name[i+1]=='O' && irt_name[i+2]=='F' && irt_name[i+3]==32 )
	{
		if ( irt_name[ i+4 ] > 47 && irt_name[ i+4 ] < 58 )
		{
			return 0;
		}
	}
	if ( irt_name[i]==96 )
	{
		brtnm++;
	}
	return 1;
}


char *GetQuoteInitials()
{
    static char s_szQuoteInitials[8];

    brtnm = 0;
    if ( irt_name[0]==96 )
    {
        s_szQuoteInitials[0] = irt_name[2];
    }
    else if (irt_name[0]==34)
    {
        s_szQuoteInitials[0] = (irt_name[1]==96) ? irt_name[3] : irt_name[1];
    }
    else
    {
        s_szQuoteInitials[0] = irt_name[0];
    }

    int i1=1;
    for ( int i=1; i < wwiv::stringUtils::GetStringLength(irt_name) && i1 < 6 && irt_name[i] != '#' && irt_name[i] != '<' && ste(i) && brtnm != 2; i++ )
    {
        if ( irt_name[i]==32 && irt_name[i+1]!='#' && irt_name[i+1]!=96 && irt_name[i+1]!='<' )
        {
            if ( irt_name[ i + 1 ] == '(' )
            {
                if ( !isdigit( irt_name[ i + 2 ] ) )
                {
                    i1 = 0;
                }
                i++;
            }
            if ( irt_name[i] != '(' || !isdigit( irt_name[i + 1] ) )
            {
                s_szQuoteInitials[ i1++ ] = irt_name[ i+1 ];
            }
        }
    }
    s_szQuoteInitials[ i1 ] = 0;
    return s_szQuoteInitials;
}

void grab_quotes(messagerec * m, const char *aux)
{
    char *ss, *ss1, temp[255];
    long l, l1, l2, l3;
    FILE *fpFile;
    char *pfx;
    int cp = 0, ctla = 0, ctlc = 0, ns = 0, ctld = 0;
    int pfxlen;
    char szQuotesTextFileName[MAX_PATH], szQuotesIndexFileName[MAX_PATH];
    char cc=QUOTECOL+48;
    int linelen=LINELEN,tf=0;

    sprintf( szQuotesTextFileName, "%s%s", syscfgovr.tempdir, QUOTES_TXT );
    sprintf( szQuotesIndexFileName, "%s%s", syscfgovr.tempdir, QUOTES_IND );

    WFile::SetFilePermissions( szQuotesTextFileName, WFile::permReadWrite );
    WFile::Remove(szQuotesTextFileName);
    WFile::SetFilePermissions( szQuotesIndexFileName, WFile::permReadWrite );
    WFile::Remove(szQuotesIndexFileName);
    if (quotes_nrm)
    {
        BbsFreeMemory(quotes_nrm);
    }
    if (quotes_ind)
    {
        BbsFreeMemory(quotes_ind);
    }

    quotes_nrm = quotes_ind = NULL;
    quotes_nrm_l = quotes_ind_l = 0;

    if (m && aux)
    {
        pfx = GetQuoteInitials();
        strcat(pfx, "> ");
        pfxlen = strlen(pfx);

        ss = readfile(m, aux, &l);

        if (ss)
        {
            quotes_nrm = ss;
            quotes_nrm_l = l;

            fpFile = fsh_open(szQuotesTextFileName, "wb");
            if (fpFile)
            {
                fsh_write(ss, 1, l, fpFile);
                fsh_close(fpFile);
            }
            fpFile = fsh_open(szQuotesIndexFileName, "wb");
            if (fpFile)
            {
                l3 = l2 = 0;
                ss1 = NULL;
                sess->internetFullEmailAddress = "";
                if ((strnicmp("internet", sess->GetNetworkName(), 8) == 0) ||
                    (strnicmp("filenet", sess->GetNetworkName(), 7) == 0))
                {
                        for (l1 = 0; l1 < l; l1++)
                        {
                            if ((ss[l1] == 4) && (ss[l1 + 1] == '0') && (ss[l1 + 2] == 'R') &&
                                (ss[l1 + 3] == 'M'))
                            {
                                    l1 += 3;
                                    while ((ss[l1] != '\r') && (l1 < l))
                                    {
                                        temp[l3++] = ss[l1];
                                        l1++;
                                    }
                                    temp[l3] = 0;
                                    if (strnicmp(temp, "Message-ID", 10) == 0)
                                    {
                                        if (temp[0] != 0)
                                        {
                                            ss1 = strtok(temp, ":");
                                            if (ss1)
                                            {
                                                ss1 = strtok(NULL, "\r\n");
                                            }
                                            if (ss1)
                                            {
                                                sess->usenetReferencesLine = ss1;
                                            }
                                        }
                                    }
                                    l1 = l;
                                }
                        }
                    }
                    l3 = l2 = 0;
                    ss1 = NULL;
                    if ( sess->IsMessageThreadingEnabled() )
                    {
                        for (l1 = 0; l1 < l; l1++)
                        {
                            if ((ss[l1] == 4) && (ss[l1 + 1] == '0') && (ss[l1 + 2] == 'P'))
                            {
                                l1 += 4;
                                sess->threadID = "";
                                while ((ss[l1] != '\r') && (l1 < l))
                                {
                                    sprintf(temp, "%c", ss[l1]);
                                    sess->threadID += temp;
                                    l1++;
                                }
                                l1 = l;
                            }
                        }
                    }
                    for (l1 = 0; l1 < l; l1++)
                    {
                        if (ctld == -1)
                        {
                            ctld = ss[l1];
                        }
                        else switch (ss[l1])
						{
						case 1:
							ctla = 1;
							break;
						case 2:
							break;
						case 3:
							if (!ss1)
							{
								ss1 = ss + l1;
							}
							l2++;
							ctlc = 1;
							break;
						case 4:
							ctld = -1;
							break;
						case '\n':
							tf = 0;
							if (ctla)
							{
								ctla = 0;
							}
							else
							{
								cc = QUOTECOL + 48;
								FLSH;
								ctld = 0;
								NL;
							}
							break;
						case ' ':
						case '\r':
							if (ss1)
							{
								FLSH;
							}
							else
							{
								if (ss[l1] == ' ')
								{
									if (cp + 1 >= linelen)
									{
										NL;
									}
									if (!cp)
									{
										if (ctld)
										{
											fprintf(fpFile, "\x04%c", ctld);
										}
										WRTPFX;
									}
									cp++;
									fsh_write(" ", 1, 1, fpFile);
								}
							}
							break;
						default:
							if (!ss1)
							{
								ss1 = ss + l1;
							}
							l2++;
							if (ctlc)
							{
								if (ss[l1] == 48)
								{
									ss[l1] = QUOTECOL + 48;
								}
								cc = ss[l1];
								ctlc = 0;
							}
							else
							{
								l3++;
								if (!tf)
								{
									if (ss[l1]=='>')
									{
										tf=1;
										linelen=LINELEN;
									}
									else
									{
										tf=2;
										linelen=LINELEN-5;
									}
								}
							}
							break;
						}
                    }
                    FLSH;
                    if (cp)
					{
                        fsh_write("\r\n", 1, 2, fpFile);
					}
                    fsh_close(fpFile);
#ifdef SAVE_IN_MEM
					WFile ff( szQuotesIndexFileName );
					if ( ff.Open( WFile::modeBinary | WFile::modeReadOnly ) )
					{
						quotes_ind_l = ff.GetLength();
                        quotes_ind = static_cast<char*>( BbsAllocA( quotes_ind_l ) );
                        if (quotes_ind)
						{
							ff.Read( quotes_ind, quotes_ind_l );
                        }
						else
						{
                            quotes_ind_l = 0;
						}
						ff.Close();
                    }
#else
                    BbsFreeMemory(quotes_nrm);
                    quotes_nrm = NULL;
                    quotes_nrm_l = 0;
#endif
            }
        }
    }
}



void auto_quote(char *org, long len, int type, long daten)
{
	char s1[81], s2[81], buf[255],
		*p, *b,
		tb[81], b1[81],
		*tb1;


	p=b=org;
	WFile fileInputMsg( syscfgovr.tempdir, INPUT_MSG );
	fileInputMsg.Delete();
	if (!hangup)
    {
		fileInputMsg.Open( WFile::modeBinary|WFile::modeCreateFile|WFile::modeReadWrite, WFile::shareUnknown, WFile::permReadWrite );
		fileInputMsg.Seek( 0L, WFile::seekEnd );
		while (*p!='\r')
        {
			++p;
        }
		*p='\0';
		strcpy(s1,b);
		p+=2;
		len=len-(p-b);
		b=p;
		while (*p!='\r')
        {
			++p;
        }
		p+=2;
		len=len-(p-b);
		b=p;
		strcpy(s2, W_DateString(daten, "WDT", "at"));

		//    s2[strlen(s2)-1]='\0';
		strip_to_node(s1, tb);
		properize(tb);
		tb1 = GetQuoteInitials();
		switch (type)
        {
		case 1:
			sprintf(buf,"\003""3On \003""1%s, \003""2%s\003""3 wrote:\003""0",s2,tb);
			break;
		case 2:
			sprintf(buf,"\003""3In your e-mail of \003""2%s\003""3, you wrote:\003""0",s2);
			break;
		case 3:
			sprintf(buf,"\003""3In a message posted \003""2%s\003""3, you wrote:\003""0",s2);
			break;
		case 4:
			sprintf(buf,"\003""3Message forwarded from \003""2%s\003""3, sent on %s.\003""0",
				tb,s2);
			break;
		}
		strcat(buf,"\r\n");
		WriteBuf( fileInputMsg, buf );
		while (len>0)
        {
			while ((strchr("\r\001",*p)==NULL) && ((p-b)<(len<253 ? len : 253)))
            {
				++p;
            }
			if (*p=='\001')
            {
				*(p++)='\0';
            }
			*p='\0';
			if ( *b!='\004' && strchr( b,'\033' ) == NULL )
            {
				int jj=0;
				for (int j = 0; j < static_cast<int>(77-(strlen(tb1))); j++ )
                {
					if (((b[j]=='0') && (b[j-1]!='\003')) || (b[j]!='0'))
                    {
						b1[jj]=b[j];
                    }
					else
                    {
						b1[jj]='5';
                    }
					b1[jj+1]=0;
					jj++;
				}
				sprintf( buf, "\003""1%s\003""7>\003""5%s\003""0", tb1, b1 );
				WriteBuf( fileInputMsg, buf );
			}
			p += 2;
			len = len - ( p - b );
			b = p;
		}
        char ch = CZ;
		fileInputMsg.Write( &ch, 1 );
		fileInputMsg.Close();
		if ( sess->thisuser.GetNumMessagesPosted() < 10 )
		{
			printfile(QUOTE_NOEXT);
		}
		irt_name[0]='\0';
	}
	BbsFreeMemory( org );
}


void get_quote(int fsed)
{
    static char s[141], s1[10];
    static int i, i1, i2, i3, rl;
    static int l1, l2;

    sess->SetQuoting( ( fsed ) ? true : false );
    if ( quotes_ind == NULL )
    {
        if ( fsed )
        {
            sess->bout << "\x0c";
        }
        else
        {
            nl();
        }
        sess->bout << "Not replying to a message!  Nothing to quote!\r\n\n";
        if ( fsed )
        {
            pausescr();
        }
        sess->SetQuoting( false );
        return;
    }
    rl = 1;
    do
    {
        if (fsed)
        {
            sess->bout << "\x0c";
        }
        if (rl)
        {
            i = 1;
            l1 = l2 = 0;
            i1 = i2 = 0;
            bool abort = false;
            bool next = false;
            do
            {
                if (quotes_ind[l2++] == 10)
                {
                    l1++;
                }
            } while ((l2 < quotes_ind_l) && (l1 < 2));
            do
            {
                if (quotes_ind[l2] == 0x04)
                {
                    while ((quotes_ind[l2++] != 10) && (l2 < quotes_ind_l))
                    {
                    }
                }
                else
                {
                    if (irt_name[0])
                    {
                        s[0] = 32;
                        i3 = 1;
                    }
                    else
                    {
                        i3 = 0;
                    }
                    if (abort)
                    {
                        do {
                            l2++;
                        } while ((quotes_ind[l2] != RETURN) && (l2 < quotes_ind_l));
                    }
                    else
                    {
                        do
                        {
                            s[i3++] = quotes_ind[l2++];
                        } while ((quotes_ind[l2] != RETURN) && (l2 < quotes_ind_l));
                    }
                    if (quotes_ind[l2])
					{
                        l2 += 2;
                        s[i3] = 0;
                    }
                    sprintf(s1, "%3d", i++);
                    osan(s1, &abort, &next);
                    pla(s, &abort);
                }
            } while (l2 < quotes_ind_l);
            --i;
        }
        nl();

        if ( !i1 && !hangup )
        {
            do
            {
                sprintf(s,"Quote from line 1-%d? (?=relist, Q=quit) ",i);
				sess->bout << "|#2" << s;
                input(s, 3);
            } while ((!s[0]) && (!hangup));
            if (s[0] == 'Q')
            {
                rl = 0;
            }
            else if (s[0] != '?')
            {
                i1 = atoi(s);
                if (i1 >= i)
                {
                    i2 = i1 = i;
                }
                if (i1 < 1)
                {
                    i1 = 1;
                }
            }
        }

        if ( i1 && !i2 && !hangup )
        {
            do
            {
				sess->bout << "|#2through line " << i1 << "-" << i << "? (Q=quit) ";
                input(s, 3);
            } while ( !s[0] && !hangup );
            if (s[0] == 'Q')
            {
                rl = 0;
            }
            else if (s[0] != '?')
            {
                i2 = atoi(s);
                if (i2 > i)
                {
                    i2 = i;
                }
                if (i2 < i1)
                {
                    i2 = i1;
                }
            }
        }
        if ( i2 && rl && !hangup )
        {
            if (i1 == i2)
            {
                sess->bout << "|#5Quote line " << i1 << "? ";
            }
            else
            {
                sess->bout << "|#5Quote lines " << i1 << "-" << i2 << "? ";
            }
            if (!noyes())
            {
                i1 = 0;
                i2 = 0;
            }
        }
    } while ( !hangup && rl && !i2 );
    sess->SetQuoting( false );
    charbufferpointer = 0;
    if ( i1 > 0 && i2 >= i1 && i2 <= i && rl && !hangup )
    {
        bquote = i1;
        equote = i2;
    }
}
