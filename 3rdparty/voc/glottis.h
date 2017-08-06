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

#ifndef GLOTTIS_H
#define GLOTTIS_H

#include "noise.h"

struct glottis_state;

struct glottis_state *glottis_init(unsigned long sample_rate, unsigned seed);
void glottis_shutdown(struct glottis_state *g);

float glottis_process(struct glottis_state *g, float lambda, struct noise_state *noise_source, float pm);

void glottis_finish_block(struct glottis_state *g, int auto_wobble, int always_voice);

void glottis_set_target_frequency(struct glottis_state *g, float f);
void glottis_set_target_tenseness(struct glottis_state *g, float t);

#endif
