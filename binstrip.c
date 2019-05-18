/*
Copyright 2019 Samadi van Koten <samadi@vktec.org.uk>

Permission to use, copy, modify, and/or distribute this software for any purpose with or without fee is hereby granted, provided that the above copyright notice and this permission notice appear in all copies.

THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/

#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define try(exp) if ((exp) < 0) return 1

/* Options */
char strip_byte = '\0';

void parse_args(int argc, char *argv[]) {
	static const char *optstr = "hc:n:";
	int opt;
	while ((opt = getopt(argc, argv, optstr)) >= 0) {
		switch (opt) {
		case 'h':
			printf(
				"Usage: %s [OPTIONS] <INPUT_FILE >OUTPUT_FILE\n"
				"\n"
				"Options:\n"
				"  -c CHAR  Sets the byte to be stripped to the first byte of CHAR\n"
				"  -n NUM   Sets the byte to be stripped to the byte specified by the hexadecimal NUM (default: 00)\n",
				argv[0]);
			exit(0);

		case 'c':
			strip_byte = *optarg;
			break;

		case 'n':
			strip_byte = strtol(optarg, NULL, 16);
			if (strip_byte < 0) strip_byte = 0;
			break;
		}
	}
}

int process(FILE *in, FILE *out) {
	size_t count;
	int ch;

	while ((ch = getc(in)) != EOF) {
		if (ch == strip_byte) {
			/* Count the characters */
			count = 1;
			while ((ch = getc(in)) == strip_byte) count++;

			/* If the file has ended, we're done */
			if (ch == EOF) break;
			/* Otherwise, write the characters we just skipped */
			while (count--) try(putc(strip_byte, out));
		}

		/* Copy the character from in to out */
		try(putc(ch, out));
	}
	return 0;
}

int main(int argc, char *argv[]) {
	parse_args(argc, argv);
	return process(stdin, stdout);
}
