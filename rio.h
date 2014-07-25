/*
 *	rio.h
 *	
 *	Rubust I/O
 *
 */
 
#ifndef RIO_H_
#define RIO_H_

 
#include <unistd.h>

/* input & output functions WITHOUT buffer */
ssize_t rio_readn(int fd, char *usrbuf, size_t n);
ssize_t rio_writen(int fd, char *usrbuf, size_t n);


/* input functions WITH buffer */
const int RIO_BUFSIZE = 8192;
struct rio_t
{	/* internal read_buffer_struct for functions WITH buffer */
	int rio_fd;					/* descriptor for this internal buf */
	int rio_cnt;				/* dnread bytes in niternal buf */
	char *rio_bufptr;			/* next unread byte in internal buf */
	char rio_buf[RIO_BUFSIZE];	/* internal buf */
};
void rio_readinitb(rio_t *rp, int fd);
ssize_t rio_readlineb(rio_t *rp, char *usrbuf, size_t maxlen);
ssize_t rio_readnb(rio_t *rp, char *usrbuf, size_t n);


#endif
