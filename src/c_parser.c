#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "c_parser.h"
#include "c_types.h"

struct Function_T * load_from_file(char * fname) {
  FILE * f = fopen(fname, "r");
  char buffer[1000];
  int i = 0;
  while (!i || buffer[i-1] != EOF) {
    buffer[i] = fgetc(f);
    i++;
  }
  return parse_source_text(buffer, global_context);
}


// This will eat the datatype text, any * for pointers, and all trailing whitespace
struct DataType_T scan_datatype(char * text, int * pos) {
  struct DataType_T data_type = {.basic_type=BASICTYPE_INT, .pointer_level=0};

  if (text[*pos] == 'i')
    data_type.basic_type = BASICTYPE_INT;
  if (text[*pos] == 'c')
    data_type.basic_type = BASICTYPE_CHAR;
  
  while(text[*pos] != ' ' && text[*pos] != '\0')
    (*pos)++;
  while(text[*pos] == ' ')
    (*pos)++;
  while(text[*pos] == '*') {
    data_type.pointer_level++;
    (*pos)++;
    while(text[*pos] == ' ')
      (*pos)++;
  }
  return data_type;
}

struct Variable_T * variable_search(struct Variable_T * var, char * name) {
  while (var) {
    if (!strcmp(name, var->name)) 
      break;
    var = var->next;
  }
  return var;
}

struct Function_T * function_search(struct Function_T * func, char * name) {
  while (func) {
    if (!strcmp(name, func->name)) 
      break;
    func = func->next;
  }
  return func;
}

int parse_word(char * text, int * pos, char * buffer, int buffer_len) {
  int i = 0;
  for (i = 0; i < buffer_len-1; i++) {
    char c = text[*pos];
    // Only count numbers, letters, and underscores
    if (!(48 <= c && c <= 57) && !(65 <= c && c <= 90) && !(97 <= c && c <= 122) && c != '_')
      break;
    buffer[i] = c;
    (*pos)++;
  }
  buffer[i] = '\0';
  return i;
}

enum Condition_T parse_condition(char * text, int * pos) {
  int i;
  char buff[3];
  for (i = 0; i < 2; i++) {
    buff[i] = text[*pos];
    if (buff[i] != '<' && buff[i] != '>' && buff[i] != '=' && buff[i] != '!')
      break;
    (*pos)++;
  }
  buff[i] = '\n';
  if (i == 1) {
    if (buff[0] == '<')
      return CONDITION_LESS_THAN;
    else
      return CONDITION_GREATER_THAN;
  }
  else {
    if (buff[0] == '=')
      return CONDITION_EQUAL;
    if (buff[0] == '!')
      return CONDITION_NOT_EQUAL;
    if (buff[1] == '=') {
      if (buff[0] == '<')
        return CONDITION_LESS_THAN_OR_EQUAL;
      if (buff[0] == '>')
        return CONDITION_GREATER_THAN_OR_EQUAL;
    }
  }
  return CONDITION_EQUAL;
}

// This a greedy parser, it will tolerate bad formatting etc.
struct Function_T * parse_source_text(char * source_text, struct Function_T * context) {
  int i = 0;
  int arg_index = 0;
  struct Function_T * first_function = NULL;
  struct Function_T ** next_function_link_target = &first_function;
  struct Function_T * prev_function = NULL;
  struct Function_T * function = malloc(sizeof(struct Function_T));
  function->type = FUNCTION_TYPE_CUSTOM;
  function->prev = prev_function;
  function->next = NULL;
  function->first_codeline = NULL;
  function->first_var = NULL;
  
  int count;
  
