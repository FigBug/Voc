#ifndef NOISE_H
#define NOISE_H

struct noise_state;

struct noise_state *
noise_init
	(unsigned long sample_rate
	,float f1
	,float q1
	,float f2
	,float q2
	);

void noise_shutdown(struct noise_state *n);

float noise_next(struct noise_state *n);

#endif
