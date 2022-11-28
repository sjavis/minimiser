#include "PFWetting.h"

#include <array>
#include <math.h>
#include <stdexcept>
#include <functional>
#include "State.h"
#include "utils/vec.h"


namespace minim {
  typedef std::vector<double> Vector;


  std::array<int,3> getCoord(int i, std::array<int,3> gridSize) {
    int z = i % gridSize[2];
    int y = (i - z) / gridSize[2] % gridSize[1];
    int x = i / (gridSize[1] * gridSize[2]);
    return {x, y, z};
  }

  // Used to calculate the indicies of the neighbouring nodes
  class Neighbours {
    public:
      std::array<int,26> di;
      std::array<std::array<int,3>,26> dx {{
        // Adjacent faces
        {-1,  0,  0}, { 0, -1,  0}, { 0,  0, -1}, { 0,  0,  1}, { 0,  1,  0}, { 1,  0,  0},
        // Adjacent edges
        {-1, -1,  0}, {-1,  0, -1}, {-1,  0,  1}, {-1,  1,  0},
        { 0, -1, -1}, { 0, -1,  1}, { 0,  1, -1}, { 0,  1,  1},
        { 1, -1,  0}, { 1,  0, -1}, { 1,  0,  1}, { 1,  1,  0},
        // Adjacent corners
        {-1, -1, -1}, {-1, -1,  1}, {-1,  1, -1}, {-1,  1,  1},
        { 1, -1, -1}, { 1, -1,  1}, { 1,  1, -1}, { 1,  1,  1}
      }};

      int operator[](int i) { return di[i]; };

      Neighbours(std::array<int,3> gridSize, int i0) {
        auto x0 = getCoord(i0, gridSize);
        for (int i=0; i<26; i++) {
          int x = (x0[0] + dx[i][0]) % gridSize[0];
          int y = (x0[1] + dx[i][1]) % gridSize[1];
          int z = (x0[2] + dx[i][2]) % gridSize[2];
          di[i] = (x*gridSize[1] + y)*gridSize[2] + z;
        }
      }
  };


  void PFWetting::init() {
    double f1Mag = vec::norm(force1);
    double f2Mag = vec::norm(force1);
    Vector f1Norm = force1 / f1Mag;
    Vector f2Norm = force2 / f2Mag;

    elements = {};
    int nGrid = gridSize[0] * gridSize[1] * gridSize[2];
    for (int i=0; i<nGrid; i++) {
      if (solid[i]) continue;

      // Get fluid volume and solid surface area for each node
      double volume = 1;
      double surfaceArea = 0;
      int type = getType(i);
      if (type > 0) {
        volume = type / 8.0;
        if (type == 2 || type == 4 || type == 6) surfaceArea = 1;
        if (type == 1 || type == 7) surfaceArea = 0.75;
        if (type == 3 || type == 5) surfaceArea = 1.25;
      }

      // Set bulk fluid elements
      if (!solid[i]) {
        Neighbours di(gridSize, i);
        elements.push_back({0, {i, di[0], di[1], di[2], di[3], di[4], di[5]}, {volume}});
      }

      // Set surface fluid elements
      if (type > 0 && !contactAngle.empty()) {
        double wettingParam = sqrt(2) * cos(contactAngle[i]);
        if (wettingParam != 0) {
          elements.push_back({1, {i}, {surfaceArea, wettingParam}});
        }
      }

      // Set external force elements
      if (f1Mag > 0 || f2Mag > 0) {
        Vector params{volume, f1Mag, f2Mag, f1Norm[0], f1Norm[1], f1Norm[2], f2Norm[0], f2Norm[1], f2Norm[2]};
        elements.push_back({2, {i}, params});
      }
    }
  }


  void PFWetting::blockEnergyGradient(const Vector& coords, double* e, Vector* g) const {
    if (e) *e = 0;
    if (g) *g = Vector(coords.size());

    for (auto el : elements) {
      elementEnergyGradient(el, coords, e, g);
    }

    // Compute the gradient of the halo energy elements
    if (g) {
      for (auto el : elements_halo) {
        elementEnergyGradient(el, coords, nullptr, g);
      }
    }
  }


