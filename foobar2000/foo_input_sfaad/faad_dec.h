#ifndef __FAAD_DEC_H__
#define __FAAD_DEC_H__

#include <windows.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include "neaacdec.h"
#include "mp4read.h"


class faad_dec {//析构前请调用exit函数释放资源
public:
	int exit();
#ifndef _FOOBAR2000_H_
	//只有使用C的文件流才需要析构FILE指针
	int init(const char *mp4file);
	int seek(uint64_t samplesPos);
	int run(float **out_data, unsigned int *count);
#else
	//不需要额外的析构，使用foobar2000的file::ptr会自动析构之
	int init(const char *mp4file, abort_callback & p_abort);
	int seek(uint64_t samplesPos, abort_callback & p_abort);
	int run(float **out_data, unsigned int *count, abort_callback & p_abort);
#endif
public:
	unsigned long samplerate;
	unsigned char channels;
	long sampleId, remSamples;
	double p_length;
	NeAACDecHandle hDecoder = NULL;
	NeAACDecConfigurationPtr config;
	NeAACDecFrameInfo frameInfo;
	mp4AudioSpecificConfig mp4ASC;

	/* for gapless decoding */
	unsigned int framesize;
	uint64_t decoded;//decoded已解码采样数
	uint64_t endSamples;//最多解码的采样数
	mp4read mp4inf;
};



#endif