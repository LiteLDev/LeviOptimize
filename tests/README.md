# Optimization helper tests

These tests exercise the production state and instruction-matching helpers in `src/features` through their public interfaces. They do not require a BDS executable, LeviLamina, or xmake packages. The five suites contain 54 cases.

Build and run from the repository root with a C++23 compiler:

```sh
clang++ -std=c++23 -Wall -Wextra -Werror -pthread -Isrc tests/ChunkOptimizationStateTest.cpp -o ChunkOptimizationStateTest
./ChunkOptimizationStateTest
clang++ -std=c++23 -Wall -Wextra -Werror -Isrc tests/ChunkGenDispatcherSignatureTest.cpp -o ChunkGenDispatcherSignatureTest
./ChunkGenDispatcherSignatureTest
clang++ -std=c++23 -Wall -Wextra -Werror -Isrc tests/ChunkCacheSignaturesTest.cpp -o ChunkCacheSignaturesTest
./ChunkCacheSignaturesTest
clang++ -std=c++23 -Wall -Wextra -Werror -Isrc tests/VanillaWaypointsTest.cpp -o VanillaWaypointsTest
./VanillaWaypointsTest
clang++ -std=c++23 -Wall -Wextra -Werror -Isrc tests/EntityPacketRecipientsTest.cpp -o EntityPacketRecipientsTest
./EntityPacketRecipientsTest
```

On Windows, use a Visual Studio developer terminal so clang can locate the Windows SDK, MSVC standard library, and linker:

```powershell
clang++ -std=c++23 -Wall -Wextra -Werror -Isrc tests/ChunkOptimizationStateTest.cpp -o ChunkOptimizationStateTest.exe
./ChunkOptimizationStateTest.exe
clang++ -std=c++23 -Wall -Wextra -Werror -Isrc tests/ChunkGenDispatcherSignatureTest.cpp -o ChunkGenDispatcherSignatureTest.exe
./ChunkGenDispatcherSignatureTest.exe
clang++ -std=c++23 -Wall -Wextra -Werror -Isrc tests/ChunkCacheSignaturesTest.cpp -o ChunkCacheSignaturesTest.exe
./ChunkCacheSignaturesTest.exe
clang++ -std=c++23 -Wall -Wextra -Werror -Isrc tests/VanillaWaypointsTest.cpp -o VanillaWaypointsTest.exe
./VanillaWaypointsTest.exe
clang++ -std=c++23 -Wall -Wextra -Werror -Isrc tests/EntityPacketRecipientsTest.cpp -o EntityPacketRecipientsTest.exe
./EntityPacketRecipientsTest.exe
```

The executable reports every case and returns a nonzero exit status on failure. Checks remain active when `NDEBUG` is defined.

Coverage includes deferred update deduplication, nested calls, context restoration before callbacks, exception unwinding, thread isolation, exact refresh intervals, empty maps, out-of-range indices, map growth and shrinkage, stale key replacement, chunk payload ownership, and selected keys remaining valid after snapshot refresh and destruction. Key enumeration is observed through a map wrapper to check rebuild frequency without inspecting private cache state.

The dispatcher and cache signature suites test the production instruction matchers: displacement wildcard byte values, corruption of each fixed byte, every incorrect expected byte, truncated buffers and tails, function boundaries, capacity values, already-patched sequences, and rejection of multiple matches. These tests validate matching, not writes to live BDS code or Windows unwind metadata.

The waypoint suite covers self exclusion, tracked and expired players, duplicate and boundary IDs, joins/leaves between calls, bulk membership and live-lookup short circuiting. The recipient suite covers invalid IDs, relevant players, missing user identifiers, subclient preservation, discarded partial selections and per-send isolation. These are production selection helpers; native group creation, locator toggles, dimension filtering, the SDK/native actor hash and actual packet delivery still require runtime checks.

BDS hook resolution, ABI compatibility, chunk save/load behavior, redstone behavior, and gameplay regressions still require server/client integration tests on the supported game version.
