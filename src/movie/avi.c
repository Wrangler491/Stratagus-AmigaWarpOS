//       _________ __                 __
//      /   _____//  |_____________ _/  |______     ____  __ __  ______
//      \_____  \\   __\_  __ \__  \\   __\__  \   / ___\|  |  \/  ___/
//      /        \|  |  |  | \// __ \|  |  / __ \_/ /_/  >  |  /\___ |
//     /_______  /|__|  |__|  (____  /__| (____  /\___  /|____//____  >
//             \/                  \/          \//_____/            \/
//  ______________________                           ______________________
//                        T H E   W A R   B E G I N S
//         Stratagus - A free fantasy real time strategy game engine
//
/**@name avi.c - avi support */
//
//      (c) Copyright 2002-2004 by Lutz Sammer.
//
//      This program is free software; you can redistribute it and/or modify
//      it under the terms of the GNU General Public License as published by
//      the Free Software Foundation; only version 2 of the License.
//
//      This program is distributed in the hope that it will be useful,
//      but WITHOUT ANY WARRANTY; without even the implied warranty of
//      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//      GNU General Public License for more details.
//
//      You should have received a copy of the GNU General Public License
//      along with this program; if not, write to the Free Software
//      Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
//      02111-1307, USA.
//
//      $Id: avi.c,v 1.17 2004/06/24 17:18:58 jarod42 Exp $

//@{

/*----------------------------------------------------------------------------
-- Includes
----------------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "stratagus.h"

#include "iolib.h"
#include "myendian.h"
#include "avi.h"

/*----------------------------------------------------------------------------
--  Declaration
----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------
--  Functions
----------------------------------------------------------------------------*/

