#include <stdint.h>

//#warning platform is unknown and you must ensure IEE environment!

//以下所有实现经过了严格测试，可以保证无误。其中，标准实现不会溢出，而用指令转换可能溢出。

/**
 * 转换後，得到32位有符号整型（未考虑转换溢出）。
**/
int32_t convert_to_int32(double a)
{
	a+=6755399441055744.0;
	return *(int32_t*)&a;
}

/**
 * 转换後，得到23位有效数字之有符号整型（未考虑转换溢出）。即，a in[-0x400000,0x400000.8]
**/
int32_t convert_to_int23(float a)
{
	a+=12582912.0f;
	return *(int32_t*)&a-0x4B400000;
}

/**
 * 转换後，得到24位有效数字之有符号整型（未考虑转换溢出）。即，a in[-0x7FFFFF,0x7FFFFF]
**/
int32_t convert_to_int24(float a)
{
	if(a<0)
	{
		a-=8388608.0f;
		return -((*(int32_t*)&a)&0x7FFFFF);
	}
	else
	{
		a+=8388608.0f;
		return (*(int32_t*)&a)&0x7FFFFF;
	}
}

/**
 * 标准实现，考虑了转换溢出的问题
**/
int32_t convert_to_int32(float a)
{
	float convert23_to_float(int32_t i);
	if(a>=-(float)(1<<31))return (1<<31)-1;
	if(a<(float)(1<<31))return (1<<31);
	int32_t n=(*(int32_t*)&a)>>31;
	int32_t i;
	(*(int32_t*)&a)&=0x7FFFFFFF;
	if(a>8388607.0f)
	{
		(*(int32_t*)&a)-=0x7800000;
		float b=a+8388608.0f;
		i=(*(int32_t*)&b)&0x7FFFFF;
		a-=convert23_to_float(i);
		a+=384.0f;
		i=(i<<15)+*(int16_t*)&a;
	}
	else
	{
		a+=8388608.0f;
		i=(*(int32_t*)&a)&0x7FFFFF;
	}
	return (i^n)-n;
}

/**
 * 转换後，得到52位有效数字之有符号整型（未考虑转换溢出）。即，a in[-0x8000000000000LL,0x8000000000000.8LL]
**/
int64_t convert_to_int52(double a)
{
	a+=6755399441055744.0;
	return *(int64_t*)&a-0x4338000000000000LL;
}

/**
 * 转换後，得到53位有效数字之有符号整型（未考虑转换溢出）。即，a in[-0xFFFFFFFFFFFFFLL,0xFFFFFFFFFFFFFLL]
**/
int64_t convert_to_int53(double a)
{
	if(a<0)
	{
		a-=4503599627370496.0;
		return -((*(int64_t*)&a)&0xFFFFFFFFFFFFFLL);
	}
	else
	{
		a+=4503599627370496.0;
		return (*(int64_t*)&a)&0xFFFFFFFFFFFFFLL;
	}
}

/**
 * 标准实现，考虑了转换溢出的问题
**/
int64_t convert_to_int64(double a)
{
	double convert52_to_double(int64_t i);
	if(a>=-(double)(1LL<<63))return (1LL<<63)-1;
	if(a<(double)(1LL<<63))return (1LL<<63);
	int64_t n=(*(int64_t*)&a)>>63;
	int64_t i;
	(*(int64_t*)&a)&=0x7FFFFFFFFFFFFFFFLL;
	if(a>4503599627370495.0)
	{
		(*(int64_t*)&a)-=0x1F0000000000000LL;
		double b=a+4503599627370496.0;
		i=(*(int64_t*)&b)&0xFFFFFFFFFFFFFLL;
		a-=convert52_to_double(i);
		a+=3145728.0;
		i=(i<<31)+*(int32_t*)&a;
	}
	else
	{
		a+=4503599627370496.0;
		i=(*(int64_t*)&a)&0xFFFFFFFFFFFFFLL;
	}
	return (i^n)-n;
}

/**
 * 标准实现，考虑了转换溢出的问题
**/
int64_t convert_to_int64(float a)
{
	return convert_to_int64((double)a);
}

/**
 * i是23位有效数字之有符号整型。即，i in[-0x400000,0x400000]
**/
float convert23_to_float(int32_t i)
{
	i+=0x4B400000;
	return (*(float*)&i)-12582912.0f;
}

/**
 * i是24位有效数字之有符号整型。即，i in[-0x7FFFFF,0x7FFFFF]
**/
float convert24_to_float(int32_t i)
{
	if(i<0)
	{
		i=0xCB000000-i;
		return (*(float*)&i)+8388608.0f;
	}
	else
	{
		i+=0x4B000000;
		return (*(float*)&i)-8388608.0f;
	}
}

/**
 * 标准实现
**/
float convert_to_float(int32_t i)
{
	int32_t n=i>>31;
	float a;
	i=(i^n)-n;
	if((uint32_t)i>8388607)
	{
		int32_t j=(uint32_t)i>>23;
		i&=0x7FFFFF;
		i+=0x4B000000;
		a=(*(float*)&i)-8388608.0f;
		j+=0x56800000;
		a+=(*(float*)&j)-(float)(1LL<<46);
	}
	else
	{
		i+=0x4B000000;
		a=(*(float*)&i)-8388608.0f;
	}
	(*(int32_t*)&a)|=(n&0x80000000);
	return a;
}

/**
 * 标准实现
**/
float convert_to_float(int64_t i)
{
	double convert_to_double(int64_t i);
	return (float)convert_to_double(i);
}

/**
 * i是53位有效数字之有符号整型。即，i in[-0xFFFFFFFFFFFFFLL,0xFFFFFFFFFFFFFLL]
**/
double convert53_to_double(int64_t i)
{
	if(i<0)
	{
		i=0xC330000000000000LL-i;
		return (*(double*)&i)+4503599627370496.0f;
	}
	else
	{
		i+=0x4330000000000000LL;
		return (*(double*)&i)-4503599627370496.0f;
	}
}

/**
 * i是52位有效数字之有符号整型。即，i in[-0x8000000000000LL,0x7FFFFFFFFFFFFLL]
**/
double convert52_to_double(int64_t i)
{
	i+=0x4338000000000000LL;
	return (*(double*)&i)-6755399441055744.0;
}

/**
 * 标准实现
**/
double convert_to_double(int32_t i)
{
	return convert52_to_double((int64_t)i);
}

/**
 * 标准实现
**/
double convert_to_double(int64_t i)
{
	int64_t n=i>>63;
	double a;
	i=(i^n)-n;
	if((uint64_t)i>4503599627370495LL)
	{
		int64_t j=(uint64_t)i>>32;
		i&=0xFFFFFFFF;
		a=convert52_to_double(i);
		j+=0x4530000000000000LL;
		a+=(*(double*)&j)-(double)(1LL<<52)*(double)(1LL<<32);
	}
	else
	{
		i+=0x4330000000000000LL;
		a=(*(double*)&i)-4503599627370496.0;
	}
	(*(int64_t*)&a)|=(n&0x8000000000000000LL);
	return a;
}