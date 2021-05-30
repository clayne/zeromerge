## zeromerge

### Combine file with zero blocks into one unified file

Author: Jody Bruchon <jody@jodybruchon.com>

License: The MIT License _(see LICENSE file for details)_

Copyright (C) 2020 Jody Bruchon and contributors

This program merges two files that are supposed to be identical but have
missing data in one or both files that manifests as zeroed blocks. This
is commonly seen when the same file exists in multiple torrents but each
individual torrent is not fully seeded. For example, if a torrent stops at
99.9% completion but the last block is not available, but another torrent
is available with that same block fully available, the two partial file
downloads can be merged with this tool and force-rechecked to fill in the
data gap and recover the full file.

The program will abort if the file sizes differ or if any of the file data
that mismatches is not all zeroes.

### Please consider financially supporting continued development!

One-time or recurring donations are greatly appreciated, but not required.
Use PayPal, Flattr, Ko-Fi, SubscribeStar, etc. links at jodybruchon.com
