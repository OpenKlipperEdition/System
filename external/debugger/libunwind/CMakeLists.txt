set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -D__STDC_FORMAT_MACROS")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DHAVE_CONFIG_H -DNDEBUG -D_GNU_SOURCE -DHAVE_ENDIAN_H")
set(PROJLIBNAME unwind)

set(libunwind_src_files_asm_arm
  "src/arm/getcontext.S"
  )
set(libunwind_src_files_arm

 "src/arm/Gis_signal_frame.c"
 "src/arm/Gex_tables.c"
 "src/arm/Lis_signal_frame.c"
 "src/arm/Lex_tables.c"
  )

set(libunwind_src_files_arm64
	"src/aarch64/Gis_signal_frame.c"
	"src/aarch64/Lis_signal_frame.c"
	)
set(libunwind_src_files_asm_mips
 "src/mips/getcontext-android.S"
  )
set(libunwind_src_files_mips
 "src/mips/Gis_signal_frame.c"
 "src/mips/Lis_signal_frame.c"
 )

set(libunwind_src_files_asm_x86
  "src/x86/getcontext-linux.S"
  )

set(libunwind_src_files_x86
	"src/x86/Gos-linux.c"
	"src/x86/Los-linux.c"
	)

set(libunwind_src_files_asm_x86_64
  "src/x86_64/getcontext.S"
  "src/x86_64/setcontext.S"
)
set(libunwind_src_files_x86_64

	"src/x86_64/Gstash_frame.c"
	"src/x86_64/Gtrace.c"
	"src/x86_64/Gos-linux.c"
	"src/x86_64/Lstash_frame.c"
	"src/x86_64/Ltrace.c"
	"src/x86_64/Los-linux.c"
	)

set(libunwind_src_files_mips64 ${libunwind_src_files_mips})

#common_c_includes_mips64 := $(LOCAL_PATH)/include/tdep-mips

# 64-bit architectures
set(libunwind_src_files_arm64 "${libunwind_src_files_arm64}" "src/elf64.c")
set(libunwind_src_files_mips64 "${libunwind_src_files_mips}" "src/elf64.c")
set(libunwind_src_files_x86_64 "${libunwind_src_files_x86_64}" "src/elf64.c")

# 32-bit architectures
set(libunwind_src_files_arm "${libunwind_src_files_arm}" "src/elf32.c")
set(libunwind_src_files_mips "${libunwind_src_files_mips}" "src/elf32.c")
set(libunwind_src_files_x86 "${libunwind_src_files_x86}" "src/elf32.c")
set(libunwind_src_files
  	"src/mi/init.c"
	"src/mi/flush_cache.c"
	"src/mi/mempool.c"
	"src/mi/strerror.c"
	"src/mi/backtrace.c"
	"src/mi/dyn-cancel.c"
	"src/mi/dyn-info-list.c"
	"src/mi/dyn-register.c"
	"src/mi/map.c"
	"src/mi/Lmap.c"
	"src/mi/Ldyn-extract.c"
	"src/mi/Lfind_dynamic_proc_info.c"
	"src/mi/Lget_proc_info_by_ip.c"
	"src/mi/Lget_proc_name.c"
	"src/mi/Lput_dynamic_unwind_info.c"
	"src/mi/Ldestroy_addr_space.c"
	"src/mi/Lget_reg.c"
	"src/mi/Lset_reg.c"
	"src/mi/Lget_fpreg.c"
	"src/mi/Lset_fpreg.c"
	"src/mi/Lset_caching_policy.c"
	"src/mi/Gdyn-extract.c"
	"src/mi/Gdyn-remote.c"
	"src/mi/Gfind_dynamic_proc_info.c"
	"src/mi/Gget_accessors.c"
	"src/mi/Gget_proc_info_by_ip.c"
	"src/mi/Gget_proc_name.c"
	"src/mi/Gput_dynamic_unwind_info.c"
	"src/mi/Gdestroy_addr_space.c"
	"src/mi/Gget_reg.c"
	"src/mi/Gset_reg.c"
	"src/mi/Gget_fpreg.c"
	"src/mi/Gset_fpreg.c"
	"src/mi/Gset_caching_policy.c"
	"src/dwarf/Lexpr.c"
	"src/dwarf/Lfde.c"
	"src/dwarf/Lparser.c"
	"src/dwarf/Lpe.c"
	"src/dwarf/Lstep_dwarf.c"
	"src/dwarf/Lfind_proc_info-lsb.c"
	"src/dwarf/Lfind_unwind_table.c"
	"src/dwarf/Gexpr.c"
	"src/dwarf/Gfde.c"
	"src/dwarf/Gfind_proc_info-lsb.c"
	"src/dwarf/Gfind_unwind_table.c"
	"src/dwarf/Gparser.c"
	"src/dwarf/Gpe.c"
	"src/dwarf/Gstep_dwarf.c"
	"src/dwarf/global.c"
	"src/os-common.c"
	"src/os-linux.c"
	"src/Los-common.c"
  )

