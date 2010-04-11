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
#define SPOTIFY_URI_2 "spotify:track:13GSFj7uIxqL9eNItNob3p"
#define SPOTIFY_URI_ERROR "spotify:track:deadbeefdeadbeefdeadbeef"
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
  g_list_foreach (buffers, (GFunc) gst_mini_object_unref, NULL);
  g_list_free (buffers);
  buffers = NULL;

  gst_pad_set_active (mysinkpad, FALSE);
  gst_check_teardown_sink_pad (spot);
  gst_check_teardown_element (spot);
}

static void
play_and_verify_buffers (GstElement *spot, int num_buffs)
{
  fail_unless (gst_element_set_state (spot,
				      GST_STATE_PLAYING) == GST_STATE_CHANGE_SUCCESS,
	       "could not set to playing");

  g_mutex_lock (check_mutex);
  while (g_list_length (buffers) < num_buffs) {
    g_cond_wait (check_cond, check_mutex);
  }
  g_mutex_unlock (check_mutex);

  g_list_foreach (buffers, (GFunc) gst_mini_object_unref, NULL);
  g_list_free (buffers);
  buffers = NULL;
}

GST_START_TEST (test_login_and_play_pause)
{
  GstElement *spot;

  g_print ("STARTING TEST LOGIN PLAY PAUSE\n");
  spot = setup_spot();

  g_print ("PLAY\n");
  play_and_verify_buffers (spot, 10);
  g_print ("PAUSE\n");
  fail_unless (gst_element_set_state (spot,
				      GST_STATE_PAUSED) == GST_STATE_CHANGE_SUCCESS,
	       "could not pause element");
  g_print ("PLAY\n");
  play_and_verify_buffers (spot, 10);
  g_print ("PAUSE\n");
  fail_unless (gst_element_set_state (spot,
				      GST_STATE_PAUSED) == GST_STATE_CHANGE_SUCCESS,
	       "could not pause element");
  g_print ("PLAY\n");
  play_and_verify_buffers (spot, 10);
  g_print ("PAUSE\n");
  fail_unless (gst_element_set_state (spot,
				      GST_STATE_PAUSED) == GST_STATE_CHANGE_SUCCESS,
	       "could not pause element");
  g_print ("PLAY\n");
  play_and_verify_buffers (spot, 10);
  g_print ("STOP\n");
  fail_unless (gst_element_set_state (spot,
				      GST_STATE_NULL) == GST_STATE_CHANGE_SUCCESS,
	       "could not pause element");
  g_print ("STOPPED\n");

  /* cleanup */
  cleanup_spot (spot);
  g_print ("SUCCESS TEST LOGIN PLAY PAUSE\n");
}
GST_END_TEST;

GST_START_TEST (test_change_track)
{
  GstElement *spot;

  spot = setup_spot();

  g_print ("STARTING CHANGE TRACK\n");
  g_print ("PLAY\n");
  play_and_verify_buffers (spot, 10);
  g_print ("STOP\n");
  fail_unless (gst_element_set_state (spot,
				      GST_STATE_NULL) == GST_STATE_CHANGE_SUCCESS,
	       "could not pause element");
  g_print ("STOPPED\n");
  g_object_set (G_OBJECT (spot), "spotifyuri", SPOTIFY_URI_2, NULL);
  g_print ("PLAY WITH NEW TRACK\n");
  play_and_verify_buffers (spot, 10);
  g_print ("VERIFIED NEW TRACK\n");

  /* cleanup */
  cleanup_spot (spot);
  g_print ("SUCCESS CHANGE TRACK\n");
}
GST_END_TEST;

GST_START_TEST (test_seek)
{
  GstElement *spot;
  gint64 duration;
  GstFormat format = GST_FORMAT_TIME;

  spot = setup_spot();

  g_print ("STARTING SEEK\n");

  g_print ("SEEK NOT STARTED TRACK\n");
  fail_unless (gst_element_query_duration (spot, &format, &duration));

  play_and_verify_buffers (spot, 10);
  fail_unless (gst_element_query_duration (spot, &format, &duration));

  gst_element_seek_simple (spot, GST_FORMAT_TIME, GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_KEY_UNIT, GST_SECOND * 1);

  /* FIXME: THIS LOCKS SINK. */
  /* play_and_verify_buffers (spot, 10); */

  /* gst_element_seek_simple (spot, GST_FORMAT_TIME, GST_SEEK_FLAG_FLUSH, GST_SECOND * 10); */
  /* gst_element_seek_simple (spot, GST_FORMAT_TIME, GST_SEEK_FLAG_FLUSH, GST_SECOND * 100); */

  /* play_and_verify_buffers (spot, 10); */

  /* gst_element_seek_simple (spot, GST_FORMAT_TIME, GST_SEEK_FLAG_FLUSH, GST_SECOND * 10); */
  /* gst_element_seek_simple (spot, GST_FORMAT_TIME, GST_SEEK_FLAG_FLUSH, duration); */
  /* gst_element_seek_simple (spot, GST_FORMAT_TIME, GST_SEEK_FLAG_FLUSH, duration + GST_SECOND * 10); */

  /* cleanup */
  cleanup_spot (spot);
  g_print ("SUCCESS SEEK\n");
}
GST_END_TEST;

static Suite *
spot_suite (void)
{
  Suite *s = suite_create ("spot");
  TCase *tc_chain = tcase_create ("general");

  suite_add_tcase (s, tc_chain);
  tcase_set_timeout (tc_chain, 20);
  tcase_add_test (tc_chain, test_login_and_play_pause);
  tcase_add_test (tc_chain, test_change_track);
  tcase_add_test (tc_chain, test_seek);

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