  void PFWetting::elementEnergyGradient(const Element el, const Vector& coords, double* e, Vector* g) const {
    switch (el.type) {
      case 0: {
        double vol = el.parameters[0];
        // Bulk energy
        double phi = coords[el.idof[0]];
        if (e) *e += 0.25/epsilon * (pow(phi,4) - 2*pow(phi,2) + 1) * vol;
        if (g) (*g)[el.idof[0]] += 1/epsilon * (pow(phi,3) - phi) * vol;
        // Gradient energy
        double diffxm = !solid[el.idof[1]] ? phi - coords[el.idof[1]] : 0;
        double diffym = !solid[el.idof[2]] ? phi - coords[el.idof[2]] : 0;
        double diffzm = !solid[el.idof[3]] ? phi - coords[el.idof[3]] : 0;
        double diffzp = !solid[el.idof[4]] ? phi - coords[el.idof[4]] : 0;
        double diffyp = !solid[el.idof[5]] ? phi - coords[el.idof[5]] : 0;
        double diffxp = !solid[el.idof[6]] ? phi - coords[el.idof[6]] : 0;
        int nfx = (!solid[el.idof[1]]) + (!solid[el.idof[6]]); // = 2 if both sides fluid
        int nfy = (!solid[el.idof[2]]) + (!solid[el.idof[5]]); // = 1 if one side solid
        int nfz = (!solid[el.idof[3]]) + (!solid[el.idof[4]]); // = 0 is an error
        double gradx2 = (pow(diffxm, 2) + pow(diffxp, 2)) / nfx;
        double grady2 = (pow(diffym, 2) + pow(diffyp, 2)) / nfy;
        double gradz2 = (pow(diffzm, 2) + pow(diffzp, 2)) / nfz;
        double grad2 = gradx2 + grady2 + gradz2;
        double factor = 0.5 * epsilon * vol;
        if (e) *e += factor * grad2;
        if (g) {
          (*g)[el.idof[0]] += factor * (diffxm + diffxp + diffym + diffyp + diffzm + diffzp);
          (*g)[el.idof[1]] -= factor * diffxm;
          (*g)[el.idof[2]] -= factor * diffym;
          (*g)[el.idof[3]] -= factor * diffzm;
          (*g)[el.idof[4]] -= factor * diffzp;
          (*g)[el.idof[5]] -= factor * diffyp;
          (*g)[el.idof[6]] -= factor * diffxp;
        }
      } break;

      case 1: {
        // Surface energy
        // parameter[0]: Surface area
        // parameter[1]: Wetting parameter sqrt(2)cos(theta)
        double phi = coords[el.idof[0]];
        if (e) *e += el.parameters[1]/6 * (pow(phi,3) - 3*phi - 2) * el.parameters[0];
        if (g) (*g)[el.idof[0]] += el.parameters[1]*0.5 * (pow(phi,2) - 1) * el.parameters[0];
      } break;

      case 2: {
        // External force
        // Parameters:
        //   0: Volume
        //   1: Magnitude of force on component 1
        //   2: Magnitude of force on component 2
        //   3-5: Direction force on component 1
        //   6-8: Direction force on component 2
        double phi = coords[el.idof[0]];
        double f1 = el.parameters[1];
        double f2 = el.parameters[2];
        Vector f1Norm = {el.parameters[3], el.parameters[4], el.parameters[5]};
        Vector f2Norm = {el.parameters[6], el.parameters[7], el.parameters[8]};
        std::array<int,3> coordA = getCoord(el.idof[0]);
        Vector coord(coordA.begin(), coordA.end());
        double h1 = - vec::dotProduct(coord, f1Norm);
        double h2 = - vec::dotProduct(coord, f2Norm);
        if (e) *e += 0.5 * ((1+phi)*f1*h1 + (1-phi)*f2*h2) * el.parameters[0];
        if (g) (*g)[el.idof[0]] += 0.5 * (f1*h1 - f2*h2) * el.parameters[0];
      } break;

      default:
        std::invalid_argument("Unknown energy element type.");
    }
  }


  PFWetting& PFWetting::setGridSize(std::array<int,3> gridSize) {
    this->gridSize = gridSize;
    return *this;
  }

  PFWetting& PFWetting::setEpsilon(double epsilon) {
    this->epsilon = epsilon;
    return *this;
  }

  PFWetting& PFWetting::setResolution(double resolution) {
    this->resolution = resolution;
    return *this;
  }

  PFWetting& PFWetting::setPressure(double pressure) {
    this->pressure = pressure;
    return *this;
  }

  PFWetting& PFWetting::setVolume(double volume, double volConst) {
    this->volume = volume;
    this->volConst = volConst;
    return *this;
  }

