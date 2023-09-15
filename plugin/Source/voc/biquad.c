#include "biquad.h"

#include <math.h>
#include <stdlib.h>

#define PI (3.14159265359)

struct biquad_state
{
	float A;
	float w0;
	float a_q;
	float a_q_db;
	float S;
	float a_s;
	
	float b0;
	float b1;
	float b2;
	
	float a0;
	float a1;
	float a2;
	
	float x1; /* x[-1] */
	float x2; /* x[-2] */
	
	float y1; /* y[-1] */
	float y2; /* y[-2] */
};

static
struct biquad_state *
biquad_init
	(float sample_rate
	,float frequency
	,float gain
	,float Q
	)
{
	struct biquad_state *b = malloc(sizeof b[0]);
	
	b->A = pow(10.0f, gain / 40.0f);
	b->w0 = 2.0f * PI * frequency / sample_rate;
	b->a_q = sin(b->w0) / 2 * Q;
	b->a_q_db = sin(b->w0) / 2.0f * pow(10.0f, Q / 20.0f);
	b->S = 1.0f;
	b->a_s = (sin(b->w0) / 2.0f) * sqrt((b->A + (1.0f / b->A)) * ((1.0f / b->S) - 1.0f) + 2.0f);
	
	return b;
}



struct biquad_state *
biquad_init_bandpass
	(float sample_rate
	,float frequency
	,float gain
	,float Q
	)
{
	struct biquad_state *b = biquad_init(sample_rate, frequency, gain, Q);
	
	b->b0 = b->a_q;
	b->b1 = 0.0f;
	b->b2 = -b->a_q;
	
	b->a0 = 1.0f + b->a_q;
	b->a1 = -2.0f * cos(b->w0);
	b->a2 = 1.0f - b->a_q;
	
	return b;
}

void biquad_shutdown(struct biquad_state *b)
{
	free(b);
}

float biquad_process(struct biquad_state *b, float x0)
{
	/* Stupid direct form implementation */
	float y0 = (b->b0 / b->a0) * x0
	         + (b->b1 / b->a0) * b->x1 + (b->b2 / b->a0) * b->x2
	         - (b->a1 / b->a0) * b->y1 - (b->a2 / b->a0) * b->y2;
	
	b->y2 = b->y1;
	b->y1 = y0;
	
	b->x2 = b->x1;
	b->x1 = x0;
	
	return y0;
}

