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

#ifndef NOT_BBS
#include "wwiv.h"
#include "WStringUtils.h"
#endif


#ifndef MAX_TO_CACHE
#define MAX_TO_CACHE 15                     // max postrecs to hold in cache
#endif

static postrec *cache;                      // points to sub cache memory
static bool believe_cache;                  // true if cache is valid
static int cache_start;                     // starting msgnum of cache
static int last_msgnum;                     // last msgnum read
static WFile fileSub;						// WFile object for '.sub' file
static char subdat_fn[MAX_PATH];            // filename of .sub file


void close_sub()
{
	if ( fileSub.IsOpen() )
	{
		fileSub.Close();
	}
}

bool open_sub(bool wr)
{
    postrec p;

	close_sub();

    if (wr)
    {
#if (NOT_BBS == 2)
        fileSub = open_wc(subdat_fn);
#else
		fileSub.SetName( subdat_fn );
		fileSub.Open( WFile::modeBinary | WFile::modeCreateFile | WFile::modeReadWrite, WFile::shareUnknown, WFile::permReadWrite );
#endif
			if ( fileSub.IsOpen() )
        {
            // re-read info from file, to be safe
            believe_cache = false;
			fileSub.Seek( 0L, WFile::seekBegin );
			fileSub.Read( &p, sizeof( postrec ) );
            sess->SetNumMessagesInCurrentMessageArea( p.owneruser );
        }
    }
    else
    {
		fileSub.SetName( subdat_fn );
		fileSub.Open( WFile::modeReadOnly | WFile::modeBinary );
    }

	return fileSub.IsOpen();
}

bool iscan1(int si, bool quick)
// Initializes use of a sub value (subboards[], not usub[]).  If quick, then
// don't worry about anything detailed, just grab qscan info.
{
    postrec p;

    // make sure we have cache space
    if (!cache)
    {
        cache = static_cast<postrec *>( BbsAllocA(MAX_TO_CACHE * sizeof( postrec ) ) );
        if (!cache)
        {
            return false;
        }
    }
    // forget it if an invalid sub #
    if ( si < 0 || si >= sess->num_subs )
    {
        return false;
    }

    // skip this stuff if being called from the WFC cache code
    if (!quick)
    {
#ifndef NOT_BBS
        // go to correct net #
        if (xsubs[si].num_nets)
        {
            set_net_num(xsubs[si].nets[0].net_num);
        }
        else
        {
            set_net_num( 0 );
        }
#endif

        // see if a sub has changed
        app->statusMgr->Read();
        if ( sess->subchg )
        {
            sess->SetCurrentReadMessageArea( -1 );
        }

        // if already have this one set, nothing more to do
        if ( si == sess->GetCurrentReadMessageArea() )
        {
            return true;
        }
    }
    // sub cache no longer valid
    believe_cache = false;

    // set sub filename
    sprintf( subdat_fn, "%s%s.sub", syscfg.datadir, subboards[si].filename );

    // open file, and create it if necessary
    if (!WFile::Exists(subdat_fn))
    {
        if ( !open_sub( true ) )
        {
            return false;
        }
        p.owneruser = 0;
		fileSub.Write( &p, sizeof( postrec ) );
    }
    else if ( !open_sub( false ) )
    {
        return false;
    }

    // set sub
    sess->SetCurrentReadMessageArea( si );
    sess->subchg = 0;
    last_msgnum = 0;

    // read in first rec, specifying # posts
	fileSub.Seek( 0L, WFile::seekBegin );
	fileSub.Read( &p, sizeof( postrec ) );
    sess->SetNumMessagesInCurrentMessageArea( p.owneruser );

#ifndef NOT_BBS
    // read in sub date, if don't already know it
    if ( sess->m_SubDateCache[si] == 0 )
    {
        if ( sess->GetNumMessagesInCurrentMessageArea() )
        {
			fileSub.Seek( sess->GetNumMessagesInCurrentMessageArea() * sizeof( postrec ), WFile::seekBegin );
			fileSub.Read( &p, sizeof( postrec ) );
            sess->m_SubDateCache[si] = p.qscan;
        }
        else
        {
            sess->m_SubDateCache[si] = 1;
        }
    }
#endif

    // close file
    close_sub();

    // iscanned correctly
    return true;
}


#ifndef NOT_BBS

int iscan(int b)
// Initializes use of a sub (usub[] value, not subboards[] value).
//
{
  return iscan1(usub[b].subnum, false);
}

