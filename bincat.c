#include <stdio.h>

int main(int argc, char *argv[]) {
	int i;
	FILE *fp, *out;
	char buf[1024];
	size_t len;

	out = stdout;

	for (i = 1; i < argc; i++) {
		fp = fopen(argv[i], "rb");
		do {
			len = fread(buf, 1, sizeof buf, fp);
			if (fwrite(buf, 1, len, out) < len) break;
		} while (len == sizeof buf);

		if (ferror(fp)) {
			perror("fread");
			return 1;
		}

		if (ferror(out)) {
			perror("fwrite");
			return 1;
		}
	}

	return 0;
}
