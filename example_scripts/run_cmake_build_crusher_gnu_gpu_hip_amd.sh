#!/bin/bash
# module load parmetis/4.0.3

module swap PrgEnv-cray PrgEnv-gnu
module load cmake
module load rocm/5.1.0
module load cray-libsci/22.12.1.1	
module load cray-mpich/8.1.17		 
export LD_LIBRARY_PATH="$CRAY_LD_LIBRARY_PATH:$LD_LIBRARY_PATH"

export CRAYPE_LINK_TYPE=dynamic
export PARMETIS_ROOT=/ccs/home/liuyangz/my_software/parmetis-4.0.3_crusher_gcc
export PARMETIS_BUILD_DIR=${PARMETIS_ROOT}/build/Linux-x86_64
export ACC=GPU
rm -rf CMakeCache.txt
rm -rf CMakeFiles
rm -rf CTestTestfile.cmake
rm -rf cmake_install.cmake
rm -rf DartConfiguration.tcl 




cmake .. \
	-DTPL_PARMETIS_INCLUDE_DIRS="${PARMETIS_ROOT}/include;${PARMETIS_ROOT}/metis/include" \
	-DTPL_PARMETIS_LIBRARIES="${PARMETIS_BUILD_DIR}/libparmetis/libparmetis.so;${PARMETIS_BUILD_DIR}/libmetis/libmetis.so;/opt/rocm-5.1.0/lib/libroctx64.so;/opt/rocm-5.1.0/lib/libroctracer64.so" \
	-DBUILD_SHARED_LIBS=OFF \
	-DCMAKE_Fortran_COMPILER=ftn \
	-DCMAKE_C_COMPILER=cc \
	-DCMAKE_CXX_COMPILER=CC \
	-DCMAKE_INSTALL_PREFIX=. \
	-DTPL_ENABLE_LAPACKLIB=ON \
	-DTPL_BLAS_LIBRARIES="/opt/cray/pe/libsci/22.12.1.1/GNU/9.1/x86_64/lib/libsci_gnu_82_mpi_mp.so;/opt/cray/pe/libsci/22.12.1.1/GNU/9.1/x86_64/lib/libsci_gnu_82_mp.so" \
	-DTPL_LAPACK_LIBRARIES="/opt/cray/pe/libsci/22.12.1.1/GNU/9.1/x86_64/lib/libsci_gnu_82_mpi_mp.so;/opt/cray/pe/libsci/22.12.1.1/GNU/9.1/x86_64/lib/libsci_gnu_82_mp.so" \
	-DCMAKE_BUILD_TYPE=Release \
	-DTPL_ENABLE_HIPLIB=ON \
	-DXSDK_ENABLE_Fortran=OFF \
	-DHIP_HIPCC_FLAGS="--amdgpu-target=gfx906,gfx908,gfx90a -I/opt/cray/pe/mpich/8.1.17/ofi/GNU/9.1/include" \
	-DCMAKE_VERBOSE_MAKEFILE:BOOL=ON \
	-DCMAKE_CXX_FLAGS="-Wno-format -Wno-unused-value -Wno-return-type -Wno-unsequenced -Wno-switch -Wno-parentheses -DPRNTlevel=1 -DPROFlevel=0 -DDEBUGlevel=0 " \
	-DCMAKE_C_FLAGS="  -Wno-format -Wno-unused-value -Wno-return-type -Wno-unsequenced -Wno-switch -Wno-parentheses -DPRNTlevel=1 -DPROFlevel=0 -DDEBUGlevel=0 "
make pddrive -j16	
make pddrive3d -j16	
#	-DTPL_BLAS_LIBRARIES="/opt/intel/compilers_and_libraries_2017.2.174/linux/mkl/lib/intel64/libmkl_intel_lp64.so;/opt/intel/compilers_and_libraries_2017.2.174/linux/mkl/lib/intel64/libmkl_sequential.so;/opt/intel/compilers_and_libraries_2017.2.174/linux/mkl/lib/intel64/libmkl_core.so"

#	-DTPL_BLAS_LIBRARIES="/opt/intel/compilers_and_libraries_2017.2.174/linux/mkl/lib/intel64/libmkl_intel_lp64.so;/opt/intel/compilers_and_libraries_2017.2.174/linux/mkl/lib/intel64/libmkl_sequential.so;/opt/intel/compilers_and_libraries_2017.2.174/linux/mkl/lib/intel64/libmkl_core.so" \
#        -DCMAKE_CXX_FLAGS="-g -trace -Ofast -std=c++11 -DAdd_ -DRELEASE -tcollect -L$VT_LIB_DIR -lVT $VT_ADD_LIBS" \


#	-DTPL_BLAS_LIBRARIES="/opt/intel/compilers_and_libraries_2017.2.174/linux/mkl/lib/intel64/libmkl_lapack95_lp64.a;/opt/intel/compilers_and_libraries_2017.2.174/linux/mkl/lib/intel64/libmkl_blas95_lp64.a"

#	-DTPL_BLAS_LIBRARIES="/opt/intel/compilers_and_libraries_2017.2.174/linux/mkl/lib/intel64/libmkl_intel_lp64.a;/opt/intel/compilers_and_libraries_2017.2.174/linux/mkl/lib/intel64/libmkl_sequential.a;/opt/intel/compilers_and_libraries_2017.2.174/linux/mkl/lib/intel64/libmkl_core.a"  


#	-DCMAKE_CXX_FLAGS="-Ofast -std=c++11 -DAdd_ -DRELEASE ${INC_VTUNE}" \
# DCMAKE_BUILD_TYPE=Release or Debug compiler options set in CMAKELIST.txt

#        -DCMAKE_C_FLAGS="-g -O0 -std=c99 -DPRNTlevel=2 -DPROFlevel=1 -DDEBUGlevel=0" \
#	-DCMAKE_C_FLAGS="-g -O0 -std=c11 -DPRNTlevel=1 -DPROFlevel=1 -DDEBUGlevel=0" \
