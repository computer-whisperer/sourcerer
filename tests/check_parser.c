void parser_loopback_test() {
  struct Function_T * start_func = build_example_tree();
  struct GeneratedLine_T * generated_start_func = generate_c(start_func);
  print_generated_lines(generated_start_func);
  char * start_func_text = concat_generated_lines(generated_start_func, -1);
  printf("Starting parse\n");
  struct Function_T * funcs = malloc(sizeof(struct Function_T)*3);
  funcs[0] = (struct Function_T){.type=FUNCTION_TYPE_BASIC_OP, .name="+", .next=&(funcs[1]), .prev=NULL,};
  funcs[1] = (struct Function_T){.type=FUNCTION_TYPE_BUILTIN, .name="putchar", .next=NULL, .prev=&(funcs[0])};
  struct Function_T * second_func = parse_source_text(start_func_text, &(funcs[0]));
  printf("Finished parse\n");
  struct GeneratedLine_T * generated_second_func = generate_c(second_func);
  char * second_func_text = concat_generated_lines(generated_second_func, -1);
  print_generated_lines(generated_second_func);
  if (strcmp(start_func_text, second_func_text))
    printf("\n\nEPIC FAIL!\n\n");
  else
    printf("\n\nSUCCESS!\n\n");
}
