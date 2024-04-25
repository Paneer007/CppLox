# Parent project does not export its library target, so this CML implicitly
# depends on being added from it, i.e. the testing is done only from the build
# tree and is not feasible from an install location

project(CppLoxTests LANGUAGES CXX)

# ---- Tests ----

add_executable(CppLox_test source/CppLox_test.cpp)
target_link_libraries(CppLox_test PRIVATE CppLox_lib)
target_compile_features(CppLox_test PRIVATE cxx_std_17)

add_test(NAME CppLox_test COMMAND CppLox_test)

# ---- End-of-file commands ----

add_folders(Test)
