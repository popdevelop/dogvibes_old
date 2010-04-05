/* GStreamer
 *
 * unit test for spot
 *
 * Copyright (C) <2010> Johan Gyllenspez
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include <unistd.h>

#include <gst/check/gstcheck.h>

/* For ease of programming we use globals to keep refs for our floating
 * src and sink pads we create; otherwise we always have to do get_pad,
 * get_peer, and then remove references in every test function */
static GstPad *mysinkpad;

#define CAPS_TEMPLATE_STRING            \
    "audio/x-raw-int, "                 \
    "channels = (int) 2, "              \
    "rate = (int) 44100, "              \
    "endianness = (int) { 1234 }, "	\
    "width = (int) 16, "                \
    "depth = (int) 16, "                \
    "signed = (bool) TRUE"

#define SPOTIFY_URI "spotify:track:0E4rbyLYVCGLu75H3W6O67"
#define SPOTIFY_USER "user"
#define SPOTIFY_PASS "pass"

static GstStaticPadTemplate sinktemplate = GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS (CAPS_TEMPLATE_STRING)
    );

static GstElement *
setup_spot (void)
{
  GstElement *spot;

  g_print ("setup_spot\n");
  spot = gst_check_setup_element ("spot");
  g_object_set (G_OBJECT (spot), "spotifyuri", SPOTIFY_URI, NULL);
  g_object_set (G_OBJECT (spot), "user", SPOTIFY_USER, NULL);
  g_object_set (G_OBJECT (spot), "pass", SPOTIFY_PASS, NULL);
  mysinkpad = gst_check_setup_sink_pad (spot, &sinktemplate, NULL);
  gst_pad_set_active (mysinkpad, TRUE);

  return spot;
}

static void
cleanup_spot (GstElement * spot)
{
  GST_DEBUG ("cleanup_spot");

  g_list_foreach (buffers, (GFunc) gst_mini_object_unref, NULL);
  g_list_free (buffers);
  buffers = NULL;

  gst_pad_set_active (mysinkpad, FALSE);
  gst_check_teardown_sink_pad (spot);
  gst_check_teardown_element (spot);
}

GST_START_TEST (test_login_and_play)
{
  GstElement *spot;

  g_print ("I am here to start with\n");
  spot = setup_spot();
  fail_unless (gst_element_set_state (spot,
				      GST_STATE_PLAYING) == GST_STATE_CHANGE_SUCCESS,
	       "could not set to playing");

  g_mutex_lock (check_mutex);
  while (g_list_length (buffers) < 10) {
    g_cond_wait (check_cond, check_mutex);
  }
  g_mutex_unlock (check_mutex);

  gst_element_set_state (spot, GST_STATE_READY);

  g_list_foreach (buffers, (GFunc) gst_mini_object_unref, NULL);
  g_list_free (buffers);
  buffers = NULL;

  /* cleanup */
  cleanup_spot (spot);
}

GST_END_TEST;

static Suite *
spot_suite (void)
{
  Suite *s = suite_create ("spot");
  TCase *tc_chain = tcase_create ("general");

  suite_add_tcase (s, tc_chain);
  tcase_set_timeout (tc_chain, 90);
  tcase_add_test (tc_chain, test_login_and_play);

  return s;
}

int
main (int argc, char **argv)
{
  int nf;

  Suite *s = spot_suite ();
  SRunner *sr = srunner_create (s);

  gst_check_init (&argc, &argv);

  srunner_run_all (sr, CK_NORMAL);
  nf = srunner_ntests_failed (sr);
  srunner_free (sr);

  return nf;
}
