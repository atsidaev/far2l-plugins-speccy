#ifndef DTR_HPP
#define DTR_HPP

#include <windows.h>

typedef BYTE byte;

#pragma pack(push, 1)
struct HoHdr
{
  char name[8];
  char type;
  union
  {
	WORD start;
	struct
	{
		char type1;
		char type2;
	};
  };
  WORD length;
  BYTE reserved;
  BYTE noSex;
  WORD checkSum;
};
#pragma pack(pop)

enum Action { do_nothing = 0, do_strip_hobeta, do_extract };

struct DetectorResult
{
	int    structSize;//для совместимости с новыми версиями
	Action action;//пока не определено
	DWORD  internal_id;//для передачи параметров в extract и qview
	char   format [0x80];//для assotiation и types.ini (если будет поддержка в xTRD) (в xTRD sizeof(value)=128)
	char   title  [50];//заголовок панели просмотра
	char   ext    [5];//расширение для извлечения при входе в файл (вместе с точкой)
	HoHdr  hdr;//если можно восстановить имя файла, например как HRUST 1.x и 2.2+
	char   comment[100];//если будет поддержка в xTRD (в xTRD sizeof(comment)=100)
	WORD   noProcessedBytes;//сколько байт планируетс обработать (без учёта заголовка HoBeta)
};

extern "C"
{
	void WINAPI init      (const char *path);

	bool WINAPI detect    (DetectorResult& result, const char *file_name, const HoHdr *hdr, const BYTE *data, int file_size);
	bool WINAPI extract   (HoHdr *hdr, const BYTE *source, DWORD sourceSize, BYTE *unpackBuffer, DWORD *noUnpackedBytes, DWORD internal_id);
	bool WINAPI quickView (HoHdr *hdr, const BYTE *source, DWORD sourceSize, BYTE *unpackBuffer, DWORD *noUnpackedBytes, DWORD internal_id);

	void WINAPI cleanup   (void);
};

#endif
