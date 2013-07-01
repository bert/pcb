/*!
 * \file updater.c
 *
 * \author Copyright 2011, 2013 by Bert Timmerman <bert.timmerman@xs4all.nl>
 *
 * \brief Check for any updates available.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.\n
 * \n
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.\n
 * \n
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.\n
 *
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <stdarg.h>
#include <glib.h>
#include <glib/gprintf.h>
#include <curl/curl.h>
#include <curl/easy.h>

#ifdef HAVE_CONFIG_H
  #include "config.h" /* for the gettext domain */
#endif

#define PCB_LATEST_VERSION_URI "http://ljh4timm.home.xs4all.nl/testing-pcb-updater/latest-version"

typedef struct
{
    gint major;
    gint minor;
    gint mini;
    gchar *extra;
}
pcb_version_struct;

char* pcb_latest_version = NULL;

struct MemoryStruct
{
  char *memory;
  size_t size;
};


struct FtpFile
{
  const char *filename;
  FILE *stream;
};


static size_t
updater_fwrite (void *buffer, size_t size, size_t nmemb, void *stream)
{
  struct FtpFile *out= (struct FtpFile *) stream;
  if (out && !out->stream)
  {
    /* open file for writing */ 
    out->stream = fopen (out->filename, "wb");
    if (!out->stream)
      return -1; /* failure, can't open file to write */ 
  }
  return fwrite (buffer, size, nmemb, out->stream);
}


int
updater_get_file (char *filename)
{
  struct FtpFile ftpfile =
  {
    "pcb-latest.tar.gz", /* name to store the file as if successful */ 
    NULL
  };
  CURL *curl;
  CURLcode res;

  curl = curl_easy_init ();
  if (curl)
  {
    curl_easy_setopt (curl, CURLOPT_URL, filename);
    /* Define our callback to get called when there's data to be written */ 
    curl_easy_setopt (curl, CURLOPT_WRITEFUNCTION, updater_fwrite);
    /* Set a pointer to our struct to pass to the callback */ 
    curl_easy_setopt (curl, CURLOPT_WRITEDATA, &ftpfile);
    /* Switch on full protocol/debug output */ 
    curl_easy_setopt (curl, CURLOPT_VERBOSE, 1L);
    res = curl_easy_perform (curl);
    curl_easy_cleanup (curl);
    if (CURLE_OK != res)
    {
      fprintf (stderr, "curl told us %d\n", res);
    }
    else
      {
        fprintf (stderr, "received Content-Type: %d\n", res);
      }
  }
  if (ftpfile.stream)
  {
    fclose (ftpfile.stream);
  }
  return 0;
}


static size_t
updater_write_memory_callback (void *contents, size_t size,
  size_t nmemb, void *userp)
{
  size_t realsize = size * nmemb;
  struct MemoryStruct *mem = (struct MemoryStruct *)userp;

  mem->memory = realloc (mem->memory, mem->size + realsize + 1);
  if (mem->memory == NULL)
  {
    /* out of memory! */ 
    fprintf (stderr, "not enough memory (realloc returned NULL)\n");
    exit (EXIT_FAILURE);
  }
  memcpy (&(mem->memory[mem->size]), contents, realsize);
  mem->size += realsize;
  mem->memory[mem->size] = 0;
  return realsize;
}


gint
updater_get_latest_version_info (void)
{
  CURL *curl; /* CURL handle */
  CURLcode res;
  struct MemoryStruct chunk;
  gchar *used_curl_version = curl_version ();

  chunk.memory = malloc (1);  /* will be grown as needed by the realloc above */ 
  chunk.size = 0;    /* no data at this point */ 
  curl = curl_easy_init ();
  if (curl)
  {
    curl_easy_setopt (curl, CURLOPT_URL, PCB_LATEST_VERSION_URI);
    curl_easy_setopt (curl, CURLOPT_WRITEFUNCTION, updater_write_memory_callback);
    curl_easy_setopt (curl, CURLOPT_WRITEDATA, (void *)&chunk);
    curl_easy_setopt (curl, CURLOPT_USERAGENT, "libcurl-agent/1.0");
    res = curl_easy_perform (curl);
    if (CURLE_OK == res)
    {
      char *ct; /* content type */
      res = curl_easy_getinfo (curl, CURLINFO_CONTENT_TYPE, &ct);
      if ((CURLE_OK == res) && ct)
      {
        fprintf (stderr, "Using: %s to check the latest pcb version.\n", used_curl_version);
        fprintf (stderr, "Received Content-Type: %s\n", ct);
        fprintf (stderr, "%lu bytes retrieved\n", (long) chunk.size);
        fprintf (stderr, "Latest version is: %s\n", chunk.memory);
        pcb_latest_version = g_strdup (chunk.memory);
      }
    }
    curl_easy_cleanup (curl);
  }
  else
  {
    fprintf (stderr, "Could not obtain a valid CURL handle.\n");
  }
  if (chunk.memory)
  {
    free (chunk.memory);
  }
  return (EXIT_SUCCESS);
}


/*!
 * Based on some code by Sylpheed project.
 * http://sylpheed.sraoss.jp/en/
 * GPL FTW! */
static void
updater_parse_version_string (const gchar *ver, gint *major, gint *minor,
  gint *micro, gchar **extra)
{
  gchar **vers;
  vers = g_strsplit (ver, ".", 4);
  if (vers[0])
  {
    *major = atoi (vers[0]);
    if (vers[1])
    {
      *minor = atoi (vers[1]);
      if (vers[2])
      {
        *micro = atoi (vers[2]);
        if (vers[3])
        {
          *extra = g_strdup (vers[3]);
        }
        else
        {
          *extra = NULL;
        }
      }
      else
      {
        *micro = 0;
      }
    }
    else
    {
      *minor = 0;
    }
  }
  else
  {
    major = 0;
  }
  g_strfreev (vers);
}


/*!
 * It is assumed that \c version contains a string with a format similar
 * to "pcb-4.9.9.20151231".
 * Returns TRUE if the version installed is < as the version found
 * on the server. All other cases a causes a FALSE.
 */
static gboolean
updater_version_compare (const gchar *version)
{
  pcb_version_struct running;
  pcb_version_struct latest;

  updater_parse_version_string (PACKAGE_VERSION, &running.major,
    &running.minor, &running.mini, &running.extra);

  updater_parse_version_string (version, &latest.major,
    &latest.minor, &latest.mini, &latest.extra);

  if ((running.major < latest.major) ||
    (running.minor < latest.minor) ||
    (running.minor < latest.minor))
  {
    return TRUE;
  }
  else
  {
    return FALSE;
  }
}


int
main (int argc, char **argv)
{
  curl_global_init (CURL_GLOBAL_DEFAULT);
  updater_get_latest_version_info ();
  /* strip package name from received string. */
  if (updater_version_compare (pcb_latest_version))
  {
    /* installed version < latest version. */
    /* get the latest version tarball ? */
  }
  else
  {
    /* installed version = latest version. */
  }
  curl_global_cleanup ();
  return 0;
}


/* EOF */

