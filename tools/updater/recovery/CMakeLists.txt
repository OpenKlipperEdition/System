cmake_minimum_required(VERSION 3.8.0)
project(recovery LANGUAGES C)
set(PROJRECOVERY recovery)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=gnu11 -Wall -Wuninitialized -Wundef")
if(KERNEL_5_10)
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -D_TIME_BITS=64 -D_FILE_OFFSET_BITS=64 -D_LARGEFILE_SOURCE -D_LARGEFILE64_SOURCE")
endif()

if(BUILD_MMC_DEVICE)
	add_definitions(-DLOCAL_PACKAGE)
	add_definitions(-DMMC_DEVICE)
	add_definitions(-DMMC_INDEX=${MMC_INDEX})
endif()

option(NV_USE_STATIC_LIB "get recovery needed static library" "OFF")
if(NV_USE_STATIC_LIB)
# 函数定义: 按照原文定义过滤文件列表
function(filter_files ALL_FILES_AS_STRING EXCLUDE_DIRS_STR EXCLUDE_FILES_STR OUTPUT_VAR)
    string(REPLACE "," ";" ALL_SRC_FILES "${ALL_FILES_AS_STRING}")
    string(REPLACE "," ";" EXCLUDE_DIRS "${EXCLUDE_DIRS_STR}")
    string(REPLACE "," ";" EXCLUDE_FILES "${EXCLUDE_FILES_STR}")

    set(FILTERED_FILES)
    foreach(file ${ALL_SRC_FILES})
        set(skip FALSE)
        foreach(dir ${EXCLUDE_DIRS})
            if(file MATCHES "${dir}")
                set(skip TRUE)
                break()
            endif()
        endforeach()

        if(NOT skip)
            foreach(excludeFile ${EXCLUDE_FILES})
                get_filename_component(baseName ${file} NAME)
                if(${baseName} MATCHES ${excludeFile})
                    set(skip TRUE)
                    break()
                endif()
            endforeach()
        endif()

        if(NOT skip)
            list(APPEND FILTERED_FILES "${file}")
        endif()
    endforeach()
    set(${OUTPUT_VAR} ${FILTERED_FILES} PARENT_SCOPE)
endfunction()

set(COMMON_INCLUDE_DIRS
    "${PROJECT_SOURCE_DIR}/include"
    "${PROJECT_SOURCE_DIR}/include/lib"
    "${PROJECT_SOURCE_DIR}/include/lib/zlib"
    "${PROJECT_SOURCE_DIR}/include/lib/minizip"
    )

set(EXCLUDE_DIRS
    "testunit"
    "SDL"
    "zzip"
    )
set(EXCLUDE_FILES
    "libiniparser*"
    "win"
    )

