#include "GradDescent.h"

#include <math.h>
#include <vector>
#include "State.h"
#include "Minimiser.h"
#include "vec.h"


GradDescent::GradDescent(State &state, AdjustFunc adjustModel)
  : Minimiser(state, adjustModel), _g(state.ndof)
{}


GradDescent& GradDescent::setAlpha(double alpha) {
  _alpha = alpha;
  return *this;
}


GradDescent& GradDescent::setMaxIter(int maxIter) {
  Minimiser::setMaxIter(maxIter);
  return *this;
}


void GradDescent::iteration() {
  _g = state.gradient();
  auto step = vec::multiply(-_alpha, _g);
  state.blockCoords(vec::sum(state.blockCoords(), step));
}


bool GradDescent::checkConvergence() {
  double sum = state.comm.dotProduct(_g, _g);
  double rms = sqrt(sum/state.ndof);
  return (rms < state.convergence);
}
