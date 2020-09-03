#!/bin/sh

# Test zeromerge for correct behavior

ERR=0

echo -n "Testing 4K blocks: "
./zeromerge test/missing1.bin test/missing2.bin test_output.bin
if cmp -s test/correct.bin test_output.bin 
	then echo "PASSED"; ERR=0
	else echo "FAILED"; ERR=1
fi

echo -n "Testing 4K w/tail: "
./zeromerge test/missing1_short.bin test/missing2_short.bin test_output.bin
if cmp -s test/correct_short.bin test_output.bin
	then echo "PASSED"; ERR=0
	else echo "FAILED"; ERR=1
fi

exit $ERR
