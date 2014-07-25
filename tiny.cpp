/*
 *	Tiny - A simple, interative HTTP/1.0 Web Sever that uses
 *		the GET method to serve static and dynamic content.
 */
 
#include <iostream>
#include <cstring>
#include <cstdio>
#include <cstdlib>		/* atoi(), setenv() */
#include <unistd.h>
#include <arpa/inet.h>	/* in_addr, inet_aton()*/
#include <sys/socket.h>
#include <sys/stat.h>	/* stat */
#include <sys/mman.h>	/* mmap() */
#include <sys/wait.h>	/* wait() */
#include <fcntl.h>		/* open() */
#include "rio.h"

using namespace std;

const int MAXLINE = 4096;
const int MAXBUF = 8192;

void doIt(int fd);
void readRequestHdrs(rio_t *pRio);
int parseURI(char *uri, char *filename, char *cgiArgs);
void serveStatic(int fd, char *filename, int filesize);
void getFiletype(char *filename, char *filetype);
void serveDynamic(int fd, char *filename, char *cgiArgs);
void clientError(int fd, char *cause, char *errCode, char *shortMsg, char *longMsg);
int open_listenfd(int port);

int main(int argc, char **argv)
{
 	int fdListen, fdConnect, port;
 	sockaddr_in addrClient;
 	
 	/* Check command line argments */
 	if (argc != 2)
 	{
 		cout << "usage: " << argv[0] << "<port>" << endl;
 		return -1;
 	}
 	
 	port = atoi(argv[1]);
 	
 	fdListen = open_listenfd(port);
 	
 	while (1)
 	{
 		unsigned int len = sizeof(addrClient);
 		fdConnect = accept(fdListen, (sockaddr *)&addrClient, &len);
 		doIt(fdConnect);
 		close(fdConnect);
 	} 	
 	
	return 0;
}

void doIt(int fd)
{
	char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
	char filename[MAXLINE], cgiArgs[MAXLINE];
	rio_t rio;
	
	/* Read request line and headers */
	rio_readinitb(&rio, fd);
	rio_readlineb(&rio, buf, MAXLINE);
	sscanf(buf, "%s %s %s", method, uri, version);
	if (strcasecmp(method, "GET"))
	{
		clientError(fd, method, "501", "Not found", "Tiny couldn't find this file");
		return;
	}
	readRequestHdrs(&rio);
	
	/*Parse UPI from GET request*/
	int isStatic = parseURI(uri, filename, cgiArgs);
	
	struct stat bufStat;
	if (stat(filename, &bufStat) < 0)
	{
		clientError(fd, filename, "404", "Not found", "Tiny couldn't find this file");
		return;
	}
	
	if (isStatic)
	{	/* Serve static content */
		if (!S_ISREG(bufStat.st_mode) || !(S_IRUSR & bufStat.st_mode))
		{
			clientError(fd, filename, "403", "Forbidden", "Tiny couldn't read this file");
			return;
		}
		else
		{
			serveStatic(fd, filename, bufStat.st_size);
		}
	}
	else
	{
		if (!S_ISREG(bufStat.st_mode) || !(S_IXUSR & bufStat.st_mode))
		{
			clientError(fd, filename, "403", "Forbidden", "Tiny couldn't read this file");
			return;
		}
		else
		{
			serveDynamic(fd, filename, cgiArgs);
		}
	}	
}

void readRequestHdrs(rio_t *pRio)
{
	char buf[MAXLINE];
	
	rio_readlineb(pRio, buf, MAXLINE);
	while (strcmp(buf, "\r\n"))
	{
		rio_readlineb(pRio, buf, MAXLINE);
		cout << buf;
	}
	
	return;
}

int parseURI(char *uri, char *filename, char *cgiArgs)
{	
	if (!strstr(uri, "cgi-bin"))
	{	/* Static content */
		cgiArgs[0] = '\0';
		strcpy(filename, ".");
		strcat(filename, uri);
		if (uri[strlen(uri) - 1] == '/')
		{
			strcat(filename, "home.html");
		}
		
		return 1;
	}
	else
	{	/* Dynamic content */
		char *ptr = index(uri, '?');
		if (ptr)
		{
			strcpy(cgiArgs, ptr + 1);
			*ptr = '\0';
		}
		else
		{
			cgiArgs[0] = '\0';
		}
		
		strcpy(filename, ".");
		strcat(filename, uri);
		
		return 0;
	}
	
	return 0;
}

