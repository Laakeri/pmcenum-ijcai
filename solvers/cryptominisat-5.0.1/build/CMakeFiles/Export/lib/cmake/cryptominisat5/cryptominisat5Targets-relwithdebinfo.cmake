#----------------------------------------------------------------
# Generated CMake target import file for configuration "RelWithDebInfo".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "libcryptominisat5" for configuration "RelWithDebInfo"
set_property(TARGET libcryptominisat5 APPEND PROPERTY IMPORTED_CONFIGURATIONS RELWITHDEBINFO)
set_target_properties(libcryptominisat5 PROPERTIES
  IMPORTED_LINK_INTERFACE_LIBRARIES_RELWITHDEBINFO ""
  IMPORTED_LOCATION_RELWITHDEBINFO "/usr/local/lib/libcryptominisat5.so.5.0"
  IMPORTED_SONAME_RELWITHDEBINFO "libcryptominisat5.so.5.0"
  )

list(APPEND _IMPORT_CHECK_TARGETS libcryptominisat5 )
list(APPEND _IMPORT_CHECK_FILES_FOR_libcryptominisat5 "/usr/local/lib/libcryptominisat5.so.5.0" )

# Import target "cryptominisat5" for configuration "RelWithDebInfo"
set_property(TARGET cryptominisat5 APPEND PROPERTY IMPORTED_CONFIGURATIONS RELWITHDEBINFO)
set_target_properties(cryptominisat5 PROPERTIES
  IMPORTED_LOCATION_RELWITHDEBINFO "${_IMPORT_PREFIX}/bin/cryptominisat5"
  )

list(APPEND _IMPORT_CHECK_TARGETS cryptominisat5 )
list(APPEND _IMPORT_CHECK_FILES_FOR_cryptominisat5 "${_IMPORT_PREFIX}/bin/cryptominisat5" )

# Import target "cryptominisat5_simple" for configuration "RelWithDebInfo"
set_property(TARGET cryptominisat5_simple APPEND PROPERTY IMPORTED_CONFIGURATIONS RELWITHDEBINFO)
set_target_properties(cryptominisat5_simple PROPERTIES
  IMPORTED_LOCATION_RELWITHDEBINFO "${_IMPORT_PREFIX}/bin/cryptominisat5_simple"
  )

list(APPEND _IMPORT_CHECK_TARGETS cryptominisat5_simple )
list(APPEND _IMPORT_CHECK_FILES_FOR_cryptominisat5_simple "${_IMPORT_PREFIX}/bin/cryptominisat5_simple" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
