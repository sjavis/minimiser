#ifndef POTENTIAL_H
#define POTENTIAL_H

#include <vector>

class State;


class Args {
  public:
    struct Element {
      int id;
      int type;
      std::vector<int> idof;
      std::vector<double> parameters;
    };

    const int ndof;
    std::vector<Element> elements;
    std::vector<Element> elements_halo;

    Args(int ndof) : ndof(ndof) {};
    Args(int ndof, std::vector<std::vector<int>> idofs);
    Args(int ndof, std::vector<std::vector<int>> idofs, std::vector<int> types,
         std::vector<std::vector<double>> parameters);
    ~Args() {};
};


class Potential {

  typedef std::vector<double> Vector;
  typedef double (*EFunc)(Vector, Args&);
  typedef Vector (*GFunc)(Vector, Args&);

  public:
    Potential(EFunc energy, GFunc gradient) : _energy(energy), _gradient(gradient) {};
    ~Potential() {};

    virtual double energy(Vector coords, Args &args);
    virtual Vector gradient(Vector coords, Args &args);

    virtual Args* newArgs(int ndof);
    State newState(Vector coords);
    State newState(int ndof);

  protected:
    Potential() {};

  private:
    EFunc _energy;
    GFunc _gradient;
};


#endif
