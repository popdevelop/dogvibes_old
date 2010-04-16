/* GStreamer
 * Copyright (C) 2009 Joel and Johan
 *
 * gstspotsrc.c:
 *
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

#include <spotify/api.h>
#include <gst/base/gstadapter.h>
#include <gst/gst.h>

#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <glib.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "gstspotobj.h"
#include "gstspotsrc.h"
#include "config.h"

#define DEFAULT_URI "spotify://spotify:track:3odhGRfxHMVIwtNtc4BOZk"
#define DEFAULT_SPOTIFY_URI "spotify:track:3odhGRfxHMVIwtNtc4BOZk"
#define BUFFER_TIME_MAX 50000000
#define BUFFER_TIME_DEFAULT 2000000
#define SPOTIFY_DEFAULT_SAMPLE_RATE 44100
#define SPOTIFY_DEFAULT_NUMBER_CHANNELS 2

static GstStaticPadTemplate srctemplate = GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("audio/x-raw-int, "
        "endianness = (int) { 1234 }, "
        "signed = (boolean) { TRUE }, "
        "width = (int) 16, "
        "depth = (int) 16, "
        "rate = (int) 44100, channels = (int) 2; ")
    );

GST_DEBUG_CATEGORY_STATIC (gst_spot_src_debug);
GST_DEBUG_CATEGORY_STATIC (gst_spot_src_debug_threads);
GST_DEBUG_CATEGORY_STATIC (gst_spot_src_debug_audio);
GST_DEBUG_CATEGORY_STATIC (gst_spot_src_debug_cb);
#define GST_CAT_DEFAULT gst_spot_src_debug

/* src attributes */
enum
{
  ARG_0,
  ARG_SPOTIFY_URI,
  ARG_USER,
  ARG_PASS,
  ARG_URI,
  ARG_BUFFER_TIME
};

/* src signals */
enum
{
  SIGNAL_PLAY_TOKEN_LOST,
  LAST_SIGNAL
};

static guint gst_spot_signals[LAST_SIGNAL] = { 0 };

/* thread safe functions */
static int run_spot_cmd (GstSpotSrc *spot, enum spot_cmd cmd, gint64 opt);
static void do_end_of_track (GstSpotSrc *spot);

/* libspotify */
static int spotify_cb_music_delivery (sp_session *spotify_session, const sp_audioformat *format, const void *frames, int num_frames);
static void spotify_cb_logged_in (sp_session *spotify_session, sp_error error);
static void spotify_cb_logged_out (sp_session *spotify_session);
static void spotify_cb_connection_error (sp_session *spotify_session, sp_error error);
static void spotify_cb_notify_main_thread (sp_session *spotify_session);
static void spotify_cb_log_message (sp_session *spotify_session, const char *data);
static void spotify_cb_metadata_updated (sp_session *session);
static void spotify_cb_message_to_user (sp_session *session, const char *msg);
static void spotify_cb_play_token_lost (sp_session *session);
static void spotify_cb_end_of_track (sp_session *session);
static void* spotify_thread_func (void *ptr);

/* basesrc stuff */
static void gst_spot_src_finalize (GObject * object);
static void gst_spot_src_set_property (GObject * object, guint prop_id, const GValue * value, GParamSpec * pspec);
static void gst_spot_src_get_property (GObject * object, guint prop_id, GValue * value, GParamSpec * pspec);
static gboolean gst_spot_src_start (GstBaseSrc * basesrc);
static gboolean gst_spot_src_stop (GstBaseSrc * basesrc);
static gboolean gst_spot_src_unlock (GstBaseSrc * basesrc);
static gboolean gst_spot_src_unlock_stop (GstBaseSrc * basesrc);
static gboolean gst_spot_src_is_seekable (GstBaseSrc * src);
static gboolean gst_spot_src_get_size (GstBaseSrc * src, guint64 * size);
static gboolean gst_spot_src_query (GstBaseSrc * src, GstQuery * query);
static GstFlowReturn gst_spot_src_create (GstBaseSrc * src, guint64 offset, guint length, GstBuffer ** buffer);

/* uri interface */
static gboolean gst_spot_src_set_spotifyuri (GstSpotSrc * src, const gchar * spotify_uri);
static gboolean gst_spot_src_uri_set_uri (GstURIHandler * handler, const gchar * uri);
static void gst_spot_src_uri_handler_init (gpointer g_iface, gpointer iface_data);
static const gchar *gst_spot_src_uri_get_uri (GstURIHandler * handler);
static gchar **gst_spot_src_uri_get_protocols (void);
static GstURIType gst_spot_src_uri_get_type (void);

static gboolean spot_obj_create_session (GstSpotSrc *spot, SpotObj *self);
static gboolean spot_obj_login (GstSpotSrc *spot, SpotObj *self);

/* libspotify */
static sp_session_callbacks g_callbacks = {
  &spotify_cb_logged_in,
  &spotify_cb_logged_out,
  &spotify_cb_metadata_updated,
  &spotify_cb_connection_error,
  &spotify_cb_message_to_user,
  &spotify_cb_notify_main_thread,
  &spotify_cb_music_delivery,
  &spotify_cb_play_token_lost,
  &spotify_cb_log_message,
  &spotify_cb_end_of_track
};

