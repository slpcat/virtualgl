/* Copyright (C)2004 Landmark Graphics
 *
 * This library is free software and may be redistributed and/or modified under
 * the terms of the wxWindows Library License, Version 3 or (at your option)
 * any later version.  The full license is in the LICENSE.txt file included
 * with this distribution.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * wxWindows Library License for more details.
 */

#include "rrdisplayclient.h"
#include "fakerconfig.h"

static FakerConfig rrfconfig;

// C wrappers

#define checkdpyhandle(h,s,r) rrdisplayclient *rrc; \
	if((rrc=(rrdisplayclient *)h)==NULL) { \
		_throwlasterror("Invalid handle in "s);  return r; \
	}

#define checkframehandle(h,s,r) rrframe *f=NULL;  rrjpeg *j=NULL; \
	if(!frame) {_throwlasterror("Invalid argument in "s);  return r;} \
	if(frame->compressed) { \
		j=(rrjpeg *)frame->_opaque; \
		if(!j) {_throwlasterror("Invalid handle in "s);  return r;} \
	} else { \
		f=(rrframe *)frame->_opaque; \
		if(!f) {_throwlasterror("Invalid handle in "s);  return r;} \
	}

#define setframe(frame, f) { \
	frame->_opaque=(void *)f; \
	frame->h=f->_h; \
	frame->bits=f->_bits;}

#define _throwlasterror(m) __lasterror.init(__FILE__, (char *)m, __LINE__);

#define _catch() catch(rrerror &e) {__lasterror=e;}

rrerror __lasterror;



extern "C" {

DLLEXPORT RRDisplay DLLCALL
	RROpenDisplay(char *displayname, unsigned short port, int ssl, int numprocs)
{
	rrdisplayclient *rrc=NULL;
	try
	{
		rrc=new rrdisplayclient(displayname, port, ssl, numprocs);
		if(!rrc) _throw("Could not allocate memory for class");
		return (RRDisplay)rrc;
	}
	catch(rrerror &e)
	{
		__lasterror=e;
		if(rrc) delete rrc;
	}
	return NULL;
}

DLLEXPORT int DLLCALL
	RRCloseDisplay(RRDisplay rrdpy)
{
	checkdpyhandle(rrdpy, "RRCloseDisplay()", RR_ERROR);
	try
	{
		delete rrc;
		return RR_SUCCESS;
	}
	_catch()
	return RR_ERROR;
}

DLLEXPORT int DLLCALL
	RRGetFrame(RRDisplay rrdpy, int width, int height, int pixelformat,
		int bottomup, RRFrame *frame)
{
	checkdpyhandle(rrdpy, "RRGetFrame()", RR_ERROR);
	try
	{
		rrframe *f=NULL;
		if(width<1 || height<1 || !frame) _throw("Invalid argument");
		int pixelsize=3, flags=0;
		switch(pixelformat)
		{
			case RR_RGB:   pixelsize=3;  flags=0;  break;
			case RR_RGBA:  pixelsize=4;  flags=0;  break;
			case RR_BGR:   pixelsize=3;  flags=RRBMP_BGR;  break;
			case RR_BGRA:  pixelsize=4;  flags=RRBMP_BGR;  break;
			case RR_ABGR:  pixelsize=4;  flags=RRBMP_BGR|RRBMP_ALPHAFIRST;  break;
			case RR_ARGB:  pixelsize=4;  flags=RRBMP_ALPHAFIRST;  break;
			default: _throw("Invalid argument");
		}
		memset(frame, 0, sizeof(RRFrame));
		errifnot(f=rrc->getbitmap(width, height, pixelsize));
		if(bottomup) flags|=RRBMP_BOTTOMUP;
		f->_flags=flags;
		setframe(frame, f);
		return RR_SUCCESS;
	}
	_catch();
	return RR_ERROR;
}

DLLEXPORT int DLLCALL
	RRCompressFrame(RRDisplay rrdpy, RRFrame *frame)
{
	checkdpyhandle(rrdpy, "RRCompressFrame()", RR_ERROR);
	checkframehandle(frame, "RRCompressFrame()", RR_ERROR);
	if(frame->compressed) return RR_SUCCESS;
	try
	{
		j=new rrjpeg();
		if(!j) _throw("Could not allocate compressed image structure");
		f->_h=frame->h;
		*j=*f;
		rrc->release(f);
		setframe(frame, j);
		frame->compressed=1;
		return RR_SUCCESS;
	}
	_catch();
	return RR_ERROR;
}

DLLEXPORT int DLLCALL
	RRReleaseFrame(RRDisplay rrdpy, RRFrame *frame)
{
	checkdpyhandle(rrdpy, "RRReleaseFrame()", RR_ERROR);
	checkframehandle(frame, "RRReleaseFrame()", RR_ERROR);
	try
	{
		if(frame->compressed) {if(j) delete j;}
		else rrc->release(f);
		memset(frame, 0, sizeof(RRFrame));
		return RR_SUCCESS;
	}
	_catch()
	return RR_ERROR;
}

DLLEXPORT int DLLCALL
	RRFrameReady(RRDisplay rrdpy)
{
	checkdpyhandle(rrdpy, "RRFrameReady()", RR_ERROR);
	try
	{
		return rrc->frameready()?1:0;
	}
	_catch()
	return RR_ERROR;
}

DLLEXPORT int DLLCALL
	RRSendFrame(RRDisplay rrdpy, RRFrame *frame)
{
	checkdpyhandle(rrdpy, "RRSendFrame()", RR_ERROR);
	checkframehandle(frame, "RRSendFrame()", RR_ERROR);
	try
	{
		if(frame->compressed)
		{
			j->_h=frame->h;
			rrc->sendcompressedframe(frame->h, frame->bits);
			delete j;
		}
		else
		{
			f->_h=frame->h;
			rrc->sendframe(f);
		}
		memset(frame, 0, sizeof(RRFrame));
		return RR_SUCCESS;
	}
	_catch()
	return RR_ERROR;
}

DLLEXPORT int DLLCALL
	RRSendRawFrame(RRDisplay rrdpy, rrframeheader *h, unsigned char *bits)
{
	checkdpyhandle(rrdpy, "RRSendRawFrame()", RR_ERROR);
	try
	{
		if(!h || !bits) _throw("Invalid argument");
		rrc->sendcompressedframe(*h, bits);
		return RR_SUCCESS;
	}
	_catch()
	return RR_ERROR;
}

DLLEXPORT int DLLCALL
	RRGetFakerConfig(fakerconfig *fc)
{
	try
	{
		if(!fc) _throw("Invalid argument");
		fc->client=rrfconfig.client;
		fc->server=rrfconfig.localdpystring;
		fc->qual=rrfconfig.currentqual;
		fc->subsamp=rrfconfig.currentsubsamp;
		fc->port=rrfconfig.port;
		fc->spoil=rrfconfig.spoil;
		fc->ssl=rrfconfig.ssl;
		fc->numprocs=rrfconfig.np;
		return RR_SUCCESS;
	}
	_catch()
	return RR_ERROR;
}

DLLEXPORT char * DLLCALL
	RRErrorString(void)
{
	return __lasterror? __lasterror.getMessage():(char *)"No Error";
}

DLLEXPORT const char * DLLCALL
	RRErrorLocation(void)
{
	return __lasterror? __lasterror.getMethod():"(Unknown error location)";
}

}
