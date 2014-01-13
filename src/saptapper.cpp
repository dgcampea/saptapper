/*
** Simple utility to convert a GBA Rom to a GSF.
** Based on EXE2PSF code, written by Neill Corlett
** Released under the terms of the GNU General Public License
**
** You need zlib to compile this.
** It's available at http://www.gzip.org/zlib/
*/

#define APP_NAME	"Saptapper"
#define APP_VER		"[2014-01-13]"
#define APP_DESC	"Automated GSF ripper tool"
#define APP_AUTHOR	"Caitsith2, revised by loveemu <http://github.com/loveemu/saptapper>"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <string>
#include <sstream>
#include <map>

#include "zlib.h"

#include "saptapper.h"
#include "BytePattern.h"
#include "cbyteio.h"
#include "cpath.h"

#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#include <sys/stat.h>
#include <direct.h>
#else
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#define _chdir(s)	chdir((s))
#define _mkdir(s)	mkdir((s), 0777)
#define _rmdir(s)	rmdir((s))
#endif

// example:
// PUSH    {LR}
// LSLS    R0, R0, #0x10
// LDR     R2, =dword_827CCD8
// LDR     R1, =dword_827CD38 ; song table offset (sappyoffset)
// LSRS    R0, R0, #0xD
// ADDS    R0, R0, R1
// LDRH    R3, [R0,#4]
// LSLS    R1, R3, #1
// ADDS    R1, R1, R3
// LSLS    R1, R1, #2
// ADDS    R1, R1, R2
// LDR     R2, [R1]
// LDR     R1, [R0]
// MOVS    R0, R2
// BL      sub_80D9A64
// POP     {R0}
// BX      R0
const uint8_t Saptapper::selectsong[0x1E] = {
	0x00, 0xB5, 0x00, 0x04, 0x07, 0x4A, 0x08, 0x49, 
	0x40, 0x0B, 0x40, 0x18, 0x83, 0x88, 0x59, 0x00, 
	0xC9, 0x18, 0x89, 0x00, 0x89, 0x18, 0x0A, 0x68, 
	0x01, 0x68, 0x10, 0x1C, 0x00, 0xF0,
};

// example:
// PUSH    {R4-R6,LR}
// LDR     R0, =0x80D874D
// MOVS    R1, #2
const uint8_t Saptapper::init[2][INIT_COUNT] = { 
	{0x70, 0xB5},
	{0xF0, 0xB5}
};

// example:
// PUSH    {LR}
// BL      sub_80D86C8
// POP     {R0}
// BX      R0
const uint8_t Saptapper::soundmain[2] = {
	0x00, 0xB5,
};

// note that following pattern omits the first 5 bytes:
// LDR     R0, =dword_3007FF0
// LDR     R0, [R0]
// LDR     R2, =0x68736D53
// LDR     R3, [R0]
// SUBS    R3, R3, R2
const uint8_t Saptapper::vsync[5] = {
	0x4A, 0x03, 0x68, 0x9B, 0x1A,
};

