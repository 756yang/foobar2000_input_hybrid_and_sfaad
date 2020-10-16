/****************************************************************************
    MP4 input module

    Copyright (C) 2017 Krzysztof Nikiel
	Rewrite by Yang Yujun
	implement a c++ mp4read and suitable for foobar2000.

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
****************************************************************************/

//#include "stdafx.h"
//请注意预编译头文件包含，其中_FOOBAR2000_H_决定了本文件的实现
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <limits.h>

#include "unicode_support.h"
#include "mp4read.h"

enum ATOM_TYPE
{
    ATOM_STOP = 0 /* end of atoms */ ,
    ATOM_NAME /* plain atom */ ,
    ATOM_DESCENT,               /* starts group of children */
    ATOM_ASCENT,                /* ends group */
    ATOM_DATA,
};

static inline uint32_t bswap32(const uint32_t u32)
{
#ifndef WORDS_BIGENDIAN
#if defined (__GNUC__) && ((__GNUC__ > 4) || ((__GNUC__ == 4) && (__GNUC_MINOR__ >= 3)))
    return __builtin_bswap32(u32);
#elif defined (_MSC_VER)
    return _byteswap_ulong(u32);
#else
    return (u32 << 24) | ((u32 << 8) & 0xFF0000) | ((u32 >> 8) & 0xFF00) | (u32 >> 24);
#endif
#else
    return u32;
#endif
}

static inline uint16_t bswap16(const uint16_t u16)
{
#ifndef WORDS_BIGENDIAN
#if defined (__GNUC__) && ((__GNUC__ > 4) || ((__GNUC__ == 4) && (__GNUC_MINOR__ >= 8)))
    return __builtin_bswap16(u16);
#elif defined (_MSC_VER)
    return _byteswap_ushort(u16);
#else
    return (u16 << 8) | (u16 >> 8);
#endif
#else
    return u16;
#endif
}

enum {ERR_OK = 0, ERR_FAIL = -1, ERR_UNSUPPORTED = -2};

#ifndef _FOOBAR2000_H_
inline int datain(void *data, int size, FILE *g_fin)
#else
inline int datain(void *data, int size, service_ptr_t<file> &g_fin, abort_callback & p_abort)
#endif
{
    if (
#ifndef _FOOBAR2000_H_
		fread(data, 1, size, g_fin)
#else
		g_fin->read(data, size, p_abort)
#endif
		!= size)
        return ERR_FAIL;
    return size;
}

#ifndef _FOOBAR2000_H_
inline int stringin(char *txt, int sizemax, FILE *g_fin)
#else
inline int stringin(char *txt, int sizemax, service_ptr_t<file> &g_fin, abort_callback & p_abort)
#endif
{
    int size;
    for (size = 0; size < sizemax; size++)
    {
        if (
#ifndef _FOOBAR2000_H_
			fread(txt + size, 1, 1, g_fin)
#else
			g_fin->read(txt + size, 1, p_abort)
#endif
			!= 1)
            return ERR_FAIL;
        if (!txt[size])
            break;
    }
    txt[sizemax-1] = '\0';

    return size;
}

#ifndef _FOOBAR2000_H_
inline uint32_t u32in(FILE *g_fin)
#else
inline uint32_t u32in(service_ptr_t<file> &g_fin, abort_callback & p_abort)
#endif
{
    uint32_t u32;
    datain(&u32, 4, g_fin
#ifdef _FOOBAR2000_H_
		, p_abort
#endif
	);
    u32 = bswap32(u32);
    return u32;
}

#ifndef _FOOBAR2000_H_
inline uint16_t u16in(FILE *g_fin)
#else
inline uint16_t u16in(service_ptr_t<file> &g_fin, abort_callback & p_abort)
#endif
{
    uint16_t u16;
    datain(&u16, 2, g_fin
#ifdef _FOOBAR2000_H_
		, p_abort
#endif
	);
    u16 = bswap16(u16);
    return u16;
}

#ifndef _FOOBAR2000_H_
inline int u8in(FILE *g_fin)
#else
inline int u8in(service_ptr_t<file> &g_fin, abort_callback & p_abort)
#endif
{
    uint8_t u8;
    datain(&u8, 1, g_fin
#ifdef _FOOBAR2000_H_
		, p_abort
#endif
	);
    return u8;
}

#ifndef _FOOBAR2000_H_
static int ftypin(int size, mp4read *mp4inf)
#else
static int ftypin(int size, mp4read *mp4inf, abort_callback & p_abort)
#endif
{
    enum {BUFSIZE = 40};
    char buf[BUFSIZE];
    uint32_t u32;

    buf[4] = 0;
	datain(buf, 4, mp4inf->g_fin
#ifdef _FOOBAR2000_H_
		, p_abort
#endif
	);
    u32 = u32in(mp4inf->g_fin
#ifdef _FOOBAR2000_H_
		, p_abort
#endif
	);

    if (mp4inf->mp4config.verbose.header)
        fprintf(stderr, "Brand:\t\t\t%s(version %d)\n", buf, u32);

    stringin(buf, BUFSIZE, mp4inf->g_fin
#ifdef _FOOBAR2000_H_
		, p_abort
#endif
	);

    if (mp4inf->mp4config.verbose.header)
        fprintf(stderr, "Compatible brands:\t%s\n", buf);

    return size;
}

enum
{ SECSINDAY = 24 * 60 * 60 };
static char *mp4time(time_t t)
{
    int y;

    // subtract some seconds from the start of 1904 to the start of 1970
    for (y = 1904; y < 1970; y++)
    {
        t -= 365 * SECSINDAY;
        if (!(y & 3))
            t -= SECSINDAY;
    }
    return ctime(&t);
}

