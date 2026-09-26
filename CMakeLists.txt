cmake_minimum_required(VERSION 3.20)
project(pg_orca LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_POSITION_INDEPENDENT_CODE TRUE)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug")
endif()
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

# -------------------------------------------------------------------
# PostgreSQL
# -------------------------------------------------------------------
find_program(_PG_CONFIG_DEFAULT pg_config)
set(PG_CONFIG "${_PG_CONFIG_DEFAULT}"
    CACHE PATH "Path to pg_config")

# execute_process results are not cached across cmake regeneration
# (e.g. when ninja detects CMakeLists.txt changed and re-invokes cmake).
# Strategy: always run pg_config; if it succeeds update the cache so
# changing PG_CONFIG is picked up immediately; if it fails (pg_config
# not on PATH during regeneration) keep the existing cached value.
macro(pg_config_query FLAG VAR DOC)
    execute_process(COMMAND ${PG_CONFIG} ${FLAG}
        OUTPUT_VARIABLE _pgcfg_result OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET RESULT_VARIABLE _pgcfg_rc)
    if(NOT _pgcfg_rc AND _pgcfg_result)
        set(${VAR} "${_pgcfg_result}" CACHE STRING "${DOC}" FORCE)
    elseif(NOT DEFINED ${VAR})
        set(${VAR} "" CACHE STRING "${DOC}")
    endif()
    # else: pg_config failed but cache already has a value — keep it
endmacro()

pg_config_query(--includedir-server PG_SERVER_INCLUDEDIR "PG server include dir")
pg_config_query(--includedir        PG_INCLUDEDIR        "PG include dir")
pg_config_query(--pkglibdir         PG_PKGLIBDIR         "PG pkglibdir")
pg_config_query(--bindir            PG_BINDIR            "PG bindir")
pg_config_query(--sharedir          PG_SHAREDIR          "PG sharedir")
pg_config_query(--ldflags_sl        PG_LDFLAGS_SL        "PG shared-lib ldflags")
pg_config_query(--version           PG_VERSION           "PG version string")

if(NOT PG_PKGLIBDIR)
    message(FATAL_ERROR "pg_config not found or returned empty output. "
        "Set -DPG_CONFIG=/path/to/pg_config explicitly.")
endif()

message(STATUS "PostgreSQL: ${PG_VERSION}")
message(STATUS "  server headers: ${PG_SERVER_INCLUDEDIR}")
message(STATUS "  pkglibdir:      ${PG_PKGLIBDIR}")

# -------------------------------------------------------------------
# xerces-c (needed by ORCA for DXL XML parsing).
#
# Default on Linux: build the bundled submodule under third_party/xerces-c
# as a static library folded into pg_orca.so. This makes the extension
# self-contained — no runtime libxerces-c.so dependency — which matters
# for the deb/rpm release packages (self-hosted Supabase, distroless
# containers, hosts without a matching xerces-c package).
#
# Default on macOS: use Homebrew's xerces-c. Release packaging is
# Linux-only; for macOS dev builds the system library is easier and
# avoids quirks of the static build on the Apple toolchain.
#
# Override either way with -DPG_ORCA_BUNDLED_XERCES=ON/OFF.
# -------------------------------------------------------------------
if(APPLE)
    set(_pg_orca_bundled_default OFF)
else()
    set(_pg_orca_bundled_default ON)
endif()
option(PG_ORCA_BUNDLED_XERCES
    "Build xerces-c from third_party/ submodule and link statically"
    ${_pg_orca_bundled_default})

