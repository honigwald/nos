#include "printf.h"

#define MAX_SEQ_LEN 10

static int pow(int base, int exp)
{
	if (exp < 0)
		return 0; 
	int i;
	int x = 1;
	for (i = 0; i < exp; ++i) {
		x *= base;
	}

	return x;
}

/**
 * escape_sequence_config() - processes '%' escape sequence and sets format
 * configuration accordingly.
 *
 * @fmt: pointer to '%' character of current escape sequence
 * @sequence_length: configures minimum amount of chars to print
 * @padding_flag: configures padding character
 * @hex_prefix_flag: configures whether "0x" should be printed
 * 
 * fmt is expected to point to '%'
 *
 * format insert value length specification number max digit amount is 10
 *
 * if that digit amount is violated, specific format will be ignored
 *
 * Return: fmt points to last char in esc sequ (like i or c) or at first if
 * requested format is unknown.
 */
static char *escape_sequence_config(char *fmt,\
				    int *sequence_length,\
				    int *padding_flag,\
				    int *hex_prefix_flag)
{
	char *fmt_copy = fmt+1;

	if (*fmt_copy == '#') {
		*hex_prefix_flag = 1;
		fmt_copy++;
	}
	if(*fmt_copy == '0') {
		*padding_flag = 1;
		fmt_copy++;
	}

	char length_val_array[MAX_SEQ_LEN];
	int i;
	for (i = 0; i < MAX_SEQ_LEN; ++i)
		length_val_array[i] = 0;

	int last_write = -1;	// length_val_array index of last write
	i = 0;
	for (; (*fmt_copy >= 48) && (*fmt_copy <= 57); ++i) {
		if (i < MAX_SEQ_LEN) {
			length_val_array[i] = *fmt_copy;
			last_write++;
		}
		fmt_copy++;
	}

	if (i < MAX_SEQ_LEN && last_write > -1) {	
		// max limit was not violated ==> process sequence_array
		*sequence_length = 0;
		for(i = last_write; i >= 0; --i) {
			*sequence_length += (length_val_array[i] - 48)\
				* pow(10, last_write-i);
		}
	} else {
		*sequence_length = -1;
	}

	switch (*(fmt_copy)) {
		case 'c':
		case 's':
		case 'p':
		case 'x':
		case 'i':
		case 'u':
			fmt = fmt_copy;
			break;
		default:	// unknown format request
			*sequence_length = -1;
			padding_flag = 0;
			hex_prefix_flag = 0;
			break;
	}

	return fmt;
}

/*
 * process_[*]() - set of functions to outsource switch case procedures
 *
 * calculates padding and translates argument values to output string
 *
 * depending on the specific case hex_prefix_flag will be ignored
 */

static void process_c(int arg, int sequence_length, int padding_flag)
{
	if (sequence_length > 1) {
		char padding_char = ' ';
		if (padding_flag == 1)
			padding_char = '0';

		int i;
		for (i = 0; i < sequence_length-1; ++i)
			uart_putc(padding_char);
	}

	uart_putc((char)arg);

	return;
}

static void process_s(char *arg, int sequence_length, int padding_flag)
{
	int string_length = 0;
	for (string_length = 0; arg[string_length] != '\0'; ++string_length)
		continue;

	if (sequence_length > string_length) {
		char padding_char = ' ';
		if (padding_flag == 1)
			padding_char = '0';

		int i;
		for (i = 0; i < sequence_length-string_length; ++i)
			uart_putc(padding_char);
	}

	char *s = (char*)arg;
	while(*s) {
		uart_putc(*s);
		s++;
	}

	return;
}

