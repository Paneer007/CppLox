install(
    TARGETS CppLox_exe
    RUNTIME COMPONENT CppLox_Runtime
)

if(PROJECT_IS_TOP_LEVEL)
  include(CPack)
endif()
