/*!
 * \file updater.c
 *
 * \author Copyright 2011 by Bert Timmerman <bert.timmerman@xs4all.nl>
 *
 * \brief .
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
#include <curl/curl.h>
#include <curl/easy.h>


int
updater_get_latest_version_info (void)
{
  CURL *curl;
  CURLcode res;

  curl = curl_easy_init ();
  if (curl)
  {
    curl_easy_setopt (curl, CURLOPT_URL, "http://ljh4timm.home.xs4all.nl/testing-pcb-updater/latest-version");
    res = curl_easy_perform (curl);
    if (CURLE_OK == res)
    {
      char *ct; /* content type */
      res = curl_easy_getinfo (curl, CURLINFO_CONTENT_TYPE, &ct);
      if ((CURLE_OK == res) && ct)
      {
        fprintf (stderr, "Received Content-Type: %s\n", ct);
      }
    }
    curl_easy_cleanup (curl);
  }
  return 0;
}


int
main (void)
{

  curl_global_init (CURL_GLOBAL_DEFAULT);
  updater_get_latest_version_info ();
  curl_global_cleanup ();
  return 0;
}


/* EOF */

