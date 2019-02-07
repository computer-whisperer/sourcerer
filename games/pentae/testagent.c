int step(int * board, int* move_out_x, int* move_out_y, int board_width){
	int works;
	int x;
	int y;
	int zero;
	int one;
	int * cell;
	int offset;
	works = 1;
	zero = 0;
	one = 1;
	x = -1;
	y = 0;
	while (works != zero) {
		x = x + one;
		if (x == board_width) {
			y = y + one;
			x = zero;
		}
		offset = x * board_width;
		offset = offset + y;
		cell = board + offset;
		works = *cell;
	}
	*move_out_x = x;
	*move_out_y = y;
	return zero;
}
