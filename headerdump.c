#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define try(exp) if ((exp) < 0) return 1

/* Options */
const char *varname = "data";
size_t wrap = 16;

void parse_args(int argc, char *argv[]) {
	static const char *optstr = "hsv:b:";
	int opt;
	while ((opt = getopt(argc, argv, optstr)) >= 0) {
		switch (opt) {
		case 'h':
			printf("Usage: %s [-v VARNAME] [-b WRAP] <INPUT_FILE >OUTPUT_FILE\n", argv[0]);
			exit(0);

		case 'v':
			varname = optarg;
			break;

		case 'b':
			wrap = atoll(optarg);
			break;
		}
	}
}

int process(FILE *in, FILE *out) {
	size_t count;
	int ch;

	/* Write the header */
	try(fprintf(out, "#ifndef %s_H\n#define %s_H\n\nunsigned char %s[] = {\n\t", varname, varname, varname));

	/* Write the file contents as hex */
	while ((ch = getc(in)) != EOF) {
		if (wrap && count && count % wrap == 0) {
			try(fputs("\n\t", out));
		}

		count++;
		try(fprintf(out, "0x%02x, ", ch));
	}

	/* Write the trailer */
	try(fprintf(out, "\n};\n\n#define %s_len (sizeof %s)\n\n#endif\n", varname, varname));

	return 0;
}

int main(int argc, char *argv[]) {
	parse_args(argc, argv);
	return process(stdin, stdout);
}