const uint8_t Saptapper::SAPPYBLOCK[248] =
{
	0x00, 0x80, 0x2D, 0xE9, 0x01, 0x00, 0xBD, 0xE8, 0x50, 0x10, 0xA0, 0xE3, 0x00, 0x20, 0x90, 0xE5, 
	0x04, 0x00, 0x80, 0xE2, 0x04, 0x10, 0x41, 0xE2, 0x00, 0x00, 0x51, 0xE3, 0xFA, 0xFF, 0xFF, 0x1A, 
	0x0B, 0x00, 0x00, 0xEA, 0x53, 0x61, 0x70, 0x70, 0x79, 0x20, 0x44, 0x72, 0x69, 0x76, 0x65, 0x72, 
	0x20, 0x52, 0x69, 0x70, 0x70, 0x65, 0x72, 0x20, 0x62, 0x79, 0x20, 0x43, 0x61, 0x69, 0x74, 0x53, 
	0x69, 0x74, 0x68, 0x32, 0x5C, 0x5A, 0x6F, 0x6F, 0x70, 0x64, 0x2C, 0x20, 0x28, 0x63, 0x29, 0x20, 
	0x32, 0x30, 0x30, 0x34, 0x00, 0x40, 0x2D, 0xE9, 0x80, 0x00, 0x9F, 0xE5, 0x21, 0x00, 0x00, 0xEB, 
	0x88, 0x00, 0x9F, 0xE5, 0x00, 0x80, 0x2D, 0xE9, 0x02, 0x00, 0xBD, 0xE8, 0x30, 0x10, 0x81, 0xE2, 
	0x00, 0x10, 0x80, 0xE5, 0x01, 0x03, 0xA0, 0xE3, 0x08, 0x10, 0xA0, 0xE3, 0x04, 0x10, 0x80, 0xE5, 
	0x01, 0x10, 0xA0, 0xE3, 0x00, 0x12, 0x80, 0xE5, 0x08, 0x12, 0x80, 0xE5, 0x60, 0x00, 0x9F, 0xE5, 
	0x40, 0x10, 0x9F, 0xE5, 0x14, 0x00, 0x00, 0xEB, 0x00, 0x00, 0x02, 0xEF, 0xFD, 0xFF, 0xFF, 0xEA, 
	0x00, 0x40, 0x2D, 0xE9, 0x38, 0x00, 0x9F, 0xE5, 0x0E, 0x00, 0x00, 0xEB, 0x28, 0x00, 0x9F, 0xE5, 
	0x0C, 0x00, 0x00, 0xEB, 0x01, 0x03, 0xA0, 0xE3, 0x01, 0x18, 0xA0, 0xE3, 0x01, 0x10, 0x81, 0xE2, 
	0x00, 0x12, 0x80, 0xE5, 0x24, 0x00, 0x9F, 0xE5, 0x04, 0x10, 0x00, 0xE5, 0x01, 0x00, 0xBD, 0xE8, 
	0x10, 0xFF, 0x2F, 0xE1, 0xFF, 0xFF, 0xFF, 0xFF, 0x59, 0x81, 0x03, 0x08, 0x4D, 0x81, 0x03, 0x08, 
	0xD5, 0x80, 0x03, 0x08, 0x89, 0x7A, 0x03, 0x08, 0x10, 0xFF, 0x2F, 0xE1, 0x11, 0xFF, 0x2F, 0xE1, 
	0xFC, 0x7F, 0x00, 0x03, 0x30, 0x00, 0x00, 0x00, 
};

void Saptapper::put_gsf_exe_header(uint8_t *exe, uint32_t entrypoint, uint32_t load_offset, uint32_t rom_size)
{
	mput4l(entrypoint, &exe[0]);
	mput4l(load_offset, &exe[4]);
	mput4l(rom_size, &exe[8]);
}

bool Saptapper::exe2gsf(const std::string& gsf_path, uint8_t *exe, size_t exe_size)
{
	std::map<std::string, std::string> tags;
	return exe2gsf(gsf_path, exe, exe_size, tags);
}

