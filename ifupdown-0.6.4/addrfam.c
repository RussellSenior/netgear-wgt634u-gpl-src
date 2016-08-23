#line 430 "ifupdown.nw"
#include <stdlib.h>
#include "header.h"

#line 3606 "ifupdown.nw"
extern address_family addr_inet;
#line 3761 "ifupdown.nw"
extern address_family addr_inet6;
#line 3831 "ifupdown.nw"
extern address_family addr_ipx;

#line 435 "ifupdown.nw"
address_family *addr_fams[] = {
	
#line 3610 "ifupdown.nw"
&addr_inet, 
#line 3765 "ifupdown.nw"
&addr_inet6,
#line 3835 "ifupdown.nw"
&addr_ipx,
#line 437 "ifupdown.nw"
	NULL
};
