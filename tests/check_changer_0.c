void test_code_changer() {
  global_context = build_context();
  struct Function_T * func = load_from_file("test.c");
  struct GeneratedLine_T * gen_one = generate_c(func);
  char * buff_one = concat_generated_lines(gen_one, -1);
  print_function(func);
  struct Change_T * change = find_new_change(func, global_context);
  apply_change(func, change);
  print_function(func);
  apply_change(func, change->inverse);
  struct GeneratedLine_T * gen_two = generate_c(func);
  char * buff_two = concat_generated_lines(gen_two, -1);
  print_function(func);
  if (strcmp(buff_one, buff_two))
    printf("\n\nEPIC FAIL!\n\n");
  else
    printf("\n\nSUCCESS!\n\n");
  
}
