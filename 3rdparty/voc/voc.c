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

#include "voc.h"
#include "glottis.h"
#include "tract.h"

#include <stdlib.h>
#include <math.h>


struct voc_state
{
	unsigned block_length;
	float block_time;
	
	float time;
	float temp_a;
	float temp_b;
	
	int always_voice;
	int auto_wobble;
	
	float constriction_position;
	float constriction_amount;
	
	struct noise_state *glottis_noise;
	struct noise_state *tract_noise;
	
	struct glottis_state *glottis;
	struct tract_state *tract;
	
	unsigned t;
};

struct voc_state *voc_init(unsigned long sample_rate, unsigned seed)
{
	struct voc_state *v = malloc(sizeof v[0]); 
	v->block_length = 512;
	v->block_time = (float)v->block_length / (float)sample_rate;
	v->time = 0.0f;
	v->temp_a = 0.0f;
	v->temp_b = 0.0f;
	v->always_voice = 0;
	v->auto_wobble = 0; /* TODO: This seems to be broken */
	
	v->glottis_noise = noise_init(sample_rate, 500.0, 0.5, 1000.0, 0.5);
	v->tract_noise = noise_init(sample_rate, 500.0, 0.5, 1000.0, 0.5);
	
	v->glottis = glottis_init(sample_rate, seed);
	v->tract = tract_init(sample_rate);
	
	v->constriction_position = 0.5f;
	v->constriction_amount = 0.0f;
	return v;
}


void voc_shutdown(struct voc_state *v)
{
	noise_shutdown(v->glottis_noise);
	noise_shutdown(v->tract_noise);
	
	glottis_shutdown(v->glottis);
	tract_shutdown(v->tract);
	
	free(v);
}

void voc_tenseness_set(struct voc_state *voc, float t)
{
	glottis_set_target_tenseness(voc->glottis, t);
}

void voc_constriction_position_set(struct voc_state *voc, float position)
{
	voc->constriction_position = position;
	tract_set_constriction(voc->tract, voc->constriction_position, voc->constriction_amount);
}

void voc_constriction_amount_set(struct voc_state *voc, float amount)
{
	voc->constriction_amount = amount;
	tract_set_constriction(voc->tract, voc->constriction_position, voc->constriction_amount);
}

void voc_smoothing_set(struct voc_state *voc, float amount)
{
	tract_set_smoothing(voc->tract, amount);
}

static
float
note_to_freq
	(int note
	)
{
	return pow(2.0, (note - 69) / 12.0) * 440.0f;
}


void voc_note_on(struct voc_state *v, int note, int velocity)
{
	v->always_voice = 1;
	glottis_set_target_frequency(v->glottis, note_to_freq(note));
}

void voc_note_off(struct voc_state *v, int velocity)
{
	v->always_voice = 0;
}

float voc_f(struct voc_state *v, float pm)
{
	float inv_length = 1.0f / (float)v->block_length;
	float lambda1 = (float)(v->t) * inv_length;
	float lambda2 = ((float)(v->t) + 0.5f) * inv_length;
	float glottal_output = glottis_process(v->glottis, lambda1, v->glottis_noise, pm);
	float vocal_output = 0.0f;
	float lip_output;
	float nose_output;
	float out;
	
	tract_process(v->tract, glottal_output, v->tract_noise, lambda1, &lip_output, &nose_output);
	vocal_output += lip_output + nose_output;
	tract_process(v->tract, glottal_output, v->tract_noise, lambda2, &lip_output, &nose_output);
	vocal_output += lip_output + nose_output;
	
	out = vocal_output * 0.125f;
	
	v->t++;
	if (v->t >= v->block_length)
	{
		glottis_finish_block(v->glottis, v->auto_wobble, v->always_voice);
		tract_finish_block(v->tract, v->block_length);
		v->t = 0;
	}
	
	return out;
} 

