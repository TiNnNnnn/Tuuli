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
    message(FATAL_ERROR
        "ANTLR4 tool jar not found. Set -DANTLR4_JAR=/path/to/antlr-4.8-complete.jar "
        "(or the ANTLR4_JAR env var). Download from https://www.antlr.org/download/ .")
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
