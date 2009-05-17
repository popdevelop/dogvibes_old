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

#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <glib.h>
#include <gst/gst.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <gst/base/gstadapter.h>
#include <spotify/api.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "gstspotsrc.h"
#include "config.h"

#define DEFAULT_USER "anonymous"
#define DEFAULT_PASS ""
#define DEFAULT_URI "spotify://spotify:track:3odhGRfxHMVIwtNtc4BOZk"
#define DEFAULT_SPOTIFY_URI "spotify:track:3odhGRfxHMVIwtNtc4BOZk"

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
#define GST_CAT_DEFAULT gst_spot_src_debug


/* SpotSrc signals and args */
enum
{
  /* FILL ME */
  LAST_SIGNAL
};

enum
{
  ARG_0,
  ARG_SPOTIFY_URI,
  ARG_USER,
  ARG_PASS,
  ARG_URI
};

/* libspotify */
static void logged_in (sp_session *session, sp_error error);
static void logged_out (sp_session *session);
static void connection_error (sp_session *session, sp_error error);
static void notify_main_thread (sp_session *session);
static void log_message (sp_session *session, const char *data);
static int music_delivery (sp_session *sess, const sp_audioformat *format, const void *frames, int num_frames);

static gboolean
gst_spot_src_set_location (GstSpotSrc * src, const gchar * spotify_uri);

static sp_session_callbacks g_callbacks = {
  &logged_in,
  &logged_out,
  NULL,
  &connection_error,
  NULL,
  &notify_main_thread,
  &music_delivery,
  NULL,
  &log_message
};

