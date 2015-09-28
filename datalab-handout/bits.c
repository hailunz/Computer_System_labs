/* 
 * CS:APP Data Lab 
 * 
 * Hailun Zhu-hailunz 
 * bits.c - Source file with your solutions to the Lab.
 *          This is the file you will hand in to your instructor.
 *
 * WARNING: Do not include the <stdio.h> header; it confuses the dlc
 * compiler. You can still use printf for debugging without including
 * <stdio.h>, although you might get a compiler warning. In general,
 * it's not good practice to ignore compiler warnings, but in this
 * case it's OK.  
 */

#if 0
/*
 * Instructions to Students:
 *
 * STEP 1: Read the following instructions carefully.
 */

You will provide your solution to the Data Lab by
editing the collection of functions in this source file.

INTEGER CODING RULES:
 
  Replace the "return" statement in each function with one
  or more lines of C code that implements the function. Your code 
  must conform to the following style:
 
  int Funct(arg1, arg2, ...) {
      /* brief description of how your implementation works */
      int var1 = Expr1;
      ...
      int varM = ExprM;

      varJ = ExprJ;
      ...
      varN = ExprN;
      return ExprR;
  }

  Each "Expr" is an expression using ONLY the following:
  1. Integer constants 0 through 255 (0xFF), inclusive. You are
      not allowed to use big constants such as 0xffffffff.
  2. Function arguments and local variables (no global variables).
  3. Unary integer operations ! ~
  4. Binary integer operations & ^ | + << >>
    
  Some of the problems restrict the set of allowed operators even further.
  Each "Expr" may consist of multiple operators. You are not restricted to
  one operator per line.

  You are expressly forbidden to:
  1. Use any control constructs such as if, do, while, for, switch, etc.
  2. Define or use any macros.
  3. Define any additional functions in this file.
  4. Call any functions.
  5. Use any other operations, such as &&, ||, -, or ?:
  6. Use any form of casting.
  7. Use any data type other than int.  This implies that you
     cannot use arrays, structs, or unions.

 
  You may assume that your machine:
  1. Uses 2s complement, 32-bit representations of integers.
  2. Performs right shifts arithmetically.
  3. Has unpredictable behavior when shifting an integer by more
     than the word size.

EXAMPLES OF ACCEPTABLE CODING STYLE:
  /*
   * pow2plus1 - returns 2^x + 1, where 0 <= x <= 31
   */
  int pow2plus1(int x) {
     /* exploit ability of shifts to compute powers of 2 */
     return (1 << x) + 1;
  }

  /*
   * pow2plus4 - returns 2^x + 4, where 0 <= x <= 31
   */
  int pow2plus4(int x) {
     /* exploit ability of shifts to compute powers of 2 */
     int result = (1 << x);
     result += 4;
     return result;
  }

FLOATING POINT CODING RULES

For the problems that require you to implent floating-point operations,
the coding rules are less strict.  You are allowed to use looping and
conditional control.  You are allowed to use both ints and unsigneds.
You can use arbitrary integer and unsigned constants.

You are expressly forbidden to:
  1. Define or use any macros.
  2. Define any additional functions in this file.
  3. Call any functions.
  4. Use any form of casting.
  5. Use any data type other than int or unsigned.  This means that you
     cannot use arrays, structs, or unions.
  6. Use any floating point data types, operations, or constants.


NOTES:
  1. Use the dlc (data lab checker) compiler (described in the handout) to 
     check the legality of your solutions.
  2. Each function has a maximum number of operators (! ~ & ^ | + << >>)
     that you are allowed to use for your implementation of the function. 
     The max operator count is checked by dlc. Note that '=' is not 
     counted; you may use as many of these as you want without penalty.
  3. Use the btest test harness to check your functions for correctness.
  4. Use the BDD checker to formally verify your functions
  5. The maximum number of ops for each function is given in the
     header comment for each function. If there are any inconsistencies 
     between the maximum ops in the writeup and in this file, consider
     this file the authoritative source.

/*
 * STEP 2: Modify the following functions according the coding rules.
 * 
 *   IMPORTANT. TO AVOID GRADING SURPRISES:
 *   1. Use the dlc compiler to check that your solutions conform
 *      to the coding rules.
 *   2. Use the BDD checker to formally verify that your solutions produce 
 *      the correct answers.
 */


#endif
/* 
 * evenBits - return word with all even-numbered bits set to 1
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 8
 *   Rating: 1
 */
