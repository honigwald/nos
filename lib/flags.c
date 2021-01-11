#include "flags.h"

int timer_irq_print_f = 0;
int context_change_print_f = 0;

void toggle_flag(int *flag)
{
	*flag = (*flag) ? 0 : 1;
}
