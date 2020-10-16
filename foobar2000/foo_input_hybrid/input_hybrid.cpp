#include "stdafx.h"
#include "pcm_convert.h"

//请启用SSE指令集

#define MAIN_AUDIO_EXTENSION "m4a"
#define ADDITION_AUDIO_EXTENSION "tak"

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
class input_hybrid : public input_stubs {
public:
	void open(service_ptr_t<file> p_filehint,const char * p_path,t_input_open_reason p_reason,abort_callback & p_abort) {
		// for this case, only FOOBAR2000_TARGET_VERSION == 79 can walk all input_entry, else throw exception for walking input_entry only once!
		input_entry::ptr svc;
		pfc::string_replace_extension p_path_add(p_path, ADDITION_AUDIO_EXTENSION);
		is_hybrid_add = foobar2000_io::filesystem::g_exists(p_path_add, p_abort);
		switch (p_reason)
		{
		case input_open_info_read:
			//input_entry::g_open_for_info_read(info_reader, p_filehint, p_path, p_abort, true);//our implementation is redirect.
			get_input_entry(svc, p_path);
			svc->open(info_reader, p_filehint, p_path, p_abort);
			if (is_hybrid_add) {
				input_entry::g_open_for_info_read(info_reader_add, NULL, p_path_add, p_abort, true);
			}
			break;
		case input_open_decode:
			//input_entry::g_open_for_decoding(input, p_filehint, p_path, p_abort, true);//our implementation is redirect.
			if (is_hybrid_add) {
				get_sfaad_input(svc, p_path);
				FB2K_console_formatter1() << "open addition decoder!";
				input_entry::g_open_for_decoding(input_add, NULL, p_path_add, p_abort, true);
				info_reader_add = input_add;
			}
			else{
				get_input_entry(svc, p_path);
			}
			svc->open(input, p_filehint, p_path, p_abort);
			info_reader = input;
			break;
		case input_open_info_write:
			//input_entry::g_open_for_info_write(info_writer, p_filehint, p_path, p_abort, true);//our implementation is redirect.
			get_input_entry(svc, p_path);
			svc->open(info_writer, p_filehint, p_path, p_abort);
			info_reader = info_writer;
			is_hybrid_add = false;//必须置为false
			break;
		default:
			break;
		}
	}

	t_uint32 get_subsong(unsigned p_index) { return info_reader->get_subsong(p_index); };
	unsigned get_subsong_count() { return info_reader->get_subsong_count(); };
	void retag_commit(abort_callback & p_abort) {
		info_writer->commit(p_abort);
	};
	void retag_set_info(t_uint32 p_subsong, const file_info & p_info, abort_callback & p_abort) {
		info_writer->set_info(p_subsong, p_info, p_abort);
	};

	void get_info(t_uint32 p_subsong,file_info & p_info,abort_callback & p_abort) {
		info_reader->get_info(p_subsong, p_info, p_abort);
		if (is_hybrid_add) {
			file_info_impl p_info_add;
			info_reader_add->get_info(info_reader_add->get_subsong(0), p_info_add, p_abort);
			bitrate_add = p_info_add.info_get_bitrate();
			t_int64 temp_bitrate = p_info.info_get_bitrate() + bitrate_add;
			p_info.info_set_bitrate(temp_bitrate);
			bit_per_sample=p_info_add.info_get_decoded_bps();
			p_info.info_set_int("bitspersample", bit_per_sample);
			if (p_info.info_exists("decoded_bitspersample")) {
				p_info.info_set_int("decoded_bitspersample", bit_per_sample);
			}
			p_info.info_set("encoding", "hybrid");
#if 0
			switch (bit_per_sample)
			{
			case 8:
				FB2K_console_formatter1() << "this is a 8 bit unsigned audio!";
				break;
			case 16:
				FB2K_console_formatter1() << "this is a 16 bit signed audio!";
				break;
			case 24:
				FB2K_console_formatter1() << "this is a 24 bit signed audio!";
				break;
			case 32:
				FB2K_console_formatter1() << "this is a 32 bit float audio!";
				break;
			default:
				FB2K_console_formatter1() << "can\'t get audio bit_per_sample!";
				break;
			}
			FB2K_console_formatter() << p_info.info_get_bitrate();
			FB2K_console_formatter() << p_info.info_get_decoded_bps();
			FB2K_console_formatter() << p_info.info_get("encoding");
			FB2K_console_formatter() << p_info.info_get_int("decoded_bitspersample");
			FB2K_console_formatter() << p_info.info_get_int("bitspersample");
#endif
		}
	}
	t_filestats get_file_stats(abort_callback & p_abort) {return info_reader->get_file_stats(p_abort);}

