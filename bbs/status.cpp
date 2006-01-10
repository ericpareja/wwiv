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


void StatusMgr::Get(bool bFailOnFailure, bool bLockFile)
{
    if ( !m_statusFile.IsOpen() )
	{
        m_statusFile.SetName( syscfg.datadir, STATUS_DAT );
        int nLockMode = (bLockFile) ? (WFile::modeReadWrite | WFile::modeBinary) : (WFile::modeReadOnly | WFile::modeBinary);
        m_statusFile.Open( nLockMode );
	}
	else
	{
        m_statusFile.Seek( 0L, WFile::seekBegin );
	}
    if ( !m_statusFile .IsOpen() )
	{
		if ( !bFailOnFailure )
		{
			sysoplog( "CANNOT READ STATUS" );
            std::cout << m_statusFile.GetName() << " NOT FOUND" << std::endl;
			app->AbortBBS();
		}
		else
		{
			sysoplog( "CANNOT READ STATUS" );
		}
	}
	else
	{
    	char fc[7];
		unsigned long lQScanPtr = status.qscanptr;
		for (int nFcIndex = 0; nFcIndex < 7; nFcIndex++)
		{
			fc[nFcIndex] = status.filechange[nFcIndex];
		}
        m_statusFile.Read( &status, sizeof( statusrec ) );
		if (!bLockFile)
		{
            m_statusFile.Close();
		}

		if ( lQScanPtr != status.qscanptr )
		{
			if (sess->m_SubDateCache)
			{
				// kill subs cache
				for (int i1 = 0; i1 < sess->num_subs; i1++)
				{
					sess->m_SubDateCache[i1] = 0L;
				}
			}
            sess->SetMessageAreaCacheNumber( 0 );
			sess->subchg = 1;
			g_szMessageGatFileName[0] = 0;
		}
		for (int i = 0; i < 7; i++)
		{
			if (fc[i] != status.filechange[i])
			{
				switch (i)
				{
				case filechange_names:            // re-read names.lst
					if (smallist)
					{
                        WFile namesFile( syscfg.datadir, NAMES_LST );
                        if( namesFile.Open( WFile::modeBinary | WFile::modeReadOnly ) )
						{
                            namesFile.Read( smallist, ( sizeof( smalrec ) * status.users ) );
                            namesFile.Close();
						}
					}
					break;
				case filechange_upload:           // kill dirs cache
                    {
					    if (sess->m_DirectoryDateCache)
					    {
						    for (int i1 = 0; i1 < sess->num_dirs; i1++)
						    {
							    sess->m_DirectoryDateCache[i1] = 0L;
						    }
					    }
					    sess->SetFileAreaCacheNumber( 0 );
                    }
					break;
				case filechange_posts:
					sess->subchg = 1;
					g_szMessageGatFileName[0] = 0;
					break;
				case filechange_email:
					emchg = true;
					mailcheck = false;
					break;
				case filechange_net:
                    {
					    int nOldNetNum = sess->GetNetworkNumber();
					    zap_bbs_list();
					    for ( int i1 = 0; i1 < sess->GetMaxNetworkNumber(); i1++ )
					    {
						    set_net_num( i1 );
						    zap_call_out_list();
						    zap_contacts();
					    }
					    set_net_num( nOldNetNum );
                    }
					break;
				}
			}
		}
	}
}


void StatusMgr::Lock()
{
	this->Get(true, true);
}


void StatusMgr::Read()
{
	this->Get(true, false);
}


void StatusMgr::Write()
{
    if ( !m_statusFile.IsOpen() )
	{
        m_statusFile.SetName( syscfg.datadir, STATUS_DAT );
        m_statusFile.Open( WFile::modeReadWrite | WFile::modeBinary );
	}
	else
	{
        m_statusFile.Seek( 0L, WFile::seekBegin );
	}

    if ( !m_statusFile.IsOpen() )
	{
		sysoplog("CANNOT SAVE STATUS");
	}
	else
	{
        m_statusFile.Write( &status, sizeof( statusrec ) );
        m_statusFile.Close();
	}
}