static const uint8_t g_appkey[] = {
        0x01, 0x0B, 0x26, 0x66, 0xEA, 0x7A, 0x82, 0x83, 0x61, 0x73, 0x78, 0xC3, 0xAC, 0x7E, 0xF6, 0x62,
        0x6C, 0xF4, 0xF8, 0xCE, 0xF1, 0x61, 0xB7, 0x70, 0x54, 0xB3, 0xE8, 0x8E, 0x3E, 0x32, 0x68, 0x98,
        0xEF, 0x63, 0x42, 0xAC, 0x7E, 0x5B, 0x7C, 0x7C, 0x58, 0xA9, 0x97, 0x5B, 0xEA, 0xBC, 0x9C, 0xFB,
        0x2A, 0x34, 0xA5, 0x17, 0xBD, 0x3B, 0xF2, 0x6A, 0xD2, 0xB4, 0x5F, 0x1C, 0x30, 0x52, 0x49, 0x2C,
        0x03, 0x71, 0xE1, 0x1D, 0xE5, 0xB1, 0x93, 0xEF, 0x6C, 0x38, 0xAB, 0x62, 0x40, 0x9B, 0x10, 0x6A,
        0x31, 0x24, 0x27, 0x77, 0x40, 0x1D, 0x06, 0x1B, 0xE2, 0xA5, 0xA3, 0x55, 0x57, 0x57, 0xD5, 0x12,
        0xAC, 0xDE, 0xB0, 0xBA, 0x48, 0xC3, 0x22, 0x4D, 0xA9, 0x13, 0x13, 0xD9, 0x22, 0x02, 0x87, 0x25,
        0x05, 0x51, 0xEA, 0x91, 0x5A, 0xAE, 0xCA, 0x73, 0x23, 0x0F, 0xC7, 0x7D, 0xCF, 0x80, 0x03, 0x8A,
        0x6F, 0x92, 0xC7, 0x75, 0x21, 0xEC, 0x0E, 0xBE, 0xB7, 0xE3, 0x7C, 0x7F, 0x49, 0x69, 0x30, 0x71,
        0xC9, 0x8A, 0x61, 0x1B, 0x50, 0xAC, 0x92, 0x88, 0x9C, 0x17, 0x21, 0x5F, 0x32, 0xF4, 0xD2, 0x15,
        0x7F, 0xF8, 0x86, 0x11, 0x25, 0x02, 0x53, 0xAA, 0x8D, 0x0C, 0x51, 0x13, 0x51, 0x17, 0x02, 0x10,
        0x86, 0xED, 0x68, 0xCD, 0x19, 0x22, 0x4B, 0x3F, 0xA3, 0x73, 0x6F, 0xD9, 0xDD, 0xAE, 0xAF, 0x85,
        0xD6, 0xF3, 0x08, 0xDB, 0xA7, 0x49, 0x3B, 0x56, 0xD1, 0x77, 0xC8, 0x9B, 0xCA, 0x06, 0x1E, 0xB0,
        0x4A, 0xE9, 0x92, 0xAC, 0x04, 0xAB, 0xDF, 0x90, 0x39, 0x0F, 0xD3, 0xD7, 0x16, 0xEF, 0xA5, 0xFF,
        0xDC, 0x81, 0x3F, 0x09, 0x8D, 0x3D, 0xAC, 0x92, 0x86, 0x21, 0x9F, 0x72, 0x12, 0xA5, 0x1A, 0x6A,
        0xB3, 0x09, 0xEA, 0xCB, 0x3C, 0xCE, 0x73, 0xBD, 0x91, 0x1D, 0x99, 0xCE, 0x45, 0xB6, 0x6F, 0x7E,
        0x6A, 0x99, 0x33, 0x6D, 0x10, 0x11, 0x3F, 0xB4, 0x3E, 0x98, 0xD2, 0x37, 0xDD, 0x35, 0xB9, 0x59,
        0x5E, 0x41, 0x55, 0x9C, 0xC2, 0xFE, 0x72, 0x75, 0x37, 0xEA, 0x7A, 0xCF, 0x4F, 0x49, 0x37, 0x31,
        0xA4, 0x51, 0xC5, 0x0F, 0x42, 0x19, 0x7E, 0x43, 0x71, 0x43, 0x97, 0xE7, 0x76, 0x79, 0xBD, 0x2F,
        0x65, 0x7D, 0x9C, 0x3D, 0x97, 0x7A, 0x76, 0xE0, 0xAE, 0xED, 0x96, 0x74, 0xD5, 0x01, 0x41, 0x61,
        0xAD,
};

static const size_t g_appkey_size = sizeof (g_appkey);

SpotObj *spot_instance;

/* list of spotify commad work structs to be processed */
static GstSpotSrc *ugly_spot;

/*****************************************************************************/
/*** LIBSPOTIFY FUNCTIONS ****************************************************/

static void
spotify_cb_connection_error (sp_session *spotify_session, sp_error error)
{
  GST_CAT_ERROR_OBJECT (gst_spot_src_debug_cb, ugly_spot, "Connection_error callback %s",
      sp_error_message (error));
}

static void
spotify_cb_end_of_track (sp_session *session)
{
  GST_CAT_DEBUG_OBJECT (gst_spot_src_debug_cb, ugly_spot, "End_of_track callback");
  g_mutex_lock (GST_SPOT_SRC_ADAPTER_MUTEX (ugly_spot));
  ugly_spot->end_of_track = TRUE;
  g_cond_broadcast (GST_SPOT_SRC_ADAPTER_COND (ugly_spot));
  g_mutex_unlock (GST_SPOT_SRC_ADAPTER_MUTEX (ugly_spot));
}

static void
spotify_cb_log_message (sp_session *spotify_session, const char *data)
{
  GST_CAT_DEBUG_OBJECT (gst_spot_src_debug_cb, ugly_spot, "Log_message callback, data='%s'", data);
}

static void
spotify_cb_logged_in (sp_session *spotify_session, sp_error error)
{
  if (SP_ERROR_OK != error) {
    GST_CAT_ERROR_OBJECT (gst_spot_src_debug_cb, ugly_spot, "Failed to log in to Spotify: %s\n", sp_error_message (error));
    return;
  }

  sp_user *me = sp_session_user (spotify_session);
  const char *my_name = (sp_user_is_loaded (me) ?
                         sp_user_display_name (me) :
                         sp_user_canonical_name (me));
  GST_CAT_DEBUG_OBJECT (gst_spot_src_debug_cb, ugly_spot, "Logged_in callback, user=%s", my_name);
  SPOT_OBJ_LOGGED_IN (spot_instance) = TRUE;
}

static void
spotify_cb_logged_out (sp_session *spotify_session)
{
  GST_CAT_DEBUG_OBJECT (gst_spot_src_debug_cb, ugly_spot, "Logged_out callback");
}

static void
spotify_cb_notify_main_thread (sp_session *spotify_session)
{
  GST_CAT_LOG_OBJECT (gst_spot_src_debug_cb, ugly_spot, "Notify_main_thread callback");
  GST_CAT_DEBUG_OBJECT (gst_spot_src_debug_threads, ugly_spot, "Broadcast process_events_cond");
  g_cond_broadcast (ugly_spot->process_events_cond);
}

static int
spotify_cb_music_delivery (sp_session *spotify_session, const sp_audioformat *format,const void *frames, int num_frames)
{
  GstBuffer *buffer;
  guint sample_rate = format->sample_rate;
  guint channels = format->channels;
  guint bufsize = num_frames * sizeof (int16_t) * channels;
  guint availible;

  GST_CAT_DEBUG_OBJECT (gst_spot_src_debug_cb, ugly_spot, "Music_delivery callback");

  GST_SPOT_SRC_FORMAT (ugly_spot)->sample_rate = sample_rate;
  GST_SPOT_SRC_FORMAT (ugly_spot)->channels = channels;
  GST_SPOT_SRC_FORMAT (ugly_spot)->sample_type = format->sample_type;

  GST_CAT_LOG_OBJECT (gst_spot_src_debug_audio, ugly_spot, "Start %p with %d frames with size=%d\n", frames, num_frames, bufsize);

  if (num_frames == 0) {
    /* we have a seek */
    return 0;
  }

  buffer = gst_buffer_new_and_alloc (bufsize);

  memcpy (GST_BUFFER_DATA (buffer), (guint8*)frames, bufsize);

  g_mutex_lock (GST_SPOT_SRC_ADAPTER_MUTEX (ugly_spot));
  availible = gst_adapter_available (GST_SPOT_SRC_ADAPTER (ugly_spot));
  GST_CAT_LOG_OBJECT (gst_spot_src_debug_audio, ugly_spot, "Availiable before push = %d\n", availible);
  /* see if we have buffertime of audio */
  if (availible >= (GST_SPOT_SRC_BUFFER_TIME (ugly_spot)/1000000) * sample_rate * 4) {
    GST_CAT_LOG_OBJECT (gst_spot_src_debug_audio, ugly_spot, "Return 0, adapter is full = %d\n", availible);
    gst_buffer_unref (buffer);
    /* data is available broadcast read thread */
    g_cond_broadcast (GST_SPOT_SRC_ADAPTER_COND (ugly_spot));
    g_mutex_unlock (GST_SPOT_SRC_ADAPTER_MUTEX (ugly_spot));
    return 0;
  }

  gst_adapter_push (GST_SPOT_SRC_ADAPTER (ugly_spot), buffer);
  availible = gst_adapter_available (GST_SPOT_SRC_ADAPTER (ugly_spot));
  GST_CAT_LOG_OBJECT (gst_spot_src_debug_audio, ugly_spot, "Availiable after push = %d\n", availible);
  /* data is available broadcast read thread */
  g_cond_broadcast (GST_SPOT_SRC_ADAPTER_COND (ugly_spot));
  g_mutex_unlock (GST_SPOT_SRC_ADAPTER_MUTEX (ugly_spot));
  GST_CAT_LOG_OBJECT (gst_spot_src_debug_audio, ugly_spot, "Return num_frames=%d\n", num_frames);
  return num_frames;
}

