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

#include <stdint.h>

typedef struct
{
	uint16_t opcode;
	const void *data;
} creator_t;

typedef struct
{
    uint32_t ctime, mtime;
    uint32_t samplerate;
    // total sound samples
    uint32_t samples;
    uint32_t channels;
    // sample depth
    uint32_t bits;
    // buffer config
    uint16_t buffersize;
    uint32_t bitratemax;
    uint32_t bitrateavg;
    uint32_t framesamples;
    struct
    {
        uint32_t *data;
        uint32_t ents;
        int current;
        int maxsize;
    } frame;
    // AudioSpecificConfig data:
    struct
    {
        uint8_t buf[10];
        int size;
    } asc;
    uint32_t mdatofs;
    uint32_t mdatsize;
    struct {
        int size;
        uint8_t *data;
    } bitbuf;
    struct {
        int header;
        int tags;
    } verbose;
} mp4config_t;


class mp4read {
public:
	int mp4read_close(void);
#ifndef _FOOBAR2000_H_
	friend static int moovin(int sizemax, mp4read *mp4inf);
	int mp4read_open(const char *name);
	int mp4read_seek(int framenum);
	void mp4info(void);
	int mp4read_frame(void);
protected:
	int parse(uint32_t *sizemax);
public:
	FILE *g_fin;
#else
	friend static int moovin(int sizemax, mp4read *mp4inf, abort_callback & p_abort);
	int mp4read_open(const char *name, abort_callback & p_abort);
	int mp4read_seek(int framenum, abort_callback & p_abort);
	void mp4info(abort_callback & p_abort);
	int mp4read_frame(abort_callback & p_abort);
protected:
	int parse(uint32_t *sizemax, abort_callback & p_abort);
public:
	service_ptr_t<file> g_fin;
#endif
	mp4read(){
		g_fin = NULL;
		g_atom = NULL;
		memset(&mp4config, 0, sizeof(mp4config_t));
	}
	creator_t *g_atom;
	mp4config_t mp4config;
};
