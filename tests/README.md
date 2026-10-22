# Chunk optimization state tests

These tests exercise the production helpers in `src/features/ChunkOptimizationState.h` through their public interfaces. They do not require a BDS executable, LeviLamina, or xmake packages.

Build and run from the repository root with a C++20 compiler:

```sh
clang++ -std=c++20 -Wall -Wextra -Werror -pthread -Isrc tests/ChunkOptimizationStateTest.cpp -o ChunkOptimizationStateTest
./ChunkOptimizationStateTest
clang++ -std=c++20 -Wall -Wextra -Werror -Isrc tests/ChunkGenDispatcherSignatureTest.cpp -o ChunkGenDispatcherSignatureTest
./ChunkGenDispatcherSignatureTest
clang++ -std=c++20 -Wall -Wextra -Werror -Isrc tests/ChunkCacheSignaturesTest.cpp -o ChunkCacheSignaturesTest
./ChunkCacheSignaturesTest
```

On Windows, use a Visual Studio developer terminal so clang can locate the Windows SDK, MSVC standard library, and linker:

```powershell
clang++ -std=c++20 -Wall -Wextra -Werror -Isrc tests/ChunkOptimizationStateTest.cpp -o ChunkOptimizationStateTest.exe
./ChunkOptimizationStateTest.exe
clang++ -std=c++20 -Wall -Wextra -Werror -Isrc tests/ChunkGenDispatcherSignatureTest.cpp -o ChunkGenDispatcherSignatureTest.exe
./ChunkGenDispatcherSignatureTest.exe
clang++ -std=c++20 -Wall -Wextra -Werror -Isrc tests/ChunkCacheSignaturesTest.cpp -o ChunkCacheSignaturesTest.exe
./ChunkCacheSignaturesTest.exe
```

The executable reports every case and returns a nonzero exit status on failure. Checks remain active when `NDEBUG` is defined.

Coverage includes deferred update deduplication, nested calls, context restoration before callbacks, exception unwinding, thread isolation, exact refresh intervals, empty maps, out-of-range indices, map growth and shrinkage, stale key replacement, and chunk payload ownership. Key enumeration is observed through a map wrapper to check rebuild frequency without inspecting private cache state.

The dispatcher and cache signature suites test the production instruction matchers: displacement wildcard byte values, corruption of each fixed byte, every incorrect expected byte, truncated buffers and tails, function boundaries, capacity values, already-patched sequences, and rejection of multiple matches. These tests validate matching, not writes to live BDS code or Windows unwind metadata.

These checks cover cache and batching behavior. BDS hook resolution, ABI compatibility, chunk save/load behavior, redstone behavior, and gameplay regressions still require server/client integration tests on the supported game version.
