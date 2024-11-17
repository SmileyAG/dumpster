/*******************************************************************************\
|* Zoe.cpp - Zoe Main Implementation File                                      *|
|* Alhexx' James Bond: Nightfire .007 Compressor                               *|
|* Version 1.03                                                                *|
>*******************************************************************************<
|* Copyright © 1994 - 2003 by Alhexx                                           *|
>*******************************************************************************<
|* Visit or contact me:                                                        *|
|* --------------------                                                        *|
|* Home  : http://www.alhexx.com                                               *|
|* Forum : http://forums.alhexx.com                                            *|
|* or    : http://ffab.mypage.sk                                               *|
|* Mail  : alhexx@alhexx.com                                                   *|
\*******************************************************************************/
// Last update: 2006-05-12
#include "Zoe.h"
#include <malloc.h>
#include "zlib.h"
#include "MD5.h"

#define MAX_FILTER  256

int iCompressorLevel = 9;
int iNumFilter = 0;
char* szFilter[MAX_FILTER] = {NULL};

HANDLE hnd = NULL;
COORD CurPos1;		// Folders Done
COORD CurPos2;		// Files Done
COORD CurPos3;		// Megs written
COORD CurPos4;		// Files written
int iNumFolders = 0;
int iNumFiles = 0;
int iNumFoldersDone = 0;
int iNumFilesDone = 0;
int iNumFilesWritten = 0;
unsigned int uMegsWritten = 0;
// MD5 Crypto-Vars
int iMD5Start = 8;		// always 8
int iMD5End = 0;		// to be written

int main(int argc, char* argv[])
{
	// Print Header
	PrintHeader();
	// Enough Arguments?
	if(argc < 2)
	{
		PrintUsage();
		return -1;
	}
	// Decode Arguments
	DecodeArguments(argc, argv);
	// Open Archive
	FILE* stream = fopen(argv[argc-1], "w+b");
	if(!stream)
	{
		printf("ERROR: Cannot open '%s'!\n\n", argv[argc-1]);
		DestroyFilterFile();
		return -1;
	}
	// Now Scan our Directory
	t_dirnode dirRoot;
	printf("Scanning Directories...\n");
	ScanDirectory("ROOT", &dirRoot);
	// Directory Scanned
	// Initialize Console Function vars
	hnd = GetStdHandle(STD_OUTPUT_HANDLE);
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	memset(&csbi, 0, sizeof(CONSOLE_SCREEN_BUFFER_INFO));
	printf("Directories     : ");
	GetConsoleScreenBufferInfo(hnd, &csbi);
	CurPos1 = csbi.dwCursorPosition;
	printf("    0 of %5i", iNumFolders);
	printf("\nFiles processed : ");
	GetConsoleScreenBufferInfo(hnd, &csbi);
	CurPos2 = csbi.dwCursorPosition;
	printf("    0 of %5i", iNumFiles);
	printf("\nFiles written   : ");
	GetConsoleScreenBufferInfo(hnd, &csbi);
	CurPos3 = csbi.dwCursorPosition;
	printf("    0 of %5i", iNumFiles);
	printf("\nMegs written    : ");
	GetConsoleScreenBufferInfo(hnd, &csbi);
	CurPos4 = csbi.dwCursorPosition;
	printf("    0.00");
	// Write Header
	int header[2] = {1, 3};
	fwrite(&header[0], 8, 1, stream);
	// Write our Dirs
	WriteDirectory(stream, &dirRoot);
	// Set our written Megs
	uMegsWritten = ftell(stream);
	iMD5End = (int)uMegsWritten;
	// And now the raw data
	WriteFiles(stream, &dirRoot);
	// Now write the MD5 Checksum
	printf("\n\nWriting Checksum...");
	WriteMD5Checksum(stream);
	// And close it
	fclose(stream);
	// Destroy our Directory
	DestroyDirectory(&dirRoot);
	// Destroy our filter list
	DestroyFilterFile();
	printf("\n\nFinished!\n\n");
	PrintFooter();
	return 0;
}

