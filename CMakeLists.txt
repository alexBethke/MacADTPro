cmake_minimum_required(VERSION 3.10)
project(MacADTPro VERSION 0.1.0 LANGUAGES C)
if(NOT CMAKE_SYSTEM_NAME STREQUAL "Retro68")
    message(FATAL_ERROR "Configure with the Retro68 68k toolchain file")
endif()
configure_file(src/rainbow.pict rainbow.pict COPYONLY)
configure_file(src/bootstrap_assets/grub2.dmp grub2.dmp COPYONLY)
configure_file(src/bootstrap_assets/PDSpeed.raw PDSpeed.raw COPYONLY)
configure_file(src/bootstrap_assets/adtpro.raw adtpro.raw COPYONLY)
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/disks)
configure_file(src/bootstrap_assets/ADTPRO-2.1.0.DSK
               disks/ADTPRO-2.1.0.DSK COPYONLY)
configure_file(src/version.h.in generated/version.h @ONLY)
configure_file(src/version.r.in generated/version.r @ONLY)
configure_file(distribution/ReadMe.txt.in generated/ReadMe.txt @ONLY)
add_application(MacADTPro CREATOR "MADP" src/main.c src/serial.c src/bootstrap.c src/wide.c src/image_folder.c src/directory.c src/image.c src/image_size.c src/disk_order.c src/transfer.c src/upload.c src/upload_packet.c src/preferences.c src/preferences_format.c src/app.r src/app_icons.rsrc.bin ${CMAKE_CURRENT_BINARY_DIR}/generated/version.r src/bootstrap_text.r src/bootstrap_prodos.r src/bootstrap_client.r)
target_include_directories(MacADTPro PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/generated)
target_compile_options(MacADTPro PRIVATE -Wall -Wextra -ffunction-sections)
set_target_properties(MacADTPro PROPERTIES LINK_FLAGS "-Wl,-gc-sections -Wl,--mac-single")

get_filename_component(RETRO68_TOOL_DIRECTORY "${REZ}" DIRECTORY)
find_program(HMOUNT_EXECUTABLE hmount HINTS ${RETRO68_TOOL_DIRECTORY} REQUIRED)
find_program(HCOPY_EXECUTABLE hcopy HINTS ${RETRO68_TOOL_DIRECTORY} REQUIRED)
find_program(HDEL_EXECUTABLE hdel HINTS ${RETRO68_TOOL_DIRECTORY} REQUIRED)
find_program(HATTRIB_EXECUTABLE hattrib HINTS ${RETRO68_TOOL_DIRECTORY} REQUIRED)
find_program(HUMOUNT_EXECUTABLE humount HINTS ${RETRO68_TOOL_DIRECTORY} REQUIRED)
find_program(PYTHON_EXECUTABLE python3 REQUIRED)
set(DISTRIBUTION_DISK "MacADTPro-${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.dsk")
set(DISTRIBUTION_INPUT "MacADTPro-distribution-input.bin")
add_custom_command(
    OUTPUT ${DISTRIBUTION_DISK}
    COMMAND ${CMAKE_COMMAND} -E remove -f ${DISTRIBUTION_DISK}
    COMMAND ${CMAKE_COMMAND} -E copy MacADTPro.dsk ${DISTRIBUTION_DISK}
    COMMAND ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/scripts/set_bundle_bit.py
            ${CMAKE_CURRENT_BINARY_DIR}/MacADTPro.bin
            ${CMAKE_CURRENT_BINARY_DIR}/${DISTRIBUTION_INPUT}
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/scripts/add_readme_to_disk.sh
            ${HMOUNT_EXECUTABLE} ${HCOPY_EXECUTABLE} ${HDEL_EXECUTABLE}
            ${HATTRIB_EXECUTABLE} ${HUMOUNT_EXECUTABLE}
            ${CMAKE_CURRENT_BINARY_DIR}/${DISTRIBUTION_DISK}
            ${CMAKE_CURRENT_BINARY_DIR}/${DISTRIBUTION_INPUT}
            ${CMAKE_CURRENT_BINARY_DIR}/generated/ReadMe.txt
    COMMAND ${CMAKE_COMMAND} -E remove -f ${DISTRIBUTION_INPUT}
    COMMAND ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/scripts/lock_hfs_volume.py
            ${CMAKE_CURRENT_BINARY_DIR}/${DISTRIBUTION_DISK}
    DEPENDS MacADTPro_APPL MacADTPro.bin MacADTPro.dsk
            ${CMAKE_CURRENT_BINARY_DIR}/generated/ReadMe.txt
            scripts/add_readme_to_disk.sh scripts/lock_hfs_volume.py
            scripts/set_bundle_bit.py
    VERBATIM)
add_custom_target(MacADTPro_distribution ALL
                  DEPENDS ${DISTRIBUTION_DISK})
