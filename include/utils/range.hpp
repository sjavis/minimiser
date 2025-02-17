#ifndef MINIM_RANGE_HPP
#define MINIM_RANGE_HPP


namespace minim {
  namespace range {

    inline int make1dIndex(const vector<int>& indices, const vector<int>& sizes) {
      int index = indices[0];
      for (int i=1; i<(int)indices.size(); i++) {
        index = index*sizes[i] + indices[i];
      }
      return index;
    }

    inline std::vector<int> makeNdIndices(int index, const std::vector<int>& sizes) {
      int nDim = sizes.size();
      std::vector<int> indices(nDim);
      for (int iDim=nDim-1; iDim>0; iDim--) {
        indices[iDim] = index % sizes[iDim];
        index = index / sizes[iDim];
      }
      indices[0] = index;
      return indices;
    }


    template <typename TOut>
    Iterator<TOut>::Iterator(std::vector<int> xSize, std::vector<int> xStart, std::vector<int> xEnd, std::vector<int> xHalo)
      : xSize(xSize), xStart(xStart), xEnd(xEnd), xHalo(xHalo)
    {
      nDim = xSize.size();
      stepSize = vector<int>(nDim);
      int step = 1;
      int higherDims = 1;
      for (int iDim=nDim-1; iDim>=0; --iDim) {
        stepSize[iDim] = step;
        step += 2 * xHalo[iDim] * higherDims;
        higherDims *= xSize[iDim];
      }
    }

    template <typename TOut>
    Iterator<TOut> Iterator<TOut>::operator()(int i) {
      this->i = i;
      this->x = range::makeNdIndices(i, xSize);
      return *this;
    }

    template <>
    inline int Iterator<int>::operator*() const {
      return i;
    }

    template <>
    inline std::vector<int> Iterator<std::vector<int>>::operator*() const {
      return x;
    }

    // Increment x and i, skipping the halo nodes
    template <typename TOut>
    Iterator<TOut>& Iterator<TOut>::operator++() {
      for (int iDim=nDim-1; iDim>=0; iDim--) {
        x[iDim]++;
        if (x[iDim] < xEnd[iDim]) {
          i += stepSize[iDim];
          return *this;
        }
        x[iDim] = xStart[iDim];
      }
      i = -1; // To match end()
      return *this;
    }

  }


  inline RangeX::RangeX(std::vector<int> xSize)
    : iStart(0), iter(xSize, std::vector<int>(xSize.size()), xSize, std::vector<int>(xSize.size()))
  {
    // Ensure Iterator does not run if any size is 0
    for (int s : xSize) {
      if (s <= 0) {
        iStart = -1;
        return;
      }
    }
  }

  inline RangeX::RangeX(std::vector<int> xSize, int halo)
   : RangeX(xSize, std::vector<int>(xSize.size(), halo))
  {}

  inline RangeX::RangeX(std::vector<int> xSize, std::vector<int> xHalo)
   : iter({}, {}, {}, {})
  {
    int nDim = xSize.size();
    std::vector<int> xEnd(nDim);
    for (int i=0; i<nDim; i++) {
      xEnd[i] = xSize[i] - xHalo[i];
    }
    iter = Iterator(xSize, xHalo, xEnd, xHalo);
    iStart = range::make1dIndex(xHalo, xSize);
    // Ensure Iterator does not run if any size is 0
    for (int i=0; i<nDim; i++) {
      if (xSize[i] <= 2*xHalo[i]) {
        iStart = -1;
        return;
      }
    }
  }


  inline RangeI::RangeI(std::vector<int> xSize)
    : iStart(0), iter(xSize, std::vector<int>(xSize.size()), xSize, std::vector<int>(xSize.size()))
  {
    // Ensure Iterator does not run if any size is 0
    for (int s : xSize) {
      if (s <= 0) {
        iStart = -1;
        return;
      }
    }
  }

  inline RangeI::RangeI(std::vector<int> xSize, int halo)
   : RangeI(xSize, std::vector<int>(xSize.size(), halo))
  {}

  inline RangeI::RangeI(std::vector<int> xSize, std::vector<int> xHalo)
   : iter({}, {}, {}, {})
  {
    int nDim = xSize.size();
    std::vector<int> xEnd(nDim);
    for (int i=0; i<nDim; i++) {
      xEnd[i] = xSize[i] - xHalo[i];
    }
    iter = Iterator(xSize, xHalo, xEnd, xHalo);
    iStart = range::make1dIndex(xHalo, xSize);
    // Ensure Iterator does not run if any size is 0
    for (int i=0; i<nDim; i++) {
      if (xSize[i] <= 2*xHalo[i]) {
        iStart = -1;
        return;
      }
    }
  }

}

#endif
