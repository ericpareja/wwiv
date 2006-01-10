/****************************************************************************/
/*                                                                          */
/*                             WWIV Version 5.0x                            */
/*            Copyright (C) 1998-2003 by WWIV Software Services             */
/*                       Copyright (c) ????, ????????????                   */
/*                           Used with permission                           */
/*                                                                          */
/*      Distribution or publication of this source code, it's individual    */
/*       components, or a compiled version thereof, whether modified or     */
/*        unmodified, without PRIOR, WRITTEN APPROVAL of WWIV Software      */
/*        Services is expressly prohibited.  Distribution of compiled       */
/*            versions of WWIV is restricted to copies compiled by          */
/*           WWIV Software Services.  Violators will be procecuted!         */
/*                                                                          */
/****************************************************************************/
#include "port.h"

#include <conio.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "version.cpp"

#define VER 0x0101

char datadir[81];
static char *Strs;
static char **StrIdx;
int iNumStrs;

void print(int color, char *fmt,...)
{
  va_list ap;
  char s[512];

  va_start(ap, fmt);
  vsprintf(s, fmt, ap);
  va_end(ap);

#ifdef __BORLANDC__
  textcolor(YELLOW);
  cputs(" � ");
  textcolor(color);
  strcat(s, "\r\n");
  cputs(s);
#else
  fputs(" � ", stdout);
  puts(s);
  color = color;
#endif
}

void stripnl(char *instr)
{
  unsigned int i;

  if (strlen(instr) == 0)
    return;

  i = 0;
  do {
    if ((instr[i] == '\n') || (instr[i] == '\r')) {
      instr[i] = '\0';
    }
    i++;
  } while (i < strlen(instr));
}

void ReadChainText()
{
  FILE *fp;
  char szTemp[_MAX_PATH];
  int iX;

  sprintf(szTemp, "CHAIN.TXT");
  fp = fopen(szTemp, "rt");
  if (!fp) {
    print(LIGHTRED, "");
    print(LIGHTRED, "ERR: Unable to open chain.txt");
    exit(1);
  }
  for (iX = 0; iX <= 17; iX++) {
    fgets(datadir, 80, fp);
  }

  stripnl(datadir);
}

void ReadMenuSetup()
{

  if (Strs == NULL) {
    char szTemp[_MAX_PATH];
    FILE *fp;
    int iAmt, *index, iLen, iX;

    sprintf(szTemp, "%s%s", datadir, MENUCMDS_DAT);
    fp = fopen(szTemp, "rb");
    if (!fp) {
      print(LIGHTRED, "");
      print(LIGHTRED, "ERR: Unable to open menucmds.dat");
      exit(1);
    }
    fseek(fp, 0, SEEK_END);
    iLen = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    fread(&iAmt, 2, 1, fp);
    if (iAmt == 0) {
      print(LIGHTRED, "");
      print(LIGHTRED, "ERR: No menu strings found in menucmds.dat");
      exit(1);
    }
    iNumStrs = iAmt;

    index = (int *)bbsmalloc(sizeof(int) * iAmt);
    fread(index, 2, iAmt, fp);
    iLen -= ftell(fp);
    Strs = (char *)bbsmalloc(iLen);
    StrIdx = (char **)bbsmalloc(sizeof(char **) * iAmt);
    fread(Strs, iLen, 1, fp);
    fclose(fp);

    for (iX = 0; iX < iAmt; iX++)
      StrIdx[iX] = Strs + index[iX];

    free(index);
  }
}

void Usage()
{
  /* Long line split to avoid line wrap */
  print(LIGHTGREEN, "");
  print(LIGHTGREEN, "Purpose:");
  print(LIGHTGREEN, "");
  print(LIGHTCYAN, "   Program to give you the string number of a"
        " menu command in MENUCMDS.DAT");
  print(LIGHTCYAN, "Makes modding the Asylum Menus a lot easier."
        "  If you've ever added a new");
  print(LIGHTCYAN, "command to MENUCMDS.DAT (and MENU.C) you'll probably"
        " find this useful.");
  print(LIGHTGREEN, "");
  print(LIGHTGREEN, "Usage:");
  print(LIGHTGREEN, "");
  print(LIGHTCYAN, "   MenuTell.exe <MenuStringVar>");
  exit(1);
}

int main(int argc, char *argv[])
{
  int retval = 0;
  int strnum = -1;
  register int iX;

  print(LIGHTBLUE, "MenuTell v%d.%02d for %s", VER / 256, VER % 256,
        wwiv_version);
  ReadChainText();
  ReadMenuSetup();
  if (argc < 2) {
    Usage();
  }
  for (iX = 0; iX <= iNumStrs; iX++) {
    if (!strcmpi(StrIdx[iX], argv[1])) {
      strnum = iX;
    }
  }

  if (strnum < 0) {
    print(LIGHTRED, "");
    print(LIGHTRED, "Unable to find string %s in MENUCMDS.DAT", argv[1]);
    retval = 1;
  } else {
    print(LIGHTCYAN, "");
    print(LIGHTCYAN, "%s = StrIdx[%d]", StrIdx[strnum], strnum);
  }
  return (retval);
}
