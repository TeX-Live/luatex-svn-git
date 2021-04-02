/* Copyright (C) 2007,2008 by George Williams */
/*
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.

 * The name of the author may not be used to endorse or promote products
 * derived from this software without specific prior written permission.

 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
 * EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "pfaedit.h"
#include "uiinterface.h"

static void NClipboard_Grab(void) {
}

static void NClipboard_AddDataType(const char *type, void *data, int cnt, int size,
	void *(*gendata)(void *,int32 *len), void (*freedata)(void *)) {
  (void)type;
  (void)cnt;
  (void)size;
  (void)gendata;
    if ( freedata!=NULL && data !=NULL )
	(freedata)(data);
}

/* Asks for the clip and waits for the response. */
static void *NClipboard_Request(const char *mimetype,int *len) {
    *len = 0;
  (void)mimetype;
return( NULL );
}

static int NClipboard_HasType(const char *mimetype) {
  (void)mimetype;
return( 0 );
}

static struct clip_interface noui_clip_interface = {
    NClipboard_Grab,
    NClipboard_AddDataType,
    NClipboard_HasType,
    NClipboard_Request
};

struct clip_interface *clip_interface = &noui_clip_interface;

void FF_SetClipInterface(struct clip_interface *clipi) {
    clip_interface = clipi;
}

