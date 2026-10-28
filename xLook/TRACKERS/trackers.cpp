#include <windows.h>
#include "../dtr.hpp"

#include "pt2.hpp"
#include "pt3.hpp"
#include "module.hpp"

namespace TRACKERS
{

// detector functions
int is_pt1(const BYTE *data, int size)
{
	int playerSize=0;

	BYTE pt1_player[] = {0x21,0xFD,0xC3,0xFD,0xCD,0xFD,0xC3,0xFF};
        if(search_with_table(data, pt1_player, size))
          playerSize = 0x6C6;

        if(playerSize > size || size - playerSize < 256)
          return -1;
        size -= playerSize;
        data += playerSize;

        BYTE delay = data[0];
        BYTE music_length  = data[1];
	BYTE loop_position = data[2];

	WORD pattern_table_offset = *(WORD*)(data+0x43);
	WORD position_list_offset = 0x64;

	if(delay < 3 || delay > 0x0f) return -1;

	if(music_length < loop_position) return -1;

	if(pattern_table_offset != position_list_offset + music_length) return -1;

        if(pattern_table_offset > size)
          return -1;
        if(data[pattern_table_offset - 1] != 0xff)
          return -1;

        return playerSize;
}

bool quickView_pt1(const BYTE *source, DWORD sourceSize, BYTE *unpackBuffer, DWORD *noUnpackedBytes, int playerSize)
{
	strcpy((char*)unpackBuffer, " tracker: ProTracker 1.x\n");

	if(playerSize)
		strcat((char*)unpackBuffer, "  player: yes\n");
	else
		strcat((char*)unpackBuffer, "  player: no\n");

	char buf[32] = "";
	strncpy (buf, (char*)source+0x45+playerSize, 31);
	trimRight(buf);
	sprintf((char*)unpackBuffer + strlen((char*)unpackBuffer), "  module: %s\n", buf);

	*noUnpackedBytes = strlen((char*)unpackBuffer);
	return true;
}

int is_as0(const BYTE *data, int size)
{
	BYTE asm0xx_player[] = {0xFE,0x0B,0xC3,0xFD,0xC3,0xFD,0xC3,0xFE,0x41,0xAF,0x21,0xFE,0x1C,0x78,0xC4,0xFF};
        if(search_with_table(data, asm0xx_player, size))
          return 111;

        BYTE asm012_player[] = {0xFE, 0x0B, 0xC3, 0xFD, 0xC3, 0xFD, 0xC3, 0xFE,
                                0x41, 0xAF, 0x21, 0xFE, 0x1C, 0xA7, 0xC4, 0xFF};
        if(search_with_table(data, asm012_player, size))
          return 111;

        if(data[0] < 2 || data[0] >= 0x33)
          return -1;
        if(data[7] >= 0x64)
          return -1;

        WORD size1 = *(WORD*)(data+1);
	if(size1 < 8) return -1;
	WORD size2 = *(WORD*)(data+3);
	if(size2 < 0x18) return -1;
	WORD size3 = *(WORD*)(data+5);
	if(size3 < 0x87) return -1;

        if(size2 - 1 >= size)
          return -1;
        if(data[size2 - 1] != 0xFF)
          return -1;

        if(size1 >= size2)
          return -1;
        if(size2 >= size3) return -1;

	if(8 + data[7] != size1) return -1;

	return 0;
}

int is_asm(const BYTE *data, int size)
{
	BYTE asm200_player[] = {0x18,0x5C,0x18,0x51,0x18,0x3F,0xFE,63,0x16,0x0D,0x01,0xFD,0xED,0x51,0x06,0xBF,0xED,0x71,0xFF};
        if(search_with_table(data, asm200_player, size))
          return 111;

        BYTE asm112_player[] = {0xFE, 0x0B, 0x18, 0xFD, 0xC3, 0xFD, 0xC3, 0xFE, 0x41,
                                0xCD, 0x52, 0x00, 0xFD, 0xFD, 0x71, 0xC6, 0xFF};
        if(search_with_table(data, asm112_player, size))
          return 111;

        BYTE asm112_2_player[] = {0xFE, 0x0B, 0x18, 0xFD, 0xC3, 0xFD, 0xC3, 0xFE, 0x41,
                                  0xCD, 0x52, 0,    0xFD, 0xFD, 0xDF, 0x04, 0xFF};
        if(search_with_table(data, asm112_2_player, size))
          return 111;

        BYTE asm111_player[] = {0xFE, 0x0B, 0x18, 0xFD, 0xC3, 0xFD, 0xC3, 0xFE, 0x41,
                                0xCD, 0x52, 0x00, 0xFD, 0xFD, 0x17, 0xC6, 0xFF};
        if(search_with_table(data, asm111_player, size))
          return 111;

        BYTE asm111_2_player[] = {0xFE, 0x0B, 0x18, 0xFD, 0xC3, 0xFD, 0xC3, 0xFE, 0x41,
                                  0xCD, 0x52, 0x00, 0xFD, 0xFD, 0x8D, 0x04, 0xFF};
        if(search_with_table(data, asm111_2_player, size))
          return 111;

        BYTE asm110_player[] = {0xFE, 0x0B, 0xC3, 0xFD, 0xC3, 0xFD, 0xC3, 0xFE,
                                0x41, 0xAF, 0x21, 0xFE, 0x1C, 0xF9, 0xC5, 0xFF};
        if(search_with_table(data, asm110_player, size))
          return 111;

        BYTE asm103_player[] = {0xFE, 0x0B, 0xC3, 0xFD, 0xC3, 0xFD, 0xC3, 0xFE,
                                0x41, 0xAF, 0x21, 0xFE, 0x1C, 0xCC, 0xC4, 0xFF};
        if(search_with_table(data, asm103_player, size))
          return 111;

        if(data[0] < 2 || data[0] >= 0x33)
          return -1;
        if(data[1] >= 100)
          return -1;
        if(data[8] == 0 || data[8] > 100) return -1;

	WORD size1 = *(WORD*)(data+2);
	if(size1 < 9) return -1;
	WORD size2 = *(WORD*)(data+4);
	if(size2 < 0x18) return -1;
	WORD size3 = *(WORD*)(data+6);
	if(size3 < 0x8B) return -1;

        if(size2 - 1 >= size)
          return -1;
        if(data[size2 - 1] != 0xFF)
          return -1;

        if(size1 >= size2)
          return -1;
        if(size2 >= size3) return -1;

	for(int i = 0; i < data[8]; ++i) if(data[9+i] >= 0x20) return -1;

	return 0;
}

int is_sqt(const BYTE *data, int size)
{
	int playerSize=0;

	BYTE sqt_player[] = {0x3E,0xFE,0x01,0x32,0xFD,0x32,0xFD,0x32,0xFD,0x01,0x01,0x01,0xCD,0xFD,0x2A,0xFD,0xDD,0x21,0xFD,0xCD,0xFD,0xCD,0xFD,0xCD,0xFD,0x11,0xFD,0xCD,0xFD,0x1E,0x00,0x14,0x7A,0xFD,0x20,0xF5,0xC9,0x21,0xFF};
        if(search_with_table(data, sqt_player, size))
          playerSize = 0x510;

        if(data[3] < 0x40)
          return -1;

        WORD size1 = *(WORD *)(data + 0);
        WORD size2 = *(WORD*)(data+2);
	WORD size3 = *(WORD*)(data+6);
	WORD size4 = *(WORD*)(data+8);
	WORD size5 = *(WORD*)(data+10);

	WORD bc  = size2 - 10;
	WORD sq1 = size1 + bc;
	WORD hl  = size1 + size2 - 10;
	if(hl < size4) return -1;
	hl -= size4;
	if(hl%7) return -1;
	if(hl/7 == 0 || hl/7 >= 121) return -1;
	if(sq1 < size5) return -1;
	if(size4 > size5) return -1;
	if(size3+2 < size2) return -1;

	return playerSize;
}

int is_stp(const BYTE *data, int size)
{
	int playerSize=0;

	//BYTE stp_player[] = {0x21,0xFD,0xC3,0xFD,0xC3,0xFD,0xED,0xFE,0x03,0xC3,0xFF};

        //BYTE stp20_player[] = {0xC3,0xFD,0xC3,0xFD,0xFE,0x48-0x06,0xF3,0x21,0xFD,0x22,0xFD,0x3E,0xFE,1,0x32,0xFD,0x32,0xFD,0x32,0xFD,0x7E,0x23,0x32,0xFD,0xCD,0xFD,0x7E,0xFF};

        BYTE stp_player[] = {0x21, 0xFD, 0xC3, 0xFD, 0xC3, 0xFD, 0xED, 0xFE, 0x03, 0xC3, 0xFF};
        if(search_with_table(data, stp_player, size))
          playerSize = 0x768;

        BYTE stp20_player[] = {0xC3, 0xFD, 0xC3, 0xFD, 0xFE, 0x48 - 0x06, 0xF3, 0x21, 0xFD, 0x22,
                               0xFD, 0x3E, 0xFE, 1,    0x32, 0xFD,        0x32, 0xFD, 0x32, 0xFD,
                               0x7E, 0x23, 0x32, 0xFD, 0xCD, 0xFD,        0x7E, 0xFF};
        if(search_with_table(data, stp20_player, size))
          playerSize = 0x788;

        if(playerSize > size || size - playerSize < 256)
          return -1;
        size -= playerSize;
        data += playerSize;

        BYTE delay = data[0];

        if(delay < 3 || delay > 0x0f) return -1;

	WORD position_list_offset = *(WORD*)(data+1);
	WORD ornament_list_offset = *(WORD*)(data+3);
	WORD pattern_list_offset  = *(WORD*)(data+5);
	WORD samples_list_offset  = *(WORD*)(data+7);

	if(position_list_offset <= 0x3c) return -1;
	if(ornament_list_offset <= 0x40) return -1;
	if(pattern_list_offset  <= 0x46) return -1;
	if(samples_list_offset  <= 0x66) return -1;

	if(position_list_offset >= ornament_list_offset) return -1;
	if(ornament_list_offset >= pattern_list_offset) return -1;

	if(samples_list_offset - pattern_list_offset != 0x20) return -1;

        if(position_list_offset + 2 > size)
          return -1;
        BYTE music_length = data[position_list_offset];
        BYTE loop_position = data[position_list_offset + 1];
        if(music_length < loop_position) return -1;
        if(position_list_offset + 2 + 2 * music_length > size)
          return -1;
        for(BYTE i = 0; i < music_length; ++i)
          if(data[position_list_offset + 2 + 2 * i] % 6)
            return -1;

        return playerSize;
}

bool quickView_stp(const BYTE *source, DWORD sourceSize, BYTE *unpackBuffer, DWORD *noUnpackedBytes, int playerSize)
{
	strcpy((char*)unpackBuffer, " tracker: Sound Tracker Pro\n");

	if(playerSize)
		strcat((char*)unpackBuffer, "  player: yes\n");
	else
		strcat((char*)unpackBuffer, "  player: no\n");

	char buf[64] = "";

	if(playerSize == 0x768)
		strncpy(buf, (char*)source+0x2D, 0x1A);

	if(playerSize == 0x788)
		strncpy(buf, (char*)source+0x25, 0x1C);

	if(!playerSize && source[0x0A] == 'K' && source[0x0B] == 'S' && source[0x0C] == 'A')
		strncpy(buf, (char*)source+0x26, 0x1A);

	trimRight(buf);
	if(*buf)
		sprintf((char*)unpackBuffer + strlen((char*)unpackBuffer), "  module: %s\n", buf);

	*noUnpackedBytes = strlen((char*)unpackBuffer);
	return true;
}

int is_stf(const BYTE *data, int size)
{
	int playerSize=0;

	BYTE stf_player[] = {0x20,'S','T','P','r','o',0x20,'f','i','x','p','l','a','y','e','r',0x20,'b','y',0x20,'R','e','d',0x20,'A','l','e','x',0x2f,'C','I','A',0x27,'0','1',0x20,0xFF};
        if(size > 0x0B && search_with_table(data + 0x0B, stf_player, size - 0x0B))
          playerSize = 0x74C;

        if(playerSize > size || size - playerSize < 256)
          return -1;
        size -= playerSize;
        data += playerSize;

        BYTE stf_header[] = {0x20, 'T', 'R', 'E', 'K', ':', 0x20, 0xFF};
        if(search_with_table(data + 1, stf_header, size - 1))
          return playerSize;

        return -1;
}

bool quickView_stf(const BYTE *source, DWORD sourceSize, BYTE *unpackBuffer, DWORD *noUnpackedBytes, int playerSize)
{
	strcpy((char*)unpackBuffer, " tracker: Sound Tracker Pro fix by Red Alex\n");

	if(playerSize)
		strcat((char*)unpackBuffer, "  player: yes\n");
	else
		strcat((char*)unpackBuffer, "  player: no\n");

	char buf[64] = "";
	strncpy(buf, (char*)source+playerSize+8, 26);
	sprintf((char*)unpackBuffer + strlen((char*)unpackBuffer), "  module: %s\n", buf);

	*noUnpackedBytes = strlen((char*)unpackBuffer);
	return true;
}

int is_str(const BYTE *data, int size)
{
	int playerSize=0;

	BYTE str_300_player[] = {0xFE,0x0B,0x18,0xFD,0x18,0xFD,0x18,0xFE,0x41,0xAF,0x06,0xBF,0xC3,0xFF};
        if(search_with_table(data, str_300_player, size))
          playerSize = 0x4A8;

        BYTE str_ksa_player[] = {0x21, 0xFD, 0xC3, 0xFD, 0xC3, 0xFD, 0xC3, 0xFF};
        if(search_with_table(data, str_ksa_player, size))
          playerSize = 0x69A;

        BYTE str_std_player[] = {0x21, 0xFD, 0xC3, 0xFD, 0xC3, 0xFE, 0x03, 0x7E, 0x32, 0xFF};
        if(search_with_table(data, str_std_player, size))
          playerSize = 0x43C;

        BYTE str_fls_player[] = {0x21, 0xFD, 0xC3, 0xFD, 0x97, 0x32, 0xFD, 0x32, 0xFD, 0x21, 0xFF};
        if(search_with_table(data, str_fls_player, size))
          playerSize = 0x59D;

        if(playerSize > size || size - playerSize < 256)
          return -1;
        size -= playerSize;
        data += playerSize;

        BYTE delay = data[0];

        if(delay < 3 || delay > 0x0f) return -1;

	WORD position_list_offset = *(WORD*)(data+1);
	WORD ornament_list_offset = *(WORD*)(data+3);
	WORD pattern_list_offset  = *(WORD*)(data+5);

	if(position_list_offset <= 0x1A) return -1;
	if(ornament_list_offset <= 0x1D) return -1;
	if(pattern_list_offset  <= 0x3E) return -1;

	if(position_list_offset >= ornament_list_offset) return -1;
	if(ornament_list_offset >= pattern_list_offset) return -1;

	if(pattern_list_offset >= size) return -1;

	BYTE position_list_size = data[position_list_offset];
	if((ornament_list_offset != position_list_offset + position_list_size*2 + 3) &&
		(ornament_list_offset != position_list_offset + position_list_size*2 + 1)) return -1;

	return playerSize;
}

bool quickView_str(const BYTE *source, DWORD sourceSize, BYTE *unpackBuffer, DWORD *noUnpackedBytes, int playerSize)
{
	strcpy((char*)unpackBuffer, " tracker: Sound Tracker\n");

	if(playerSize)
		strcat((char*)unpackBuffer, "  player: yes\n");
	else
		strcat((char*)unpackBuffer, "  player: no\n");

	char buf[64] = "";
	switch (playerSize)
	{
	case 0x69A: // KSA comment in header
		strncpy(buf, (char*)source+0x28, 28);
		break;
	case 0x4A8: // ST3 comment in header
		strncpy(buf, (char*)source+0x31, 11);
		break;
	default:
		strncpy(buf, (char*)source+playerSize+7, 19);
		break;
	}
	trimRight(buf);
	if(strcmp(buf, "SONG BY ST COMPILE") &&
		strcmp(buf, "SONG BY MB COMPILE") &&
		strcmp(buf, "SONG BY ST-COMPILE") &&
		strcmp(buf, "SOUND TRACKER v1.1") &&
		strcmp(buf, "S.T.FULL EDITION  ") &&
		strcmp(buf, "SOUND TRACKER v1.3") &&
		strcmp(buf, " STU SONG COMPILER") &&
		strcmp(buf, "(C) KLAV \"S_SONIC\""))
		sprintf((char*)unpackBuffer + strlen((char*)unpackBuffer), "  module: %s\n", buf);

	if(playerSize == 0x4A8) // ST3 autor info
	{
		strncpy (buf, (char*)source+0x3F, 13);
		trimRight(buf);
		sprintf((char*)unpackBuffer + strlen((char*)unpackBuffer), "      by: %s\n", buf);
	}

	*noUnpackedBytes = strlen((char*)unpackBuffer);
	return true;
}

int is_gtr(const BYTE *data, int size)
{
	int playerSize=0;
	//BYTE gtr1xx_player[] = {0x21,0xFD,0xC3,0xFD,0xC3,0xFD,0xFE,143,0x00,0xE1,0x00,0xD6,0x00,0xC8,0x00,0xBD,0x00,0xB2,0x00,0xA8,0x00,0x9F,0x00,0x96,0x00,0x8E,0xFF};

	BYTE gtrHeader[] = {'G','T','R',0xFF};
        if(size > 0x6C9 && search_with_table(data + 0x6C9, gtrHeader, size - 0x6C9))
          playerSize = 0x6C8;
        if(size > 0x5F6 && search_with_table(data + 0x5F6, gtrHeader, size - 0x5F6))
          playerSize = 0x5F5;
        if(size > 0x63B && search_with_table(data + 0x63B, gtrHeader, size - 0x63B))
          playerSize = 0x63A;

        if(playerSize > size || size - playerSize < 256)
          return -1;
        size -= playerSize;
        data += playerSize;

        if(size < 295)
          return -1;
        BYTE delay = data[0];
        BYTE music_length = data[293];
        BYTE loop_position = data[294];

	const BYTE *id     = data + 1;

	if(id[0] != 'G' || id[1] != 'T' || id[2] != 'R') return -1;

	if(delay < 3 || delay > 0x0f) return -1;
	if(music_length + 1 < loop_position) return -1;

        if(295 + music_length > size)
          return -1;
        const BYTE * position_list = data + 293 + 2;
        for(BYTE i = 0; i < music_length; ++i)
          if(position_list[i] % 6)
            return -1;

	return playerSize;
}

bool quickView_gtr(const BYTE *source, DWORD sourceSize, BYTE *unpackBuffer, DWORD *noUnpackedBytes, int playerSize)
{
	sprintf((char*)unpackBuffer, " tracker: Global Tracker %c.%c\n", '0'+(source[playerSize+4]>>4), '0'+(source[playerSize+4]&0xF));

	if(playerSize)
		strcat((char*)unpackBuffer, "  player: yes\n");
	else
		strcat((char*)unpackBuffer, "  player: no\n");

	char buf[64] = "";
	strncpy (buf, (char*)source+7+playerSize, 33);
	trimRight(buf);
	sprintf((char*)unpackBuffer + strlen((char*)unpackBuffer), "  module: %s\n", buf);

	*noUnpackedBytes = strlen((char*)unpackBuffer);
	return true;
}

int is_psc(const BYTE *data, int size)
{
	int playerSize=0;
	//BYTE psc1xx_player[] = {0x21,0xFD,0xC3,0xFD,0xC3,0xFE,0x19,0x22,0xFD,0xEB,0x21,0x4C,0x00,0x19,0x22,0xFD,0x22,0xFD,0x22,0xFD,0x2B,0x2B,0x7E,0x23,0x66,0x6F,0x19,0x22,0xFF};

	BYTE pscHeader[] = {'P','S','C',' ','V','1','.',0xFD,' ','C','O','M','P','I','L','A','T','I','O','N',' ','O','F',0xFF};
        if(size > 0xC95 && search_with_table(data + 0xC95, pscHeader, size - 0xC95))
          playerSize = 0xC95;
        if(size > 0xBA2 && search_with_table(data + 0xBA2, pscHeader, size - 0xBA2))
          playerSize = 0xBA2;
        if(size > 0xBAB && search_with_table(data + 0xBAB, pscHeader, size - 0xBAB))
          playerSize = 0xBAB;

        if(playerSize > size || size - playerSize < 256)
          return -1;
        size -= playerSize;
        data += playerSize;

        BYTE delay = data[73];
        if(delay < 3 || delay >= 32) return -1;

	WORD pattern_list_offset  = *(WORD*)(data+71);
	if(pattern_list_offset >= size) return -1;

	const BYTE *p = data + pattern_list_offset;

	for(BYTE i = 0; i < 0xff; ++i)
	{
          if(p - data + 2 > size)
            return -1;
          if(*p == i || p[1] != 0xff) {
            if(p[1] > 65 || p[1] == 0)
              return -1;
            if(p - data + 8 > size)
              return -1;
            p += 8;
          } else {
            if(*p > i)
              return -1;
          }
        }
        return playerSize;
}

bool quickView_psc(const BYTE *source, DWORD sourceSize, BYTE *unpackBuffer, DWORD *noUnpackedBytes, int playerSize)
{
	sprintf((char*)unpackBuffer, " tracker: Pro Sound Creator 1.%c%c\n", source[playerSize+7], source[playerSize+8]);

	if(playerSize)
		strcat((char*)unpackBuffer, "  player: yes\n");
	else
		strcat((char*)unpackBuffer, "  player: no\n");

	char buf[64] = "";
	strncpy (buf, (char*)source+0x19+playerSize, 0x15);
	trimRight(buf);
	sprintf((char*)unpackBuffer + strlen((char*)unpackBuffer), "  module: %s\n", buf);

	strncpy (buf, (char*)source+0x31+playerSize, 0x15);
	trimRight(buf);
	sprintf((char*)unpackBuffer + strlen((char*)unpackBuffer), "      by: %s\n", buf);

	*noUnpackedBytes = strlen((char*)unpackBuffer);
	return true;
}

// end of detectors

typedef int  (*detector_t) (const BYTE *data, int file_size);
typedef bool (*viewer_t)   (const BYTE *source, DWORD sourceSize, BYTE *unpackBuffer, DWORD *noUnpackedBytes, int playerSize);

struct Detector
{
	detector_t  detect;
	viewer_t    quickView;
        const char * title;
};

Detector detectors[] = {{is_pt1, quickView_pt1, "[ProTracker v1.xx]"},
                        {is_pt2, quickView_pt2, "[ProTracker v2.xx]"},
                        {is_pt3, quickView_pt3, "[ProTracker v3.xx]"},
                        {is_as0, NULL, "[ASM v0.12]"},
                        {is_asm, NULL, "[ASM v1.xx]"},
                        {is_sqt, NULL, "[SQ-Tracker]"},
                        {is_stp, quickView_stp, "[Sound Tracker Pro]"},
                        {is_stf, quickView_stf, "[Sound Tracker Pro fix by Red Alex]"},
                        {is_str, quickView_str, "[Sound Tracker]"},
                        {is_gtr, quickView_gtr, "[Global Tracker]"},
                        {is_psc, quickView_psc, "[Pro Sound Creator]"}};

bool WINAPI extract(HoHdr *, const BYTE * source, DWORD sourceSize, BYTE * output, DWORD * size,
                    DWORD id) {
  size_t index = id >> 16, offset = id & 0xffff;
  if(index >= sizeof(detectors) / sizeof(detectors[0]) || offset > sourceSize ||
     sourceSize - offset < 256)
    return false;
  char text[2048]{};
  DWORD length = sizeof(text);
  if(detectors[index].quickView) {
    if(!detectors[index].quickView(source, sourceSize, reinterpret_cast<BYTE *>(text), &length,
                                   offset))
      return false;
  } else {
    snprintf(text, sizeof(text), "Tracker: %s\n", detectors[index].title);
    length = strlen(text);
  }
  if(length >= sizeof(text))
    return false;
  if(*size < length) {
    *size = length;
    return false;
  }
  memcpy(output, text, length);
  *size = length;
  return true;
}

bool WINAPI detect(DetectorResult &result, const char*, const HoHdr*, const BYTE *data, int file_size)
{
  if(file_size < 256)
    return false;
  for(int i = 0; i < sizeof(detectors) / sizeof(Detector); ++i) {
    int playerSize = detectors[i].detect(data, file_size);
    if(playerSize != -1) {
      result.internal_id = playerSize + (i << 16);
      if(playerSize < 0 || playerSize > file_size || file_size - playerSize < 256)
        continue;
      result.action = do_extract;
      strcpy(result.ext, ".txt");
      strcpy(result.title, detectors[i].title);
      strcpy(result.format, "tracker");
      return true;
    }
  }
  return false;
}
}