static void process_x(unsigned int arg, int sequence_length, int padding_flag,\
		      int hex_prefix_flag)
{
	if (hex_prefix_flag == 1) {
		uart_putc('0');
		uart_putc('x');
	}

	char hexnum[] = {48,48,48,48,48,48,48,48};
	int hex_length = 0;
	int i = 0;
	while (arg > 0) {
		char digit = (arg % 16) + 48;
		if (digit >= 48 && digit <= 57) {
			hexnum[hex_length] = digit;
		} else {
			switch (digit) {
			case 58:
				hexnum[hex_length] = 'a';
				break;
			case 59:
				hexnum[hex_length] = 'b';
				break;
			case 60:
				hexnum[hex_length] = 'c';
				break;
			case 61:
				hexnum[hex_length] = 'd';
				break;
			case 62:
				hexnum[hex_length] = 'e';
				break;
			case 63:
				hexnum[hex_length] = 'f';
				break;
			default:
				hexnum[hex_length] = '?';
				break;
			}
		}
		hex_length++;
		arg /= 16;
	}

	if (sequence_length > hex_length) {
		char padding_char = ' ';
		if (padding_flag == 1)
			padding_char = '0';
		for (i = 0; i < sequence_length-hex_length; ++i)
			uart_putc(padding_char);
	}

	for (i = hex_length-1;i >= 0; --i) {
		uart_putc(hexnum[i]);
	}
	return;
}

static void process_i(int arg, int sequence_length, int padding_flag)
{
	char number[11] = {48,48,48,48,48,48,48,48,48,48,48};
	int number_length = 0;
	int i = 0;
	if (arg < 0) {
		unsigned int positiv_arg = arg * -1;
		while (positiv_arg > 0) {
			char digit = (positiv_arg % 10) + 48;
			number[number_length] = digit;
			number_length++;
			positiv_arg /= 10;
		}
		number[number_length] = '-';
		number_length++;
	} else {
		while (arg > 0) {
			char digit = (arg % 10) + 48;
			number[number_length] = digit;
			number_length++;
			arg /= 10;
		}
	}

	if (number_length == 0) number_length = 1;

	if (sequence_length > number_length) {
		char padding_char = ' ';
		if (padding_flag == 1)
			padding_char = '0';
		for (i = 0; i < sequence_length-number_length; ++i)
			uart_putc(padding_char);
	}

	for (i = number_length-1;i >= 0; --i)
		uart_putc(number[i]);
}

static void process_u(unsigned int arg, int sequence_length, int padding_flag)
{
	char number[10] = {48,48,48,48,48,48,48,48,48,48};
	int number_length = 0;
	int i = 0;
	while (arg > 0) {
		char digit = (arg % 10) + 48;
		number[number_length] = digit;
		number_length++;
		arg /= 10;
	}

	if (number_length == 0) number_length = 1;

	if (sequence_length > number_length) {
		char padding_char = ' ';
		if (padding_flag == 1)
			padding_char = '0';
		for (i = 0; i < sequence_length-number_length; ++i)
			uart_putc(padding_char);
	}

	for (i = number_length-1;i >= 0; --i)
		uart_putc(number[i]);
}

/**
 * printf() - subset of the famous stdio.h function
 *
 * @fmt: format string to be printed w/ '%'-escape sequences
 * 
 * '%'-escape sequence format: ^%[#]{0,1}[0]{0,1}[0-9]*[c,s,p,x,i,u]{1}$
 *
 * Return: currently only returns 1
 */
int printf(char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);

	while (*fmt) {
		int sequence_length = 0;// length of inserted format string
		int padding_flag = 0;	// flag for '0' char instead of ' '
		int hex_prefix_flag = 0;// flag for "0x" prefix

		if (*fmt == '%') {
			fmt = escape_sequence_config(fmt,\
						     &sequence_length,\
						     &padding_flag,\
						     &hex_prefix_flag);

			switch (*fmt) {
			case 'c': // unsigned char
				process_c(va_arg(ap, int),\
					  sequence_length,\
					  padding_flag);
				break;
			case 's': // string
				process_s(va_arg(ap, char*),\
					  sequence_length,\
					  padding_flag);
				break;
			case 'p': // addr as hex number - 10 char incl. "0x"
				process_x(va_arg(ap, unsigned int), 8, 1, 1);
				break;
			case 'x': // unsigned int as hex number
				process_x(va_arg(ap, unsigned int),\
					  sequence_length,\
					  padding_flag,\
					  hex_prefix_flag);
				break;
			case 'i': // signed int as dec number
				process_i(va_arg(ap, int),\
					  sequence_length,\
					  padding_flag);
				break;
			case 'u': // unsigned int as dec number
				process_u(va_arg(ap, unsigned int),\
					  sequence_length,\
					  padding_flag);
				break;
			default:
				uart_putc(*fmt);
				break;
			}
		} else {
			uart_putc(*fmt);
		}
		fmt++;
	}

	va_end(ap);
	return 1;
}
