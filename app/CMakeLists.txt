# Build the app, using the HAL
# find_package(Threads REQUIRED)
include_directories(include)
file(GLOB MY_SOURCES "src/*.c")
add_executable(beatbox ${MY_SOURCES})

# Make use of the HAL library
target_link_libraries(beatbox LINK_PRIVATE hal)

#ALSA Support
find_package(ALSA REQUIRED)
find_package(Threads REQUIRED)
target_link_libraries(beatbox LINK_PRIVATE asound)
target_link_libraries(beatbox LINK_PRIVATE Threads::Threads)



# Copy executable to final location
add_custom_command(TARGET beatbox POST_BUILD 
  COMMAND "${CMAKE_COMMAND}" -E copy 
     "$<TARGET_FILE:beatbox>"
     "~/cmpt433/public/myApps/beatbox" 
  COMMENT "Copying ARM executable to public NFS directory")

# Copy WAV folder to NFS
add_custom_command(TARGET beatbox POST_BUILD
    COMMAND "${CMAKE_COMMAND}" -E copy_directory
    "${CMAKE_SOURCE_DIR}/wave-files"
    "~/cmpt433/public/myApps/beatbox-wav-files" 
    COMMENT "Copying WAVE files to public NFS directory")

# Deploy Node.js and install packages
add_custom_command(TARGET beatbox POST_BUILD
    COMMAND make -C "${CMAKE_SOURCE_DIR}/app/web-server" deploy node_install
    COMMENT "Deploying Node.js server code and installing packages")
