/*
 * This is free and unencumbered software released into the public domain.
 *
 * Anyone is free to copy, modify, publish, use, compile, sell, or
 * distribute this software, either in source code form or as a compiled
 * binary, for any purpose, commercial or non-commercial, and by any
 * means.
 *
 * In jurisdictions that recognize copyright laws, the author or authors
 * of this software dedicate any and all copyright interest in the
 * software to the public domain. We make this dedication for the benefit
 * of the public at large and to the detriment of our heirs and
 * successors. We intend this dedication to be an overt act of
 * relinquishment in perpetuity of all present and future rights to this
 * software under copyright law.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 * For more information, please refer to <http://unlicense.org/>
 */

#define _POSIX_C_SOURCE 200809L
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define _usage(fmt, ...) fprintf(stderr, "Usage: datobj FILE SYMBOL [FILE SYMBOL ...] >OBJECT\n\n" fmt "%c", __VA_ARGS__)
#define usage(...) (_usage(__VA_ARGS__, '\n'), 1)

#define _error(fmt, ...) fprintf(stderr, fmt ": %s\n", __VA_ARGS__)
#define error(...) exit((_error(__VA_ARGS__, strerror(errno)), 1))

#define elf_byte(i) ((i) & 0xFF)
#define elf_half(i)	elf_byte(i), elf_byte((i)>>8)
#define elf_word(i) elf_half(i), elf_half((i)>>16)
#define elf_xword(i) elf_word(i), elf_word((i)>>32)

// align must be power of 2
#define alignup(x, align) (-(-(x) & -(align)))

