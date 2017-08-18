#include <err.h>
#include <limits.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/stat.h>

#include "queue.h"
#include "libgbt.h"

#define MIN(a,b) ((a)<(b)?(a):(b))
#define MAX(a,b) ((a)>(b)?(a):(b))

static int isnum(char);
static char * bparseint(struct blist *, char *, size_t);
static char * bparsestr(struct blist *, char *, size_t);
static char * bparselnd(struct blist *, char *, size_t);
static char * bparseany(struct blist *, char *, size_t);

static int bcountlist(const struct blist *);
static size_t bpathfmt(const struct blist *, char *);
static char * metastr(const struct blist *, const char *);
static size_t metafiles(const struct blist *, struct torrent *);
static size_t metapieces(const struct blist *, struct torrent *);

static int
isnum(char c) {
	return (c >= '0' && c <= '9');
}

static char *
bparseint(struct blist *bl, char *buf, size_t len)
{
	long n = 0;
	char *p = buf;
	struct bdata *np = NULL;

	if (*p++ != 'i')
		errx(1, "not an integer\n");

	if (*p == '0' && *(p+1) != 'e')
		errx(1, "0 not followed by e\n");

	if (*p == '-' && (isnum(*p+1) && *p+1 > '0'))
		errx(1, "invalid negative number\n");

	np = malloc(sizeof(struct bdata));
	if (!np)
		return NULL;

	while (*p != 'e' && p < buf + len) {
		if (*p == '-') {
			n = -1;
		} else if (isnum(*p)) {
			n *= 10;
			n += *p - '0';
		} else {
			free(np);
			errx(1, "'%c': invalid character\n", *p);
		}
		p++;
	}

	np->type = 'i';
	np->num = n;
	TAILQ_INSERT_TAIL(bl, np, entries);
	return p;
}

static char *
bparsestr(struct blist *bl, char *buf, size_t len)
{
	char *p = buf;
	struct bdata *np = NULL;

	if (!isnum(buf[0]))
		errx(1, "not a string\n");

	np = malloc(sizeof(struct bdata));
	if (!np)
		return NULL;

	np->len = 0;
	while (*p != ':' && p < (buf + len)) {
		np->len *= 10;
		np->len += *p++ - '0';
	}

	np->str = ++p;
	np->type = 's';
	TAILQ_INSERT_TAIL(bl, np, entries);
	return p + np->len - 1;
}

static char *
bparselnd(struct blist *bl, char *buf, size_t len)
{
	char *p = buf;
	struct bdata *np = NULL;

	if (*p != 'l' && *p != 'd')
		errx(1, "not a dictionary or list\n");

	if (*++p == 'e')
		errx(1, "dictionary or list empty\n");

	np = malloc(sizeof(struct bdata));
	if (!np)
		return NULL;

	np->bl = malloc(sizeof(struct blist));
	if (!np->bl)
		return NULL;
	TAILQ_INIT(np->bl);

	while (*p != 'e' && p < buf + len)
		p = bparseany(np->bl, p, len - (size_t)(p - buf)) + 1;

	np->type = *buf;
	TAILQ_INSERT_TAIL(bl, np, entries);
	return p;
}

static char *
bparseany(struct blist *bl, char *buf, size_t len)
{
	switch (buf[0]) {
	case 'l': /* FALLTHROUGH */
	case 'd':
		return bparselnd(bl, buf, len);
		break; /* NOTREACHED */
	case 'i':
		return bparseint(bl, buf, len);
		break; /* NOTREACHED */
	case 'e':
		return buf;
	default:
		if (isnum(*buf))
			return bparsestr(bl, buf, len);

		errx(1, "'%c' unexpected\n", *buf);
		break; /* NOTREACHED */
	}
	return buf;
}

struct blist *
bdecode(char *buf, size_t len)
{
	char *p = buf;
	size_t s = len;
	struct blist *bl = NULL;

	bl = malloc(sizeof(struct blist));
	if (!bl)
		return NULL;

	TAILQ_INIT(bl);

	while (s > 1) {
		p = bparseany(bl, p, s);
		s = len - (p - buf);
		p++;
	}

	return bl;
}

int
bfree(struct blist *bl)
{
	struct bdata *np = NULL;
	while (!TAILQ_EMPTY(bl)) {
		np = TAILQ_FIRST(bl);
		switch(np->type) {
		case 'd':
		case 'l':
			bfree(np->bl);
			break;
		}
		TAILQ_REMOVE(bl, np, entries);
		free(np);
	}
	free(bl);
	return 0;
}