static void
spotify_cb_metadata_updated (sp_session *session)
{
  GST_CAT_DEBUG_OBJECT (gst_spot_src_debug_cb, ugly_spot, "Metadata_updated callback");
}

static void
spotify_cb_message_to_user (sp_session *session, const char *msg)
{
  GST_CAT_DEBUG_OBJECT (gst_spot_src_debug_cb, ugly_spot, "Message_to_user callback, msg='%s'", msg);
}

static void
spotify_cb_play_token_lost (sp_session *session)
{
  GST_CAT_ERROR_OBJECT (gst_spot_src_debug_cb, ugly_spot, "Play_token_lost callback");
  ugly_spot->play_token_lost = TRUE;
  g_signal_emit (ugly_spot, gst_spot_signals[SIGNAL_PLAY_TOKEN_LOST], 0);
}


/*****************************************************************************/
/*** SPOTIFY THREAD FUNCTIONS ************************************************/

static void
do_end_of_track (GstSpotSrc *spot)
{
  GstPad *src_pad = gst_element_get_static_pad (GST_ELEMENT (spot), "src");
  GstPad *peer_pad = gst_pad_get_peer (src_pad);
  gst_pad_send_event (peer_pad, gst_event_new_eos ());
  spot->end_of_track = FALSE;
  gst_object_unref (peer_pad);
}

static int
run_spot_cmd (GstSpotSrc *spot, enum spot_cmd cmd, gint64 opt)
{
  struct spot_work *spot_work;
  int ret;

  /* create work struct */
  spot_work = g_new0 (struct spot_work, 1);
  spot_work->spot_cond = g_cond_new ();
  spot_work->spot_mutex = g_mutex_new ();
  spot_work->cmd = cmd;
  spot_work->ret = 0;
  spot_work->opt = opt;

  /* add work struct to list of works */
  g_mutex_lock (spot->process_events_mutex);
  spot->spot_works = g_list_append (spot->spot_works, spot_work);
  g_mutex_unlock (spot->process_events_mutex);

  /* wait for processing */
  g_mutex_lock (spot_work->spot_mutex);
  GST_CAT_DEBUG_OBJECT (gst_spot_src_debug_threads, spot, "Broadcast process_events_cond");
  g_cond_broadcast (spot->process_events_cond);
  g_cond_wait (spot_work->spot_cond, spot_work->spot_mutex);
  g_mutex_unlock (spot_work->spot_mutex);

  /* save return value */
  ret = spot_work->ret;

  /* remove work struct */
  g_cond_free (spot_work->spot_cond);
  g_mutex_free (spot_work->spot_mutex);
  g_free (spot_work);

  return ret;
}

/* only used to trigger sp_session_process_events when needed,
 * looks like about once a second */
static void*
spotify_thread_func (void *data)
{
  int timeout = -1;
  GTimeVal t;
  GstSpotSrc *spot = (GstSpotSrc *) data;

  if (!spot_obj_create_session (spot, spot_instance)) {
    GST_ERROR_OBJECT (spot, "Spot_obj_create_session error");
    return FALSE;
  }

  while (spot->keep_spotify_thread) {
    sp_session_process_events (SPOT_OBJ_SPOTIFY_SESSION (spot_instance), &timeout);
    g_get_current_time (&t);
    g_time_val_add (&t, timeout * 1000);
    g_cond_timed_wait (spot->process_events_cond, spot->process_events_mutex, &t);
    spot->spotify_thread_initiated = TRUE;
    while (spot->spot_works) {
      struct spot_work *spot_work;
      int ret;
      spot_work = (struct spot_work *)spot->spot_works->data;
      g_mutex_lock (spot_work->spot_mutex);
      switch (spot_work->cmd) {
        case SPOT_CMD_START:
          GST_DEBUG_OBJECT (spot, "Uri = %s", SPOT_OBJ_SPOTIFY_URI (spot_instance));
          if (!spot_obj_login (spot, spot_instance)) {
            /* error message from within function */
            spot_work->ret = -1;
            goto work_error;
          }

          sp_link *link = sp_link_create_from_string (SPOT_OBJ_SPOTIFY_URI (spot_instance));

          if (!link) {
            GST_ERROR_OBJECT (spot, "Incorrect track ID:%s", SPOT_OBJ_SPOTIFY_URI (spot_instance));
            spot_work->ret = -1;
            goto work_error;
          }

          SPOT_OBJ_CURRENT_TRACK (spot_instance) = sp_link_as_track (link);

          if (!SPOT_OBJ_CURRENT_TRACK (spot_instance)) {
            GST_ERROR_OBJECT (spot, "Could get track from uri=%s", SPOT_OBJ_SPOTIFY_URI (spot_instance));
            spot_work->ret = -1;
            goto work_error;
          }

          if (!sp_track_is_available (SPOT_OBJ_CURRENT_TRACK (spot_instance))) {
            /* this probably happens for tracks avaiable in other countries or
               something */
            GST_ERROR_OBJECT (spot, "Track is not available, uri=%s", SPOT_OBJ_SPOTIFY_URI (spot_instance));
            spot_work->ret = -1;
            goto work_error;
          }

          sp_track_add_ref (SPOT_OBJ_CURRENT_TRACK (spot_instance));
          sp_link_add_ref (link);

          sp_session_process_events (SPOT_OBJ_SPOTIFY_SESSION (spot_instance), &timeout);
          while (sp_track_is_loaded (SPOT_OBJ_CURRENT_TRACK (spot_instance)) == 0) {
            sp_session_process_events (SPOT_OBJ_SPOTIFY_SESSION (spot_instance), &timeout);
            usleep (10000);
          }

          GST_DEBUG_OBJECT (spot, "Now playing \"%s\"", sp_track_name (SPOT_OBJ_CURRENT_TRACK (spot_instance)));

          ret = sp_session_player_load (SPOT_OBJ_SPOTIFY_SESSION (spot_instance), SPOT_OBJ_CURRENT_TRACK (spot_instance));
          if (ret != SP_ERROR_OK) {
            GST_ERROR_OBJECT (spot, "Failed to load track '%s' uri=%s", sp_track_name (SPOT_OBJ_CURRENT_TRACK (spot_instance)),
                (SPOT_OBJ_SPOTIFY_URI (spot_instance)));
            spot_work->ret = -1;
            goto work_error;
          }

          sp_session_process_events (SPOT_OBJ_SPOTIFY_SESSION (spot_instance), &timeout);
          ret = sp_session_player_play (SPOT_OBJ_SPOTIFY_SESSION (spot_instance), TRUE);
          if (ret != SP_ERROR_OK) {
            GST_ERROR_OBJECT (spot, "Failed to play track '%s' uri=%s", sp_track_name (SPOT_OBJ_CURRENT_TRACK (spot_instance)),
                (SPOT_OBJ_SPOTIFY_URI (spot_instance)));
            spot_work->ret = -1;
            goto work_error;
          }
          break;
        case SPOT_CMD_PROCESS:
          sp_session_process_events (SPOT_OBJ_SPOTIFY_SESSION (spot_instance), &timeout);
          break;

        case SPOT_CMD_PLAY:
          sp_session_player_play (SPOT_OBJ_SPOTIFY_SESSION (spot_instance), TRUE);
          break;

        case SPOT_CMD_DURATION:
          if (SPOT_OBJ_LOGGED_IN (spot_instance)) {
            spot_work->ret = sp_track_duration (SPOT_OBJ_CURRENT_TRACK (spot_instance));
          }
          break;

        case SPOT_CMD_STOP:
          if (SPOT_OBJ_LOGGED_IN (spot_instance)) {
            sp_session_player_play (SPOT_OBJ_SPOTIFY_SESSION (spot_instance), FALSE);
            sp_session_player_unload (SPOT_OBJ_SPOTIFY_SESSION (spot_instance));
          }
          break;

        case SPOT_CMD_SEEK:
          if (SPOT_OBJ_LOGGED_IN (spot_instance)) {
            spot_work->ret = sp_session_player_seek (SPOT_OBJ_SPOTIFY_SESSION (spot_instance), spot_work->opt);
          }
          break;
        default:
          g_assert_not_reached ();
          break;

      }

work_error:
      spot->spot_works = g_list_remove (spot->spot_works, spot->spot_works->data);
      g_mutex_unlock (spot_work->spot_mutex);
      g_cond_broadcast (spot_work->spot_cond);
    }
  }

  return NULL;
}