if(PG_ORCA_BUNDLED_XERCES)
    set(_xerces_src "${CMAKE_SOURCE_DIR}/third_party/xerces-c")
    if(NOT EXISTS "${_xerces_src}/CMakeLists.txt")
        message(FATAL_ERROR
            "PG_ORCA_BUNDLED_XERCES=ON but ${_xerces_src}/CMakeLists.txt is "
            "missing. Run: git submodule update --init --recursive")
    endif()

    # Force xerces options before adding its subdirectory. FORCE is required
    # because xerces-c declares these with CACHE; without FORCE our values
    # would be ignored on the second cmake run.
    set(BUILD_SHARED_LIBS    OFF       CACHE BOOL   "" FORCE)
    set(network              OFF       CACHE BOOL   "" FORCE)  # no socket-based entity resolver
    set(transcoder           "gnuiconv" CACHE STRING "" FORCE) # use glibc iconv, not ICU
    set(message-loader       "inmemory" CACHE STRING "" FORCE) # no external catalog files
    set(threads              ON        CACHE BOOL   "" FORCE)
    # Suppress xerces install rules; we never install its headers/libs.
    set(CMAKE_SKIP_INSTALL_RULES_BACKUP "${CMAKE_SKIP_INSTALL_RULES}")
    set(CMAKE_SKIP_INSTALL_RULES TRUE)
    add_subdirectory(third_party/xerces-c EXCLUDE_FROM_ALL)
    set(CMAKE_SKIP_INSTALL_RULES "${CMAKE_SKIP_INSTALL_RULES_BACKUP}")

    set_target_properties(xerces-c PROPERTIES
        POSITION_INDEPENDENT_CODE ON
        CXX_VISIBILITY_PRESET     hidden
        VISIBILITY_INLINES_HIDDEN ON)

    # NOTE: xerces-c's own CMakeLists does NOT export its headers as a PUBLIC
    # usage requirement (no target_include_directories(xerces-c PUBLIC ...)),
    # so linking the target alone does NOT put its headers on the include path.
    # We must supply them explicitly. They live in three places:
    #   - the source tree src/           (hand-written headers, e.g. xercesc/*)
    #   - the build tree src/            (generated Xerces_autoconf_config.hpp)
    #   - the build tree root            (generated config.h)
    set(XERCES_LIB     xerces-c)
    set(XERCES_INCLUDE
        "${_xerces_src}/src"
        "${CMAKE_BINARY_DIR}/third_party/xerces-c/src"
        "${CMAKE_BINARY_DIR}/third_party/xerces-c")
    message(STATUS "xerces-c: bundled static build (third_party/xerces-c)")
else()
    # Legacy path: find a system-installed xerces-c.
    find_package(PkgConfig QUIET)
    if(PKG_CONFIG_FOUND)
        pkg_check_modules(XERCES xerces-c)
    endif()

    if(XERCES_FOUND)
        # pkg-config succeeded — prefer absolute paths from XERCES_LINK_LIBRARIES
        # so the linker doesn't need to search for it. On macOS Homebrew,
        # xerces-c lives in /opt/homebrew/opt/xerces-c/lib which is not a
        # default search path, so passing only "-lxerces-c" fails.
        if(XERCES_LINK_LIBRARIES)
            set(XERCES_LIB ${XERCES_LINK_LIBRARIES})
        else()
            set(XERCES_LIB ${XERCES_LIBRARIES})
        endif()
        set(XERCES_INCLUDE ${XERCES_INCLUDE_DIRS})
        message(STATUS "xerces-c (pkg-config): ${XERCES_LIB}")
    else()
        find_library(XERCES_LIB xerces-c
            PATHS
                /opt/homebrew/opt/xerces-c/lib   # Apple Silicon Homebrew
                /usr/local/opt/xerces-c/lib      # Intel Homebrew
                /usr/local/lib                   # Linux generic
                /usr/lib
                /usr/lib/x86_64-linux-gnu        # Debian/Ubuntu amd64
                /usr/lib/aarch64-linux-gnu       # Debian/Ubuntu arm64
        )
        find_path(XERCES_INCLUDE xercesc/util/XercesDefs.hpp
            PATHS
                /opt/homebrew/opt/xerces-c/include
                /usr/local/opt/xerces-c/include
                /usr/local/include
                /usr/include
        )
        if(NOT XERCES_LIB OR NOT XERCES_INCLUDE)
            message(FATAL_ERROR
                "xerces-c not found. Install it (e.g. 'apt install libxerces-c-dev' "
                "or 'brew install xerces-c'), set -DXERCES_LIB= and "
                "-DXERCES_INCLUDE= manually, or use the default "
                "-DPG_ORCA_BUNDLED_XERCES=ON to build from third_party/.")
        endif()
        message(STATUS "xerces-c (manual): ${XERCES_LIB}, headers: ${XERCES_INCLUDE}")
    endif()
