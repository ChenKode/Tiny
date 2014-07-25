#include <iostream>
#include <cstring>
#include <cstdio>	/* stdout, fflush() */
#include <cstdlib>	/* atoi(), getenv() */

const int MAXLINE = 8192;

using namespace std;

int main()
{
	char *buf, *p;
	char arg1[MAXLINE], arg2[MAXLINE], content[MAXLINE];
	int n1 = 0, n2 = 0;
	
	/* Extract the two argments*/
	if ((buf = getenv("QUERY_STRING")) != NULL)
	{
		p = strchr(buf, '&');
		*p = '\0';
		strcpy(arg1, buf);
		strcpy(arg2, p + 1);
		n1 = atoi(arg1);
		n2 = atoi(arg2);
	}
	
	/* Make the response body */
	sprintf(content, "Welcome to add.com: The Internet addition portal.\r\n<p>");
	sprintf(content, "%sthe answer is: %d + %d = %d\r\n<p>", content, n1, n2, n1 + n2);
	sprintf(content, "%sThank you for visting!\r\n", content);
	
	/* Generate the HTTP response */
	cout << "Content-length: " << strlen(content) << "\r\n";
	cout << "Content-type: text/html\r\n\r\n";
	cout << content;
	fflush(stdout);

	return 0;
}
