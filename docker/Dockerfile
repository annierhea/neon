
FROM fedora:28

# Install any needed packages specified in dependencies.txt
RUN dnf update -y

RUN dnf install -y wget bzip2 patch lcov git mercurial cmake clang gcc-c++ openblas-devel MUMPS-devel boost-devel vtk-devel hwloc-devel scotch-devel tbb-devel arpack-devel libomp-devel

RUN pip3 install gcovr

# Copy the current directory contents into the container at root directory /
COPY docker/add_libraries.diff docker/cmake_patch.diff docker/install_pastix.sh /

RUN sh install_pastix.sh
