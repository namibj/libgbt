#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "queue.h"
#include "libgbt.h"
#include "config.h"

#define INPUT "april"

int
main(int argc, char *argv[])
{
	char *OUTPUT = NULL;
        struct blist *head = NULL;
	struct bdata *dict = NULL;
        head = bparselist(stdin);
        assert(head != NULL);
	dict = TAILQ_FIRST(head);
        assert(dict->type == DICTIONARY);
	OUTPUT = bdictget(dict->list, "name")->string;
	assert(!strcmp(INPUT, OUTPUT));
        bfree(head);
	return 0;
}
