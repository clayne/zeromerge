/* Zero Merge - combine files with empty (zero) blocks
 * Copyright (C) 2020 by Jody Bruchon <jody@jodybruchon.com>
 * Distributed under The MIT License
 */

#include <fcntl.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <time.h>
#include <sys/time.h>

#include "jody_win_unicode.h"
#include "oom.h"
#include "version.h"

/* File read size */
#define READSIZE 4194304

/* Detect Windows and modify as needed */
#if defined _WIN32 || defined __CYGWIN__
 #ifndef ON_WINDOWS
  #define ON_WINDOWS 1
 #endif
 #define NO_SYMLINKS 1
 #define NO_PERMS 1
 #define NO_SIGACTION 1
 #ifndef WIN32_LEAN_AND_MEAN
  #define WIN32_LEAN_AND_MEAN
 #endif
 #include <windows.h>
 #include <io.h>
 #include "win_stat.h"
 #define STAT win_stat
// const char dir_sep = '\\';
 #ifdef UNICODE
  const wchar_t *FOPEN_R = L"rbS";
  const wchar_t *FOPEN_W = L"wbS";
 #else
  const char *FOPEN_R = "rbS";
  const char *FOPEN_W = "wbS";
 #endif /* UNICODE */

#else /* Not Windows */
 #include <sys/stat.h>
 #define STAT stat
 const char *FOPEN_R = "rb";
 const char *FOPEN_W = "wb";
// const char dir_sep = '/';
 #ifdef UNICODE
  #error Do not define UNICODE on non-Windows platforms.
  #undef UNICODE
 #endif
#endif /* _WIN32 || __CYGWIN__ */

/* Windows + Unicode compilation */
#ifdef UNICODE
 int out_mode = _O_TEXT;
 int err_mode = _O_TEXT;
#endif /* UNICODE */

static char program_name[PATH_MAX + 4];
static FILE *fp1, *fp2, *fp3;


void clean_exit(void)
{
	if (fp1) fclose(fp1);
	if (fp2) fclose(fp2);
	if (fp3) fclose(fp3);
	return;
}


static void version(void)
{
	printf("zeromerge %s (%s) by Jody Bruchon <jody@jodybruchon.com>\n", VER, VERDATE);
	printf("\nLatest versions and support:\n");
	printf("        https://github.com/jbruchon/zeromerge\n");
	printf("\nPlease consider supporting continued development:\n");
	printf("        https://www.SubscribeStar.com/JodyBruchon\n\n");
	return;
}


static void usage(void)
{
	printf("Usage: %s file1 file2 outfile\n", program_name);
	return;
}