/*****************************************************************************/
/*** SPOTOBJ FUNCTIONS *******************************************************/

enum {
        SPOT_OBJ_ARG_USER = 1,
        SPOT_OBJ_ARG_PASS,
        SPOT_OBJ_ARG_SPOTIFY_URI,
};

static GObjectClass *spot_obj_parent_class = NULL;

static gboolean spot_obj_create_session (GstSpotSrc *spot, SpotObj *self)
{
  sp_session_config config;
  sp_error error;

  config.api_version = SPOTIFY_API_VERSION;
  //FIXME check if these paths are appropiate
  config.cache_location = "tmp";
  config.settings_location = "tmp";
  config.application_key = g_appkey;
  config.application_key_size = g_appkey_size;
  config.user_agent = "spotify-gstreamer-src";
  config.callbacks = &g_callbacks;

  error = sp_session_init (&config, &SPOT_OBJ_SPOTIFY_SESSION (spot_instance));

  if (SP_ERROR_OK != error) {
    GST_ERROR_OBJECT ("Failed to create spotify_session: %s", sp_error_message (error));
    return FALSE;
  }

  GST_DEBUG_OBJECT (spot, "Created spotify session");
  return TRUE;
}

static gboolean spot_obj_login (GstSpotSrc *spot, SpotObj *self)
{
  sp_error error;
  if (SPOT_OBJ_LOGGED_IN (spot_instance)) {
    GST_DEBUG_OBJECT (spot, "Already logged in\n");
    return TRUE;
  }

  GST_DEBUG_OBJECT (spot, "Trying to login\n");

  /* Login using the credentials given on the command line */
  error = sp_session_login (SPOT_OBJ_SPOTIFY_SESSION (spot_instance), SPOT_OBJ_USER (spot_instance) , SPOT_OBJ_PASS (spot_instance));

  if (SP_ERROR_OK != error) {
    GST_ERROR_OBJECT (spot, "Failed to login: %s\n", sp_error_message (error));
    return FALSE;
  }
  int timeout = -1;

  sp_session_process_events (SPOT_OBJ_SPOTIFY_SESSION (spot_instance), &timeout);
  while (!SPOT_OBJ_LOGGED_IN (spot_instance)) {
    usleep (10000);
    sp_session_process_events (SPOT_OBJ_SPOTIFY_SESSION (spot_instance), &timeout);
  }

  GST_DEBUG_OBJECT (spot, "Login ok!\n");

 return TRUE;
}


static void
spot_obj_instance_init (GTypeInstance *instance, gpointer g_class)
{
  SpotObj *self = (SpotObj *)instance;
  self->user = NULL;
  self->pass = NULL;
  self->spotify_uri = NULL;
}


static void
spot_obj_set_property (GObject *object, guint property_id, const GValue *value, GParamSpec *pspec)
{
  SpotObj *self = (SpotObj *) object;

  switch (property_id) {
  case SPOT_OBJ_ARG_USER:
    g_free (SPOT_OBJ_USER (self));
    SPOT_OBJ_USER (self) = g_strdup (g_value_get_string (value));
    break;
  case SPOT_OBJ_ARG_PASS:
    g_free (SPOT_OBJ_PASS (self));
    SPOT_OBJ_PASS (self) = g_strdup (g_value_get_string (value));
    break;
  case SPOT_OBJ_ARG_SPOTIFY_URI:
    g_free (SPOT_OBJ_SPOTIFY_URI (self));
    SPOT_OBJ_SPOTIFY_URI (self) = g_strdup (g_value_get_string (value));
    GST_DEBUG_OBJECT (ugly_spot, "Set spotifyuri=%s", g_value_get_string (value));
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object,property_id,pspec);
    break;
  }
}

static void
spot_obj_get_property (GObject *object, guint property_id, GValue *value, GParamSpec *pspec)
{
  SpotObj *self = (SpotObj *) object;

  switch (property_id) {
  case SPOT_OBJ_ARG_USER:
    g_value_set_string (value, SPOT_OBJ_USER (self));
    break;
  case SPOT_OBJ_ARG_PASS:
    g_value_set_string (value, SPOT_OBJ_PASS (self));
    break;
  case SPOT_OBJ_ARG_SPOTIFY_URI:
    g_value_set_string (value, SPOT_OBJ_SPOTIFY_URI (self));
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object,property_id,pspec);
    break;
  }
}

