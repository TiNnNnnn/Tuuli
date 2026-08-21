# ---------------------------------------------------------------------------
# MONSOON DSL rule engine — ANTLR4 grammar code generation.
#
# Runs the ANTLR4 tool (Java) at build time to generate the C++ lexer/parser
# for DSLRule.g4 into ${DSL_GENERATED_DIR}. The generated sources and their
# include dir are exported to the parent scope so the main CMakeLists can add
# them to orca_core. Requires:
#   - a Java runtime (java on PATH)
#   - the ANTLR4 "complete" tool jar, located via ANTLR4_JAR (cache var) or the
#     ANTLR4_JAR environment variable. The same antlr-4.8-complete.jar WeTune
#     uses works here.
# ---------------------------------------------------------------------------

find_package(Java COMPONENTS Runtime REQUIRED)

# Locate the ANTLR4 tool jar. Prefer an explicit -DANTLR4_JAR=..., then the
# environment, then a few well-known spots.
if(NOT ANTLR4_JAR)
    if(DEFINED ENV{ANTLR4_JAR})
        set(ANTLR4_JAR "$ENV{ANTLR4_JAR}")
    else()
        find_file(ANTLR4_JAR
            NAMES antlr-4.8-complete.jar antlr4-complete.jar antlr-complete.jar
            PATHS
                /usr/local/lib /usr/share/java /opt/antlr4
            DOC "Path to the ANTLR4 complete (tool) jar")
    endif()
endif()

if(NOT ANTLR4_JAR OR NOT EXISTS "${ANTLR4_JAR}")
    # A clean checkout must be buildable without an ignored local .cache.
    # Pin both the tool version and digest so CI does not execute an
    # unauthenticated, mutable download. An explicit ANTLR4_JAR still wins.
    set(_ANTLR4_DOWNLOAD_DIR "${CMAKE_BINARY_DIR}/_deps/antlr4-tool")
    set(_ANTLR4_DOWNLOAD_JAR
        "${_ANTLR4_DOWNLOAD_DIR}/antlr-4.8-complete.jar")
    file(MAKE_DIRECTORY "${_ANTLR4_DOWNLOAD_DIR}")
    message(STATUS "ANTLR4 tool jar not found locally; downloading 4.8")
    file(DOWNLOAD
        "https://www.antlr.org/download/antlr-4.8-complete.jar"
        "${_ANTLR4_DOWNLOAD_JAR}"
        EXPECTED_HASH
            "SHA256=73a49d6810d903aa4827ee32126937b85d3bebec0a8e679b0dd963cbcc49ba5a"
        TLS_VERIFY ON
        STATUS _ANTLR4_DOWNLOAD_STATUS
        LOG _ANTLR4_DOWNLOAD_LOG)
    list(GET _ANTLR4_DOWNLOAD_STATUS 0 _ANTLR4_DOWNLOAD_CODE)
    if(NOT _ANTLR4_DOWNLOAD_CODE EQUAL 0)
        message(FATAL_ERROR
            "Failed to download the pinned ANTLR4 4.8 tool jar: "
            "${_ANTLR4_DOWNLOAD_STATUS}\n${_ANTLR4_DOWNLOAD_LOG}\n"
            "Set -DANTLR4_JAR=/path/to/antlr-4.8-complete.jar for an offline build.")
    endif()
    set(ANTLR4_JAR "${_ANTLR4_DOWNLOAD_JAR}")
endif()
message(STATUS "ANTLR4 tool jar: ${ANTLR4_JAR}")

set(DSL_GRAMMAR "${CMAKE_CURRENT_SOURCE_DIR}/DSLRule.g4")
set(DSL_GENERATED_DIR "${CMAKE_BINARY_DIR}/generated/dsl")

# The generated translation units we compile. ANTLR 4.8 always emits the
# listener pair even without -visitor; we compile them (harmless) since our
# parser walks the typed contexts directly rather than via callbacks.
set(DSL_GENERATED_SRC
    ${DSL_GENERATED_DIR}/DSLRuleLexer.cpp
    ${DSL_GENERATED_DIR}/DSLRuleParser.cpp
    ${DSL_GENERATED_DIR}/DSLRuleListener.cpp
    ${DSL_GENERATED_DIR}/DSLRuleBaseListener.cpp
)

add_custom_command(
    OUTPUT ${DSL_GENERATED_SRC}
        ${DSL_GENERATED_DIR}/DSLRuleLexer.h
        ${DSL_GENERATED_DIR}/DSLRuleParser.h
    COMMAND ${CMAKE_COMMAND} -E make_directory ${DSL_GENERATED_DIR}
    COMMAND ${Java_JAVA_EXECUTABLE} -jar ${ANTLR4_JAR}
        -Dlanguage=Cpp -package dsl
        -o ${DSL_GENERATED_DIR}
        ${DSL_GRAMMAR}
    DEPENDS ${DSL_GRAMMAR}
    COMMENT "ANTLR4: generating C++ parser for DSLRule.g4"
    VERBATIM
)

add_custom_target(dsl_codegen DEPENDS ${DSL_GENERATED_SRC})

# Mark generated sources as GENERATED so CMake doesn't demand them upfront.
set_source_files_properties(${DSL_GENERATED_SRC} PROPERTIES GENERATED TRUE)

# Export to parent scope.
set(DSL_GENERATED_SRC "${DSL_GENERATED_SRC}" PARENT_SCOPE)
set(DSL_GENERATED_DIR "${DSL_GENERATED_DIR}" PARENT_SCOPE)
