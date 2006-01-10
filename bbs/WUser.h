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

#ifndef __INCLUDED_WUSER_H__
#define __INCLUDED_WUSER_H__

#ifdef _WIN32
#pragma pack( push, 1 )
#elif defined ( _UNIX )
#pragma pack( 1 )
#endif

#include "vardec.h"

/**
 * User Class - Represents a User record
 */
class WUser
{
public:
    //
    // Constants
    //

    // USERREC.inact
    static const int userDeleted                ;// = 0x01;
    static const int userInactive               ;// = 0x02;

    // USERREC.exempt
    static const int exemptRatio                ;// = 0x01;
    static const int exemptTime                 ;// = 0x02;
    static const int exemptPost                 ;// = 0x04;
    static const int exemptAll                  ;// = 0x08;
    static const int exemptAutoDelete           ;// = 0x10;

    // USERREC.restrict
    static const int restrictLogon              ;// = 0x0001;
    static const int restrictChat               ;// = 0x0002;
    static const int restrictValidate           ;// = 0x0004;
    static const int restrictAutomessage        ;// = 0x0008;
    static const int restrictAnony              ;// = 0x0010;
    static const int restrictPost               ;// = 0x0020;
    static const int restrictEmail              ;// = 0x0040;
    static const int restrictVote               ;// = 0x0080;
    static const int restrictMultiNodeChat      ;// = 0x0100;
    static const int restrictNet                ;// = 0x0200;
    static const int restrictUpload             ;// = 0x0400;

    // USERREC.sysstatus
    static const int ansi                       ;// = 0x00000001;
    static const int color                      ;// = 0x00000002;
    static const int music                      ;// = 0x00000004;
    static const int pauseOnPage                ;// = 0x00000008;
    static const int expert                     ;// = 0x00000010;
    static const int SMW                        ;// = 0x00000020;
    static const int fullScreen                 ;// = 0x00000040;
    static const int nscanFileSystem            ;// = 0x00000080;
    static const int extraColor                 ;// = 0x00000100;
    static const int clearScreen                ;// = 0x00000200;
    static const int upperASCII                 ;// = 0x00000400;
    static const int noTag                      ;// = 0x00000800;
    static const int conference                 ;// = 0x00001000;
    static const int noChat                     ;// = 0x00002000;
    static const int noMsgs                     ;// = 0x00004000;
    static const int menuSys                    ;// = 0x00008000; // not used?
    static const int listPlus                   ;// = 0x00010000;
    static const int autoQuote                  ;// = 0x00020000;
    static const int twentyFourHourClock        ;// = 0x00040000;
    static const int msgPriority                ;// = 0x00080000;  // not used?

//
// Data
//
public:
    struct userrec data;


public:
    //
    // Constructors and Destructors
    //
    WUser();
    ~WUser();

    WUser( const WUser& w );
    WUser& operator=( const WUser& rhs );

    //
    // Member Functions
    //
    void FixUp();    // was function fix_user_rec
    void ZeroUserData();

    //
    // Accessor Functions
    //
    const char *GetUserNameAndNumber( int nUserNumber ) const;
    const char *GetUserNameNumberAndSystem( int nUserNumber, int nSystemNumber ) const;

    // USERREC.inact
    void SetInactFlag( int nFlag )          { data.inact |= nFlag; }
    void ToggleInactFlag( int nFlag )       { data.inact ^= nFlag; }
    void ClearInactFlag( int nFlag )        { data.inact &= ~nFlag; }
    bool isUserDeleted() const              { return ( data.inact & WUser::userDeleted ) != 0; }
    bool isUserInactive() const             { return ( data.inact & WUser::userInactive ) != 0; }