#ifndef _FOOBAR2000_H_
static int mdhdin(int size, mp4read *mp4inf)
#else
static int mdhdin(int size, mp4read *mp4inf, abort_callback & p_abort)
#endif
{
    // version/flags
    u32in(mp4inf->g_fin
#ifdef _FOOBAR2000_H_
		, p_abort
#endif
	);
    // Creation time
	mp4inf->mp4config.ctime = u32in(mp4inf->g_fin
#ifdef _FOOBAR2000_H_
		, p_abort
#endif
	);
    // Modification time
	mp4inf->mp4config.mtime = u32in(mp4inf->g_fin
#ifdef _FOOBAR2000_H_
		, p_abort
#endif
	);
    // Time scale
	mp4inf->mp4config.samplerate = u32in(mp4inf->g_fin
#ifdef _FOOBAR2000_H_
		, p_abort
#endif
	);
    // Duration
	mp4inf->mp4config.samples = u32in(mp4inf->g_fin
#ifdef _FOOBAR2000_H_
		, p_abort
#endif
	);
    // Language
    u16in(mp4inf->g_fin
#ifdef _FOOBAR2000_H_
		, p_abort
#endif
	);
    // pre_defined
    u16in(mp4inf->g_fin
#ifdef _FOOBAR2000_H_
		, p_abort
#endif
	);

    return size;
};

#ifndef _FOOBAR2000_H_
static int hdlr1in(int size, mp4read *mp4inf)
#else
static int hdlr1in(int size, mp4read *mp4inf, abort_callback & p_abort)
#endif
{
    uint8_t buf[5];

    buf[4] = 0;
    // version/flags
    u32in(mp4inf->g_fin
#ifdef _FOOBAR2000_H_
		, p_abort
#endif
	);
    // pre_defined
    u32in(mp4inf->g_fin
#ifdef _FOOBAR2000_H_
		, p_abort
#endif
	);
    // Component subtype
    datain(buf, 4, mp4inf->g_fin
#ifdef _FOOBAR2000_H_
		, p_abort
#endif
	);
    if (mp4inf->mp4config.verbose.header)
        fprintf(stderr, "*track media type: '%s': ", buf);
    if (memcmp("soun", buf, 4))
    {
        if (mp4inf->mp4config.verbose.header)
            fprintf(stderr, "unsupported, skipping\n");
        return ERR_UNSUPPORTED;
    }
    else
    {
        if (mp4inf->mp4config.verbose.header)
            fprintf(stderr, "OK\n");
    }
    // reserved
    u32in(mp4inf->g_fin
#ifdef _FOOBAR2000_H_
		, p_abort
#endif
	);
    u32in(mp4inf->g_fin
#ifdef _FOOBAR2000_H_
		, p_abort
#endif
	);
    u32in(mp4inf->g_fin
#ifdef _FOOBAR2000_H_
		, p_abort
#endif
	);
    // name
    // null terminate
    u8in(mp4inf->g_fin
#ifdef _FOOBAR2000_H_
		, p_abort
#endif
	);

    return size;
};

#ifndef _FOOBAR2000_H_
static int stsdin(int size, mp4read *mp4inf)
#else
static int stsdin(int size, mp4read *mp4inf, abort_callback & p_abort)
#endif
{
    // version/flags
    u32in(mp4inf->g_fin
#ifdef _FOOBAR2000_H_
		, p_abort
#endif
	);
    // Number of entries(one 'mp4a')
    if (u32in(mp4inf->g_fin
#ifdef _FOOBAR2000_H_
		, p_abort
#endif
	) != 1) //fixme: error handling
        return ERR_FAIL;

    return size;
};

#ifndef _FOOBAR2000_H_
static int mp4ain(int size, mp4read *mp4inf)
#else
static int mp4ain(int size, mp4read *mp4inf, abort_callback & p_abort)
#endif
{
    // Reserved (6 bytes)
    u32in(mp4inf->g_fin
#ifdef _FOOBAR2000_H_
		, p_abort
#endif
	);
    u16in(mp4inf->g_fin
#ifdef _FOOBAR2000_H_
		, p_abort
#endif
	);
    // Data reference index
    u16in(mp4inf->g_fin
#ifdef _FOOBAR2000_H_
		, p_abort
#endif
	);
    // Version
    u16in(mp4inf->g_fin
#ifdef _FOOBAR2000_H_
		, p_abort
#endif
	);
    // Revision level
    u16in(mp4inf->g_fin
#ifdef _FOOBAR2000_H_
		, p_abort
#endif
	);
    // Vendor
    u32in(mp4inf->g_fin
#ifdef _FOOBAR2000_H_
		, p_abort
#endif
	);
    // Number of channels
	mp4inf->mp4config.channels = u16in(mp4inf->g_fin
#ifdef _FOOBAR2000_H_
		, p_abort
#endif
	);
    // Sample size (bits)
	mp4inf->mp4config.bits = u16in(mp4inf->g_fin
#ifdef _FOOBAR2000_H_
		, p_abort
#endif
	);
    // Compression ID
    u16in(mp4inf->g_fin
#ifdef _FOOBAR2000_H_
		, p_abort
#endif
	);
    // Packet size
    u16in(mp4inf->g_fin
#ifdef _FOOBAR2000_H_
		, p_abort
#endif
	);
    // Sample rate (16.16)
    // fractional framerate, probably not for audio
    // rate integer part
    u16in(mp4inf->g_fin
#ifdef _FOOBAR2000_H_
		, p_abort
#endif
	);
    // rate reminder part
    u16in(mp4inf->g_fin
#ifdef _FOOBAR2000_H_
		, p_abort
#endif
	);

    return size;
}