int main(int argc, char *argv[]) {
	// Validate args
	if (argc < 2) return usage("Not enough arguments");

	if (argc % 2 == 0) return usage("Missing symbol name for file: %s", argv[argc-1]);

	// Ensure not writing to tty
	if (isatty(STDOUT_FILENO)) return usage("stdout is a TTY, should be redirected to a file");

	size_t max_pad = 0;

	int nfiles = argc/2;
	struct {const char *sym, *data; size_t symlen, len, pad;} files[nfiles];
	size_t rodata_len = 0, strtab_len = 1;
 	for (int i = 1; i < argc; i += 2) {
		files[i/2].sym = argv[i+1];
		files[i/2].symlen = strlen(files[i/2].sym);
		strtab_len += 2*files[i/2].symlen + strlen("_end") + 2;

		int fd = open(argv[i], O_RDONLY);
		if (fd < 0) error("Failed to open file: %s", argv[i]);

		struct stat st;
		if (fstat(fd, &st)) error("Failed to stat file: %s", argv[i]);
		files[i/2].len = st.st_size;
		size_t aligned_len = alignup(st.st_size, 8);
		files[i/2].pad = aligned_len - files[i/2].len;
		if (files[i/2].pad > max_pad) max_pad = files[i/2].pad;
		rodata_len += aligned_len;

		files[i/2].data = mmap(NULL, files[i/2].len, PROT_READ, MAP_PRIVATE, fd, 0);
		if (!files[i/2].data) error("Failed to map file: %s", argv[i]);

		close(fd);
	}

	size_t header_len = 64 + 5*64; // file header + 5 section headers
#define SHSTRTAB "\0.rodata\0.shstrtab\0.symtab\0.strtab"
	size_t shstrtab_len = sizeof SHSTRTAB;
	size_t symtab_len = 24 * (2*nfiles + 1);
	size_t rodata_off_unaligned = header_len + shstrtab_len + symtab_len + strtab_len;
	size_t rodata_off = alignup(rodata_off_unaligned, 8);
	size_t rodata_pad = rodata_off - rodata_off_unaligned;
	if (rodata_pad > max_pad) max_pad = rodata_pad;

	// ELF header
	const unsigned char header[] = {
		// 0x0000
		// file header, 64 bytes {{{
		// ident, 16 bytes {{{
		0x7f, 'E', 'L', 'F',
		2, // CLASS: 64bit
		1, // DATA: LSB
		1, // VERSION: CURRENT
		0, // OSABI: SYSV
		0, // ABIVERSION: 0

		0,0,0,0,0,0,0, // padding
		// }}}

		1,0, // TYPE: REL
		62,0, // MACHINE: X86_64
		1,0,0,0, // VERSION: CURRENT
		0,0,0,0, 0,0,0,0, // ENTRY: n/a
		0,0,0,0, 0,0,0,0, // PHOFF: n/a
		64,0,0,0, 0,0,0,0, // SHOFF: 64 bytes
		0,0,0,0, // FLAGS: 0
		64, 0, // EHSIZE: 48 bytes
		0,0, // PHENTSIZE: n/a
		0,0, // PHNUM: n/a
		64,0, // SHENTSIZE: 64 bytes
		5,0, // SHNUM: 5 sections
		2,0, // SHSTRNDX: 1st section
		// }}}

		// 0x0040
		// reserved section header, 64 bytes {{{
		0,0,0,0, // NAME
		0,0,0,0, // TYPE
		0,0,0,0, 0,0,0,0, // FLAGS
		0,0,0,0, 0,0,0,0, // ADDR
		0,0,0,0, 0,0,0,0, // OFFSET
		0,0,0,0, 0,0,0,0, // SIZE
		0,0,0,0, // LINK
		0,0,0,0, // INFO
		0,0,0,0, 0,0,0,0, // ADDRALIGN
		0,0,0,0, 0,0,0,0, // ENTSIZE
		// }}}

		// 0x0080
		// read-only data section header, 64 bytes {{{
		1,0,0,0, // NAME: ".rodata"
		1,0,0,0, // TYPE: PROGBITS
		2,0,0,0, 0,0,0,0, // FLAGS: ALLOC
		0,0,0,0, 0,0,0,0, // ADDR: n/a
		elf_xword(rodata_off), // OFFSET: header size + shstrtab size + symtab size + strtab size
		elf_xword(rodata_len), // SIZE: computed from lengths of input files
		0,0,0,0, // LINK: n/a
		0,0,0,0, // INFO: n/a
		8,0,0,0, 0,0,0,0, // ADDRALIGN: 8 bytes (in case the data is not simple bytes)
		0,0,0,0, 0,0,0,0, // ENTSIZE: n/a
		// }}}

		// 0x00c0
		// section name table section header, 64 bytes {{{
		elf_word(sizeof ".rodata"), // NAME: ".shstrtab"
		3,0,0,0, // TYPE: STRTAB
		0,0,0,0, 0,0,0,0, // FLAGS: n/a
		0,0,0,0, 0,0,0,0, // ADDR: n/a
		elf_xword(header_len + symtab_len), // OFFSET: header size + symtab size
		elf_xword(shstrtab_len), // SIZE: 1 byte
		0,0,0,0, // LINK: n/a
		0,0,0,0, // INFO: n/a
		1,0,0,0, 0,0,0,0, // ADDRALIGN: 1 byte
		0,0,0,0, 0,0,0,0, // ENTSIZE: n/a
		// }}}

		// 0x0100
		// symbol table section header, 64 bytes {{{
		elf_word(sizeof ".rodata\0.shstrtab"), // NAME: ".symtab"
		2,0,0,0, // TYPE: SYMTAB
		0,0,0,0, 0,0,0,0, // FLAGS: none
		0,0,0,0, 0,0,0,0, // ADDR: n/a
		elf_xword(header_len), // OFFSET: header size
		elf_xword(symtab_len), // SIZE: 24 bytes per symbol
		4,0,0,0, // LINK: 4th section (symbol string table)
		1,0,0,0, // INFO: 0 local symbols
		8,0,0,0, 0,0,0,0, // ADDRALIGN: 8 bytes
		24,0,0,0, 0,0,0,0, // ENTSIZE: 24 bytes
		// }}}

		// 0x0140
		// symbol string table section header, 64 bytes {{{
		elf_word(sizeof ".rodata\0.shstrtab\0.symtab"), // NAME: ".strtab"
		3,0,0,0, // TYPE: STRTAB
		0,0,0,0, 0,0,0,0, // FLAGS: none
		0,0,0,0, 0,0,0,0, // ADDR: n/a
		elf_xword(header_len + symtab_len + shstrtab_len), // OFFSET: header size + symtab size + shstrtab size
		elf_xword(strtab_len), // SIZE: computed from lengths of input symbols
		0,0,0,0, // LINK: n/a
		0,0,0,0, // INFO: n/a
		1,0,0,0, 0,0,0,0, // ADDRALIGN: 1 byte
		0,0,0,0, 0,0,0,0, // ENTSIZE: n/a
		// }}}
	};
	fwrite(header, 1, header_len, stdout);

	// 0x0180
	// Symbol table
	const unsigned char symtab_prelude[] = {
		// Reserved
		0,0,0,0, // NAME
		0, // INFO
		0, // OTHER
		0,0, // SHNDX
		0,0,0,0, 0,0,0,0, // VALUE
		0,0,0,0, 0,0,0,0, // SIZE

		/*// rodata section symbol
		0,0,0,0, // NAME: none
		3, // INFO: local, section
		0, // OTHER
		1,0, // SHNDX: rodata section
		0,0,0,0, 0,0,0,0, // VALUE: n/a
		0,0,0,0, 0,0,0,0, // SIZE: n/a*/
	};
	fwrite(symtab_prelude, 1, sizeof symtab_prelude, stdout);

	size_t dat_offset = 0;
	size_t sym_offset = 1;
 	for (int i = 0; i < nfiles; i++) {
		for (int j = 0; j < 2; j++) {
			const unsigned char entry[] = {
				elf_word(sym_offset), // NAME: specified by symbol index
				0x10, // INFO: GLOBAL, NOTYPE
				0, // OTHER
				1,0, // SHNDX: rodata section
				elf_xword(dat_offset), // VALUE: file's offset into rodata
				0,0,0,0, 0,0,0,0, // SIZE: n/a
			};
			fwrite(entry, 1, sizeof entry, stdout);
			sym_offset += files[i].symlen+1;
			if (!j) dat_offset += files[i].len;
		}
		sym_offset += strlen("_end");
		dat_offset += files[i].pad;
	}

	// Section name table
	fwrite(SHSTRTAB, 1, shstrtab_len, stdout);

	// Symbol string table
	putchar(0);
 	for (int i = 0; i < nfiles; i++) {
		fwrite(files[i].sym, 1, files[i].symlen+1, stdout);
		fwrite(files[i].sym, 1, files[i].symlen, stdout);
		fwrite("_end", 1, sizeof "_end", stdout);
	}

	// Data
	char pad[max_pad];
	if (rodata_pad) fwrite(pad, 1, rodata_pad, stdout);

	for (int i = 0; i < nfiles; i++) {
		fwrite(files[i].data, 1, files[i].len, stdout);
		fwrite(pad, 1, files[i].pad, stdout);
	}

	return 0;
 }