/**
**  Open an AVI file.
**
**  @param name  File name to open as avi file.
**
**  @return      FIXME: docu
*/
AviFile* AviOpen(const char* name)
{
	AviFile* avi;
	CLFile* f;
	unsigned char buf[256];
	unsigned char* hdr_buf;
	long hdr_len;
	unsigned long magic;
	unsigned long length;
	int i;
	int j;
	int stream;

	//
	// Open and check if it is an avi file.
	//
	if (!(f = CLopen(name, CL_OPEN_READ))) {
		fprintf(stderr, "Can't open `%s'\n", name);
		return NULL;
	}
	//
	//  Read first 12 bytes and check if it is an AVI file
	//
	if (CLread(f, buf, 12) != 12) {
		CLclose(f);
		return NULL;
	}
	//
	//  Check the header magics
	//
	if (AccessLE32(buf) != 0x46464952) {  // RIFF
		DebugPrint("Wrong magic %x (not %x)\n" _C_ AccessLE32(buf) _C_
			0x46464952);
		CLclose(f);
		return NULL;
	}
	if (AccessLE32(buf + 8) != 0x20495641) {  // "AVI "
		DebugPrint("Wrong magic %x (not %x)\n" _C_ AccessLE32(buf) _C_
			0x20495641);
		CLclose(f);
		return NULL;
	}

	if (!(avi = calloc(sizeof(*avi), 1))) {
		fprintf(stderr, "Out of memory\n");
		CLclose(f);
		return NULL;
	}
	avi->FileHandle = f;
	avi->VideoStream = -1;
	avi->AudioStream = -1;

	//
	//  Find header
	//
	if (CLread(f, buf, 8) != 8) {  // EOF
		DebugPrint("Unexpected eof\n");
		CLclose(f);
		free(avi);
		return NULL;
	}
	magic = AccessLE32(buf);
	hdr_len = AccessLE32(buf + 4);
	hdr_len = (hdr_len + 1) & ~1;  // pad even
	DebugPrint("Bytes %ld\n" _C_ hdr_len);

	if (magic != 0x5453494C) {  // LIST
		DebugPrint("Wrong magic %x (not %x)\n" _C_ AccessLE32(buf) _C_
			0x5453494C);
		CLclose(f);
		free(avi);
		return NULL;
	}
	if (CLread(f, buf, 4) != 4) {  // EOF
		DebugPrint("Unexpected eof\n");
		CLclose(f);
		free(avi);
		return NULL;
	}
	magic = AccessLE32(buf);
	if (magic != 0x6C726468) {  // hdrl
		DebugPrint("Wrong magic %x (not %x)\n" _C_ AccessLE32(buf) _C_
			0x6C726468);
		CLclose(f);
		free(avi);
		return NULL;
	}
	hdr_len -= 4;

	hdr_buf = alloca(hdr_len);
	if (CLread(f, hdr_buf, hdr_len) != hdr_len) {  // EOF
		DebugPrint("Unexpected eof\n");
		CLclose(f);
		free(avi);
		return NULL;
	}
	//
	//  Header
	//
	for (stream = i = 0; i < hdr_len;) {
		magic = AccessLE32(hdr_buf + i);
		length = AccessLE32(hdr_buf + i + 4);
		length = (length + 1) & ~1;  // pad even
		DebugPrint("Magic %lx %ld\n" _C_ magic _C_ length);

		if (magic == 0x68697661) {  // avih
			i += length + 8;
			continue;  // ignore it
		}

		if (magic != 0x5453494C) {  // LIST
			DebugPrint("Wrong magic %lx (not %x)\n" _C_ magic _C_
				0x5453494C);
			CLclose(f);
			free(avi);
			return NULL;
		}
		magic = AccessLE32(hdr_buf + i + 8);
		if (magic != 0x6C727473) {  // strl
			DebugPrint("Wrong magic %lx (not %x)\n" _C_ magic _C_
				0x6C727473);

			i += length + 8;
			continue;  // Ignore unknown LIST
		}

		magic = AccessLE32(hdr_buf + i + 12);
		if (magic != 0x68727473) {  // strh
			DebugPrint("Wrong magic %lx (not %x)\n" _C_ magic _C_
				0x68727473);
			CLclose(f);
			free(avi);
			return NULL;
		}

		magic = AccessLE32(hdr_buf + i + 20);
		DebugPrint("Magic %lx\n" _C_ magic);
		if (magic == 0x73646976) {  // vids
			int scale;
			int rate;

			memcpy(avi->VideoCodec, hdr_buf + i + 24, 4);

			scale = AccessLE32(hdr_buf + i + 40);
			rate = AccessLE32(hdr_buf + i + 44);
			avi->FPS100 = rate * 100 / scale;

			avi->NumFrames = AccessLE32(hdr_buf + i + 52);

			avi->VideoStream = stream;

			DebugPrint("Video%d: %s #%ld %3.2f fps " _C_
				avi->VideoStream _C_ avi->VideoCodec _C_ avi->NumFrames _C_
				avi->FPS100 / 100.0);

			j = AccessLE32(hdr_buf + i + 16);
			j = (j + 1) & ~1;  // pad even
			j += i + 20;
			magic = AccessLE32(hdr_buf + j);
			if (magic != 0x66727473) { // strf
				DebugPrint("Wrong magic %lx (not %x)\n" _C_ magic _C_
					0x66727473);
				CLclose(f);
				free(avi);
				return NULL;
			}

			avi->Width = AccessLE32(hdr_buf + j + 12);
			avi->Height = AccessLE32(hdr_buf + j + 16);

			DebugPrint("%dx%d\n" _C_ avi->Width _C_ avi->Height);

			avi->VideoTag =
				((avi->VideoStream / 10 + '0')) |
				((avi->VideoStream % 10 + '0') << 8) |
				('d' << 16) /*| ('b' << 24)*/;

		} else if (magic == 0x73647561) {  // auds

			avi->AudioStream = stream;

			DebugPrint("Audio%d: %d\n" _C_ stream  _C_
				AccessLE32(hdr_buf + i + 32));

			j = AccessLE32(hdr_buf + i + 16);
			j = (j + 1) & ~1;  // pad even
			j += i + 20;
			magic = AccessLE32(hdr_buf + j);
			if (magic != 0x66727473) {  // strf
				DebugPrint("Wrong magic %lx (not %x)\n" _C_ magic _C_
					0x66727473);
				CLclose(f);
				free(avi);
				return NULL;
			}

			DebugPrint("Audio%d: Format %x Channels %d Rate %d Bitrate %d Bits %d\n" _C_
				avi->AudioStream  _C_
				AccessLE16(hdr_buf + j + 8 + 0) _C_
				AccessLE16(hdr_buf + j + 8 + 2) _C_
				AccessLE32(hdr_buf + j + 8 + 4) _C_
				AccessLE32(hdr_buf + j + 8 + 8) _C_
				AccessLE16(hdr_buf + j + 8 + 14));

			switch (AccessLE16(hdr_buf + j + 8 + 0)) {
				case 'O' + ('g' << 8):
					DebugPrint("Original stream compatible\n");
					break;
				case 'P' + ('g' << 8):
					DebugPrint("Have independent header\n");
					break;
				case 'Q' + ('g' << 8):
					DebugPrint("Have no codebook header\n");
					break;
				case 'o' + ('g' << 8):
					DebugPrint("Original stream compatible\n");
					break;
				case 'p' + ('g' << 8):
					DebugPrint("Have independent header\n");
					break;
				case 'q' + ('g' << 8):
					DebugPrint("Have no codebook header\n");
					break;
			}

			avi->AudioTag =
				((avi->AudioStream / 10 + '0')) |
				((avi->AudioStream % 10 + '0') << 8) |
				('w' << 16) | ('b' << 24);

		} else {
			DebugPrint("Wrong magic %lx\n" _C_ magic);
			CLclose(f);
			free(avi);
			return NULL;
		}

		i += length + 8;
		++stream;
	}

	//
	//  Find movi
	//
	for (;;) {
	    if (CLread(f, buf, 8) != 8) {  // EOF
			DebugPrint("Unexpected eof\n");
			CLclose(f);
			free(avi);
			return NULL;
		}
		magic = AccessLE32(buf);
		length = AccessLE32(buf + 4);
		length = (length + 1) & ~1;  // pad even
		DebugPrint("Magic %lx %ld\n" _C_ magic _C_ length);

		if (magic == 0x4b4e554a) {  // JUNK
			CLseek(f, length, SEEK_CUR);  // skip section
			continue;
		}

		if (magic != 0x5453494C) {  // LIST
			DebugPrint("Wrong magic %lx (not %x)\n" _C_ magic _C_
				0x5453494C);
			CLclose(f);
			free(avi);
			return NULL;
		}

		if (CLread(f, buf, 4) != 4) {  // EOF
			DebugPrint("Unexpected eof\n");
			CLclose(f);
			free(avi);
			return NULL;
		}
		magic = AccessLE32(buf);
		length -= 4;
		DebugPrint("Magic %lx %ld\n" _C_ magic _C_ length);
		if (magic != 0x69766f6d) {  // movi
			DebugPrint("Wrong magic %lx (not %x)\n" _C_ magic _C_
				0x69766f6d);
			CLclose(f);
			free(avi);
			return NULL;
		}
		break;
	}

	avi->VideoFramesTail = &avi->VideoFrames;
	avi->AudioFramesTail = &avi->AudioFrames;

	// Seek point after movi!

	return avi;
}