#ifndef _FOOBAR2000_H_
inline uint32_t getsize(FILE *g_fin)
#else
inline uint32_t getsize(service_ptr_t<file> &g_fin, abort_callback & p_abort)
#endif
{
    int cnt;
    uint32_t size = 0;
    for (cnt = 0; cnt < 4; cnt++)
    {
        int tmp = u8in(g_fin
#ifdef _FOOBAR2000_H_
			, p_abort
#endif
		);

        size <<= 7;
        size |= (tmp & 0x7f);
        if (!(tmp & 0x80))
            break;
    }
    return size;
}

#ifndef _FOOBAR2000_H_
static int esdsin(int size, mp4read *mp4inf)
#else
static int esdsin(int size, mp4read *mp4inf, abort_callback & p_abort)
#endif
{
    // descriptor tree:
    // MP4ES_Descriptor
    //   MP4DecoderConfigDescriptor
    //      MP4DecSpecificInfoDescriptor
    //   MP4SLConfigDescriptor
    enum
    { TAG_ES = 3, TAG_DC = 4, TAG_DSI = 5, TAG_SLC = 6 };

    // version/flags
    u32in(mp4inf->g_fin
#ifdef _FOOBAR2000_H_
		, p_abort
#endif
	);
    if (u8in(mp4inf->g_fin
#ifdef _FOOBAR2000_H_
		, p_abort
#endif
	) != TAG_ES)
        return ERR_FAIL;
    getsize(mp4inf->g_fin
#ifdef _FOOBAR2000_H_
		, p_abort
#endif
	);
    // ESID
    u16in(mp4inf->g_fin
#ifdef _FOOBAR2000_H_
		, p_abort
#endif
	);
    // flags(url(bit 6); ocr(5); streamPriority (0-4)):
    u8in(mp4inf->g_fin
#ifdef _FOOBAR2000_H_
		, p_abort
#endif
	);

    if (u8in(mp4inf->g_fin
#ifdef _FOOBAR2000_H_
		, p_abort
#endif
	) != TAG_DC)
        return ERR_FAIL;
    getsize(mp4inf->g_fin
#ifdef _FOOBAR2000_H_
		, p_abort
#endif
	);
    if (u8in(mp4inf->g_fin
#ifdef _FOOBAR2000_H_
		, p_abort
#endif
	) != 0x40) /* not MPEG-4 audio */
        return ERR_FAIL;
    // flags
    u8in(mp4inf->g_fin
#ifdef _FOOBAR2000_H_
		, p_abort
#endif
	);
    // buffer size (24 bits)
	mp4inf->mp4config.buffersize = u16in(mp4inf->g_fin
#ifdef _FOOBAR2000_H_
		, p_abort
#endif
	) << 8;
	mp4inf->mp4config.buffersize |= u8in(mp4inf->g_fin
#ifdef _FOOBAR2000_H_
		, p_abort
#endif
	);
    // bitrate
	mp4inf->mp4config.bitratemax = u32in(mp4inf->g_fin
#ifdef _FOOBAR2000_H_
		, p_abort
#endif
	);
	mp4inf->mp4config.bitrateavg = u32in(mp4inf->g_fin
#ifdef _FOOBAR2000_H_
		, p_abort
#endif
	);

    if (u8in(mp4inf->g_fin
#ifdef _FOOBAR2000_H_
		, p_abort
#endif
	) != TAG_DSI)
        return ERR_FAIL;
	mp4inf->mp4config.asc.size = getsize(mp4inf->g_fin
#ifdef _FOOBAR2000_H_
		, p_abort
#endif
	);
    if (mp4inf->mp4config.asc.size > sizeof(mp4inf->mp4config.asc.buf))
        return ERR_FAIL;
    // get AudioSpecificConfig
    datain(mp4inf->mp4config.asc.buf, mp4inf->mp4config.asc.size, mp4inf->g_fin
#ifdef _FOOBAR2000_H_
		, p_abort
#endif
	);

    if (u8in(mp4inf->g_fin
#ifdef _FOOBAR2000_H_
		, p_abort
#endif
	) != TAG_SLC)
        return ERR_FAIL;
    getsize(mp4inf->g_fin
#ifdef _FOOBAR2000_H_
		, p_abort
#endif
	);
    // "predefined" (no idea)
    u8in(mp4inf->g_fin
#ifdef _FOOBAR2000_H_
		, p_abort
#endif
	);

    return size;
}

#ifndef _FOOBAR2000_H_
static int sttsin(int size, mp4read *mp4inf)
#else
static int sttsin(int size, mp4read *mp4inf, abort_callback & p_abort)
#endif
{
    if (size < 16) //min stts size
        return ERR_FAIL;

    return size;
}

#ifndef _FOOBAR2000_H_
static int stszin(int size, mp4read *mp4inf)
#else
static int stszin(int size, mp4read *mp4inf, abort_callback & p_abort)
#endif
{
    int cnt;
    uint32_t ofs;

    // version/flags
    u32in(mp4inf->g_fin
#ifdef _FOOBAR2000_H_
		, p_abort
#endif
	);
    // Sample size
    u32in(mp4inf->g_fin
#ifdef _FOOBAR2000_H_
		, p_abort
#endif
	);
    // Number of entries
	mp4inf->mp4config.frame.ents = u32in(mp4inf->g_fin
#ifdef _FOOBAR2000_H_
		, p_abort
#endif
	);

    if (!(mp4inf->mp4config.frame.ents + 1))
        return ERR_FAIL;

	mp4inf->mp4config.frame.data = (uint32_t*)malloc(sizeof(*(mp4inf->mp4config.frame.data))
                                  * (mp4inf->mp4config.frame.ents + 1));

    if (!mp4inf->mp4config.frame.data)
        return ERR_FAIL;

    ofs = 0;
	mp4inf->mp4config.frame.data[0] = ofs;
    for (cnt = 0; cnt < mp4inf->mp4config.frame.ents; cnt++)
    {
        uint32_t fsize = u32in(mp4inf->g_fin
#ifdef _FOOBAR2000_H_
			, p_abort
#endif
		);

        ofs += fsize;
        if (mp4inf->mp4config.frame.maxsize < fsize)
			mp4inf->mp4config.frame.maxsize = fsize;

		mp4inf->mp4config.frame.data[cnt + 1] = ofs;

        if (ofs < mp4inf->mp4config.frame.data[cnt])
            return ERR_FAIL;
    }

    return size;
}

