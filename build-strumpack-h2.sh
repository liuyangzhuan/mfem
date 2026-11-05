#!/bin/bash
# MFEM v4.10 against the H2 STRUMPACK (STRUMPACK_H2_integration, bpack_newformat,
# build_bpack_gpu_dense, static, CUDA) and ButterflyPACK (H2_optimization,
# build_gpu).  MFEM itself is CPU-only; STRUMPACK uses the GPUs.  Sequential
# OpenBLAS, no cray-libsci.  STRUMPACK's dependencies are given as a list (from
# its examples' link line): MFEM's own STRUMPACK_REQUIRED_PACKAGES search does
# not know ButterflyPACK, CUDA, MAGMA, ZFP or PaRSEC.
#   usage: ./build-strumpack-h2.sh [targets...]   (default: mfem ex1p ex2p ex3p)
set -e
module load PrgEnv-gnu cray-fftw cmake/3.30.2 cudatoolkit/13.2 craype-accel-nvidia80 >/dev/null 2>&1
module unload cray-libsci >/dev/null 2>&1

R=/global/cfs/cdirs/m2957/liuyangz/my_research
L=/global/cfs/cdirs/m2957/lib/lib/PrgEnv-gnu
SP=$R/STRUMPACK_H2_integration/build_bpack_gpu_dense/install
BP=$R/ButterflyPACK_H2_partial_factorization/build_gpu/lib64
CU=/opt/nvidia/hpc_sdk/Linux_x86_64/26.5/cuda/13.2/targets/x86_64-linux/lib
ML=/opt/nvidia/hpc_sdk/Linux_x86_64/26.5/math_libs/13.2/lib64
PM=$L/parmetis-4.0.3/install
OB=$L/OpenBLAS_sequential/build/install/lib/libopenblas.so
SL=$L/scalapack-2.2.0_sequential/build/install/lib/libscalapack.so
NV=$L/nvcomp-linux-x86_64-5.3.0.16_cuda13-archive/lib

DEPS="/usr/lib64/gcc/x86_64-suse-linux/14/libgomp.so;$PM/lib/libparmetis.so;$PM/lib/libmetis.so"
DEPS="$DEPS;$BP/libbutterflypack.so.5.0.0"
DEPS="$DEPS;$R/zfp-1.0.0_gcc_perlmutter/install/lib64/libzFORp.so;$R/zfp-1.0.0_gcc_perlmutter/install/lib64/libzfp.so"
DEPS="$DEPS;$SL;$OB;$R/../my_software/parsec_pr759/install/lib64/libparsec.so.4.1.0"
DEPS="$DEPS;/global/cfs/cdirs/m2957/lib/magma_v2.10.0/lib/libmagma.so"
DEPS="$DEPS;$CU/libnvrtc.so;$NV/libnvcomp.so;$NV/libnvcomp_cpu.so;$CU/libcudart.so"
DEPS="$DEPS;$ML/libcusolver.so;$ML/libcusparse.so;$CU/libnvJitLink.so;$ML/libcublas.so;$ML/libcublasLt.so"
DEPS="$DEPS;$CU/stubs/libcuda.so;mpifort_gnu;gfortran;quadmath;dl"

cd "$(dirname "$0")"
mkdir -p build && cd build
cmake .. \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_CXX_COMPILER=CC \
  -DBUILD_SHARED_LIBS=OFF \
  -DMFEM_USE_MPI=YES \
  -DHYPRE_DIR=$R/hypre/src/hypre \
  -DMFEM_USE_METIS=YES -DMFEM_USE_METIS_5=YES \
  -DMETIS_DIR=$PM \
  -DMFEM_USE_STRUMPACK=YES \
  -DSTRUMPACK_DIR=$SP \
  -DSTRUMPACK_INCLUDE_DIRS="$SP/include;$PM/include" \
  -DSTRUMPACK_LIBRARIES="$SP/lib64/libstrumpack.a;$DEPS" \
  -DSTRUMPACK_REQUIRED_PACKAGES="" \
  -DMFEM_ENABLE_EXAMPLES=OFF -DMFEM_ENABLE_MINIAPPS=OFF -DMFEM_ENABLE_TESTING=OFF
if [ $# -gt 0 ]; then make -j 32 "$@"; else make -j 32 mfem && make -j 3 ex1p ex2p ex3p; fi
