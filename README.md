# adecc-core-lib

**Modern C++23 building blocks for learning, experimentation and reusable software architecture.**

adecc-core-lib is a **header-only C++ library** accompanying the educational work of
[adecc Scholar](https://www.youtube.com/@adeccScholar) and *Rethinking C++ (C++ neu denken)*.
It brings together reusable abstractions for strong types, conversion policies, ranges,
data handling, persistence, and database integration. DeckKernel is one of its consumer projects.

## Contents

| Directory | Purpose |
| --- | --- |
| `adecc/cpp_core/` | Generic utilities, typed values, ranges, grids, persistence and database abstractions |
| `adecc/cpp_domain/` | Domain types, validation and conversion policies |
| `adecc/mssql/` | Microsoft SQL Server ODBC adapter headers |
| `adecc/postgre/` | PostgreSQL/libpqxx adapter headers |
| `adecc/openXlsxTools/` | OpenXLSX grid and workbook adapters |

The library itself does not build or distribute binary archives. Depending on
the headers used, consumers may need external libraries such as libpqxx or OpenXLSX.
The basic C++23 headers can be used independently of those adapters.

## Using the repository with CMake

Clone or add the repository as a Git submodule:

```sh
git submodule add https://github.com/adeccscholar/adecc-core-lib.git adecc-core-lib
```

In your project's `CMakeLists.txt`:

```cmake
add_subdirectory(adecc-core-lib)
target_link_libraries(my_application PRIVATE adecc::core)
```

Because the CMake interface exposes the repository root as an include directory,
headers retain their full, stable paths:

```cpp
#include <adecc/cpp_core/fixed_string.h>
#include <adecc/cpp_domain/IBAN.h>
```

Optional convenience targets: `adecc::domain`, `adecc::mssql`,
`adecc::postgre`, `adecc::openxlsx`. These are header-only interface
targets; they do **not** automatically provision third-party libraries.

### C++Builder / BCC64X

The project targets modern C++23 and is developed with Embarcadero C++Builder.
Consumers should configure their compiler/toolchain and required third-party
packages independently. The repository does not require a separate adecc DLL.

### Existing Git submodule checkouts

```sh
git clone --recurse-submodules https://github.com/adeccscholar/DeckKernel.git
```

The current DeckKernel submodule path is `adecc-core-lib/`, so public headers
live at `adecc-core-lib/adecc/...`. Include the repository root or use its
`adecc::core` target, not the old `DeckKernel/adecc/` directory.

## Licensing

Public noncommercial use is governed by **PolyForm Noncommercial License 1.0.0**.
Qualifying purchasers of *Rethinking C++ (C++ neu denken)* may receive an
additional, MIT-like permissive grant under the
[adecc Book License](adecc/ADECC_BOOK_LICENSE.md), including proprietary and
commercial uses as specified by its terms.

The supplemental license is a **custom license**, not the standard MIT License,
and is currently **a draft pending legal review**. Third-party components
retain their respective upstream licenses.

**Educational code, provided AS IS.** The library illustrates C++ techniques
and architecture. It is not certified or warranted as suitable for production
use. Users are responsible for their own verification, security and testing.

## Development

Use independent consumer builds to verify headers and compatibility.
Changes to this repository are adopted by consumers by updating their pinned
Git submodule commit, keeping builds reproducible.