const uint8_t g_appkey[] = {
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
const size_t g_appkey_size = sizeof (g_appkey);

//FIXME this should probably be a private property of the object
sp_session *session;
gboolean buffered = FALSE;
static gboolean loggedin = FALSE;
static GMutex *mutex;
static GMutex *sp_mutex;
static GCond *cond;
static GThread *thread;
GstSpotSrc *spot;
sp_track *t;



static void gst_spot_src_finalize (GObject * object);
static void gst_spot_src_set_property (GObject * object, guint prop_id, const GValue * value, GParamSpec * pspec);
static void gst_spot_src_get_property (GObject * object, guint prop_id, GValue * value, GParamSpec * pspec);
static gboolean gst_spot_src_start (GstBaseSrc * basesrc);
static gboolean gst_spot_src_stop (GstBaseSrc * basesrc);
static gboolean gst_spot_src_is_seekable (GstBaseSrc * src);
static gboolean gst_spot_src_get_size (GstBaseSrc * src, guint64 * size);
static GstFlowReturn gst_spot_src_create (GstBaseSrc * src, guint64 offset, guint length, GstBuffer ** buffer);
static gboolean gst_spot_src_query (GstBaseSrc * src, GstQuery * query);
static void gst_spot_src_uri_handler_init (gpointer g_iface, gpointer iface_data);

/* FIXME: fix this */
//static GThread *spotify_thread;
//static gboolean keep_threads = TRUE;
static GstAdapter *adapter;
static GMutex *adapter_mutex;
static GMutex *fd_mutex;
static int fd;
static int position = 0;

static void connection_error (sp_session *session, sp_error error)
{
  GST_ERROR ("connection to Spotify failed: %s\n",
      sp_error_message (error));
}

static void logged_in (sp_session *session, sp_error error)
{
  if (SP_ERROR_OK != error) {
    GST_ERROR ("failed to log in to Spotify: %s\n",
        sp_error_message (error));
    return;
  }
  sp_user *me = sp_session_user (session);
  const char *my_name = (sp_user_is_loaded (me) ?
                         sp_user_display_name (me) :
                         sp_user_canonical_name (me));

  //FIXME debug
  GST_DEBUG ("Logged in to Spotify as user %s", my_name);
  loggedin = TRUE;
}


static void logged_out (sp_session *session)
{
  //FIXME debug
  printf ("LOGGED OUT FROM SPOTIFY");
}

static void log_message (sp_session *session, const char *data)
{
  //FIXME debug
  printf ("log_message: %s", data);
}

static void notify_main_thread (sp_session *session)
{
  GST_DEBUG ("BROADCAST COND\n");
  /* signal thread to process events */
  g_cond_broadcast (cond);
}

/* only used to trigger sp_session_process_events when needed,
 * looks like about once a second */
void *thread_func( void *ptr )
{
   GTimeVal t;
   gboolean in_time;
   int timeout = -1;
   /* wait for first broadcast */
   g_cond_wait (cond, mutex);
   while (1) {
     g_mutex_lock (sp_mutex);
     sp_session_process_events (session, &timeout);
     g_mutex_unlock (sp_mutex);
     g_get_current_time(&t);
     g_time_val_add(&t, timeout*1000);
     g_print ("\n\nWAITING FOR BROADCAST (timeout = %d ms)\n\n\n", timeout);
     in_time = g_cond_timed_wait (cond, mutex, &t);
     GST_DEBUG ("GOT %s\n", in_time ? "BROADCAST" : "TIMEOUT");
   }
}

/* end libspotifyr */


static int music_delivery (sp_session *sess, const sp_audioformat *format,const void *frames, int num_frames)
{
  guint bufsize = num_frames * 4;
  GstBuffer *buffer;
  int sample_rate = 44100;
  int availible;
  g_print ("%s - start %p with %d frames with size=%d\n",__FUNCTION__, frames, num_frames, bufsize);
  buffer = gst_buffer_new_and_alloc (bufsize);

  memcpy (GST_BUFFER_DATA(buffer), (guint8*)frames, bufsize);

  /* see if we have 1 second of audio */
  g_mutex_lock(adapter_mutex);             /* lock adapter */
  availible = gst_adapter_available (adapter);
  g_print ("%s - availiable before push = %d\n", __FUNCTION__, availible);
  if (availible >= sample_rate * 4) {
    g_print ("%s - return 0, adapter is full = %d\n", __FUNCTION__, availible);
    g_mutex_unlock(adapter_mutex);        /* unlock adapter */
    return 0;
  }

  gst_adapter_push (adapter, buffer);
  availible = gst_adapter_available (adapter);
  g_print ("%s - availiable after push = %d\n", __FUNCTION__, availible);
  g_mutex_unlock(adapter_mutex);           /* unlock adapter */

  g_print ("%s - return %d\n",__FUNCTION__, num_frames);
  return num_frames;
}


#if 0
void* spotify_stub_thread (void *unused)
{
  #define READ_SIZE 8192
  #define FILENAME "miljon.wav"
  int sleeptime;
  gchar *frames;

  g_print ("%s - start\n",__FUNCTION__);
  frames = g_malloc0 (READ_SIZE);
  adapter = gst_adapter_new();
  adapter_mutex = g_mutex_new();
  fd_mutex = g_mutex_new();

  /* open the file first */
  g_mutex_lock(fd_mutex);             /* lock fd */
  fd = open (FILENAME, O_RDONLY, 0);
  g_mutex_unlock(fd_mutex);           /* unlock fd */
  if (fd < 0)
    goto music_open_failed;

  do {
    gint ret, rewind_len, consumed;
    gint num_frames = READ_SIZE / 4;
    /* read a spotsize of data here */
    g_print ("%s - reading %d bytes\n", __FUNCTION__, READ_SIZE);
    g_mutex_lock(fd_mutex);             /* lock fd */
    ret = read (fd, frames, READ_SIZE);
    position += ret;
    g_mutex_unlock(fd_mutex);           /* unlock fd */
    if (G_UNLIKELY (ret < 0))
      goto music_could_not_read;
    /* seekable regular files should have given us what we expected */
    if (G_UNLIKELY ((guint) ret < READ_SIZE))
      goto music_unexpected_eof;

    /* all ok, we read data, now feed the fake callback */
    g_print ("%s - feed music_delivery\n",__FUNCTION__);
    consumed = music_delivery (frames, num_frames); /* 2 channels 2 bytes each */

    if (consumed > num_frames) {
      g_print ("%s - ERROR consumed(%d) > READ_SIZE(%d)\n", __FUNCTION__, consumed, READ_SIZE);
      consumed = num_frames;
    }


    /* see how much they took, make sure we start at right position next time
       its in frames, times 4 for bytes */
    rewind_len = -1 * (num_frames - consumed) * 4;
    if (rewind_len != 0) {
      gint seek;
      g_print ("%s - rewind_len = %d position=%d\n", __FUNCTION__, rewind_len, position);
      g_mutex_lock(fd_mutex);             /* lock fd */
      position += rewind_len;
      seek = lseek (fd, rewind_len, SEEK_CUR);
      g_mutex_unlock(fd_mutex);           /* unlock fd */
      if (G_UNLIKELY (seek < 0 || seek != position)) {
        g_print ("%s seek FAILED\n", __FUNCTION__);
        goto music_seek_failed;
      }
      sleeptime = G_USEC_PER_SEC/1000;
      g_print ("%s - sleep %d usec\n", __FUNCTION__, sleeptime);
      //g_usleep (sleeptime);
    }


  } while (keep_threads);

  g_print ("%s - exit\n",__FUNCTION__);
  g_thread_exit(NULL);
music_could_not_read:
  {
    g_print ("%s - could not read\n", __FUNCTION__);
    g_thread_exit(NULL);
  }
music_unexpected_eof:
  {
    g_print ("%s - eof\n", __FUNCTION__);
    g_thread_exit(NULL);
  }
music_seek_failed:
  {
    g_print ("%s - seek failed\n", __FUNCTION__);
    g_thread_exit(NULL);
  }
music_open_failed:
  {
    switch (errno) {
      case ENOENT:
        g_print ("%s - open failed - ENOENT\n", __FUNCTION__);
        break;
      default:
        g_print ("%s - open failed\n", __FUNCTION__);
        break;
    }
    g_thread_exit(NULL);
  }
  return NULL;
}
#endif
/* close to boilerplate stuff */

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
  GST_DEBUG_CATEGORY_INIT (gst_spot_src_debug, "spotsrc", 0, "spotsrc element");
}

