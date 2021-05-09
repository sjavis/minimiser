#ifdef PARALLEL
#include <mpi.h>
#endif

#include <iostream>
#include "communicator.h"


void mpiInit() {
  minim::mpi.init(0, 0);
}

void mpiInit(int *argc, char ***argv) {
  minim::mpi.init(argc, argv);
}


Communicator minim::mpi;

Communicator::Communicator() {
  size = 1;
  rank = 0;
}

void Communicator::init(int *argc, char ***argv) {
#ifdef PARALLEL
  MPI_Init(argc, argv);
  MPI_Comm_size(MPI_COMM_WORLD, &size);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  _init = true;
#else
  std::cerr << "Warning: MPI not initialised. -DPARALLEL flag must be passed to the compiler." << std::endl;
#endif
}

Communicator::~Communicator() {
#ifdef PARALLEL
  if (_init = true) {
    MPI_Finalize();
  }
#endif
}
