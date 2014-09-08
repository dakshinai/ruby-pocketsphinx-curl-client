#include <stdio.h>
#include <sys/types.h>
#include <dirent.h>
     
int
main (int argc, char **argv)
{
DIR *dp;
struct dirent *ep;
char *folder; 
int c=0;
unsigned char isFile =0x8;

folder=argv[1];
dp = opendir (folder);
if (dp != NULL)
{
while (ep = readdir (dp))
{
if (ep->d_type == isFile)
{
puts (ep->d_name);
c=c+1;
}
}
(void) closedir (dp);
}
else
perror ("Couldn't open the directory");
printf("Count:%d\n",c);
return 0;
}
