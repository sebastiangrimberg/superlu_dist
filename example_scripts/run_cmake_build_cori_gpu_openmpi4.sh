#!/bin/bash
# Bash script to submit many files to Cori/Edison/Queue
module unload cray-mpich/7.7.6
module swap PrgEnv-intel PrgEnv-gnu
export MKLROOT=/opt/intel/compilers_and_libraries_2019.3.199/linux/mkl
export LD_0LIBRARY_PATH=$LD_LIBRARY_PATH:/opt/intel/compilers_and_libraries_2019.3.199/linux/mkl/lib/intel64

# module use /global/common/software/m3169/cori/modulefiles
# module unload openmpi
module unload cmake
module load cmake
# module load cudatoolkit
module load cgpu
module load cuda/11.1.1
module swap gcc gcc/8.3.0
module load openmpi/4.0.3

export ACC=GPU
export CRAYPE_LINK_TYPE=dynamic
export PARMETIS_ROOT=~/Cori/my_software/parmetis-4.0.3_dynamic_openmpi403_gnu
export PARMETIS_BUILD_DIR=${PARMETIS_ROOT}/build/Linux-x86_64 
rm -rf CMakeCache.txt
rm -rf CMakeFiles
rm -rf CTestTestfile.cmake
rm -rf cmake_install.cmake
rm -rf DartConfiguration.tcl 


cmake .. \
	-DTPL_PARMETIS_INCLUDE_DIRS="${PARMETIS_ROOT}/include;${PARMETIS_ROOT}/metis/include" \
	-DTPL_PARMETIS_LIBRARIES="${PARMETIS_BUILD_DIR}/libparmetis/libparmetis.so;${PARMETIS_BUILD_DIR}/libmetis/libmetis.so;${LIB_VTUNE};${CUDA_ROOT}/lib64/libcublas.so;${CUDA_ROOT}/lib64/libcudart.so" \
	-DBUILD_SHARED_LIBS=ON \
	-DTPL_BLAS_LIBRARIES="${MKLROOT}/lib/intel64/libmkl_gf_lp64.so;${MKLROOT}/lib/intel64/libmkl_gnu_thread.so;${MKLROOT}/lib/intel64/libmkl_core.so;${MKLROOT}/lib/intel64/libmkl_def.so;${MKLROOT}/lib/intel64/libmkl_avx.so" \
	-DTPL_LAPACK_LIBRARIES="${MKLROOT}/lib/intel64/libmkl_gf_lp64.so;${MKLROOT}/lib/intel64/libmkl_gnu_thread.so;${MKLROOT}/lib/intel64/libmkl_core.so;${MKLROOT}/lib/intel64/libmkl_def.so;${MKLROOT}/lib/intel64/libmkl_avx.so" \
	-DCMAKE_C_COMPILER=mpicc \
    -DCMAKE_CXX_COMPILER=mpic++ \
    -DCMAKE_Fortran_COMPILER=mpif90 \
	-DCMAKE_INSTALL_PREFIX=. \
	-DTPL_ENABLE_CUDALIB=ON \
	-DTPL_ENABLE_LAPACKLIB=ON \
	-DCMAKE_BUILD_TYPE=Release \
	-DCMAKE_VERBOSE_MAKEFILE:BOOL=ON \
	-DCMAKE_CXX_FLAGS="-Ofast -DRELEASE ${INC_VTUNE} -I${CUDA_ROOT}/include" \
    -DCMAKE_C_FLAGS="-DGPU_SOLVE -std=c11 -DPRNTlevel=1 -DPROFlevel=0 -DDEBUGlevel=0 ${INC_VTUNE} -I${CUDA_ROOT}/include" \
	-DCMAKE_CUDA_FLAGS="-lineinfo --disable-warnings -DPRNTlevel=1 -DPROFlevel=0 -DDEBUGlevel=0 -gencode arch=compute_70,code=sm_70 -I/usr/common/software/openmpi/4.0.3/gcc/8.3.0/cuda/10.2.89/include"
make pddrive	
make pddrive3d	
# make install 	
#	-DTPL_BLAS_LIBRARIES="/opt/intel/compilers_and_libraries_2017.2.174/linux/mkl/lib/intel64/libmkl_intel_lp64.so;/opt/intel/compilers_and_libraries_2017.2.174/linux/mkl/lib/intel64/libmkl_sequential.so;/opt/intel/compilers_and_libraries_2017.2.174/linux/mkl/lib/intel64/libmkl_core.so"

#	-DTPL_BLAS_LIBRARIES="/opt/intel/compilers_and_libraries_2017.2.174/linux/mkl/lib/intel64/libmkl_intel_lp64.so;/opt/intel/compilers_and_libraries_2017.2.174/linux/mkl/lib/intel64/libmkl_sequential.so;/opt/intel/compilers_and_libraries_2017.2.174/linux/mkl/lib/intel64/libmkl_core.so" \
#        -DCMAKE_CXX_FLAGS="-g -trace -Ofast -std=c++11 -DAdd_ -DRELEASE -tcollect -L$VT_LIB_DIR -lVT $VT_ADD_LIBS" \


#	-DTPL_BLAS_LIBRARIES="/opt/intel/compilers_and_libraries_2017.2.174/linux/mkl/lib/intel64/libmkl_lapack95_lp64.a;/opt/intel/compilers_and_libraries_2017.2.174/linux/mkl/lib/intel64/libmkl_blas95_lp64.a"

#	-DTPL_BLAS_LIBRARIES="/opt/intel/compilers_and_libraries_2017.2.174/linux/mkl/lib/intel64/libmkl_intel_lp64.a;/opt/intel/compilers_and_libraries_2017.2.174/linux/mkl/lib/intel64/libmkl_sequential.a;/opt/intel/compilers_and_libraries_2017.2.174/linux/mkl/lib/intel64/libmkl_core.a"  


#	-DCMAKE_CXX_FLAGS="-Ofast -std=c++11 -DAdd_ -DRELEASE ${INC_VTUNE}" \
# DCMAKE_BUILD_TYPE=Release or Debug compiler options set in CMAKELIST.txt

#        -DCMAKE_C_FLAGS="-g -O0 -std=c99 -DPRNTlevel=2 -DPROFlevel=1 -DDEBUGlevel=0" \
#	-DCMAKE_C_FLAGS="-g -O0 -std=c11 -DPRNTlevel=1 -DPROFlevel=1 -DDEBUGlevel=0" \
