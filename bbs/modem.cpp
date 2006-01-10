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

char cid_num[50];
char cid_name[50];

#define modem_time 3.5

bool InitializeComPort( int nComPortNumber );


void rputs(const char *pszText )
/* This function ouputs a string to the com port.  This is mainly used
 * for modem commands
 */
{
	// Rob fix for COM/IP weirdness
	if ( ok_modem_stuff )
    {
        app->comm->write( pszText, strlen( pszText ) );
    }

}


/**
 * Reads a string from the modem
 * @param pszLine String that was read from the modem
 * @param d How long to wait for the string
 * @param allowa allow aborting from the keyboard
 */
void get_modem_line(char *pszLine, double d, bool allowa)
{
#ifndef _UNIX
	int i = 0;
	char ch = 0, ch1;
	double t;

	t = timer();
	do
	{
		if (app->comm->incoming())
		{
			ch = bgetchraw();
		}
		else
		{
			ch = 0;
		}
		if (app->localIO->LocalKeyPressed() && allowa)
		{
			ch1 = app->localIO->getchd1();
			if (wwiv::UpperCase<char>(ch1) == 'H')
			{
				ch = RETURN;
				i = pszLine[0] = 1;

			}
		}
		else if (!ch)
		{
			giveup_timeslice();
		}
		if ( ch >= SPACE )
		{
			pszLine[i++] = wwiv::UpperCase<char>(ch);
		}
	} while ( ( ch != RETURN ) && (fabs(timer() - t) < d) && (i <= 40));
	pszLine[i] = '\0';
#else
	strcpy( pszLine, "OK" );
#endif
}


void do_result(result_info * ri)
{
	if (ri->description[0])
    {
		if (ri->flag_value & flag_append)
        {
            sess->SetCurrentSpeed( ri->description );
        }
		else
        {
            sess->SetCurrentSpeed( ri->description );
        }
    }

	if (ri->main_mode)
    {
		modem_mode = ri->main_mode;
    }

	// ignore the ringing mode, caller-id stuff
	if ((modem_mode == mode_ringing) ||
		(modem_mode == mode_cid_num) ||
		(modem_mode == mode_cid_name))
    {
		modem_mode = 0;
	}

    //  modem_flag = (modem_flag & ri->flag_mask) | ri->flag_value;
	//  if (modem_flag & flag_fc)
	//    flow_control = 1;

	if (ri->com_speed)
    {
		com_speed = ri->com_speed;
        if ( ok_modem_stuff && NULL != app->comm )
        {
            app->comm->setup( 'N', 8, 1, com_speed );
        }
	}
	if (ri->modem_speed)
    {
		modem_speed = ri->modem_speed;
    }
}


/**
 * Processes a result string from the model, and sets the result
 */
void process_full_result(char *pszResultCode)
{
	// first, check for caller-id info
	for (int i = 0; i < modem_i->num_resl; i++)
	{
		int i1 = strlen(modem_i->resl[i].result);
		if (strncmp(modem_i->resl[i].result, pszResultCode, i1) == 0)
		{
			switch (modem_i->resl[i].main_mode)
			{
			case mode_cid_num:
				strcpy(cid_num, pszResultCode + i1);
				return;
			case mode_cid_name:
				strcpy(cid_name, pszResultCode + i1);
				return;
			}
		}
	}

	char* ss = strtok(pszResultCode, modem_i->sepr);

	while (ss)
	{
		for (int i2 = 0; i2 < modem_i->num_resl; i2++)
		{
			if ( wwiv::stringUtils::IsEquals( modem_i->resl[i2].result, ss ) )
			{
				do_result(&(modem_i->resl[i2]));
				break;
			}
		}
		ss = strtok(NULL, modem_i->sepr);
	}
}


/**
 * Reads the result from the modem and switches the current mode based upon results.
 * @param d Timeout value
 * @param allowa Allow a local keypress to abort operation
 */
int mode_switch(double d, bool allowa)
{
	char s[81];
	bool abort = false;

	double t = timer();
	modem_mode = 0;

	if ( app->comm != NULL )
	{

	while ( modem_mode == 0 && fabs( timer() - t ) < d && !abort )
	{
		get_modem_line( s, d + t - timer(), allowa );
        std::cout << "DEBUG: get_modem_line(" << s << ")" << std::endl;
		if ( s[0] == '\x01' )
		{
			abort = true;
		}
		else if ( s[0] )
		{
			process_full_result( s );
		}
#ifdef _UNIX
		modem_mode = mode_dis;
#endif
	}
	if ( abort )
	{                   /* make sure modem hangs up */
		app->comm->dtr( false );
		while ((fabs(timer() - t) < d) && (!app->comm->incoming()))
		{
			wait1(18);
			rputch('\r');
		}
		dump();
	}

	}
	return modem_mode;
}