/**
**  Avi frame handler.
**
**  @param avi     AVI Filehandle.
**  @param video   Video frame wanted
**  @param framep  Frame pointer.
**
**  @return        FIXME: docu
*/
static int AviFrameHandler(AviFile* avi, int video, unsigned char** framep)
{
	CLFile* f;
	unsigned long magic;
	long length;
	char buf[256];
	AviFrameBuffer* afb;

	f = avi->FileHandle;

	for (;;) {
		if (CLread(f, buf, 8) != 8) {  // EOF
			return -1;
		}

		magic = AccessLE32(buf);
		length = AccessLE32(buf + 4);
		length = (length + 1) & ~1;  // pad even FIXME: johns is this good?

		if (magic == avi->AudioTag) {
			if (!length) {
				break;
			}

			if (video) {  // Video wanted
				afb = malloc(length + sizeof(AviFrameBuffer) - 1);
				afb->Next = NULL;

				*avi->AudioFramesTail = afb;
				avi->AudioFramesTail = &afb->Next;
			} else {
				if (avi->AudioBuffer) {
					// FIXME: reuse the buffer!
					free(avi->AudioBuffer);
				}
				avi->AudioBuffer = afb =
					malloc(length + sizeof(AviFrameBuffer) - 1);
				*framep = avi->AudioBuffer->Data;
			}
			afb->Length = length;

			if (CLread(f, afb->Data, length) != length) {  // error
				return -1;
			}

			if (video) {
				continue;
			}
			break;
		}

		if ((magic & 0x00FFFFFF) == avi->VideoTag) {
			if (!length) {
				break;
			}

			if (video) {  // Video wanted
				if (avi->VideoBuffer) {
					// FIXME: reuse the buffer!
					free(avi->VideoBuffer);
				}
				avi->VideoBuffer = afb =
					malloc(length + sizeof(AviFrameBuffer) - 1);
				*framep = avi->VideoBuffer->Data;
			} else {
				afb = malloc(length + sizeof(AviFrameBuffer) - 1);
				afb->Next = NULL;

				*avi->VideoFramesTail = afb;
				avi->VideoFramesTail = &afb->Next;
			}
			afb->Length = length;

			if (CLread(f, afb->Data, length) != length) {  // error
				return -1;
			}

			if (video) {
				break;
			}
			continue;
		}

		if (magic != 0x4b4e554a) {  // JUNK
			DebugPrint("Tag %lx\n" _C_ magic);
		}
		CLseek(f, length, SEEK_CUR);  // skip section
	}

	return length;
}