#ifndef _FOOBAR2000_H_
static int stcoin(int size, mp4read *mp4inf)
#else
static int stcoin(int size, mp4read *mp4inf, abort_callback & p_abort)
#endif
{
    // version/flags
    u32in(mp4inf->g_fin
#ifdef _FOOBAR2000_H_
		, p_abort
#endif
	);
    // Number of entries
    if (u32in(mp4inf->g_fin
#ifdef _FOOBAR2000_H_
		, p_abort
#endif
	) < 1)
        return ERR_FAIL;
    // first chunk offset
	mp4inf->mp4config.mdatofs = u32in(mp4inf->g_fin
#ifdef _FOOBAR2000_H_
		, p_abort
#endif
	);
    // ignore the rest

    return size;
}


#ifndef _FOOBAR2000_H_
static int metain(int size, mp4read *mp4inf)
#else
static int metain(int size, mp4read *mp4inf, abort_callback & p_abort)
#endif
{
    // version/flags
    u32in(mp4inf->g_fin
#ifdef _FOOBAR2000_H_
		, p_abort
#endif
	);

    return ERR_OK;
};

#ifndef _FOOBAR2000_H_
static int hdlr2in(int size, mp4read *mp4inf)
#else
static int hdlr2in(int size, mp4read *mp4inf, abort_callback & p_abort)
#endif
{
    uint8_t buf[4];

    // version/flags
    u32in(mp4inf->g_fin
#ifdef _FOOBAR2000_H_
		, p_abort
#endif
	);
    // Predefined
    u32in(mp4inf->g_fin
#ifdef _FOOBAR2000_H_
		, p_abort
#endif
	);
    // Handler type
    datain(buf, 4, mp4inf->g_fin
#ifdef _FOOBAR2000_H_
		, p_abort
#endif
	);
    if (memcmp(buf, "mdir", 4))
        return ERR_FAIL;
    datain(buf, 4, mp4inf->g_fin
#ifdef _FOOBAR2000_H_
		, p_abort
#endif
	);
    if (memcmp(buf, "appl", 4))
        return ERR_FAIL;
    // Reserved
    u32in(mp4inf->g_fin
#ifdef _FOOBAR2000_H_
		, p_abort
#endif
	);
    u32in(mp4inf->g_fin
#ifdef _FOOBAR2000_H_
		, p_abort
#endif
	);
    // null terminator
    u8in(mp4inf->g_fin
#ifdef _FOOBAR2000_H_
		, p_abort
#endif
	);
    u8in(mp4inf->g_fin
#ifdef _FOOBAR2000_H_
		, p_abort
#endif
	);

    return size;
};