  PFWetting& PFWetting::setSolid(std::vector<bool> solid) {
    if ((int)solid.size() != gridSize[0]*gridSize[1]*gridSize[2]) std::invalid_argument("Invalid size of solid array.");
    this->solid = solid;
    return *this;
  }

  PFWetting& PFWetting::setSolid(std::function<bool(int,int,int)> solidFn) {
    solid = std::vector<bool>(gridSize[0]*gridSize[1]*gridSize[2]);
    int itot = 0;
    for (int i=0; i<gridSize[0]; i++) {
      for (int j=0; j<gridSize[1]; j++) {
        for (int k=0; k<gridSize[2]; k++) {
          solid[itot] = solidFn(i, j, k);
          itot ++;
        }
      }
    }
    return *this;
  }

  PFWetting& PFWetting::setContactAngle(Vector contactAngle) {
    if ((int)contactAngle.size() != gridSize[0]*gridSize[1]*gridSize[2]) std::invalid_argument("Invalid size of contactAngle array.");
    this->contactAngle = contactAngle;
    return *this;
  }

  PFWetting& PFWetting::setContactAngle(std::function<double(int,int,int)> contactAngleFn) {
    contactAngle = Vector(gridSize[0]*gridSize[1]*gridSize[2]);
    int itot = 0;
    for (int i=0; i<gridSize[0]; i++) {
      for (int j=0; j<gridSize[1]; j++) {
        for (int k=0; k<gridSize[2]; k++) {
          contactAngle[itot] = contactAngleFn(i, j, k);
          itot ++;
        }
      }
    }
    return *this;
  }


  State PFWetting::newState(const Vector& coords) {
    init();
    return State(*this, coords);
  }


  int PFWetting::nGrid() const {
    return gridSize[0] * gridSize[1] * gridSize[2];
  }


  std::array<int,3> PFWetting::getCoord(int i) const {
    return minim::getCoord(i, gridSize);
  }


  int PFWetting::getType(int i) const {
    // Types:
    // -1: Solid (▪▪)   0: Bulk fluid (  )
    //           (▪▪)                 (  )
    // 1: ▫    2: ▫▫   3: ▫▫   4: ▫▫   5: ▪▫   6: ▪▪   7: ▪▪
    //                    ▫       ▫▫      ▫▫      ▫▫      ▪▫
    if (solid[i]) return -1;

    // Get which neighbouring nodes are solid
    Neighbours di(gridSize, i);
    std::array<bool,26> neiSolid;
    for (int iNei=0; iNei<26; iNei++) {
      neiSolid[iNei] = solid[di[iNei]];
    }
    int nSolidF = neiSolid[0] + neiSolid[1] + neiSolid[2] + neiSolid[3] + neiSolid[4] + neiSolid[5];
    int nSolidE = neiSolid[6]  + neiSolid[7]  + neiSolid[8]  + neiSolid[9]  +
                  neiSolid[10] + neiSolid[11] + neiSolid[12] + neiSolid[13] +
                  neiSolid[14] + neiSolid[15] + neiSolid[16] + neiSolid[17];
    int nSolidC = neiSolid[18] + neiSolid[19] + neiSolid[20] + neiSolid[21] +
                  neiSolid[22] + neiSolid[23] + neiSolid[24] + neiSolid[25];
    int nSolid = nSolidF + nSolidE + nSolidC;

    // Bulk node
    if (nSolid == 0) return 0;

    // Determine surface type
    if (nSolidF == 0) {
      if (nSolidE == 0 && nSolidC == 1) {
        return 1;
      } else if (nSolidE == 1 && nSolidC <= 2) {
        return 2;
      } else if (nSolidE == 2 && nSolidC <= 3) {
        return 3;
      }

    } else if (nSolidF == 1) {
      bool oneSide =
        ((neiSolid[0] || neiSolid[5]) && !(neiSolid[10] || neiSolid[11] || neiSolid[12] || neiSolid[13])) ||
        ((neiSolid[1] || neiSolid[4]) && !(neiSolid[7]  || neiSolid[8]  || neiSolid[15] || neiSolid[16])) ||
        ((neiSolid[2] || neiSolid[3]) && !(neiSolid[6]  || neiSolid[9]  || neiSolid[14] || neiSolid[17]));
      if (oneSide) {
        return 4;
      } else {
        return 5;
      }

    } else if (nSolidF == 2) {
      return 6;

    } else if (nSolidF == 3) {
      return 7;
    }
    throw std::runtime_error("Undefined surface type");
  }

}
