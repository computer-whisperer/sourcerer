#include "c_types.h"

#ifndef C_PARSER_H
#define C_PARSER_H

enum ParseChainType_T {
  PARSE_CHAIN_TYPE_ROOT,

  PARSE_CHAIN_TYPE_BLOCK,
  PARSE_CHAIN_TYPE_BLOCK_BODY,
  PARSE_CHAIN_TYPE_BLOCK_DECLARATION,

  PARSE_CHAIN_TYPE_FUNCTION,
  PARSE_CHAIN_TYPE_FUNCTION_DECLARATION,
  PARSE_CHAIN_TYPE_FUNCTION_ARGUMENTS,
  PARSE_CHAIN_TYPE_FUNCTION_ARGUMENT,
  PARSE_CHAIN_TYPE_FUNCTION_BODY,

  PARSE_CHAIN_TYPE_DECLARATION,
  PARSE_CHAIN_TYPE_DECLARATION_NAME,
  PARSE_CHAIN_TYPE_DECLARATION_DATATYPE,
  PARSE_CHAIN_TYPE_ARGUMENTS,
  PARSE_CHAIN_TYPE_CODELINE,
  PARSE_CHAIN_TYPE_VARIABLE_DECLARATION,
  PARSE_CHAIN_TYPE_VARIABLE_REFERENCE,
  PARSE_CHAIN_TYPE_UNKNOWN,
};

struct ParseChain_T {
  enum ParseChainType_T type;
  
  char * data_start;
  char * data_end;
  
  struct ParseChain_T * first_sub_chain;
  struct ParseChain_T * last_sub_chain;
  
  struct ParseChain_T * parent;
  struct ParseChain_T * next;
  struct ParseChain_T * prev;
};


void load_from_file(struct Environment_T * env, char * fname);
void load_from_data(struct Environment_T * env, char * data);

#endif  /* C_PARSER_H */
