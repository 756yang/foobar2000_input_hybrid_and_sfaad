#include "stdafx.h"

#undef _FOOBAR2000_H_
#include "faad_dec.h"
/*
//使用window api
static __time64_t GetFileModifiedTime(const wchar_t *path)
{
	__time64_t m_timestamp;
	struct _stat64i32 statbuf;
	_wstat64i32(path, &statbuf);
	m_timestamp = statbuf.st_mtime * 10000000 + 0x19DB1DED53E8000LL;
	return m_timestamp;
}

//UTF-8到Unicode的转换
int Utf8ToUnicode(const char* utf8, wchar_t* unicode, unsigned int uni_bufsize)
{
	assert(NULL != utf8 && (NULL != unicode || 0 == uni_bufsize));
	int len = MultiByteToWideChar(CP_UTF8, 0, utf8, -1, NULL, 0);
	if (len > uni_bufsize) return -len;
	MultiByteToWideChar(CP_UTF8, 0, utf8, -1, unicode, len);
	return len;
}
*/
static GUID input_get_guid(input_entry::ptr e) {
#ifdef FOOBAR2000_DESKTOP
	input_entry_v2::ptr p;
	if (p &= e) return p->get_guid();
#endif
	return pfc::guid_null;
}


// Note that input class does *not* implement virtual methods or derive from interface classes.
// Our methods get called over input framework templates.
// input_stubs just provides stub implementations of mundane methods that are irrelevant for most implementations.
class input_sfaad : public input_stubs {
public:
	~input_sfaad() {
		if (fdec.exit() != 0) FB2K_console_formatter1() << "faad_decode_exit failure!";
	}
	void open(service_ptr_t<file> p_filehint,const char * p_path,t_input_open_reason p_reason,abort_callback & p_abort) {
		if (p_reason == input_open_info_write) throw exception_io_unsupported_format();
		input_entry::ptr svc;
		get_input_entry(svc, p_path);
		svc->open(info_reader, NULL, p_path, p_abort);
		if (p_reason == input_open_info_read) {
			return;
		}
#ifndef _FOOBAR2000_H_
		if (strncmp(p_path, "file://", 7) != 0) throw exception_io_unsupported_format();
		/* 不能读取到tag，由于m4a文件的tag是id3v2，而其并未实现之，详见read_id3v2_trailing
		service_ptr_t<file> m_file;
		filesystem::g_open(m_file, p_path, filesystem::open_mode_read, p_abort);
		tag_processor::read_id3v2_trailing(m_file, fp_info, p_abort);
		filestat = m_file->get_stats(p_abort);
		m_file->on_idle(p_abort);
		//以下是读取文件状态信息
		wchar_t buf[4096];
		Utf8ToUnicode(p_path + 7, buf, 4096);
		struct _stat64 statbuf;
		_wstat64(buf, &statbuf);
		filestat.m_size = statbuf.st_size;
		filestat.m_timestamp = statbuf.st_mtime * 10000000 + 0x19DB1DED53E8000LL;
		*/
		if (fdec.init(p_path + 7) != 0) {
			FB2K_console_formatter1() << "faad_decode_init failure!";
			throw exception_io_unsupported_format();
		}
		else FB2K_console_formatter1() << "faad_decode_init success!";
#else
		if (fdec.init(p_path, p_abort) != 0) {
			FB2K_console_formatter1() << "faad_decode_init failure!";
			throw exception_io_unsupported_format();
		}
		else FB2K_console_formatter1() << "faad_decode_init success!";
		//filesize = fdec.mp4inf.g_fin->get_size(p_abort);
#endif
		bufsize = 20480 * fdec.channels;
	}

	t_uint32 get_subsong(unsigned p_index) {//input_std的subsong和p_index一致
		return info_reader->get_subsong(p_index);
	};
	unsigned get_subsong_count() {
		return info_reader->get_subsong_count();
	};
	void retag_commit(abort_callback & p_abort) {
		throw exception_io_unsupported_format();
	};
	void retag_set_info(t_uint32 p_subsong, const file_info & p_info, abort_callback & p_abort) {
		throw exception_io_unsupported_format();
	};

