#define F (1 << 14) //fixed-point 1 in 17.14 form
#define INT_MAX ((1 << 31) - 1) //maximum integer
#define INT_MIN (-(1 << 31)) //minimum integer

//x and y are fixed-point numbers
//n is an integer
int int2fp (int n);		//convert int to fp
int fp2int (int x);		//convert fp to int
int fp2int_round (int x);	//convert fp to int (rounded)
int add_fp (int x, int y);	//add two fps
int add_both (int x, int n);	//add fp and int
int sub_fp (int x, int y);	//subtract y from x
int sub_both (int x, int n);	//subtract int from fp
int mult_fp (int x, int y);	//multiply two fps
int mult_both (int x, int n);	//multiply fp and int
int div_fp (int x, int y);	//divide y from x
int div_both (int x, int n);	//divide int from fp

int
int2fp (int n) {
	return n * F;
}

int
fp2int (int x) {
	return x / F;
}

int
fp2int_round (int x) {
	if (x >= 0)
		return (x + F / 2) / F;
	else
		return (x - F / 2) / F;
}

int
add_fp (int x, int y) {
	return x + y;
}

int
add_both (int x, int n) {
	return x + n * F;
}

int
sub_fp (int x, int y) {
	return x - y;
}

int
sub_both (int x, int n) {
	return x - n * F;
}

int
mult_fp (int x, int y) {
	return ((int64_t) x) * y / F;
}

int
mult_both (int x, int n) {
	return x * n;
}

int
div_fp (int x, int y) {
	return ((int64_t) x) * F / y;
}

int
div_both (int x, int n) {
	return x / n;
}
