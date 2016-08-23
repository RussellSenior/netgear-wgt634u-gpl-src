#line 1013 "ifupdown.nw"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#line 1024 "ifupdown.nw"
#include "header.h"
#line 1227 "ifupdown.nw"
#include <errno.h>
#line 1361 "ifupdown.nw"
#include <ctype.h>
#line 1204 "ifupdown.nw"
static int get_line(char **result, size_t *result_len, FILE *f, int *line);
#line 1431 "ifupdown.nw"
static char *next_word(char *buf, char *word, int maxlen);
#line 1665 "ifupdown.nw"
static address_family *get_address_family(address_family *af[], char *name);
#line 1700 "ifupdown.nw"
static method *get_method(address_family *af, char *name);
#line 1761 "ifupdown.nw"
static int duplicate_if(interface_defn *ifa, interface_defn *ifb);
#line 1137 "ifupdown.nw"
interfaces_file *read_interfaces(char *filename) {
	
#line 1174 "ifupdown.nw"
FILE *f;
int line;
#line 1211 "ifupdown.nw"
char *buf = NULL;
size_t buf_len = 0;
#line 1411 "ifupdown.nw"
interface_defn *currif = NULL;
mapping_defn *currmap = NULL;
enum { NONE, IFACE, MAPPING } currently_processing = NONE;
#line 1422 "ifupdown.nw"
char firstword[80];
char *rest;
#line 1139 "ifupdown.nw"
	interfaces_file *defn;

	
#line 1158 "ifupdown.nw"
defn = malloc(sizeof(interfaces_file));
if (defn == NULL) {
	return NULL;
}
defn->max_autointerfaces = defn->n_autointerfaces = 0;
defn->autointerfaces = NULL;
defn->mappings = NULL;
defn->ifaces = NULL;
#line 1142 "ifupdown.nw"
	
#line 1179 "ifupdown.nw"
f = fopen(filename, "r");
if ( f == NULL ) return NULL;
line = 0;

#line 1144 "ifupdown.nw"
	while (
#line 1219 "ifupdown.nw"
get_line(&buf,&buf_len,f,&line)
#line 1144 "ifupdown.nw"
                                             ) {
		
#line 1458 "ifupdown.nw"
rest = next_word(buf, firstword, 80);
if (rest == NULL) continue; /* blank line */

if (strcmp(firstword, "mapping") == 0) {
	
#line 1507 "ifupdown.nw"
currmap = malloc(sizeof(mapping_defn));
if (currmap == NULL) {
	
#line 1890 "ifupdown.nw"
perror(filename);
return NULL;
#line 1510 "ifupdown.nw"
}
#line 1514 "ifupdown.nw"
currmap->max_matches = 0;
currmap->n_matches = 0;
currmap->match = NULL;

while((rest = next_word(rest, firstword, 80))) {
	if (currmap->max_matches == currmap->n_matches) {
		char **tmp;
		currmap->max_matches = currmap->max_matches * 2 + 1;
		tmp = realloc(currmap->match, 
			sizeof(*tmp) * currmap->max_matches);
		if (tmp == NULL) {
			currmap->max_matches = (currmap->max_matches - 1) / 2;
			
#line 1890 "ifupdown.nw"
perror(filename);
return NULL;
#line 1527 "ifupdown.nw"
		}
		currmap->match = tmp;
	}

	currmap->match[currmap->n_matches++] = strdup(firstword);
}
#line 1536 "ifupdown.nw"
currmap->script = NULL;

currmap->max_mappings = 0;
currmap->n_mappings = 0;
currmap->mapping = NULL;
#line 1544 "ifupdown.nw"
{
	mapping_defn **where = &defn->mappings;
	while(*where != NULL) {
		where = &(*where)->next;
	}
	*where = currmap;
	currmap->next = NULL;
}
#line 1463 "ifupdown.nw"
	currently_processing = MAPPING;
} else if (strcmp(firstword, "iface") == 0) {
	
#line 1596 "ifupdown.nw"
{
	
#line 1629 "ifupdown.nw"
char iface_name[80];
char address_family_name[80];
char method_name[80];

#line 1599 "ifupdown.nw"
	
#line 1617 "ifupdown.nw"
currif = malloc(sizeof(interface_defn));
if (!currif) {
	
#line 1890 "ifupdown.nw"
perror(filename);
return NULL;
#line 1620 "ifupdown.nw"
}

#line 1601 "ifupdown.nw"
	
#line 1635 "ifupdown.nw"
rest = next_word(rest, iface_name, 80);
rest = next_word(rest, address_family_name, 80);
rest = next_word(rest, method_name, 80);

if (rest == NULL) {
	
#line 1895 "ifupdown.nw"
fprintf(stderr, "%s:%d: too few parameters for iface line\n", filename, line);
return NULL;
#line 1641 "ifupdown.nw"
}

if (rest[0] != '\0') {
	
#line 1900 "ifupdown.nw"
fprintf(stderr, "%s:%d: too many parameters for iface line\n", filename, line);
return NULL;
#line 1645 "ifupdown.nw"
}

#line 1603 "ifupdown.nw"
	
#line 1651 "ifupdown.nw"
currif->iface = strdup(iface_name);
if (!currif->iface) {
	
#line 1890 "ifupdown.nw"
perror(filename);
return NULL;
#line 1654 "ifupdown.nw"
}
#line 1604 "ifupdown.nw"
	
#line 1669 "ifupdown.nw"
currif->address_family = get_address_family(addr_fams, address_family_name);
if (!currif->address_family) {
	
#line 1905 "ifupdown.nw"
fprintf(stderr, "%s:%d: unknown address type\n", filename, line);
return NULL;
#line 1672 "ifupdown.nw"
}
#line 1605 "ifupdown.nw"
	
#line 1704 "ifupdown.nw"
currif->method = get_method(currif->address_family, method_name);
if (!currif->method) {
	
#line 1910 "ifupdown.nw"
fprintf(stderr, "%s:%d: unknown method\n", filename, line);
return NULL;
#line 1707 "ifupdown.nw"
	return NULL; /* FIXME */
}
#line 1606 "ifupdown.nw"
	
#line 1727 "ifupdown.nw"
currif->automatic = 1;
currif->max_options = 0;
currif->n_options = 0;
currif->option = NULL;

#line 1608 "ifupdown.nw"
	
#line 1743 "ifupdown.nw"
{
	interface_defn **where = &defn->ifaces; 
	while(*where != NULL) {
		if (duplicate_if(*where, currif)) {
			
#line 1915 "ifupdown.nw"
fprintf(stderr, "%s:%d: duplicate interface\n", filename, line);
return NULL;
#line 1748 "ifupdown.nw"
		}
		where = &(*where)->next;
	}

	*where = currif;
	currif->next = NULL;
}
#line 1609 "ifupdown.nw"
}
#line 1466 "ifupdown.nw"
	currently_processing = IFACE;
} else if (strcmp(firstword, "auto") == 0) {
	
#line 1847 "ifupdown.nw"
while((rest = next_word(rest, firstword, 80))) {
	
#line 1854 "ifupdown.nw"
{
	int i;

	for (i = 0; i < defn->n_autointerfaces; i++) {
		if (strcmp(firstword, defn->autointerfaces[i]) == 0) {
			
#line 1920 "ifupdown.nw"
fprintf(stderr, "%s:%d: interface declared auto twice\n", filename, line);
return NULL;
#line 1860 "ifupdown.nw"
		}
	}
}
#line 1849 "ifupdown.nw"
	
#line 1866 "ifupdown.nw"
if (defn->n_autointerfaces == defn->max_autointerfaces) {
	char **tmp;
	defn->max_autointerfaces *= 2;
	defn->max_autointerfaces++;
	tmp = realloc(defn->autointerfaces, 
		sizeof(*tmp) * defn->max_autointerfaces);
	if (tmp == NULL) {
		
#line 1890 "ifupdown.nw"
perror(filename);
return NULL;
#line 1874 "ifupdown.nw"
	}
	defn->autointerfaces = tmp;
}

defn->autointerfaces[defn->n_autointerfaces] = strdup(firstword);
defn->n_autointerfaces++;
#line 1850 "ifupdown.nw"
}
#line 1469 "ifupdown.nw"
	currently_processing = NONE;
} else {
	
#line 1476 "ifupdown.nw"
switch(currently_processing) {
	case IFACE:
		
#line 1781 "ifupdown.nw"
{
	int i;

	if (strlen (rest) == 0) {
		
#line 1940 "ifupdown.nw"
fprintf(stderr, "%s:%d: option with empty value\n", filename, line);
return NULL;
#line 1786 "ifupdown.nw"
	}

	if (strcmp(firstword, "up") != 0
	    && strcmp(firstword, "down") != 0
	    && strcmp(firstword, "pre-up") != 0
	    && strcmp(firstword, "post-down") != 0)
        {
		for (i = 0; i < currif->n_options; i++) {
			if (strcmp(currif->option[i].name, firstword) == 0) {
				
#line 1925 "ifupdown.nw"
fprintf(stderr, "%s:%d: duplicate option\n", filename, line);
return NULL;
#line 1796 "ifupdown.nw"
			}
		}
	}
}
#line 1808 "ifupdown.nw"
if (currif->n_options >= currif->max_options) {
	
#line 1830 "ifupdown.nw"
{
	variable *opt;
	currif->max_options = currif->max_options + 10;
	opt = realloc(currif->option, sizeof(*opt) * currif->max_options);
	if (opt == NULL) {
		
#line 1890 "ifupdown.nw"
perror(filename);
return NULL;
#line 1836 "ifupdown.nw"
	}
	currif->option = opt;
}
#line 1810 "ifupdown.nw"
}

currif->option[currif->n_options].name = strdup(firstword);
currif->option[currif->n_options].value = strdup(rest);

if (!currif->option[currif->n_options].name) {
	
#line 1890 "ifupdown.nw"
perror(filename);
return NULL;
#line 1817 "ifupdown.nw"
}

if (!currif->option[currif->n_options].value) {
	
#line 1890 "ifupdown.nw"
perror(filename);
return NULL;
#line 1821 "ifupdown.nw"
}

currif->n_options++;	
#line 1479 "ifupdown.nw"
		break;
	case MAPPING:
		
#line 1559 "ifupdown.nw"
if (strcmp(firstword, "script") == 0) {
	
#line 1569 "ifupdown.nw"
if (currmap->script != NULL) {
	
#line 1930 "ifupdown.nw"
fprintf(stderr, "%s:%d: duplicate script in mapping\n", filename, line);
return NULL;
#line 1571 "ifupdown.nw"
} else {
	currmap->script = strdup(rest);
}
#line 1561 "ifupdown.nw"
} else if (strcmp(firstword, "map") == 0) {
	
#line 1577 "ifupdown.nw"
if (currmap->max_mappings == currmap->n_mappings) {
	char **opt;
	currmap->max_mappings = currmap->max_mappings * 2 + 1;
	opt = realloc(currmap->mapping, sizeof(*opt) * currmap->max_mappings);
	if (opt == NULL) {
		
#line 1890 "ifupdown.nw"
perror(filename);
return NULL;
#line 1583 "ifupdown.nw"
	}
	currmap->mapping = opt;
}
currmap->mapping[currmap->n_mappings] = strdup(rest);
currmap->n_mappings++;
#line 1563 "ifupdown.nw"
} else {
	
#line 1935 "ifupdown.nw"
fprintf(stderr, "%s:%d: misplaced option\n", filename, line);
return NULL;
#line 1565 "ifupdown.nw"
}
#line 1482 "ifupdown.nw"
		break;
	case NONE:
	default:
		
#line 1935 "ifupdown.nw"
fprintf(stderr, "%s:%d: misplaced option\n", filename, line);
return NULL;
#line 1486 "ifupdown.nw"
}
#line 1472 "ifupdown.nw"
}
#line 1146 "ifupdown.nw"
	}
	if (
#line 1231 "ifupdown.nw"
ferror(f) != 0
#line 1147 "ifupdown.nw"
                                           ) {
		
#line 1890 "ifupdown.nw"
perror(filename);
return NULL;
#line 1149 "ifupdown.nw"
	}

	
#line 1185 "ifupdown.nw"
fclose(f);
line = -1;

#line 1153 "ifupdown.nw"
	return defn;
}
#line 1244 "ifupdown.nw"
static int get_line(char **result, size_t *result_len, FILE *f, int *line) {
	
#line 1269 "ifupdown.nw"
size_t pos;

#line 1247 "ifupdown.nw"
	do {
		
#line 1276 "ifupdown.nw"
pos = 0;
#line 1249 "ifupdown.nw"
		
#line 1287 "ifupdown.nw"
do {
	
#line 1308 "ifupdown.nw"
if (*result_len - pos < 10) {
	char *newstr = realloc(*result, *result_len * 2 + 80);
	if (newstr == NULL) {
		return 0;
	}
	*result = newstr;
	*result_len = *result_len * 2 + 80;
}
#line 1289 "ifupdown.nw"
	
#line 1337 "ifupdown.nw"
if (!fgets(*result + pos, *result_len - pos, f)) {
	if (ferror(f) == 0 && pos == 0) return 0;
	if (ferror(f) != 0) return 0;
}
pos += strlen(*result + pos);
#line 1290 "ifupdown.nw"
} while(
#line 1328 "ifupdown.nw"
pos == *result_len - 1 && (*result)[pos-1] != '\n'
#line 1290 "ifupdown.nw"
                                   );

#line 1349 "ifupdown.nw"
if (pos != 0 && (*result)[pos-1] == '\n') {
	(*result)[--pos] = '\0';
}

#line 1294 "ifupdown.nw"
(*line)++;

assert( (*result)[pos] == '\0' );
#line 1250 "ifupdown.nw"
		
#line 1365 "ifupdown.nw"
{ 
	int first = 0; 
	while (isspace((*result)[first]) && (*result)[first]) {
		first++;
	}

	memmove(*result, *result + first, pos - first + 1);
	pos -= first;
}
#line 1251 "ifupdown.nw"
	} while (
#line 1389 "ifupdown.nw"
(*result)[0] == '#'
#line 1251 "ifupdown.nw"
                               );

	while (
#line 1393 "ifupdown.nw"
(*result)[pos-1] == '\\'
#line 1253 "ifupdown.nw"
                               ) {
		
#line 1397 "ifupdown.nw"
(*result)[--pos] = '\0';
#line 1255 "ifupdown.nw"
		
#line 1287 "ifupdown.nw"
do {
	
#line 1308 "ifupdown.nw"
if (*result_len - pos < 10) {
	char *newstr = realloc(*result, *result_len * 2 + 80);
	if (newstr == NULL) {
		return 0;
	}
	*result = newstr;
	*result_len = *result_len * 2 + 80;
}
#line 1289 "ifupdown.nw"
	
#line 1337 "ifupdown.nw"
if (!fgets(*result + pos, *result_len - pos, f)) {
	if (ferror(f) == 0 && pos == 0) return 0;
	if (ferror(f) != 0) return 0;
}
pos += strlen(*result + pos);
#line 1290 "ifupdown.nw"
} while(
#line 1328 "ifupdown.nw"
pos == *result_len - 1 && (*result)[pos-1] != '\n'
#line 1290 "ifupdown.nw"
                                   );

#line 1349 "ifupdown.nw"
if (pos != 0 && (*result)[pos-1] == '\n') {
	(*result)[--pos] = '\0';
}

#line 1294 "ifupdown.nw"
(*line)++;

assert( (*result)[pos] == '\0' );
#line 1256 "ifupdown.nw"
	}

	
#line 1377 "ifupdown.nw"
while (isspace((*result)[pos-1])) { /* remove trailing whitespace */
	pos--;
}
(*result)[pos] = '\0';

#line 1260 "ifupdown.nw"
	return 1;
}
#line 1435 "ifupdown.nw"
static char *next_word(char *buf, char *word, int maxlen) {
	if (!buf) return NULL;
	if (!*buf) return NULL;

	while(!isspace(*buf) && *buf) {
		if (maxlen-- > 1) *word++ = *buf;
		buf++;
	}
	if (maxlen > 0) *word = '\0';

	while(isspace(*buf) && *buf) buf++;

	return buf;
}
#line 1681 "ifupdown.nw"
static address_family *get_address_family(address_family *af[], char *name) {
	int i;
	for (i = 0; af[i]; i++) {
		if (strcmp(af[i]->name, name) == 0) {
			return af[i];
		}
	}
	return NULL;
}
#line 1712 "ifupdown.nw"
static method *get_method(address_family *af, char *name) {
	int i;
	for (i = 0; i < af->n_methods; i++) {
		if (strcmp(af->method[i].name, name) == 0) {
			return &af->method[i];
		}
	}
	return NULL;
}
#line 1765 "ifupdown.nw"
static int duplicate_if(interface_defn *ifa, interface_defn *ifb) {
	if (strcmp(ifa->iface, ifb->iface) != 0) return 0;
	if (ifa->address_family != ifb->address_family) return 0;
	return 1;
}