#define CHUNK 16384
bool Saptapper::exe2gsf(const std::string& gsf_path, uint8_t *exe, size_t exe_size, std::map<std::string, std::string>& tags)
{
	FILE *gsf_file = NULL;

	z_stream z;
	uint8_t zbuf[CHUNK];
	uLong zcrc;
	uLong zlen;
	int zflush;
	int zret;

	// check exe size
	if (exe_size > MAX_GSF_EXE_SIZE)
	{
		return false;
	}

	// open output file
	gsf_file = fopen(gsf_path.c_str(), "wb");
	if (gsf_file == NULL)
	{
		return false;
	}

	// write PSF header
	// (EXE length and CRC will be set later)
	fwrite(PSF_SIGNATURE, strlen(PSF_SIGNATURE), 1, gsf_file);
	fputc(GSF_PSF_VERSION, gsf_file);
	fput4l(0, gsf_file);
	fput4l(0, gsf_file);
	fput4l(0, gsf_file);

	// init compression
	z.zalloc = Z_NULL;
	z.zfree = Z_NULL;
	z.opaque = Z_NULL;
	if (deflateInit(&z, Z_DEFAULT_COMPRESSION) != Z_OK)
	{
		return false;
	}

	// compress exe
	z.next_in = exe;
	z.avail_in = exe_size;
	z.next_out = zbuf;
	z.avail_out = CHUNK;
	zflush = Z_FINISH;
	zcrc = crc32(0L, Z_NULL, 0);
	do
	{
		// compress
		zret = deflate(&z, zflush);
		if (zret != Z_STREAM_END && zret != Z_OK)
		{
			deflateEnd(&z);
			fclose(gsf_file);
			return false;
		}

		// write compressed data
		zlen = CHUNK - z.avail_out;
		if (zlen != 0)
		{
			if (fwrite(zbuf, zlen, 1, gsf_file) != 1)
			{
				deflateEnd(&z);
				fclose(gsf_file);
				return false;
			}
			zcrc = crc32(zcrc, zbuf, zlen);
		}

		// give space for next chunk
		z.next_out = zbuf;
		z.avail_out = CHUNK;
	} while (zret != Z_STREAM_END);

	// set EXE info to PSF header
	fseek(gsf_file, 8, SEEK_SET);
	fput4l(z.total_out, gsf_file);
	fput4l(zcrc, gsf_file);
	fseek(gsf_file, 0, SEEK_END);

	// end compression
	deflateEnd(&z);

	// write tags
	if (!tags.empty())
	{
		fwrite(PSF_TAG_SIGNATURE, strlen(PSF_TAG_SIGNATURE), 1, gsf_file);

		for (std::map<std::string, std::string>::iterator it = tags.begin(); it != tags.end(); ++it)
		{
			const std::string& key = it->first;
			const std::string& value = it->second;
			std::istringstream value_reader(value);
			std::string line;

			// process for each lines
			while (std::getline(value_reader, line))
			{
				if (fprintf(gsf_file, "%s=%s\n", key.c_str(), line.c_str()) < 0)
				{
					fclose(gsf_file);
					return false;
				}
			}
		}
	}

	fclose(gsf_file);
	return true;
}

bool Saptapper::make_minigsf(const std::string& gsf_path, uint32_t offset, size_t size, uint32_t num, std::map<std::string, std::string>& tags)
{
	uint8_t exe[GSF_EXE_HEADER_SIZE + 4];

	// limit size
	if (size > 4)
	{
		return false;
	}

	// make exe
	put_gsf_exe_header(exe, 0x08000000, offset, size);
	mput4l(num, &exe[GSF_EXE_HEADER_SIZE]);

	// write minigsf file
	return exe2gsf(gsf_path, exe, GSF_EXE_HEADER_SIZE + size, tags);
}

int Saptapper::isduplicate(uint8_t *rom, uint32_t sappyoffset, int num)
{
	int i, j;
	unsigned char data[8];

	for (i = 0; i < 8; i++)
	{
		data[i] = rom[sappyoffset + (num * 8) + i];
	}

	if (num == 0)
	{
		return 0;
	}
	else
	{
		for (i = 0; i < num; i++)
		{
			for (j = 0; j < 8; j++)
			{
				if (data[j] != rom[sappyoffset + (i * 8) + j])
				{
					break;
				}
			}
			if (j == 8)
			{
				break;
			}
		}
		if (i != num)
		{
			return 1;
		}
	}
	return 0;
}

const char* Saptapper::get_gsflib_error(EGsfLibResult gsflibstat)
{
	switch (gsflibstat)
	{
	case GSFLIB_OK:
		return "Operation finished successfully";

	case GSFLIB_NOMAIN:
		return "sappy_main not found";

	case GSFLIB_NOSELECT:
		return "sappy_selectsongbynum not found";

	case GSFLIB_NOINIT:
		return "sappy_init not found";

	case GSFLIB_NOVSYNC:
		return "sappy_vsync not found";

	case GSFLIB_NOSPACE:
		return "Insufficient space found";

	case GSFLIB_ZLIB_ERR:
		return "GSFLIB zlib compression error";

	case GSFLIB_INFILE_E:
		return "File read error";

	case GSFLIB_OTFILE_E:
		return "File write error";

	default:
		return "Undefined error";
	}
}

