#include <windows.h>
#include "extract.hpp"
#include "../shared/bytes.hpp"
static size_t operations;

static void step() {
  if(++operations > 8000000)
    throw speccy::DecodeError();
}

//некоторые команды
#define xor_a       a^=a; fz=1; fc=0;
#define djnz(label) b--; if(b) goto label;

//регистры Z80
#define bc BC.r16
#define de DE.r16
#define hl HL.r16
#define d DE.r8.HI
#define e DE.r8.LO
#define b BC.r8.HI
#define c BC.r8.LO
#define h HL.r8.HI
#define l HL.r8.LO

#define af AF.r16
#define a  AF.r8.HI
#define fc AF.f.f_c
#define fz AF.f.f_z

extern BYTE *curTo, *curFrom;
extern WORD toSize_, fromSize_;
extern WORD noBytesRead, noBytesWritten;

//регистры Z80
struct _rlh{BYTE LO; BYTE HI;};
struct _f{unsigned f_s:1;unsigned f_z:1;unsigned f_5:1;unsigned f_h:1;unsigned f3_:1;unsigned f_p:1;unsigned f_n:1;unsigned f_c:1;unsigned aa:8;};
union reg
{
  WORD r16;
  struct _rlh r8;
  struct _f f;
} AF, HL, BC, DE, af_, hl_, bc_, de_;

//все функции
int call7205();int call77de();int call77f5();int call75aa();int call763f();int call7529();int call782c();int call78a7();int call78e1();int call78b7();int call78fb();int call78b1();int call7479();int call74b8();int call78f3();int call7924();int call78db();int call7659();int call769d();int call76d2();int call76fd();int call7732();
int l78ab();int l78c7();int l78e7();int l790a();

//функции работы со стеком
WORD zxstack[40], sp=0;
void push(WORD val)
{
  if(sp >= 40)
    throw speccy::DecodeError();
  zxstack[sp]=val;
  sp++;
}

void pop(WORD &val)
{
  if(sp == 0)
    throw speccy::DecodeError();
  sp--;
  val=zxstack[sp];
}

//обмен регистров
void ex_sp_hl(void)
{
  if(sp == 0)
    throw speccy::DecodeError();
  WORD tmp=zxstack[sp-1];
  zxstack[sp-1]=hl;
  hl=tmp;
}

void ex_de_hl(void)
{
  WORD tmp;
  tmp=de; de=hl; hl=tmp;
}