static GObject *
spot_obj_constructor (GType type, guint n_construct_properties, GObjectConstructParam *construct_properties)
{
  GObject *obj;

  {
    /* Invoke parent constructor. */
    SpotObjClass *klass;
    klass = SPOT_OBJ_CLASS (g_type_class_peek (SPOT_OBJ_TYPE));
    obj = spot_obj_parent_class->constructor (type,
                                              n_construct_properties,
                                              construct_properties);
  }

  return obj;
}

static void
spot_obj_dispose (GObject *obj)
{

  /*
   * In dispose, you are supposed to free all types referenced from this
   * object which might themselves hold a reference to self. Generally,
   * the most simple solution is to unref all members on which you own a
   * reference.
   */

  /* Chain up to the parent class */
  GST_DEBUG_OBJECT (ugly_spot, "Dispose");
  G_OBJECT_CLASS (spot_obj_parent_class)->dispose (obj);
}

static void
spot_obj_finalize (GObject *obj)
{
  SpotObj *self = (SpotObj *)obj;

  /*
   * Here, complete object destruction.
   * You might not need to do much...
   */
  g_free (self->user);
  g_free (self->pass);
  g_free (self->spotify_uri);

  /* Chain up to the parent class */
  GST_DEBUG_OBJECT (ugly_spot, "Finalize");
  G_OBJECT_CLASS (spot_obj_parent_class)->finalize (obj);
}