#ifndef _FOOBAR2000_H_
static int ilstin(int size, mp4read *mp4inf)
#else
static int ilstin(int size, mp4read *mp4inf, abort_callback & p_abort)
#endif
{
    enum {NUMSET = 1, GENRE, EXTAG};
    int read = 0;

    static struct {
        char *name;
        char *id;
        int flag;
    } tags[] = {
        {"Album       ", "\xa9" "alb"},
        {"Album Artist", "aART"},
        {"Artist      ", "\xa9" "ART"},
        {"Comment     ", "\xa9" "cmt"},
        {"Cover image ", "covr"},
        {"Compilation ", "cpil"},
        {"Copyright   ", "cprt"},
        {"Date        ", "\xa9" "day"},
        {"Disc#       ", "disk", NUMSET},
        {"Genre       ", "gnre", GENRE},
        {"Grouping    ", "\xa9" "grp"},
        {"Lyrics      ", "\xa9" "lyr"},
        {"Title       ", "\xa9" "nam"},
        {"Rating      ", "rtng"},
        {"BPM         ", "tmpo"},
        {"Encoder     ", "\xa9" "too"},
        {"Track       ", "trkn", NUMSET},
        {"Composer    ", "\xa9" "wrt"},
        {0, "----", EXTAG},
        {0},
    };

    static const char *genres[] = {
        "Blues", "Classic Rock", "Country", "Dance",
        "Disco", "Funk", "Grunge", "Hip-Hop",
        "Jazz", "Metal", "New Age", "Oldies",
        "Other", "Pop", "R&B", "Rap",
        "Reggae", "Rock", "Techno", "Industrial",
        "Alternative", "Ska", "Death Metal", "Pranks",
        "Soundtrack", "Euro-Techno", "Ambient", "Trip-Hop",
        "Vocal", "Jazz+Funk", "Fusion", "Trance",
        "Classical", "Instrumental", "Acid", "House",
        "Game", "Sound Clip", "Gospel", "Noise",
        "Alternative Rock", "Bass", "Soul", "Punk",
        "Space", "Meditative", "Instrumental Pop", "Instrumental Rock",
        "Ethnic", "Gothic", "Darkwave", "Techno-Industrial",
        "Electronic", "Pop-Folk", "Eurodance", "Dream",
        "Southern Rock", "Comedy", "Cult", "Gangsta",
        "Top 40", "Christian Rap", "Pop/Funk", "Jungle",
        "Native US", "Cabaret", "New Wave", "Psychadelic",
        "Rave", "Showtunes", "Trailer", "Lo-Fi",
        "Tribal", "Acid Punk", "Acid Jazz", "Polka",
        "Retro", "Musical", "Rock & Roll", "Hard Rock",
        "Folk", "Folk-Rock", "National Folk", "Swing",
        "Fast Fusion", "Bebob", "Latin", "Revival",
        "Celtic", "Bluegrass", "Avantgarde", "Gothic Rock",
        "Progressive Rock", "Psychedelic Rock", "Symphonic Rock", "Slow Rock",
        "Big Band", "Chorus", "Easy Listening", "Acoustic",
        "Humour", "Speech", "Chanson", "Opera",
        "Chamber Music", "Sonata", "Symphony", "Booty Bass",
        "Primus", "Porn Groove", "Satire", "Slow Jam",
        "Club", "Tango", "Samba", "Folklore",
        "Ballad", "Power Ballad", "Rhythmic Soul", "Freestyle",
        "Duet", "Punk Rock", "Drum Solo", "Acapella",
        "Euro-House", "Dance Hall", "Goa", "Drum & Bass",
        "Club - House", "Hardcore", "Terror", "Indie",
        "BritPop", "Negerpunk", "Polsk Punk", "Beat",
        "Christian Gangsta Rap", "Heavy Metal", "Black Metal", "Crossover",
        "Contemporary Christian", "Christian Rock", "Merengue", "Salsa",
        "Thrash Metal", "Anime", "JPop", "Synthpop",
        "Unknown",
    };

    fprintf(stderr, "----------tag list-------------\n");
    while(read < size)
    {
        int asize, dsize;
        uint8_t id[5];
        int cnt;
        uint32_t type;

        id[4] = 0;

        asize = u32in(mp4inf->g_fin
#ifdef _FOOBAR2000_H_
			, p_abort
#endif
		);
        read += asize;
        asize -= 4;
        if (datain(id, 4, mp4inf->g_fin
#ifdef _FOOBAR2000_H_
			, p_abort
#endif
		) < 4)
            return ERR_FAIL;
        asize -= 4;

        for (cnt = 0; tags[cnt].id; cnt++)
        {
            if (!memcmp(id, tags[cnt].id, 4))
                break;
        }

        if (tags[cnt].name)
            fprintf(stderr, "%s :   ", tags[cnt].name);
        else
        {
            if (tags[cnt].flag != EXTAG)
                fprintf(stderr, "'%s'       :   ", id);
        }

        dsize = u32in(mp4inf->g_fin
#ifdef _FOOBAR2000_H_
			, p_abort
#endif
		);
        asize -= 4;
        if (datain(id, 4, mp4inf->g_fin
#ifdef _FOOBAR2000_H_
			, p_abort
#endif
		) < 4)
            return ERR_FAIL;
        asize -= 4;

        if (tags[cnt].flag != EXTAG)
        {
            if (memcmp(id, "data", 4))
                return ERR_FAIL;
        }
        else
        {
            int spc;

            if (memcmp(id, "mean", 4))
                goto skip;
            dsize -= 8;
            while (dsize > 0)
            {
                u8in(mp4inf->g_fin
#ifdef _FOOBAR2000_H_
					, p_abort
#endif
				);
                asize--;
                dsize--;
            }
            if (asize >= 8)
            {
                dsize = u32in(mp4inf->g_fin
#ifdef _FOOBAR2000_H_
					, p_abort
#endif
				) - 8;
                asize -= 4;
                if (datain(id, 4, mp4inf->g_fin
#ifdef _FOOBAR2000_H_
					, p_abort
#endif
				) < 4)
                    return ERR_FAIL;
                asize -= 4;
                if (memcmp(id, "name", 4))
                    goto skip;
                u32in(mp4inf->g_fin
#ifdef _FOOBAR2000_H_
					, p_abort
#endif
				);
                asize -= 4;
                dsize -= 4;
            }
            spc = 13 - dsize;
            if (spc < 0) spc = 0;
            while (dsize > 0)
            {
                fprintf(stderr, "%c",u8in(mp4inf->g_fin
#ifdef _FOOBAR2000_H_
					, p_abort
#endif
				));
                asize--;
                dsize--;
            }
            while (spc--)
                fprintf(stderr, " ");
            fprintf(stderr, ":   ");
            if (asize >= 8)
            {
                dsize = u32in(mp4inf->g_fin
#ifdef _FOOBAR2000_H_
					, p_abort
#endif
				) - 8;
                asize -= 4;
                if (datain(id, 4, mp4inf->g_fin
#ifdef _FOOBAR2000_H_
					, p_abort
#endif
				) < 4)
                    return ERR_FAIL;
                asize -= 4;
                if (memcmp(id, "data", 4))
                    goto skip;
                u32in(mp4inf->g_fin
#ifdef _FOOBAR2000_H_
					, p_abort
#endif
				);
                asize -= 4;
                dsize -= 4;
            }
            while (dsize > 0)
            {
                fprintf(stderr, "%c",u8in(mp4inf->g_fin
#ifdef _FOOBAR2000_H_
					, p_abort
#endif
				));
                asize--;
                dsize--;
            }
            fprintf(stderr, "\n");

            goto skip;
        }
        type = u32in(mp4inf->g_fin
#ifdef _FOOBAR2000_H_
			, p_abort
#endif
		);
        asize -= 4;
        u32in(mp4inf->g_fin
#ifdef _FOOBAR2000_H_
			, p_abort
#endif
		);
        asize -= 4;

        switch(type)
        {
        case 1:
            while (asize > 0)
            {
                fprintf(stderr, "%c",u8in(mp4inf->g_fin
#ifdef _FOOBAR2000_H_
					, p_abort
#endif
				));
                asize--;
            }
            break;
        case 0:
            switch(tags[cnt].flag)
            {
            case NUMSET:
                u16in(mp4inf->g_fin
#ifdef _FOOBAR2000_H_
					, p_abort
#endif
				);
                asize -= 2;

                fprintf(stderr, "%d", u16in(mp4inf->g_fin
#ifdef _FOOBAR2000_H_
					, p_abort
#endif
				));
                asize -= 2;
                fprintf(stderr, "/%d", u16in(mp4inf->g_fin
#ifdef _FOOBAR2000_H_
					, p_abort
#endif
				));
                asize -= 2;
                break;
            case GENRE:
                {
                    uint8_t gnum = u16in(mp4inf->g_fin
#ifdef _FOOBAR2000_H_
						, p_abort
#endif
					);
                    asize -= 2;
                    if (!gnum)
                       goto skip;
                    gnum--;
                    if (gnum >= 147)
                        gnum = 147;
                    fprintf(stderr, "%s", genres[gnum]);
                }
                break;
            default:
                while(asize > 0)
                {
                    fprintf(stderr, "%d/", u16in(mp4inf->g_fin
#ifdef _FOOBAR2000_H_
						, p_abort
#endif
					));
                    asize-=2;
                }
            }
            break;
        case 0x15:
            //fprintf(stderr, "(8bit data)");
            while(asize > 0)
            {
                fprintf(stderr, "%d", u8in(mp4inf->g_fin
#ifdef _FOOBAR2000_H_
					, p_abort
#endif
				));
                asize--;
                if (asize)
                    fprintf(stderr, "/");
            }
            break;
        case 0xd:
            fprintf(stderr, "(image data)");
            break;
        default:
            fprintf(stderr, "(unknown data type)");
            break;
        }
        fprintf(stderr, "\n");

    skip:
        // skip to the end of atom
        while (asize > 0)
        {
            u8in(mp4inf->g_fin
#ifdef _FOOBAR2000_H_
				, p_abort
#endif
			);
            asize--;
        }
    }
    fprintf(stderr, "-------------------------------\n");

    return size;
};


