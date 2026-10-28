#include <windows.h>
#include "pt3.hpp"
#include "module.hpp"

int is_pt3(const BYTE* data, int size)
{
	int moduleOffset = 0;
	// let's try to find player
	if(size >= 3*256+216 && data[0] == 0x21 /*&& data[3] == 0x18*/)
	{
		// LD HL, data: JR init
		WORD offset = 3*256 + data[1];
		BYTE ptSignature[] = { 'P','r','o','T','r','a','c','k','e','r',0xFF};
		BYTE vtSignature[] = { 'V','o','r','t','e','x',' ','T','r','a','c','k','e','r',0xFF};
		for(int i = 0; i < 20; ++i)
		{
			if(size - offset < 216) break;
			//if(compareMemory(data + offset, ptSignature, sizeof(ptSignature)) ||
				//compareMemory(data + offset, vtSignature, sizeof(vtSignature)))
                        if(search_with_table(data + offset, ptSignature, size - offset) ||
                           search_with_table(data + offset, vtSignature, size - offset)) {
                          moduleOffset = offset;
                          break;
                        }
                        offset += 256;
                }
		if(moduleOffset == 0) return -1;
	}

        if(moduleOffset > size || size - moduleOffset < 0xCA)
          return -1;
        const BYTE * module = data + moduleOffset;
        BYTE delay = module[0x64];
        BYTE music_length  = module[101];

	WORD pattern_table_offset = *(WORD*)(module+103);
	WORD position_list_offset = 0xC9;

	if(delay < 1 || delay >= 0x40) return -1;
	if(music_length == 0)          return -1;

        if(position_list_offset + music_length >= size - moduleOffset)
          return -1;
        const BYTE * position_list = module + position_list_offset;
        WORD i = 0;
        for(; i < music_length; ++i)
	{
		if(position_list[i] > 0x30*3) return -1;
		if(position_list[i] % 3)      return -1;
	}
	if(position_list[i] != 0xff) return -1;

	return moduleOffset;
}

bool quickView_pt3(const BYTE *source, DWORD sourceSize, BYTE *unpackBuffer, DWORD *noUnpackedBytes, int playerSize)
{
	if(source[playerSize] == 'P')
		sprintf((char*)unpackBuffer, " tracker: ProTracker 3.%c\n", source[playerSize+13]);
	else
		sprintf((char*)unpackBuffer, " tracker: Vortex Tracker II 1.%c\n", source[playerSize+20]);

	if(playerSize)
		strcat((char*)unpackBuffer, "  player: yes\n");
	else
		strcat((char*)unpackBuffer, "  player: no\n");

	char buf[64] = "";
	strncpy (buf, (char*)source+0x1e + playerSize, 33);
	trimRight(buf);
	sprintf((char*)unpackBuffer + strlen((char*)unpackBuffer), "  module: %s\n", buf);

	strncpy (buf, (char*)source+0x42+playerSize, 33);
	trimRight(buf);
	sprintf((char*)unpackBuffer + strlen((char*)unpackBuffer), "      by: %s\n", buf);

	*noUnpackedBytes = strlen((char*)unpackBuffer);
	return true;
}

