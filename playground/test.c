
int main() {
  hi();
}

int hi(){
  int result;
  int i;
  int char_spacing;
  int nine;
  int ten;
  int zero;
  int sub_value;
  int one;
  int value;
  value = 4;
  char_spacing = 48;
  nine = 9;
  ten = 10;
  zero = 0;
  i = 1;
  one = 1;
  result = 1;
  while(i <= value){
    result = result * i;
    i = i + one;
  }
  while(result > zero){
    sub_value = result + zero;
    while (sub_value > nine) {
      sub_value = sub_value - ten;
    }
    sub_value = sub_value + char_spacing;
    i = putchar(sub_value);
    result = result / ten;
  }
}