static void
spot_obj_class_init (gpointer g_class,
                      gpointer g_class_data)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (g_class);
  SpotObjClass *klass = SPOT_OBJ_CLASS (g_class);

  gobject_class->set_property = spot_obj_set_property;
  gobject_class->get_property = spot_obj_get_property;
  gobject_class->dispose = spot_obj_dispose;
  gobject_class->finalize = spot_obj_finalize;
  gobject_class->constructor = spot_obj_constructor;

  spot_obj_parent_class = g_type_class_peek_parent (klass);


  g_object_class_install_property (gobject_class, SPOT_OBJ_ARG_USER,
      g_param_spec_string ("user", "Username", "Username for premium spotify account", "unknown",
          G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, SPOT_OBJ_ARG_PASS,
      g_param_spec_string ("pass", "Password", "Password for premium spotify account", "unknown",
          G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, SPOT_OBJ_ARG_SPOTIFY_URI,
      g_param_spec_string ("spotifyuri", "Spotify URI", "Spotify URI", "unknown",
          G_PARAM_READWRITE));
}

GType spot_obj_get_type (void)
{
  static GType type = 0;
  if (type == 0) {
    static const GTypeInfo info = {
      sizeof (SpotObjClass),
      NULL,                  /* base_init */
      NULL,                  /* base_finalize */
      spot_obj_class_init,   /* class_init */
      NULL,                  /* class_finalize */
      NULL,                  /* class_data */
      sizeof (SpotObj),
      0,                     /* n_preallocs */
      spot_obj_instance_init /* instance_init */
    };
    type = g_type_register_static (G_TYPE_OBJECT,
                                   "SpotObjType",
                                   &info, 0);
  }
  return type;
}

/*****************************************************************************/
/*** BASESRC FUNCTIONS *******************************************************/

static void
_do_init (GType spotsrc_type)
{
  static const GInterfaceInfo urihandler_info = {
    gst_spot_src_uri_handler_init,
    NULL,
    NULL
  };

  g_type_add_interface_static (spotsrc_type, GST_TYPE_URI_HANDLER,
      &urihandler_info);

 /* How the debug system works:
  *
  * GST_LOG (level 5)
  * GST_INFO (level 4)
  * GST_DEBUG (level 3)
  * GST_WARNING (level 2)
  * GST_ERROR (level 1)
  *
  * To make the debug easier to follow, use a higher level for messages
  * that are printed very often. The combination of levels and categories should
  * make it easy to filter out the correct information.
  *
  * GST_DEBUG=spot*:2,spot_audio:3,spot_threads:5 for example,
  * this will give you all errors and warnings, some info on from
  * audio parts and all info for the threads
  *
  */

  GST_DEBUG_CATEGORY_INIT (gst_spot_src_debug, "SPOTSRC", 0, "spotsrc element");
  GST_DEBUG_CATEGORY_INIT (gst_spot_src_debug_threads, "SPOTSRC_THREADS", 0, "spotsrc element mutex/cond debug");
  GST_DEBUG_CATEGORY_INIT (gst_spot_src_debug_audio, "SPOTSRC_AUDIO", 0, "spotsrc element audio debug");
  GST_DEBUG_CATEGORY_INIT (gst_spot_src_debug_cb, "SPOTSRC_CB", 0, "spotsrc libspotify callbacks debug");
}

GST_BOILERPLATE_FULL (GstSpotSrc, gst_spot_src, GstBaseSrc, GST_TYPE_BASE_SRC,
    _do_init);

static void
gst_spot_src_base_init (gpointer g_class)
{

  GstElementClass *gstelement_class = GST_ELEMENT_CLASS (g_class);

  gst_element_class_set_details_simple (gstelement_class,
      "Spot Source",
      "Source/spot",
      "Read from arbitrary point in a file with raw audio",
      "tilljoel@gmail.com (http://hackr.se) & johan.gyllenspetz@gmail.com");
  gst_element_class_add_pad_template (gstelement_class,
      gst_static_pad_template_get (&srctemplate));
}

static void
gst_spot_src_class_init (GstSpotSrcClass * klass)
{
  GObjectClass *gobject_class;
  GstBaseSrcClass *gstbasesrc_class;
  gobject_class = G_OBJECT_CLASS (klass);
  gstbasesrc_class = GST_BASE_SRC_CLASS (klass);

  gobject_class->set_property = gst_spot_src_set_property;
  gobject_class->get_property = gst_spot_src_get_property;

  gobject_class->finalize = GST_DEBUG_FUNCPTR (gst_spot_src_finalize);

  gstbasesrc_class->start = GST_DEBUG_FUNCPTR (gst_spot_src_start);
  gstbasesrc_class->stop = GST_DEBUG_FUNCPTR (gst_spot_src_stop);
  gstbasesrc_class->unlock = GST_DEBUG_FUNCPTR (gst_spot_src_unlock);
  gstbasesrc_class->unlock_stop = GST_DEBUG_FUNCPTR (gst_spot_src_unlock_stop);
  gstbasesrc_class->is_seekable = GST_DEBUG_FUNCPTR (gst_spot_src_is_seekable);
  gstbasesrc_class->get_size = GST_DEBUG_FUNCPTR (gst_spot_src_get_size);
  gstbasesrc_class->create = GST_DEBUG_FUNCPTR (gst_spot_src_create);
  gstbasesrc_class->query = GST_DEBUG_FUNCPTR (gst_spot_src_query);

  g_object_class_install_property (gobject_class, ARG_USER,
      g_param_spec_string ("user", "Username", "Username for premium spotify account", "unknown",
          G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, ARG_PASS,
      g_param_spec_string ("pass", "Password", "Password for premium spotify account", "unknown",
          G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, ARG_URI,
      g_param_spec_string ("uri", "URI", "A URI", "unknown",
          G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, ARG_SPOTIFY_URI,
      g_param_spec_string ("spotifyuri", "Spotify URI", "A spotify URI", "unknown",
          G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, ARG_BUFFER_TIME,
      g_param_spec_uint64 ("buffer-time", "buffer time in us", "buffer time in us",
                      0,BUFFER_TIME_MAX,BUFFER_TIME_DEFAULT,
                      G_PARAM_READWRITE));

  gst_spot_signals[SIGNAL_PLAY_TOKEN_LOST] =
      g_signal_new ("play-token-lost", G_TYPE_FROM_CLASS (klass), G_SIGNAL_RUN_FIRST,
      0, NULL, NULL,
      g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);
}

static void
gst_spot_src_init (GstSpotSrc * spot, GstSpotSrcClass * g_class)
{
  GError *err;
  spot->read_position = 0;

  /* its childish to use static global variables */
  ugly_spot = GST_SPOT_SRC (spot);

  GST_SPOT_SRC_URI (spot) = g_strdup (DEFAULT_URI);

  GST_SPOT_SRC_BUFFER_TIME (spot) = BUFFER_TIME_DEFAULT;

  GST_SPOT_SRC_ADAPTER_MUTEX (spot) = g_mutex_new ();
  GST_SPOT_SRC_ADAPTER_COND (spot) = g_cond_new ();
  GST_SPOT_SRC_ADAPTER (spot) = gst_adapter_new ();

  /* initiate format to default format */
  GST_SPOT_SRC_FORMAT (spot) = g_malloc0 (sizeof (sp_audioformat));
  GST_SPOT_SRC_FORMAT (spot)->sample_rate = SPOTIFY_DEFAULT_SAMPLE_RATE;
  GST_SPOT_SRC_FORMAT (spot)->channels = SPOTIFY_DEFAULT_NUMBER_CHANNELS;
  GST_SPOT_SRC_FORMAT (spot)->sample_type = SP_SAMPLETYPE_INT16_NATIVE_ENDIAN;

  spot_instance = g_object_new (SPOT_OBJ_TYPE, NULL);

  /* initiate state varables */
  spot->spot_works = NULL;
  spot->play_token_lost = FALSE;
  spot->end_of_track = FALSE;
  spot->unlock_state = FALSE;

  /* intiate worker thread and its state variables */
  spot->keep_spotify_thread = TRUE;
  spot->spotify_thread_initiated = FALSE;
  spot->process_events_mutex = g_mutex_new ();
  spot->process_events_cond = g_cond_new ();
  spot->process_events_thread = g_thread_create ((GThreadFunc)spotify_thread_func, spot, TRUE, &err);

  if (spot->process_events_thread == NULL) {
     GST_CAT_ERROR_OBJECT (gst_spot_src_debug_threads, spot,"G_thread_create failed: %s!\n", err->message );
     g_error_free (err) ;
  }

  /* make sure spotify thread is up and running, before continuing. */
  GST_CAT_DEBUG_OBJECT (gst_spot_src_debug_threads, spot, "Broadcast process_events_cond");
  g_cond_broadcast (spot->process_events_cond);
  while (!spot->spotify_thread_initiated) {
    /* ugly but hey it yields right. */
    usleep (40);
    GST_CAT_DEBUG_OBJECT (gst_spot_src_debug_threads, spot, "Broadcast process_events_cond, in loop");
    g_cond_broadcast (spot->process_events_cond);
  }
}

static void
gst_spot_src_finalize (GObject * object)
{
  GstSpotSrc *spot;

  spot = GST_SPOT_SRC (object);

  /* Make thread quit. */
  g_mutex_lock (spot->process_events_mutex);
  spot->keep_spotify_thread = FALSE;
  GST_CAT_DEBUG_OBJECT (gst_spot_src_debug_threads, spot, "Broadcast process_events_cond");
  g_cond_broadcast (spot->process_events_cond);
  g_mutex_unlock (spot->process_events_mutex);
  g_thread_join (spot->process_events_thread);

  g_object_unref (spot_instance);

  g_free (GST_SPOT_SRC_URI (spot));
  g_free (GST_SPOT_SRC_FORMAT (spot));

  g_list_free (spot->spot_works);

  g_cond_free (spot->process_events_cond);
  g_mutex_free (spot->process_events_mutex);

  g_mutex_free (GST_SPOT_SRC_ADAPTER_MUTEX (spot));
  g_cond_free (GST_SPOT_SRC_ADAPTER_COND (spot));
  g_object_unref (GST_SPOT_SRC_ADAPTER (spot));

  G_OBJECT_CLASS (parent_class)->finalize (object);

}

static void
gst_spot_src_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  GstSpotSrc *spot;

  g_return_if_fail (GST_IS_SPOT_SRC (object));

  spot = GST_SPOT_SRC (object);

  switch (prop_id) {
    case ARG_USER:
      g_object_set (spot_instance, "user", g_value_get_string (value), NULL);
      break;
    case ARG_PASS:
      g_object_set (spot_instance, "pass", g_value_get_string (value), NULL);
      break;
    case ARG_URI:
      //FIXME: how do handle error from this func?
      gst_spot_src_set_spotifyuri (spot, g_value_get_string (value));
      break;
    case ARG_SPOTIFY_URI:
      g_object_set (spot_instance, "spotifyuri", g_value_get_string (value), NULL);
      break;
    case ARG_BUFFER_TIME:
      GST_SPOT_SRC_BUFFER_TIME (spot) = (g_value_get_uint64 (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_spot_src_get_property (GObject * object, guint prop_id, GValue * value,
    GParamSpec * pspec)
{
  GstSpotSrc *spot;

  g_return_if_fail (GST_IS_SPOT_SRC (object));

  spot = GST_SPOT_SRC (object);

  switch (prop_id) {
    case ARG_USER:
      g_value_set_string (value, SPOT_OBJ_USER (spot_instance));
      break;
    case ARG_PASS:
      g_value_set_string (value, SPOT_OBJ_PASS (spot_instance));
      break;
    case ARG_SPOTIFY_URI:
      g_value_set_string (value, SPOT_OBJ_SPOTIFY_URI (spot_instance));
      break;
    case ARG_URI:
      g_value_set_string (value, GST_SPOT_SRC_URI (spot));
      break;
    case ARG_BUFFER_TIME:
      g_value_set_uint64 (value, GST_SPOT_SRC_BUFFER_TIME (spot));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static GstFlowReturn
gst_spot_src_create_read (GstSpotSrc * spot, guint64 offset, guint length, GstBuffer ** buffer)
{
  if (spot->unlock_state) {
    return GST_FLOW_WRONG_STATE;
  }

  if (G_UNLIKELY (spot->read_position != offset)) {
    sp_error error;
    /* implement spotify seek here */
    gint sample_rate = GST_SPOT_SRC_FORMAT (spot)->sample_rate;
    gint channels = GST_SPOT_SRC_FORMAT (spot)->channels;
    gint64 frames = offset / (channels * sizeof (int16_t));
    gint64 seek_usec = frames / ((float)sample_rate / 1000);

    GST_CAT_DEBUG_OBJECT (gst_spot_src_debug_audio, spot,
        "Seek_usec = (%" G_GINT64_FORMAT ") = frames (%" G_GINT64_FORMAT ") /  sample_rate (%d/1000)\n",
        seek_usec, frames, sample_rate);
    GST_CAT_DEBUG_OBJECT (gst_spot_src_debug_audio, spot,
        "Perform seek to %" G_GINT64_FORMAT " bytes and %" G_GINT64_FORMAT " usec\n",
        offset, seek_usec);

    error = run_spot_cmd (spot, SPOT_CMD_SEEK, seek_usec);
    g_mutex_lock (GST_SPOT_SRC_ADAPTER_MUTEX (spot));
    gst_adapter_clear (GST_SPOT_SRC_ADAPTER (spot));
    g_mutex_unlock (GST_SPOT_SRC_ADAPTER_MUTEX (spot));
    run_spot_cmd (spot, SPOT_CMD_PLAY, 0);

    if (error != SP_ERROR_OK) {
      GST_CAT_ERROR_OBJECT (gst_spot_src_debug_audio, spot, "Seek failed");
      goto create_seek_failed;
    }
    spot->read_position = offset;
  }

  /* see if we have bytes to write */
  GST_CAT_DEBUG_OBJECT (gst_spot_src_debug_audio, spot, "Length=%u offset=%" G_GINT64_FORMAT " end=%" G_GINT64_FORMAT " read_position=%" G_GINT64_FORMAT,
                    length, offset, offset+length, spot->read_position);

  g_mutex_lock (GST_SPOT_SRC_ADAPTER_MUTEX (spot));
  while (1) {
    *buffer = gst_adapter_take_buffer (GST_SPOT_SRC_ADAPTER (spot), length);
    if (*buffer) {
      spot->read_position += length;
      GST_BUFFER_SIZE (*buffer) = length;
      GST_BUFFER_OFFSET (*buffer) = offset;
      GST_BUFFER_OFFSET_END (*buffer) = offset + length;
      g_mutex_unlock (GST_SPOT_SRC_ADAPTER_MUTEX (spot));
      return GST_FLOW_OK;
    }
    if (spot->end_of_track) {
      g_mutex_unlock (GST_SPOT_SRC_ADAPTER_MUTEX (spot));
      do_end_of_track (spot);
      GST_CAT_DEBUG_OBJECT (gst_spot_src_debug_audio, spot, "End of track\n");
      return GST_FLOW_WRONG_STATE;
    }
    //should be used in a tight conditional while
    g_cond_wait (GST_SPOT_SRC_ADAPTER_COND (spot), GST_SPOT_SRC_ADAPTER_MUTEX (spot));
    if (spot->unlock_state) {
      g_mutex_unlock (GST_SPOT_SRC_ADAPTER_MUTEX (spot));
      return GST_FLOW_WRONG_STATE;
    }
  }

  GST_CAT_ERROR_OBJECT (gst_spot_src_debug_audio, spot, "Create_read failed");

  create_seek_failed:
  return GST_FLOW_ERROR;
}

static GstFlowReturn
gst_spot_src_create (GstBaseSrc * basesrc, guint64 offset, guint length, GstBuffer ** buffer)
{
  GstSpotSrc *spot;
  GstFlowReturn ret;
  spot = GST_SPOT_SRC (basesrc);

  if (spot->play_token_lost) {
    run_spot_cmd (spot, SPOT_CMD_PLAY, 0);
    spot->play_token_lost = FALSE;
  }

  ret = gst_spot_src_create_read (spot, offset, length, buffer);

  return ret;
}

static gboolean
gst_spot_src_query (GstBaseSrc * basesrc, GstQuery * query)
{
  gboolean ret = TRUE;
  GstSpotSrc *spot = GST_SPOT_SRC (basesrc);
  gint samplerate = GST_SPOT_SRC_FORMAT (spot)->sample_rate;
  gint64 src_val, dest_val;

  if (!GST_SPOT_SRC_FORMAT (spot)) {
    ret = FALSE;
    goto no_format_yet;
  }

  samplerate = GST_SPOT_SRC_FORMAT (spot)->sample_rate;

  switch (GST_QUERY_TYPE (query)) {
    case GST_QUERY_URI:
      gst_query_set_uri (query, spot->uri);
    break;

    case GST_QUERY_DURATION:{
      GstFormat format;
      gint duration;
      gint64 value;
      gst_query_parse_duration (query, &format, &value);
      /* duration in ms */

      duration = run_spot_cmd (spot, SPOT_CMD_DURATION, 0);
      switch (format) {
        case GST_FORMAT_BYTES:
          {
          guint64 duration_bytes = (duration/1000) * samplerate * 4;
          GST_LOG_OBJECT (spot, "Query_duration, duration_bytes=%" G_GUINT64_FORMAT, duration_bytes);
          gst_query_set_duration (query, format, duration_bytes);
          }
          break;
        case GST_FORMAT_TIME:
          {
          /* set it to nanoseconds */
          guint64 duration_time = duration * 1000;
          GST_LOG_OBJECT (spot, "Query_duration, duration_time=%" G_GUINT64_FORMAT, duration_time);
          gst_query_set_duration (query, format, duration_time);
          }
          break;
        default:
          ret = FALSE;
          g_assert_not_reached ();
          break;
      }
      break;
    }
    case GST_QUERY_CONVERT:
      {
        GstFormat src_fmt, dest_fmt;

        gst_query_parse_convert (query, &src_fmt, &src_val, &dest_fmt, &dest_val);
        GST_LOG_OBJECT (spot, "Convert src_fmt %d dst_fmt %d", src_fmt, dest_fmt);

        if (src_fmt == dest_fmt) {
          dest_val = src_val;
          goto done;
        }

        switch (src_fmt) {
        case GST_FORMAT_BYTES:
          GST_LOG_OBJECT (spot,"Dst_fmt == %d == FORMAT_DEFAULT", dest_fmt);
          switch (dest_fmt) {
          case GST_FORMAT_TIME:
            /* samples to time */
            dest_val = src_val / ((float)samplerate * 4 / 1000000);
            GST_LOG_OBJECT (spot,"Dst_val == %" G_GINT64_FORMAT, dest_val);
            break;
            default:
              ret = FALSE;
              g_assert_not_reached ();
              break;
          }
          break;
        case GST_FORMAT_TIME:
          GST_LOG_OBJECT (spot,"Dst_fmt == %d == FORMAT_TIME", dest_fmt);
          switch (dest_fmt) {
          case GST_FORMAT_BYTES:
            /* time to samples */
            dest_val = src_val/1000000 * samplerate * 4;
            GST_LOG_OBJECT (spot,"Dst_val == %" G_GINT64_FORMAT, dest_val);
            break;
          default:
            ret = FALSE;
            g_assert_not_reached ();
            break;
          }
          break;
        default:
          ret = FALSE;
          g_assert_not_reached ();
          break;
        }
        GST_LOG_OBJECT (spot, "Done src_val %" G_GINT64_FORMAT " dst_val %" G_GINT64_FORMAT "\n", src_val, dest_val);
      done:
        gst_query_set_convert (query, src_fmt, src_val, dest_fmt, dest_val);
        break;
      }
  default:
    ret = FALSE;
    GST_LOG_OBJECT (spot, "Query type unknown, default");
    /* FIXME: add case for query type and remove comment
       g_assert_not_reached (); */
    break;
  }

  if (!ret) {
    ret = GST_BASE_SRC_CLASS (parent_class)->query (basesrc, query);
  }

 no_format_yet:
  if (!ret) {
    GST_DEBUG_OBJECT (spot, "Query failed");
  }

  return ret;
}

static gboolean
gst_spot_src_is_seekable (GstBaseSrc * basesrc)
{
  return TRUE;
}

static gboolean
gst_spot_src_get_size (GstBaseSrc * basesrc, guint64 * size)
{
  GstSpotSrc *spot;
  int duration = 0;

  spot = GST_SPOT_SRC (basesrc);
  duration = run_spot_cmd (spot, SPOT_CMD_DURATION, 0);

  if (!duration) {
    GST_CAT_ERROR_OBJECT (gst_spot_src_debug_audio, spot, "No duration error");
    goto no_duration;
  }

  *size = (duration/1000) * 44100 * 4;
  GST_CAT_LOG_OBJECT (gst_spot_src_debug_audio, spot, "Duration=%d => size=%lld\n", duration, *size);

  return TRUE;

no_duration:
  return FALSE;
}

static gboolean
gst_spot_src_start (GstBaseSrc * basesrc)
{
  int error;
  GstSpotSrc *spot = (GstSpotSrc *) basesrc;

  GST_DEBUG_OBJECT (spot, "Start");
  error = run_spot_cmd (spot, SPOT_CMD_START, 0);

  return error != -1;
}

static gboolean
gst_spot_src_stop (GstBaseSrc * basesrc)
{
  GstSpotSrc *spot = (GstSpotSrc *) basesrc;

  GST_DEBUG_OBJECT (spot, "Stop");
  spot = GST_SPOT_SRC (basesrc);

  run_spot_cmd (spot, SPOT_CMD_STOP, 0);
  spot->read_position = 0;
  /* clear adapter (we are stopped and do not continue from same place) */
  g_mutex_lock (GST_SPOT_SRC_ADAPTER_MUTEX (spot));
  spot->end_of_track = FALSE;
  gst_adapter_clear (GST_SPOT_SRC_ADAPTER (spot));
  g_mutex_unlock (GST_SPOT_SRC_ADAPTER_MUTEX (spot));
  return TRUE;
}

static gboolean
gst_spot_src_unlock (GstBaseSrc *bsrc)
{
  GstSpotSrc *spot = (GstSpotSrc *) bsrc;

  spot->unlock_state = TRUE;
  GST_DEBUG_OBJECT (spot, "Unlock");
  GST_DEBUG_OBJECT (spot, "Broadcast process_events_cond - GST_SPOT_SRC_UNLOCK");
  g_cond_broadcast (GST_SPOT_SRC_ADAPTER_COND (spot));
  return TRUE;
}

static gboolean
gst_spot_src_unlock_stop (GstBaseSrc *bsrc)
{
  GstSpotSrc *spot = (GstSpotSrc *) bsrc;

  spot->unlock_state = FALSE;
  GST_DEBUG_OBJECT (spot, "Unlock stop");
  g_cond_broadcast (GST_SPOT_SRC_ADAPTER_COND (spot));
  return TRUE;
}

static gboolean
gst_spot_src_set_spotifyuri (GstSpotSrc * spot, const gchar * spotify_uri)
{
  GstState state;

  gchar *s_uri = SPOT_OBJ_SPOTIFY_URI (spot_instance);
  /* the element must be stopped in order to do this */
  state = GST_STATE (spot);
  if (state != GST_STATE_READY && state != GST_STATE_NULL) {
    goto wrong_state;
  }

  g_free (s_uri);
  g_free (spot->uri);

  /* clear the both uri/spotify_uri if we get a NULL (is that possible?) */
  if (spotify_uri == NULL) {
    s_uri = NULL;
    spot->uri = NULL;
  } else {
    /* we store the spotify_uri as received by the application. On Windoes this
     * should be UTF8 */
    s_uri = g_strdup (spotify_uri);
    spot->uri = gst_uri_construct ("spotify", s_uri);
  }
  g_object_notify (G_OBJECT (spot), "spotifyuri"); /* why? */
  gst_uri_handler_new_uri (GST_URI_HANDLER (spot), spot->uri);

  return TRUE;

  /* ERROR */
wrong_state:
  GST_DEBUG_OBJECT (spot, "Setting spotify_uri in wrong state");
  return FALSE;
}


/*****************************************************************************/
/*** GSTURIHANDLER INTERFACE *************************************************/

static GstURIType
gst_spot_src_uri_get_type (void)
{
  return GST_URI_SRC;
}

static gchar **
gst_spot_src_uri_get_protocols (void)
{
  static gchar *protocols[] = { "spot", NULL };

  return protocols;
}

static const gchar *
gst_spot_src_uri_get_uri (GstURIHandler * handler)
{
  GstSpotSrc *spot = GST_SPOT_SRC (handler);

  return spot->uri;
}

static gboolean
gst_spot_src_uri_set_uri (GstURIHandler * handler, const gchar * uri)
{
  gchar *location, *hostname = NULL;
  gboolean ret = FALSE;
  GstSpotSrc *spot = GST_SPOT_SRC (handler);
  GST_DEBUG_OBJECT (spot, "URI '%s' for filesrc", uri);

  location = g_filename_from_uri (uri, &hostname, NULL);

  if (!location) {
    GST_WARNING_OBJECT (spot, "Invalid URI '%s' for filesrc", uri);
    goto beach;
  }

  ret = gst_spot_src_set_spotifyuri (spot, location);

beach:
  if (location) {
    g_free (location);
  }
  if (hostname) {
    g_free (hostname);
  }

  return ret;
}

static void
gst_spot_src_uri_handler_init (gpointer g_iface, gpointer iface_data)
{
  GstURIHandlerInterface *iface = (GstURIHandlerInterface *) g_iface;

  iface->get_type = gst_spot_src_uri_get_type;
  iface->get_protocols = gst_spot_src_uri_get_protocols;
  iface->get_uri = gst_spot_src_uri_get_uri;
  iface->set_uri = gst_spot_src_uri_set_uri;
}
