# tcllib7zip

tcllib7zip is a tcl binding to 7z.dll/7z.so from 7zip.org.

tcllib7zip provides readonly (list/extract) access to a wide range of archive files, currently supported:
apfs img apm ar a deb udeb lib arj b64 bz2 bzip2 tbz2 tbz msi msp doc xls ppt cpio cramfs dmg elf ext ext2 ext3 ext4 img fat img flv gz gzip tgz tpz apk gpt mbr hfs hfsx ihex lpimg img lzh lha lzma lzma86 macho mbr mslz mub ntfs img exe dll sys obj te pmd qcow qcow2 qcow2c rpm simg img 001 squashfs swf swf scap uefif vdi vhd vhdx avhdx vmdk xar pkg xip xz txz z taz zst tzst 7z cab chm chi chq chw hxs hxi hxr hxq hxw lit iso img nsis rar r00 rar r00 tar ova udf iso img wim swm esd ppkg zip z01 zipx jar xpi odt ods docx xlsx epub ipa apk appx 

dependencies:

	7zip:
		https://github.com/ip7z/7zip

	lib7zip:
		https://github.com/ooa64/lib7zip fork of
		https://github.com/celyrin/lib7zip fork of
		https://github.com/stonewell/lib7zip

installation:

	git clone -b dev https://github.com/ooa64/lib7zip
	cd lib7zip
	git submodule update --init --single-branch
	mkdir build
	cd build
	cmake .. -DBUILD_SHARED_LIB=OFF
	make
	cd ../../

	git clone https://github.com/ooa64/tcllib7zip
	cd tcllib7zip
	./configure
	make
	make 7z.so
	make test
	sudo make install	

usage:

	$ tclsh
	% package require sevenzip
	0.2
	% set cmd [sevenzip open tests/files/test.zip]
	sevenzip0
	% $cmd list
	test.txt
	% $cmd extract -c test.txt stdout
	test
