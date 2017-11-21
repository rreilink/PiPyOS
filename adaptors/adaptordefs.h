struct timeval {
	long	tv_sec;		/* seconds */
	long	tv_usec;	/* and microseconds */
};

struct timezone {
    int tz_minuteswest;     /* minutes west of Greenwich */
    int tz_dsttime;         /* type of DST correction */
};

typedef unsigned int mode_t;
typedef long int time_t;


struct stat {
    mode_t st_mode;
    unsigned int st_size; //off_t st_size;
    time_t st_mtime;
    int st_dev; //used in random.c 308
    int st_ino; //used in random.c 309
};

int gettimeofday(struct timeval *tv, struct timezone *tz);

//extern unsigned int errno;
extern int fileno(FILE *stream);
extern int isatty(int fd);
int fstat(int fd, struct stat *buf);
int stat(const char *path, struct stat *buf);


int open(const char *pathname, int flags);
FILE *fdopen(int ft, char *mode);

typedef void (*sighandler_t)(int);

sighandler_t signal(int signum, sighandler_t handler);
