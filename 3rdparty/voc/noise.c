#include "noise.h"
#include "biquad.h"
#include <stdlib.h>


struct noise_state
{
	struct biquad_state *aspirate_filter;
	struct biquad_state *fricative_filter;
};

struct noise_state *
noise_init
	(unsigned long sample_rate
	,float f1
	,float q1
	,float f2
	,float q2
	)
{
	struct noise_state *n = malloc(sizeof n[0]);
	
	n->aspirate_filter = biquad_init_bandpass(sample_rate, f1, 1.0f, q1);
	n->fricative_filter = biquad_init_bandpass(sample_rate, f2, 1.0f, q2);
	
	return n;
}

void noise_shutdown(struct noise_state *n)
{
	biquad_shutdown(n->aspirate_filter);
	biquad_shutdown(n->fricative_filter);
	free(n);
}

float noise_next(struct noise_state *n)
{
	int r = rand();
	float uniform = (float)r / (float)RAND_MAX;
	
	uniform = uniform * 2.0f - 1.0f;
	
	return biquad_process(n->fricative_filter, biquad_process(n->aspirate_filter, uniform));
}