set(generate_arches arm arm64 mips x86 x86_64 mips64)

foreach(arch ${generate_arches})
  set(libunwind_src_files_${arch} "${libunwind_src_files_${arch}}"
	"src/${arch}/is_fpreg.c"
	"src/${arch}/regname.c"
	"src/${arch}/Gcreate_addr_space.c"
	"src/${arch}/Gget_proc_info.c"
	"src/${arch}/Gget_save_loc.c"
	"src/${arch}/Gglobal.c"
	"src/${arch}/Ginit.c"
	"src/${arch}/Ginit_local.c"
	"src/${arch}/Ginit_remote.c"
	"src/${arch}/Gregs.c"
	"src/${arch}/Gresume.c"
	"src/${arch}/Gstep.c"
	"src/${arch}/Lcreate_addr_space.c"
	"src/${arch}/Lget_proc_info.c"
	"src/${arch}/Lget_save_loc.c"
	"src/${arch}/Lglobal.c"
	"src/${arch}/Linit.c"
	"src/${arch}/Linit_local.c"
	"src/${arch}/Linit_remote.c"
	"src/${arch}/Lregs.c"
	"src/${arch}/Lresume.c"
	"src/${arch}/Lstep.c"
	)
endforeach(arch)

foreach(arch ${generate_arches})
  set(libunwind_src_files_${arch}
	"${libunwind_src_files_${arch}}" "${libunwind_src_files}"
	)
endforeach(arch)

set(common_c_includes
  include
  src
  )

foreach(arch ${generate_arches})
  set(common_c_includes_${arch} ${common_c_includes} include/tdep-${arch})
endforeach(arch)

set(common_c_includes_mips64 ${common_c_includes_mips})
set(libunwind_src_files_mips64 ${libunwind_src_files_mips})

set_file_to_c(libunwind_src_files_asm_${TARGET_ARCH})

include_directories (
  ${common_c_includes_${TARGET_ARCH}}
  )
add_library(${PROJLIBNAME} STATIC ${libunwind_src_files_asm_${TARGET_ARCH}} ${libunwind_src_files_${TARGET_ARCH}})



SET(PROJLIBNAME unwind-ptrace)

set(libunwind-ptrace_src_files
    "src/ptrace/_UPT_elf.c"
	"src/ptrace/_UPT_accessors.c"
	"src/ptrace/_UPT_access_fpreg.c"
	"src/ptrace/_UPT_access_mem.c"
	"src/ptrace/_UPT_access_reg.c"
	"src/ptrace/_UPT_create.c"
	"src/ptrace/_UPT_destroy.c"
	"src/ptrace/_UPT_find_proc_info.c"
	"src/ptrace/_UPT_get_dyn_info_list_addr.c"
	"src/ptrace/_UPT_put_unwind_info.c"
	"src/ptrace/_UPT_get_proc_name.c"
	"src/ptrace/_UPT_reg_offset.c"
	"src/ptrace/_UPT_resume.c")

