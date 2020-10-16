#ifndef __FAAD_DEC_H__
#define __FAAD_DEC_H__

#include <windows.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include "neaacdec.h"
#include "mp4read.h"


class faad_dec {//����ǰ�����exit�����ͷ���Դ
public:
	int exit();
#ifndef _FOOBAR2000_H_
	//ֻ��ʹ��C���ļ�������Ҫ����FILEָ��
	int init(const char *mp4file);
	int seek(uint64_t samplesPos);
	int run(float **out_data, unsigned int *count);
#else
	//����Ҫ�����������ʹ��foobar2000��file::ptr���Զ�����֮
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
	uint64_t decoded;//decoded�ѽ��������
	uint64_t endSamples;//������Ĳ�����
	mp4read mp4inf;
};



#endif