Saptapper::EGsfLibResult Saptapper::dogsflib(const char *from, const char *to)
{
	FILE *f;
	uint32_t ucl;
	//uint32_t cl;
	//uint32_t ccrc;
	//int r;
	uint8_t *rom = uncompbuf+12;
	int i, j, k, rompointer;
	int result;
	//uLong zul;

	char s[1000];

	unsigned char sappyblock[248];

	for (i = 0; i < 1000; i++)
	{
		s[i] = 0;
	}

	memcpy(sappyblock, Saptapper::SAPPYBLOCK, sizeof(sappyblock));

	i = (int)(strchr(from, '.') - from); 
	strncpy(s, from, i);




	minigsfcount = 0;
	//fprintf(stderr, "%s: ", to);

	f = fopen(from,"rb");
	if (!f) {
		perror(from);
		return GSFLIB_INFILE_E;
	}
	ucl = (uint32_t)fread(uncompbuf + 12, 1, MAX_GBA_ROM_SIZE - 12, f);
	fclose(f);

	entrypoint = load_offset = 0x8000000;
	rom_size = ucl;
	mput4l(entrypoint, &uncompbuf[0]);
	mput4l(load_offset, &uncompbuf[4]);
	mput4l(rom_size, &uncompbuf[8]);

	char stroffset[256];

	int diff = 0;
	j = 0;
	k = 0;
selectcontinue:
	for (i = k; i < rom_size; i++)
	{
		if (rom[i] == selectsong[j])
		{
			for (j = 0; j < 0x1E; j++)
			{
				if(rom[i + j] != selectsong[j])
				{
					diff++;
					//j = 0;
					//break;
				}
			}

			//if (j != 0x1E)
			//{
			//	j=0;
			//}
			//else
			if (diff < 8)
			{
				break;
			}
			else
			{
				j = 0;
				diff = 0;
			}
		}
	}
	if (diff < 8)
	{
		sappyoffset = gba_address_to_offset(mget4l(&rom[i + 40]));
		unsigned long sappyoffset2;
		sappyoffset2 = sappyoffset;
		while ((rom[sappyoffset2 + 3] & 0x0E) == 0x08)
		{
			minigsfcount++;
			sappyoffset2 += 8;
		}
		if (!minigsfcount)
		{
			k = i + 1;
			goto selectcontinue;
		}
		mput3l(i + 1, &sappyblock[0xD8]);
	}
	else
	{
		// fprintf(stderr, "Unable to Locate sappy_selectsongbynum\n");
		return GSFLIB_NOSELECT;
	}
	// search prior function
	j = 0;
	rompointer = i - 0x20;
	for (i--; i > rompointer; i--)
	{
		if (rom[i] == soundmain[j])
		{
			for (j = 0; j < 0x2; j++)
			{
				if (rom[i + j] != soundmain[j])
				{
					j = 0;
					break;
				}
			}
			if(j != 0x2)
			{
				j = 0;
			}
			else
			{
				break;
			}
		}
	}
	if (i == rompointer)
	{
		//	  fprintf(stderr,"Unable to locate sappy_Soundmain\n");
		if (manual)
		{
			fprintf(stderr,"Enter a hex offset, or 0 to cancel - ");
			gets(stroffset);
			i = strtol(stroffset, NULL, 16);
			if (!i)
			{
				return GSFLIB_NOMAIN;
			}
			manual=2;
		}
		else
		{
			return GSFLIB_NOMAIN;
		}
	}
	mput3l(i + 1, &sappyblock[0xDC]);
	// search prior function, again
	j = 0;
	rompointer = i - 0x100;
	for (i--; i > rompointer; i--)
	{
		for (k = 0; k < INIT_COUNT; k++)
		{
			if (rom[i] == init[k][j])
			{
				result = 1;
				break;
			}
			else
			{
				result = 0;
			}
		}
		if (result)
		{
			for (k = 0; k < INIT_COUNT; k++)
			{
				for (j = 0; j < 0x2; j++)
				{
					if (rom[i + j] != init[k][j])
					{
						j = 0;
						break;
					}
				}
				if(j == 2)
				{
					break;
				}
			}
			if (j != 0x2)
			{
				j = 0;
			}
			else
			{
				break;
			}
		}
	}
	if (i == rompointer)
	{
		//  fprintf(stderr, "Unable to locate sappy_SoundInit\n");
		if (manual)
		{
			fprintf(stderr, "Enter a hex offset, or 0 to cancel - ");
			gets(stroffset);
			i = strtol(stroffset, NULL, 16);
			if(!i)
			{
				return GSFLIB_NOINIT;
			}
			manual = 2;
		}
		else
		{
			return GSFLIB_NOINIT;
		}
	}
	mput3l(i + 1, &sappyblock[0xE0]);
	j = 0;
	//i += 0x1800;
	// and so on...
	rompointer = i - 0x800;
	for (i--; (i > 0 && i > rompointer); i--)
	{
		if (rom[i] == vsync[j])
		{
			for (j = 0; j < 0x5; j++)
			{
				if (rom[i + j] != vsync[j])
				{
					j = 0;
					break;
				}
			}
			if (j != 0x5)
			{
				j = 0;
			}
			else
			{
				break;
			}
		}
	}

	if (i == rompointer || i == 0)
	{
		//	  fprintf(stderr,"Unable to locate sappy_VSync\n");
		if (manual)
		{
			fprintf(stderr, "Enter a hex offset, or 0 to cancel - ");
			gets(stroffset);
			i = strtol(stroffset, NULL, 16);
			if(!i)
			{
				return GSFLIB_NOVSYNC;
			}
			manual = 2;
		}
		else
		{
			return GSFLIB_NOVSYNC;
		}
	}
	else
	{
		i -= 5;
	}
	mput3l(i + 1, &sappyblock[0xE4]);

	rompointer = 0xFF;
lookforspace:
	for (i = 0; i < rom_size; i += 4)
	{
		if (rom[i] == (unsigned char)rompointer)
		{
			for (j = 0; j < 0x200; j++)
			{
				if (rom[i + j] != (unsigned char)rompointer)
				{
					j = 0;
					break;
				}
			}
			if (j != 0x200)
			{
				j = 0;
			}
			else
			{
				break;
			}
		}
	}
	if (j == 0x200)
	{
		minigsfoffset = 0x8000000 + i + 0x1FC;
		i += 0x108;
		memcpy(rom + i, sappyblock, sizeof(sappyblock));
		i >>= 2;
		i -= 2;
		mput3l(i, &rom[0]);
	}
	else
	{
		if (rompointer == 0xFF)
		{
			rompointer = 0x00;
			goto lookforspace;
		}
		else
		{
			//	  fprintf(stderr, "Unable to find sufficient unprogrammed space\n");
			return GSFLIB_NOSPACE;
		}
	}

	//  fprintf(stdout, "uncompressed: %ld bytes\n", ucl);
	//fflush(stdout);

	if (!exe2gsf(to, uncompbuf, GSF_EXE_HEADER_SIZE + ucl))
	{
		return GSFLIB_OTFILE_E;
	}

	//fprintf(stderr, "ok\n");

	return GSFLIB_OK;
}

