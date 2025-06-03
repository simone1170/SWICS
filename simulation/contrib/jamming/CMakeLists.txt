build_lib(
  LIBNAME jamming
  SOURCE_FILES 
    model/mmwave-jamming-model.cc
    helper/jamming-model-helper.cc
  HEADER_FILES 
    model/mmwave-jamming-model.h
    helper/jamming-model-helper.h

  LIBRARIES_TO_LINK ${libspectrum} ${libnr} ${libmmwave}
                   
  TEST_SOURCES
    test/simple-jamming-test.cc
)