endif()

# -------------------------------------------------------------------
# ANTLR4 C++ runtime (for the MONSOON DSL rule parser)
# Mirrors the xerces-c bundling: build the runtime statically from a
# submodule and fold it into pg_orca.so / gporca_test. The generated
# lexer/parser (produced at build time by the ANTLR4 tool jar, see
# libgpopt/src/dsl/CMakeLists.txt) link against this runtime.
# -------------------------------------------------------------------
option(PG_ORCA_BUNDLED_ANTLR4
    "Build the ANTLR4 C++ runtime from third_party/ and link statically"
    ON)

if(PG_ORCA_BUNDLED_ANTLR4)
    set(_antlr4_src "${CMAKE_SOURCE_DIR}/third_party/antlr4/runtime/Cpp")
    if(NOT EXISTS "${_antlr4_src}/CMakeLists.txt")
        message(FATAL_ERROR
            "PG_ORCA_BUNDLED_ANTLR4=ON but ${_antlr4_src}/CMakeLists.txt is "
            "missing. Run: git submodule update --init --recursive")
    endif()

    # Force runtime options before adding its subdirectory (CACHE + FORCE, same
    # reason as xerces). Build only the static lib; disable demo/tests.
    set(ANTLR4_INSTALL       OFF CACHE BOOL "" FORCE)
    set(ANTLR_BUILD_CPP_TESTS OFF CACHE BOOL "" FORCE)
    set(WITH_DEMO            OFF CACHE BOOL "" FORCE)
    # The vendored runtime defaults to libc++ whenever Clang is used on Linux.
    # Linux CI and normal distro toolchains use Clang with libstdc++; forcing
    # libc++ there makes even standard headers such as <algorithm> unavailable
    # unless a second C++ standard library is installed.
    set(WITH_LIBCXX          OFF CACHE BOOL
        "Build the bundled ANTLR runtime with libc++ instead of libstdc++")
    # ANTLR 4.8 declares cmake_minimum_required(VERSION 2.8). CMake 4 no
    # longer enables policies older than 3.5 unless this floor is explicit.
    set(CMAKE_POLICY_VERSION_MINIMUM 3.5 CACHE STRING
        "Oldest CMake policy version accepted by bundled dependencies")

    # ANTLR 4.8 explicitly selects OLD behavior for several CMake 3.x
    # policies. CMake 4 removed those OLD implementations, so merely raising
    # CMAKE_POLICY_VERSION_MINIMUM is insufficient and configuration aborts.
    # Keep the pinned runtime sources untouched: on CMake 4+, build a private
    # compatibility copy with the obsolete policy selections changed to NEW.
    set(_antlr4_build_src "${_antlr4_src}")
    if(CMAKE_VERSION VERSION_GREATER_EQUAL 4.0)
        set(_antlr4_build_src
            "${CMAKE_BINARY_DIR}/_deps/antlr4-runtime-cpp-src")
        file(COPY "${_antlr4_src}/" DESTINATION "${_antlr4_build_src}")
        file(READ "${_antlr4_build_src}/CMakeLists.txt" _antlr4_cmake)
        foreach(_policy CMP0042 CMP0045 CMP0054 CMP0059)
            string(REPLACE
                "CMAKE_POLICY(SET ${_policy} OLD)"
                "CMAKE_POLICY(SET ${_policy} NEW)"
                _antlr4_cmake "${_antlr4_cmake}")
        endforeach()
        file(WRITE "${_antlr4_build_src}/CMakeLists.txt" "${_antlr4_cmake}")
    endif()

    set(CMAKE_SKIP_INSTALL_RULES_BACKUP "${CMAKE_SKIP_INSTALL_RULES}")
    set(CMAKE_SKIP_INSTALL_RULES TRUE)
    add_subdirectory(
        "${_antlr4_build_src}"
        "${CMAKE_BINARY_DIR}/third_party/antlr4/runtime/Cpp"
        EXCLUDE_FROM_ALL)
    set(CMAKE_SKIP_INSTALL_RULES "${CMAKE_SKIP_INSTALL_RULES_BACKUP}")

    # The static runtime target is "antlr4_static".
    set_target_properties(antlr4_static PROPERTIES
        # ANTLR 4.8 defaults to the source tree's dist/, shared by all builds.
        ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/third_party/antlr4/lib"
        POSITION_INDEPENDENT_CODE ON
        CXX_VISIBILITY_PRESET     hidden
        VISIBILITY_INLINES_HIDDEN ON)

    set(ANTLR4_LIB     antlr4_static)
    # The runtime headers live under runtime/src; expose them to consumers.
    set(ANTLR4_INCLUDE "${_antlr4_build_src}/runtime/src")
    message(STATUS "ANTLR4 C++ runtime: bundled static build")
