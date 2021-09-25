#include <vector>
#include <math.h>
#include "Potential.h"
#include "BarAndHinge.h"
#include "vec.h"
#include "print.h"


double BarAndHinge::energy(const std::vector<double> &coords, const Args &args) {
  double e = 0;
  for (auto el: args.elements) {
    switch (el.type) {
      case 0:
        stretching(coords, el, &e, NULL);
        break;
      case 1:
        bending(coords, el, &e, NULL);
        break;
    }
  }
  return e;
}


std::vector<double> BarAndHinge::gradient(const std::vector<double> &coords, const Args &args) {
  std::vector<double> g(coords.size());
  for (auto el: args.elements) {
    switch (el.type) {
      case 0:
        stretching(coords, el, NULL, &g);
        break;
      case 1:
        bending(coords, el, NULL, &g);
        break;
    }
  }
  return g;
}


void BarAndHinge::stretching(const std::vector<double> &coords, Args::Element el,
                             double *e, std::vector<double> *g) {
  std::vector<double> x1(coords.cbegin()+el.idof[0], coords.cbegin()+el.idof[2]+1);
  std::vector<double> x2(coords.cbegin()+el.idof[3], coords.cbegin()+el.idof[5]+1);

  // Compute distance
  auto dx = vec::diff(x1, x2);
  auto l = vec::norm(dx);
  auto dl = l - el.parameters[1];

  if (e != NULL) {
    *e += el.parameters[0] * pow(dl, 2);
  }
  if (g != NULL) {
    auto g_factor = 2 * el.parameters[0] * dl / l;
    (*g)[el.idof[0]] += g_factor * dx[0];
    (*g)[el.idof[1]] += g_factor * dx[1];
    (*g)[el.idof[2]] += g_factor * dx[2];
    (*g)[el.idof[3]] -= g_factor * dx[3];
    (*g)[el.idof[4]] -= g_factor * dx[4];
    (*g)[el.idof[5]] -= g_factor * dx[5];
  }
}


void BarAndHinge::bending(const std::vector<double> &coords, Args::Element el,
                          double *e, std::vector<double> *g) {
  std::vector<double> x1(coords.cbegin()+el.idof[0], coords.cbegin()+el.idof[2]+1);
  std::vector<double> x2(coords.cbegin()+el.idof[3], coords.cbegin()+el.idof[5]+1);
  std::vector<double> x3(coords.cbegin()+el.idof[6], coords.cbegin()+el.idof[8]+1);
  std::vector<double> x4(coords.cbegin()+el.idof[9], coords.cbegin()+el.idof[11]+1);

  // Compute bond vectors
  auto b1 = vec::diff(x2, x1);
  auto b2 = vec::diff(x3, x2);
  auto b3 = vec::diff(x4, x3);
  double b2m = vec::norm(b2);

  // Compute normal vectors
  auto n1 = vec::crossProduct(b1, b2);
  auto n2 = vec::crossProduct(b2, b3);
  double n1sq = vec::dotProduct(n1, n1);
  double n2sq = vec::dotProduct(n2, n2);
  double n12m = sqrt(n1sq * n2sq);

  // Compute cos and sin
  double c = vec::dotProduct(n1, n2) / n12m;
  double s = b2m / n12m * vec::dotProduct(n1, b3);
  double c0 = cos(el.parameters[1]);
  double s0 = sin(el.parameters[1]);

  if (e != NULL) {
    *e += el.parameters[0] * (1 + c*c0 - s*s0); // Double angle formula for cos(t - t0)
  }
  if (g != NULL) {
    double gfactor = - el.parameters[0] * (s*c0 + c*s0);
    // Normal vectors with magnitude 1 / triangle height
    auto n1h = vec::multiply(n1, b2m/n1sq);
    auto n2h = vec::multiply(n2, b2m/n2sq);
    // Quantify triangular skew, 0.5 if symmetric
    double skew1 = -vec::dotProduct(b1, b2) / (b2m*b2m);
    double skew2 = -vec::dotProduct(b3, b2) / (b2m*b2m);
    (*g)[el.idof[0]] += -gfactor * n1h[0];
    (*g)[el.idof[1]] += -gfactor * n1h[1];
    (*g)[el.idof[2]] += -gfactor * n1h[2];
    (*g)[el.idof[3]] +=  gfactor * ((1-skew1)*n1h[0] - skew2*n2h[0]);
    (*g)[el.idof[4]] +=  gfactor * ((1-skew1)*n1h[1] - skew2*n2h[1]);
    (*g)[el.idof[5]] +=  gfactor * ((1-skew1)*n1h[2] - skew2*n2h[2]);
    (*g)[el.idof[6]] += -gfactor * ((1-skew2)*n2h[0] - skew1*n1h[0]);
    (*g)[el.idof[7]] += -gfactor * ((1-skew2)*n2h[1] - skew1*n1h[1]);
    (*g)[el.idof[8]] += -gfactor * ((1-skew2)*n2h[2] - skew1*n1h[2]);
    (*g)[el.idof[9]]  += gfactor * n2h[0];
    (*g)[el.idof[10]] += gfactor * n2h[1];
    (*g)[el.idof[11]] += gfactor * n2h[2];
  }
}
