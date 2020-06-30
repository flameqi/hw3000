#include "fun.h"
/*
��������fת��Ϊ4���ֽ����ݴ����byte[4]��
*/
void Float_to_Byte(float f, uint8_t byte[], int start)
{
    FloatLongType fl;
    fl.fdata = f;
    byte[start + 0] = (uint8_t)fl.ldata;
    byte[start + 1] = (uint8_t)(fl.ldata >> 8);
    byte[start + 2] = (uint8_t)(fl.ldata >> 16);
    byte[start + 3] = (uint8_t)(fl.ldata >> 24);
}
/*
��4���ֽ�����byte[4]ת��Ϊ�����������*f��
*/
void Byte_to_Float(float *f, uint8_t byte[], int start)
{
    FloatLongType fl;
    fl.ldata = 0;
    fl.ldata = byte[start + 3];
    fl.ldata = (fl.ldata << 8) | byte[start + 2];
    fl.ldata = (fl.ldata << 8) | byte[start + 1];
    fl.ldata = (fl.ldata << 8) | byte[start + 0];
    *f = fl.fdata;
}
void ByteToLong(unsigned long *l, uint8_t byte[], int start)
{
    *l = byte[start + 3];
    *l = *l | ((unsigned long)byte[start + 2] << 8);
    *l = *l | ((unsigned long)byte[start + 1] << 16);
    *l = *l | ((unsigned long)byte[start + 0] << 24);
}
void LongToByte(unsigned long l, uint8_t byte[], int start)
{
    byte[start+3]=l;
    byte[start+2]=l>>8;
    byte[start+1]=l>>16;
    byte[start+0]=l>>24;
}