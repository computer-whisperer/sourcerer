#include "c_types.h"

#ifndef C_PARSER_H
#define C_PARSER_H

struct Function_T * parse_source_text(char * source_text, struct Function_T * context);

#endif  /* C_PARSER_H */