#endif


postrec *get_post( int mn )
// Returns info for a post.  Maintains a cache.  Does not correct anything
// if the sub has changed.
{
    postrec p;
    bool bCloseSubFile = false;

    if (mn == 0)
    {
        return NULL;
    }

    if (sess->subchg == 1)
    {
        // sub has changed (detected in app->statusMgr->Read); invalidate cache
        believe_cache = false;

        // kludge: subch==2 leaves subch indicating change, but the '2' value
        // indicates, to this routine, that it has been handled at this level
        sess->subchg = 2;
    }
    // see if we need new cache info
    if ( !believe_cache ||
        mn < cache_start ||
        mn >= ( cache_start + MAX_TO_CACHE ) )
    {
		if ( !fileSub.IsOpen() )
        {
            // open the sub data file, if necessary
            if ( !open_sub( false ) )
            {
                return NULL;
            }
            bCloseSubFile = true;
        }

        // re-read # msgs, if needed
        if ( sess->subchg == 2 )
        {
			fileSub.Seek( 0L, WFile::seekBegin );
			fileSub.Read( &p, sizeof( postrec ) );
            sess->SetNumMessagesInCurrentMessageArea( p.owneruser );

            // another kludge: subch==3 indicates we have re-read # msgs also
            // only used so we don't do this every time through
            sess->subchg = 3;

            // adjust msgnum, if it is no longer valid
            if ( mn > sess->GetNumMessagesInCurrentMessageArea() )
            {
                mn = sess->GetNumMessagesInCurrentMessageArea();
            }
        }
        // select new starting point of cache
        if ( mn >= last_msgnum )
        {
            // going forward
            if ( sess->GetNumMessagesInCurrentMessageArea() <= MAX_TO_CACHE)
            {
                cache_start = 1;
            }
            else if ( mn > ( sess->GetNumMessagesInCurrentMessageArea() - MAX_TO_CACHE  ) )
            {
                cache_start = sess->GetNumMessagesInCurrentMessageArea() - MAX_TO_CACHE + 1;
            }
            else
            {
                cache_start = mn;
            }
        }
        else
        {
            // going backward
            if ( mn > MAX_TO_CACHE )
            {
                cache_start = mn - MAX_TO_CACHE + 1;
            }
            else
            {
                cache_start = 1;
            }
        }

        if ( cache_start < 1 )
        {
            cache_start = 1;
        }

        // read in some sub info
		fileSub.Seek( cache_start * sizeof( postrec ), WFile::seekBegin );
		fileSub.Read( cache, MAX_TO_CACHE * sizeof( postrec ) );

        // now, close the file, if necessary
        if ( bCloseSubFile )
        {
            close_sub();
        }
        // cache is now valid
        believe_cache = true;
    }
    // error if msg # invalid
    if ( mn < 1 || mn > sess->GetNumMessagesInCurrentMessageArea() )
    {
        return NULL;
    }
    last_msgnum = mn;
    return ( cache + ( mn - cache_start ) );
}



void write_post(int mn, postrec * pp)
{
    postrec *p1;

	if ( fileSub.IsOpen() )
    {
		fileSub.Seek( mn * sizeof( postrec ), WFile::seekBegin );
		fileSub.Write( pp, sizeof( postrec ) );
        if ( believe_cache )
        {
            if ( mn >= cache_start && mn < ( cache_start + MAX_TO_CACHE ) )
            {
                p1 = cache + (mn - cache_start);
                if (p1 != pp)
                {
                    *p1 = *pp;
                }
            }
        }
    }
}

void add_post(postrec * pp)
{
    postrec p;
    bool bCloseSubFile = false;

    // open the sub, if necessary

	if ( !fileSub.IsOpen() )
    {
        open_sub( true );
        bCloseSubFile = true;
    }
	if ( fileSub.IsOpen() )
    {
        // get updated info
        app->statusMgr->Read();
		fileSub.Seek( 0L, WFile::seekBegin );
		fileSub.Read( &p, sizeof( postrec ) );

        // one more post
        p.owneruser++;
        sess->SetNumMessagesInCurrentMessageArea( p.owneruser );
		fileSub.Seek( 0L, WFile::seekBegin );
		fileSub.Write( &p, sizeof( postrec ) );

        // add the new post
		fileSub.Seek( sess->GetNumMessagesInCurrentMessageArea() * sizeof( postrec ), WFile::seekBegin );
		fileSub.Write( pp, sizeof( postrec ) );

        // we've modified the sub
        believe_cache = false;
        sess->subchg = 0;
#ifndef NOT_BBS
        sess->m_SubDateCache[sess->GetCurrentReadMessageArea()] = pp->qscan;
#endif
    }
    if ( bCloseSubFile )
    {
        close_sub();
    }
}

