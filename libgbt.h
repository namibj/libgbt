/* structure defining _ANY_ type of bencoding data */
struct bdata {
	char type;
	size_t len;
	long num;
	char *str;
	struct blist *bl;
	TAILQ_ENTRY(bdata) entries;
};
TAILQ_HEAD(blist, bdata);

struct torrent {
	char *url;
	uint8_t *bits;
	struct file {
		char *path;
		size_t len;
	} *files;
	struct piece {
		uint8_t sha1[20];
		uint8_t *data;
		size_t len;
	} *pieces;
};

int bfree(struct blist *);
struct blist * bdecode(char *, size_t);
struct bdata * bsearchkey(struct blist *, const char *);
