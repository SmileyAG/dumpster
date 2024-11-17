// prjconverter.cpp : Defines the entry point for the console application.
//

#include <stdio.h>
#include <windows.h>
#include "slnprocess.h"



int main(int argc, char* argv[])
{
	if (argc!=2)
	{
		printf ("prjconverter : Converts a VC++7 project to a VC++6 project\n" \
			    ".sln/.vcproj pairs are converted to .dsw/.dsp pairs\n\n" \
				"Usage: prjconverter.exe <solutionname (full filepath)>[.sln]\n" \
				"Example: prjconverter c:\\tmp\\betterxml.sln\n\n\n" \
				"Codeproject, Stephane Rodriguez, Sept 22 2002.\n");
		return 0;
	}


	// make sure the .sln (or .vcproj) file actually exists
	//

	CString szSolutionName = argv[1];
	if (!szSolutionName.Right(strlen(".vcproj")).CompareNoCase(".vcproj") &&
		!szSolutionName.Right(strlen(".sln")).CompareNoCase(".sln"))
		szSolutionName += ".sln";

	HANDLE hFind;
	WIN32_FIND_DATA fd;

	if ((hFind=::FindFirstFile(szSolutionName,&fd))==INVALID_HANDLE_VALUE)
	{
		printf ("%s does not exist.\n",szSolutionName);
		return 0;
	}

	if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
	{
		printf ("%s is a directory!\n",szSolutionName);
		return 0;
	}

	::FindClose(hFind);


	slnprocess p;

	// *.sln passed in the cmdline
	if ( szSolutionName.Right(strlen(".sln")).CompareNoCase(".sln") )
	{
		p.process(szSolutionName);
	}
	else if ( szSolutionName.Right(strlen(".vcproj")).CompareNoCase(".vcproj") ) // *.vcproj passed in the cmdline
	{
		p.processProjectOnly(szSolutionName); // szSolutionName is not a *.sln name
	}
	else
	{
		printf ("Only .sln and .vcproj filenames are processed.\n",szSolutionName);
		return 0;
	}

	return 0;
}