add_library(${PROJLIBNAME} STATIC ${libunwind-ptrace_src_files})


















  # message(STATUS "CMAKE_C_FLAGS = " ${CMAKE_C_FLAGS})
  # message(STATUS "CMAKE_C_FLAGS_DEBUG = " ${CMAKE_C_FLAGS_DEBUG})
  # message(STATUS "CMAKE_C_FLAGS_MINSIZEREL = " ${CMAKE_C_FLAGS_MINSIZEREL})
  # message(STATUS "CMAKE_C_FLAGS_RELEASE = " ${CMAKE_C_FLAGS_RELEASE})
  # message(STATUS "CMAKE_C_FLAGS_RELWITHDEBINFO = " ${CMAKE_C_FLAGS_RELWITHDEBINFO})

  # message(STATUS "CMAKE_CXX_FLAGS = " ${CMAKE_CXX_FLAGS})
  # message(STATUS "CMAKE_CXX_FLAGS_DEBUG = " ${CMAKE_CXX_FLAGS_DEBUG})
  # message(STATUS "CMAKE_CXX_FLAGS_MINSIZEREL = " ${CMAKE_CXX_FLAGS_MINSIZEREL})
  # message(STATUS "CMAKE_CXX_FLAGS_RELEASE = " ${CMAKE_CXX_FLAGS_RELEASE})
  # message(STATUS "CMAKE_CXX_FLAGS_RELWITHDEBINFO = " ${CMAKE_CXX_FLAGS_RELWITHDEBINFO})

  # message(STATUS "CMAKE_EXE_LINKER_FLAGS = " ${CMAKE_EXE_LINKER_FLAGS})
  # message(STATUS "CMAKE_EXE_LINKER_FLAGS_DEBUG = " ${CMAKE_EXE_LINKER_FLAGS_DEBUG})
  # message(STATUS "CMAKE_EXE_LINKER_FLAGS_MINSIZEREL = " ${CMAKE_EXE_LINKER_FLAGS_MINSIZEREL})
  # message(STATUS "CMAKE_EXE_LINKER_FLAGS_RELEASE = " ${CMAKE_EXE_LINKER_FLAGS_RELEASE})
  # message(STATUS "CMAKE_EXE_LINKER_FLAGS_RELWITHDEBINFO = " ${CMAKE_EXE_LINKER_FLAGS_RELWITHDEBINFO})

  # message(STATUS "CMAKE_MODULE_LINKER_FLAGS = " ${CMAKE_MODULE_LINKER_FLAGS})
  # message(STATUS "CMAKE_MODULE_LINKER_FLAGS_DEBUG = " ${CMAKE_MODULE_LINKER_FLAGS_DEBUG})
  # message(STATUS "CMAKE_MODULE_LINKER_FLAGS_MINSIZEREL = " ${CMAKE_MODULE_LINKER_FLAGS_MINSIZEREL})
  # message(STATUS "CMAKE_MODULE_LINKER_FLAGS_RELEASE = " ${CMAKE_MODULE_LINKER_FLAGS_RELEASE})
  # message(STATUS "CMAKE_MODULE_LINKER_FLAGS_RELWITHDEBINFO = " ${CMAKE_MODULE_LINKER_FLAGS_RELWITHDEBINFO})

  # message(STATUS "CMAKE_SHARED_LINKER_FLAGS = " ${CMAKE_SHARED_LINKER_FLAGS})
  # message(STATUS "CMAKE_SHARED_LINKER_FLAGS_DEBUG = " ${CMAKE_SHARED_LINKER_FLAGS_DEBUG})
  # message(STATUS "CMAKE_SHARED_LINKER_FLAGS_MINSIZEREL = " ${CMAKE_SHARED_LINKER_FLAGS_MINSIZEREL})
  # message(STATUS "CMAKE_SHARED_LINKER_FLAGS_RELEASE = " ${CMAKE_SHARED_LINKER_FLAGS_RELEASE})
  # message(STATUS "CMAKE_SHARED_LINKER_FLAGS_RELWITHDEBINFO = " ${CMAKE_SHARED_LINKER_FLAGS_RELWITHDEBINFO})

  # message(STATUS "CMAKE_STATIC_LINKER_FLAGS = " ${CMAKE_STATIC_LINKER_FLAGS})
  # message(STATUS "CMAKE_STATIC_LINKER_FLAGS_DEBUG = " ${CMAKE_STATIC_LINKER_FLAGS_DEBUG})
  # message(STATUS "CMAKE_STATIC_LINKER_FLAGS_MINSIZEREL = " ${CMAKE_STATIC_LINKER_FLAGS_MINSIZEREL})
  # message(STATUS "CMAKE_STATIC_LINKER_FLAGS_RELEASE = " ${CMAKE_STATIC_LINKER_FLAGS_RELEASE})
  # message(STATUS "CMAKE_STATIC_LINKER_FLAGS_RELWITHDEBINFO = " ${CMAKE_STATIC_LINKER_FLAGS_RELWITHDEBINFO})
