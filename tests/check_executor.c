#include "c_types.h"
#include "executor.h"

struct Function_T * build_example_tree() {
  struct DataType_T dt_i = {BASICTYPE_INT, 0};
  struct DataType_T dt_c = {BASICTYPE_CHAR, 0};
  struct Variable_T * vars = malloc(sizeof(struct Variable_T)*5);
  vars[0] = (struct Variable_T){.name="a", .is_arg=0, .data_type=dt_i, .next=&(vars[1]), .prev=NULL};
  vars[1] = (struct Variable_T){.name="b", .is_arg=1, .data_type=dt_i, .next=&(vars[2]), .prev=&(vars[0])};
  vars[2] = (struct Variable_T){.name="c", .is_arg=1, .data_type=dt_i, .next=&(vars[3]), .prev=&(vars[1])};
  vars[3] = (struct Variable_T){.name="d", .is_arg=0, .data_type=dt_c, .next=&(vars[4]), .prev=&(vars[2])};
  vars[4] = (struct Variable_T){.name="e", .is_arg=0, .data_type=dt_i, .next=NULL      , .prev=&(vars[3])};
  struct Function_T * funcs = malloc(sizeof(struct Function_T)*3);
  funcs[0] = (struct Function_T){.type=FUNCTION_TYPE_BASIC_OP, .name="+", .next=&(funcs[1]), .prev=NULL,};
  funcs[1] = (struct Function_T){.type=FUNCTION_TYPE_BUILTIN, .name="putchar", .next=&(funcs[2]), .prev=&(funcs[0])};
  funcs[2] = (struct Function_T){.type=FUNCTION_TYPE_CUSTOM, .name="main", .return_datatype=dt_i, .next=NULL, .prev=&(funcs[1]), .first_var=&(vars[0]), .last_var=&(vars[4]), .function_arguments={&(vars[1]), &(vars[2])}};
  int codeline_count = 9;
  struct CodeLine_T * codelines = malloc(sizeof(struct CodeLine_T)*codeline_count);
  codelines[0] = (struct CodeLine_T){.type=CODELINE_TYPE_CONSTANT_ASSIGNMENT, .assigned_variable=&(vars[4]), .constant={.i=48}, .parent_function=&(funcs[2])};
  codelines[1] = (struct CodeLine_T){.type=CODELINE_TYPE_FUNCTION_CALL, .assigned_variable=&(vars[0]), .target_function=&(funcs[0]), .function_arguments={&(vars[1]), &(vars[2])}, .parent_function=&(funcs[2])};
  codelines[2] = (struct CodeLine_T){.type=CODELINE_TYPE_FUNCTION_CALL, .assigned_variable=&(vars[0]), .target_function=&(funcs[0]), .function_arguments={&(vars[0]), &(vars[4])}, .parent_function=&(funcs[2])};
  codelines[3] = (struct CodeLine_T){.type=CODELINE_TYPE_FUNCTION_CALL, .assigned_variable=&(vars[3]), .target_function=&(funcs[1]), .function_arguments={&(vars[0]), NULL}, .parent_function=&(funcs[2])};
  codelines[4] = (struct CodeLine_T){.type=CODELINE_TYPE_IF, .function_arguments={&(vars[1]), &(vars[2])}, .block_other_end=&(codelines[8]), .condition=CONDITION_GREATER_THAN, .parent_function=&(funcs[2])};
  codelines[5] = (struct CodeLine_T){.type=CODELINE_TYPE_FUNCTION_CALL, .assigned_variable=&(vars[3]), .target_function=&(funcs[1]), .function_arguments={&(vars[0]), NULL}, .parent_function=&(funcs[2])};
  codelines[6] = (struct CodeLine_T){.type=CODELINE_TYPE_CONSTANT_ASSIGNMENT, .assigned_variable=&(vars[3]), .constant={.c='h'}, .parent_function=&(funcs[2])};
  codelines[7] = (struct CodeLine_T){.type=CODELINE_TYPE_FUNCTION_CALL, .assigned_variable=&(vars[3]), .target_function=&(funcs[1]), .function_arguments={&(vars[3]), NULL}, .parent_function=&(funcs[2])};
  codelines[8] = (struct CodeLine_T){.type=CODELINE_TYPE_BLOCK_END, .prev=&(codelines[3]), .next=NULL, .parent_function=&(funcs[2]), .block_other_end=&(codelines[4])};
  for (int i = 0; i < codeline_count; i++) {
    codelines[i].next = codelines + i + 1;
    codelines[i].prev = codelines + i - 1;
  }
  codelines[0].prev = NULL;
  codelines[codeline_count-1].next = NULL;

  funcs[2].first_codeline = &(codelines[0]);
  funcs[2].last_codeline = &(codelines[codeline_count-1]);
  return &(funcs[0]);
}

void parse_interpret_test(char * fname) {
  FILE * f = fopen(fname, "r");
  char buffer[1000];
  int i = 0;
  while (buffer[i] != EOF) {
    i++;
    buffer[i] = fgetc(f);
  }
  struct Function_T * context = build_context();
  struct Function_T * main_func = parse_source_text(buffer, context);
  main_func->prev = context->next;
  context->next->next = main_func;
  print_generated_lines(generate_c(main_func));
  union ExecutorValue_T interpret_args[FUNCTION_ARG_COUNT] = {{.i=2}, {.i=4}};
  interpret_function(main_func, interpret_args, NULL);
  printf("\n");
}


int main() {
  struct Function_T * first_func = build_example_tree();
  struct GeneratedLine_T * generated_code = generate_c(first_func);
  print_generated_lines(generated_code);
  union ExecutorValue_T interpret_args[FUNCTION_ARG_COUNT] = {{.i=2}, {.i=4}};
  //for (int i = 0; i < 100000000; i++){
    interpret_function(first_func->next->next, interpret_args, NULL);
  //}
  printf("Did it!");
}
