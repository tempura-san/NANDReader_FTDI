#ifndef NANDID_HPP
#define NANDID_HPP

#include <string>
using namespace std;

const int MAXIDLEN = 8;

class NandID
{
public:
	NandID(unsigned char *idbytes);
	string getManufacturer();
	string getDesc();
	int getPageSize();
	int getSizeMB();
	int getOobSize();
	bool isLargePage();
	int getAddrByteCount();
	unsigned char *getID();

private:
	typedef struct
	{
		const char *name;
		unsigned char id;
		int pagesize;
		int chipsizeMB;
		int erasesize;
		int oobsize;
		int options;
		int addrcycles;
	} DevCodes;

	static const DevCodes m_devCodes[];
	unsigned char m_idBytes[MAXIDLEN];

	string m_nandManuf;
	string m_nandDesc;
	int m_nandPageSz;
	int m_nandbw;
	int m_nandOobSz;
	int m_nandEraseSz;
	int m_nandChipSzMB;
	int m_nandaddrcyc;
	bool m_nandIsLP;
};
#endif
