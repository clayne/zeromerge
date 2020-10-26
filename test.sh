#!/bin/sh

# Test zeromerge for correct behavior

ERR=0

PROG="./zeromerge"

! $PROG -v && echo "Compile the program first" && exit 1

echo -n "Testing 4K blocks: "
$PROG test/missing1.bin test/missing2.bin test_output.bin >/dev/null
if cmp -s test/correct.bin test_output.bin 
	then echo "PASSED"; ERR=0
	else echo "FAILED"; ERR=1
fi

echo -n "Testing 4K + tail: "
$PROG test/missing1_short.bin test/missing2_short.bin test_output.bin >/dev/null
if cmp -s test/correct_short.bin test_output.bin
	then echo "PASSED"; ERR=0
	else echo "FAILED"; ERR=1
fi

echo -n "Testing mismatch1: "
if ! $PROG test/missing1.bin test/mismatch.bin test_output.bin 2>/dev/null >/dev/null
	then echo "PASSED"; ERR=0
	else echo "FAILED"; ERR=1
fi

echo -n "Testing mismatch2: "
if ! $PROG test/missing1_short.bin test/mismatch_short.bin test_output.bin >/dev/null 2>/dev/null
	then echo "PASSED"; ERR=0
	else echo "FAILED"; ERR=1
fi

echo -n "Testing UnicodeNm: "
$PROG test/ちゅくちゅく１.bin test/ちゅくちゅく２.bin test_output.bin >/dev/null
if cmp -s test/万斉.bin test_output.bin 
	then echo "PASSED"; ERR=0
	else echo "FAILED"; ERR=1
fi

exit $ERR
exit $ERR
