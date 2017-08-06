#ifndef BIQUAD_H
#define BIQUAD_H

struct biquad_state;

struct biquad_state *
biquad_init_bandpass
	(float sample_rate
	,float frequency
	,float gain
	,float Q
	);

void biquad_shutdown(struct biquad_state *b);

float biquad_process(struct biquad_state *b, float in);

#endif
