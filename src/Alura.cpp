/*******************************************************************************\
|* Alura.cpp - Alura Main Implementation File                                  *|
|* Alhexx' James Bond: Nightfire .007 Decompressor                             *|
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
#include "Alura.h"
#include <malloc.h>
#include "zlib.h"	// Needed for ZIP-compression

HANDLE hnd = NULL;
COORD CurPos1;		// Folders Done
COORD CurPos2;		// Files Done
COORD CurPos3;		// Megs written
int iNumFolders = 0;
int iNumFiles = 0;
int iNumFoldersDone = 0;
int iNumFilesDone = 0;
unsigned int uMegsWritten = 0;

int main(int argc, char* argv[])
{
	// Print Header
	PrintHeader();
	// Enought Arguments?
	if(argc != 2)
	{
		PrintUsage();
		return -1;
	}
	FILE* in = fopen(argv[argc-1], "rb");
	// Open Source File
	if(!in)
	{
		printf("ERROR: Failed to open '%s'!\n", argv[argc-1]);
		return -1;
	}
	printf("Decompressing '%s'...\n", argv[argc-1]);
	// Read "ID"
	int iDummy[2] = {0};
	fread(&iDummy[0], 8, 1, in);
	if((iDummy[0] != 1) || (iDummy[1] != 3))
	{
		printf("ERROR: Incorrect Archive ID!\n");
		fclose(in);
		return -1;
	}
	// Create node for ROOT-Directory
	t_dirnode	dirRoot;
	int			iCurrentFilePos = 0;
	// Read Root Directory
	if(ReadDirectory(in, &dirRoot, iCurrentFilePos))
	{
		// We ran out of memory
		printf("ERROR: Out of Memory!\n");
		DestroyDirectory(&dirRoot);
		fclose(in);
		return -1;
	}
	// Everything has gone well
	// Initialize Console Function vars
	hnd = GetStdHandle(STD_OUTPUT_HANDLE);
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	memset(&csbi, 0, sizeof(CONSOLE_SCREEN_BUFFER_INFO));
	printf("Directories  : ");
	GetConsoleScreenBufferInfo(hnd, &csbi);
	CurPos1 = csbi.dwCursorPosition;
	printf("    0 of %5i", iNumFolders);
	printf("\nFiles        : ");
	GetConsoleScreenBufferInfo(hnd, &csbi);
	CurPos2 = csbi.dwCursorPosition;
	printf("    0 of %5i", iNumFiles);
	printf("\nMegs written : ");
	GetConsoleScreenBufferInfo(hnd, &csbi);
	CurPos3 = csbi.dwCursorPosition;
	printf("    0.00");
	// Skip the last terminator
	fseek(in, 4, SEEK_CUR);
	// Now extract our Data
	ExtractDirectory(in, NULL, &dirRoot);
	printf("\n\nPosition: %u", ftell(in));
	// And clear all the buffer
	DestroyDirectory(&dirRoot);
	// And close our source file
	fclose(in);
	// Print Footer
	printf("\n\nFinished!\n\n");
	PrintFooter();
	// Exit
	return 0;
}

void RefreshConsole()
{	// Folders
	SetConsoleCursorPosition(hnd, CurPos1);
	printf("%5i of %5i", iNumFoldersDone, iNumFolders);
	SetConsoleCursorPosition(hnd, CurPos2);
	printf("%5i of %5i", iNumFilesDone, iNumFiles);
	SetConsoleCursorPosition(hnd, CurPos3);
	printf("%8.2f", double(uMegsWritten) / (1024 * 1024));
}

int ReadDirectory(FILE* stream, t_dirnode* pDirectory, /*t_dirnode* pParent,*/ int& iCurFilePos)
{
	iNumFolders++;
	// Free buffer
	memset(pDirectory, 0, sizeof(t_dirnode));
	// Read Dir Name Length
	fread(&pDirectory->iDirNameLength, 4, 1, stream);
	// Alloc Memory for Dir name
	pDirectory->szDirName = (char*)malloc(pDirectory->iDirNameLength + 1);
	if(!pDirectory->szDirName)
		return ERROR_OUTOFMEMORY;
	memset(pDirectory->szDirName, 0, pDirectory->iDirNameLength + 1);
	// Read Dir Name
	fread(pDirectory->szDirName, pDirectory->iDirNameLength, 1, stream);
	// Read Subdirs
	fread(&pDirectory->iNumSubDirs, 4, 1, stream);
	// Read File Name Buffer Length
	fread(&pDirectory->iFileNamesLength, 4, 1, stream);
	// Alloc Memory for File Names
	if(pDirectory->iFileNamesLength)
	{
		pDirectory->szFileNames = (char*)malloc(pDirectory->iFileNamesLength);
		if(!pDirectory->szFileNames)
			return ERROR_OUTOFMEMORY;
		memset(pDirectory->szFileNames, 0, pDirectory->iFileNamesLength);
	}
	// And now Read all files and subdirs
	char byCompressor = 0;		// Dummy for Compressor
	int iDummy = 0;				// Dummy for Length
	int iCurFile = 0;			// Current File Index
	int iCurNamePos = 0;		// Current Filename Position
	int	iNumSubDirsDone = 0;	// Number of processed Subdirs
	t_filenode* pFile = NULL;	// Pointer to File
	
	for(;;)
	{	/*  There is a problem when simply reading the length first.
			If you do so, then when you have fully processed a sub-dir,
			then the file pointer will be after the last sub-dirs'
			terminator. This will cause the function not to see this
			terminator and simply jump to the next file...
			B'cause of this, we have to check first, if all Sub-dirs and
			Files have been processed to avoid reading the last terminator
			of this directory, so the parent directory can read it.			*/

		// Check
		if((iNumSubDirsDone >= pDirectory->iNumSubDirs) &&
			(iCurNamePos >= pDirectory->iFileNamesLength))
			return 0;
		
		// Read Length
		fread(&iDummy, 4, 1, stream);
		// Is it a File?
		if(iDummy)
		{	// We've found a new File
			iNumFiles++;
			pDirectory->iNumFiles++;
			iCurFile++;
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
			pFile = GetFileFromDir(pDirectory, iCurFile-1);
			// Free our Buffer
			memset(pFile, 0, sizeof(t_filenode));
			// Set its Name Size
			pFile->iFileNameLength = iDummy;
			// Read the filename into our directory buffer
			fread(pDirectory->szFileNames + iCurNamePos, pFile->iFileNameLength, 1, stream);
			// Set our filename position
			pFile->iFileNamePosition = iCurNamePos;
			// Read the Compressor Flag (and ignore it)
			fread(&byCompressor, 1, 1, stream);
			// Read the File's Real Size
			fread(&pFile->iRealSize, 4, 1, stream);
			// Read the File's Compressed Size
			fread(&pFile->iCompressedSize, 4, 1, stream);
			// Set the File Position of our File
			//pFile->iFilePosition = iCurFilePos;
			// Update the Name Position for next File
			iCurNamePos += pFile->iFileNameLength + 1;
			// Update the File Add Position
			iCurFilePos += pFile->iCompressedSize;
		}
		else
		{	// We've (maybe) found a new Sub-Directory
			if(iNumSubDirsDone < pDirectory->iNumSubDirs)
			{	// Yeah, a new Subdir
				if(iNumSubDirsDone == 0)
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
				// Read the Sub-Directory with all its Files
				ReadDirectory(stream, GetSubDirFromDir(pDirectory, iNumSubDirsDone), iCurFilePos);
				// Increment Counter
				iNumSubDirsDone++;
			}
		}
	}
	// This *should* never happen
	return 0;
}