#ifndef _FOOBAR2000_H_
int mp4read::parse(uint32_t *sizemax)
#else
int mp4read::parse(uint32_t *sizemax, abort_callback & p_abort)
#endif
{
    long apos = 0;
	long aposmax =
#ifndef _FOOBAR2000_H_
		ftell(g_fin)
#else
		g_fin->get_position(p_abort)
#endif
		+ *sizemax;
    uint32_t size;

    if (g_atom->opcode != ATOM_NAME)
    {
        fprintf(stderr, "parse error: root is not a 'name' opcode\n");
        return ERR_FAIL;
    }
    //fprintf(stderr, "looking for '%s'\n", (char *)g_atom->data);

    // search for atom in the file
    while (1)
    {
        char name[4];
        uint32_t tmp;

        apos =
#ifndef _FOOBAR2000_H_
			ftell(g_fin)
#else
			g_fin->get_position(p_abort)
#endif
			;
        if (apos >= (aposmax - 8))
        {
            fprintf(stderr, "parse error: atom '%s' not found\n", (char *)g_atom->data);
            return ERR_FAIL;
        }
        if ((tmp = u32in(g_fin
#ifdef _FOOBAR2000_H_
			, p_abort
#endif
		)) < 8)
        {
            fprintf(stderr, "invalid atom size %x @%lx\n", tmp,
#ifndef _FOOBAR2000_H_
				ftell(g_fin)
#else
				g_fin->get_position(p_abort)
#endif
			);
            return ERR_FAIL;
        }

        size = tmp;
        if (datain(name, 4, g_fin
#ifdef _FOOBAR2000_H_
			, p_abort
#endif
		) != 4)
        {
            // EOF
            fprintf(stderr, "can't read atom name @%lx\n",
#ifndef _FOOBAR2000_H_
				ftell(g_fin)
#else
				g_fin->get_position(p_abort)
#endif
			);
            return ERR_FAIL;
        }

        //fprintf(stderr, "atom: '%c%c%c%c'(%x)", name[0],name[1],name[2],name[3], size);

        if (!memcmp(name, g_atom->data, 4))
        {
            //fprintf(stderr, "OK\n");
            break;
        }
        //fprintf(stderr, "\n");

#ifndef _FOOBAR2000_H_
		fseek(g_fin, apos + size, SEEK_SET);
#else
		g_fin->ensure_seekable();
		g_fin->seek(apos + size, p_abort);
#endif
    }
    *sizemax = size;
    g_atom++;
    if (g_atom->opcode == ATOM_DATA)
    {
#ifndef _FOOBAR2000_H_
		int err = ((int(*)(int, mp4read*)) g_atom->data)(size - 8, this);
#else
		int err = ((int(*)(int, mp4read*, abort_callback&)) g_atom->data)(size - 8, this, p_abort);
#endif
        if (err < ERR_OK)
        {
#ifndef _FOOBAR2000_H_
			fseek(g_fin, apos + size, SEEK_SET);
#else
			g_fin->ensure_seekable();
			g_fin->seek(apos + size, p_abort);
#endif
            return err;
        }
        g_atom++;
    }
    if (g_atom->opcode == ATOM_DESCENT)
    {
        long apos =
#ifndef _FOOBAR2000_H_
			ftell(g_fin)
#else
			g_fin->get_position(p_abort)
#endif
			;

        //fprintf(stderr, "descent\n");
        g_atom++;
        while (g_atom->opcode != ATOM_STOP)
        {
            uint32_t subsize = size - 8;
            int ret;
            if (g_atom->opcode == ATOM_ASCENT)
            {
                g_atom++;
                break;
            }
#ifndef _FOOBAR2000_H_
			fseek(g_fin, apos, SEEK_SET);
#else
			g_fin->ensure_seekable();
			g_fin->seek(apos, p_abort);
#endif
            if ((ret = parse(&subsize
#ifdef _FOOBAR2000_H_
				, p_abort
#endif
			)) < 0)
                return ret;
        }
        //fprintf(stderr, "ascent\n");
    }

#ifndef _FOOBAR2000_H_
	fseek(g_fin, apos + size, SEEK_SET);
#else
	g_fin->ensure_seekable();
	g_fin->seek(apos + size, p_abort);
#endif

    return ERR_OK;
}