int WriteMD5Checksum(FILE* stream)
{
	if(!stream)
		return -1;
	if(iMD5End <= iMD5Start)
		return -1;
	// Create Buffer
	unsigned int iMD5Size = iMD5End - iMD5Start;
	unsigned char* Buffer = (unsigned char*)malloc(iMD5Size);
	if(!Buffer)
		return -1;
	// Read Buffer
	fseek(stream, iMD5Start, SEEK_SET);
	fread(Buffer, iMD5Size, 1, stream);
	// Generate Checksum
	MD5_CTX context;
	unsigned char CheckSum[16];
	MD5Init(&context);
	MD5Update(&context, Buffer, iMD5Size);
	MD5Final(CheckSum, &context);
	// Write the Checksum
	fseek(stream, 0, SEEK_END);
	int iLength = 16;
	fwrite(&iLength, 4, 1, stream);
	fwrite(CheckSum, 16, 1, stream);
	// Clear Buffer
	free(Buffer);
	// okay
	return 0;
}

int ScanDirectory(char* szDirectory, t_dirnode* pDirectory)
{
	iNumFolders++;
	// Free buffer
	memset(pDirectory, 0, sizeof(t_dirnode));
	pDirectory->iDirNameLength = strlen(szDirectory);
	pDirectory->szDirName = (char*)malloc(pDirectory->iDirNameLength + 1);
	if(!pDirectory->szDirName)
		return ERROR_OUTOFMEMORY;
	memset(pDirectory->szDirName, 0, pDirectory->iDirNameLength + 1);
	memcpy(pDirectory->szDirName, szDirectory, pDirectory->iDirNameLength);
	// Change current working Directory
	SetCurrentDirectory(szDirectory);
	WIN32_FIND_DATA FindData;
	memset(&FindData, 0, sizeof(WIN32_FIND_DATA));
	// Vars
	int iCurNamePos = 0;
	t_filenode* pFile = NULL;
	BOOL bFound = TRUE;
	// Find the first file/dir
	HANDLE hFind = FindFirstFile("*", &FindData);
	if(!hFind)
	{
		return -1;
	}
	// We've found a file/dir
	do
	{	// Do we have a file or a dir?
		if(!(FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
		{	// We've got a file
			iNumFiles++;
			pDirectory->iNumFiles++;
			if(pDirectory->iNumFiles == 1)
			{	// This is our first file here
				pDirectory->pFiles = malloc(sizeof(t_filenode));
				if(!pDirectory->pFiles)
					return ERROR_OUTOFMEMORY;
				iCurNamePos = 0;	// First Filename
			}
			else
			{	// Just another file out there
				pDirectory->pFiles = realloc(pDirectory->pFiles, 
					sizeof(t_filenode) * pDirectory->iNumFiles);
				if(!pDirectory->pFiles)
					return ERROR_OUTOFMEMORY;
			}
			// Get Pointer to our file
			pFile = GetFileFromDir(pDirectory, pDirectory->iNumFiles-1);
			// Free our Buffer
			memset(pFile, 0, sizeof(t_filenode));
			// Set its Name Size
			pFile->iFileNameLength = strlen(FindData.cFileName);
			// Set its Name Position
			pFile->iFileNamePosition = iCurNamePos;
			// Add our name to the list
			pDirectory->iFileNamesLength += pFile->iFileNameLength + 1;
			if(pDirectory->iNumFiles == 1)
			{	// Our First Name
				pDirectory->szFileNames = (char*)malloc(pDirectory->iFileNamesLength);
				if(!pDirectory->szFileNames)
					return ERROR_OUTOFMEMORY;
			}
			else
			{
				pDirectory->szFileNames = (char*)realloc(pDirectory->szFileNames,
					pDirectory->iFileNamesLength);
				if(!pDirectory->szFileNames)
					return ERROR_OUTOFMEMORY;
			}
			memcpy(pDirectory->szFileNames + iCurNamePos, FindData.cFileName,
				pFile->iFileNameLength + 1);
			// Refresh current Name Postition
			iCurNamePos += pFile->iFileNameLength + 1;
			// Set its Filesize
			pFile->iRealSize = FindData.nFileSizeLow;
			if(!iCompressorLevel || !CompressFile(FindData.cFileName))
				pFile->iCompressedSize = pFile->iRealSize;
		}
		else
		{	// Directory
			// is it a "." or ".." dir?
			if((  FindData.cFileName[0] == '.') && 
				((strlen(FindData.cFileName) == 1) ||
				((strlen(FindData.cFileName) == 2) &&
				 (FindData.cFileName[1] == '.'))))
			{
				bFound = FindNextFile(hFind, &FindData);
				continue;
			}
			// It is a real dir
			pDirectory->iNumSubDirs++;
			if(pDirectory->iNumSubDirs == 1)
			{	// This is our first file here
				pDirectory->pSubDirs = malloc(sizeof(t_dirnode));
				if(!pDirectory->pSubDirs)
					return ERROR_OUTOFMEMORY;
			}
			else
			{	// Just another file out there
				pDirectory->pSubDirs = realloc(pDirectory->pSubDirs, 
					sizeof(t_dirnode) * pDirectory->iNumSubDirs);
				if(!pDirectory->pSubDirs)
					return ERROR_OUTOFMEMORY;
			}
			ScanDirectory(FindData.cFileName, GetSubDirFromDir(pDirectory, 
				pDirectory->iNumSubDirs - 1));
		}
		bFound = FindNextFile(hFind, &FindData);
	} while(bFound);
	SetCurrentDirectory("..");
	// Close our FindHandle
	FindClose(hFind);
	return 0;
}

int DestroyDirectory(t_dirnode* pDirectory)
{	// First, destroy the Directory Name Buffer
	free(pDirectory->szDirName);
	// Then destroy the Filenames Buffer
	if(pDirectory->szFileNames)
		free(pDirectory->szFileNames);
	// And all File Nodes
	if(pDirectory->pFiles)
		free(pDirectory->pFiles);
	// Now loop through all Subdirectories and destroy them, too
	for(int i = 0; i < pDirectory->iNumSubDirs; i++)
		DestroyDirectory(GetSubDirFromDir(pDirectory, i));
	// Now destroy the Subdirectory nodes
	if(pDirectory->pSubDirs)
		free(pDirectory->pSubDirs);
	// Done
	return 0;
}

int WriteDirectory(FILE* stream, t_dirnode* pDirectory)
{
	// Change current dir
	SetCurrentDirectory(pDirectory->szDirName);
	// Write Directory Name Length
	fwrite(&pDirectory->iDirNameLength, 4, 1, stream);
	// Write Directory Name
	fwrite(pDirectory->szDirName, pDirectory->iDirNameLength, 1, stream);
	// Write Subdir Count
	fwrite(&pDirectory->iNumSubDirs, 4, 1, stream);
	// Write File Names Length
	fwrite(&pDirectory->iFileNamesLength, 4, 1, stream);
	// Okay, our dir is saved, now save its files
	for(int i = 0; i < pDirectory->iNumFiles; i++)
	{	// Get our file
		t_filenode* pFile = GetFileFromDir(pDirectory, i);
		// Write the file name length
		fwrite(&pFile->iFileNameLength, 4, 1, stream);
		// Write Filename
		fwrite(pDirectory->szFileNames + pFile->iFileNamePosition, pFile->iFileNameLength, 1, stream);
		char cFlag = 0;
		if(pFile->iCompressedSize != pFile->iRealSize)
		{	// Compress File
			cFlag = 1;
			char buffer[256] = {0};
			FILE* in  = fopen(pDirectory->szFileNames + pFile->iFileNamePosition, "rb");
			if(!in)
				return -1;
			sprintf(buffer, "%s.zoe", pDirectory->szFileNames + pFile->iFileNamePosition);
			FILE* out = fopen(buffer, "wb");
			if(!out)
			{
				fclose(in);
				return -1;
			}
			// Create Buffer for compressed Data
			pFile->iCompressedSize = int(pFile->iRealSize * 1.01) + 12;
			void* oribuffer = malloc(pFile->iRealSize);
			if(!oribuffer)
			{
				fclose(in);
				fclose(out);
				return ERROR_OUTOFMEMORY;
			}
			void* zipbuffer = malloc(pFile->iCompressedSize);
			if(!zipbuffer)
			{
				fclose(in);
				fclose(out);
				free(oribuffer);
				return ERROR_OUTOFMEMORY;
			}
			// Buffer created
			// Read Source Data and close source file
			fread(oribuffer, pFile->iRealSize, 1, in);
			fclose(in);
			// Compress Data
			if(compress2((unsigned char*)zipbuffer,
				(unsigned long*)&pFile->iCompressedSize,
				(unsigned char*)oribuffer,
				(unsigned long)pFile->iRealSize,
				iCompressorLevel) != Z_OK)
			{
				fclose(out);
				free(oribuffer);
				free(zipbuffer);
				return -1;
			}
			// Compressed, write data
			fwrite(zipbuffer, pFile->iCompressedSize, 1, out);
			fclose(out);
			// Free Buffer
			free(oribuffer);
			free(zipbuffer);
		}
		// Write Compressor Flag
		fwrite(&cFlag, 1, 1, stream);
		// Write Real Size
		fwrite(&pFile->iRealSize, 4, 1, stream);
		// Write Compressed Size
		fwrite(&pFile->iCompressedSize, 4, 1, stream);
		// File written
		iNumFilesDone++;
		uMegsWritten = ftell(stream);
		RefreshConsole();
	}
	// Write Terminator
	int iTerm = 0;
	fwrite(&iTerm, 4, 1, stream);
	// Now the subdirs
	for(i = 0; i < pDirectory->iNumSubDirs; i++)
	{
		if(WriteDirectory(stream, GetSubDirFromDir(pDirectory, i)))
			return -1;
	}
	// Now go back
	uMegsWritten = ftell(stream);
	iNumFoldersDone++;
	RefreshConsole();
	SetCurrentDirectory("..");
	// okay
	return 0;
}

void RefreshConsole()
{	// Folders
	SetConsoleCursorPosition(hnd, CurPos1);
	printf("%5i of %5i", iNumFoldersDone, iNumFolders);
	SetConsoleCursorPosition(hnd, CurPos2);
	printf("%5i of %5i", iNumFilesDone, iNumFiles);
	SetConsoleCursorPosition(hnd, CurPos3);
	printf("%5i of %5i", iNumFilesWritten, iNumFiles);
	SetConsoleCursorPosition(hnd, CurPos4);
	printf("%8.2f", double(uMegsWritten) / (1024 * 1024));
}

int WriteFiles(FILE* stream, t_dirnode* pDirectory)
{
	// Change current dir
	SetCurrentDirectory(pDirectory->szDirName);
	for(int i = 0; i < pDirectory->iNumFiles; i++)
	{
		t_filenode* pFile = GetFileFromDir(pDirectory, i);
		FILE* in = NULL;
		char zoebuffer[256] = {0};
		if(pFile->iCompressedSize != pFile->iRealSize)
		{
			sprintf(zoebuffer, "%s.zoe", pDirectory->szFileNames + pFile->iFileNamePosition);
			in = fopen(zoebuffer, "rb");
		}
		else
		{
			in = fopen(pDirectory->szFileNames + pFile->iFileNamePosition, "rb");
		}
		if(!in)
			return -1;
		// Create Buffer
		void* buffer = malloc(pFile->iCompressedSize);
		if(!buffer)
			return ERROR_OUTOFMEMORY;
		// Read Buffer
		fread(buffer, pFile->iCompressedSize, 1, in);
		// Write Buffer
		fwrite(buffer, pFile->iCompressedSize, 1, stream);
		// Close Source File
		fclose(in);
		// Free Buffer
		free(buffer);
		// Kill .zoe file
		if(pFile->iCompressedSize < pFile->iRealSize)
			DeleteFile(zoebuffer);
		// File written
		uMegsWritten += pFile->iCompressedSize;
		iNumFilesWritten++;
		RefreshConsole();
	}
	// And now the Subdirs
	for(i = 0; i < pDirectory->iNumSubDirs; i++)
	{
		if(WriteFiles(stream, GetSubDirFromDir(pDirectory, i)))
			return -1;
	}
	SetCurrentDirectory("..");
	// Okay
	return 0;	
}

void DecodeArguments(int argc, char* argv[])
{
	bool bFilterGiven = false;
	for(int i = 1; i < argc - 1; i++)
	{	// Compressor Level set
		if(strstr(argv[i], "-cl="))
		{
			int iTemp = 0;
			sscanf(argv[i], "-cl=%i", &iTemp);
			if((iTemp < 0) || (iTemp > 9))
			{
				printf("WARNING: %i is not a valid compression level!\nUsing level 6 instead.\n", iTemp);
				iCompressorLevel = 6;
			}
			else
				iCompressorLevel = iTemp;
		}
		// Compressor Filter File
		if(strstr(argv[i], "-cf="))
		{
			if(ReadFilterFile(argv[i] + 4))
				printf("Failed to read Filter File!\n");	// Something failed
			bFilterGiven = true;
		}
	}
	// No filter? Filter PNG then
	if(!bFilterGiven)
	{
		szFilter[0] = (char*)malloc(4);
		szFilter[0] = "png";
		iNumFilter = 1;
	}
}

int ReadFilterFile(char* szFileName)
{	// Open Filter File
	FILE* filter = fopen(szFileName, "rt");
	if(!filter)
	{
		printf("ERROR: Cannot open '%s'!", szFileName);
		return -1;
	}

	char buffer[256] = {0};
	int iSize = 0;
	// Read it
	while((iNumFilter < MAX_FILTER) && (!feof(filter)))
	{	// Free buffer
		memset(buffer, 0, 256);
		fscanf(filter, "%[^\n]%*c", buffer);	// BUG: Article ID: Q60336 
		// Is it a comment?
		if(strstr(buffer, "//"))
			continue;
		// Is it empty?
		if(!strlen(buffer))
			continue;
		// Try to save our new extension
		iSize = strlen(buffer) + 1;
		szFilter[iNumFilter] = (char*)malloc(iSize);
		if(!szFilter[iNumFilter])
		{	// An error occured
			printf("ERROR: Out of Memory!\n");
			fclose(filter);
			DestroyFilterFile();
			return -1;
		}
		// Store the extension
		memcpy(szFilter[iNumFilter], _strlwr(buffer), iSize);
		iNumFilter++;
	}
	// Close our file
	fclose(filter);
	return 0;
}

int CompressFile(char* szFileName)
{
	for(int i = 0; i < iNumFilter; i++)
	{
		if(strrstr(szFileName, szFilter[i]) == (char*)(szFileName + strlen(szFileName) -  strlen(szFilter[i])))
			return FALSE;
	}
	return TRUE;
}

char* strrstr(char* string, char* strCharSet)
{
	char sr[256] = {0};
	char cr[256] = {0};
	memcpy(sr, string, strlen(string));
	memcpy(cr, strCharSet, strlen(strCharSet));
	_strlwr(sr);
	_strlwr(cr);
	strrev(sr);
	strrev(cr);
	if(strstr(sr, cr))
	{

		return (char*)(string + int(strlen(string)) + (int(sr) - int(strstr(sr, cr))) - int(strlen(cr)));
	}
	else
		return NULL;
}


void DestroyFilterFile()
{	// Free our Buffer
	for(int i = 0; i < iNumFilter; i++)
		free(szFilter[i]);
	iNumFilter = 0;
}

t_filenode* GetFileFromDir(t_dirnode* pDirectory, int iFileIndex)
{
	if((iFileIndex >= pDirectory->iNumFiles) || (iFileIndex < 0))
		return NULL;

	return ((t_filenode*)pDirectory->pFiles + iFileIndex);
}

t_dirnode* GetSubDirFromDir(t_dirnode* pDirectory, int iDirIndex)
{
	if((iDirIndex >= pDirectory->iNumSubDirs) || (iDirIndex < 0))
		return NULL;

	return ((t_dirnode*)pDirectory->pSubDirs + iDirIndex);
}

void PrintHeader()
{
	printf("Zoe v1.06 - Alhexx' 007 Archive Compressor\n\n");
}

void PrintUsage()
{
	printf("USAGE:\tZoe.exe [-cl=*] [-cf=*] filename\n\n");
	printf("\t-cl=*\t\tCompression Level (* = 0 - 9) (default: 6)\n");
	printf("\t-cf=*\t\tCompression Filter File. Specifies a Files\n");
	printf("\t\t\twith all extensions, which should not be compressed.\n");
	printf("\tfilename\tName of Archive to compress (e.g. assets.007)\n\n");
}

void PrintFooter()
{
	printf("Zoe is Copyright © 1994 - 2003 by Alhexx\n\n");
	printf("Visit or contact me:\n");
	printf("--------------------\n");
	printf("Home  : http://www.alhexx.com\n");
	printf("Forum : http://forums.alhexx.com\n");
	printf("Mail  : alhexx@alhexx.com\n\n");
}