#ifndef CAMPSEUDO_COMMON_H
#define CAMPSEUDO_COMMON_H

#include <stdbool.h>
#include <stdint.h>

#define DEBUG_CHUNK
#define DEBUG_AST
#define DEBUG_OBJ

#define DEBUG_TRACE_EXECUTION

#ifdef DEBUG_TRACE_EXECUTION
#define DEBUG_CHUNK
#endif

#ifdef DEBUG_CHUNK
#define DEBUG_OBJ
#endif

#endif