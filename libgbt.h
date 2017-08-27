#include <limits.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

#define PEERID "-GT0000-000000000000"
#define PIECE_MAX 1048576
#define BLOCK_MAX 16384
#define MESSAGE_MAX ((BLOCK_MAX) + 9)

enum {
	PWP_CHOKE = 0,
	PWP_UNCHOKE,
	PWP_INTEREST,
	PWP_UNINTEREST,
	PWP_HAVE,
	PWP_BITFIELD,
	PWP_REQUEST,
	PWP_PIECE,
	PWP_CANCEL,
	PWP_HANDSHAKE
};

enum {
	THP_NONE,
	THP_STARTED,
	THP_STOPPED,
	THP_COMPLETED
};

/* structure defining _ANY_ type of bencoding data */
struct bdata {
	char type;
	size_t len;
	long num;
	char *str;
	char *s;
	char *e;
	struct blist *bl;
	TAILQ_ENTRY(bdata) entries;
};
TAILQ_HEAD(blist, bdata);

struct file {
	char path[PATH_MAX];
	size_t len;
};

struct piece {
	uint8_t sha1[20];
	uint8_t *data;
	size_t len;
};

struct peer {
	int sockfd;
	uint8_t choked;
	uint8_t interrested;
	uint8_t *bitfield;
	struct sockaddr_in peer;
	TAILQ_ENTRY(peer) entries;
};
TAILQ_HEAD(peers, peer);

struct torrent {
	char announce[PATH_MAX];
	char *buf;
	struct blist meta;
	uint8_t peerid[21];
	uint8_t infohash[20];
	uint8_t *bitfield;
	uint8_t *pieces;
	size_t size;
	size_t filnum;
	size_t pcsnum;
	size_t piecelen;
	size_t upload;
	size_t download;
	struct file *files;
	struct peers *peers;
};

struct torrent * metainfo(const char *);
int getpeers(struct torrent *);
ssize_t pwpsend(struct torrent *, struct peer *, int);
