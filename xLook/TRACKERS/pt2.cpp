#include <windows.h>
#include "pt2.hpp"
#include "module.hpp"

int is_pt2(const BYTE *data, int size)
{
	int offset = 0;

	BYTE pt20_player[] = {0x21,0xFD,0xC3,0xFD,0xC3,0xFE,0xFF,0xFE,0xFF,0xFE,53,0xF3,0xE5,0x22,0xFD,0xE5,0x7E,0x32,0xFF};
        if(search_with_table(data, pt20_player, size))
          offset = 0xA1F;

        //BYTE pt21_player[] = {0x21,0xFD,0xC3,0xFD,0xC3,0xFE,0x0E,0x32,0x2E,0x31,0xFF};
        BYTE pt21_player[] = {0x21, 0xFD, 0xC3, 0xFD, 0xC3, 0xFD, 0x20, 'P',  'R',  'O',
                              'T',  'R',  'A',  'C',  'K',  'E',  'R',  0x20, 0xFE, 0x01,
                              '.',  '1',  0x20, 'B',  'Y',  0x20, 'G',  'O',  'L',  'D',
                              'E',  'N',  0x20, 'D',  'I',  'S',  'K',  0xFF};
        if(search_with_table(data, pt21_player, size))
          offset = 0xA2F;

        BYTE pt24_player[] = {0x21, 0xFD, 0x18, 3,    0xC3, 0xFD, 0xF3, 0xE5, 0x7E,
                              0x32, 0xFD, 0x32, 0xFD, 0x23, 0x23, 0x7E, 0x23, 0xFF};
        if(search_with_table(data, pt24_player, size))
          offset = 0xA45;

        if(offset > size || size - offset < 0x84)
          return -1;
        const BYTE * module = data + offset;

        BYTE delay = module[0];
        BYTE music_length  = module[1];
	BYTE loop_position = module[2];

	WORD pattern_table_offset = *(WORD*)(module+0x63);
	WORD position_list_offset = 0x84;

	if(delay < 3 || delay > 0x0f) return -1;

	if(music_length < loop_position) return -1;

	if(pattern_table_offset != position_list_offset + music_length) return -1;

        if(pattern_table_offset > size - offset)
          return -1;
        if(module[pattern_table_offset - 1] != 0xff)
          return -1;

        return offset;
}

bool quickView_pt2(const BYTE *source, DWORD sourceSize, BYTE *unpackBuffer, DWORD *noUnpackedBytes, int playerSize)
{
	char version = 'x';
	switch(playerSize)
	{
		case 0xA1F: version = '0'; break;
		case 0xA2F: version = '1'; break;
		case 0xA45: version = '4'; break;
	}

	if(version=='1'&&source[0x15]=='3')
		strcpy((char*)unpackBuffer, " tracker: ProTracker 3.1 remix by Frunze/SG\n");
	else
		sprintf((char*)unpackBuffer, " tracker: ProTracker 2.%c\n", version);

	if(playerSize)
		strcat((char*)unpackBuffer, "  player: yes\n");
	else
		strcat((char*)unpackBuffer, "  player: no\n");

	char buf[32] = "";
	strncpy (buf, (char*)source+0x65+playerSize, 31);
	trimRight(buf);
	sprintf((char*)unpackBuffer + strlen((char*)unpackBuffer), "  module: %s\n", buf);

	*noUnpackedBytes = strlen((char*)unpackBuffer);
	return true;
}