  while (source_text[i] != '\0') {
    // Initialize new linked lists for both variables and codelines
    struct Variable_T ** next_var_link_target = &(function->first_var);
    struct Variable_T * prev_variable = NULL;
    struct Variable_T * variable = malloc(sizeof(struct Variable_T));
    variable->parent_function = function;
    variable->prev = prev_variable;
    variable->next = NULL;
    
    struct CodeLine_T ** next_codeline_link_target = &(function->first_codeline);
    struct CodeLine_T * prev_codeline = NULL;
    struct CodeLine_T * codeline = malloc(sizeof(struct CodeLine_T));
    codeline->parent_function = function;
    codeline->prev = prev_codeline;
    codeline->next = NULL;
    codeline->type = CODELINE_TYPE_INVALID;
    
    int nest_level = 0;
    struct CodeLine_T * codeline_nest_list[100];
    
    /* If we see an EOF at any point in this while, all subloops will 
     * just short-circuit and execution will flow to the bottom of the 
     * while, where we can clean up any messes
     * */
    
    
    // Hunting for a function start
    while (source_text[i] != '\0') {
      while(source_text[i] == ' ' || source_text[i] == '\n') {i++;}
      function->return_datatype = scan_datatype(source_text, &i);
      count = parse_word(source_text, &i, function->name, FUNCTION_NAME_LEN);
    
      if (count > 0 || source_text[i] == '\0')
        break;
        
      // If we failed to get a name, jump forward a character and try again.
      i++;
    }

    // Have the name, scan to argument list start
    while(source_text[i] != '(' && source_text[i] != '\0') {i++;}
    if (source_text[i] != '\0')
      i++;

    // Scan over argument list
    for (arg_index = 0; arg_index < FUNCTION_ARG_COUNT; arg_index++) {
      function->function_arguments[arg_index] = NULL;
      // Eat whitespace
      while (source_text[i] == ' '){i++;}
      if (source_text[i] == '\0' || source_text[i] == ')')
        break;
      // Must be a datatype then variable name next
      variable->data_type = scan_datatype(source_text, &i);
      count = parse_word(source_text, &i, variable->name, VARIABLE_NAME_LEN);

      if (count > 0) { // if j is 0, variable read failure
        // Read variable successfully, save and start new variable
        variable->is_arg = 1;
        *next_var_link_target = variable;
        next_var_link_target = &(variable->next);
        // Save link for argument
        function->function_arguments[arg_index] = variable;
        // New variable
        prev_variable = variable;
        variable = malloc(sizeof(struct Variable_T));
        variable->parent_function = function;
        variable->prev = prev_variable;
        variable->next = NULL;
      }
      else {
        // Retry for this arg_index
        arg_index--;
      }
      
      // Eat whitespace
      while (source_text[i] == ' '){i++;}
      if (source_text[i] == ',') {
        i++;
      }
    }

    // We should be pointing at the end parenthesis of the argument list now
    while(source_text[i] != '{' && source_text[i] != '\0') {i++;}
    if (source_text[i] != '\0')
      i++;
    
    // Iterate over codelines
    while (1) {
      while (source_text[i] == ' ' || source_text[i] == '\n'){i++;}
      if (source_text[i] == '\0')
        break;
      if (source_text[i] == '}' && nest_level == 0)
        break;
      // We are now pointing at an interesting line
      // Test if this is a declaration
      if (!strncmp(source_text + i, "int ", 4) || !strncmp(source_text + i, "char ", 5)) {
        // Handle a variable declaration
        variable->data_type = scan_datatype(source_text, &i);
        count = parse_word(source_text, &i, variable->name, VARIABLE_NAME_LEN);
        
        if (count > 0) { // if j is 0, variable read failure
          // Read variable successfully, save and start new variable
          variable->is_arg = 0;
          *next_var_link_target = variable;
          next_var_link_target = &(variable->next);
          // New variable
          prev_variable = variable;
          variable = malloc(sizeof(struct Variable_T));
          variable->parent_function = function;
          variable->prev = prev_variable;
          variable->next = NULL;
        }
      }
      else {
        char name_buffer[CODELINE_LEN];
        // Line is not a declaration -- so it could be an assignment, if statement, or while loop.
        if (!strncmp(source_text + i, "if", 2)){
          // need to get a variable, a condition, then a variable
          i += 2;
          while(source_text[i] == ' ') {i++;}
          // Should be pointing at opening parenthesis
          if (source_text[i] == '(') i++;
          while(source_text[i] == ' ') {i++;}
          parse_word(source_text, &i, name_buffer, VARIABLE_NAME_LEN);
          codeline->function_arguments[0] = variable_search(function->first_var, name_buffer);
          while(source_text[i] == ' ') {i++;}
          // Should now be looking at the condition operation, grab it
          codeline->condition = parse_condition(source_text, &i);
          while(source_text[i] == ' ') {i++;}
          // Get second variable
          parse_word(source_text, &i, name_buffer, VARIABLE_NAME_LEN);
          codeline->function_arguments[1] = variable_search(function->first_var, name_buffer);
          codeline->function_arguments[2] = NULL;
          // Step over parenthesis and open bracket
          while(source_text[i] == ' ') {i++;}
          if (source_text[i] == ')') i++;
          while(source_text[i] == ' ' || source_text[i] == '\n') {i++;}
          if (source_text[i] == '{') i++;
          
          // Should be success, verify
          if (codeline->function_arguments[0] && codeline->function_arguments[1]) {
            codeline->type = CODELINE_TYPE_IF;
            codeline_nest_list[nest_level] = codeline; // Save reference to codeline for linking up to block end later
            nest_level++;
          }
        }
        else if (!strncmp(source_text + i, "while", 5)) {
          // need to get a variable, a condition, then a variable
          i += 5;
          while(source_text[i] == ' ') {i++;}
          // Should be pointing at opening parenthesis
          if (source_text[i] == '(') i++;
          while(source_text[i] == ' ') {i++;}
          parse_word(source_text, &i, name_buffer, VARIABLE_NAME_LEN);
          codeline->function_arguments[0] = variable_search(function->first_var, name_buffer);
          while(source_text[i] == ' ') {i++;}
          // Should now be looking at the condition operation, grab it
          codeline->condition = parse_condition(source_text, &i);
          while(source_text[i] == ' ') {i++;}
          // Get second variable
          parse_word(source_text, &i, name_buffer, VARIABLE_NAME_LEN);
          codeline->function_arguments[1] = variable_search(function->first_var, name_buffer);
          codeline->function_arguments[2] = NULL;
          // Step over parenthesis and open bracket
          while(source_text[i] == ' ') {i++;}
          if (source_text[i] == ')') i++;
          while(source_text[i] == ' ' || source_text[i] == '\n') {i++;}
          if (source_text[i] == '{') i++;
          
          // Should be success, verify
          if (codeline->function_arguments[0] && codeline->function_arguments[1]) {
            codeline->type = CODELINE_TYPE_WHILE;
            codeline_nest_list[nest_level] = codeline; // Save reference to codeline for linking up to block end later
            nest_level++;
          }
        }
        else if (source_text[i] == '}') {
          // End of for or while loop
          i++;
          if (nest_level > 0) {
            nest_level--;
            codeline->block_other_end = codeline_nest_list[nest_level];
            codeline_nest_list[nest_level]->block_other_end = codeline;
            codeline->type = CODELINE_TYPE_BLOCK_END;
          }
        }
        else {
          // load it as an assignment
          count = parse_word(source_text, &i, name_buffer, VARIABLE_NAME_LEN);
          codeline->assigned_variable = variable_search(function->first_var, name_buffer);
          if (!codeline->assigned_variable) {
            printf("Did not recognize assigning variable %s", name_buffer);
            while (source_text[i] != '\n' && source_text[i] != '\0') i++;
          }

          while(source_text[i] == ' ') {i++;}
          // This should be an equal sign
          if (source_text[i] == '=')
            i++;
          while(source_text[i] == ' ') {i++;}
          // We should now be pointing at the start of the expression, test for constant expressions
          if (source_text[i] == '+' || source_text[i] == '-' || (48 <= source_text[i] && source_text[i] <= 57)){
            // Must be an integer constant, parse accordingly
            int sign = 1;
            if (source_text[i] == '+'){
              i++;
            }
            if (source_text[i] == '-'){
              sign = -1;
              i++;
            }
            while(source_text[i] == ' ') {i++;}
            int value = 0;
            while(48 <= source_text[i] && source_text[i] <= 57) {
              value = value*10 + (source_text[i] - 48);
              i++;
            }
            value *= sign;
            codeline->constant.i = value;
            // Success, int constant
            codeline->type = CODELINE_TYPE_CONSTANT_ASSIGNMENT;
          }
          else if (source_text[i] == '(') {
            // Assuming this is a char constant
            while (source_text[i] != ')' && source_text[i] != '\0'){i++;} // Fast-forward past )
            if (source_text[i] == ')') i++;
            while (source_text[i] == ' '){i++;}
            int value = 0;
            while(48 <= source_text[i] && source_text[i] <= 57) {
              value = value*10 + (source_text[i] - 48);
              i++;
            }
            codeline->constant.c = (char)value;
            codeline->type = CODELINE_TYPE_CONSTANT_ASSIGNMENT;
            // Success, char constant
          }
          else {
            count = parse_word(source_text, &i, name_buffer, CODELINE_LEN);
            while (source_text[i] == ' '){i++;}
            
            if (source_text[i] == '(') { // If next interesting character is (, then it is either a function call or a char constant
              // Must be a function call
                
              codeline->target_function =  function_search(context, name_buffer);
              i++; // Step past opening parenthesis
              // Parse through arguments
              
              for (arg_index = 0; arg_index < FUNCTION_ARG_COUNT; arg_index++) {
                while (source_text[i] == ' '){i++;}
                if (source_text[i] == ';' || source_text[i] == ')' || source_text[i] == '\0')
                  break;
                
                parse_word(source_text, &i, name_buffer, VARIABLE_NAME_LEN);
                codeline->function_arguments[arg_index] = variable_search(function->first_var, name_buffer);
                if (!codeline->function_arguments[arg_index])
                  arg_index--; // Failed to recognize that one, skip it
                while (source_text[i] == ' '){i++;}
                if (source_text[i] == ',')
                  i++;
              } 
              if (source_text[i] == ')') i++;
              if (codeline->target_function) {
                // Success
                codeline->type = CODELINE_TYPE_FUNCTION_CALL;
              }
            }
            else {
              // Must be a basic arithmetic operation
              char op = source_text[i]; // I points to op character
              codeline->function_arguments[0] = variable_search(function->first_var, name_buffer);
              name_buffer[0] = op;
              name_buffer[1] = '\0';
              codeline->target_function = function_search(context, name_buffer);
              // Scan in second argument
              i++;
              while (source_text[i] == ' '){i++;}
              
              count = parse_word(source_text, &i, name_buffer, VARIABLE_NAME_LEN);

              codeline->function_arguments[1] = variable_search(function->first_var, name_buffer);
              codeline->function_arguments[2] = NULL;
              
              if (!codeline->function_arguments[0] || !codeline->function_arguments[1] || !codeline->target_function) {
                printf("Sum ting wong...");
              }
              else
                codeline->type = CODELINE_TYPE_FUNCTION_CALL;
            }
            
          }
        }
      }
      
      if (codeline->type != CODELINE_TYPE_INVALID) {
        // Save the codeline
        *next_codeline_link_target = codeline;
        next_codeline_link_target = &(codeline->next);
        // New variable
        prev_codeline = codeline;
        codeline = malloc(sizeof(struct CodeLine_T));
        codeline->parent_function = function;
        codeline->prev = prev_codeline;
        codeline->next = NULL;
        codeline->type = CODELINE_TYPE_INVALID;
      }
      
      // Eat through any trailing semicolon
      while (source_text[i] == ' '){i++;}
      if (source_text[i] == ';'){i++;}
      
    }
    
    // If we are looking at an end bracket, the function read was successful.
    if (source_text[i] == '}') {
      i++;// eat }
      
      // Save this function
      function->last_var = prev_variable;
      function->last_codeline = prev_codeline;
      *next_function_link_target = function;
      next_function_link_target = &(function->next);
      // New function
      prev_function = function;
      function = malloc(sizeof(struct Function_T));
      function->type = FUNCTION_TYPE_CUSTOM;
      function->prev = prev_function;
      function->next = NULL;
      function->first_codeline = NULL;
      function->first_var = NULL;
      // Save variables and codelines used in this function from getting freed
      variable->prev = NULL;
      codeline->prev = NULL;
    }
    
    // Free any dangling linked-list segments and clean up for the next iteration
    
    while(variable){
      struct Variable_T * to_free_v = variable;
      variable = variable->prev;
      free(to_free_v);
    }
    
    while(codeline){
      struct CodeLine_T * to_free_c = codeline;
      codeline = codeline->prev;
      free(to_free_c);
    }
    
  }
  
  // Free the working function
  free(function);
  
  // We should be done!
  return first_function;
}
