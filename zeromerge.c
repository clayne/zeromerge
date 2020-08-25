/* Zero Merge - combine files with empty (zero) blocks
 * Copyright (C) 2020 by Jody Bruchon <jody@jodybruchon.com>
 * Distributed under The MIT License
 */

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>

/* Block size to scan for merging */
#define BSIZE 4096

#ifdef ON_WINDOWS
 #define FOPEN_R "rbS"
 #define FOPEN_W "wbS"
#else
 #define FOPEN_R "rb"
 #define FOPEN_W "wb"
#endif /* ON_WINDOWS */


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


int main(int argc, char **argv)
{
	static char buf1[BSIZE], buf2[BSIZE];
	static FILE *file1, *file2, *file3;

	if (argc != 3) goto error_argcount;

	file1 = fopen(argv[1], FOPEN_R);
	if (!file1) goto error_file1;
	file2 = fopen(argv[2], FOPEN_R);
	if (!file2) goto error_file2;
	file3 = fopen(argv[3], FOPEN_W);
	if (!file3) goto error_file3;

	fread(&buf1, BSIZE, 1, file1);
	fread(&buf2, BSIZE, 1, file2);

	switch (compare_blocks(buf1, buf2, BSIZE)) {
		default:
		case -1:
			goto error_different;
			break;
		case 0:
			break;
		case 1:
			break;
		case 2:
			break;
	}

	exit(EXIT_SUCCESS);

error_different:
	fprintf(stderr, "Error: files contain different non-zero data\n");
	exit(EXIT_FAILURE);
error_argcount:
	fprintf(stderr, "Usage: %s file1 file2 outfile\n", argv[0]);
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
}
