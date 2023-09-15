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

#ifndef SIMPLEX_H
#define SIMPLEX_H

struct simplex_state;

struct simplex_state *simplex_init(unsigned seed);
void simplex_shutdown(struct simplex_state *ss);

float simplex1(struct simplex_state *s, float a);

#endif