    // USERREC.sysstatus
    void setStatusFlag( int nFlag )         { data.sysstatus |= nFlag; }
    void toggleStatusFlag( int nFlag )      { data.sysstatus ^= nFlag; }
    void clearStatusFlag( int nFlag )       { data.sysstatus &= ~nFlag; }
    bool hasStatusFlag( int nFlag ) const   { return ( data.sysstatus & nFlag ) != 0; }
    long GetStatus() const                  { return static_cast<long>( data.sysstatus ); }
    void SetStatus( long l )                { data.sysstatus = static_cast<unsigned long>( l ); }
    bool hasAnsi() const                    { return hasStatusFlag( WUser::ansi ); }
    bool hasColor() const                   { return hasStatusFlag( WUser::color ); }
    bool hasMusic() const                   { return hasStatusFlag( WUser::music ); }
    bool hasPause() const                   { return hasStatusFlag( WUser::pauseOnPage ); }
    bool isExpert() const                   { return hasStatusFlag( WUser::expert ); }
    bool hasShortMessage() const            { return hasStatusFlag( WUser::SMW ); }
    bool isFullScreen() const               { return hasStatusFlag( WUser::fullScreen ); }
    bool isNewScanFiles() const             { return hasStatusFlag( WUser::nscanFileSystem ); }
    bool isUseExtraColor() const            { return hasStatusFlag( WUser::extraColor ); }
    bool isUseClearScreen() const           { return hasStatusFlag( WUser::clearScreen ); }
    bool isUseNoTagging() const             { return hasStatusFlag( WUser::noTag ); }
    bool isUseConference() const            { return hasStatusFlag( WUser::conference ); }
    bool isIgnoreChatRequests() const       { return hasStatusFlag( WUser::noChat ); }
    bool isIgnoreNodeMessages() const       { return hasStatusFlag( WUser::noMsgs ); }
    bool isUseListPlus() const              { return hasStatusFlag( WUser::listPlus ); }
    bool isUseAutoQuote() const             { return hasStatusFlag( WUser::autoQuote ); }
    bool isUse24HourClock() const           { return hasStatusFlag( WUser::twentyFourHourClock ); };

    // USERREC.exempt
    void setExemptFlag( int nFlag )         { data.exempt |= nFlag; }
    void toggleExemptFlag( int nFlag )      { data.exempt ^= nFlag; }
    void clearExemptFlag( int nFlag )       { data.exempt &= ~nFlag; }
    bool hasExemptFlag( int nFlag ) const   { return ( data.exempt & nFlag ) != 0; }

    bool isExemptRatio()                    { return hasExemptFlag( WUser::exemptRatio ); }
    bool isExemptTime()                     { return hasExemptFlag( WUser::exemptTime ); }
    bool isExemptPost()                     { return hasExemptFlag( WUser::exemptPost ); }
    bool isExemptAll()                      { return hasExemptFlag( WUser::exemptAll ); }
    bool isExemptAutoDelete()               { return hasExemptFlag( WUser::exemptAutoDelete ); }

    // USERREC.restrict
    void setRestrictionFlag( int nFlag )        { data.restrict |= nFlag; }
    void toggleRestrictionFlag( int nFlag )     { data.restrict ^= nFlag; }
    void clearRestrictionFlag( int nFlag )      { data.restrict &= ~nFlag; }
    bool hasRestrictionFlag( int nFlag ) const  { return ( data.restrict & nFlag ) != 0; }
    unsigned short GetRestriction() const       { return data.restrict; }
    void SetRestriction( int n )                { data.restrict = static_cast<unsigned short>( n ); }

    bool isRestrictionLogon()                   { return hasRestrictionFlag( WUser::restrictLogon ); }
    bool isRestrictionChat()                    { return hasRestrictionFlag( WUser::restrictChat ); }
    bool isRestrictionValidate()                { return hasRestrictionFlag( WUser::restrictValidate ); }
    bool isRestrictionAutomessage()             { return hasRestrictionFlag( WUser::restrictAutomessage ); }
    bool isRestrictionAnonymous()               { return hasRestrictionFlag( WUser::restrictAnony ); }
    bool isRestrictionPost()                    { return hasRestrictionFlag( WUser::restrictPost ); }
    bool isRestrictionEmail()                   { return hasRestrictionFlag( WUser::restrictEmail ); }
    bool isRestrictionVote()                    { return hasRestrictionFlag( WUser::restrictVote ); }
    bool isRestrictionMultiNodeChat()           { return hasRestrictionFlag( WUser::restrictMultiNodeChat ); }
    bool isRestrictionNet()                     { return hasRestrictionFlag( WUser::restrictNet ); }
    bool isRestrictionUpload()                  { return hasRestrictionFlag( WUser::restrictUpload ); }

