#include <iostream>
#include <fstream>
#include <iomanip>
#include <cstdlib>
#include <math.h>
#include <time.h>
using namespace std;

ifstream position("../liquid256.txt");
ofstream energy_out("./output/energy.txt");
ofstream momenta_out("output/momenta.txt");
ofstream tp_out("output/tp.txt");

const int N = 256, STEP = 500000;
const double dlt = 0.002;
const double L = 7.4, RC = 2.5;
const double KB = 1.3806e-23;
const double EPSILON = 1.67e-21;
const double SIGMA = 3.4e-10;
const double MASS = 6.63e-26;
const double TAU = 0.05;
const double VMAX = 5;
const double TSET = 100;

void checkr(double r[3], double bot, double up);
double distance(double r1[3], double r2[3]);
double getURC(double r);
double getFRC(double r);
double LJ(double r, double URC, double FRC);
double force(double r, double URC, double FRC);
double potential(double r[N][3], double URF, double FRC);
double kinetic(double v[N][3], double m[N]);
void momenta(double M[3], double v[N][3], double m[N]);
double temperature(double K);
double pressure(double T, double r[N][3], double URC, double FRC);
double verletv(double v, double f, double m);


int main() {
    srand(unsigned(time(NULL)));
    int i, j, k, t;
    double r[N][3], m[N], v[N][3];
    double f[N][3], tempv[N][3];
    double K[STEP], U[STEP], E[STEP], M[STEP][3]; // Define energy and momenta
    double T[STEP], P[STEP];
    double vtotal[3]; // Support value for making system momenta to be 0
    double URC, FRC;
    double tempr, tempf, temp, eta;
    double vecr[3], zeror[3], vecf[3];

    URC = getURC(RC);
    FRC = getFRC(RC);

    // Initialize the position, mass, force, energy, generate random value of velocity
    vtotal[0] = vtotal[1] = vtotal[2] = 0;
    zeror[0] = zeror[1] = zeror[2] = 0;
    for (i = 0; i < N; i++) {
        for (k = 0; k < 3; k++) {
            position >> r[i][k];
            v[i][k] = VMAX * rand() / RAND_MAX;
            vtotal[k] = vtotal[k] + v[i][k];
        }
        m[i] = 1;
    }
    // Initialize velocity to make system momenta to be 0
    for (i = 0; i < N; i++) {
        for (k = 0; k < 3; k++) {
            v[i][k] = v[i][k] - vtotal[k] / N;
        }
    }
    temp = kinetic(v, m); // The dimentionless kinetic energy of this stage
    temp = temperature(temp) * EPSILON / KB; // The temperature of this stage
    // Calculate the force at t=0 for the verlet loop
    for (i = 0; i < N; i++) {
        f[i][0] = f[i][1] = f[i][2] = 0;
        for (j = 0; j < N; j++) {
            if (j == i) {
                continue;
            }
            else {
                for (k = 0; k < 3; k++) {
                    vecr[k] = r[i][k] - r[j][k];
                }
                checkr(vecr, -L / 2, L / 2);
                tempr = distance(vecr, zeror); // Calculate the distance between two atoms
                tempf = force(tempr, URC, FRC); // Calculate the force(scalar) between two atoms
                for (k = 0; k < 3; k++) {
                    f[i][k] = f[i][k] + tempf / tempr * vecr[k]; // Spilt force into each axis
                }
            }
        }
    }
    eta = 0;
    // Loop with time steps
    for (t = 0; t < STEP; t++) {
        if (t % 100 == 0) {
            cout << "Time step: " << t << endl;
        }
        momenta(M[t], v, m); // Collecting monenta vectors
        U[t] = potential(r, URC, FRC);
        K[t] = kinetic(v, m);
        E[t] = U[t] + K[t];
        T[t] = temperature(K[t]) * EPSILON / KB;
        P[t] = pressure(T[t], r, URC, FRC);
        energy_out << t*dlt << ' ' << U[t] << ' ' << K[t] << ' ' << E[t] << endl;
        tp_out << t*dlt << ' ' << T[t]  << ' ' << P[t] << endl;
        momenta_out << t*dlt << ' ' << M[t][0] << ' ' << M[t][1] << ' ' << M[t][2] << endl;

        // Check boundary first, if the atom run away from the simulation box, add a mirror one into it
        for (i = 0; i < N; i++) {
            checkr(r[i], 0, L);
        }

        for (i = 0; i < N; i++) {
            for (k = 0; k < 3; k++) {
                // Verlet loop, calculate the velocity of (t + dlt / 2)
                tempv[i][k] = v[i][k] + (f[i][k] / m[i] - eta * v[i][k]) * dlt / 2; 
            }
        }
        for (i = 0; i < N; i++) {
            for (k = 0; k < 3; k++) {
                // Verlet loop, calculate the new position of (t + dlt)
                r[i][k] = r[i][k] + tempv[i][k] * dlt; 
            }
        }
        eta = eta + dlt / (TAU * TAU) * (T[t] / TSET - 1);
        // Check whether the position is out of the simulation box.
        for (i = 0; i < N; i++) {
            checkr(r[i], 0, L);
        }
        // Upedate force with the (t+dlt) position for the later calculation of Verlet loop
        for (i = 0; i < N; i++) {
            f[i][0] = f[i][1] = f[i][2] = 0;
            for (j = 0; j < N; j++) {
                if (j == i) {
                    continue;
                }
                else {
                    for (k = 0; k < 3; k++) {
                        vecr[k] = r[i][k] - r[j][k];
                    }
                    // Check whether the atom are interaction with the shortest disstance mirror one
                    checkr(vecr, -L / 2, L / 2);
                    tempr = distance(vecr, zeror); // Calculate the distance between two atoms
                    tempf = force(tempr, URC, FRC); // Calculate the force(scalar) between two atoms
                    for (k = 0; k < 3; k++) {
                        f[i][k] = f[i][k] + tempf / tempr * vecr[k]; // Spilt force into each axis
                    }
                }
            }
        }
        for (i = 0; i < N; i++) {
            for (k = 0; k < 3; k++) {
                // verlet loop, calculate the new velocity of (t + dlt)
                v[i][k] = (tempv[i][k] + (f[i][k] * dlt / (2 * m[i]))) / (1 + eta * dlt / 2); 
            }
        }


    }
}

