#include "pcm_convert.h"
#include "typeconvert.h"

//需要SSE指令集编译，否则出错！
void pcmfloat_scale_to_int8(audio_sample * p_float, size_t p_count) {
	audio_sample *p_end = p_float + p_count;
	while (p_float != p_end) {
		audio_sample f = *p_float + 98304.0f;
		*(p_float++) = f - 98304.0f;
	}
}

void pcmfloat_scale_to_int16(audio_sample * p_float, size_t p_count) {
	audio_sample *p_end = p_float + p_count;
	while (p_float != p_end) {
		audio_sample f = *p_float + 384.0f;
		*(p_float++) = f - 384.0f;
	}
}

void pcmfloat_scale_to_int24(audio_sample * p_float, size_t p_count) {
	audio_sample *p_end = p_float + p_count;
	while (p_float != p_end) {
		double d = (double)*p_float + 805306368.0;
		*(p_float++) = (float)(d - 805306368.0);
	}
}

/*
__declspec(naked) void __fastcall pcmfloat_scale_to_int24(float *p_float)
{//内嵌汇编不能inline
	_asm {
		mov eax, DWORD PTR[ecx];
		xor edx, edx;
		shl eax, 1;
		setb dl;
		cmp eax, 0x7F000000;
		jae L1;
		movss xmm1, DWORD PTR c[edx * 4];
		movss xmm0, DWORD PTR[ecx];
		addss xmm0, xmm1;
		subss xmm0, xmm1;
		movss DWORD PTR[ecx], xmm0;
	L1:
		ret;
	}
}

const float c[2] = { 1.0f,-1.0f };
__declspec(naked) void __fastcall pcmfloat_scale_to_int24(audio_sample * p_float, size_t p_count) {//这是错误的做法，如果2>p_float>1，可能出错，这时需要舍入最後一位
	_asm {
		push ebx;
		lea ebx, [ecx + edx * 4];
		xor edx, edx;
		cmp ecx, ebx;
		je L2;
	L0:
		mov eax, DWORD PTR[ecx];
		shl eax, 1;
		setb dl;
		cmp eax, 0x7F000000;
		jae L1;//大于1直接跳转，这是错误的
		movss xmm1, DWORD PTR c[edx * 4];
		movss xmm0, DWORD PTR[ecx];
		addss xmm0, xmm1;
		subss xmm0, xmm1;
		movss DWORD PTR[ecx], xmm0;
	L1:
		add ecx, 4;
		cmp ecx, ebx;
		jne L0;
	L2:
		pop ebx;
		ret;
	}
}
*/
void convert_from_uint8(uint8_t * p_source, size_t p_count, audio_sample * p_output)
{
	while (p_count != 0) {
		*(p_output++) = ((float)(int8_t)(*(p_source++) ^ 0x80))/0x80;
		p_count--;
	}
}

void convert_from_int16(int16_t * p_source, size_t p_count, audio_sample * p_output)
{
	while (p_count != 0) {
		*(p_output++) = ((float)(*(p_source++)))/0x8000;
		p_count--;
	}
}

void convert_from_int32(int32_t * p_source, size_t p_count, audio_sample * p_output, audio_sample scale)
{
	while (p_count != 0) {
		*(p_output++) = ((float)(*(p_source++))) / scale;
		p_count--;
	}
}


void convert_to_uint8(audio_sample * p_source, size_t p_count, uint8_t * p_output)
{
	int32_t i;
	float f;
	while (p_count != 0) {
		f = *(p_source++) * 0x80;
#ifdef _NO__GNUC__NO_MSC_VER
		i = convert_to_int23(f);
#else
		FROUND(i,f);
#endif
		PCMLIMITB(i, 8);
		*(p_output++) = (uint8_t)(i ^ 0x80);
		p_count--;
	}
}

void convert_to_int16(audio_sample * p_source, size_t p_count, int16_t * p_output)
{
	int32_t i;
	float f;
	while (p_count != 0) {
		f = *(p_source++) * 0x8000;
#ifdef _NO__GNUC__NO_MSC_VER
		i = convert_to_int23(f);
#else
		FROUND(i,f);
#endif
		PCMLIMITB(i, 16);
		*(p_output++) = (int16_t)i;
		p_count--;
	}
}

void convert_to_int32(audio_sample * p_source, size_t p_count, int32_t * p_output)
{
	int32_t i;
	float f;
	while (p_count != 0) {
		f = *(p_source++) * 0x800000;
#ifdef _NO__GNUC__NO_MSC_VER
		if (f > (float)0x7FFFFF)i = 0x7FFFFF;
		else if (f < -(float)0x7FFFFF)i = -0x800000;
		else i = convert_to_int24(f);
#else
		FROUND(i, f);
		PCMLIMITB(i, 24);
		if (i > 0x7FFFFF)i = 0x7FFFFF;
		if (i < -0x800000)i = -0x800000;
#endif
		*(p_output++) = i;
		p_count--;
	}
}

void convert_to_int32(audio_sample * p_source, size_t p_count, int32_t * p_output, int32_t scale)
{
	int32_t i;
	float f;
	while (p_count != 0) {
		f = *(p_source++) * scale;
		FROUND(i,f);
		if (i > scale - 1)i = scale - 1;
		if (i < -scale)i = -scale;
		*(p_output++) = i;
		p_count--;
	}
}
