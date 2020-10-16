//#include "stdafx.h"
//请注意预编译头文件包含，其中_FOOBAR2000_H_决定了本文件的实现
#include "faad_dec.h"

#ifndef _FOOBAR2000_H_
int faad_dec::init(const char *mp4file) {
#else
int faad_dec::init(const char *mp4file, abort_callback & p_abort) {
#endif
	if (mp4inf.mp4read_open(mp4file
#ifdef _FOOBAR2000_H_
		,p_abort
#endif
	))
	{
		/* unable to open file */
		return 1;
	}

	hDecoder = NeAACDecOpen();

	/* Set configuration */
	config = NeAACDecGetCurrentConfiguration(hDecoder);
	config->defObjectType = LC;
	config->defSampleRate = mp4inf.mp4config.samplerate;
	config->useOldADTSFormat = 0;
	config->outputFormat = FAAD_FMT_FLOAT;
	config->downMatrix = 0;//不用混合声道为双声道
	config->dontUpSampleImplicitSBR = 0;
	NeAACDecSetConfiguration(hDecoder, config);

	if (NeAACDecInit2(hDecoder, mp4inf.mp4config.asc.buf, mp4inf.mp4config.asc.size,
		&samplerate, &channels) < 0)
	{
		/* If some error initializing occured, skip the file */
		NeAACDecClose(hDecoder);
		mp4inf.mp4read_close(
#ifdef _FOOBAR2000_H_
			p_abort
#endif
		);
		return 1;
	}

	framesize = 1024;
	decoded = 0;

	if (mp4inf.mp4config.asc.size)
	{
		if (NeAACDecAudioSpecificConfig(mp4inf.mp4config.asc.buf, mp4inf.mp4config.asc.size, &mp4ASC) >= 0)
		{
			if (mp4ASC.frameLengthFlag == 1) framesize = 960;
			if (mp4ASC.sbr_present_flag == 1) framesize *= 2;
		}
	}
	p_length = (double)mp4inf.mp4config.samples / (double)mp4ASC.samplingFrequency;
	mp4inf.mp4read_seek(0
#ifdef _FOOBAR2000_H_
		, p_abort
#endif
	);
	remSamples = 0;
	sampleId = 0;
	endSamples = mp4inf.mp4config.samples;
	memset(&frameInfo, 0, sizeof(NeAACDecFrameInfo));
	return 0;
}

#ifndef _FOOBAR2000_H_
int faad_dec::seek(uint64_t samplesPos) {
#else
int faad_dec::seek(uint64_t samplesPos, abort_callback & p_abort) {
#endif
	decoded -= sampleId * framesize;
	if (samplesPos > endSamples) return -1;
	sampleId = (int)(samplesPos / framesize);
	remSamples = (int)(samplesPos % framesize) - decoded;
	decoded = samplesPos - remSamples;
	mp4inf.mp4read_seek(sampleId
#ifdef _FOOBAR2000_H_
		, p_abort
#endif
	);
	return 0;
}

#ifndef _FOOBAR2000_H_
int faad_dec::run(float **out_data, unsigned int *count) {
#else
int faad_dec::run(float **out_data, unsigned int *count, abort_callback & p_abort) {
#endif
	if (sampleId >= mp4inf.mp4config.frame.ents || decoded >= endSamples)
		return -1;
	/*int rc;*/
	long dur;
	unsigned int sample_count;
	unsigned int delay = 0;

	if (mp4inf.mp4read_frame(
#ifdef _FOOBAR2000_H_
		p_abort
#endif
	))
		return -1;

	void *sample_buffer = NeAACDecDecode(hDecoder, &frameInfo, mp4inf.mp4config.bitbuf.data, mp4inf.mp4config.bitbuf.size);
	NeAACDecGetCurrentConfiguration(hDecoder);//这个函数修改了hDecoder->__r1,__r2重置为1，以解决不确定性
	if (!sample_buffer) {
		/* unable to decode file, abort */
		return -1;
	}

	dur = frameInfo.samples / frameInfo.channels;
	decoded += dur;

	if (decoded > endSamples)
		dur -= (decoded - endSamples);

	if (dur > framesize)
	{
		dur = framesize;
	}

	if (mp4inf.mp4config.samplerate != samplerate) {
		sample_count = frameInfo.samples;
	}
	else {
		sample_count = (unsigned int)(dur * frameInfo.channels);
		if (sample_count > frameInfo.samples)
			sample_count = frameInfo.samples;
	}

	*out_data = (float*)sample_buffer + (remSamples * frameInfo.channels);
	
	if (frameInfo.error > 0)
	{
		NeAACDecGetErrorMessage(frameInfo.error);
		sample_count = 0;
	}
	else {
		if (sample_count >= remSamples * frameInfo.channels) {
			sample_count -= remSamples * frameInfo.channels;
			remSamples = 0;
		}
		else {
			remSamples -= sample_count / frameInfo.channels;
			sample_count = 0;
		}
	}
	*count = sample_count;
	sampleId++;
	return 0;
}

int faad_dec::exit() {
	if (hDecoder != NULL) {
		NeAACDecClose(hDecoder);
		hDecoder = NULL;
	}
	mp4inf.mp4read_close();
	return frameInfo.error;
}


#if 0
#include "audio.h"
#include "unicode_support.h"

static int faad_main(int argc, char *argv[])
{
	float *out_data;
	unsigned int count;
	faad_dec dec;
	double p_length;
	DWORD begin = GetTickCount();
	dec.init(argv[1]);
	audio_file *aufile = open_audio_file(argv[2], dec.samplerate, dec.channels, FAAD_FMT_FLOAT, 1, 0);
	while (dec.run(&out_data, &count) == 0) {
		if (count != 0) write_audio_file(aufile, out_data, count, 0);
	}
	dec.exit();
	close_audio_file(aufile);
	printf("SFAAD decode cost %f sec......\n", (GetTickCount() - begin) / 1000.0f);
	return 0;
}

int main(int argc, char *argv[])
{
#if defined WIN32 || defined _WIN32 || defined WIN64 || defined _WIN64
	int argc_utf8, exit_code;
	char **argv_utf8;
	init_console_utf8(stderr);
	init_commandline_arguments_utf8(&argc_utf8, &argv_utf8);
	exit_code = faad_main(argc_utf8, argv_utf8);
	free_commandline_arguments_utf8(&argc_utf8, &argv_utf8);
	uninit_console_utf8();
	return exit_code;
#else
	return faad_main(argc, argv);
#endif
}
#endif