//некоторые участки мамяти Z80 под таблицы и функции доступа к ним
BYTE m73d2[16]={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
     m7713[31]={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
                0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
     m789a[13]={0,0,0,0,0,0,0,0,0,0,0,0,0},
     m7f80[256];

void get16(WORD &reg, WORD adr)
{
  step();
  if((adr>=0x73d2)&&(adr<0x73e1)) {reg=m73d2[adr-0x73d2]+m73d2[adr-0x73d1]*256; return;}
  if((adr>=0x7713)&&(adr<0x7731)) {reg=m7713[adr-0x7713]+m7713[adr-0x7712]*256; return;}
  if((adr>=0x789a)&&(adr<0x78a6)) {reg=m789a[adr-0x789a]+m789a[adr-0x7899]*256; return;}
  if((adr>=0x7e80)&&(adr<0x7f7f)) {reg=m7f80[adr-0x7e80]+m7f80[adr-0x7e7f]*256; return;}
}

void get8(BYTE &reg, WORD adr)
{
  step();
  if((adr>=0x73d2)&&(adr<0x73e2)) {reg=m73d2[adr-0x73d2]; return;}
  if((adr>=0x7713)&&(adr<0x7732)) {reg=m7713[adr-0x7713]; return;}
  if((adr>=0x789a)&&(adr<0x78a7)) {reg=m789a[adr-0x789a]; return;}
  if((adr>=0x7e80)&&(adr<0x7f80)) {reg=m7f80[adr-0x7e80]; return;}
}

void put16(WORD adr, WORD reg)
{
  step();
  if((adr>=0x73d2)&&(adr<0x73e1)) {m73d2[adr-0x73d2]=reg&0xff; m73d2[adr-0x73d1]=reg>>8; return;}
  if((adr>=0x7713)&&(adr<0x7731)) {m7713[adr-0x7713]=reg&0xff; m7713[adr-0x7713]=reg>>8; return;}
  if((adr>=0x789a)&&(adr<0x78a6)) {m789a[adr-0x789a]=reg&0xff; m789a[adr-0x7899]=reg>>8; return;}
  if((adr>=0x7e80)&&(adr<0x7f7f)) {m7f80[adr-0x7e80]=reg&0xff; m7f80[adr-0x7e7f]=reg>>8; return;}
}

void put8(WORD adr, BYTE reg)
{
  step();
  if((adr>=0x73d2)&&(adr<0x73e2)) {m73d2[adr-0x73d2]=reg; return;}
  if((adr>=0x7713)&&(adr<0x7732)) {m7713[adr-0x7713]=reg; return;}
  if((adr>=0x789a)&&(adr<0x78a7)) {m789a[adr-0x789a]=reg; return;}
  if((adr>=0x7e80)&&(adr<0x7f80)) {m7f80[adr-0x7e80]=reg; return;}
}

//команда ldir
void ldir(void)
{
  BYTE tmp;
  while(bc--)
  {
    get8(tmp,hl);
    put8(de,tmp);
    hl++;de++;
  }
}

//таблица по 13 байт (занимает половину памяти из 128к) и функции доступа
BYTE tabl[65520];//[1260*4*13];
void call785a(void)// hl=789a
{
  for(BYTE k=0; k<13; k++)
  {
    BYTE tmp;
    get8(tmp,0x789a+k);
    if(size_t(hl) * 13 + k >= sizeof(tabl))
      throw speccy::DecodeError();
    tabl[hl*13+k]=tmp;
  }
}

void call7847(void)// de=789a
{
  for(BYTE k=0; k<13; k++)
  {
    BYTE tmp;
    if(size_t(hl) * 13 + k >= sizeof(tabl))
      throw speccy::DecodeError();
    tmp=tabl[hl*13+k];
    put8(0x789a+k,tmp);
  }
}

//функции сдвига
void rla(void)
{
  BYTE tmp=a;
  a=a<<1;
  if (fc) a++;
  fc=(tmp&0x80)?1:0;
}

void rl(BYTE &reg)
{
  BYTE tmp=reg;
  reg=reg<<1;
  if (fc) reg++;
  fc=(tmp&0x80)?1:0;
}

void rra(void)
{
  BYTE tmp=a;
  a=a>>1;
  if (fc) a|=0x80;
  fc=tmp&1;
}

void rr(BYTE &reg)
{
  BYTE tmp=reg;
  reg=reg>>1;
  if (fc) reg|=0x80;
  fc=tmp&1;
}

//работа с битами
void bit(BYTE bit, BYTE reg)
{
  fz=!((reg>>bit)&1);
  fc=0;
}

void cp(BYTE val)
{
  fz=(a==val)?1:0;
  if(val>a) fc=1;
  else      fc=0;
}

//логические функции
void andReg(BYTE reg) {
  a&=reg;
  fc=0;
  fz=(a==0)?1:0;
}

void orReg(BYTE reg) {
  a|=reg;
  fc=0;
  fz=(a==0)?1:0;
}

//работа со стеком
void exx(void)
{
  WORD tmp;
  tmp=bc; bc=bc_.r16; bc_.r16=tmp;
  tmp=de; de=de_.r16; de_.r16=tmp;
  tmp=hl; hl=hl_.r16; hl_.r16=tmp;
}

//чтение нескольких байт
bool putBytes(BYTE count1)// put some bytes
{
  BYTE count=0x80-count1;
  for(WORD k=0x7f80-count; k<0x7f80; k++)
  {
    BYTE tmp;
    get8(tmp,k);
    if(!putByte(tmp)) return false;
  }
  return true;
}

//вычитание из HL
void sbc_hl(WORD reg)
{
  WORD tmp=hl;
  fz=(hl==(reg+fc))?1:0;
  hl-=reg+fc;
  fc=(tmp<(reg+fc))?1:0;
}

//рапаковщик для LZPress
void unlz(BYTE* from, WORD fromSize, BYTE* to, WORD toSize)
{
  operations = 0;
  sp = 0;
  AF = {};
  HL = {};
  BC = {};
  DE = {};
  af_ = {};
  hl_ = {};
  bc_ = {};
  de_ = {};
  memset(m7f80, 0, sizeof(m7f80));
  memset(tabl, 0, sizeof(tabl));
  curTo     = to;
  curFrom   = from;
  fromSize_ = fromSize;
  toSize_   = toSize;

  noBytesRead = 0; noBytesWritten = 0;

       //d=0xcd;

        call7205();
        exx();
        hl=0;
        exx();
        call77de();
        if(fc) return;
        get16(bc,0x73d2);
        push(hl);
        fc=1;
        sbc_hl(bc);
        pop(hl);
        if(!fc) return;
        a=h;
        orReg(l);
        if(fz) return;
        bc=0x7f00;
        call77f5();
        if(!putBytes(a)) return;
        e=a;
        call75aa();

   while(1)
   {
        push(hl);
        exx();
        pop(hl);
        exx();
        call77de();
        if(fc) return;
        push(bc);
        push(hl);
        get16(bc,0x73d2);
        fc=1;
        sbc_hl(bc);
        pop(hl);
        pop(bc);
        if(!fc) return;
        a=h;
        orReg(l);

        if(!fz) {
          call77f5();
          if(!putBytes(a))
            return;
          call75aa();
          e = a;
          exx();
          call763f();
          exx();
          push(hl);
          l = e;
          h = 0;
          hl += bc;
          get8(a, hl);
          pop(hl);
          exx();
          call7529();
          exx();
        } else {
          a = e;
          call782c();
          if(a & 0x80)
            return; //or a; ret     m
          if(!putBytes(a))
            return;
          e = a;
          l = a;
          hl += bc;
          get8(a, hl);
          exx();
          call7529();
          push(hl);
          exx();
          pop(hl);
          call75aa();
        }
   }
}


int call7205()
{
        push(hl);
        push(de);
        push(bc);
        push(af);
        hl=0; BYTE tmp;
        for(tmp=0;tmp<16;tmp++) m73d2[tmp]=0;
        for(tmp=0;tmp<31;tmp++) m7713[tmp]=0;
        for(tmp=0;tmp<13;tmp++) m789a[tmp]=0;
        call785a();
        a=0xd9; bc=0; de=0x78a7;
l7239:  a+=0x47;
        hl=0;
        call7529();
        djnz(l7239);
        pop(af);
        pop(bc);
        pop(de);
        pop(hl);
        return 0;
}


int call7248()
{
        push(af);
        push(hl);
        push(de);
        push(bc);
        a=b;
        orReg(c);
        if(fz) goto l7266;
        call7847();
        push(hl);
        call78a7();
        push(hl);
        l=c;
        h=b;
        call7847();
        ex_sp_hl();
        call78e1();
        pop(hl);
        call785a();
        pop(hl);
l7266:  call7847();
        ex_de_hl();
        call78a7();
        call7847();
        push(hl);
        a=0xff;
l7273:  call78b7();
  orReg(a);
  sbc_hl(de);
  if(!fz)
    goto l7282;
  l = c;
  h = b;
  call78fb();
  goto l7287;
l7282:  a++;
        cp(02);
        if(!fz) goto l7273;
l7287:  pop(hl);
        call785a();
        pop(bc);
        pop(de);
        pop(hl);
        pop(af);
        return 0;
}


int call7290()
{
        push(af);
        push(de);
        push(bc);
l7293:  call7847();
        a=01;
        ex_de_hl();
        call78b7();
        a=l;
        orReg(h);
        if(!fz) goto l7293;
        a--;
        call78b7();
        c=l;
        b=h;
        ex_de_hl();
        call7248();
        pop(bc);
        pop(de);
        pop(af);
        return 0;
}

int call72ae()
{
        push(af);
        push(de);
        push(bc);
l72b1:  e=l;
        d=h;
        call7847();
        call78a7();
        call7847();
        xor_a;
        call78b7();
        sbc_hl(de);
        if(fz) goto l72cd;
        ex_de_hl();
        call7847();
        call78a7();
        goto l72b1;
l72cd:  ex_de_hl();
        call7847();
        call78a7();
        pop(bc);
        pop(de);
        pop(af);
        return 0;
}


int call72d8()
{
        push(af);
        push(de);
        push(bc);
        push(hl);
        call7847();
        ex_de_hl();
        a=0xff;
        call78b7();
        a=l;
        orReg(h);
        if(!fz) goto l72f1;
        a++;
l72ea:  call78b7();
        c=l;
        b=h;
        goto l735a;
l72f1:  a=0x01;
        call78b7();
        a=l;
        orReg(h);
        if(!fz) goto l72fd;
        a--;
        goto l72ea;
l72fd:  a=0xff;
        call78b7();
        call7290();
        c=l;
        b=h;
        ex_de_hl();
        call7847();
        a=01;
        push(hl);
        call78b7();
        ex_sp_hl();
        push(hl);
        a=0xff;
        call78b7();
        ex_sp_hl();
        ex_de_hl();
        call7847();
        a=0xff;
        ex_sp_hl();
        call78fb();
        ex_sp_hl();
        call785a();
        pop(hl);
        a=l;
        orReg(h);
        if(fz) goto l7339;
        call7847();
        push(hl);
        l=c;
        h=b;
        call78e1();
        pop(hl);
        call785a();
l7339:  l=c;
        h=b;
        call7847();
        ex_sp_hl();
        a=01;
        call78fb();
        ex_sp_hl();
        call785a();
        pop(hl);
        a=l;
        orReg(h);
        if(fz) goto l735a;
        call7847();
        push(hl);
        l=c;
        h=b;
        call78e1();
        pop(hl);
        call785a();
l735a:  ex_de_hl();
        call7248();
        pop(hl);
        pop(bc);
        pop(de);
        pop(af);
        return 0;
}


int call7363()
{
        push(af);
        push(de);
        push(bc);
        ex_de_hl();
        get16(hl,0x73da);
l736a:  a=h;
  orReg(l);
  if(fz)
    goto l73ce;
  push(hl);
  sbc_hl(de);
  pop(hl);
  if(fz)
    goto l737e;
  push(hl);
  call72ae();
  orReg(a);
  sbc_hl(de);
  pop(hl);
  if(!fz)
    goto l7386;
l737e:  call7847();
        call78b1();
        goto l736a;
l7386:  call72d8();
        call7847();
        ex_de_hl();
        call78a7();
        push(hl);
        call7847();
        xor_a;
        call78b7();
        a=l;
        orReg(h);
        if(fz) goto l739f;
l739c:  pop(hl);
        goto l73c7;
l739f:  get8(a,0x78a6);
  andReg(0x80);
  if(!fz)
    goto l739c;
  get8(a, 0x78a6);
  andReg(0x7f);
  cp(01);
  if(fz)
    goto l739c;
  orReg(0x80);
  put8(0x78a6, a);
  pop(hl);
  call785a();
  bc = 0x0080;
  push(hl);
  sbc_hl(bc);
  if(fc)
    goto l739c;
  pop(hl);
  bc = 0x73d8;
  call7479();
l73c7:  bc=0x73d8;
        ex_de_hl();
        call74b8();
l73ce:  pop(bc);
        pop(de);
        pop(af);
        return 0;
}


int call7404()
{
        push(hl);
        push(de);
        push(bc);
        push(af);
        e=a;
        call7847();
        push(hl);
        hl=0;
        a=0xff;
l7412:  call78fb();
        a++;
        cp(02);
        if(!fz) goto l7412;
        a=e;
        put8(0x789a,a);
        l=c;
        h=b;
        call78e1();
        ex_sp_hl();
        call785a();
        ex_sp_hl();
        call7847();
        a=d;
        ex_sp_hl();
        call78fb();
        ex_sp_hl();
        call785a();
        get8(a,0x78a6);
        andReg(0x7f);
        bit(0,d);
        if(!fz) goto l743e;
        a++;
l743e:  cp(01);
        if(fz) goto l7444;
        orReg(0x80);
l7444:  ex_sp_hl();
        call7847();
        put8(0x78a6,a);
        call785a();
        pop(hl);
        call7847();
        get8(a,0x78a6);
        bit(7,a);
        if(fz) goto l7474;
        bit(0,d);
        if(!fz) goto l7474;
        andReg(0x7f);
        put8(0x78a6,a);
        call785a();
        push(hl);
        bc=0x0080;
        sbc_hl(bc);
        pop(hl);
        if(fc) goto l7474;
        bc=0x73d8;
        call74b8();
l7474:  pop(af);
        pop(bc);
        pop(de);
        pop(hl);
        return 0;
}


int call7479()
{
        push(hl);
        push(af);
        push(de);
        push(bc);
        ex_de_hl();
        l=c;
        h=b;
        get8(c,hl);
        hl++;
        get8(b,hl);
        a=c;
        orReg(b);
        if(!fz) goto l748d;
        hl++;
        put8(hl,e);
        hl++;
        put8(hl,d);
        goto l749a;
l748d:  l=c;
        h=b;
        call7847();
        ex_de_hl();
        call78f3();
        ex_de_hl();
        call785a();
l749a:  push(bc);
        ex_de_hl();
        call7847();
        ex_sp_hl();
        call7924();
        hl=0;
        call78f3();
        pop(hl);
        call785a();
        ex_de_hl();
        pop(hl);
        push(hl);
        put8(hl,e);
        hl++;
        put8(hl,d);
        pop(bc);
        pop(de);
        pop(af);
        pop(hl);
        return 0;
}


int call74b8()
{
        push(hl);
        push(de);
        push(af);
        push(bc);
        ex_de_hl();
        h=b;
        l=c;
        get8(a,hl);
        cp(e);
        hl++;
        if(!fz) goto l74d6;
        get8(a,hl);
        cp(d);
        if(!fz) goto l74d6;
        ex_de_hl();
        call7847();
        push(hl);
        call78db();
        ex_de_hl();
        put8(hl,d);
        hl--;
        put8(hl,e);
        hl++;
        pop(de);
l74d6:  hl++;
        get8(a,hl);
        hl++;
        cp(e);
        if(!fz) goto l74ed;
        get8(a,hl);
        cp(d);
        if(!fz) goto l74ed;
        ex_de_hl();
        call7847();
        push(hl);
        call78b1();
        ex_de_hl();
        put8(hl,d);
        hl--;
        put8(hl,e);
        pop(de);
l74ed:  ex_de_hl();
        call7847();
        ex_de_hl();
        call78db();
        a=l;
        orReg(h);
        if(fz) goto l7509;
        push(hl);
        call78b1();
        ex_sp_hl();
        call7847();
        ex_sp_hl();
        call78f3();
        pop(hl);
        call785a();
l7509:  ex_de_hl();
        call7847();
        call78b1();
        a=l;
        orReg(h);
        if(fz) goto l7524;
        push(hl);
        call78db();
        ex_sp_hl();
        call7847();
        ex_sp_hl();
        call7924();
        pop(hl);
        call785a();
l7524:  pop(bc);
        pop(af);
        pop(de);
        pop(hl);
        return 0;
}


int call7529()
{
        push(de);
        push(bc);
        push(af);
        b=a;
        call7847();
        get8(a,0x78a6);
        andReg(0x7f);
        cp(0x3f);
        if(!fc) {hl=0; goto l75a1;}  //goto l75a5;
        push(hl);
        ex_de_hl();
        xor_a;
l753c:  c=a;
        call78b7();
        a=l;
        orReg(h);
        if(fz) goto l7556;
        call7847();
        get8(a,0x789a);
        cp(b);
        if(fz) goto l75a0;
        ex_de_hl();
        a=01;
        if(fc) goto l753c;
        a=0xff;
        goto l753c;
l7556:  get16(hl,0x73d2);
        hl++;
        hl++;
        bit(4,h);
        hl--;
        if(fz) goto l7567;
        pop(hl);
        push(hl);
        call7363();
        goto l756a;
l7567:  put16(0x73d2,hl);
l756a:  a=l;
  orReg(h);
  if(fz)
    goto l75a0;
  a = b;
  b = d;
  d = c;
  c = e;
  call7404();
  call7847();
  xor_a;
  put8(0x78a4, a);
  put8(0x78a5, a);
  call785a();
  bc = 0x0080;
  push(hl);
  sbc_hl(bc);
  pop(hl);
  if(!fc)
    goto l7593;
  bc = 0x73d4;
  call7479();
  goto l75a0;
l7593:  get8(a,0x78a6);
  andReg(0x80);
  if(fz)
    goto l75a0;
  bc = 0x73d8;
  call7479();
l75a0:  pop(af);
l75a1:  pop(af);
        pop(bc);
        pop(de);
        return 0;
}


int call75aa() ///call75aa
{
        push(af);
        push(de);
        push(bc);
        a=h;
        orReg(l);
        if(fz) goto l763b;
        call7847();
        push(hl);
        get16(hl,0x78a4);
        hl++;
        put16(0x78a4,hl);
        ex_de_hl();
        hl=0;
        put16(0x73dc,hl);
        put16(0x73de,hl);
        pop(hl);
        call785a();
        bc=0x0080;
        push(hl);
        sbc_hl(bc);
        pop(hl);
        if(!fc) goto l75df;
        bc=0x73d4;
l75d7:  call74b8();
        call7479();
        goto l763b;
l75df:  push(hl);
        get16(hl,0x73d6);
        call7847();
        get16(hl,0x78a4);
        sbc_hl(de);
        pop(hl);
        if(fc) goto l75fd;
        call7847();
        get8(a,0x78a6);
        andReg(0x80);
        if(fz) goto l763b;
        bc=0x73d8;
        goto l75d7;
l75fd:  push(hl);
        call7847();
        get8(a,0x78a6);
        andReg(0x80);
        if(fz) goto l760e;
        bc=0x73d8;
        call74b8();
l760e:  get16(hl,0x73d6);
        bc=0x73d4;
        call74b8();
        pop(bc);
        call7659();
        put16(0x73de,hl);
        put16(0x73dc,bc);
        push(bc);
        bc=0x73d4;
        call7479();
        ex_sp_hl();
        call7847();
        get8(a,0x78a6);
        andReg(0x80);
        if(fz) goto l763a;
        bc=0x73d8;
        call7479();
l763a:  pop(hl);
l763b:  pop(bc);
        pop(de);
        pop(af);
        return 0;
}


int call763f()
{
        push(af);
        push(de);
        ex_de_hl();
        get16(hl,0x73dc);
        orReg(a);
        sbc_hl(de);
        get16(hl,0x73de);
        if(fz) goto l7656;
        orReg(a);
        sbc_hl(de);
        get16(hl,0x73dc);
        if(fz) goto l7656;
        ex_de_hl();
l7656:  pop(de);
        pop(af);
        return 0;
}


int call7659()
{
        push(af);
        push(de);
        push(bc);
        push(hl);
        push(bc);
        de=0x7720;
        call769d();
        ex_sp_hl();
        de=0x7729;
        call769d();
        pop(de);
        call76d2();
        ex_de_hl();
        call76d2();
        call7847();
        call76fd();
        ex_de_hl();
        call7847();
        call76fd();
        call785a();
        ex_de_hl();
        call76fd();
        call785a();
        push(de);
        de=0x7729;
        call7732();
        de=0x7720;
        pop(hl);
        call7732();
        pop(hl);
        pop(bc);
        pop(de);
        pop(af);
        return 0;
}


int call769d()
{
        push(hl);
        call7847();
        call78a7();
        a=l;
        put8(de,a);
        de++;
        a=h;
        put8(de,a);
        de++;
        a=0xff;
l76ac:  call78b7();
        ex_de_hl();
        put8(hl,e);
        hl++;
        put8(hl,d);
        hl++;
        ex_de_hl();
        a++;
        cp(02);
        if(!fz) goto l76ac;
        call78a7();
        call7847();
        pop(bc);
        push(bc);
        a=0xff;
l76c4:  call78b7();
  orReg(a);
  sbc_hl(bc);
  if(fz)
    goto l76cf;
  a++;
  goto l76c4;
l76cf:  put8(de,a);
        pop(hl);
        return 0;
}


int call76d2()
{
        push(hl);
        push(hl);
        pop(bc);
        call7847();
        call78a7();
        orReg(a);
        sbc_hl(de);
        if(!fz) goto l76e5;
        h=b;
        l=c;
        call78e1();
l76e5:  a=0xff;
l76e7:  call78b7();
  orReg(a);
  sbc_hl(de);
  if(!fz)
    goto l76f4;
  l = c;
  h = b;
  call78fb();
l76f4:  a++;
        cp(02);
        if(!fz) goto l76e7;
        pop(hl);
        //goto l785a;
        call785a();
        return 0;
}


int call76fd()
{
        push(hl);
        push(de);
        b=0x0d;
        hl=0x789a;
        de=0x7713;
l7707:  get8(a,de);
        get8(c,hl);
        put8(hl,a);
        a=c;
        put8(de,a);
        hl++;
        de++;
        djnz(l7707);
        pop(de);
        pop(hl);
        return 0;
}



int call7732()
{
	ex_de_hl();
        push(hl);
        WORD ix=hl;
        get8(a,ix+0x08);
        get8(l,ix+00);
        get8(h,ix+01);
        orReg(a);
        push(hl);
        sbc_hl(de);
        pop(hl);
        if(fz) goto l7753;
        call7847();
        ex_de_hl();
        call78fb();
        ex_de_hl();
        call785a();
l7753:  b=03;
l7755:  get8(l,ix+02);
        get8(h,ix+03);
        a=l;
        orReg(h);
        if(fz) goto l7770;
        push(hl);
        sbc_hl(de);
        pop(hl);
        if(fz) goto l7770;
        call7847();
        ex_de_hl();
        call78e1();
        ex_de_hl();
        call785a();
l7770:  ix++;
        ix++;
        djnz(l7755);
        pop(ix);
        ex_de_hl();
        return 0;
}


int call777a()
{
        push(af);
        push(bc);
        push(de);
        get16(bc,0x73df);
        e=a;
        get8(l,0x73e1); //get16(hl,0x73e1);
        h=00;
        c=h;
        a=b;
l7789:  cp(e);
        if(!fc) goto l77b1;
        if(!getByte(a)) goto l77d9;
        push(de);
        ex_de_hl();
        l=a;
        h=00;
        a=b;
l7797:
  orReg(a);
  if(fz)
    goto l77a3;
  rl(l);
  rl(h);
  rl(c);
  a--;
  goto l7797;
l77a3:  a=l;
  orReg(e);
  e = a;
  a = h;
  orReg(d);
  d = a;
  ex_de_hl();
  pop(de);
  a = 0x08;
  a += b;
  b = a;
  goto l7789;
l77b1:  push(hl);
        a=e;
l77b3:
  orReg(a);
  rr(c);
  rr(h);
  rr(l);
  b--;
  a--;
  fz = (a == 0) ? 1 : 0;
  if(!fz)
    goto l77b3;
  a = b;
  put8(0x73e0, a);
  a = l;
  put8(0x73e1, a);
  a = 0x0f;
  pop(hl);
  andReg(h);
  h = a;
  a = e;
  cp(07);
  if(!fz)
    goto l77d4;
  h = 00;
  l = l & 0x7f; //res     7,l
l77d4:  pop(de);
        pop(bc);
        pop(af);
        andReg(a);
        return 0;

l77d9:  pop(de);
        pop(bc);
        pop(af);
        fc=1;//scf
        return 0;
}


int call77de()
{
        push(af);
        a=01;
        call777a();
        if(fc) {pop(af); fc=1; return 0;} //goto l77db;
        a=07;
        rr(l);
        if(fc) goto l77ee;
        a=0x0c;
l77ee:  call777a();
        if(fc) {pop(af); fc=1; return 0;} //goto l77db;
        {
          pop(af);
          andReg(a);
          return 0;
        } //goto l77d6;
}


int call77f5()
{
        push(hl);
        push(de);
        a=0x80;
l77f9:  a--;
        push(af);
        call7847();
        e=a;
        d=00;
        ex_de_hl();
        hl+=bc;
        get8(a,0x789a);
        put8(hl,a);
        ex_de_hl();
        call72ae();
        a=l;
        orReg(h);
        if(fz) goto l7812;
        pop(af);
        goto l77f9;
l7812:  pop(af);
        pop(de);
        pop(hl);
        return 0;
}


int call782c()
{
        push(bc);
        push(hl);
        push(de);
        push(af);
        l=a;
        h=00;
        a=0x80;
        a-=l;  //sub     l
        hl+=bc;
        c=a;
        get8(a,hl);
        d=h;
        e=l;
        de--;
        b=00;
        ldir();
        put8(de,a);
        pop(af);
        a--;
        pop(de);
        pop(hl);
        pop(bc);
        return 0;
}


int call78a7()
{
        push(af);
        get16(hl,0x789b);
        l78ab();
        return 0;
}int l78ab(){
        a=0x0f;
        andReg(h);
        h=a;
        pop(af);
        return 0;
}


int call78b1()
{       push(af);
        get16(hl,0x78a1);
        l78ab();return 0; //goto l78ab;
}

int call78b7()
{
        push(af);
        get16(hl,0x789e);
        orReg(a);
        if(fz) {l78ab(); return 0;}//goto l78ab;
        hl=0x789c;
        a++; fz=(a==0)?1:0;
        if(fz) {l78c7(); return 0;}//goto l78c7;
        hl++;
        hl++;
        hl++;
        l78c7();
        return 0;
}
int l78c7()
{
        get8(a,hl);
        hl++;
        get8(h,hl);
        andReg(0xf0);
        rr(h);
        rra();
        rr(h);
        rra();
        rr(h);
        rra();
        rr(h);
        rra();
        l=a;
        pop(af);
        return 0;
}


int call78db()
{
        push(af);
        hl=0x78a2;
        l78c7(); return 0; //goto l78c7;
}


int call78e1()
{
        push(af);
        push(hl);
        push(de);
        de=0x789c;
        l78e7();
        return 0;
}int l78e7(){
        ex_de_hl();
        get8(a,hl);
        andReg(0xf0);
        orReg(d);
        put8(hl,a);
        hl--;
        put8(hl,e);
        pop(de);
        pop(hl);
        pop(af);
        return 0;
}

int call78f3()
{ 
        push(af);
        push(hl);
        push(de);
        de=0x78a2;
        l78e7(); return 0; //goto l78e7;
}


int call78fb()
{
        push(af);
        push(hl);
        push(de);
        de=0x789f;
        orReg(a);
        if(fz) {l78e7(); return 0;}//goto l78e7;
        a++; fz=(a==0)?1:0;
        if(!fz) {l790a(); return 0; }//goto l790a;
        de--;
        de--;
        de--;
        l790a();
        return 0;
}int l790a(){
        ex_de_hl();
        a=d;
        rl(e);
        rla();
        rl(e);
        rla();
        rl(e);
        rla();
        rl(e);
        rla();
        d=a;
        get8(a,hl);
        andReg(0x0f);
        orReg(e);
        put8(hl,a);
        hl++;
        put8(hl,d);
        pop(de);
        pop(hl);
        pop(af);
        return 0;
}


int call7924()
{
        push(af);
        push(hl);
        push(de);
        de=0x78a2;
        l790a(); return 0;//goto l790a;
}

#undef xor_a
#undef djnz
#undef bc
#undef de
#undef hl
#undef d
#undef e
#undef b
#undef c
#undef h
#undef l
#undef af
#undef a
#undef fc
#undef fz
