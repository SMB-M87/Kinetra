file(MAKE_DIRECTORY "${CONSUMER_BINARY}")
execute_process(COMMAND "${CMAKE_COMMAND}" -S "${KINETRA_SOURCE}/tests/consumer" -B "${CONSUMER_BINARY}"
    "-DKINETRA_SOURCE=${KINETRA_SOURCE}"
    "-DCONSUMER_GRAPHICS=${CONSUMER_GRAPHICS}" "-DCMAKE_PREFIX_PATH=${CONSUMER_PREFIX}"
    "-DCMAKE_CXX_FLAGS=${CONSUMER_CXX_FLAGS}" "-DCMAKE_C_FLAGS=${CONSUMER_C_FLAGS}"
    -DCMAKE_BUILD_TYPE=Release RESULT_VARIABLE result)
if(NOT result EQUAL 0)
    message(FATAL_ERROR "External consumer configuration failed")
endif()
execute_process(COMMAND "${CMAKE_COMMAND}" --build "${CONSUMER_BINARY}" --parallel 2 RESULT_VARIABLE result)
if(NOT result EQUAL 0)
    message(FATAL_ERROR "External consumer build failed")
endif()
execute_process(COMMAND "${CONSUMER_BINARY}/consumer" RESULT_VARIABLE result)
if(NOT result EQUAL 0)
    message(FATAL_ERROR "External simulation consumer failed")
endif()
