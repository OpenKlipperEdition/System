CMAKE_MINIMUM_REQUIRED(VERSION 3.0)

PROJECT(install-wifi-common)

SET(WIFIMAC_ADDR_PATH /data/misc/wifi)

MESSAGE(STATUS ">>> rtl8723ds")

# BCM43362_A2 >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
MESSAGE(STATUS ">>> bcm43362 a2")
install(PROGRAMS ${PROJECT_SOURCE_DIR}/../wifi_bcm/wifi_bcm_43362/fw_bcmdhd-43362a2-apsta.bin DESTINATION /lib/firmware/)
install(PROGRAMS ${PROJECT_SOURCE_DIR}/../wifi_bcm/wifi_bcm_43362/nvram_ap6181.txt  DESTINATION /lib/firmware/)
# <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<< BCM43362_A2

# BCM43438_A0 >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
MESSAGE(STATUS ">>> bcm43438 a0")
file(COPY ${PROJECT_SOURCE_DIR}/../wifi_bcm/wifi_bcm_43438/fw_43438_a0 DESTINATION ${DESTDIR}/lib/firmware)
file(COPY ${PROJECT_SOURCE_DIR}/../wifi_bcm/wifi_bcm_43438/nv_43438_a0 DESTINATION ${DESTDIR}/lib/firmware)
execute_process(COMMAND sh -c "ln -sf ./fw_43438_a0 ${DESTDIR}/lib/firmware/BCM43438_A0")
execute_process(COMMAND sh -c "ln -sf ./nv_43438_a0 ${DESTDIR}/lib/firmware/BCM43438_A0.cal")
install(FILES ${PROJECT_SOURCE_DIR}/../wifi_bcm/wifi_bcm_43438/fw_43438_a0 DESTINATION /lib/firmware)
install(FILES ${PROJECT_SOURCE_DIR}/../wifi_bcm/wifi_bcm_43438/nv_43438_a0 DESTINATION /lib/firmware)
# <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<< BCM43438_A0

## BCM43438_A1 >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
MESSAGE(STATUS ">>> bcm43438 a1")
file(COPY ${PROJECT_SOURCE_DIR}/../wifi_bcm/wifi_bcm_43438/fw_43438_a1 DESTINATION ${DESTDIR}/lib/firmware)
file(COPY ${PROJECT_SOURCE_DIR}/../wifi_bcm/wifi_bcm_43438/nv_43438_a1 DESTINATION ${DESTDIR}/lib/firmware)
execute_process(COMMAND sh -c "ln -sf ./fw_43438_a1 ${DESTDIR}/lib/firmware/BCM43438_A1")
execute_process(COMMAND sh -c "ln -sf ./nv_43438_a1 ${DESTDIR}/lib/firmware/BCM43438_A1.cal")
install(FILES ${PROJECT_SOURCE_DIR}/../wifi_bcm/wifi_bcm_43438/fw_43438_a1 DESTINATION /lib/firmware/)
install(FILES ${PROJECT_SOURCE_DIR}/../wifi_bcm/wifi_bcm_43438/nv_43438_a1 DESTINATION /lib/firmware/)
## <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<< BCM43438_A1

# BCM4345 >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
MESSAGE(STATUS ">>> bcm4345")
file(COPY ${PROJECT_SOURCE_DIR}/../wifi_bcm/wifi_bcm_4345/fw_bcm43455c0_ag.bin DESTINATION ${DESTDIR}/lib/firmware)
file(COPY ${PROJECT_SOURCE_DIR}/../wifi_bcm/wifi_bcm_4345/nvram_ap6255.txt DESTINATION ${DESTDIR}/lib/firmware)
execute_process(COMMAND sh -c "ln -sf ./fw_bcm43455c0_ag.bin ${DESTDIR}/lib/firmware/BCM43455"
				COMMAND sh -c "ln -sf ./nvram_ap6255.txt ${DESTDIR}/lib/firmware/BCM43455.cal")
install(FILES ${PROJECT_SOURCE_DIR}/../wifi_bcm/wifi_bcm_4345/fw_bcm43455c0_ag.bin 
			  ${PROJECT_SOURCE_DIR}/../wifi_bcm/wifi_bcm_4345/nvram_ap6255.txt
		DESTINATION /lib/firmware/)
# <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<< BCM4345

execute_process(COMMAND ${PROJECT_SOURCE_DIR}/script/create_exec_shell.sh ${DESTDIR} ${WIFIMAC_ADDR_PATH})
