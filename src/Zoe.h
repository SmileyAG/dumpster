/*******************************************************************************\
|* Zoe.h - Zoe Main Definition Header File                                     *|
|* Alhexx' James Bond: Nightfire .007 Compressor                             *|
|* Version 1.03                                                                *|
>*******************************************************************************<
|* Copyright � 1994 - 2003 by Alhexx                                           *|
>*******************************************************************************<
|* Visit or contact me:                                                        *|
|* --------------------                                                        *|
|* Home  : http://www.alhexx.com                                               *|
|* Forum : http://forums.alhexx.com                                            *|
|* or    : http://ffab.mypage.sk                                               *|
|* Mail  : alhexx@alhexx.com                                                   *|
\*******************************************************************************/
// Last update: 2006-05-12
#pragma once
#include "stdio.h"

struct t_dirnode
{
	int			iDirNameLength;		// Length of Directory Name
	int			iFileNamesLength;	// Length of File Names
	int			iNumSubDirs;		// Number of SubDirs
	int			iNumFiles;			// Number of Files (calculated)
	char*		szDirName;			// Pointer to Directory Name
	char*		szFileNames;		// Pointer to File Names
	void*		pSubDirs;			// Pointer to t_dirnode-array of Subdirs
	void*		pFiles;				// Pointer to t_filenode-array
};

struct t_filenode
{
	int		iFileNameLength;		// Length of Filename
	int		iFileNamePosition;		// Position in t_dirnode.szFileNames
	//char	cCompressorFlag;		// File compressed?
	int		iRealSize;				// Uncompressed Size
	int		iCompressedSize;		// Compressed Size
	//int		iFilePosition;			// Position of File in Archive (calculated)
};

int main(int argc, char* argv[]);
void PrintHeader();
void PrintUsage();
void PrintFooter();
t_filenode* GetFileFromDir(t_dirnode*, int);
t_dirnode* GetSubDirFromDir(t_dirnode*, int);
void DecodeArguments(int argc, char* argv[]);
int ReadFilterFile(char*);
void DestroyFilterFile();
int CompressFile(char*);
int ScanDirectory(char*, t_dirnode*);
char* strrstr(char*, char*);
int DestroyDirectory(t_dirnode*);
int WriteDirectory(FILE*, t_dirnode*);
int WriteFiles(FILE* stream, t_dirnode* pDirectory);
void RefreshConsole();
int WriteMD5Checksum(FILE* stream);
