#ifndef _PROCESS_RITA2_H
#define _PROCESS_RITA2_H

#include <math.h>
#include <stdio.h>

// Desired pixels
const int Nx = 128;
const int Ny = 128;

// Pixels not used for main signal after rescaling
const int correction_deltax = 3;
const int correction_deltay = 3;

// Constants for stretching
const double correction_dx = 2.688596e+03;
const double correction_x1 = 7.175933e+02;
const double correction_dy = 3.241703e+03;
const double correction_y1 = 4.382790e+02;

// Constants for unskewing
const double correction_a_1 = 3.696742e-08;
const double correction_a_0 = -7.581946e-05;
const double correction_b_1 = -1.501326e-04;
const double correction_b_0 = 3.070921e-01;
const double correction_c_1 = 1.151074e+00;
const double correction_c_0 = -3.091622e+02;

#endif