else()
    find_package(antlr4-runtime CONFIG REQUIRED)
    set(ANTLR4_LIB     antlr4_shared)
    set(ANTLR4_INCLUDE "")
    message(STATUS "ANTLR4 C++ runtime: system package")
endif()

# Generate the DSL parser (defines DSL_GENERATED_SRC / DSL_GENERATED_DIR).
add_subdirectory(libgpopt/src/dsl)

# -------------------------------------------------------------------
# gettext/libintl headers (needed by libintl used in gpos)
# On Linux, libintl.h is part of glibc and always in the standard
# include path. On macOS it is a separate Homebrew formula.
# -------------------------------------------------------------------
set(EXTRA_SYSTEM_INCLUDES "")
if(APPLE)
    foreach(DIR
            /opt/homebrew/opt/gettext/include   # Apple Silicon Homebrew
            /usr/local/opt/gettext/include)     # Intel Homebrew
        if(EXISTS "${DIR}/libintl.h")
            list(APPEND EXTRA_SYSTEM_INCLUDES "${DIR}")
            break()
        endif()
    endforeach()
endif()

# -------------------------------------------------------------------
# ORCA core libraries (libgpos, libnaucrates, libgpopt, libgpdbcost)
# Built as a single object library so all symbols are available to
# the extension shared library without needing a separate .a/.so.
# -------------------------------------------------------------------
file(GLOB_RECURSE ORCA_CORE_SRC CONFIGURE_DEPENDS
    ${CMAKE_SOURCE_DIR}/libgpos/src/*.cpp
    ${CMAKE_SOURCE_DIR}/libnaucrates/src/*.cpp
    ${CMAKE_SOURCE_DIR}/libgpopt/src/*.cpp
    ${CMAKE_SOURCE_DIR}/libgpdbcost/src/*.cpp
)

add_library(orca_core OBJECT ${ORCA_CORE_SRC} ${DSL_GENERATED_SRC})

# orca_core includes the ANTLR4-generated DSL parser; make sure codegen runs
# first.
add_dependencies(orca_core dsl_codegen)

# Pull in xerces-c headers via the target's PUBLIC interface. For the
# bundled build XERCES_LIB is the "xerces-c" target which propagates its
# header search path; for the system build XERCES_LIB is a library path
# with no usage requirements (XERCES_INCLUDE supplies headers separately,
# see target_include_directories below). OBJECT libs accept
# target_link_libraries since CMake 3.12 — no symbol is actually linked,
# but compile-time properties propagate.
target_link_libraries(orca_core PUBLIC ${XERCES_LIB} ${ANTLR4_LIB})

target_compile_definitions(orca_core PUBLIC
    $<$<CONFIG:Debug>:GPOS_DEBUG>
    USE_CMAKE
)

target_compile_options(orca_core PUBLIC
    -Wall
    -Wno-unused-parameter
    -Wno-sign-compare
)

target_include_directories(orca_core PUBLIC
    ${CMAKE_SOURCE_DIR}/libgpos/include
    ${CMAKE_SOURCE_DIR}/libnaucrates/include
    ${CMAKE_SOURCE_DIR}/libgpopt/include
    ${CMAKE_SOURCE_DIR}/libgpdbcost/include
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}           # allows "compat/utils/foo.h" style includes
    ${CMAKE_SOURCE_DIR}/compat   # stub headers replacing GPDB-specific includes
    ${DSL_GENERATED_DIR}         # ANTLR4-generated DSLRuleParser.h etc.
    SYSTEM
    ${PG_SERVER_INCLUDEDIR}
    ${PG_INCLUDEDIR}
    ${XERCES_INCLUDE}
    ${ANTLR4_INCLUDE}
    ${EXTRA_SYSTEM_INCLUDES}
)

# -------------------------------------------------------------------
# PG integration layer (gpopt/ directory)
# -------------------------------------------------------------------
file(GLOB_RECURSE GPOPT_SRC CONFIGURE_DEPENDS
    ${CMAKE_SOURCE_DIR}/gpopt/config/*.cpp
    ${CMAKE_SOURCE_DIR}/gpopt/relcache/*.cpp
    ${CMAKE_SOURCE_DIR}/gpopt/translate/*.cpp
    ${CMAKE_SOURCE_DIR}/gpopt/utils/*.cpp
    ${CMAKE_SOURCE_DIR}/gpopt/CGPOptimizer.cpp
    ${CMAKE_SOURCE_DIR}/gpopt/gpdbwrappers.cpp
)

# -------------------------------------------------------------------
# pg_orca shared library (the actual PostgreSQL extension)
# -------------------------------------------------------------------
add_library(pg_orca MODULE
    pg_orca.cpp
    ${GPOPT_SRC}
    $<TARGET_OBJECTS:orca_core>
    compat/utils/walkers.c
    compat/utils/misc.c
    compat/utils/flatten_join_alias_var.c
    compat/utils/remove_redundant_results.cpp
    compat/executor/dyn_scan.c
)

set_target_properties(pg_orca PROPERTIES
    PREFIX ""       # PostgreSQL extensions have no "lib" prefix
    SUFFIX ".so"    # pg_config expects .so even on macOS
)

# On macOS the extension must reference the postgres binary as the "loader"
if(APPLE)
    set_target_properties(pg_orca PROPERTIES
        LINK_FLAGS "${PG_LDFLAGS_SL} -bundle_loader ${PG_BINDIR}/postgres"
    )
endif()

target_compile_definitions(pg_orca PRIVATE
    $<$<CONFIG:Debug>:GPOS_DEBUG>
    USE_CMAKE
)

target_compile_options(pg_orca PRIVATE
    -Wall
    -Wno-unused-parameter
    -Wno-sign-compare
    -Wno-ignored-attributes
    -fvisibility=hidden
)

target_include_directories(pg_orca PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/libgpos/include
    ${CMAKE_SOURCE_DIR}/libnaucrates/include
    ${CMAKE_SOURCE_DIR}/libgpopt/include
    ${CMAKE_SOURCE_DIR}/libgpdbcost/include
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/compat
    SYSTEM
    ${PG_SERVER_INCLUDEDIR}
    ${PG_INCLUDEDIR}
    ${XERCES_INCLUDE}
    ${EXTRA_SYSTEM_INCLUDES}
)

target_link_libraries(pg_orca PRIVATE ${XERCES_LIB} ${ANTLR4_LIB})

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    target_link_libraries(pg_orca PRIVATE dl)
endif()

# -------------------------------------------------------------------
# Install
# -------------------------------------------------------------------
install(TARGETS pg_orca
    LIBRARY DESTINATION "${PG_PKGLIBDIR}")

# On macOS, PostgreSQL looks for .dylib but we build .so — create a symlink.
if(APPLE)
    install(CODE "
        execute_process(COMMAND ${CMAKE_COMMAND} -E create_symlink
            pg_orca.so
            \"${PG_PKGLIBDIR}/pg_orca.dylib\")
    ")
endif()

install(FILES
    pg_orca.control
    pg_orca--1.0.0.sql
    DESTINATION "${PG_SHAREDIR}/extension")

# -------------------------------------------------------------------
# gporca_test — standalone ORCA optimizer test binary
# Supports: -d <file.mdp>  execute a minidump
#           -p             print the resulting DXL plan
#           -i <plan_id>   select a specific plan (enables enumeration)
#           -T <flag>      set a trace flag
# -------------------------------------------------------------------
add_executable(gporca_test
    tools/gporca_test/main.cpp
    tools/gporca_test/pg_cost_stubs.cpp
    $<TARGET_OBJECTS:orca_core>
)

target_compile_definitions(gporca_test PRIVATE
    $<$<CONFIG:Debug>:GPOS_DEBUG>
    USE_CMAKE
)

target_compile_options(gporca_test PRIVATE
    -Wall
    -Wno-unused-parameter
    -Wno-sign-compare
)

target_include_directories(gporca_test PRIVATE
    ${CMAKE_SOURCE_DIR}/libgpos/include
    ${CMAKE_SOURCE_DIR}/libnaucrates/include
    ${CMAKE_SOURCE_DIR}/libgpopt/include
    ${CMAKE_SOURCE_DIR}/libgpdbcost/include
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/compat
    SYSTEM
    ${PG_SERVER_INCLUDEDIR}
    ${PG_INCLUDEDIR}
    ${XERCES_INCLUDE}
    ${EXTRA_SYSTEM_INCLUDES}
)

target_link_libraries(gporca_test PRIVATE ${XERCES_LIB} ${ANTLR4_LIB})

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    target_link_libraries(gporca_test PRIVATE dl)
endif()

# -------------------------------------------------------------------
# gpopt_dsl_test — unit tests for the MONSOON DSL rule engine
# (parser, IR, loader; later the xform engine). Uses the GPOS CUnittest
# framework. Run: ./gpopt_dsl_test
# -------------------------------------------------------------------
file(GLOB_RECURSE DSL_TEST_SRC CONFIGURE_DEPENDS
    ${CMAKE_SOURCE_DIR}/libgpopt/server/src/unittest/*.cpp
)

add_executable(gpopt_dsl_test
    tools/gpopt_dsl_test/main.cpp
    tools/gporca_test/pg_cost_stubs.cpp
    ${DSL_TEST_SRC}
    $<TARGET_OBJECTS:orca_core>
)
add_dependencies(gpopt_dsl_test dsl_codegen)

target_compile_definitions(gpopt_dsl_test PRIVATE
    $<$<CONFIG:Debug>:GPOS_DEBUG>
    USE_CMAKE
)

target_compile_options(gpopt_dsl_test PRIVATE
    -Wall
    -Wno-unused-parameter
    -Wno-sign-compare
)

target_include_directories(gpopt_dsl_test PRIVATE
    ${CMAKE_SOURCE_DIR}/libgpos/include
    ${CMAKE_SOURCE_DIR}/libnaucrates/include
    ${CMAKE_SOURCE_DIR}/libgpopt/include
    ${CMAKE_SOURCE_DIR}/libgpopt/server/include   # unittest/gpopt/dsl/*.h
    ${CMAKE_SOURCE_DIR}/libgpdbcost/include
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/compat
    ${DSL_GENERATED_DIR}
    SYSTEM
    ${PG_SERVER_INCLUDEDIR}
    ${PG_INCLUDEDIR}
    ${XERCES_INCLUDE}
    ${ANTLR4_INCLUDE}
    ${EXTRA_SYSTEM_INCLUDES}
)

target_link_libraries(gpopt_dsl_test PRIVATE ${XERCES_LIB} ${ANTLR4_LIB})

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    target_link_libraries(gpopt_dsl_test PRIVATE dl)
endif()

# -------------------------------------------------------------------
# pgorca_rule_audit — static capability audit for real MONSOON rule corpora.
# Reuses the production parser/IR and XformFactory; writes coverage.json,
# unsupported_features.csv, and replacement_candidates.csv.
# -------------------------------------------------------------------
add_executable(pgorca_rule_audit
    tools/pgorca_rule_audit/main.cpp
    tools/gporca_test/pg_cost_stubs.cpp
    $<TARGET_OBJECTS:orca_core>
)
add_dependencies(pgorca_rule_audit dsl_codegen)

target_compile_definitions(pgorca_rule_audit PRIVATE
    $<$<CONFIG:Debug>:GPOS_DEBUG>
    USE_CMAKE
)

target_compile_options(pgorca_rule_audit PRIVATE
    -Wall
    -Wno-unused-parameter
    -Wno-sign-compare
)

target_include_directories(pgorca_rule_audit PRIVATE
    ${CMAKE_SOURCE_DIR}/libgpos/include
    ${CMAKE_SOURCE_DIR}/libnaucrates/include
    ${CMAKE_SOURCE_DIR}/libgpopt/include
    ${CMAKE_SOURCE_DIR}/libgpdbcost/include
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/compat
    ${DSL_GENERATED_DIR}
    SYSTEM
    ${PG_SERVER_INCLUDEDIR}
    ${PG_INCLUDEDIR}
    ${XERCES_INCLUDE}
    ${ANTLR4_INCLUDE}
    ${EXTRA_SYSTEM_INCLUDES}
)

target_link_libraries(pgorca_rule_audit PRIVATE ${XERCES_LIB} ${ANTLR4_LIB})

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    target_link_libraries(pgorca_rule_audit PRIVATE dl)
endif()

enable_testing()
option(PGORCA_BUILD_ML_ORCA_TESTS "Enable optional ML-ORCA Python component tests" OFF)
if(PGORCA_BUILD_ML_ORCA_TESTS)
    if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tools/ml-orca/CMakeLists.txt")
        message(FATAL_ERROR "Run git submodule update --init tools/ml-orca before enabling ML-ORCA tests")
    endif()
    add_subdirectory(tools/ml-orca)
endif()
add_test(NAME gpopt_dsl_test COMMAND gpopt_dsl_test)
add_test(
    NAME pgorca_rule_audit_fixture
    COMMAND pgorca_rule_audit
            ${CMAKE_SOURCE_DIR}/test/dsl/audit
            ${CMAKE_BINARY_DIR}/rule-audit-fixture
)
set_tests_properties(pgorca_rule_audit_fixture PROPERTIES
    PASS_REGULAR_EXPRESSION
    "candidates=7 parsed=5 supported_static=4 unsupported_static=1 parse_failed=1 skipped_non_eq=1 native_exploration=98 semantic_rewrite=50 join_enumeration=19 implementation_property=29 graph_nodes=3 static_edges=5 unresolved_inputs=3"
)

# -------------------------------------------------------------------
# Packaging (DEB/RPM via CPack). Linux only.
# -------------------------------------------------------------------
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    include(${CMAKE_SOURCE_DIR}/packaging/CPackConfig.cmake)
endif()
