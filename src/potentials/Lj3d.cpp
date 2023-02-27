#include "Lj3d.h"

#include <math.h>
#include <stdexcept>
#include "State.h"

namespace minim {
  using std::vector;


  Lj3d::Lj3d(double sigma, double epsilon) : Lj3d() {
    this->sigma = sigma;
    this->epsilon = epsilon;
  }


  void Lj3d::init(const vector<double>& coords) {
    int ndof = coords.size();
    if (ndof % 3 != 0) throw std::invalid_argument("Length of coords must be a multiple of 3.");
    n_particle = ndof / 3;
    // Generate energy elements
    elements = {};
    for (int i=0; i<n_particle; i++) {
      for (int j=i+1; j<n_particle; j++) {
        elements.push_back({0, {3*i, 3*i+1, 3*i+2, 3*j, 3*j+1, 3*j+2}});
      }
    }
  }


  void Lj3d::elementEnergyGradient(const vector<double>& coords, const Element& el, double* e, vector<double>* g) const {
    double dx = coords[el.idof[0]] - coords[el.idof[3]];
    double dy = coords[el.idof[1]] - coords[el.idof[4]];
    double dz = coords[el.idof[2]] - coords[el.idof[5]];
    double r2 = dx*dx + dy*dy + dz*dz;
    double lj6 = pow(sigma, 6) / pow(r2, 3);

    if (e != nullptr) *e += 4*epsilon * (lj6*lj6 - lj6);

    if (g != nullptr) {
      double r = sqrt(r2);
      double dedr = -24 * epsilon * (2*lj6*lj6 - lj6) / r;
      (*g)[el.idof[0]] += dx/r * dedr;
      (*g)[el.idof[1]] += dy/r * dedr;
      (*g)[el.idof[2]] += dz/r * dedr;
      (*g)[el.idof[3]] -= dx/r * dedr;
      (*g)[el.idof[4]] -= dy/r * dedr;
      (*g)[el.idof[5]] -= dz/r * dedr;
    }
  }


  Lj3d& Lj3d::setSigma(double sigma) {
    this->sigma = sigma;
    return *this;
  }


  Lj3d& Lj3d::setEpsilon(double epsilon) {
    this->epsilon = epsilon;
    return *this;
  }

}
