
add_library(catch_main STATIC test_main.cpp)
add_dependencies(catch_main catch)
target_include_directories(catch_main PRIVATE ${CATCH_INCLUDE_DIR})

foreach(test
             boundary
             constitutive
             dof_numbering
             gauss_quadrature
             linear_beam_theory
             linear_solvers
             eigen_solvers
             jacobian_determinant
             material
             material_coordinates
             matrix_system
             mesh
             nodal_coordinates
             nodal_variables
             tensor
             time_stepping
             time_integrators
             trapezoidal)

    add_executable(${test}_test ${test}.cpp)

    add_dependencies(${test}_test neon catch_main)

    target_link_libraries(${test}_test PRIVATE catch_main neon OpenMP::OpenMP_CXX)

    target_include_directories(${test}_test PUBLIC ${CMAKE_SOURCE_DIR}/src
                                                   ${CATCH_INCLUDE_DIR}
                                                   ${EIGEN_INCLUDE_DIR}
                                                   ${CATCH_INCLUDE_DIR}
                                                   ${VTK_INCLUDE_DIRS}
                                                   ${RV3_INCLUDE_DIR})

    set_target_properties(${test}_test PROPERTIES CXX_STANDARD 17
                                                  CXX_STANDARD_REQUIRED YES
                                                  CXX_EXTENSIONS NO
                                       COMPILE_FLAGS "-Wall -Werror")

    if(ENABLE_COVERAGE)
        set_target_properties(${test}_test PROPERTIES COMPILE_FLAGS "--coverage")
        target_link_libraries(${test}_test PRIVATE --coverage)
    endif()

    add_test(${test}_test ${CMAKE_CURRENT_BINARY_DIR}/${test}_test)
endforeach()
