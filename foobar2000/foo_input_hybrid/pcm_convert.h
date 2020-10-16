#include <stdint.h>

//需要SSE指令集编译，否则出错！
//FTURN(f,i)不使用，可以直接强制类型转换而不出现问题
#if defined(__GNUC__)
//g++,gcc
#define FROUND(i,f) \
    asm(\
		"cvtss2sil %1,%0 \n\t"\
		:"=r"(i)\
		:"x"(f)\
	)
#define FTURN(f,i) \
    asm(\
		"cvtsi2ssl %1,%0 \n\t"\
		:"+x"(f)\
		:"r"(i)\
	)
#elif defined(_MSC_VER)
//msvc++
#include <xmmintrin.h>
#define FROUND(i,f) ((i)=_mm_cvtss_si32(_mm_load_ss(&f)))
#define FTURN(f,i) ((f)=_mm_cvtss_f32(_mm_cvt_si2ss(_mm_load_ss(&f),i)))
#else
//其他
#define _NO__GNUC__NO_MSC_VER
#define FROUND(i,f) ((i)=convert_to_int32(f))
#define FTURN(f,i) ((f)=convert_to_float(i))
#endif

#define PCMLIMIT(i,s) if ((i) > (s)-1) (i) = (s)-1;\
	else if ((i) < -(s)) (i) = -(s);
#define PCMLIMITB(i,b) if ((i) > (1<<((b)-1))-1) (i) = (1<<((b)-1))-1;\
	else if ((i) < -(1<<((b)-1))) (i) = -(1<<((b)-1));

typedef float audio_sample;


/**
 * 将PCM float格式数据截断为只有PCM 8位精度的数据
**/
void pcmfloat_scale_to_int8(audio_sample * p_float, size_t p_count);

/**
 * 将PCM float格式数据截断为只有PCM 16位精度的数据
**/
void pcmfloat_scale_to_int16(audio_sample * p_float, size_t p_count);

/**
 * 将PCM float格式数据截断为只有PCM 24位精度的数据
**/
void pcmfloat_scale_to_int24(audio_sample * p_float, size_t p_count);


/**
 * 将PCM 8位无符号数据p_source转换为PCM 32位单精度浮点数据p_output
**/
void convert_from_uint8(uint8_t * p_source, size_t p_count, audio_sample * p_output); 

/**
 * 将PCM 16位有符号数据p_source转换为PCM 32位单精度浮点数据p_output
**/
void convert_from_int16(int16_t * p_source, size_t p_count, audio_sample * p_output);

/**
 * 将（scale == 0x800000 时）PCM 24位有符号数据p_source转换为PCM 32位单精度浮点数据p_output
**/
void convert_from_int32(int32_t * p_source, size_t p_count, audio_sample * p_output, audio_sample scale = 0x800000);


/**
 * 将PCM 32位单精度浮点数据p_source转换为PCM 8位无符号数据p_output
**/
void convert_to_uint8(audio_sample * p_source, size_t p_count, uint8_t * p_output);

/**
 * 将PCM 32位单精度浮点数据p_source转换为PCM 16位有符号数据p_output
**/
void convert_to_int16(audio_sample * p_source, size_t p_count, int16_t * p_output);

/**
 * 将PCM 32位单精度浮点数据p_source转换为PCM 24位有符号数据p_output
**/
void convert_to_int32(audio_sample * p_source, size_t p_count, int32_t * p_output);

/**
 * 将（scale == 0x800000 时）PCM 32位单精度浮点数据p_source转换为PCM 24位有符号数据p_output
**/
void convert_to_int32(audio_sample * p_source, size_t p_count, int32_t * p_output, int32_t scale);