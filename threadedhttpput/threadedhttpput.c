/***************************************************************************
 *                                  _   _ ____  _
 *  Project                     ___| | | |  _ \| |
 *                             / __| | | | |_) | |
 *                            | (__| |_| |  _ <| |___
 *                             \___|\___/|_| \_\_____|
 *
 * Copyright (C) 1998 - 2012, Daniel Stenberg, <daniel@haxx.se>, et al.
 *
 * This software is licensed as described in the file COPYING, which
 * you should have received as part of this distribution. The terms
 * are also available at http://curl.haxx.se/docs/copyright.html.
 *
 * You may opt to use, copy, modify, merge, publish, distribute and/or sell
 * copies of the Software, and permit persons to whom the Software is
 * furnished to do so, under the terms of the COPYING file.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ***************************************************************************/
#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <curl/curl.h>

struct threadparams
{

  char *url;
  char *file;

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

  struct threadparams *tparams = (struct threadparams *) params;

  char *file = tparams->file;
  char *url = tparams->url;

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

  curl_global_cleanup ();

  return NULL;
}


int
main (int argc, char **argv)
{
  int i;
  int error;
  int NUMT;

  char *folder;
  char *url;

  if (argc < 4)
    return 1;

  url = argv[1];
  NUMT = atoi (argv[2]);
  folder = argv[3];

  pthread_t tid[NUMT];

  struct threadparams params;
  params.url = url;
  params.file = folder;

  /* Must initialize libcurl before any threads are started */
  curl_global_init (CURL_GLOBAL_ALL);

  for (i = 0; i < NUMT; i++)
    {
      error = pthread_create (&tid[i], NULL,	/* default attributes please */
			      pull_one_url, (void *) &params);
      if (0 != error)
	fprintf (stderr, "Couldn't run thread number %d, errno %d\n", i,
		 error);
      else
	fprintf (stderr, "Thread %d, gets %s\n", i, url);
    }

  /* now wait for all threads to terminate */
  for (i = 0; i < NUMT; i++)
    {
      error = pthread_join (tid[i], NULL);
      fprintf (stderr, "Thread %d terminated\n", i);
    }

  return 0;
}