void serveStatic(int fd, char *filename, int filesize)
{
	char filetype[MAXLINE], buf[MAXBUF];
	
	/* Send response header to client */
	getFiletype(filename, filetype);
	sprintf(buf, "HTTP/1.0 200 OK\r\n");
	sprintf(buf, "%sServer: Tiny Web Server\r\n", buf);
	sprintf(buf, "%sContent-length: %d\r\n", buf, filesize);
	sprintf(buf, "%sContent-type: %s\r\n\r\n", buf, filetype);
	rio_writen(fd, buf, strlen(buf));
	
	/*Send response body to client*/
	/* Open requested source file in read-only way */
	int fdSrc = open(filename, O_RDONLY, 0);
	/* map file to private read-only virtul memory*/
	char *pSrc = (char *)mmap(0, filesize, PROT_READ, MAP_PRIVATE, fdSrc, 0);
	close(fdSrc);			/* OK to close requested source file after map*/
	rio_writen(fd, pSrc, filesize);
	munmap(pSrc, filesize);	/*unmap*/
}

void getFiletype(char *filename, char *filetype)
{
	if (strstr(filename, ".html"))
		strcpy(filetype, "text/html");
	else if (strstr(filename, ".gif"))
		strcpy(filetype, "image/gif");
	else if (strstr(filename, ".jpg"))
		strcpy(filetype, "image/jpeg");
	else
		strcpy(filetype, "text/plain");
}

void serveDynamic(int fd, char *filename, char *cgiArgs)
{
	char buf[MAXLINE];
	
	/* Return first part of HTTP response */
	sprintf(buf, "HEEP/1.0 200 OK\r\n");
	rio_writen(fd, buf, strlen(buf));
	sprintf(buf, "Server: Tiny Web Server\r\n");
	rio_writen(fd, buf, strlen(buf));
	
	char **emptyList = {NULL};
	if (fork() == 0)
	{	/* clild */
		/* Real server would set all CGI variables here */
		setenv("QUERY_STRING", cgiArgs, 1);
		/* Redirect stdout to client */
		dup2(fd, STDOUT_FILENO);
		/* run CGI program */
		execve(filename, emptyList, environ);
	}
	/* Parent waits for and reaps child */
	wait(NULL);	
}

void clientError(int fd, char *cause, char *errCode, char *shortMsg, char *longMsg)
{
	char buf[MAXLINE], body[MAXBUF];
	
	/* Build the HTTP response body */
	sprintf(body, "<html><title>Tiny Error</title>");
	sprintf(body, "%s<body bgcolor=""ffffff"">\r\n", body);
	sprintf(body, "%s%s: %s\r\n", body, errCode, shortMsg);
	sprintf(body, "%s<p>%s: %s\r\n", body, longMsg, cause);
	sprintf(body, "%s<hr><em>The Tiny Web server</em>\r\n", body);
	
	/* Print the HTTP response */
	sprintf(buf, "HTTP/1.0 %s %s\r\n", errCode, shortMsg);
	rio_writen(fd, buf, strlen(buf));
	sprintf(buf, "Content-type: text/html\r\n");
	rio_writen(fd, buf, strlen(buf));
	sprintf(buf, "Content-length: %d\r\n\r\n", strlen(body));
	rio_writen(fd, buf, strlen(buf));
	rio_writen(fd, body, strlen(body));
}

int open_listenfd(int port)
{
	int listenfd, optval = 1;
	sockaddr_in serveraddr;
	
	if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		return -1;
	}
	
	if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, (const void*)&optval, sizeof(int)) < 0)
	{
		return -1;
	}
	
	memset((char *)&serveraddr, 0, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
	serveraddr.sin_port = htons((unsigned short)port);
	
	if (bind(listenfd, (sockaddr *)&serveraddr, sizeof(serveraddr)) < 0)
	{
		return -1;
	}
	
	if (listen(listenfd, 3) < 0)
	{
		return -1;
	}
	
	return listenfd; 
}



