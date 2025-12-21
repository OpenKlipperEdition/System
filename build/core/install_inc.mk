ifeq ($(LOCAL_MODULE_BUILD),)
$(error "install_inc: called without LOCAL MODULE")
endif

PREFIX_INCLUDE_FILES_TAR := $(addprefix $(__local_stamp_build)@,\
	$(addprefix $(OUT_$(BUILD_MODE)_INCLUDE_DIR)/,\
		$(subst include/,,$(filter include/%,$(LOCAL_EXPORT_C_INCLUDE_FILES)))))

INCLUDE_FILES_TAR := $(addprefix $(__local_stamp_build)@,\
	$(addprefix $(OUT_$(BUILD_MODE)_INCLUDE_DIR)/,\
		$(filter-out include/%,$(LOCAL_EXPORT_C_INCLUDE_FILES))))

$(PREFIX_INCLUDE_FILES_TAR):$(__local_stamp_build)@$(OUT_$(BUILD_MODE)_INCLUDE_DIR)/%.h:$(LOCAL_PATH)/include/%.h
	$(eval __dest := $(word 2, $(subst @, , $@)))
	$(hide)mkdir -p $(dir $(__dest))
	$(hide)cp -df $< $(__dest)
	$(hide)/usr/bin/install -D /dev/null $@

$(INCLUDE_FILES_TAR):$(__local_stamp_build)@$(OUT_$(BUILD_MODE)_INCLUDE_DIR)/%.h:$(LOCAL_PATH)/%.h
	$(eval __dest := $(word 2, $(subst @, , $@)))
	$(hide)mkdir -p $(dir $(__dest))
	$(hide)cp -df $< $(__dest)
	$(hide)/usr/bin/install -D /dev/null $@


objsA:=$(word 1,$(LOCAL_EXPORT_C_INCLUDE_DIRS))
objs:=$(subst :, ,$(objsA))

ifeq (x$(strip $(word 2,$(objs))),x)

PREFIX_INCLUDE_DIRS_TAR := $(addprefix $(__local_stamp_build)@,\
	$(addprefix $(OUT_$(BUILD_MODE)_INCLUDE_DIR)/,\
		$(subst include/,,$(filter include/%,$(LOCAL_EXPORT_C_INCLUDE_DIRS)))))

INCLUDE_DIRS_TAR := $(addprefix $(__local_stamp_build)@,\
	$(addprefix $(OUT_$(BUILD_MODE)_INCLUDE_DIR)/,\
		$(filter-out include/%,$(LOCAL_EXPORT_C_INCLUDE_DIRS))))

$(PREFIX_INCLUDE_DIRS_TAR):$(__local_stamp_build)@$(OUT_$(BUILD_MODE)_INCLUDE_DIR)/%:$(LOCAL_PATH)/include/%
	$(eval __dest := $(word 2, $(subst @, , $@)))
	$(hide)mkdir -p $(dir $(__dest))
	$(hide)cp -Tdfr $< $(__dest)
	$(hide)/usr/bin/install -D /dev/null $@

$(INCLUDE_DIRS_TAR):$(__local_stamp_build)@$(OUT_$(BUILD_MODE)_INCLUDE_DIR)/%:$(LOCAL_PATH)/%
	$(eval __dest := $(word 2, $(subst @, , $@)))
	$(hide)mkdir -p $(dir $(__dest))
	$(hide)cp -Tdfr $< $(__dest)
	$(hide)/usr/bin/install -D /dev/null $@
else

define SPLIT
	$(eval objs:=$(patsubst %:,% ,$1))
	$(word $2,$(objs))
endef
PREFIX_INCLUDE_DIRS_TAR:=
define COPYDIRS
$(2):$(1)
	$(eval __dest := $(word 2, $(subst @, , $(2))))
	$(hide)mkdir -p $(__dest)
	$(hide)cp -Tdfr $(1) $(__dest)
	$(hide)/usr/bin/install -D /dev/null $(2)
endef

$(foreach oo,$(LOCAL_EXPORT_C_INCLUDE_DIRS),\
	$(eval objs:=$(subst :,  ,$(oo))) \
	$(eval SRC:=$(word 1,$(objs))) \
	$(eval TAR:=$(word 2,$(objs))) \
	$(eval PSRC:=$(LOCAL_PATH)/$(SRC)) \
	$(eval PTAR:=$(__local_stamp_build)/$(SRC)/@$(OUT_$(BUILD_MODE)_USER_DIR)/$(TAR)) \
	$(eval PREFIX_INCLUDE_DIRS_TAR += $(PTAR))\
	$(eval $(call COPYDIRS,$(PSRC),$(PTAR))) \
)
endif

ALL_INCLUDE_DEPANNER:= $(strip $(PREFIX_INCLUDE_FILES_TAR) $(INCLUDE_FILES_TAR) $(PREFIX_INCLUDE_DIRS_TAR) $(INCLUDE_DIRS_TAR))

# exsample:
#    LOCAL_EXPORT_C_INCLUDE_DIRS := src/include:include src/port/include:include
#    cp $(soucedir)/src/inclue to sysroot/usr/include