    void SetArFlag( int nFlag )                 { data.ar |= nFlag; }
    void toggleArFlag( int nFlag )              { data.ar ^= nFlag; }
    void clearArFlag( int nFlag )               { data.ar &= ~nFlag; }
    bool hasArFlag( int nFlag ) const           { return ( data.ar & nFlag ) != 0; }
    unsigned short GetAr() const                { return data.ar; }
    void SetAr( int n )                         { data.ar = static_cast<unsigned short>( n ); }

    void SetDarFlag( int nFlag )                 { data.dar |= nFlag; }
    void toggleDarFlag( int nFlag )              { data.dar ^= nFlag; }
    void clearDarFlag( int nFlag )               { data.dar &= ~nFlag; }
    bool hasDarFlag( int nFlag ) const           { return ( data.dar & nFlag ) != 0; }
    unsigned short GetDar() const                { return data.dar; }
    void SetDar( int n )                         { data.dar = static_cast<unsigned short>( n ); }

    const char *GetName() const             { return reinterpret_cast<const char*>( data.name ); }
    void SetName( const char *s )           { strcpy( reinterpret_cast<char*>( data.name ), s ); }
    const char *GetRealName() const         { return reinterpret_cast<const char*>( data.realname ); }
    void SetRealName( const char *s )       { strcpy( reinterpret_cast<char*>( data.realname ), s ); }
    const char *GetCallsign() const         { return reinterpret_cast<const char*>( data.callsign ); }
    void SetCallsign( const char *s )       { strcpy( reinterpret_cast<char*>( data.callsign ), s ); }
    const char *GetVoicePhoneNumber() const { return reinterpret_cast<const char*>( data.phone ); }
    void SetVoicePhoneNumber( const char *s )   { strcpy( reinterpret_cast<char*>( data.phone ), s ); }
    const char *GetDataPhoneNumber() const  { return reinterpret_cast<const char*>( data.dataphone ); }
    void SetDataPhoneNumber( const char *s )    { strcpy( reinterpret_cast<char*>( data.dataphone ), s ); }
    const char *GetStreet() const           { return reinterpret_cast<const char*>( data.street ); }
    void SetStreet( const char *s )         { strcpy( reinterpret_cast<char*>( data.street ), s ); }
    const char *GetCity() const             { return reinterpret_cast<const char*>( data.city ); }
    void SetCity( const char *s )           { strcpy( reinterpret_cast<char*>( data.city ), s ); }
    const char *GetState() const            { return reinterpret_cast<const char*>( data.state ); }
    void SetState( const char *s )          { strcpy( reinterpret_cast<char*>( data.state ), s ); }
    const char *GetCountry() const          { return reinterpret_cast<const char*>( data.country ); }
    void SetCountry( const char *s )        { strcpy( reinterpret_cast<char*>( data.country) , s ); }
    const char *GetZipcode() const          { return reinterpret_cast<const char*>( data.zipcode ); }
    void SetZipcode( const char *s )        { strcpy( reinterpret_cast<char*>( data.zipcode ), s ); }
    const char *GetPassword() const         { return reinterpret_cast<const char*>( data.pw ); }
    void SetPassword( const char *s )       { strcpy( reinterpret_cast<char*>( data.pw ), s ); }
    const char *GetLastOn() const           { return reinterpret_cast<const char*>( data.laston ); }
    void SetLastOn( const char *s )         { strcpy( reinterpret_cast<char*>( data.laston ), s ); }
    const char *GetFirstOn() const          { return reinterpret_cast<const char*>( data.firston ); }
    void SetFirstOn( const char *s )        { strcpy( reinterpret_cast<char*>( data.firston ), s ); }
    const char *GetNote() const             { return reinterpret_cast<const char*>( data.note ); }
    void SetNote( const char *s )           { strcpy( reinterpret_cast<char*>( data.note ), s ); }
    const char *GetMacro( int nLine ) const { return reinterpret_cast<const char*>( data.macros[ nLine ] ); }
    void SetMacro( int nLine, const char *s )   {  memset( &data.macros[ nLine ][0], 0, 80 ); strcpy( reinterpret_cast<char*>( data.macros[ nLine ] ), s ); }
    const char GetGender() const            { return static_cast<char>( data.sex ); }
    void SetGender( const char c )          { data.sex = static_cast<unsigned char>( c ); }
    const char *GetEmailAddress() const     { return data.email; }
    void SetEmailAddress( const char *s )   { strcpy( reinterpret_cast<char*>( data.email ), s ); }
    const int  GetAge() const               { return data.age; }
    void SetAge( int n )                    { data.age = static_cast<unsigned char>( n ); }
    const int  GetComputerType() const      { return data.comp_type; }
    void SetComputerType( int n )           { data.comp_type = static_cast<char>( n ); }