GST_BOILERPLATE_FULL (GstSpotSrc, gst_spot_src, GstBaseSrc, GST_TYPE_BASE_SRC,
    _do_init);

static gboolean is_logged_in = FALSE;

static void
gst_spot_src_base_init (gpointer g_class)
{

  GstElementClass *gstelement_class = GST_ELEMENT_CLASS (g_class);

  gst_element_class_set_details_simple (gstelement_class,
      "Spot Source",
      "Source/spot",
      "Read from arbitrary point in a file with raw audio",
      "joelbits@gmail.com & johan.gyllenspetz@gmail.com");
  gst_element_class_add_pad_template (gstelement_class,
      gst_static_pad_template_get (&srctemplate));


  sp_session_config config;
  sp_error error;

  if (is_logged_in) {
    return;
  }

  GST_DEBUG_OBJECT (spot, "OPEN");

  config.api_version = SPOTIFY_API_VERSION;
  //FIXME check if these paths are appropiate
  config.cache_location = "tmp";
  config.settings_location = "tmp";
  config.application_key = g_appkey;
  config.application_key_size = g_appkey_size;
  config.user_agent = "spotify-gstreamer-src";
  config.callbacks = &g_callbacks;

  error = sp_session_init (&config, &session);

  if (SP_ERROR_OK != error) {
    GST_ERROR ("failed to create session: %s\n", sp_error_message (error));
    return;
  }

  /* Login using the credentials given on the command line */
  error = sp_session_login (session, GST_SPOT_SRC_USER (spot) , GST_SPOT_SRC_PASS (spot));

  if (SP_ERROR_OK != error) {
    GST_ERROR ("failed to login: %s\n", sp_error_message (error));
    return;
  }

  //FIXME this is probably not the best way to wait to be logged in
  g_cond_broadcast(cond);
  while (!loggedin) {
    usleep(10000);
  }
  g_print ("logged in!\n");

  is_logged_in = TRUE;


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
  gstbasesrc_class->is_seekable = GST_DEBUG_FUNCPTR (gst_spot_src_is_seekable);
  gstbasesrc_class->get_size = GST_DEBUG_FUNCPTR (gst_spot_src_get_size);
  gstbasesrc_class->create = GST_DEBUG_FUNCPTR (gst_spot_src_create);
  gstbasesrc_class->query = GST_DEBUG_FUNCPTR (gst_spot_src_query);

  if (g_thread_supported () ) {
     printf ("%s - g_thread_supported\n", __FUNCTION__);
  } else {
     g_thread_init (NULL);
     printf ("ERROR %s - g_thread_supported not supported\n", __FUNCTION__);
  }

#if 0
  if ((spotify_thread = g_thread_create ((GThreadFunc)spotify_stub_thread, (void *)NULL, TRUE, &err)) == NULL) {
     printf ("g_thread_create failed: %s!!\n", err->message );
     g_error_free (err) ;
  }
#endif
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

}

