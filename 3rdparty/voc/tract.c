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

#include "tract.h"

#include <stdlib.h>
#include <assert.h>
#include <math.h>

#define N (44)
#define NOSE_LENGTH ((28 * N) / 44)

#define GLOTTAL_REFLECTION (0.75f)
#define LIP_REFLECTION (-0.85f)
#define FADE (1.0f)
#define MOVEMENT_SPEED (15.0f)
#define VELUM_TARGET (0.01f)



#define NOSE_START (N - NOSE_LENGTH + 1)
#define TIP_START ((int)(32.0f * N / 44.0f))

#define MAX_TRANSIENTS (N)

struct transient
{
	int position;
	float time_alive;
	float life_time;
	float strength;
	float exponent;
};

struct tract_state
{
	float inv_sample_rate;
	
	float R[N];
	float L[N];
	float reflection[N + 1];
	float new_reflection[N + 1];
	float junction_output_R[N + 1];
	float junction_output_L[N + 1];
	float diameter[N];
	float rest_diameter[N];
	float slow_target_diameter[N];
	float fast_target_diameter[N];
	float new_diameter[N];
	float A[N];
	
	float reflection_left;
	float reflection_right;
	float reflection_nose;
	float new_reflection_left;
	float new_reflection_right;
	float new_reflection_nose;
	
	int last_obstruction;
	
	unsigned num_transients;
	struct transient transients[MAX_TRANSIENTS];
	
	float nose_R[NOSE_LENGTH];
	float nose_L[NOSE_LENGTH];
	float nose_junction_output_R[NOSE_LENGTH + 1];
	float nose_junction_output_L[NOSE_LENGTH + 1];
	float nose_reflection[NOSE_LENGTH + 1];
	float nose_diameter[NOSE_LENGTH];
	float nose_A[NOSE_LENGTH];
	
	float target_constriction_amount;
	float constriction_amount;
	float target_constriction_position;
	float constriction_position;
	float smoothing;
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

static float move_towards_bi(float current, float target, float amount_up, float amount_down)
{
	
	if (current < target) {
		return vmin(current + amount_up, target);
	}
	return vmax(current - amount_down, target); 
}

static void calculate_reflections(struct tract_state *t)
{
	int i;
	float sum;
	
	for (i = 0; i < N; i++) {
		t->A[i] = t->diameter[i] * t->diameter[i];
	}
	for (i = 1; i < N; i++) {
		t->reflection[i] = t->new_reflection[i];
		if (t->A[i] == 0) {
			t->new_reflection[i] = 0.999f;
		}
		else {
			t->new_reflection[i] = (t->A[i - 1] - t->A[i]) / (t->A[i - 1] + t->A[i]);
		}
	}
	t->reflection_left = t->new_reflection_left;
	t->reflection_right = t->new_reflection_right;
	t->reflection_nose = t->new_reflection_nose;
	
	sum = t->A[NOSE_START] + t->A[NOSE_START + 1] + t->nose_A[0];
	
	t->new_reflection_left = (2.0f * t->A[NOSE_START] - sum) / sum;
	t->new_reflection_right = (2.0f * t->A[NOSE_START + 1] - sum) / sum;
	t->new_reflection_nose = (2.0f * t->nose_A[0] - sum) / sum;
}


static void calculate_nose_reflections(struct tract_state *t)
{
	int i;
	for (i = 0; i < NOSE_LENGTH; i++) {
		t->nose_A[i] = t->nose_diameter[i] * t->nose_diameter[i];
	}
	for (i = 1; i < NOSE_LENGTH; i++) {
		t->nose_reflection[i] = (t->nose_A[i - 1] - t->nose_A[i]) / (t->nose_A[i - 1] + t->nose_A[i]);
	}
}


struct tract_state *tract_init(unsigned long sample_rate)
{
	struct tract_state *t = malloc(sizeof t[0]);
	unsigned i;
	
	t->inv_sample_rate = 1.0f / (float)sample_rate;
	
	t->last_obstruction = -1;
	t->num_transients = 0;
	
	for (i = 0; i < N; i++) {
		float diameter;
		t->R[i] = 0.0f;
		t->L[i] = 0.0f;
		t->A[i] = 0.0f;
		
		if (i < 7 * (N / 44.0f - 0.5f)) {
			diameter = 0.6f;
		}
		else if (i < 12 * N / 44.0f - 0.5f) {
			diameter = 1.1f;
		}
		else {
			diameter = 1.5f;
		}
		
		t->diameter[i] = diameter;
		t->rest_diameter[i] = diameter;
		t->fast_target_diameter[i] = diameter;
		t->slow_target_diameter[i] = diameter;
		t->new_diameter[i] = diameter;
	}
	
