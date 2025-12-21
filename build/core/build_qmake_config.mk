
#QMAKE_SPEC_FILE:=$(OUT_HOST_DIR)/usr/mkspecs/devices/linux-mips-manhaton-g++/qmake.conf
QMAKE=$(OUT_HOST_DIR)/usr/bin/qmake

$(QMAKE_SPEC_FILE):$(BUILD_SYSTEM)/build_qmake.mk
	@mkdir -p $(dir $@)
	@echo "" > $@
	@echo 'CROSS_COMPILE = ' $(DEVICE_COMPILER_PREFIX)- >> $@
	@echo 'include(../common/linux_device_pre.conf)' >> $@
	@echo 'QMAKE_CFLAGS            = -EL -march=mips32r2' >> $@
	@echo 'QMAKE_CXXFLAGS          = $${QMAKE_CFLAGS}'  >> $@
	@echo 'QMAKE_LFLAGS            = -EL' >> $@
	@echo 'QT_QPA_DEFAULT_PLATFORM = linuxfb' >> $@
	@echo 'include(../common/linux_device_post.conf)' >> $@
	@echo 'load(qt_config)' >> $@
	@echo '' >> $@
	@echo '#include "../../linux-g++/qplatformdefs.h"' > $(dir $@)qplatformdefs.h
