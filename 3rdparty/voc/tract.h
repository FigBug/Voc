/* This code is heavily modified (and ported) from code with this license: */

/*
Copyright 2017 Neil Thapen 

Permission is hereby granted, free of charge, to any person obtaining a 
copy of this software and associated documentation files (the "Software"), 
to deal in the Software without restriction, including without limitation 
the rights to use, copy, modify, merge, publish, distribute, sublicense, 
and/or sell copies of the Software, and to permit persons to whom the 
Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in 
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR 
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE 
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER 
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS 
IN THE SOFTWARE.
*/

#ifndef TRACT_H
#define TRACT_H

#include "noise.h"


struct tract_state;


struct tract_state *tract_init(unsigned long sample_rate);
void tract_shutdown(struct tract_state *t);

void
tract_process
	(struct tract_state *t
	,float glottal_output
	,struct noise_state *turbulence_noise
	,float lambda
	,float *lip_output
	,float *nose_output
	);

void tract_finish_block(struct tract_state *t, unsigned block_length);

void tract_set_constriction(struct tract_state *t, float position, float amount);

void tract_set_smoothing(struct tract_state *t, float amount);

#endif
