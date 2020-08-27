include_directories(${DRIVER_INCLUDE_DIRS})

add_executable(ec2writeflash writeflash.c ihex.c)
target_link_libraries(ec2writeflash ec2drv usb)

add_executable(ec2readflash readflash.c ihex.c)
target_link_libraries(ec2readflash ec2drv usb)

add_executable(ec2_update_fw ec2fw.c)
target_link_libraries(ec2_update_fw ec2drv usb)

add_executable(ec2readfw ec2rfw.c)
target_link_libraries(ec2readfw ec2drv usb)

add_executable(ec2device ec2device.cpp)
target_link_libraries(ec2device ec2drv usb)

add_executable(ec2adapters ec3adapters.c ihex.c)
target_link_libraries(ec2adapters ec2drv usb)

add_executable(ec2test-any ec2test-any.cpp ihex.c)
target_link_libraries(ec2test-any ec2drv usb)