#block
set(LIB_BLOCK "block")
file(GLOB_RECURSE ALL_BLOCK_SRC_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/block/*.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/utils/*.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/lib/mtd/*.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/lib/crc/*.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/lib/md5/*.c"
    )
list(JOIN ALL_BLOCK_SRC_FILES "," ALL_BLOCK_SRC_FILES_STR)
list(JOIN EXCLUDE_DIRS "," EXCLUDE_DIRS_STR)
list(JOIN EXCLUDE_FILES "," EXCLUDE_FILES_STR)
filter_files(${ALL_BLOCK_SRC_FILES_STR} ${EXCLUDE_DIRS_STR} ${EXCLUDE_FILES_STR} FILTERED_BLOCK_SRC_FILES )
add_library(${LIB_BLOCK} STATIC ${FILTERED_BLOCK_SRC_FILES})
target_include_directories(${LIB_BLOCK} PUBLIC ${COMMON_INCLUDE_DIRS})

#mxml
set(LIB_MXML "mxml")
file(GLOB_RECURSE ALL_MXML_SRC_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/lib/mxml/*.c"
    )
add_library(${LIB_MXML} STATIC ${ALL_MXML_SRC_FILES})
target_include_directories(${LIB_MXML} PUBLIC ${COMMON_INCLUDE_DIRS})

#zip directory
set(LIB_ZIP "zip")
file(GLOB_RECURSE ALL_ZIP_SRC_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/lib/zip/*.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/lib/zlib/zlib-1.2.8/adler32.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/lib/zlib/zlib-1.2.8/crc32.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/lib/zlib/zlib-1.2.8/deflate.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/lib/zlib/zlib-1.2.8/infback.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/lib/zlib/zlib-1.2.8/inffast.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/lib/zlib/zlib-1.2.8/inflate.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/lib/zlib/zlib-1.2.8/inftrees.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/lib/zlib/zlib-1.2.8/trees.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/lib/zlib/zlib-1.2.8/zutil.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/lib/zlib/zlib-1.2.8/compress.c"
	"${CMAKE_CURRENT_SOURCE_DIR}/lib/zlib/zlib-1.2.8/uncompr.c"
    )
list(JOIN ALL_ZIP_SRC_FILES "," ALL_ZIP_SRC_FILES_STR)
filter_files(${ALL_ZIP_SRC_FILES_STR} ${EXCLUDE_DIRS_STR} ${EXCLUDE_FILES_STR} FILTERED_ZIP_SRC_FILES )
add_library(${LIB_ZIP} STATIC ${FILTERED_ZIP_SRC_FILES})
target_include_directories(${LIB_ZIP} PUBLIC ${COMMON_INCLUDE_DIRS})

#mincrypt
set(LIB_MINCRYPT "mincrypt")
file(GLOB_RECURSE ALL_MINCRYPT_SRC_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/lib/mincrypt/*.c"
    )
add_library(${LIB_MINCRYPT} STATIC ${ALL_MINCRYPT_SRC_FILES})
target_include_directories(${LIB_MINCRYPT} PUBLIC ${COMMON_INCLUDE_DIRS})
file(MAKE_DIRECTORY ${CMAKE_SYSROOT}/usr/lib/recovery)
install(TARGETS ${LIB_BLOCK} ${LIB_MXML} ${LIB_MINCRYPT} ${LIB_ZIP} DESTINATION lib/recovery)
install(FILES include/block/sysinfo/flag.h DESTINATION include/recovery/block)
install(FILES include/block/block_manager.h DESTINATION include/recovery/block)
install(FILES include/lib/libmtd.h DESTINATION include/recovery/lib)
install(FILES include/types.h DESTINATION include/recovery)
else() #NV_USE_STATIC_LIB
add_executable(${PROJRECOVERY}
	"main.c"
	"ota/ota_manager.c"
	"utils/signal_handler.c"
	"utils/compare_string.c"
	"utils/assert.c"
	"utils/verifier.c"
	"utils/minizip.c"
	"utils/file_ops.c"
	"utils/png_decode.c"
	"utils/common.c"
	"block/block_manager.c"
	"block/blocks/mtd/mtd.c"
	"block/blocks/mtd/base.c"
	"block/blocks/mmc.c"
	"block/sysinfo/sysinfo_manager.c"
	"block/sysinfo/flag.c"
	"block/fs/fs_manager.c"
	"block/fs/normal.c"
	"block/fs/jffs2.c"
	"block/fs/cramfs.c"
	"block/fs/ubifs.c"
	"block/fs/yaffs2.c"
	"net/net_interface.c"
	"configure/configure_file.c"
	"configure/update_file.c"
	"lib/zlib/zlib-1.2.8/adler32.c"
	"lib/zlib/zlib-1.2.8/crc32.c"
	"lib/zlib/zlib-1.2.8/deflate.c"
	"lib/zlib/zlib-1.2.8/infback.c"
	"lib/zlib/zlib-1.2.8/inffast.c"
	"lib/zlib/zlib-1.2.8/inflate.c"
	"lib/zlib/zlib-1.2.8/inftrees.c"
	"lib/zlib/zlib-1.2.8/trees.c"
	"lib/zlib/zlib-1.2.8/zutil.c"
	"lib/zlib/zlib-1.2.8/compress.c"
	"lib/zlib/zlib-1.2.8/uncompr.c"
	"lib/zip/minizip/ioapi.c"
	"lib/zip/minizip/unzip.c"
	"lib/zip/minizip/zip.c"
	"lib/png/libpng-1.6.26/png.c"
	"lib/png/libpng-1.6.26/pngerror.c"
	"lib/png/libpng-1.6.26/pngget.c"
	"lib/png/libpng-1.6.26/pngmem.c"
	"lib/png/libpng-1.6.26/pngpread.c"
	"lib/png/libpng-1.6.26/pngread.c"
	"lib/png/libpng-1.6.26/pngrio.c"
	"lib/png/libpng-1.6.26/pngrtran.c"
	"lib/png/libpng-1.6.26/pngrutil.c"
	"lib/png/libpng-1.6.26/pngset.c"
	"lib/png/libpng-1.6.26/pngtrans.c"
	"lib/png/libpng-1.6.26/pngwio.c"
	"lib/png/libpng-1.6.26/pngwrite.c"
	"lib/png/libpng-1.6.26/pngwtran.c"
	"lib/png/libpng-1.6.26/pngwutil.c"
	"lib/mtd/libmtd_legacy.c"
	"lib/mtd/libmtd.c"
	"lib/mtd/ubi/libubi.c"
	"lib/mtd/ubi/libubigen.c"
	"lib/mtd/ubi/libscan.c"
	"lib/mxml/mxml-2.10/mxml-attr.c"
	"lib/mxml/mxml-2.10/mxml-entity.c"
	"lib/mxml/mxml-2.10/mxml-file.c"
	"lib/mxml/mxml-2.10/mxml-get.c"
	"lib/mxml/mxml-2.10/mxml-index.c"
	"lib/mxml/mxml-2.10/mxml-node.c"
	"lib/mxml/mxml-2.10/mxml-search.c"
	"lib/mxml/mxml-2.10/mxml-set.c"
	"lib/mxml/mxml-2.10/mxml-private.c"
	"lib/mxml/mxml-2.10/mxml-string.c"
	"lib/config/libconfig-1.5/lib/scanner.c"
	"lib/config/libconfig-1.5/lib/scanctx.c"
	"lib/config/libconfig-1.5/lib/strbuf.c"
	"lib/config/libconfig-1.5/lib/libconfig.c"
	"lib/config/libconfig-1.5/lib/grammar.c"
	"lib/mincrypt/rsa.c"
	"lib/mincrypt/rsa_e_3.c"
	"lib/mincrypt/rsa_e_f4.c"
	"lib/mincrypt/sha.c"
	"lib/md5/libmd5.c"
	"lib/crc/libcrc.c"
	"lib/cjson_helper/libcjson_helper.c"
	)

target_link_libraries(
  ${PROJRECOVERY}
  cjson
  m
  dl
  pthread
)

target_include_directories(${PROJRECOVERY} PUBLIC
	"${PROJECT_SOURCE_DIR}/include"
	"${PROJECT_SOURCE_DIR}/include/lib"
	"${PROJECT_SOURCE_DIR}/include/lib/zlib"
	"${PROJECT_SOURCE_DIR}/include/lib/minizip"
)

install(TARGETS ${PROJRECOVERY} DESTINATION "/etc/ota_res/")
install(TARGETS ${PROJRECOVERY} DESTINATION "/usr/sbin/")
INSTALL(DIRECTORY "${PROJECT_SOURCE_DIR}/../default_ota_res/" DESTINATION "/etc/ota_res")
if(RECOVERY_CONF_PATH)
	# 用于覆盖default_ota_res目录下默认的recovery.conf配置
	INSTALL(FILES ${RECOVERY_CONF_PATH}/recovery.conf DESTINATION "/etc/ota_res")
endif()
if(PARTITION_MMC_CONF_PATH)
	INSTALL(FILES ${RECOVERY_CONF_PATH}/partition_mmc.conf DESTINATION "/etc/ota_res")
endif()
install(FILES S40copy_ota_res DESTINATION "/etc/init.d/"
  PERMISSIONS
  OWNER_EXECUTE OWNER_WRITE OWNER_READ GROUP_EXECUTE GROUP_READ WORLD_EXECUTE WORLD_READ)
install(FILES update.sh DESTINATION "/etc/ota_res/"
  PERMISSIONS
  OWNER_EXECUTE OWNER_WRITE OWNER_READ GROUP_EXECUTE GROUP_READ WORLD_EXECUTE WORLD_READ)
install(FILES update_stage.sh DESTINATION "/etc/ota_res/"
  PERMISSIONS
  OWNER_EXECUTE OWNER_WRITE OWNER_READ GROUP_EXECUTE GROUP_READ WORLD_EXECUTE WORLD_READ)
install(FILES update_play.sh DESTINATION "/etc/ota_res/"
  PERMISSIONS
  OWNER_EXECUTE OWNER_WRITE OWNER_READ GROUP_EXECUTE GROUP_READ WORLD_EXECUTE WORLD_READ)
endif() #NV_USE_STATIC_LIB
