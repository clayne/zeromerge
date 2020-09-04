/* Zero Merge - combine files with empty (zero) blocks
 * Copyright (C) 2020 by Jody Bruchon <jody@jodybruchon.com>
 * Distributed under The MIT License
 */

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <limits.h>
#include "version.h"

/* Block size to scan for merging */
#define BSIZE 4096

#ifdef ON_WINDOWS
 #define FOPEN_R "rbS"
 #define FOPEN_W "wbS"
#else
 #define FOPEN_R "rb"
 #define FOPEN_W "wb"
#endif /* ON_WINDOWS */

char program_name[PATH_MAX + 4];

/* Compare blocks and check for zero blocks
 *  0 = both blocks are identical
 *  1 = buf1 is zero and buf2 is non-zero
 *  2 = buf1 is non-zero and buf2 is zero
 * -1 = blocks are non-zero and do not match
 * ^^^ This should be treated as a Bad Thing(tm) */
int compare_blocks(const char * const restrict buf1, const char * const restrict buf2, int size)
{
	static int zero1, zero2;

	if (!buf1 || !buf2 || size == 0) goto error_call;

	zero1 = 0; zero2 = 0;
	for (int len = 0; len < size; len++) {
		char c1, c2;
		c1 = *(buf1 + len);
		c2 = *(buf2 + len);
		/* Mark block non-zero if needed */
		if (c1 != '\0') zero1 = 1;
		if (c2 != '\0') zero2 = 1;
		/* Make sure the blocks are identical */
		if (c1 == c2) continue;
		/* Do both blocks contain different non-zero data? */
		if (zero1 & zero2) return -1;
		/* Blocks are different but one is zero so far - loop */
		continue;
	}
	/* Loop completed; check what blocks are zero or not */
	if (zero1 == 0 && zero2 == 1) return 1;
	if (zero1 == 1 && zero2 == 0) return 2;
	return 0;
error_call:
	fprintf(stderr, "compare_blocks(): bad parameter passed, aborting\n");
	exit(EXIT_FAILURE);
}


static void version(void)
{
	printf("zeromerge %s (%s) by Jody Bruchon <jody@jodybruchon.com>\n", VER, VERDATE);
	printf("Latest versions and support: https://github.com/jbruchon/zeromerge\n");
	return;
}


static void usage(void)
{
	printf("Usage: %s file1 file2 outfile\n", program_name);
	return;
}


int main(int argc, char **argv)
{
	static char buf1[BSIZE], buf2[BSIZE];
	static FILE *file1, *file2, *file3;
	static struct stat stat1, stat2;
	static off_t remain;
	static off_t read1, read2, write;

	strncpy(program_name, argv[0], PATH_MAX);

	/* Help text if requested */
	if (argc >= 2) {
	       if ((!strcmp(argv[1], "-h") || !strcmp(argv[1], "--help"))) {
			version();
			usage();
			exit(EXIT_SUCCESS);
		}
	       if ((!strcmp(argv[1], "-v") || !strcmp(argv[1], "-V") || !strcmp(argv[1], "--version"))) {
			version();
			exit(EXIT_SUCCESS);
		}
	}

	if (argc != 4) goto error_argcount;

	/* Open files to merge */
	file1 = fopen(argv[1], FOPEN_R);
	if (!file1) goto error_file1;
	file2 = fopen(argv[2], FOPEN_R);
	if (!file2) goto error_file2;

	/* File sizes must match; sizes also needed for loop */
	if (stat(argv[1], &stat1) != 0) goto error_file1;
	if (stat(argv[2], &stat2) != 0) goto error_file2;
	if (stat1.st_size != stat2.st_size) goto error_file_sizes;
	remain = stat1.st_size;

	/* If read and size check are OK, open file to write into */
	file3 = fopen(argv[3], FOPEN_W);
	if (!file3) goto error_file3;

	/* Main loop */
	while (remain > 0) {

		read1 = (off_t)fread(&buf1, 1, BSIZE, file1);
		read2 = (off_t)fread(&buf2, 1, BSIZE, file2);
		if ((read1 != read2)) goto error_short_read;
		if (ferror(file1)) goto error_file1;
		if (ferror(file2)) goto error_file2;
	
		switch (compare_blocks(buf1, buf2, (int)read1)) {
			default:
			case -1:  /* -1 = blocks are non-zero and do not match */
				goto error_different;
				break;
			case 0:  /*  0 = both blocks are identical */
			case 2:  /*  2 = buf1 is non-zero and buf2 is zero */
				write = (off_t)fwrite(&buf1, 1, (size_t)read1, file3);
				if (write != read1) goto error_short_write;
				break;
			case 1:  /*  1 = buf1 is zero and buf2 is non-zero */
				write = (off_t)fwrite(&buf2, 1, (size_t)read2, file3);
				if (write != read2) goto error_short_write;
				break;
		}
		remain -= read1;
		if (feof(file1) && feof(file2)) break;
	}
	exit(EXIT_SUCCESS);

error_different:
	fprintf(stderr, "Error: files contain different non-zero data\n");
	exit(EXIT_FAILURE);
error_argcount:
	usage();
	exit(EXIT_FAILURE);
error_file1:
	fprintf(stderr, "Error opening/reading '%s'\n", argv[1]);
	exit(EXIT_FAILURE);
error_file2:
	fprintf(stderr, "Error opening/reading '%s'\n", argv[2]);
	exit(EXIT_FAILURE);
error_file3:
	fprintf(stderr, "Error opening/writing '%s'\n", argv[3]);
	exit(EXIT_FAILURE);
error_file_sizes:
	fprintf(stderr, "Error: file sizes are not identical\n");
	exit(EXIT_FAILURE);
error_short_read:
	fprintf(stderr, "Error: short read\n");
	exit(EXIT_FAILURE);
error_short_write:
	fprintf(stderr, "Error: short write (%ld != %ld or %ld)\n", (long)write, (long)read1, (long)read2);
	exit(EXIT_FAILURE);
}
