/*

	Copyright (C) 1991-2001 and beyond by Bungie Studios, Inc.
	and the "Aleph One" developers.
 
	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	This license is contained in the file "COPYING",
	which is included with this source code; it is available online at
	http://www.gnu.org/licenses/gpl.html

	Alias|Wavefront Object Loader
	
	By Loren Petrich, June 16, 2001
*/
#ifndef WAVEFRONT_LOADER
#define WAVEFRONT_LOADER

#include <stdio.h>
#include "Model3D.h"
#include "FileHandler.h"

bool LoadModel_Wavefront(FileSpecifier& Spec, Model3D& Model);

// Where to emit status messages
void SetDebugOutput_Wavefront(FILE *DebugOutput);

#endif