    const int GetDefaultProtocol() const    { return data.defprot; }
    void SetDefaultProtocol( int n )        { data.defprot = static_cast<unsigned char>( n ); }
    const int GetDefaultEditor() const      { return data.defed; }
    void SetDefaultEditor( int n )          { data.defed = static_cast<unsigned char>( n ); }
    const int GetScreenChars() const        { return data.screenchars; }
    void SetScreenChars( int n )            { data.screenchars = static_cast<unsigned char>( n ); }
    const int GetScreenLines() const        { return data.screenlines; }
    void SetScreenLines( int n )            { data.screenlines = static_cast<unsigned char>( n ); }
    const int GetNumExtended() const        { return data.num_extended; }
    void SetNumExtended( int n )            { data.num_extended = static_cast<unsigned char>( n ); }
    const int GetOptionalVal() const        { return data.optional_val; }
    void SetOptionalVal( int n )            { data.optional_val = static_cast<unsigned char>( n ); }
    const int GetSl() const                 { return data.sl; }
    void SetSl( int n )                     { data.sl = static_cast<unsigned char>( n ); }
    const int GetDsl() const                { return data.dsl; }
    void SetDsl( int n )                    { data.dsl = static_cast<unsigned char>( n ); }
    const int GetExempt() const             { return data.exempt; }
    void SetExempt( int n )                 { data.exempt = static_cast<unsigned char>( n ); }
    const unsigned char GetColor( int nColor ) const  { return data.colors[ nColor ]; }
    void SetColor( int nColor, int n )      { data.colors[ nColor ] = static_cast<unsigned char>( n ); }
    const unsigned char GetBWColor( int nColor ) const    { return data.bwcolors[ nColor ]; }
    void SetBWColor( int nColor, int n )        { data.bwcolors[ nColor ] = static_cast<unsigned char>( n ); }
    const int GetVote( int nVote ) const    { return data.votes[ nVote ]; }
    void SetVote( int nVote, int n )        { data.votes[ nVote ] = static_cast<unsigned char>( n ); }
    const int GetNumIllegalLogons() const   { return data.illegal; }
    void SetNumIllegalLogons( int n )       { data.illegal = static_cast<unsigned char>( n ); }
    const int GetNumMailWaiting() const     { return data.waiting; }
    void SetNumMailWaiting( int n )         { data.waiting = static_cast<unsigned char>( n ); }
    const int GetTimesOnToday() const       { return data.ontoday; }
    void SetTimesOnToday( int n )           { data.ontoday = static_cast<unsigned char>( n ); }
    const int GetBirthdayMonth() const      { return data.month; }
    void SetBirthdayMonth( int n )          { data.month = static_cast<unsigned char>( n ); }
    const int GetBirthdayDay() const        { return data.day; }
    void SetBirthdayDay( int n )            { data.day = static_cast<unsigned char>( n ); }
    const int GetBirthdayYear() const       { return data.year; }
    void SetBirthdayYear( int n )           { data.year = static_cast<unsigned char>( n ); }
    const int GetLanguage() const           { return data.language; }
    void SetLanguage( int n )               { data.language = static_cast<unsigned char>( n ); }
    const int GetCbv() const            { return data.cbv; }
    void SetCbv( int n )                { data.cbv = static_cast<unsigned char>( n ); }