int evenBits(void) {
	/*shifting Ox55 to get the 32 bits word.*/
	int x=0x55;
	int a=x+(x<<8);
	return a+(a<<16);
}
/* 
 * isEqual - return 1 if x == y, and 0 otherwise 
 *   Examples: isEqual(5,5) = 1, isEqual(4,5) = 0
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 5
 *   Rating: 2
 */
int isEqual(int x, int y) {
	/* use !(x^y) */
	 return !(x^y);
}
/* 
 * byteSwap - swaps the nth byte and the mth byte
 *  Examples: byteSwap(0x12345678, 1, 3) = 0x56341278
 *            byteSwap(0xDEADBEEF, 0, 2) = 0xDEEFBEAD
 *  You may assume that 0 <= n <= 3, 0 <= m <= 3
 *  Legal ops: ! ~ & ^ | + << >>
 *  Max ops: 25
 *  Rating: 2
 */
int byteSwap(int x, int n, int m) {
	/*shift 0xFF to take the swaped bytes and then swap,x'=n'|m'|(x-n-m)*/
	int nMove=(n<<3);
	int mMove=(m<<3);
	int shift=(0xFF<<nMove)|(0xFF<<mMove);
	int xswap=x&shift;
	int nswap=((xswap>>nMove)&0xFF)<<mMove;
	int mswap=((xswap>>mMove)&0xFF)<<nMove;
	int result=(x&(~shift))|nswap|mswap;
	return result;
}
/* 
 * rotateRight - Rotate x to the right by n
 *   Can assume that 0 <= n <= 31
 *   Examples: rotateRight(0x87654321,4) = 0x18765432
 *   Legal ops: ~ & ^ | + << >>
 *   Max ops: 25
 *   Rating: 3 
 */
int rotateRight(int x, int n) {
	/* using left shift and right shift,
	 * have to remove the remained part of right-rotated x, 
	 * in terms of its sign */
	int u=x;
	int leftmove=~n+33;
	return  (u<<leftmove)|((x>>n)&(~((~0)<<leftmove)));
	
}
/* 
 * logicalNeg - implement the ! operator using any of 
 *              the legal operators except !
 *   Examples: logicalNeg(3) = 0, logicalNeg(0) = 1
 *   Legal ops: ~ & ^ | + << >>
 *   Max ops: 12
 *   Rating: 4 
 */
int logicalNeg(int x) {
	/*if x=0: ~x+1==x;special case is 0x80000000; 
	 * so only when x,~x+1 s' signs are the same and the sign is 0, return 1*/
	int sign=x>>31;
	return ~(sign^((~x+1)>>31))&(~sign)&1;
}
/* 
 * TMax - return maximum two's complement integer 
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 4
 *   Rating: 1
 */
int tmax(void) {
	/* ~(0x7FFFFFFF)=0x10000000*/
	return ~(1<<31);
}
/* 
 * sign - return 1 if positive, 0 if zero, and -1 if negative
 *  Examples: sign(130) = 1
 *            sign(-23) = -1
 *  Legal ops: ! ~ & ^ | + << >>
 *  Max ops: 10
 *  Rating: 2
 */
int sign(int x) {
	/* x>0 sign=0,!!x=1; x=0 sign=0,!!x=0; x<0 sign=-1,!!x=1;*/
	int sign=x>>31;
	return sign|(!!x);
}
/* 
 * isGreater - if x > y  then return 1, else return 0 
 *   Example: isGreater(4,5) = 0, isGreater(5,4) = 1
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 24
 *   Rating: 3
 */
int isGreater(int x, int y) {
	/*compare the sign of x,y,subSign; x>y,subSign>=0;*/
	int xSign=x>>31;  
	int ySign=y>>31;
	int subSign=(x+(~y))>>31;
	return (((!(xSign^ySign))&(~subSign))|((~xSign)&ySign))&1;
}
/* 
 * subOK - Determine if can compute x-y without overflow
 *   Example: subOK(0x80000000,0x80000000) = 1,
 *            subOK(0x80000000,0x70000000) = 0, 
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 20
 *   Rating: 3
 */
int subOK(int x, int y) {
	/*return 1: only when x,y have same signs or (x-y),x have same signs*/
	int xSign=x>>31;
	int ySign=y>>31;
	int subSign=(x+(~y)+1)>>31;
	int result=(!(xSign^ySign))|(!(subSign^xSign));
	return result;
}
/*
 * satAdd - adds two numbers but when positive overflow occurs, returns
 *          maximum possible value, and when negative overflow occurs,
 *          it returns minimum possible value.
 *   Examples: satAdd(0x40000000,0x40000000) = 0x7fffffff
 *             satAdd(0x80000000,0xffffffff) = 0x80000000
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 30
 *   Rating: 4
 */
