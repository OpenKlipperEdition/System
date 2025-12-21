#!/usr/bin/env python
import os,re,sys

def main():

	global ld_log
	global ld_lib_path
	global compiler_path
	global library_path
	global ld_elf_format
	global errorstr
	linesep = os.linesep
	pathsep = os.pathsep

	tmp_compiler_test = "compiler_test_%d.c" %(os.getpid())
	tmp_log = "log_%d.txt" % (os.getpid())

	cf = open(tmp_compiler_test,'w')
	st = "#include<stdio.h>"+ linesep + "int main(){}"
	cf.write(st)
	cf.close()

	cmd = "gcc -o %s.o %s -v > %s 2>&1" % (tmp_compiler_test, tmp_compiler_test, tmp_log)
	os.system(cmd)

	compiler_path_patten = re.compile(r'COMPILER_PATH')
	library_path_patten = re.compile(r'LIBRARY_PATH')
	ld_lib_patten = re.compile(r'ld-linux(.*).so.2')
	ld_elf_format_patten = re.compile(r'elf')
	#ld_lib_begin_path = re.compile(r'crtbegin(.).o')
	lib_filename = "crt1.o"
	ld_lib_begin_name = "crtbeginS.o"

	spilt_str = re.compile(r"=")
	pathsep_pat = re.compile(pathsep)
	space_pat = re.compile(r' ')


	with open(tmp_log,'r') as cl:
		for line in cl:
			if line.find("gcc: not found") > 0:
				errorstr = line
				return
			c_match = compiler_path_patten.match(line)
			if c_match:
				compiler_path_origin = c_match.string
			lib_match = library_path_patten.match(line)
			if lib_match:
				library_path_origin = lib_match.string
			ld_match = ld_lib_patten.search(line)
			if ld_match:
				ld_log = ld_match.string


	compiler_paths = pathsep_pat.split(spilt_str.split(compiler_path_origin)[1])
	library_paths = pathsep_pat.split(library_path_origin)

	for path in compiler_paths:
		rely_compiler_path = path + ld_lib_begin_name
		if(os.path.exists(rely_compiler_path)):
			compiler_path = os.path.abspath(path)
			break

	for lib_path in library_paths:
		file_path = lib_path + lib_filename
		if(os.path.exists(file_path)):
			library_path = os.path.abspath(lib_path)
			break

	ld_info = space_pat.split(ld_log)
	for ld in ld_info:
		match = ld_lib_patten.search(ld)
		if match:
			ld_lib_path = ld
			break
	ld_info = space_pat.split(ld_log)
	for ld in ld_info:
		match = ld_elf_format_patten.match(ld)
		if match:
			ld_elf_format = ld
			break

	cmd = "rm -rf %s %s.o %s" % (tmp_compiler_test, tmp_compiler_test, tmp_log)
	os.system(cmd)


if __name__ == '__main__':
	ld_log = "ld_log"
	ld_lib_path = "ld_lib_path"
	ld_elf_format = "elf"
	compiler_path = "compiler_path"
	compiler_path_origin = "compiler_path_origin"
	library_path = "library_path"
	errorstr = ""
	main()
	print(compiler_path)
	print(library_path)
	print(ld_lib_path)
	print(ld_elf_format)
	print(errorstr)
