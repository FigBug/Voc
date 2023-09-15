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

#include "glottis.h"
#include "simplex.h"

#include <stdlib.h>
#include <math.h>
#include <assert.h>

#define PI (3.14159265359)

struct glottis_state
{
	struct simplex_state *simplex;
	float time_step;
	
	float time_in_waveform;
	float target_frequency;
	float old_frequency;
	float new_frequency;
	float smooth_frequency;
	
	float old_tenseness;
	float new_tenseness;
	float target_tenseness;
	
	float total_time;
	float vibrato_amount;
	float vibrato_frequency;
	
	float intensity;
	float loudness;
	
	int is_touched;
	
	float waveform_length;
	
	float Te;
	float epsilon;
	float shift;
	float Delta;
	float E0;
	float alpha;
	float omega;
};

static float vmin(float a, float b)
{
	if (a < b) {
		return a;
	}
	return b;
}

static float vmax(float a, float b)
{
	if (a < b) {
		return b;
	}
	return a;
}

static float clamp(float n, float minimum, float maximum)
{
	return vmin(vmax(n, minimum), maximum);
}

static float normalized_lf_waveform(struct glottis_state *g, float t)
{
	float output;
	if (t > g->Te) {
		output = (-exp(-g->epsilon * (t - g->Te)) + g->shift) / g->Delta;
	}
	else {
		output = g->E0 * exp(g->alpha * t) * sin(g->omega * t);
	}
	return output * g->intensity * g->loudness;
}


static void setup_waveform(struct glottis_state *g, float lambda)
{
	float tenseness;
	float Rd;
	float Ra;
	float Rk;
	float Rg;
	float Ta;
	float Tp;
	float rhs_integral;
	float total_lower_integral;
	float total_upper_integral;
	float s;
	float y;
	float z;
	
	float frequency = g->old_frequency * (1.0f - lambda) + g->new_frequency * lambda;
	tenseness = g->old_tenseness * (1.0f - lambda) + g->new_tenseness * lambda;
	
	Rd = 3.0f * (1.0f - tenseness);
	g->waveform_length = 1.0f / frequency;
	
	Rd = clamp(Rd, 0.5f, 2.7f);
	
	Ra = -0.01f + 0.048 * Rd;
	Rk = 0.224 + 0.118 * Rd;
	Rg = (Rk / 4.0f) * (0.5f + 1.2f * Rk) / (0.11f * Rd - Ra * (0.5f + 1.2f * Rk));
	
	Ta = Ra;
	Tp = 1.0f / (2.0f * Rg);
	g->Te = Tp + Tp * Rk;
	
	g->epsilon = 1.0f / Ta;
	g->shift = exp(-g->epsilon * (1.0f - g->Te));
	g->Delta = 1.0f - g->shift;
	
	rhs_integral = (1.0f / g->epsilon) * (g->shift - 1.0f) + (1.0f - g->Te) * g->shift;
	rhs_integral /= g->Delta;
	
	total_lower_integral = -(g->Te - Tp) / 2.0f + rhs_integral;
	total_upper_integral = -total_lower_integral;
	
	g->omega = PI / Tp;
	s = sin(g->omega * g->Te);
	
	y = -PI * s * total_upper_integral / (Tp * 2.0f);
	z = log(y);
	g->alpha = z / (Tp / 2.0f - g->Te);
	g->E0 = -1.0f / (s * exp(g->alpha * g->Te));
	
}

struct glottis_state *glottis_init(unsigned long sample_rate, unsigned seed)
{
	struct glottis_state *g = malloc(sizeof g[0]);
	
	g->time_in_waveform = 0.0f;
	g->old_frequency = 140.0f;
	g->new_frequency = 140.0f;
	g->target_frequency = 140.0f;
	g->smooth_frequency = 140.0f;
	
	g->old_tenseness = 0.6f;
	g->new_tenseness = 0.6f;
	g->target_tenseness = 0.6f;
	
	g->time_in_waveform = 0.0f;
	g->total_time = 0.0f;
	g->vibrato_amount = 0.005f;
	g->vibrato_frequency = 6.0f;
	
	g->intensity = 0.0f;
	g->loudness = 1.0f;
	
	g->is_touched = 0;
	
	setup_waveform(g, 0.0f);
	
	g->time_step = 1.0f / (float)sample_rate;
	
	g->simplex = simplex_init(seed);
	
	return g;
}

void glottis_shutdown(struct glottis_state *g)
{
	simplex_shutdown(g->simplex);
	free(g);
}


void glottis_set_target_frequency(struct glottis_state *g, float f)
{
	g->target_frequency = f;
	assert(f > 0.0f);
}

void glottis_set_target_tenseness(struct glottis_state *g, float t)
{
	g->target_tenseness = t;
	assert(t >= 0.0f);
	assert(t <= 1.0f);
}

static float get_noise_modulator(const struct glottis_state *g)
{
	float voiced = 0.1 + 0.2 * vmax(0.0f, sin(PI * 2.0f * g->time_in_waveform / g->waveform_length));
	return g->target_tenseness * g->intensity * voiced + (1.0f - g->target_tenseness * g->intensity) * 0.3f;
}

float glottis_process(struct glottis_state *g, float lambda, struct noise_state *noise_source, float pm)
{
	float out;
	float aspiration;
	g->time_in_waveform += g->time_step + (pm * g->waveform_length);
	g->total_time += g->time_step;
	if (g->time_in_waveform > g->waveform_length)
	{
		g->time_in_waveform -= g->waveform_length;
		setup_waveform(g, lambda);
	}
	
	out = normalized_lf_waveform(g, g->time_in_waveform / g->waveform_length);
	aspiration = g->intensity * (1.0f - sqrt(g->target_tenseness)) * get_noise_modulator(g) * noise_next(noise_source);
	aspiration *= 0.2f + 0.02f * simplex1(g->simplex, g->total_time * 1.99f);
	
	out += aspiration;
	return out;
}

void glottis_finish_block(struct glottis_state *g, int auto_wobble, int always_voice)
{
	float vibrato = 0.0f;
	vibrato += g->vibrato_amount * sin(2.0f * PI * g->total_time * g->vibrato_frequency);
	vibrato += 0.02f * simplex1(g->simplex, g->total_time * 4.07);
	vibrato += 0.04f * simplex1(g->simplex, g->total_time * 2.15);
	
	if (auto_wobble) {
		vibrato += 0.2 * simplex1(g->simplex, g->total_time * 0.98);
		vibrato += 0.4 * simplex1(g->simplex, g->total_time * 0.5);
	}
	
	if (g->target_frequency > g->smooth_frequency) {
		g->smooth_frequency = vmin(g->smooth_frequency * 1.1f, g->target_frequency);
	}
	if (g->target_frequency < g->smooth_frequency) {
		g->smooth_frequency = vmax(g->smooth_frequency / 1.1f, g->target_frequency);
	}
	
	g->old_frequency = g->new_frequency;
	g->new_frequency = g->smooth_frequency * (1.0f + vibrato);
	
	g->old_tenseness = g->new_tenseness;
	g->new_tenseness = g->target_tenseness + 0.1f * simplex1(g->simplex, g->total_time * 0.46f) + 0.05f * simplex1(g->simplex, g->total_time * 0.36f);
	
	if (!g->is_touched && always_voice) {
		g->intensity += 0.13;
	}
	else {
		g->intensity -= 0.05;
	}
	g->intensity = clamp(g->intensity, 0.0f, 1.0f);
}