#ifndef NOT_BBS


#define BUFSIZE 32000

void delete_message(int mn)
{
    bool bCloseSubFile = false;

    // open file, if needed
	if ( !fileSub.IsOpen() )
    {
        open_sub( true );
        bCloseSubFile = true;
    }
    // see if anything changed
    app->statusMgr->Read();

	if ( fileSub.IsOpen() )
    {
        if ( mn > 0 && mn <= sess->GetNumMessagesInCurrentMessageArea() )
        {
            char *pBuffer = static_cast<char *>( BbsAllocA( BUFSIZE ) );
            if (pBuffer)
            {
                postrec *p1 = get_post( mn );
                remove_link( &( p1->msg ), subboards[sess->GetCurrentReadMessageArea()].filename );

                long cp = static_cast<long>( mn + 1 ) * sizeof( postrec );
                long len = static_cast<long>( sess->GetNumMessagesInCurrentMessageArea() + 1 ) * sizeof( postrec );

                unsigned int nb = 0;
                do
                {
                    long l = len - cp;
                    nb = (l < BUFSIZE) ? static_cast<int>( l ) : BUFSIZE;
                    if (nb)
                    {
						fileSub.Seek( cp, WFile::seekBegin );
						fileSub.Read( pBuffer, nb );
						fileSub.Seek( cp - sizeof( postrec ), WFile::seekBegin );
						fileSub.Write( pBuffer, nb );
                        cp += nb;
                    }
                } while ( nb == BUFSIZE );

                // update # msgs
                postrec p;
				fileSub.Seek( 0L, WFile::seekBegin );
				fileSub.Read( &p, sizeof( postrec ) );
                p.owneruser--;
                sess->SetNumMessagesInCurrentMessageArea( p.owneruser );
				fileSub.Seek( 0L, WFile::seekBegin );
				fileSub.Write( &p, sizeof( postrec ) );

                // cache is now invalid
                believe_cache = false;

                BbsFreeMemory( pBuffer );
            }
        }
    }
    // close file, if needed
    if ( bCloseSubFile )
    {
        close_sub();
    }
}

static bool IsSamePost(postrec * p1, postrec * p2)
{
	if ( p1 &&
		 p2 &&
		 p1->daten == p2->daten &&
		 p1->qscan == p2->qscan &&
		 p1->ownersys == p2->ownersys &&
		 p1->owneruser == p2->owneruser &&
		 wwiv::stringUtils::IsEquals( p1->title, p2->title ) )
	{
		return true;
	}
	return false;
}

void resynch(int subnum, int *msgnum, postrec * pp)
{
	postrec p, *pp1;

	// don't care about this param now
	int i = subnum;

	if (pp)
	{
		p = *pp;
	}
	else
	{
		pp1 = get_post(*msgnum);
		if (!pp1)
		{
			return;
		}
		p = *pp1;
	}

	app->statusMgr->Read();

	if (sess->subchg || pp)
	{
		pp1 = get_post(*msgnum);
		if (IsSamePost(pp1, &p))
		{
			return;
		}
		else if (!pp1 || (p.qscan < pp1->qscan))
		{
			if ( *msgnum > sess->GetNumMessagesInCurrentMessageArea() )
			{
				*msgnum = sess->GetNumMessagesInCurrentMessageArea() + 1;
			}
			for ( i = *msgnum - 1; i > 0; i-- )
			{
				pp1 = get_post( i );
				if ( IsSamePost( &p, pp1 ) || ( p.qscan >= pp1->qscan ) )
				{
					*msgnum = i;
					return;
				}
			}
			*msgnum = 0;
		}
		else
		{
			for ( i = *msgnum + 1; i <= sess->GetNumMessagesInCurrentMessageArea(); i++ )
			{
				pp1 = get_post( i );
				if ( IsSamePost( &p, pp1 ) || ( p.qscan <= pp1->qscan ) )
				{
					*msgnum = i;
					return;
				}
			}
			*msgnum = sess->GetNumMessagesInCurrentMessageArea();
		}
	}
}

#endif