	for (i = 0; i < N + 1; i++) {
		t->reflection[i] = 0.0f;
		t->new_reflection[i] = 0.0f;
		t->junction_output_R[i] = 0.0f;
		t->junction_output_L[i] = 0.0f;
	}
	
	
	for (i = 0; i < NOSE_LENGTH; i++) {
		float diameter;
		float d = 2.0f * ((float)i / (float)NOSE_LENGTH);
		if (d < 1.0f) {
			diameter = 0.4f + 1.6f * d;
		}
		else {
			diameter = 0.5f + 1.5f * (2.0f - d);
		}
		diameter = vmin(diameter, 1.9f);
		t->nose_diameter[i] = diameter;
		
		t->nose_R[i] = 0.0f;
		t->nose_L[i] = 0.0f;
		t->nose_A[i] = 0.0f;
	}
	
	for (i = 0; i < NOSE_LENGTH + 1; i++) {
		t->nose_junction_output_R[i] = 0.0f;
		t->nose_junction_output_L[i] = 0.0f;
		t->nose_reflection[i] = 0.0f;
	}
	
	t->new_reflection_left = 0.0f;
	t->new_reflection_right = 0.0f;
	t->new_reflection_nose = 0.0f;
	
	calculate_reflections(t);
	calculate_nose_reflections(t);
	
	t->nose_diameter[0] = VELUM_TARGET;
	
	t->smoothing = 1.0f;
	t->target_constriction_amount = 0.0f;
	t->target_constriction_position = 0.5f;
	t->constriction_amount = t->target_constriction_amount;
	t->constriction_position = t->target_constriction_position;
	