/**
**  Read next video frame
**
**  @param avi     AVI Filehandle.
**  @param framep  Frame pointer.
**
**  @return        FIXME: docu
*/
int AviReadNextVideoFrame(AviFile* avi, unsigned char** framep)
{
	if (avi->VideoFrames) {
		if (avi->VideoBuffer) {
			free(avi->VideoBuffer);
		}
		avi->VideoBuffer = avi->VideoFrames;
		avi->VideoFrames = avi->VideoFrames->Next;
		if (!avi->VideoFrames) {  // Last buffered frame
			avi->VideoFramesTail = &avi->VideoFrames;
		}
		*framep = avi->VideoBuffer->Data;
		return avi->VideoBuffer->Length;
	}

	return AviFrameHandler(avi, 1, framep);
}

/**
**  Read next audio frame
**
**  @param avi     AVI Filehandle.
**  @param framep  Frame pointer.
**
**  @return        FIXME: docu
*/
int AviReadNextAudioFrame(AviFile* avi, unsigned char** framep)
{
	if (avi->AudioFrames) {
		if (avi->AudioBuffer) {
			free(avi->AudioBuffer);
		}
		avi->AudioBuffer = avi->AudioFrames;
		avi->AudioFrames = avi->AudioFrames->Next;
		if (!avi->AudioFrames) {  // Last buffered frame
			avi->AudioFramesTail = &avi->AudioFrames;
		}
		*framep = avi->AudioBuffer->Data;
		return avi->AudioBuffer->Length;
	}

	return AviFrameHandler(avi, 0, framep);
}

/**
**  Close an open avi file.
**
**  @param avi  AVI Filehandle.
**
**  @return     FIXME: docu
*/
void AviClose(AviFile* avi)
{
	AviFrameBuffer* afb;
	AviFrameBuffer* tmp;

	//
	//  Free the frame buffers
	//
	for (afb = avi->VideoFrames; afb; afb = tmp) {
		tmp = afb->Next;
		free(afb);
	}
	if (avi->VideoBuffer) {
		free(avi->VideoBuffer);
	}
	for (afb = avi->AudioFrames; afb; afb = tmp) {
		tmp = afb->Next;
		free(afb);
	}
	if (avi->AudioBuffer) {
		free(avi->AudioBuffer);
	}

	CLclose(avi->FileHandle);
	free(avi);
}

//@}
