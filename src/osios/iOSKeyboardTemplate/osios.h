#pragma once
#ifndef _RIBBON_OSIOS_H_
#define _RIBBON_OSIOS_H_

//#include <errno.h>

#include <unistd.h>
#include <sys/resource.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>

/*
#include <EGL/egl.h>
#include <GLES/gl.h>
*/

namespace Ribbon { namespace iOS {
extern std::string s_resourceRoot;
} }

#endif // _RIBBON_OSIOS_H_