int satAdd(int x, int y) {
	/* posOverflow:x>0,y>0,but sum<0;negOverflow: x<0,y<0,but sum>0; */
	int maxNum=0x1<<31;
	int xSign=x>>31;
	int ySign=y>>31;
	int sumSign=(x+y)>>31;
	int posOver=(~xSign)&(~ySign)&sumSign;
	int negOver=xSign&ySign&(~sumSign);
	return (posOver&(~maxNum))|(negOver&maxNum)|((~(posOver|negOver))&(x+y));
}
/* howManyBits - return the minimum number of bits required to represent x in
 *             two's complement
 *  Examples: howManyBits(12) = 5
 *            howManyBits(298) = 10
 *            howManyBits(-5) = 4
 *            howManyBits(0)  = 1
 *            howManyBits(-1) = 1
 *            howManyBits(0x80000000) = 32
 *  Legal ops: ! ~ & ^ | + << >>
 *  Max ops: 90
 *  Rating: 4
 */
int howManyBits(int x) {
	/* use dichotomy to count the bits.~x and x have same bits.
	 * when x is negitive, use ~x to count bits.*/
	int a ;
	int xSign=x>>31;
    int bit0,bit1,bit2,bit3,bit4,bit5,bitNum;
	a=((~xSign)&x)|((xSign)&(~x));
	bit5=!!(a>>15);
	a=a>>(bit5<<4);
	bit4=!!(a>>7);
	a=a>>(bit4<<3);
	bit3=!!(a>>3);
	a=a>>(bit3<<2);
	bit2=!!(a>>1);
	a=a>>(bit2<<1);
	bit1=!(!a);
	a=a>>bit1;
	bit0=!!a;
	bitNum=(bit5<<4)+(bit4<<3)+(bit3<<2)+(bit2<<1)+bit1+bit0+1;
	return bitNum;
}
/* 
 * float_half - Return bit-level equivalent of expression 0.5*f for
 *   floating point argument f.
 *   Both the argument and result are passed as unsigned int's, but
 *   they are to be interpreted as the bit-level representation of
 *   single-precision floating point values.
 *   When argument is NaN, return argument
 *   Legal ops: Any integer/unsigned operations incl. ||, &&. also if, while
 *   Max ops: 30
 *   Rating: 4
 */
unsigned float_half(unsigned uf) {
	unsigned exp=uf&0x7f800000;
	unsigned frac=uf&0x7fffff;
	unsigned ufSign=uf&0x80000000;
	/* case1:NaN or infinity, return uf */
	if(exp==0x7f800000)
	return uf;
	/* case2:normalized and exp!=1: exp-1;*/
	else if (exp>0x800000)
	return (ufSign|(exp-0x800000)|frac);
	/* case3:exp=1 or demormalized: frac>>1, and rounding */
	else {
		if (exp==0x800000)
		frac=frac|(1<<23);
		if ((frac&3)==3)
		frac++;
		frac=frac>>1;
		return (ufSign|frac);
	}
}
/* 
 * float_f2i - Return bit-level equivalent of expression (int) f
 *   for floating point argument f.
 *   Argument is passed as unsigned int, but
 *   it is to be interpreted as the bit-level representation of a
 *   single-precision floating point value.
 *   Anything out of range (including NaN and infinity) should return
 *   0x80000000u.
 *   Legal ops: Any integer/unsigned operations incl. ||, &&. also if, while
 *   Max ops: 30
 *   Rating: 4
 */
int float_f2i(unsigned uf) {
	unsigned exp=(uf<<1)>>24;
	unsigned frac=uf&0x7fffff;
	unsigned bias=127;
	unsigned ufSign=uf>>31;
	unsigned Exp=exp-bias;
	unsigned bin;
	/* case1: E<0, return 0. */
	 if (exp<bias)
		return 0;
	/* case2: NaN, infinity, and overflow. */
	if ((exp==0xff)||(Exp>=31))
		return 0x80000000u;
	/* case3: f2i.*/
	if (Exp>22)
		bin=frac<<(Exp-23);
	else
		bin=frac>>(23-Exp);
		bin+=1<<Exp;
	if(ufSign)
		bin=-bin;
	return bin;
}
