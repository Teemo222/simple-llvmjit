add_llvm_component_library(EasyJitPass MODULE
  RegisterPasses.cpp
  Easy.cpp
  Layout.cpp
  Utils.cpp
  MayAliasTracer.cpp
  PLUGIN_TOOL clang
  )

set(EASY_JIT_PASS ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/EasyJitPass${CMAKE_SHARED_LIBRARY_SUFFIX} PARENT_SCOPE)

install(TARGETS EasyJitPass
        LIBRARY DESTINATION lib)

# Get proper shared-library behavior (where symbols are not necessarily
# resolved when the shared library is linked) on OS X.
if(APPLE)
    set_target_properties(EasyJitPass PROPERTIES
        LINK_FLAGS "-undefined dynamic_lookup"
    )
endif(APPLE)
