file(REMOVE_RECURSE
  "lib/libriss-coprocessor.pdb"
  "lib/libriss-coprocessor.a"
)

# Per-language clean rules from dependency scanning.
foreach(lang CXX)
  include(CMakeFiles/riss-coprocessor-lib-static.dir/cmake_clean_${lang}.cmake OPTIONAL)
endforeach()