#ifdef UNICODE
int wmain(int argc, wchar_t **wargv)
#else
int main(int argc, char **argv)
#endif
{
	static char buf1[READSIZE];
	static char buf2[READSIZE];
	static char *file1, *file2, *file3;
#ifdef ON_WINDOWS
	struct winstat stat1, stat2;
 #ifdef UNICODE
	/* For Unicode conversions (_wfopen) */
	static wchar_t wname[WPATH_MAX];
 #endif
#else
	struct stat stat1, stat2;
#endif
	off_t remain;
	off_t read1, read2, write;
	off_t progress, lastprogress = 0;
	struct timeval time1, time2;
	int stdout_tty = 0;
	int hide_progress = 0;
	/* Progress is show in MiB by default */
	int prog_shift = 20;
	int prog_units = 'M';

	atexit(clean_exit);

#ifdef ON_WINDOWS
	/* Windows buffers our std output; don't let it do that */
	if (setvbuf(stdout, NULL, _IONBF, 0) != 0 || setvbuf(stderr, NULL, _IONBF, 0) != 0)
		fprintf(stderr, "warning: setvbuf() failed\n");
	if (_isatty(_fileno(stdout))) stdout_tty = 1;
#else
	if (isatty(fileno(stdout))) stdout_tty = 1;
#endif /* ON_WINDOWS */

#ifdef UNICODE
	/* Create a UTF-8 **argv from the wide version */
	static char **argv;
	argv = (char **)malloc(sizeof(char *) * (size_t)argc);
	if (!argv) oom("main() unicode argv");
	widearg_to_argv(argc, wargv, argv);
	/* fix up __argv so getopt etc. don't crash */
	__argv = argv;
	/* Only use UTF-16 for terminal output, else use UTF-8 */
	if (!_isatty(_fileno(stdout))) out_mode = _O_BINARY;
	else out_mode = _O_U16TEXT;
	if (!_isatty(_fileno(stderr))) err_mode = _O_BINARY;
	else err_mode = _O_U16TEXT;
#endif /* UNICODE */

	strncpy(program_name, argv[0], PATH_MAX);

	/* Help text if requested */
	if (argc >= 2) {
		if ((!strcmp(argv[1], "-h") || !strcmp(argv[1], "--help"))) {
			usage();
			exit(EXIT_SUCCESS);
		}
		if ((!strcmp(argv[1], "-v") || !strcmp(argv[1], "-V") || !strcmp(argv[1], "--version"))) {
			version();
			exit(EXIT_SUCCESS);
		}
	}

	if (argc != 4) goto error_argcount;

	/* Set up file names to open */
	file1 = argv[1]; file2 = argv[2]; file3 = argv[3];

	/* Open files to merge */
#ifdef UNICODE
	M2W(file1, wname);
	fp1 = _wfopen(wname, FOPEN_R);
	M2W(file2, wname);
	fp2 = _wfopen(wname, FOPEN_R);
#else
	fp1 = fopen(file1, FOPEN_R);
	fp2 = fopen(file2, FOPEN_R);
#endif /* UNICODE */
	if (!fp1) goto error_file1;
	if (!fp2) goto error_file2;

	/* File sizes must match; sizes also needed for loop */
	if (STAT(argv[1], &stat1) != 0) goto error_file1;
	if (STAT(argv[2], &stat2) != 0) goto error_file2;
//	fprintf(stderr, "DEBUG: size %lld,%lld dev %d,%d ino %lld,%lld\n", stat1.st_size, stat2.st_size, stat1.st_dev, stat2.st_dev, stat1.st_ino, stat2.st_ino);
	/* Hard link protection check */
	if (stat1.st_ino == stat2.st_ino && stat1.st_dev == stat2.st_dev) goto error_same_file;
	if (stat1.st_size != stat2.st_size) goto error_file_sizes;
	if (stat1.st_size == 0) goto error_empty_file;
	remain = stat1.st_size;

	/* Set units for progress indicator */
	if (stat1.st_size < 1048576) {
		prog_shift = 10;
		prog_units = 'K';
	}

	/* If read and size check are OK, open file to write into */
#ifdef UNICODE
	M2W(file3, wname);
	fp3 = _wfopen(wname, FOPEN_W);
#else
	fp3 = fopen(file3, FOPEN_W);
#endif /* UNICODE */
	if (!fp3) goto error_file3;

	/* Set up progress indicator */
	time2.tv_sec = 0;

	/* Main loop */
	while (remain > 0) {
		read1 = (off_t)fread(&buf1, 1, READSIZE, fp1);
		read2 = (off_t)fread(&buf2, 1, READSIZE, fp2);
		if ((read1 != read2)) goto error_short_read;
		if (ferror(fp1)) goto error_file1;
		if (ferror(fp2)) goto error_file2;

		/* Merge data between buffers */
		for (read1 = 0; read1 < read2; read1++) {
			/* Error if both bytes are non-zero and different */
			if (buf1[read1] != buf2[read1] && buf1[read1] != 0 && buf2[read1] != 0)
				goto error_different;
			/* merge data into buf1 */
			buf1[read1] |= buf2[read1];
		}
		write = (off_t)fwrite(&buf1, 1, (size_t)read2, fp3);
		if (write != read2) goto error_short_write;
		remain -= read2;

		/* Progress indicator - do not show if stdout not a TTY */
		if (hide_progress == 0) {
			gettimeofday(&time1, NULL);
			if (time2.tv_sec < time1.tv_sec) {
				progress = stat1.st_size - remain;
				if (stdout_tty == 1) printf("\r");
				printf("[zeromerge] Progress: %" PRIdMAX "%%, %" PRIdMAX " of %" PRIdMAX " %ciB (%" PRIdMAX " %ciB/sec)",
						(intmax_t)((progress * 100) / stat1.st_size),
						(intmax_t)progress >> prog_shift,
						(intmax_t)stat1.st_size >> prog_shift,
						prog_units,
						(intmax_t)(progress - lastprogress) >> prog_shift,
						prog_units);
				if (stdout_tty == 0) printf("\n");
				fflush(stdout);
				time2.tv_sec = time1.tv_sec;
				lastprogress = progress;
			}
		}
		if (feof(fp1) && feof(fp2)) break;
	}

	if (hide_progress == 0) {
		if (stdout_tty == 1) printf("\r");
		printf("[zeromerge] merge complete.");
		if (stdout_tty == 0) printf("\n");
		else printf("                              \n");
	}
	exit(EXIT_SUCCESS);

error_same_file:
	fprintf(stderr, "\nError: the input files appear to be hard linked\n");
	exit(EXIT_FAILURE);
error_different:
	fprintf(stderr, "\nError: files contain different non-zero data\n");
	exit(EXIT_FAILURE);
error_argcount:
	usage();
	exit(EXIT_FAILURE);
error_file1:
	fprintf(stderr, "\nError opening/reading "); fwprint(stderr, file1, 1);
	exit(EXIT_FAILURE);
error_file2:
	fprintf(stderr, "\nError opening/reading "); fwprint(stderr, file2, 1);
	exit(EXIT_FAILURE);
error_file3:
	fprintf(stderr, "\nError opening/reading "); fwprint(stderr, file3, 1);
	exit(EXIT_FAILURE);
error_file_sizes:
	fprintf(stderr, "\nError: file sizes are not identical\n");
	exit(EXIT_FAILURE);
error_short_read:
	fprintf(stderr, "\nError: short read: file 1 %s, file2 %s\n",
			strerror(ferror(fp1)), strerror(ferror(fp2)));
	exit(EXIT_FAILURE);
error_short_write:
	fprintf(stderr, "\nError: short write (%ld != %ld or %ld): %s\n",
			(long)write, (long)read1, (long)read2, strerror(ferror(fp3)));
	exit(EXIT_FAILURE);
error_empty_file:
	fprintf(stderr, "\nError: cannot merge files that are 0 bytes in size\n");
	exit(EXIT_FAILURE);
}
