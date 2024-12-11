#include <math.h>
#include "contract.h"

double Derivative(double A, double deltaX) {
    return (cos(A + deltaX) - cos(A)) / deltaX;
}

double Pi(int K) {
    double pi = 0.0;
    for (int n = 0; n < K; n++) {
        double term = pow(-1.0, n) / (2.0 * n + 1.0);
        pi += term;
    }
    pi *= 4.0;
    return pi;
}