#ifndef _FOOBAR2000_H_
static int moovin(int sizemax, mp4read *mp4inf)
#else
static int moovin(int sizemax, mp4read *mp4inf, abort_callback & p_abort)
#endif
{
	long apos =
#ifndef _FOOBAR2000_H_
		ftell(mp4inf->g_fin)
#else
		mp4inf->g_fin->get_position(p_abort)
#endif
		;
    uint32_t atomsize;
    creator_t *old_atom = mp4inf->g_atom;
    int err, ret = sizemax;

    static creator_t mvhd[] = {
        {ATOM_NAME, "mvhd"},
        {0}
    };
    static creator_t trak[] = {
		{ATOM_NAME, "trak"},
        {ATOM_DESCENT},
        {ATOM_NAME, "tkhd"},
        {ATOM_NAME, "mdia"},
        {ATOM_DESCENT},
        {ATOM_NAME, "mdhd"},
        {ATOM_DATA, mdhdin},
        {ATOM_NAME, "hdlr"},
        {ATOM_DATA, hdlr1in},
        {ATOM_NAME, "minf"},
        {ATOM_DESCENT},
        {ATOM_NAME, "smhd"},
        {ATOM_NAME, "dinf"},
        {ATOM_NAME, "stbl"},
        {ATOM_DESCENT},
        {ATOM_NAME, "stsd"},
        {ATOM_DATA, stsdin},
        {ATOM_DESCENT},
        {ATOM_NAME, "mp4a"},
        {ATOM_DATA, mp4ain},
        {ATOM_DESCENT},
        {ATOM_NAME, "esds"},
        {ATOM_DATA, esdsin},
        {ATOM_ASCENT},
        {ATOM_ASCENT},
        {ATOM_NAME, "stts"},
        {ATOM_DATA, sttsin},
        {ATOM_NAME, "stsc"},
        {ATOM_NAME, "stsz"},
        {ATOM_DATA, stszin},
        {ATOM_NAME, "stco"},
        {ATOM_DATA, stcoin},
        {0}
    };

	mp4inf->g_atom = mvhd;
    atomsize = sizemax + apos -
#ifndef _FOOBAR2000_H_
		ftell(mp4inf->g_fin)
#else
		mp4inf->g_fin->get_position(p_abort)
#endif
		;
    if (mp4inf->parse(&atomsize
#ifdef _FOOBAR2000_H_
		, p_abort
#endif
	) < 0) {
		mp4inf->g_atom = old_atom;
        return ERR_FAIL;
    }

#ifndef _FOOBAR2000_H_
	fseek(mp4inf->g_fin, apos, SEEK_SET);
#else
	mp4inf->g_fin->seek(apos, p_abort);
#endif

    while (1)
    {
        //fprintf(stderr, "TRAK\n");
		mp4inf->g_atom = trak;
        atomsize = sizemax + apos -
#ifndef _FOOBAR2000_H_
			ftell(mp4inf->g_fin)
#else
			mp4inf->g_fin->get_position(p_abort)
#endif
			;
        if (atomsize < 8)
            break;
        //fprintf(stderr, "PARSE(%x)\n", atomsize);
        err = mp4inf->parse(&atomsize
#ifdef _FOOBAR2000_H_
			, p_abort
#endif
		);
        //fprintf(stderr, "SIZE: %x/%x\n", atomsize, sizemax);
        if (err >= 0)
            break;
        if (err != ERR_UNSUPPORTED) {
            ret = err;
            break;
        }
        //fprintf(stderr, "UNSUPP\n");
    }

	mp4inf->g_atom = old_atom;
    return ret;
}


static creator_t g_head[] = {
    {ATOM_NAME, "ftyp"},
    {ATOM_DATA, ftypin},
    {0}
};

static creator_t g_moov[] = {
    {ATOM_NAME, "moov"},
    {ATOM_DATA, moovin},
    //{ATOM_DESCENT},
    //{ATOM_NAME, "mvhd"},
    {0}
};

static creator_t g_meta1[] = {
    {ATOM_NAME, "moov"},
    {ATOM_DESCENT},
    {ATOM_NAME, "udta"},
    {ATOM_DESCENT},
    {ATOM_NAME, "meta"},
    {ATOM_DATA, metain},
    {ATOM_DESCENT},
    {ATOM_NAME, "hdlr"},
    {ATOM_DATA, hdlr2in},
    {ATOM_NAME, "ilst"},
    {ATOM_DATA, ilstin},
    {0}
};

static creator_t g_meta2[] = {
    {ATOM_NAME, "meta"},
    {ATOM_DATA, metain},
    {ATOM_DESCENT},
    {ATOM_NAME, "hdlr"},
    {ATOM_DATA, hdlr2in},
    {ATOM_NAME, "ilst"},
    {ATOM_DATA, ilstin},
    {0}
};


