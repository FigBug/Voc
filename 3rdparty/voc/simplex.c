/* This code is heavily modified (and ported) from code with this notice: */

/*
 * A speed-improved perlin and simplex noise algorithms for 2D.
 *
 * Based on example code by Stefan Gustavson (stegu@itn.liu.se).
 * Optimisations by Peter Eastman (peastman@drizzle.stanford.edu).
 * Better rank ordering method by Stefan Gustavson in 2012.
 * Converted to Javascript by Joseph Gentle.
 *
 * Version 2012-03-09
 *
 * This code was placed in the public domain by its original author,
 * Stefan Gustavson. You may use it as you see fit, but
 * attribution is appreciated.
 *
 */
#include "simplex.h"

#include <stdlib.h>
#include <math.h>

struct vec2 {
	float x;
	float y;
};

struct simplex_state {
	unsigned perm[512];
	struct vec2 gradP[512];
	
	float F2;
	float G2;
};

static const struct vec2 grad3[] =
	{{1.0f, 1.0f}, {-1.0f, 1.0f}, {1.0f, -1.0f}, {-1.0f, -1.0f}
	,{1.0f, 0.0f}, {-1.0f, 0.0f}, {1.0f, 0.0f}, {-1.0f, 0.0f}
	,{0.0f, 1.0f}, {0.0f, -1.0f}, {0.0f, 1.0f}, {0.0f, -1.0f}};


static const unsigned p[] =
	{151,160,137,91,90,15,131,13,201,95,96,53,194,233,7,225,140,36,103,30
	,69,142,8,99,37,240,21,10,23,190, 6,148,247,120,234,75,0,26,197,62,94
	,252,219,203,117,35,11,32,57,177,33,88,237,149,56,87,174,20,125,136
	,171,168, 68,175,74,165,71,134,139,48,27,166,77,146,158,231,83,111,229
	,122,60,211,133,230,220,105,92,41,55,46,245,40,244,102,143,54, 65,25
	,63,161, 1,216,80,73,209,76,132,187,208, 89,18,169,200,196,135,130,116
	,188,159,86,164,100,109,198,173,186,3,64,52,217,226,250,124,123,5,202
	,38,147,118,126,255,82,85,212,207,206,59,227,47,16,58,17,182,189,28,42
	,223,183,170,213,119,248,152,2,44,154,163,70,221,153,101,155,167,43
	,172,9,129,22,39,253,19,98,108,110,79,113,224,232,178,185,112,104,218
	,246,97,228,251,34,242,193,238,210,144,12,191,179,162,241, 81,51,145
	,235,249,14,239,107,49,192,214, 31,181,199,106,157,184, 84,204,176,115
	,121,50,45,127, 4,150,254,138,236,205,93,222,114,67,29,24,72,243,141
	,128,195,78,66,215,61,156,180};

static float dot2(struct vec2 a, float x, float y)
{
	return a.x * x + a.y * y;
}


struct simplex_state *simplex_init(unsigned seed)
{
	struct simplex_state *s = malloc(sizeof s[0]);
	unsigned i;
	
	if (seed < 256) {
		seed |= seed << 8;
	}
	
	for (i = 0; i < 256; i++) {
		unsigned v;
		if (i & 1) {
			v = p[i] & (seed & 255);
		}
		else {
			v = p[i] & ((seed >> 8) & 255);
		}
		
		s->perm[i] = v;
		s->perm[i + 256] = v;
		s->gradP[i] = grad3[v % 12];
		s->gradP[i + 256] = grad3[v % 12];
	}
	
	s->F2 = 0.5f * sqrt(3.0f) - 1.0f;
	s->G2 = (3.0f - sqrt(3.0f)) / 6.0f;
	
	return s;
}

void simplex_shutdown(struct simplex_state *ss)
{
	free(ss);
}

static float simplex2(struct simplex_state *ss, float xin, float yin)
{
	float n0;
	float n1;
	float n2;
	
	float s = (xin + yin) * ss->F2;
	int i = floor(xin + s);
	int j = floor(yin + s);
	float t = (i + j) * ss->G2;
	float x0 = xin - (float)i + t;
	float y0 = yin - (float)j + t;
	
	int i1;
	int j1;
	
	float x1;
	float y1;
	float x2;
	float y2;
	
	struct vec2 gi0;
	struct vec2 gi1;
	struct vec2 gi2;
	
	float t0;
	float t1;
	float t2;
	
	if (x0 > y0) {
		i1 = 1;
		j1 = 0;
	}
	else {
		i1 = 0;
		j1 = 1;
	}
	
	x1 = x0 - i1 + ss->G2;
	y1 = y0 - j1 + ss->G2;
	x2 = x0 - 1.0f + 2.0f * ss->G2;
	y2 = y0 - 1.0f + 2.0f * ss->G2;
	
	i &= 0xff;
	j &= 0xff;
	
	gi0 = ss->gradP[i + ss->perm[j]];
	gi1 = ss->gradP[i + i1 + ss->perm[j + j1]];
	gi2 = ss->gradP[i + 1 + ss->perm[j + 1]];
	
	t0 = 0.5f - x0 * x0 - y0 * y0;
	
	if (t0 < 0.0f) {
		n0 = 0.0f;
	}
	else {
		t0 *= t0;
		n0 = t0 * t0 * dot2(gi0, x0, y0);
	}
	t1 = 0.5f - x1 * x1 - y1 * y1;
	if (t1 < 0.0f) {
		n1 = 0.0f;
	}
	else {
		t1 *= t1;
		n1 = t1 * t1 * dot2(gi1, x1, y1);
	}
	t2 = 0.5 - x2 * x2 - y2 * y2;
	if (t2 < 0.0f) {
		n2 = 0.0f;
	}
	else {
		t2 *= t2;
		n2 = t2 * t2 * dot2(gi2, x2, y2);
	}
	
	return 70.0f * (n0 + n1 + n2);
}

float simplex1(struct simplex_state *ss, float a)
{
	return simplex2(ss, a * 1.2f, -a * 0.7f);
}


