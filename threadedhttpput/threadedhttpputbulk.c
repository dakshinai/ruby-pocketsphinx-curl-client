#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <curl/curl.h>
#include <sys/types.h>
#include <dirent.h>
#include <string.h>
#include <stdlib.h>

struct threadparams
{

  char *url;
  char *file;
  char *headers;

};

static size_t
read_callback (void *ptr, size_t size, size_t nmemb, void *stream)
{
  size_t retcode;
  curl_off_t nread;

  /* in real-world cases, this would probably get this data differently
     as this fread() stuff is exactly what the library already would do
     by default internally */
  retcode = fread (ptr, size, nmemb, stream);

  nread = (curl_off_t) retcode;

  fprintf (stderr, "*** We read %" CURL_FORMAT_CURL_OFF_T
	   " bytes from file\n", nread);

  return retcode;
}

static void *
pull_one_url (void *params)
{
  CURL *curl;
  FILE *hd_src;
  struct stat file_info;
  CURLcode res;
  struct curl_slist *headerlist=NULL;

  struct threadparams *tparams = (struct threadparams *) params;

  char *file = tparams->file;
  char *url = tparams->url;
  char *headers = tparams->headers;

  fprintf (stderr, "HTTP PUT for file %s on %s\n", file, url);

  /* get the file size of the local file */
  stat (file, &file_info);

  /* get a FILE * of the same file, could also be made with
     fdopen() from the previous descriptor, but hey this is just
     an example! */
  hd_src = fopen (file, "rb");

  /* get a curl handle */
  curl = curl_easy_init ();
  if (curl)
    {
      /* we want to use our own read function */
      curl_easy_setopt (curl, CURLOPT_READFUNCTION, read_callback);

      /* enable uploading */
      curl_easy_setopt (curl, CURLOPT_UPLOAD, 1L);

      /* HTTP PUT please */
      curl_easy_setopt (curl, CURLOPT_PUT, 1L);

      /* specify target URL, and note that this URL should include a file
         name, not only a directory */
      curl_easy_setopt (curl, CURLOPT_URL, url);

      /* set header information */
      headerlist = curl_slist_append(headerlist, headers);
      curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headerlist);

      /* specify the user agent for the server */
      curl_easy_setopt (curl, CURLOPT_USERAGENT, "threaderdhttpput/1.0");

      /* now specify which file to upload */
      curl_easy_setopt (curl, CURLOPT_READDATA, hd_src);

      /* provide the size of the upload, we specicially typecast the value
         to curl_off_t since we must be sure to use the correct data size */
      curl_easy_setopt (curl, CURLOPT_INFILESIZE_LARGE,
			(curl_off_t) file_info.st_size);

      /* Now run off and do what you've been told! */
      res = curl_easy_perform (curl);
      /* Check for errors */
      if (res != CURLE_OK)
	fprintf (stderr, "curl_easy_perform() failed: %s\n",
		 curl_easy_strerror (res));

      /* always cleanup */
      curl_easy_cleanup (curl);
    }
  fclose (hd_src);		/* close the local file */

  //curl_global_cleanup ();

  return NULL;
}


int
main (int argc, char **argv)
{
  int i;
  int error;
  DIR *dp;
  struct dirent *ep;
  unsigned char isFile = 0x8;
  int c = 0;
  char *folder;
  char *url;
  char *headers;
  char **filenames = (char **) malloc (30 * sizeof (char *));
  

  if (argc < 4)
    return 1;

  url = argv[1];
  headers=argv[2];
  folder = argv[3]; 

  dp = opendir (folder);

  if (dp != NULL)
    {
      while (ep = readdir (dp))
	{
	  if (ep->d_type == isFile)
	    {
	      filenames[c] =
		(char *) malloc ((strlen (ep->d_name) + 1) * sizeof (char));

	      strcpy (filenames[c], ep->d_name);

	      c = c + 1;
	    }
	}
      (void) closedir (dp);
    }
  else
    perror ("Couldn't open the directory");

  pthread_t tid[c];




  /* Must initialize libcurl before any threads are started */
  curl_global_init (CURL_GLOBAL_ALL);

  for (i = 0; i < c; i++)
    {

      struct threadparams params;
      params.url = url;
      params.headers = headers;
      params.file = (char *) malloc ((strlen (folder) + 1) * sizeof (char));      
 
      strcpy (params.file, folder);
      strcat (params.file, filenames[i]);

      fprintf (stderr, "Thread %d, gets %s for file %s\n", i, url, params.file);
      error = pthread_create (&tid[i], NULL,	/* default attributes please */
			      pull_one_url, (void *) &params);
				
      if (0 != error)
	fprintf (stderr, "Couldn't run thread number %d, errno %d on %s for file %s\n", i,
		 error, url, params.file);
      //else
	//fprintf (stderr, "Thread %d, gets %s for file %s\n", i, url, params.file);
    }

  /* now wait for all threads to terminate */
  for (i = 0; i < c; i++)
    {
      error = pthread_join (tid[i], NULL);
      fprintf (stderr, "Thread %d terminated\n", i);
    }

  

  return 0;
}
