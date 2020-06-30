#ifndef _FUN_H_
#define _FUN_H_
#include "hw3k_config.h"
typedef union
{
	float fdata;
	unsigned long ldata;
}FloatLongType;
 
 void Float_to_Byte(float f,unsigned char byte[],int start);
 void Byte_to_Float(float *f,unsigned char byte[],int start);
 void ByteToLong(unsigned long *l, uint8_t byte[], int start);
 void LongToByte(unsigned long l, uint8_t byte[], int start);
 
 
 
 #endif