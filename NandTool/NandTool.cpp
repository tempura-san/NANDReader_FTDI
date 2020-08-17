/*
Software to interface to a NAND-flash chip connected to a FT2232H IC.
Org (C) 2012 Jeroen Domburg (jeroen AT spritesmods.com)
New code and modifications (c) 2014 Bjoern Kerler (info AT revskills.de)

This program is free software: you can redistribute it and/or modify
t under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "stdafx.h"
#include <stdio.h>
#include "stdlib.h"
#include "string.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#ifdef WIN32
#include <io.h>
#endif
#include "FtdiNand.hpp"
#include "NandChip.hpp"

//Windows needs O_BINARY to open binary files, but the flag is undefined
//on Unix. Hack around that. (Thanks, Shawn Hoffman)
#ifndef O_BINARY
#define O_BINARY 0
#endif

#ifdef WIN32
int _tmain(int argc, _TCHAR *argv[])
#else
int main(int argc, _TCHAR *argv[])
#endif
{
	int x, r;
	int vid = 0, pid = 0;
	bool err = false;
	bool doSlow = false;
	string file = "";
	enum Action
	{
		actionNone = 0,
		actionID,
		actionRead,
		actionWrite,
		actionVerify
	};

	printf("FT2232H-based NAND reader");

	//Parse command line options
	Action action = actionNone;
	NandChip::AccessType access = NandChip::accessBoth;
	for (x = 1; x < argc; x++)
	{
		if (strcmp(argv[x], "-i") == 0)
		{
			action = actionID;
		}
		else if (strcmp(argv[x], "-r") == 0 && x <= (argc - 2))
		{
			action = actionRead;
			file = argv[++x];
		}
		else if (strcmp(argv[x], "-w") == 0 && x <= (argc - 2))
		{
			action = actionWrite;
			file = argv[++x];
		}
		else if (strcmp(argv[x], "-v") == 0 && x <= (argc - 2))
		{
			action = actionVerify;
			file = argv[++x];
		}
		else if (strcmp(argv[x], "-u") == 0 && x <= (argc - 2))
		{
			action = actionVerify;
			char *endp;
			x++;
			vid = strtol(argv[x], &endp, 16);
			if (*endp != ':')
			{
				err = true;
			}
			else
			{
				endp++;
				pid = strtol(endp, NULL, 16);
			}
		}
		else if (strcmp(argv[x], "-t") == 0 && x <= (argc - 2))
		{
			x++;
			if (strcmp(argv[x], "main") == 0)
			{
				access = NandChip::accessMain;
			}
			else if (strcmp(argv[x], "oob") == 0)
			{
				access = NandChip::accessOob;
			}
			else if (strcmp(argv[x], "both") == 0)
			{
				access = NandChip::accessBoth;
			}
			else
			{
				printf("Region must be 'main' 'oob' or 'both': %s\n", argv[x]);
				err = true;
			}
		}
		else if (strcmp(argv[x], "-s") == 0)
		{
			doSlow = true;
		}
		else
		{
			printf("Unknown option or missing argument: %s\n", argv[x]);
			err = true;
		}
	}

	if (action == actionNone || err || argc == 1)
	{
		printf("Usage: [-i|-r file|-v file] [-t main|oob|both] [-s]\n");
		printf("  -i         - Identify chip\n");
		printf("  -r file    - Read chip content to file\n");
		printf("  -w file    - Write chip content from file\n");
		printf("  -v file    - Verify chip content from file\n");
		printf("  -t region  - Select region to read/write ('main' mem, 'oob' (spare) data or 'both', interleaved)\n");
		printf("  -s         - Clock FTDI chip at 12MHz instead of 60MHz\n");
		printf("  -u vid:pid - Use different FTDI USB vid/pid (in hex).\n");
		exit(0);
	}

	FtdiNand ftdi;
	ftdi.open(vid, pid, doSlow);
	NandChip nand(&ftdi);

	if (action == actionID)
	{
		nand.showInfo();
	}
	else if (action == actionRead || action == actionVerify)
	{
		int f;
		if (action == actionRead)
		{
			f = open(file.c_str(), O_WRONLY | O_CREAT | O_TRUNC | O_BINARY, 0644);
		}
		else
		{
			f = open(file.c_str(), O_RDONLY | O_BINARY);
		}
		if (f < 0)
		{
			perror(file.c_str());
			exit(1);
		}
		NandID *id = nand.getIdPtr();
		long pages = (id->getSizeMB() * 1024LL * 1024LL) / id->getPageSize();
		int size = 0;
		if (access == NandChip::accessMain)
			size = id->getPageSize();
		if (access == NandChip::accessOob)
			size = id->getOobSize();
		if (access == NandChip::accessBoth)
			size = id->getOobSize() + id->getPageSize();
		char *pageBuf = new char[size];
		char *verifyBuf = new char[size];
		int verifyErrors = 0;
		nand.showInfo();
		printf("%sing %li pages of %i bytes...\n", action == actionRead ? "Read" : "Verify", pages, id->getPageSize());
		for (x = 0; x < pages; x++)
		{
			nand.readPage(x, pageBuf, size, access);
			if (action == actionRead)
			{
				r = write(f, pageBuf, size);
				if (r != size)
				{
					perror("writing data to file");
					exit(1);
				}
			}
			else
			{
				r = read(f, verifyBuf, size);
				if (r != size)
				{
					perror("reading data from file");
					exit(1);
				}
				for (int y = 0; y < size; y++)
				{
					if (verifyBuf[y] != pageBuf[y])
					{
						verifyErrors++;
						printf("Verify error: Page %i, byte %i: file 0x%02hhX flash 0x%02hhX\n", x, y, verifyBuf[y], pageBuf[y]);
					}
				}
			}
			if ((x & 15) == 0)
			{
				printf("%i/%li\n\033[A", x, pages);
			}
		}
		if (action == actionVerify)
		{
			printf("Verify: %i bytes differ between NAND and file.\n", verifyErrors);
		}
	}

	printf("All done.\n");
	return 0;
}