static void
gst_spot_src_init (GstSpotSrc * src, GstSpotSrcClass * g_class)
{
  GError *err;
  src->filename = NULL;
  src->uri = NULL;
  src->fd = -1;
  src->read_position = 0;

  GST_SPOT_SRC_USER (spot) = g_strdup (DEFAULT_USER);
  GST_SPOT_SRC_PASS (spot) = g_strdup (DEFAULT_PASS);
  GST_SPOT_SRC_URI (spot) = g_strdup (DEFAULT_URI);
  GST_SPOT_SRC_SPOTIFY_URI (spot) = g_strdup (DEFAULT_SPOTIFY_URI);

  cond = g_cond_new ();
  mutex = g_mutex_new ();
  sp_mutex = g_mutex_new ();
 
  if ((thread = g_thread_create((GThreadFunc)thread_func, (void *)NULL, TRUE, &err)) == NULL) {
     printf("g_thread_create failed: %s!!\n", err->message );
     g_error_free (err) ;
  }
  printf ("thread created\n");
}

static void
gst_spot_src_finalize (GObject * object)
{
  GstSpotSrc *src;

  src = GST_SPOT_SRC (object);

  printf("SRC:FINALIZED\n");
  g_free (GST_SPOT_SRC_USER (src));
  g_free (GST_SPOT_SRC_PASS (src));
  g_free (GST_SPOT_SRC_URI (src));
  g_free (GST_SPOT_SRC_SPOTIFY_URI (src));

  g_cond_free (cond);
  g_mutex_free (mutex);

  G_OBJECT_CLASS (parent_class)->finalize (object);

}

static gboolean
gst_spot_src_set_location (GstSpotSrc * src, const gchar * spotify_uri)
{
  GstState state;

  /* the element must be stopped in order to do this */
  state = GST_STATE (src);
  if (state != GST_STATE_READY && state != GST_STATE_NULL) {
    goto wrong_state;
  }

  g_free (src->spotify_uri);
  g_free (src->uri);

  /* clear the both uri/spotify_uri if we get a NULL (is that possible?) */
  if (spotify_uri == NULL) {
    src->spotify_uri = NULL;
    src->uri = NULL;
  } else {
    /* we store the spotify_uri as received by the application. On Windoes this
     * should be UTF8 */
    src->spotify_uri = g_strdup (spotify_uri);
    src->uri = gst_uri_construct ("spotify", src->spotify_uri);
  }
  g_object_notify (G_OBJECT (src), "spotifyuri"); /* why? */
  gst_uri_handler_new_uri (GST_URI_HANDLER (src), src->uri);

  return TRUE;

  /* ERROR */
wrong_state:
  {
    GST_DEBUG_OBJECT (src, "setting spotify_uri in wrong state");
    return FALSE;
  }
}

