#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define try(exp) if ((exp) < 0) return 1

/* Options */
int base = 16;

void parse_args(int argc, char *argv[]) {
	static const char *optstr = "hb:";
	int opt;
	while ((opt = getopt(argc, argv, optstr)) >= 0) {
		switch (opt) {
		case 'h':
			printf("Usage: %s [-b BASE] BYTE ... >OUTPUT_FILE\n", argv[0]);
			exit(0);

		case 'b':
			base = atoi(optarg);
			if (base <= 0) base = 16;
			break;
		}
	}
}

int process(FILE *out, int argc, char *argv[]) {
	int i;
	for (i = 0; i < argc; i++) {
		try(fputc(strtol(argv[i], NULL, base), out));
	}
	return 0;
}

int main(int argc, char *argv[]) {
	parse_args(argc, argv);
	return process(stdout, argc - optind, argv + optind);
}