bool Saptapper::make_gsf_set(const std::string& rom_path)
{
	bool result;
	EGsfLibResult gsflibstat = GSFLIB_OK;
	std::map<std::string, std::string> tags;

	// get separator positions
	const char *c_rom_path = rom_path.c_str();
	const char *c_rom_base = path_findbase(c_rom_path);
	const char *c_rom_ext = path_findext(c_rom_path);
	// extract directory path and filename
	std::string rom_dir = rom_path.substr(0, c_rom_base - c_rom_path);
	std::string rom_basename = rom_path.substr(c_rom_base - c_rom_path,
		(c_rom_ext != NULL) ? (c_rom_ext - c_rom_base) : std::string::npos);
	// construct some new paths
	std::string gsf_dir = rom_dir + rom_basename;
	std::string gsflib_name = rom_basename + ".gsflib";
	std::string gsflib_path = gsf_dir + PATH_SEPARATOR_STR + gsflib_name;

	// create output directory
	_mkdir(gsf_dir.c_str());
	if (!path_isdir(gsf_dir.c_str()))
	{
		fprintf(stderr, "Error: %s - Directory could not be created\n", gsf_dir.c_str());
		return false;
	}

	// create gsflib
	gsflibstat = dogsflib(rom_path.c_str(), gsflib_path.c_str());
	if (gsflibstat != GSFLIB_OK)
	{
		fprintf(stderr, "Error: %s - %s\n", rom_path.c_str(), get_gsflib_error(gsflibstat));
		_rmdir(gsf_dir.c_str());
		return false;
	}

	// create minigsfs
	int minigsferrors = 0;
	int size = (minigsfcount > 255) ? 2 : 1;
	tags["_lib"] = gsflib_name;
	if (manual == 2)
	{
		char nickname[64];
		printf("Enter your name for GSFby purposes.\n");
		printf("The GSFBy tag will look like,\n");
		printf("Saptapper, with help of <your name here>\n");
		gets(nickname);
		if(!strcmp(nickname, "CaitSith2"))
		{
			tags["gsfby"] = "Caitsith2";
		}
		else
		{
			tags["gsfby"]= std::string("Saptapper, with help of ") + nickname;
		}
		manual = 1;
	}
	else
	{
		tags["gsfby"] = "Saptapper";
	}
	result = false;
	for (int minigsfindex = 0; minigsfindex < minigsfcount; minigsfindex++)
	{
		char minigsfname[PATH_MAX];

		sprintf(minigsfname, "%s.%.4X.minigsf", rom_basename.c_str(), minigsfindex);
		std::string minigsf_path = gsf_dir + PATH_SEPARATOR_STR + minigsfname;

		if (isduplicate(&uncompbuf[12], sappyoffset, minigsfindex) == 0)
		{
			if (make_minigsf(minigsf_path, minigsfoffset, size, minigsfindex, tags))
			{
				result = true;
			}
			else
			{
				minigsferrors++;
			}
		}
		//else
		//{
		//	fprintf(stderr, "|");
		//}
	}
	//fprintf(stderr, "\n");

	if (minigsferrors > 0)
	{
		fprintf(stderr, "%d error(s)\n", minigsferrors);
	}
	return result;
}

