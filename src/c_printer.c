#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "c_types.h"
#include "c_printer.h"


char * concat_generated_lines(struct GeneratedLine_T * generated_lines, int max_len) {
  // Figure out the total size before allocating the buffer
  int buffer_size_needed = 0;
  struct GeneratedLine_T * pointer = generated_lines;
  while(pointer) {
    buffer_size_needed += strlen(pointer->text) + 1; // text and a newline
    if (max_len == 0)
      break;
    max_len--;
    pointer = pointer->next;
  }
  buffer_size_needed += 1; // Add a null char to the end
  
  char * buffer = malloc(buffer_size_needed);
  pointer = generated_lines;
  int i = 0;
  while(pointer) {
    strcpy(buffer+i, pointer->text);
    i += strlen(pointer->text);
    buffer[i] = '\n';
    i++;
    if (i+1 == buffer_size_needed)
      break;
    pointer = pointer->next;
  }
  buffer[i] = '\0';
  return buffer;
}

void insert_type_string(struct DataType_T * data_type, char* data, int* pos){
  
  strcpy(data + *pos, data_type->name);
  *pos += strlen(data_type->name);
  data[*pos] = ' ';
  (*pos)++;
}

char * get_condition_string(enum Condition_T condition){
  switch (condition) {
    case CONDITION_EQUAL:
      return " == ";
    case CONDITION_NOT_EQUAL:
      return " != ";
    case CONDITION_GREATER_THAN:
      return " > ";
    case CONDITION_GREATER_THAN_OR_EQUAL:
      return " >= ";
    case CONDITION_LESS_THAN:
      return " < ";
    case CONDITION_LESS_THAN_OR_EQUAL:
      return " <= ";
  }
  // Default
  return " == ";
}

struct GeneratedLine_T * generate_c(struct Function_T* function){
  int indent_level = 0;
  struct GeneratedLine_T * start_line = NULL;
  struct GeneratedLine_T ** last_link = &start_line;
  struct GeneratedLine_T * new_line;