#ifndef _FOOBAR2000_H_
int mp4read::mp4read_frame(void)
#else
int mp4read::mp4read_frame(abort_callback & p_abort)
#endif
{
    if (mp4config.frame.current >= mp4config.frame.ents)
        return ERR_FAIL;

    mp4config.bitbuf.size = mp4config.frame.data[mp4config.frame.current + 1]
        - mp4config.frame.data[mp4config.frame.current];

    if (
#ifndef _FOOBAR2000_H_
		fread(mp4config.bitbuf.data, 1, mp4config.bitbuf.size, g_fin)
#else
		g_fin->read(mp4config.bitbuf.data, mp4config.bitbuf.size, p_abort)
#endif
        != mp4config.bitbuf.size)
    {
        fprintf(stderr, "can't read frame data(frame %d@0x%x)\n",
               mp4config.frame.current,
               mp4config.frame.data[mp4config.frame.current]);

        return ERR_FAIL;
    }

    mp4config.frame.current++;

    return ERR_OK;
}

#ifndef _FOOBAR2000_H_
int mp4read::mp4read_seek(int framenum)
#else
int mp4read::mp4read_seek(int framenum, abort_callback & p_abort)
#endif
{
    if (framenum > mp4config.frame.ents)
        return ERR_FAIL;
#ifndef _FOOBAR2000_H_
    if (fseek(g_fin, mp4config.mdatofs + mp4config.frame.data[framenum], SEEK_SET))
        return ERR_FAIL;
#else
	g_fin->ensure_seekable();
	g_fin->seek(mp4config.mdatofs + mp4config.frame.data[framenum], p_abort);
#endif
    mp4config.frame.current = framenum;

    return ERR_OK;
}

#ifndef _FOOBAR2000_H_
void mp4read::mp4info(void)
#else
void mp4read::mp4info(abort_callback & p_abort)
#endif
{
    fprintf(stderr, "Modification Time:\t\t%s\n", mp4time(mp4config.mtime));
    fprintf(stderr, "Samplerate:\t\t%d\n", mp4config.samplerate);
    fprintf(stderr, "Total samples:\t\t%d\n", mp4config.samples);
    fprintf(stderr, "Total channels:\t\t%d\n", mp4config.channels);
    fprintf(stderr, "Bits per sample:\t%d\n", mp4config.bits);
    fprintf(stderr, "Buffer size:\t\t%d\n", mp4config.buffersize);
    fprintf(stderr, "Max bitrate:\t\t%d\n", mp4config.bitratemax);
    fprintf(stderr, "Average bitrate:\t%d\n", mp4config.bitrateavg);
    fprintf(stderr, "Samples per frame:\t%d\n", mp4config.framesamples);
    fprintf(stderr, "Frames:\t\t\t%d\n", mp4config.frame.ents);
    fprintf(stderr, "ASC size:\t\t%d\n", mp4config.asc.size);
    fprintf(stderr, "Duration:\t\t%.1f sec\n", (float)mp4config.samples/mp4config.samplerate);
    fprintf(stderr, "Data offset/size:\t%x/%x\n", mp4config.mdatofs, mp4config.mdatsize);
}

int mp4read::mp4read_close(void)
{
#define FREE(x) if(x){free(x);x=0;}
    FREE(mp4config.frame.data);
    FREE(mp4config.bitbuf.data);
#ifndef _FOOBAR2000_H_
	if (g_fin != NULL) {
		fclose(g_fin);
		g_fin = NULL;
	}
#endif
    return ERR_OK;
}

#ifndef _FOOBAR2000_H_
int mp4read::mp4read_open(const char *name)
#else
int mp4read::mp4read_open(const char *name, abort_callback & p_abort)
#endif
{
    uint32_t atomsize;
    int ret;

#ifndef _FOOBAR2000_H_
	if (g_fin == NULL) g_fin = faad_fopen(name, "rb");
	else{
		FREE(mp4config.frame.data);
		FREE(mp4config.bitbuf.data);
		fseek(g_fin, 0, SEEK_SET);
	}
    if (!g_fin)
        return ERR_FAIL;
#else
	filesystem::g_open(g_fin, name, filesystem::open_mode_read, p_abort);
	if (g_fin == NULL)return ERR_FAIL;
#endif
    if (mp4config.verbose.header)
        fprintf(stderr, "**** MP4 header ****\n");
    g_atom = g_head;
    atomsize = INT_MAX;
    if (parse(&atomsize
#ifdef _FOOBAR2000_H_
		, p_abort
#endif
	) < 0)
        goto err;
    g_atom = g_moov;
    atomsize = INT_MAX;
#ifndef _FOOBAR2000_H_
    rewind(g_fin);
#else
	g_fin->reopen(p_abort);
#endif
    if ((ret = parse(&atomsize
#ifdef _FOOBAR2000_H_
		, p_abort
#endif
	)) < 0)
    {
        fprintf(stderr, "parse:%d\n", ret);
        goto err;
    }

    // alloc frame buffer
    mp4config.bitbuf.data = (uint8_t*)malloc(mp4config.frame.maxsize);

    if (!mp4config.bitbuf.data)
        goto err;

    if (mp4config.verbose.header)
    {
        mp4info(
#ifdef _FOOBAR2000_H_
			p_abort
#endif
		);
        fprintf(stderr, "********************\n");
    }

    if (mp4config.verbose.tags)
    {
#ifndef _FOOBAR2000_H_
		rewind(g_fin);
#else
		g_fin->reopen(p_abort);
#endif
        g_atom = g_meta1;
        atomsize = INT_MAX;
        ret = parse(&atomsize
#ifdef _FOOBAR2000_H_
			, p_abort
#endif
		);
        if (ret < 0)
        {
#ifndef _FOOBAR2000_H_
			rewind(g_fin);
#else
			g_fin->reopen(p_abort);
#endif
            g_atom = g_meta2;
            atomsize = INT_MAX;
            ret = parse(&atomsize
#ifdef _FOOBAR2000_H_
				, p_abort
#endif
			);
        }
    }

    return ERR_OK;
err:
    mp4read_close(
#ifdef _FOOBAR2000_H_
		p_abort
#endif
	);
    return ERR_FAIL;
}