    const int GetHomeUserNumber() const     { return data.homeuser; }
    void SetHomeUserNumber( int n )         { data.homeuser = static_cast<unsigned short>( n ); }
    const int GetHomeSystemNumber() const   { return data.homesys; }
    void SetHomeSystemNumber( int n )       { data.homesys = static_cast<unsigned short>( n ); }
    const int GetForwardUserNumber() const  { return data.forwardusr; }
    void SetForwardUserNumber( int n )      { data.forwardusr = static_cast<unsigned short>( n ); }
    const int GetForwardSystemNumber() const { return data.forwardsys; }
    void SetForwardSystemNumber( int n )    { data.forwardsys = static_cast<unsigned short>( n ); }
    const int GetForwardNetNumber() const   { return data.net_num; }
    void SetForwardNetNumber( int n )       { data.net_num = static_cast<unsigned short>( n ); }
    const int GetNumMessagesPosted() const  { return data.msgpost; }
    void SetNumMessagesPosted( int n )      { data.msgpost = static_cast<unsigned short>( n ); }
    const int GetNumEmailSent() const       { return data.emailsent; }
    void SetNumEmailSent( int n )           { data.emailsent = static_cast<unsigned short>( n ); }
    const int GetNumFeedbackSent() const    { return data.feedbacksent; }
    void SetNumFeedbackSent( int n )        { data.feedbacksent = static_cast<unsigned short>( n ); }
    const int GetNumFeedbackSentToday() const   { return data.fsenttoday1; }
    void SetNumFeedbackSentToday( int n )   { data.fsenttoday1 = static_cast<unsigned short>( n ); }
    const int GetNumPostsToday() const      { return data.posttoday; }
    void SetNumPostsToday( int n )          { data.posttoday = static_cast<unsigned short>( n ); }
    const int GetNumEmailSentToday() const  { return data.etoday; }
    void SetNumEmailSentToday( int n )      { data.etoday = static_cast<unsigned short>( n ); }
    const int GetAssPoints() const          { return data.ass_pts; }
    void SetAssPoints( int n )              { data.ass_pts = static_cast<unsigned short>( n ); }
    const int GetFilesUploaded() const      { return data.uploaded; }
    void SetFilesUploaded( int n )          { data.uploaded = static_cast<unsigned short>( n ); }
    const int GetFilesDownloaded() const    { return data.downloaded; }
    void SetFilesDownloaded( int n )        { data.downloaded = static_cast<unsigned short>( n ); }
    const int GetLastBaudRate() const       { return data.lastrate; }
    void SetLastBaudRate( int n )           { data.lastrate = static_cast<unsigned short>( n ); }
    const int GetNumLogons() const          { return data.logons; }
    void SetNumLogons( int n )              { data.logons = static_cast<unsigned short>( n ); }
    const int GetNumNetEmailSent() const    { return data.emailnet; }
    void SetNumNetEmailSent( int n )        { data.emailnet = static_cast<unsigned short>( n ); }
    const int GetNumNetPosts() const        { return data.postnet; }
    void SetNumNetPosts( int n )            { data.postnet = static_cast<unsigned short>( n ); }
    const int GetNumDeletedPosts() const    { return data.deletedposts; }
    void SetNumDeletedPosts( int n )        { data.deletedposts = static_cast<unsigned short>( n ); }
    const int GetNumChainsRun() const       { return data.chainsrun; }
    void SetNumChainsRun( int n )           { data.chainsrun = static_cast<unsigned short>( n ); }
    const int GetNumGFilesRead() const      { return data.gfilesread; }
    void SetNumGFilesRead( int n )          { data.gfilesread = static_cast<unsigned short>( n ); }
    const int GetTimeBankMinutes() const    { return data.banktime; }
    void SetTimeBankMinutes( int n )        { data.banktime = static_cast<unsigned short>( n ); }
    const int GetHomeNetNumber() const      { return data.homenet; }
    void SetHomeNetNumber( int n )          { data.homenet = static_cast<unsigned short>( n ); }
    const int GetLastSubConf() const        { return data.subconf; }
    void SetLastSubConf( int n )            { data.subconf = static_cast<unsigned short>( n ); }
    const int GetLastDirConf() const        { return data.dirconf; }
    void SetLastDirConf( int n )            { data.dirconf = static_cast<unsigned short>( n ); }
    const int GetLastSubNum() const         { return data.subnum; }
    void SetLastSubNum( int n )             { data.subnum = static_cast<unsigned short>( n ); }
    const int GetLastDirNum() const         { return data.dirnum; }
    void SetLastDirNum( int n )             { data.dirnum = static_cast<unsigned short>( n ); }