  int i, j;
  while (function) {
    
    if (function->type == FUNCTION_TYPE_CUSTOM) {
      new_line = malloc(sizeof(struct GeneratedLine_T));
      *last_link = new_line;
      last_link = &(new_line->next);
      
      j=0;
      // insert type string, name, and opening parenthesis
      insert_type_string(function->return_datatype, new_line->text, &j);
      strcpy(new_line->text+j, function->name);
      j += strlen(function->name);
      new_line->text[j] = '(';
      j += 1;
      // insert arguments
      for (int k = 0; k < FUNCTION_ARG_COUNT; k++) {
        if (!function->args[k])
          break;
        insert_type_string(function->args[k]->data_type, new_line->text, &j);
        strcpy(new_line->text + j, function->args[k]->name);
        j += strlen(function->args[k]->name);
        if (k < FUNCTION_ARG_COUNT - 1 && function->args[k+1]) {
          strcpy(new_line->text + j, ", ");
          j += 2;
        }
      }
      new_line->text[j] = ')';
      new_line->text[j+1] = '{';
      new_line->text[j+2] = '\0';
      indent_level++;
      // Add sourcerer function macro
      /*
      new_line = malloc(sizeof(struct GeneratedLine_T));
      *last_link = new_line;
      last_link = &(new_line->next);
      j = 0;
      while (j < indent_level*SPACES_PER_INDENT) {
        new_line->text[j] = ' ';
        j++;
      }
      strcpy(new_line->text+j, "SOURCERER_FUNCTION()");
      */
      // Add scope variable definitions
      struct Variable_T * variable = function->first_variable;
      while (variable) {
        if (!variable->is_arg) {
          new_line = malloc(sizeof(struct GeneratedLine_T));
          *last_link = new_line;
          last_link = &(new_line->next);
          j = 0;
          while (j < indent_level*SPACES_PER_INDENT) {
            new_line->text[j] = ' ';
            j++;
          }
          // Type string;
          insert_type_string(variable->data_type, new_line->text, &j);
          // Name
          strcpy(new_line->text + j, variable->name);
          j += strlen(variable->name);
          // Semicolon
          new_line->text[j] = ';';
          new_line->text[j+1] = '\0';
        }
        variable = variable->next;
      }
      
      struct CodeLine_T * codeline = function->first_codeline;
      while (codeline) {
        new_line = malloc(sizeof(struct GeneratedLine_T));
        *last_link = new_line;
        last_link = &(new_line->next);
        
        char * cond_string;
        
        j = 0;
        switch (codeline->type) {
          case CODELINE_TYPE_FUNCTION_CALL:
            if (codeline->target_function->type == FUNCTION_TYPE_BASIC_OP) {
              while (j < indent_level*SPACES_PER_INDENT) {
                new_line->text[j] = ' ';
                j++;
              }
              strcpy(new_line->text + j, codeline->assigned_variable->name);
              j += strlen(codeline->assigned_variable->name);
              strcpy(new_line->text + j, " = ");
              j += 3;
              strcpy(new_line->text + j, codeline->args[0]->name);
              j += strlen(codeline->args[0]->name);
              strcpy(new_line->text + j, " + ");
              j += 3;
              new_line->text[j-2] = codeline->target_function->name[0];
              strcpy(new_line->text + j, codeline->args[1]->name);
              j += strlen(codeline->args[1]->name);
              strcpy(new_line->text + j, ";\0");
            }
            else {
              while (j < indent_level*SPACES_PER_INDENT) {
                new_line->text[j] = ' ';
                j++;
              }
              
              strcpy(new_line->text + j, codeline->assigned_variable->name);
              j += strlen(codeline->assigned_variable->name);
              strcpy(new_line->text + j, " = ");
              j += 3;
              strcpy(new_line->text + j, codeline->target_function->name);
              j += strlen(codeline->target_function->name);
              strcpy(new_line->text + j, "(");
              j += 1;
              for (int k = 0; k < FUNCTION_ARG_COUNT; k++) {
                if (!codeline->args[k])
                  break;
                strcpy(new_line->text + j, codeline->args[k]->name);
                j += strlen(codeline->args[k]->name);
                if (k < FUNCTION_ARG_COUNT - 1 && codeline->args[k+1]) {
                  strcpy(new_line->text + j, ", ");
                  j += 2;
                }
              }
              strcpy(new_line->text + j, ");\0");
            }
            break;
          case CODELINE_TYPE_CONSTANT_ASSIGNMENT:
            if (!strcmp(codeline->assigned_variable->data_type->name, "char")) {
              unsigned char value = codeline->constant.c;
              // for example: a = (char)004;
              while (j < indent_level*SPACES_PER_INDENT) {
                new_line->text[j] = ' ';
                j++;
              }
              strcpy(new_line->text + j, codeline->assigned_variable->name);
              j += strlen(codeline->assigned_variable->name);
              strcpy(new_line->text + j, " = ");
              j += 3;

              if (value >= 32 && value < 127 && value != '\\' && value != '\'' && value != '\"' && value != '\?') {
                // Normal single-quoted constant
                new_line->text[j] = '\'';
                new_line->text[j+1] = value;
                new_line->text[j+2] = '\'';
                j += 3;
              }
              else {
                strcpy(new_line->text + j, "(char)");
                j += 6;
                new_line->text[j+0] = '0';
                new_line->text[j+1] = ((value/64) % 8) + 48;
                new_line->text[j+2] = ((value/8) % 8) + 48;
                new_line->text[j+3] = ((value/1) % 8) + 48;
                j += 4;
              }
              strcpy(new_line->text + j, ";");
            }
            if (!strcmp(codeline->assigned_variable->data_type->name, "int")) {
              // Determine sign and calculate number of chars required for the number
              int value = codeline->constant.i;
              int const_len = 0;
              char sign = '+';
              if (value < 0) {
                sign = '-';
                value = -value;
              }
              int working_value = value;
              while (working_value > 0){
                const_len++;
                working_value = working_value/10;
              }
              // for example: i = +252;

              while (j < indent_level*SPACES_PER_INDENT) {
                new_line->text[j] = ' ';
                j++;
              }
              strcpy(new_line->text + j, codeline->assigned_variable->name);
              j += strlen(codeline->assigned_variable->name);
              strcpy(new_line->text + j, " = ");
              j += 3;
              new_line->text[j] = sign;
              j += 1;
              // Add a leading zero to make sure it actually does something when zero is the number
              if (value == 0){
                new_line->text[j] = '0';
                j++;
              }
              int k = 0;
              while (value > 0){
                new_line->text[j + const_len - k - 1] = value%10 + 48;
                value = value/10;
                k++;
              }
              j += const_len;
              strcpy(new_line->text + j, ";\0");
            }
            break;
          case CODELINE_TYPE_POINTER_ASSIGNMENT:
              while (j < indent_level*SPACES_PER_INDENT) {
                new_line->text[j] = ' ';
                j++;
              }
              for (i = 0; i < codeline->assigned_variable_reference_count; i++) {
                new_line->text[j] = '*';
                j++;
              }
              strcpy(new_line->text + j, codeline->assigned_variable->name);
              j += strlen(codeline->assigned_variable->name);
              strcpy(new_line->text + j, " = ");
              j += 3;
              if (codeline->arg0_reference_count < 0) {
                new_line->text[j] = '&';
                j++;
              }
              for (i = 0; i < codeline->arg0_reference_count; i++) {
                new_line->text[j] = '*';
                j++;
              }
              strcpy(new_line->text + j, codeline->args[0]->name);
              j += strlen(codeline->args[0]->name);
              
              if (codeline->args[1]) {
                strcpy(new_line->text + j, " + ");
                j += 3;
                strcpy(new_line->text + j, codeline->args[1]->name);
                j += strlen(codeline->args[1]->name);
              }
              strcpy(new_line->text + j, ";\0");
            break;
          case CODELINE_TYPE_IF:
            while (j < indent_level*SPACES_PER_INDENT) {
              new_line->text[j] = ' ';
              j++;
            }
            strcpy(new_line->text + j, "if (");
            j += 4;
            strcpy(new_line->text + j, codeline->args[0]->name);
            j += strlen(codeline->args[0]->name);
            cond_string = get_condition_string(codeline->condition);
            strcpy(new_line->text + j, cond_string);
            j += strlen(cond_string);
            strcpy(new_line->text + j, codeline->args[1]->name);
            j += strlen(codeline->args[1]->name);
            strcpy(new_line->text + j, "){");
            
            indent_level++;
            break;
          case CODELINE_TYPE_WHILE:
            while (j < indent_level*SPACES_PER_INDENT) {
              new_line->text[j] = ' ';
              j++;
            }
            strcpy(new_line->text + j, "while (");
            j += 7;
            strcpy(new_line->text + j, codeline->args[0]->name);
            j += strlen(codeline->args[0]->name);
            cond_string = get_condition_string(codeline->condition);
            strcpy(new_line->text + j, cond_string);
            j += strlen(cond_string);
            strcpy(new_line->text + j, codeline->args[1]->name);
            j += strlen(codeline->args[1]->name);
            strcpy(new_line->text + j, "){");
            
            indent_level++;
            break;
          case CODELINE_TYPE_BLOCK_END:
            indent_level--;
            while (j < indent_level*SPACES_PER_INDENT) {
              new_line->text[j] = ' ';
              j++;
            }
            strcpy(new_line->text + j, "}");
            break;
          case CODELINE_TYPE_RETURN:
            while (j < indent_level*SPACES_PER_INDENT) {
              new_line->text[j] = ' ';
              j++;
            }
            strcpy(new_line->text + j, "return ");
            j += 7;
            strcpy(new_line->text + j, codeline->assigned_variable->name);
            j += strlen(codeline->assigned_variable->name);
            strcpy(new_line->text + j, ";");
            break;
        }
        codeline = codeline->next;
      }
      
      // Add trailing bracket
      indent_level--;
      new_line = malloc(sizeof(struct GeneratedLine_T));
      *last_link = new_line;
      last_link = &(new_line->next);
      new_line->text[0] = '}';
      new_line->text[1] = '\0';
      // Empty trailing line
      new_line = malloc(sizeof(struct GeneratedLine_T));
      *last_link = new_line;
      last_link = &(new_line->next);
      new_line->text[0] = '\0';
    }
    
    function = function->next; // Jump to the next function
  }
  *last_link = NULL;
  return start_line;
}


void print_generated_lines(struct GeneratedLine_T* generated_code) {
  while (generated_code) {
    printf("%s\n",generated_code->text);
    generated_code = generated_code->next;
  }
}

void print_function(struct Function_T * function) {
  struct GeneratedLine_T * generated = generate_c(function);
  struct GeneratedLine_T * tofree;
  char * text = concat_generated_lines(generated, -1);
  printf("%s", text);
  free(text);
  while (generated) {
    tofree = generated;
    generated = generated->next;
    free(tofree);
  }
}

char * print_function_to_buffer(struct Function_T * function) {
  struct GeneratedLine_T * generated = generate_c(function);
  struct GeneratedLine_T * tofree;
  char * text = concat_generated_lines(generated, -1);
  while (generated) {
    tofree = generated;
    generated = generated->next;
    free(tofree);
  }
  return text;
}

void print_function_limited(struct Function_T * function) {
  struct GeneratedLine_T * generated = generate_c(function);
  struct GeneratedLine_T * tofree;
  char * text = concat_generated_lines(generated, 80);
  printf("%s", text);
  free(text);
  while (generated) {
    tofree = generated;
    generated = generated->next;
    free(tofree);
  }
}