static void
gst_spot_src_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  GstSpotSrc *src;

  g_return_if_fail (GST_IS_SPOT_SRC (object));

  src = GST_SPOT_SRC (object);

  switch (prop_id) {
    case ARG_USER:
      g_free (GST_SPOT_SRC_USER (src));
      GST_SPOT_SRC_USER (src) = g_strdup (g_value_get_string (value));
      break;
    case ARG_PASS:
      g_free (GST_SPOT_SRC_PASS (src));
      GST_SPOT_SRC_PASS (src) = g_strdup (g_value_get_string (value));
      break;
    case ARG_URI:
      g_free (GST_SPOT_SRC_URI (src));
      GST_SPOT_SRC_URI (src) = g_strdup (g_value_get_string (value));
      break;
    case ARG_SPOTIFY_URI:
      gst_spot_src_set_location (src, g_value_get_string (value));
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
  GstSpotSrc *src;

  g_return_if_fail (GST_IS_SPOT_SRC (object));

  src = GST_SPOT_SRC (object);

  switch (prop_id) {
    case ARG_USER:
      g_value_set_string (value, GST_SPOT_SRC_USER (src));
      break;
    case ARG_PASS:
      g_value_set_string (value, GST_SPOT_SRC_PASS (src));
      break;
    case ARG_URI:
      g_value_set_string (value, GST_SPOT_SRC_URI (src));
      break;
    case ARG_SPOTIFY_URI:
      g_value_set_string (value, GST_SPOT_SRC_SPOTIFY_URI (src));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

/* less boilerplate below... */

static GstFlowReturn
gst_spot_src_create_read (GstSpotSrc * src, guint64 offset, guint length, GstBuffer ** buffer)
{
  //g_print ("filename = %s\n", src->filename);
  if (strcmp (src->filename, "miljon.wav") != 0) {

    GstFlowReturn ret = GST_FLOW_OK;

    if (G_UNLIKELY (src->read_position != offset)) {
      off_t res;
      long file_posn;

      g_mutex_lock(fd_mutex);             /* lock fd */
      res = lseek (fd, offset, SEEK_SET);
      g_mutex_unlock(fd_mutex);           /* unlock fd */
      if (G_UNLIKELY (res < 0 || res != offset))
        goto create_seek_failed;

      g_mutex_lock(fd_mutex);             /* lock fd */
      file_posn = lseek(fd, 0L, SEEK_CUR );
      position = file_posn;
      g_mutex_unlock(fd_mutex);           /* unlock fd */
      g_print ("performed seek, now at %ld\n", file_posn);

      src->read_position = offset;
    }
    /* see if we have bytes to write */
    g_mutex_lock(adapter_mutex);             /* lock adapter */
    g_print ("%s - length=%u offset=%llu end=%llu read_position=%llu availible=%d\n", __FUNCTION__,
              length, offset, offset+length, src->read_position, gst_adapter_available (adapter));
    if (gst_adapter_available (adapter) >= length) {
      GstBuffer *buf;
      buf = gst_buffer_try_new_and_alloc (length);

      GST_BUFFER_SIZE (buf) = length;
      GST_BUFFER_OFFSET (buf) = offset;
      GST_BUFFER_OFFSET_END (buf) = offset + length;
      memcpy (GST_BUFFER_DATA (buf), gst_adapter_peek (adapter, length), length);
      gst_adapter_flush (adapter, length);
      src->read_position += length;
      *buffer = buf;

      //g_print ("%s - GST_FLOW_OK return\n", __FUNCTION__);
    } else {
      g_print ("%s - avaible in adapter = %d\n", __FUNCTION__, gst_adapter_available (adapter) );
      g_print ("%s - GST_FLOW_ERROR return\n", __FUNCTION__);
      ret = GST_FLOW_ERROR;
    }
    g_mutex_unlock(adapter_mutex);           /* unlock adapter */
    return ret;

  } else {

    int ret;
    GstBuffer *buf;

    if (G_UNLIKELY (src->read_position != offset)) {
      off_t res;

      res = lseek (src->fd, offset, SEEK_SET);
      if (G_UNLIKELY (res < 0 || res != offset))
        goto create_seek_failed;

      src->read_position = offset;
    }

    buf = gst_buffer_try_new_and_alloc (length);
    if (G_UNLIKELY (buf == NULL && length > 0)) {
      GST_ERROR_OBJECT (src, "Failed to allocate bufer of %u bytes", length);
      return GST_FLOW_ERROR;
    }

    GST_LOG_OBJECT (src, "Reading %d bytes at offset 0x%" G_GINT64_MODIFIER "x", length, offset);
    ret = read (src->fd, GST_BUFFER_DATA (buf), length);
    if (G_UNLIKELY (ret < 0))
      goto create_could_not_read;

    // seekable regular files should have given us what we expected
    if (G_UNLIKELY ((guint) ret < length))
      goto create_unexpected_eos;

    // other files should eos if they read 0 and more was requested
    if (G_UNLIKELY (ret == 0 && length > 0))
      goto create_eos;

    length = ret;

    GST_BUFFER_SIZE (buf) = length;
    GST_BUFFER_OFFSET (buf) = offset;
    GST_BUFFER_OFFSET_END (buf) = offset + length;

    *buffer = buf;

    src->read_position += length;

    return GST_FLOW_OK;

  create_seek_failed:
    {
      g_print ("%s - could seek failed\n", __FUNCTION__);
      return GST_FLOW_ERROR;
    }
  create_could_not_read:
    {
      g_print ("%s - could not read\n", __FUNCTION__);
      gst_buffer_unref (buf);
      return GST_FLOW_ERROR;
    }
  create_unexpected_eos:
    {
      g_print ("%s - unexpected eos\n", __FUNCTION__);
      gst_buffer_unref (buf);
      return GST_FLOW_ERROR;
    }
  create_eos:
    {
      g_print ("%s - eos\n", __FUNCTION__);
      gst_buffer_unref (buf);
      return GST_FLOW_UNEXPECTED;
    }
  }
}

static GstFlowReturn
gst_spot_src_create (GstBaseSrc * basesrc, guint64 offset, guint length, GstBuffer ** buffer)
{
  GstSpotSrc *src;
  GstFlowReturn ret;

  src = GST_SPOT_SRC (basesrc);

  ret = gst_spot_src_create_read (src, offset, length, buffer);

  return ret;
}

static gboolean
gst_spot_src_query (GstBaseSrc * basesrc, GstQuery * query)
{
  gboolean ret = FALSE;
  GstSpotSrc *src = GST_SPOT_SRC (basesrc);

  switch (GST_QUERY_TYPE (query)) {
    case GST_QUERY_URI:
      gst_query_set_uri (query, src->uri);
      ret = TRUE;
      break;
    default:
      ret = FALSE;
      break;
  }

  if (!ret)
    ret = GST_BASE_SRC_CLASS (parent_class)->query (basesrc, query);

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
  struct stat stat_results;
  GstSpotSrc *src;

  src = GST_SPOT_SRC (basesrc);

  if (fstat (src->fd, &stat_results) < 0)
    goto could_not_stat;

  *size = stat_results.st_size;

  return TRUE;

  /* ERROR */
could_not_stat:
  {
    return FALSE;
  }
}

/* open the file and mmap it, necessary to go to READY state */
static gboolean
gst_spot_src_start (GstBaseSrc * basesrc)
{
  
  //move
  spot = GST_SPOT_SRC (basesrc);

  printf("SRC:START %s\n", GST_SPOT_SRC_SPOTIFY_URI (spot));

  g_mutex_lock (sp_mutex);
  sp_link *link = sp_link_create_from_string (GST_SPOT_SRC_SPOTIFY_URI (spot));
  g_mutex_unlock (sp_mutex);

  if (!link) {
    GST_ERROR_OBJECT (spot, "Incorrect track ID");
    return FALSE;
  }

  g_mutex_lock (sp_mutex);
  t = sp_link_as_track (link);
  g_mutex_unlock (sp_mutex);
  if (!t) {
    GST_DEBUG_OBJECT (spot, "Only track ID:s are currently supported");
    return FALSE;
  }

  g_mutex_lock (sp_mutex);
  sp_track_add_ref (t);
  g_mutex_unlock (sp_mutex);
  g_mutex_lock (sp_mutex);
  sp_link_release (link);
  g_mutex_unlock (sp_mutex);

  //FIXME not the best way to wait for a track to be loaded
  g_cond_broadcast(cond);
  g_mutex_lock (sp_mutex);
  while (sp_track_is_loaded (t) == 0) {
    g_mutex_unlock (sp_mutex);
    usleep(10000);
    g_mutex_lock (sp_mutex);
  }
  g_mutex_unlock (sp_mutex);
  g_print ("track loaded!\n");

  g_mutex_lock (sp_mutex);
  GST_DEBUG_OBJECT (spot, "Now playing \"%s\"...\n", sp_track_name (t));
  g_mutex_unlock (sp_mutex);

  g_mutex_lock (sp_mutex);
  sp_session_player_load (session, t);
  g_mutex_unlock (sp_mutex);
  g_mutex_lock (sp_mutex);
  sp_session_player_play (session, 1);
  g_mutex_unlock (sp_mutex);
  return TRUE;

}

/* unmap and close the spot */
static gboolean
gst_spot_src_stop (GstBaseSrc * basesrc)
{
  printf("SRC:STOP\n");
  g_mutex_lock (sp_mutex);
  sp_session_player_unload (session);
  g_mutex_unlock (sp_mutex);

  //FIXME someone is holding references
  g_mutex_lock (sp_mutex);
  sp_track_release (t);
  g_mutex_unlock (sp_mutex);

  return TRUE;
}


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
  GstSpotSrc *src = GST_SPOT_SRC (handler);

  return src->uri;
}

static gboolean
gst_spot_src_uri_set_uri (GstURIHandler * handler, const gchar * uri)
{
  gchar *location, *hostname = NULL;
  gboolean ret = FALSE;
  GstSpotSrc *src = GST_SPOT_SRC (handler);
  GST_WARNING_OBJECT (src, "URI '%s' for filesrc", uri);

  location = g_filename_from_uri (uri, &hostname, NULL);

  if (!location) {
    GST_WARNING_OBJECT (src, "Invalid URI '%s' for filesrc", uri);
    goto beach;
  }

  if ((hostname) && (strcmp (hostname, "localhost"))) {
    /* Only 'localhost' is permitted */
    GST_WARNING_OBJECT (src, "Invalid hostname '%s' for filesrc", hostname);
    goto beach;
  }

  ret = gst_spot_src_set_location (src, location);

beach:
  if (location)
    g_free (location);
  if (hostname)
    g_free (hostname);

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