// Calculate the cutoff potential and force(they are both constant values)
double getURC(double r) {
    double u;
    double r2, r4, r6;
    r2 = r * r;
    r4 = r2 * r2;
    r6 = r4 * r2;
    u = 4 / (r6 * r6) - 4 / r6;
    return u;
}
double getFRC(double r) {
    double f;
    double r2, r4, r6;
    r2 = r * r;
    r4 = r2 * r2;
    r6 = r4 * r2;
    f = 48 / (r6 * r6 * r) - 24 / (r6 * r);
    return f;
}
void checkr(double r[3], double bot, double up) {
    int k;
    for (k = 0; k < 3; k++) {
        if (r[k] > up) {
            r[k] = r[k] - L;
        }
        else if (r[k] < bot) {
            r[k] = r[k] + L;
        }
    }
}

// Given two vectors, calculate the distance scalar between the two given atoms
double distance(double r1[3], double r2[3]) {
    double d = 0;
    double vecr[3];
    int k;
    for (k = 0; k < 3; k++) {
        vecr[k] = r1[k] - r2[k];
    }
    checkr(vecr, -L / 2, L / 2);
    for (k = 0; k < 3; k++) {
        d = d + vecr[k] * vecr[k];
    }
    return sqrt(d);
}
// Given distance between atoms, calculate the potential energy
double LJ(double r, double URC, double FRC) {
    double u;
    double r2, r4, r6;
    if (r > RC) {
        u = 0;
    }
    else {
        r2 = r * r;
        r4 = r2 * r2;
        r6 = r4 * r2;
        u = 4 / (r6 * r6) - 4 / r6 - URC + FRC * (r - RC);
    }

    return u;
}
// Given distance between atoms, calculate the force/distance scalar
double force(double r, double URC, double FRC) {
    double f;
    double r2, r4, r6;
    if (r > RC) {
        f = 0;
    }
    else {
        r2 = r * r;
        r4 = r2 * r2;
        r6 = r4 * r2;
        f = 48 / (r6 * r6 * r) - 24 / (r6 * r) - FRC;
    }
    return f;
}
// Given velovity vector of all atoms, calculate the system kinetic energy
double kinetic(double v[N][3], double m[N]) {
    double Kinetic;
    int i, k;
    Kinetic = 0;
    for (i = 0;  i < N; i++) {
        for (k = 0; k < 3; k++) {
            Kinetic += 0.5 * m[i] * v[i][k] * v[i][k];
        }
    }
    return Kinetic;
}

void momenta(double M[3], double v[N][3], double m[N]) {
    int i, k;
    M[0] = M[1] = M[2] = 0;
    for (i = 0; i < N; i++) {
        for (k = 0; k < 3; k++) {
            M[k] = M[k] + v[i][k] * m[i];
        }
    }
}
// Given position of all atoms, calculate teh system potential energy
double potential(double r[N][3], double URC, double FRC) {
    double d, Potential;
    int i, j;
    Potential = 0;
    for (i = 0; i < N; i++) {
        for (j = i + 1; j < N; j++) {
            d = distance(r[i], r[j]);
            Potential = Potential + LJ(d, URC, FRC);
        }
    }
    return Potential;
}
// Calculate time-related Temperature(in real scale unit K) with input of kinetic energy
double temperature(double K) {
    double Temperature;
    Temperature = 2 * K / (3 * (N - 1));
    return Temperature;
}
// Calculate time-related Pressure with given Force and Position
double pressure(double T,  double r[N][3], double URC, double FRC) {
    double Pressure;
    int i, j, k;
    double tempr,  tempf;
    double vecr[3], zeror[3], vecf[3];
    zeror[0] = zeror[1] = zeror[2] = 0;

    Pressure = N * KB * T;
    for (i = 0; i < N; i++) {
        for (j = i + 1; j < N; j++) {
            // Check whether the vector rij matches the minimum mirror rule
            for (k = 0; k < 3; k++) {
                vecr[k] = r[i][k] - r[j][k];
            }
            checkr(vecr, -L / 2, L / 2);
            tempr = distance(vecr, zeror);
            tempf = force(tempr, URC, FRC);
            for (k = 0; k < 3; k++) {
                vecf[k] = tempf / tempr * vecr[k];
                Pressure = Pressure + vecf[k] * vecr[k] * EPSILON / 3;
            }
        }
    }
    Pressure = Pressure / (L * SIGMA * L * SIGMA * L * SIGMA);
    return Pressure;
}