	void decode_initialize(t_uint32 p_subsong, unsigned p_flags, abort_callback & p_abort) {
		input->initialize(p_subsong, p_flags, p_abort);
		if (is_hybrid_add) {
			input_add->initialize(input_add->get_subsong(0), p_flags, p_abort);
			file_info_impl p_out;
			input_add->get_info(input_add->get_subsong(0), p_out, p_abort);
			bit_per_sample = p_out.info_get_decoded_bps();
			double p_seconds = 0;
			t_uint32 subsong;
			for (int i = 0; (subsong = info_reader->get_subsong(i)) != p_subsong; i++)
			{
				info_reader->get_info(subsong, p_out, p_abort);
				p_seconds += p_out.get_length();
			}
			input_add->seek(p_seconds, p_abort);
			p_seconds_init = p_seconds;
			p_start = p_end = 0;
		}
	}
	bool decode_run(audio_chunk & p_chunk,abort_callback & p_abort) {
		if (!(input->run(p_chunk, p_abort)))return false;
		if (is_hybrid_add) {
			unsigned int chunk_size = p_chunk.get_channel_count()*p_chunk.get_sample_count();
			audio_sample *m_data = p_chunk.get_data();
			switch (bit_per_sample)
			{
			case 8:
				pcmfloat_scale_to_int8(m_data, chunk_size);
				break;
			case 16:
				pcmfloat_scale_to_int16(m_data, chunk_size);
				break;
			case 24:
				pcmfloat_scale_to_int24(m_data, chunk_size);
				break;
			default:
				break;
			}
			while (chunk_size > p_end - p_start){
				while (p_end != p_start) {
					*(m_data++) += chunk_add[p_start++];
					chunk_size--;
				}
				p_start = 0;
				audio_chunk_impl a_chunk;
				if (!(input_add->run(a_chunk, p_abort))) {
					FB2K_console_formatter1() << "input_add->run() failure in foo_hybird audio input!";
					p_chunk.set_sample_count(p_chunk.get_sample_count() - chunk_size / p_chunk.get_channel_count());
					while (input->run(a_chunk, p_abort));
					return true;
				}
				p_end = a_chunk.get_channel_count()*a_chunk.get_sample_count();
				chunk_add.set_size_discard(p_end);
				chunk_add.set_data_fromptr(a_chunk.get_data(), p_end);
			}
			while (chunk_size != 0) {
				*(m_data++) += chunk_add[p_start++];
				chunk_size--;
			}
		}
		return true;
	}
	void decode_seek(double p_seconds,abort_callback & p_abort) {
		input->seek(p_seconds, p_abort);
		if (is_hybrid_add){
			input_add->seek(p_seconds_init + p_seconds, p_abort);
		}
	}
	bool decode_can_seek() {
		if (is_hybrid_add) {
			if (!(input_add->can_seek()))return false;
		}
		return input->can_seek();
	}
	bool decode_get_dynamic_info(file_info & p_out, double & p_timestamp_delta) { // deals with dynamic information such as VBR bitrates
		if (!(input->get_dynamic_info(p_out, p_timestamp_delta)))return false;
		if (is_hybrid_add) {
			file_info_impl p_out_add;
			double p_seconds;
			t_int64 bitrate_out;
			if (input_add->get_dynamic_info(p_out_add, p_seconds))bitrate_out = p_out_add.info_get_bitrate_vbr();
			else bitrate_out = bitrate_add;
			bitrate_out += p_out.info_get_bitrate_vbr();
			p_out.info_set_bitrate_vbr(bitrate_out);
		}
		return true;
	}
	bool decode_get_dynamic_info_track(file_info & p_out, double & p_timestamp_delta) { return input->get_dynamic_info_track(p_out, p_timestamp_delta); } // deals with dynamic information such as track changes in live streams
	void decode_on_idle(abort_callback & p_abort) {
		input->on_idle(p_abort);
		if (is_hybrid_add)input_add->on_idle(p_abort);
	}