void printUsage(const char *cmd)
{
	const char *availableOptions[] = {
		"--help", "Show this help",
	};

	printf("%s %s\n", APP_NAME, APP_VER);
	printf("======================\n");
	printf("\n");
	printf("%s. Created by %s.\n", APP_DESC, APP_AUTHOR);
	printf("\n");
	printf("Usage\n");
	printf("-----\n");
	printf("\n");
	printf("Syntax: %s <GBA Files>\n", cmd);
	printf("\n");
	printf("### Options ###\n");
	printf("\n");

	for (int i = 0; i < sizeof(availableOptions) / sizeof(availableOptions[0]); i += 2)
	{
		printf("%s\n", availableOptions[i]);
		printf("  : %s\n", availableOptions[i + 1]);
		printf("\n");
	}
}

int main(int argc, char **argv)
{
	Saptapper app;
	bool result;
	int argnum;
	int argi;

	argi = 1;
	while (argi < argc && argv[argi][0] == '-')
	{
		if (strcmp(argv[argi], "--help") == 0)
		{
			printUsage(argv[0]);
			return EXIT_SUCCESS;
		}
		else
		{
			fprintf(stderr, "Error: Unknown option \"%s\"\n", argv[argi]);
			return EXIT_FAILURE;
		}
		argi++;
	}

	argnum = argc - argi;
	if (argnum == 0)
	{
		fprintf(stderr, "Error: No input files.\n");
		fprintf(stderr, "\n");
		fprintf(stderr, "Run \"%s --help\" for help.\n", argv[0]);
		return EXIT_FAILURE;
	}

	result = true;
	for (; argi < argc; argi++)
	{
		result = result && app.make_gsf_set(argv[argi]);
	}
	return result ? EXIT_SUCCESS : EXIT_FAILURE;
}