	return t;
}

void tract_shutdown(struct tract_state *t)
{
	free(t);
}

void tract_set_smoothing(struct tract_state *t, float amount)
{
	assert(amount >= 0.0f && amount <= 1.0f);
	
	t->smoothing = amount;
}

void tract_set_constriction(struct tract_state *t, float position, float amount)
{
	assert(position >= 0.0f && position <= 1.0f);
	assert(amount >= 0.0f && amount <= 1.0f);
	
	t->constriction_amount = amount;
	t->constriction_position = position;
}

static float smooth_change(float v, float target, float smoothing, float lambda)
{
	float a = lambda * 1.0f;
	float unsmoothed = move_towards_bi(v, target, a, a);
	
	return (unsmoothed * smoothing) + (target * (1.0f - smoothing));
}

static void update_fast_target_diameter(struct tract_state *t, unsigned block_length)
{
	float lambda = (float)block_length * t->inv_sample_rate;
	unsigned i;
	
	for (i = 0; i < N; i++) {
		t->fast_target_diameter[i] =
			smooth_change
				(t->fast_target_diameter[i]
				,t->slow_target_diameter[i]
				,t->smoothing
				,lambda
				);
	}
}

static void update_constriction(struct tract_state *t, unsigned block_length)
{
	unsigned i;
	float y[N];
	float total_y = 0.0f;
	
	update_fast_target_diameter(t, block_length);
	
	for (i = 0; i < N; i++) {
		float fi = (float)i / (float)(N - 1);
		float d2 = (fi - t->constriction_position) * (fi - t->constriction_position);
		float scale = 1.0f / (0.01f + sqrt(d2));
		y[i] = scale;
		total_y += scale;
	}
	for (i = 0; i < N; i++) {
		float norm_y = y[i] / total_y;
		float shrink = 1.0f - (norm_y * t->constriction_amount);
		
		shrink = shrink * shrink;
		shrink = shrink * shrink;
		shrink = shrink * shrink;
		t->slow_target_diameter[i] = t->rest_diameter[i] * shrink;
	}
}

static void add_transient(struct tract_state *t, unsigned position)
{
	if (t->num_transients >= MAX_TRANSIENTS) {
		return;
	}
	
	t->transients[t->num_transients].position = position;
	t->transients[t->num_transients].time_alive = 0.0f;
	t->transients[t->num_transients].life_time = 0.2f;
	t->transients[t->num_transients].strength = 0.3f;
	t->transients[t->num_transients].exponent = 200.0f;
	
	t->num_transients++;
}

static void process_transients(struct tract_state *t) {
	unsigned i;
	for (i = 0; i < t->num_transients; i++) {
		struct transient *tt = &(t->transients[i]);
		float amplitude = tt->strength * pow(2.0f, -tt->exponent * tt->time_alive);
		
		t->R[tt->position] += amplitude / 2.0f;
		t->L[tt->position] += amplitude / 2.0f;
		
		tt->time_alive += t->inv_sample_rate * 0.5f;
	}
	
	for (i = 0; i < t->num_transients; i++) {
		if (t->transients[i].time_alive > t->transients[i].life_time) {
			t->num_transients--;
			t->transients[i] = t->transients[t->num_transients];
			i--;
		}
	}
}

static void add_turbulence_noise(struct tract_state *t, struct noise_state *n)
{
	
}


static void reshape_tract(struct tract_state *t, float delta_time)
{
	float amount = delta_time * MOVEMENT_SPEED;
	int new_last_obstruction = -1;
	int i;
	
	for (i = 0 ; i < N; i++) {
		float diameter = t->diameter[i];
		float fast_target_diameter = t->fast_target_diameter[i];
		float slow_return;
		
		if (diameter <= 0.0f) {
			new_last_obstruction = i;
		}
		
		if (i < NOSE_START) {
			slow_return = 0.6f;
		}
		else if (i >= TIP_START) {
			slow_return = 1.0f;
		}
		else {
			slow_return = 0.6f + 0.4f * (i - NOSE_START) / (float)(TIP_START - NOSE_START);
		}
		t->diameter[i] = move_towards_bi(diameter, fast_target_diameter, slow_return * amount, 2.0f * amount);
	}
	
	if (t->last_obstruction > -1 && new_last_obstruction == -1 && t->nose_A[0] < 0.05f) {
		add_transient(t, t->last_obstruction);
	}
	t->last_obstruction = new_last_obstruction;
	
	t->nose_diameter[0] = move_towards_bi(t->nose_diameter[0], VELUM_TARGET, amount * 0.25f, amount * 0.1f);
	t->nose_A[0] = t->nose_diameter[0] * t->nose_diameter[0];
}

void
tract_process
	(struct tract_state *t
	,float glottal_output
	,struct noise_state *turbulence_noise
	,float lambda
	,float *lip_output
	,float *nose_output
	)
{
	float r;
	int i;
	
	process_transients(t);
	add_turbulence_noise(t, turbulence_noise);
	
	t->junction_output_R[0] = t->L[0] * GLOTTAL_REFLECTION + glottal_output;
	t->junction_output_L[N] = t->R[N - 1] * LIP_REFLECTION;
	
	for (i = 1; i < N; i++) {
		r = t->reflection[i] * (1.0f - lambda) + t->new_reflection[i] * lambda;
		float w = r * (t->R[i - 1] + t->L[i]);
		t->junction_output_R[i] = t->R[i - 1] - w;
		t->junction_output_L[i] = t->L[i] + w;
	}
	
	/* Now at junction with nose */
	i = NOSE_START;
	r = t->new_reflection_left * (1.0f - lambda) + t->reflection_left * lambda;
	t->junction_output_L[i] = r * t->R[i - 1] + (1.0f + r) * (t->nose_L[0] + t->L[i]);
	r = t->new_reflection_right * (1.0f - lambda) + t->reflection_right * lambda;
	t->junction_output_R[i] = r * t->L[i] + (1.0f + r) * (t->R[i - 1] + t->nose_L[0]);
	r = t->new_reflection_nose * (1.0f - lambda) + t->reflection_nose * lambda;
	t->nose_junction_output_R[0] = r * t->nose_L[0] + (1.0f + r) * (t->L[i] + t->R[i - 1]);
	
	for (i = 0; i < N; i++) {
		t->R[i] = t->junction_output_R[i] * 0.999f;
		t->L[i] = t->junction_output_L[i + 1] * 0.999f;
		
	}
	
	*lip_output = t->R[N - 1];
	
	/* Nose */
	t->nose_junction_output_L[NOSE_LENGTH] = t->nose_R[NOSE_LENGTH - 1] * LIP_REFLECTION;
	
	for (i = 1; i < NOSE_LENGTH; i++) {
		float w = t->nose_reflection[i] * (t->nose_R[i - 1] + t->nose_L[i]);
		t->nose_junction_output_R[i] = t->nose_R[i - 1] - w;
		t->nose_junction_output_L[i] = t->nose_L[i] + w;
	}
	
	for (i =0; i < NOSE_LENGTH; i++) {
		t->nose_R[i] = t->nose_junction_output_R[i] * FADE;
		t->nose_L[i] = t->nose_junction_output_L[i + 1] * FADE;
	}
	
	*nose_output = t->nose_R[NOSE_LENGTH - 1];
}

void tract_finish_block(struct tract_state *t, unsigned block_length) {
	update_constriction(t, block_length);
	reshape_tract(t, block_length);
	calculate_reflections(t);
}


#if 0
var Tract = 
{
    
    addTurbulenceNoise : function(turbulenceNoise)
    {
        for (var j=0; j<UI.touchesWithMouse.length; j++)
        {
            var touch = UI.touchesWithMouse[j];
            if (touch.index<2 || touch.index>Tract.n) continue;
            if (touch.diameter<=0) continue;            
            var intensity = touch.fricative_intensity;
            if (intensity == 0) continue;
            this.addTurbulenceNoiseAtIndex(0.66*turbulenceNoise*intensity, touch.index, touch.diameter);
        }
    },
    
    addTurbulenceNoiseAtIndex : function(turbulenceNoise, index, diameter)
    {   
        var i = Math.floor(index);
        var delta = index - i;
        turbulenceNoise *= Glottis.getNoiseModulator();
        var thinness0 = Math.clamp(8*(0.7-diameter),0,1);
        var openness = Math.clamp(30*(diameter-0.3), 0, 1);
        var noise0 = turbulenceNoise*(1-delta)*thinness0*openness;
        var noise1 = turbulenceNoise*delta*thinness0*openness;
        this.R[i+1] += noise0/2;
        this.L[i+1] += noise0/2;
        this.R[i+2] += noise1/2;
        this.L[i+2] += noise1/2;
    }
};
#endif

