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

#ifndef __INCLUDED_MENU_H__
#define __INCLUDED_MENU_H__

#ifdef _WIN32
  #pragma pack(push, 1)
#elif defined (_UNIX)
#pragma pack( 1 )
#endif


#define MENU
#define MENU_VERSION 0x0100

#define TEST_PADDING (5)

// 'iWhich' : Which messages to read in function ReadSelectedMessages
#define RM_ALL_MSGS   (-1)
#define RM_QSCAN_MSGS (-2)


// 'iWhere' : Which subs to read in function ReadSelectedMessages
#define RM_ALL_SUBS   (-1)
#define RM_QSCAN_SUBS (-2)



#define MENU_FLAG_DELETED   (0x01)
#define MENU_FLAG_MAINMENU  (0x02)


#define MENU_NUMFLAG_NOTHING   ( 0 )
#define MENU_NUMFLAG_SUBNUMBER ( 1 )
#define MENU_NUMFLAG_DIRNUMBER ( 2 )
#define MENU_NUMFLAG_LAST      ( 3 )

#define MENU_LOGTYPE_KEY       ( 0 )
#define MENU_LOGTYPE_NONE      ( 1 )
#define MENU_LOGTYPE_COMMAND   ( 2 )
#define MENU_LOGTYPE_DESC      ( 3 )
#define MENU_LOGTYPE_LAST      ( 4 )

#define MENU_HELP_DONTFORCE    ( 0 )
#define MENU_HELP_FORCE        ( 1 )
#define MENU_HELP_ONENTRANCE   ( 2 )
#define MENU_HELP_LAST         ( 3 )

#define MENU_HIDE_NONE         ( 0 )
#define MENU_HIDE_PULLDOWN     ( 1 )
#define MENU_HIDE_REGULAR      ( 2 )
#define MENU_HIDE_BOTH         ( 3 )
#define MENU_HIDE_LAST         ( 4 )

#define MENU_ALLOWED_BOTH      ( 0 )
#define MENU_ALLOWED_PULLDOWN  ( 1 )
#define MENU_ALLOWED_REGULAR   ( 2 )
#define MENU_ALLOWED_LAST      ( 3 )

#define PDFLAGS_NOCLEAR       (0x0001)
#define PDFLAGS_NORESTORE     (0x0002)
#define PDFLAGS_NOPAUSEAFTER  (0x0004)


#define MENU_MAX_KEYS (10)



struct MenuHeader
{
  char   szSig[10];      /* Menu Signature */
  INT16  nHeadBytes;     /* Size of Menu header */
  INT16  nBodyBytes;     /* Size of Menu Record */
  char   MISC[50];

  INT16  nVersion;
  INT16  nEmpty;
  INT8   nFlags;

  INT8   nNumbers;     /* What does a number do?  Set sub#, Dir#, nothing? */
  INT8   nLogging;     /* Types of logging, Key, None, command, desc       */

  INT8   nForceHelp;   /* force, dont force, on entrance only              */
  INT8   nAllowedMenu; /* Can pulldown, regular or both menus be used?     */

  UINT8  nTitleColor, nMainBorderColor, nMainBoxColor, nMainTextColor,
         nMainTextHLColor, nMainSelectedColor, nMainSelectedHLColor;

  UINT8  nItemBorderColor, nItemBoxColor, nItemTextColor, nItemTextHLColor,
         nItemSelectedColor, nItemSelectedHLColor;

  char   szMenuTitle[21];
  char   MISC2[60];
  char   szPassWord[21];     /* required for entry of menu */
  INT16  nMinSL, nMinDSL;    /* required for entry of menu */
  UINT16 uAR, uDAR;          /* required for entry of menu */
  UINT16 uRestrict;          /* not allowed restrictions   */
  INT8   nSysop, nCoSysop;   /* Must be either sysop or co */
  char   MISC3[30];
  char   szScript[101];      /* Gets executed on entry     */
  char   szExitScript[101];  /* Executed on rtn from menu  */
};



struct MenuRec
{
  INT8 nFlags;   /* AFLAG_????? */

  char szKey[MENU_MAX_KEYS+1];  /* Keystrock to execute menu item   */
  char szExecute[101];          /* Command to execute               */
  char szMenuText[41];          /* Menu description                 */
  char szPDText[41];            /* Pulldown menu text               */

  char szHelp[81];              /* Help for this item               */
  char szSysopLog[51];          /* Msg to put in the log            */

  char szInstanceMessage[81];

  /* Security */
  INT16 nMinSL,  iMaxSL;
  INT16 nMinDSL, iMaxDSL;
  UINT16 uAR, uDAR;        /* Must match all specified to be able to run     */
  UINT16 uRestrict;        /* If any of these restrictions, you cant execute */
  INT8 nSysop, nCoSysop;   /* true and false, does it take a co/sysop to run */
  char szPassWord[21];

  INT16 nHide;             /* Hide text from PD/Regular/both or no menus */
  INT16 nPDFlags;          /* special characteristis for pulldowns       */

  char szExtendedHelp[13]; /* filename to detailed help on this item */

  char unused_data[79];

};


struct MenuRecIndex
{
  char szKey[MENU_MAX_KEYS+1];
  INT16 nRec;				/* allows alot of records    */
  INT8	nFlags;             /* Quick access to the flags */
};


struct MenuInstanceData
{
  char szMenu[MAX_PATH];
  char szPath[MAX_PATH];
  WFile *pMenuFile;
  INT16 nAmountRecs;
  INT16 nFinished;

  INT16 nReload;  /* true if we are going to reload the menus */

  char *szPrompt;
  MenuRecIndex *index;
  MenuHeader header;   /* Hold header info for current menu set in memory */

};


#ifdef _WIN32
  #pragma pack(pop)
#endif


#endif  // __INCLUDED_MENU_H__