	static bool g_is_our_content_type(const char * p_content_type) {return false;} // match against supported mime types here
	static bool g_is_our_path(const char * p_path,const char * p_extension) {
		return stricmp_utf8(p_extension,MAIN_AUDIO_EXTENSION) == 0;}
	static const char * g_get_name() { return "foo_hybrid audio input " MAIN_AUDIO_EXTENSION ":" ADDITION_AUDIO_EXTENSION ":sfaad"; }
	static const GUID g_get_guid() {
		// GUID of the decoder. Replace with your own when reusing code.
		static const GUID I_am_foo_hybrid_and_this_is_my_decoder_GUID = { 0xd9521cad, 0xc9c5, 0x4e1c,{ 0xaa, 0x1c, 0xfd, 0x54, 0xe8, 0x60, 0xb0, 0xb } };
		return I_am_foo_hybrid_and_this_is_my_decoder_GUID;
	}
	static void get_sfaad_input(input_entry::ptr &svc, const char * p_path) {
		static const GUID I_am_foo_sfaad_and_this_is_my_decoder_GUID = { 0xa159bfd0, 0x5cb6, 0x761f,{ 0xac, 0x2c, 0xd, 0x50, 0xf8, 0x61, 0xb9, 0xb7 } };
		auto ext = pfc::string_extension(p_path);
		service_enum_t<input_entry> e;
		while (e.next(svc)) {//for seek faad2 decoder
			if (IsEqualGUID(input_get_guid(svc), I_am_foo_sfaad_and_this_is_my_decoder_GUID))
				return;
		}
		throw exception_io_unsupported_format();
	}
	static void get_input_entry(input_entry::ptr &svc, const char * p_path) {
		static const GUID I_am_foo_sfaad_and_this_is_my_decoder_GUID = { 0xa159bfd0, 0x5cb6, 0x761f, { 0xac, 0x2c, 0xd, 0x50, 0xf8, 0x61, 0xb9, 0xb7 } };
		auto ext = pfc::string_extension(p_path);
		service_enum_t<input_entry> e;
		while (e.next(svc)) {//for seek faad2 decoder
			if (svc->is_our_path(p_path, ext)) {
				GUID p_guid = input_get_guid(svc);
				if (!IsEqualGUID(p_guid, g_get_guid()) && !IsEqualGUID(p_guid, I_am_foo_sfaad_and_this_is_my_decoder_GUID))
					return;
			}
		}
		throw exception_io_unsupported_format();
	}

public:
	service_ptr_t<input_decoder> input;
	service_ptr_t<input_info_reader> info_reader;
	service_ptr_t<input_info_writer> info_writer;
	bool is_hybrid_add;
	double p_seconds_init;
	t_int64 bitrate_add;
	unsigned int bit_per_sample;
	service_ptr_t<input_decoder> input_add;
	service_ptr_t<input_info_reader> info_reader_add;
	unsigned int p_start;
	unsigned int p_end;
	pfc::array_t<audio_sample> chunk_add;
};

//static input_factory_t<input_hybird, input_entry::flag_redirect> g_input_hybird_factory;//for that cue_parser is redirect so it will not call our decoder by flag_redirect.
static input_factory_t<input_hybrid> g_input_hybrid_factory;

// Declare file_extension as a supported file type to make it show in "open file" dialog etc.
DECLARE_FILE_TYPE(MAIN_AUDIO_EXTENSION " files", "*." MAIN_AUDIO_EXTENSION);

// Declaration of your component's version information
DECLARE_COMPONENT_VERSION("Input Hybrid Wrapper", "1.4", "This is a decoder wrapper created to balance lossy and lossless music. Lossy music is close to lossless and has a smaller file size, so it is almost indistinguishable from the spectrum. Lossless music can save better music information, better sound quality, but the file size is huge. Sometimes, we need both lossless and lossy music. Lossy music is suitable for playing on mobile devices, and lossless music is suitable for playing on desktop devices. It is very necessary to store lossy and lossless music at the same time. We find that lossless audio can be compressed smaller after removing the lossy part. Compared with direct compression, lossless audio consumes about 5% more disk space, and lossy music accounts for about 30%. By using two compression methods can save a lot of disk space, and this component is to restore the original audio through the decoder wrapper whit additional lossless audio.");


// This will prevent users from renaming your component around (important for proper troubleshooter behaviors) or loading multiple instances of it.
VALIDATE_COMPONENT_FILENAME("foo_input_hybrid.dll");