/*
 * Search a key within a bencoded data structure recursively.
 * each data element has to be a string, except for the very
 * first element which CAN be a dictionary.
 *
 * When we encounter a dictionary, the function will be called on this
 * dictionary so we can find nested keys.
 *
 * Because of this search algorithm, only the first occurence of each
 * key will be retrieved.
 * This should not be an issue as torrent files are not supposed to
 * appear twice, except for multifile torrent. For them, this function
 * can be called for each element of the "files" list.
 */
struct bdata *
bsearchkey(const struct blist *bl, const char *key)
{
	struct bdata *np;
	if (key == NULL) return NULL;
	TAILQ_FOREACH(np, bl, entries) {
		switch(np->type) {
		case 's':
			if (strlen(key) == np->len && !strncmp(key, np->str, np->len))
				return TAILQ_NEXT(np, entries);
			np = TAILQ_NEXT(np, entries);
		case 'd': /* FALLTHROUGH */
			if (np->type == 'd')
				return bsearchkey(np->bl, key);
			break;
		default:
			return NULL;
		}

	}
	return NULL;
}

static int
bcountlist(const struct blist *bl)
{
	int n = 0;
	struct bdata *np;

	TAILQ_FOREACH(np, bl, entries)
		n++;

	return n;
}

static size_t
bpathfmt(const struct blist *bl, char *path)
{
	struct bdata *np;

	TAILQ_FOREACH(np, bl, entries) {
		path[strlen(path)] = '/';
		strncat(path, np->str, np->len);
	}

	return strlen(path);
}

static char *
metastr(const struct blist *bl, const char *key)
{
	char *url;
	struct bdata *np = NULL;

	np = bsearchkey(bl, key);
	url = malloc(np->len + 1);
	url[np->len] = 0;
	memcpy(url, np->str, np->len);

	return url;
}

static size_t
metafiles(const struct blist *bl, struct torrent *to)
{
	int i = 0;
	size_t namelen = 0;
	char name[PATH_MAX];
	struct bdata *np;
	struct blist *head;

	np = bsearchkey(bl, "name");
	namelen = np->len;
	memset(name, 0, PATH_MAX);
	memcpy(name, np->str, MIN(namelen, PATH_MAX - 1));

	np = bsearchkey(bl, "files");
	if (np) { /* multi-file torrent */
		head = np->bl;
		to->filnum = bcountlist(head);
		to->files = malloc(sizeof(struct file) * bcountlist(head));
		TAILQ_FOREACH(np, head, entries) {
			to->files[i].len  = bsearchkey(np->bl, "length")->num;
			memset(to->files[i].path, 0, PATH_MAX);
			memcpy(to->files[i].path, name, namelen);
			bpathfmt(bsearchkey(np->bl, "path")->bl, to->files[i].path);
			i++;
		}
	} else { /* single-file torrent */
		to->files = malloc(sizeof(struct file));
		to->files[0].len = bsearchkey(bl, "length")->num;
		strcpy(to->files[0].path, name);
		to->filnum = 1;
	}
	return to->filnum;
}

static size_t
metapieces(const struct blist *bl, struct torrent *to)
{
	size_t i, len, plen, tmp = 0;
	struct bdata *np;

	for (i = 0; i < to->filnum; i++)
		len += to->files[i].len;

	plen = bsearchkey(bl, "piece length")->num;
	np = bsearchkey(bl, "pieces");
	to->pcsnum = len/plen + !!(len/plen);
	to->pieces = malloc(sizeof(struct piece) * to->pcsnum);

	for (i = 0; i < to->pcsnum; i++) {
		to->pieces[i].len = (len - tmp < plen) ? len - tmp : plen;
		tmp += to->pieces[i].len;
		memcpy(to->pieces[i].sha1, np->str + (i*20), 20);
		to->pieces[i].data = malloc(to->pieces[i].len);
		memset(to->pieces[i].data, 0, to->pieces[i].len);
	}

	return to->pcsnum;
}

struct torrent *
metainfo(const char *path)
{
	char *buf = NULL;
	FILE *f   = NULL;
	struct stat sb;
	struct blist *meta;
	struct torrent *to;

	stat(path, &sb);
	f = fopen(path, "r");
	buf = malloc(sb.st_size);
	to = malloc(sizeof(struct torrent));

	fread(buf, 1, sb.st_size, f);
	fclose(f);

	meta = bdecode(buf, sb.st_size);

	to->url = metastr(meta, "announce");
	metafiles(meta, to);
	metapieces(meta, to);

	bfree(meta);
	free(buf);

	return to;
}
