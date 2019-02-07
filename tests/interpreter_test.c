int main() {
	int i;
	int j;
	int q;
	int r;
	int one;
	char c;
	int * i_ptr;
	i_ptr = &i;
	one = 1;
	i = 0;
	j = 20;
	c = 'h';
	while (i < j) {
		r = *i_ptr;
		q = r + one;
		*i_ptr = q;
		c = putchar(c);
	}
}