int DestroyDirectory(t_dirnode* pDirectory)
{
	// First, destroy the Directory Name Buffer
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

int ExtractDirectory(FILE* stream, char* szParentDir, t_dirnode* pDirectory)
{
	// Change current dir
	if(szParentDir)
		SetCurrentDirectory(szParentDir);

	// Create the directory
	if(!CreateDirectory(pDirectory->szDirName, NULL))
		return -1;	// Directory not created

	iNumFoldersDone++;
	RefreshConsole();

	// Now create all our files
	for(int i = 0; i < pDirectory->iNumFiles; i++)
	{
		// Get our file
		t_filenode* pFile = GetFileFromDir(pDirectory, i);
		// Generate our Filename
		char filebuffer[MAX_PATH] = {0};
		sprintf(filebuffer, "%s\\%s", pDirectory->szDirName, pDirectory->szFileNames + pFile->iFileNamePosition);
		// Create the file
		FILE* out = fopen(filebuffer, "wb");
		if(!out)
			return -1;
		// Create Buffer for output
		char* pBuffer = (char*)malloc(pFile->iRealSize);
		if(!pBuffer)
			return ERROR_OUTOFMEMORY;
		// Read it
		if(pFile->iCompressedSize == pFile->iRealSize)
			fread(pBuffer, pFile->iCompressedSize, 1, stream);
		else
		{	// Compressed
			char* pTemp = (char*)malloc(pFile->iCompressedSize);
			if(!pTemp)
			{
				free(pBuffer);
				return ERROR_OUTOFMEMORY;
			}
			// Read it
			fread(pTemp, pFile->iCompressedSize, 1, stream);
			// And decompress it
			if(uncompress(
				(unsigned char*)pBuffer,
				(unsigned long*)&pFile->iRealSize,
				(unsigned char*)pTemp,
				(unsigned long)pFile->iCompressedSize) != Z_OK)
			{	// Decompression failed
				free(pBuffer);
				free(pTemp);
				return -1;
			}
			free(pTemp);
		}
		// Decompression complete, write it
		fwrite(pBuffer, pFile->iRealSize, 1, out);
		// Close and free
		fclose(out);
		free(pBuffer);
		// Refresh Console
		uMegsWritten += pFile->iRealSize;
		iNumFilesDone++;
		RefreshConsole();
	}
	// And now extract all our Subdirs
	for(i = 0; i < pDirectory->iNumSubDirs; i++)
	{
		if(ExtractDirectory(stream, pDirectory->szDirName, GetSubDirFromDir(pDirectory, i)))
			return -1;
	}
	// Change to Parent Dir
	if(szParentDir)
		SetCurrentDirectory("..");
	// okay
	return 0;
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
	printf("Alura v1.03 - Alhexx' 007 Archive Decompressor\n\n");
}

void PrintUsage()
{
	printf("USAGE:\tAlura.exe filename\n\n");
	printf("\tfilename\tName of Archive to decompress (e.g. assets.007)\n\n");
}

void PrintFooter()
{
	printf("Alura is Copyright © 1994 - 2003 by Alhexx\n\n");
	printf("Visit or contact me:\n");
	printf("--------------------\n");
	printf("Home  : http://www.alhexx.com\n");
	printf("Forum : http://forums.alhexx.com\n");
	printf("Mail  : alhexx@alhexx.com\n\n");
}