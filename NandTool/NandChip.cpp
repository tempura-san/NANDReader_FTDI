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

#include "NandChip.hpp"
#include "NandID.hpp"
#include "NandCmds.h"
#include "NandData.hpp"
#include "NandDataLP.hpp"
#include "NandDataSP.hpp"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include <cstdio>
#include <cstdlib>
using namespace std;

NandChip::NandChip(FtdiNand *fn)
{
	unsigned char id[MAXIDLEN];
	m_fn = fn;
	//Try to read the NAND ID-bytes and create the ID-object
	m_fn->sendCmd(NAND_CMD_RESET);
	usleep(10000);
	m_fn->sendCmd(NAND_CMD_READID);
	m_fn->sendAddr(0, 1);
	m_fn->readData((char *)id, MAXIDLEN);
	m_id = new NandID(id);
	//We use a different data object for large- and small-page devices
	if (m_id->isLargePage())
	{
		m_data = new NandDataLP(fn, m_id);
	}
	else
	{
		m_data = new NandDataSP(fn, m_id);
	}
}

NandChip::~NandChip()
{
	delete m_id;
}

void NandChip::showInfo()
{
	printf("NAND type:    %s\n", m_id->getDesc().c_str());
	printf("Manufacturer: %s\n", m_id->getManufacturer().c_str());
	printf("Device ID:    %02X\n", m_id->getID()[1]);
	printf("Chip info:    %02X\n", m_id->getID()[2]);
	printf("Chip size:    %iMB (pagesize: %i, OOB: %i)\n", m_id->getSizeMB(), m_id->getPageSize(), m_id->getOobSize());
	printf("%s page detected, needs %i address bytes.\n", m_id->isLargePage() ? "Large" : "Small", m_id->getAddrByteCount());
}

int NandChip::readPage(int page, char *buff, int count, NandChip::AccessType access)
{
	int r = 0;
	//Uses the data-object to read the main and/or OOB memory.
	if (access & NandChip::accessMain)
	{
		r = m_data->readPage(page, buff, count);
		count -= r;
	}
	if (access & NandChip::accessOob)
	{
		r = m_data->readOob(page, &buff[r], count);
		count -= r;
	}
	return r;
}

int NandChip::writePage(int page, char *buff, int count, NandChip::AccessType access)
{
	int r;
	//Can't read/writeback main or OOB data here because the erase size usually is bigger than the page size...
	if (access != NandChip::accessBoth)
	{
		printf("Writing of only main / OOB data isn't supported yet.\n");
		exit(0);
	}
	r = m_data->writePage(page, buff, count);
	return r;
}

NandID *NandChip::getIdPtr()
{
	return m_id;
}