    const unsigned long GetNumMessagesRead() const  { return data.msgread; }
    void SetNumMessagesRead( unsigned long l )      { data.msgread = l; }
    const unsigned long GetUploadK() const          { return data.uk; }
    void SetUploadK( unsigned long l )              { data.uk = l; }
    const unsigned long GetDownloadK() const        { return data.dk; }
    void SetDownloadK( unsigned long l )            { data.dk = l; }
    const unsigned long GetLastOnDateNumber() const { return data.daten; }
    void SetLastOnDateNumber( unsigned long l )     { data.daten = l; }
    const unsigned long GetWWIVRegNumber() const    { return data.wwiv_regnum; }
    void SetWWIVRegNumber( unsigned long l )        { data.wwiv_regnum = l; }
    const unsigned long GetFilePoints() const       { return data.filepoints; }
    void SetFilePoints( unsigned long l )           { data.filepoints = l; }
    const unsigned long GetRegisteredDateNum() const{ return data.registered; }
    void SetRegisteredDateNum( unsigned long l )    { data.registered = l; }
    const unsigned long GetExpiresDateNum() const   { return data.expires; }
    void SetExpiresDateNum( unsigned long l )       { data.expires = l; }
    const unsigned long GetNewScanDateNumber() const{ return data.datenscan; }
    void SetNewScanDateNumber( unsigned long l )    { data.datenscan = l; }

    const float GetTimeOnToday() const      { return data.timeontoday; }
    void SetTimeOnToday( float f )          { data.timeontoday = f; }
    const float GetExtraTime() const        { return data.extratime; }
    void SetExtraTime( float f )            { data.extratime = f; }
    const float GetTimeOn() const           { return data.timeon; }
    void SetTimeOn( float f )               { data.timeon = f; }
    const float GetGold() const             { return data.gold; }
    void SetGold( float f )                 { data.gold = f; }

    bool GetFullFileDescriptions() const    { return data.full_desc ? true : false; }
    void SetFullFileDescriptions( bool b )  { data.full_desc = b ? 1 : 0; }

//
// Private Methods
//
private:
    char *nam( int nUserNumber ) const;
    char *nam1( int nUserNumber, int nSystemNumber ) const;

};


/**
 *
 */
class WUserManager
{
public:
    int GetNumberOfUsers() const;
    bool ReadUserNoCache( WUser *pUser, int nUserNumber );
    bool ReadUser( WUser *pUser, int nUserNumber, bool bForceRead = false );
    bool WriteUserNoCache( WUser *pUser, int nUserNumber );
    bool WriteUser( WUser *pUser, int nUserNumber );
};


#ifdef _WIN32
#pragma pack( pop )
#endif // _WIN32

#endif // __INCLUDED_PLATFORM_WUSER_H__