/** Picks up/Hangs up phone */
void holdphone(bool bPickUpPhone)
{
#ifndef _UNIX
    double xtime = 0;

    if (!ok_modem_stuff)
    {
        return;
    }
    if (no_hangup)
    {
        return;
    }
    if ((!(modem_i->pick)) || (!(modem_i->hang)))
    {
        return;
    }

    if (app->comm == NULL)
    {
	    return ;
    }

    app->comm->dtr( true );

    if (bPickUpPhone)
    {
        if (!global_xx)
        {
            if (syscfg.sysconfig & sysconfig_off_hook)
            {
                do_result(&(modem_i->defl));
                rputs(modem_i->pick);
                xtime = timer();
                global_xx = true;
            }
        }
    }
    else
    {
        if (syscfg.sysconfig & sysconfig_off_hook)
        {
            if (global_xx)
            {
                global_xx = false;
                app->comm->dtr( true );
                if (fabs(xtime - timer()) < modem_time)
                {
                    app->localIO->LocalPuts("\r\n\r\nWaiting for modem...");
                }
                while (fabs(xtime - timer()) < modem_time)
                    ;
                rputs(modem_i->hang);
                imodem( false );
            }
        }
    }
#endif
}


/**
 * Initializes or sets up the modem.
 * <P>
 * @param x Init or Setup.  0 = init, 1 = setup
 */
void imodem(bool bSetup)
{
#ifndef _UNIX
    std::string s;

	if (!ok_modem_stuff)
	{
		//app->localIO->LocalPuts("\x0c");
		return;
	}

	char *is;
	if (bSetup)
	{
		is = modem_i->setu;
	}
	else
	{
		is = modem_i->init;
	}

	if ( !( *is ) )
	{
		return;
	}

	app->localIO->LocalPuts( "Waiting..." );
	app->comm->dtr( true );
	do_result( &modem_i->defl );
	int i = 0;
	bool done = false;
	wait1( 9 );

	while ( !done )
	{
		if ( !InitializeComPort( syscfgovr.primaryport ) )
		{
			done = true;
			break;
		}
		dump();
		rputs( is );
		if ( mode_switch( 5.0, false ) == mode_norm )
		{
			done = true;
		}
		else
		{
			++i;
			switch ( modem_mode )
			{
			case 0:
				s = "(No Response)...";
				break;
			case mode_ring:
				s = "(Ring)...";
				break;
			case mode_dis:
				s = "(Disconnect)...";
				break;
			case mode_err:
				s = "(Error)...";
				break;
			default:
				s = "(%d)...", modem_mode;
				break;
			}
			app->localIO->LocalPuts( s.c_str() );
		}
		if ( i > 5 )
        {
			done = true;
        }
	}
	app->localIO->LocalCls();
#endif
}


void answer_phone()
{
#ifndef _UNIX
	if ( !ok_modem_stuff )
	{
		return;
	}
	cid_num[0]  = '\0';
	cid_name[0] = '\0';

	app->localIO->SetCursor( WLocalIO::cursorNormal );
	app->localIO->LocalXYPuts( 3, 24, "Answering phone, 'H' to abort." );
	sess->wfc_status = 0;
	do_result( &modem_i->defl );
#ifdef _DEBUG
    std::cout << "DEBUG: " << modem_i->ansr << std::endl;
#endif // _DEBUG
	rputs( modem_i->ansr );
	double d = timer();
	if ( ( mode_switch( 45.0, true ) != mode_con ) && ( modem_mode != mode_fax ) )
	{
		if ( modem_mode == 0 )
		{
			rputch( ' ' );
			rputch( '\r' );
			wait1( 18 );
			if ( fabs( timer() - d ) < modem_time )
			{
				while ( fabs( timer() - d ) < modem_time )
					;	// NOP
			}
			imodem( false );
			imodem( false );
		}
		else
		{
            app->localIO->LocalFastPuts( sess->GetCurrentSpeed().c_str() );
			imodem( false );
			imodem( false );
		}
	}
	else
	{
		incom = outcom = true;
		if (!(modem_flag & flag_ec))
		{
			wait1(72);
		}
		else
		{
			wait1(36);
		}
	}
#endif
}


bool InitializeComPort( int nComPortNumber )
{
#ifndef _UNIX
    if ( !ok_modem_stuff )
    {
        return false;
    }

    std::cout << "\nChecking status of COM Port 'COM" << nComPortNumber << "'... ";
    app->comm->SetComPort(nComPortNumber);

	// TODO check to see if it's opened 1st.
	app->comm->close();
    if ( !app->comm->open() )
    {
        std::cout << "\nUnable to Initialize Serial Port!" << std::endl <<
            "Continuing in local-only mode..." << std::endl;
        return false;
    }
    std::cout << "Port available!" << std::endl;

    app->comm->setup( 'N', 8, 1, com_speed );
    app->comm->dtr( true );
#endif
    return true;
}

