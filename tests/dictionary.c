#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "queue.h"
#include "config.h"
#include "libgbt.h"

#define INPUT "april"

int
main(int argc, char *argv[])
{
	char *OUTPUT = NULL;
        struct blist *head = NULL;
	struct bdata *dict = NULL;
        head = bdecode(stdin);
        assert(head != NULL);
	dict = TAILQ_FIRST(head);
        assert(dict->type == 'd');
	OUTPUT = bsearchkey(dict->bl, "name")->str;
	assert(!strcmp(INPUT, OUTPUT));
        bfree(head);
	return 0;
}
