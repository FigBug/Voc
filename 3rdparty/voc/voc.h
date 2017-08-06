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

#ifndef VOC_H
#define VOC_H

struct voc_state;

struct voc_state *voc_init(unsigned long sample_rate, unsigned seed);
void voc_shutdown(struct voc_state *v);

void voc_note_on(struct voc_state *v, int note, int velocity);
void voc_note_off(struct voc_state *v, int velocity);

float voc_f(struct voc_state *voc, float pm);

void voc_tenseness_set(struct voc_state *voc, float t);
void voc_constriction_position_set(struct voc_state *voc, float position);
void voc_constriction_amount_set(struct voc_state *voc, float amount);

void voc_smoothing_set(struct voc_state *voc, float amount);

#endif
   

