#include <math.h>
#include "contract.h"

double Derivative(double A, double deltaX) {
    return (cos(A + deltaX) - cos(A - deltaX)) / (2.0 * deltaX);
}

double Pi(int K) {
    double pi_over_2 = 1.0;
    for (int n = 1; n <= K; n++) {
        double numerator = 2.0 * n;
        numerator *= numerator;
        double denominator = (2.0 * n - 1.0) * (2.0 * n + 1.0);
        pi_over_2 *= numerator / denominator;
    }
    return pi_over_2 * 2.0;
}
