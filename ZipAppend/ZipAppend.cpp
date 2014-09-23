#ifdef WIN32
#include <SDKDDKVer.h>
#endif

#include <iostream>
#include <fstream>

static const unsigned int CDMagic = 0x02014b50;
static const unsigned int EOCDMagic = 0x06054b50;

struct ZipCD
{
	~ZipCD() { delete[] Data; }

	unsigned char *Data;
	unsigned int Offset, Length;
	unsigned short EntryCount;
};

bool LoadZipCD(std::ifstream &Source, ZipCD &OutCD)
{
	OutCD.Data=nullptr;

	Source.seekg(-22,std::ios_base::end);

	char ReadBuff[22];
	Source.read(ReadBuff,22);
	//TODO: handle non-zero length comments.
	if ((Source.fail()) || (*(const unsigned int *)ReadBuff!=EOCDMagic))
		return false;

	unsigned int EndPos=(unsigned int)Source.tellg();

	OutCD.EntryCount=*(const unsigned short *)(ReadBuff+10);
	OutCD.Offset=*(const unsigned int *)(ReadBuff+16);
	OutCD.Length=EndPos-OutCD.Offset;

	Source.seekg(OutCD.Offset,std::ios_base::beg);
	if (Source.fail())
		return false;

	OutCD.Data=new unsigned char[OutCD.Length];
	Source.read((char *)OutCD.Data,OutCD.Length);
	if (!Source.fail())
		return true;
	else
	{
		delete[] OutCD.Data;
		OutCD.Data=nullptr;
		return false;
	}
}

bool AddOffset(ZipCD &Target, int Offset)
{
	unsigned short CDCount=Target.EntryCount;
	unsigned char *ReadBuff=Target.Data, *ReadBuffEnd=ReadBuff+Target.Length;
	while ((CDCount) && (ReadBuff+46<=ReadBuffEnd))
	{
		if ((*(const unsigned int *)ReadBuff!=CDMagic))
			return false;

		unsigned int CurrOffset=*(const unsigned int *)(ReadBuff+42);
		*(unsigned int *)(ReadBuff+42)=CurrOffset+Offset;

		unsigned int FNLength=*(const unsigned short *)(ReadBuff+28);
		unsigned int ExtraLength=*(const unsigned short *)(ReadBuff+30);
		unsigned int CommentLength=*(const unsigned short *)(ReadBuff+32);

		ReadBuff+=46 + FNLength+ExtraLength+CommentLength;

		CDCount--;
	}

	if (!CDCount)
	{
		//Offset the CDOffset value.
		*(unsigned int *)(Target.Data+Target.Length-22+16)=*(unsigned int *)(Target.Data+Target.Length-22+16) + Offset;
		return true;
	}
	else
		return false;
}

int main(int argc, char* argv[])
{
	if (argc<3)
	{
		std::cout << "Appends a zip file to an arbitary file.\n"
			"Usage: " << argv[0] << " ZipFile TargetFile\n";
		return 1;
	}

	std::ifstream ZipS(argv[1],std::ios_base::binary);
	if (!ZipS)
	{
		std::cout << "Cannot open ZipFile.\n";
		return 2;
	}

	std::ofstream TargetS(argv[2],std::ios_base::binary | std::ios_base::app);
	if (!TargetS)
	{
		std::cout << "Cannot open TargetFile.\n";
		return 2;
	}

	ZipCD InCD;
	if (!LoadZipCD(ZipS,InCD))
	{
		std::cout << "ZipFile is invalid.\n";
		return 2;
	}

	TargetS.seekp(0,std::ios_base::end);
	unsigned long long TargetLength=TargetS.tellp();
	if (TargetLength>=1 << 31)
	{
		std::cout << "TargetFile is too large.\n";
		return 2;
	}

	if (!AddOffset(InCD,(int)TargetLength))
	{
		std::cout << "ZipFile is invalid.\n";
		return 2;
	}

	{
		ZipS.seekg(0,std::ios_base::beg);

		unsigned int ReadPos=0;
		unsigned int ReadBuffLength=1 << 16;
		char *ReadBuff=new char[ReadBuffLength];

		while (ReadPos+ReadBuffLength<=InCD.Offset)
		{
			ZipS.read(ReadBuff,ReadBuffLength);
			TargetS.write(ReadBuff,ReadBuffLength);
			ReadPos+=ReadBuffLength;
		}

		ZipS.read(ReadBuff,InCD.Offset-ReadPos);
		TargetS.write(ReadBuff,InCD.Offset-ReadPos);

		TargetS.write((char *)InCD.Data,InCD.Length);
	}

	return 0;
}