	void get_info(t_uint32 p_subsong, file_info & p_info,abort_callback & p_abort) {
		info_reader->get_info(p_subsong, p_info, p_abort);
		p_info.info_set_int("bitspersample", 32);
#if 0
		p_info.set_length(fdec.p_length);
		p_info.info_set_int("samplerate", fdec.samplerate);
		p_info.info_set_int("channels", fdec.channels);
		p_info.info_set("encoding", "lossy");
		p_info.info_set_bitrate(((int64_t)((double)(filestat.m_size * 8)/ fdec.p_length) + 500) / 1000 /* bps to kbps */);
#endif
	}
	t_filestats get_file_stats(abort_callback & p_abort) {
		return info_reader->get_file_stats(p_abort);
	}

	void decode_initialize(t_uint32 p_subsong, unsigned p_flags, abort_callback & p_abort) {
		double p_seconds = 0;
		t_uint32 subsong; int i;
		/*for (int i = 0; i < p_subsong; i++)
		{//decode_initialize的参数p_subsong应该用get_subsong函数获取，所以这是错误的。
			file_info_impl p_out;
			info_reader->get_info(i, p_out, p_abort);
			p_seconds += p_out.get_length();
		}*/
		file_info_impl p_out;
		for (i = 0; (subsong = info_reader->get_subsong(i)) != p_subsong; i++)
		{
			info_reader->get_info(subsong, p_out, p_abort);
			p_seconds += p_out.get_length();
		}
		p_seconds_init = p_seconds;
		info_reader->get_info(subsong, p_out, p_abort);
		p_seconds += p_out.get_length();
		fdec.endSamples = audio_math::time_to_samples(p_seconds, fdec.mp4inf.mp4config.samplerate);
#ifndef _FOOBAR2000_H_
		if (fdec.seek(audio_math::time_to_samples(p_seconds_init, fdec.mp4inf.mp4config.samplerate)) != 0) {
#else
		if (fdec.seek(audio_math::time_to_samples(p_seconds_init, fdec.mp4inf.mp4config.samplerate), p_abort) != 0) {
#endif
			FB2K_console_formatter1() << "faad_decode_reinit failure!";
			throw exception_io_unsupported_format();
		}
	}
	bool decode_run(audio_chunk & p_chunk,abort_callback & p_abort) {
		float *out_data, *end_data;
		unsigned int count;
		audio_sample *m_data,*e_data;
		p_chunk.allocate(bufsize + fdec.framesize * fdec.channels);
		m_data = p_chunk.get_data();
		e_data = m_data + bufsize;
		do {
#ifndef _FOOBAR2000_H_
			if (fdec.run(&out_data, &count) != 0) {
#else
			if (fdec.run(&out_data, &count, p_abort) != 0) {
#endif
				break;
			}
#if 1
			end_data = out_data + count;
			while (out_data != end_data) {//注意-0.0!=0.0，如果out_data指向-0.0，需转为0
				if (*(int*)out_data == 0x80000000) *(int*)m_data = 0;
				else *m_data = *out_data;
				m_data++; out_data++;
			}
#else
			memcpy(m_data, out_data, count * sizeof(float));
			m_data += count;
#endif
		} while (m_data < e_data);
		count = m_data - p_chunk.get_data();
		if (count == 0)return false;
		p_chunk.set_sample_count(count/ fdec.channels);
		p_chunk.set_channels(fdec.channels);
		p_chunk.set_sample_rate(fdec.samplerate);
		return true;
	}
	void decode_seek(double p_seconds,abort_callback & p_abort) {
#ifndef _FOOBAR2000_H_
		if (fdec.seek(audio_math::time_to_samples(p_seconds_init + p_seconds, fdec.mp4inf.mp4config.samplerate)) != 0) {
#else
		if (fdec.seek(audio_math::time_to_samples(p_seconds_init + p_seconds, fdec.mp4inf.mp4config.samplerate), p_abort) != 0) {
#endif
			FB2K_console_formatter1() << "faad_decode_seek failure!";
			throw exception_io_unsupported_format();
		}
	}
	bool decode_can_seek() {
#ifndef _FOOBAR2000_H_
		return true;
#else
		return fdec.mp4inf.g_fin->can_seek();
#endif
	}
	bool decode_get_dynamic_info(file_info & p_out, double & p_timestamp_delta) { // deals with dynamic information such as VBR bitrates
		unsigned int bufsize;
		if (fdec.mp4inf.mp4config.frame.current >= fdec.mp4inf.mp4config.frame.ents)
			bufsize = fdec.mp4inf.mp4config.frame.data[fdec.mp4inf.mp4config.frame.ents]
			- fdec.mp4inf.mp4config.frame.data[fdec.mp4inf.mp4config.frame.ents - 1];
		else bufsize= fdec.mp4inf.mp4config.frame.data[fdec.mp4inf.mp4config.frame.current + 1]
			- fdec.mp4inf.mp4config.frame.data[fdec.mp4inf.mp4config.frame.current];
		p_out.info_set_bitrate_vbr(((bufsize << 3) * fdec.mp4inf.mp4config.samplerate + 500 * fdec.framesize) / (1000 * fdec.framesize));
		return true;
	}
	bool decode_get_dynamic_info_track(file_info & p_out, double & p_timestamp_delta) { return false; } // deals with dynamic information such as track changes in live streams
	void decode_on_idle(abort_callback & p_abort) {//其实什么也不用做，这是线程进入睡眠前调用的函数，不是解码退出时调用
#ifdef _FOOBAR2000_H_
		fdec.mp4inf.g_fin->on_idle(p_abort);
#endif
	}

	static bool g_is_our_content_type(const char * p_content_type) {return false;} // match against supported mime types here
	static bool g_is_our_path(const char * p_path,const char * p_extension) {
		if (stricmp_utf8(p_extension, "m4a") == 0)return true;
		if (stricmp_utf8(p_extension, "mp4") == 0)return true;
		if (stricmp_utf8(p_extension, "m4b") == 0)return true;
		return false;
	}
	static const char * g_get_name() { return "foo_sfaad input"; }
	static const GUID g_get_guid() {
		// GUID of the decoder. Replace with your own when reusing code.
		static const GUID I_am_foo_sfaad_and_this_is_my_decoder_GUID = { 0xa159bfd0, 0x5cb6, 0x761f,{ 0xac, 0x2c, 0xd, 0x50, 0xf8, 0x61, 0xb9, 0xb7 } };
		return I_am_foo_sfaad_and_this_is_my_decoder_GUID;
	}
	static void get_input_entry(input_entry::ptr &svc,const char * p_path) {
		static const GUID I_am_foo_hybrid_and_this_is_my_decoder_GUID = { 0xd9521cad, 0xc9c5, 0x4e1c,{ 0xaa, 0x1c, 0xfd, 0x54, 0xe8, 0x60, 0xb0, 0xb } };
		auto ext = pfc::string_extension(p_path);
		service_enum_t<input_entry> e;
		while (e.next(svc)) {//for seek faad2 decoder
			if (svc->is_our_path(p_path, ext)) {
				GUID p_guid = input_get_guid(svc);
				if (!IsEqualGUID(p_guid, g_get_guid()) && !IsEqualGUID(p_guid, I_am_foo_hybrid_and_this_is_my_decoder_GUID))
					return;
			}
		}
		throw exception_io_unsupported_format();
	}

public:
	service_ptr_t<input_info_reader> info_reader;
	double p_seconds_init;
	faad_dec fdec;
	size_t bufsize;
};


static input_factory_t<input_sfaad> g_input_sfaad_factory;

// Declare file_extension as a supported file type to make it show in "open file" dialog etc.
DECLARE_FILE_TYPE("MP4 AAC files", "*.m4a;*.mp4;*.m4b");

// Declaration of your component's version information
DECLARE_COMPONENT_VERSION("sfaad decoder", "2.0", "This is a sfaad decoder for foobar2000.");


// This will prevent users from renaming your component around (important for proper troubleshooter behaviors) or loading multiple instances of it.
VALIDATE_COMPONENT_FILENAME("foo_input_sfaad.dll");