/* GStreamer
 * Copyright (C) 2009 Johan Gyllenspetz <johan.gyllenspetz@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Alternatively, the contents of this file may be used under the
 * GNU Lesser General Public License Version 2.1 (the "LGPL"), in
 * which case the following provisions apply instead of the ones
 * mentioned above:
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
#include <gst/gst.h>
#include <spotify/api.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "gstspotifysrc.h"
#include "gstspotifyringbuffer.h"

#define DEFAULT_USER "anonymous"
#define DEFAULT_PASS ""
#define DEFAULT_URI "spotify:track:3odhGRfxHMVIwtNtc4BOZk"

GST_DEBUG_CATEGORY_STATIC (gst_spotify_debug);
#define GST_CAT_DEFAULT gst_spotify_debug

#define _do_init(bla) \
  GST_DEBUG_CATEGORY_INIT (gst_spotify_debug, "spotify", 0, "spotifysrc element");

GST_BOILERPLATE_FULL (GstSpotify, gst_spotify, GstBaseAudioSrc,
    GST_TYPE_BASE_AUDIO_SRC, _do_init);

/* spotifysrc */
static void gst_spotify_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec);
static void gst_spotify_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec);
static GstCaps *gst_spotify_getcaps (GstBaseSrc * bsrc);
static GstRingBuffer *gst_spotify_create_ringbuffer (GstBaseAudioSrc * src);
static void gst_spotify_finalize (GObject * object);

/* libspotify */
static void logged_in (sp_session *session, sp_error error);
static void logged_out (sp_session *session);
static void connection_error (sp_session *session, sp_error error);
static void notify_main_thread (sp_session *session);
static int music_delivery (sp_session *sess, const sp_audioformat *format,
                          const void *frames, int num_frames);
static void log_message (sp_session *session, const char *data);

static GstStaticPadTemplate spotify_src_factory = GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("audio/x-raw-int, "
        "endianness = (int) { 1234 }, "
        "signed = (boolean) { TRUE }, "
        "width = (int) 16, "
        "depth = (int) 16, "
        "rate = (int) 44100, channels = (int) 2; ")
    );


enum
{
  PROP_0,
  PROP_USER,
  PROP_PASS,
  PROP_URI,
};

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
GstRingBuffer *ring_buffer;


/* libspotify */

static int music_delivery (sp_session *sess, const sp_audioformat *format,
                          const void *frames, int num_frames)
{
  GstRingBuffer *buf;
  GstSpotifyRingBuffer *abuf;
  gint len;
  gint givenLen;
  guint8 *writeptr;
  gint writeseg;
  gint channels;

  buf = GST_RING_BUFFER_CAST (ring_buffer);
  abuf = GST_SPOTIFY_RING_BUFFER_CAST (ring_buffer);

  //FIXME this needs to be looked over
  channels = format->channels;
  len = num_frames * sizeof (int16_t) * format->channels;

  if (gst_ring_buffer_prepare_read (buf, &writeseg, &writeptr, &givenLen)) {
    printf ("%d %d\n", givenLen, len);
    memcpy (writeptr, frames, givenLen);

    /* we wrote one segment */
    gst_ring_buffer_advance (buf, 1);
  }
  return num_frames;
}

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
  printf ("Logged in to Spotify as user %s", my_name);
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
  printf ("FIXME notified but NOT handled\n");
}

/* end libspotifyr */


/* ringbuffer */

static GType
gst_spotify_ring_buffer_get_type ()
{
  static GType ringbuffer_type = 0;

  if (!ringbuffer_type) {
    static const GTypeInfo ringbuffer_info = { sizeof (GstSpotifyRingBufferClass),
      NULL,
      NULL,
      (GClassInitFunc) gst_spotify_ring_buffer_class_init,
      NULL,
      NULL,
      sizeof (GstSpotifyRingBuffer),
      0,
      (GInstanceInitFunc) gst_spotify_ring_buffer_init,
      NULL
    };

    ringbuffer_type =
        g_type_register_static (GST_TYPE_RING_BUFFER,
        "GstSpotifyRingBuffer", &ringbuffer_info, 0);
  }
  return ringbuffer_type;
}

static void
gst_spotify_ring_buffer_class_init (GstSpotifyRingBufferClass * klass)
{
  GObjectClass *gobject_class;
  GstObjectClass *gstobject_class;
  GstRingBufferClass *gstringbuffer_class;

  gobject_class = (GObjectClass *) klass;
  gstobject_class = (GstObjectClass *) klass;
  gstringbuffer_class = (GstRingBufferClass *) klass;

  ring_parent_class = g_type_class_peek_parent (klass);

  gobject_class->dispose = GST_DEBUG_FUNCPTR (gst_spotify_ring_buffer_dispose);
  gobject_class->finalize = GST_DEBUG_FUNCPTR (gst_spotify_ring_buffer_finalize);

  gstringbuffer_class->open_device =
      GST_DEBUG_FUNCPTR (gst_spotify_ring_buffer_open_device);
  gstringbuffer_class->close_device =
      GST_DEBUG_FUNCPTR (gst_spotify_ring_buffer_close_device);
  gstringbuffer_class->acquire =
      GST_DEBUG_FUNCPTR (gst_spotify_ring_buffer_acquire);
  gstringbuffer_class->release =
      GST_DEBUG_FUNCPTR (gst_spotify_ring_buffer_release);
  gstringbuffer_class->start = GST_DEBUG_FUNCPTR (gst_spotify_ring_buffer_start);
  gstringbuffer_class->pause = GST_DEBUG_FUNCPTR (gst_spotify_ring_buffer_pause);
  gstringbuffer_class->resume = GST_DEBUG_FUNCPTR (gst_spotify_ring_buffer_start);
  gstringbuffer_class->stop = GST_DEBUG_FUNCPTR (gst_spotify_ring_buffer_stop);

  gstringbuffer_class->delay = GST_DEBUG_FUNCPTR (gst_spotify_ring_buffer_delay);
}

static void
gst_spotify_ring_buffer_init (GstSpotifyRingBuffer * buf,
    GstSpotifyRingBufferClass * g_class)
{
}

static void
gst_spotify_ring_buffer_dispose (GObject * object)
{
  G_OBJECT_CLASS (ring_parent_class)->dispose (object);
}

static void
gst_spotify_ring_buffer_finalize (GObject * object)
{
  G_OBJECT_CLASS (ring_parent_class)->finalize (object);
}

/* the _open_device method should make a connection with the server
*/
static gboolean
gst_spotify_ring_buffer_open_device (GstRingBuffer * buf)
{
  sp_session_config config;
  sp_error error;
  GstSpotify *spotify;
/*   spotify = GST_SPOTIFY (asrc); */

  spotify = GST_SPOTIFY (GST_OBJECT_PARENT (buf));

  //FIXME not nice with global

  GST_DEBUG_OBJECT (spotify, "OPEN");

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
    return FALSE;
  }

  /* Login using the credentials given on the command line */
  error = sp_session_login (session, GST_SPOTIFY_USER (spotify) , GST_SPOTIFY_PASS (spotify));

  if (SP_ERROR_OK != error) {
    GST_ERROR ("failed to login: %s\n", sp_error_message (error));
    return FALSE;
  }

  //FIXME this is probably not the best way to wait to be logged in
  while (!loggedin) {
    int timeout = -1;
    sp_session_process_events (session, &timeout);
    usleep (timeout * 100);
  }

  sp_link *link = sp_link_create_from_string (GST_SPOTIFY_URI (spotify));
  if (!link) {
    GST_ERROR_OBJECT (spotify, "Incorrect track ID");
    return FALSE;
  }

  sp_track *t;
  t = sp_link_as_track (link);
  if (!t) {
    GST_DEBUG_OBJECT (spotify, "Only track ID:s are currently supported");
    return FALSE;
  }

  sp_track_add_ref (t);
  sp_link_release (link);

  //FIXME not the best way to wait for a track to be loaded
  while (sp_track_is_loaded (t) == 0) {
    int timeout = -1;
    sp_session_process_events (session, &timeout);
    usleep (timeout * 100);
  }

  GST_DEBUG_OBJECT (spotify, "Now playing \"%s\"...\n", sp_track_name (t));

  //FIXME unload at end of src life
  sp_session_player_load (session, t);
  sp_session_player_play (session, 1);

  return TRUE;
}

/* close the connection with the server
*/
static gboolean
gst_spotify_ring_buffer_close_device (GstRingBuffer * buf)
{
  sp_session_player_unload (session);
  g_print ("CLOSE DEVICE\n");
  return TRUE;
}

static gboolean
gst_spotify_ring_buffer_acquire (GstRingBuffer * buf, GstRingBufferSpec * spec)
{
  GstSpotifyRingBuffer *abuf;
  gint sample_rate;
  gint buffer_size;
  gint channels;

  printf ("ACUIRE\n");

  abuf = GST_SPOTIFY_RING_BUFFER_CAST (buf);

  /* sample rate must be that of the server */
  sample_rate = 44100;
  channels = 2;
  buffer_size = 8192/4;


  spec->segsize = buffer_size * sizeof (int16_t) * channels;
  spec->latency_time = gst_util_uint64_scale (spec->segsize,
      (GST_SECOND / GST_USECOND), spec->rate * spec->bytes_per_sample);
  /* segtotal based on buffer-time latency */
  spec->segtotal = spec->buffer_time / spec->latency_time;

  /* allocate the ringbuffer memory now */
  buf->data = gst_buffer_new_and_alloc (spec->segtotal * spec->segsize);
  memset (GST_BUFFER_DATA (buf->data), 0, GST_BUFFER_SIZE (buf->data));

  return TRUE;
}

/* function is called with LOCK */
static gboolean
gst_spotify_ring_buffer_release (GstRingBuffer * buf)
{
  GstSpotifyRingBuffer *abuf;

  abuf = GST_SPOTIFY_RING_BUFFER_CAST (buf);

  /* free the buffer */
  gst_buffer_unref (buf->data);
  buf->data = NULL;

  return TRUE;
}

static gboolean
gst_spotify_ring_buffer_start (GstRingBuffer * buf)
{
  g_print ("START\n");
  return TRUE;
}

static gboolean
gst_spotify_ring_buffer_pause (GstRingBuffer * buf)
{
  g_print ("PAUSE\n");
  return TRUE;
}

static gboolean
gst_spotify_ring_buffer_stop (GstRingBuffer * buf)
{
  g_print ("STOP\n");
  return TRUE;
}

static guint
gst_spotify_ring_buffer_delay (GstRingBuffer * buf)
{
  //FIXME delay
  return 0;
}

/* end ringbuffer */


/* spotifysrc */

static void
gst_spotify_base_init (gpointer gclass)
{
  static GstElementDetails gst_spotify_details = {
    "Audio Source (Spotify)",
    "Source/Audio",
    "Input from Spotify",
    "Johan Gyllenspetz <johan.gyllenspetz@gmail.com>"
  };
  GstElementClass *element_class = GST_ELEMENT_CLASS (gclass);

  gst_element_class_add_pad_template (element_class,
      gst_static_pad_template_get (&spotify_src_factory));
  gst_element_class_set_details (element_class, &gst_spotify_details);
}

static void
gst_spotify_class_init (GstSpotifyClass * klass)
{
  GObjectClass *gobject_class;
  GstElementClass *gstelement_class;
  GstBaseSrcClass *gstbasesrc_class;
  GstBaseAudioSrcClass *gstbaseaudiosrc_class;

  gobject_class = (GObjectClass *) klass;
  gstelement_class = (GstElementClass *) klass;

  gstbasesrc_class = (GstBaseSrcClass *) klass;
  gstbaseaudiosrc_class = (GstBaseAudioSrcClass *) klass;

  gobject_class->set_property =
      GST_DEBUG_FUNCPTR (gst_spotify_set_property);
  gobject_class->get_property =
      GST_DEBUG_FUNCPTR (gst_spotify_get_property);

  gobject_class->finalize = GST_DEBUG_FUNCPTR (gst_spotify_finalize);

  gstbasesrc_class->get_caps = GST_DEBUG_FUNCPTR (gst_spotify_getcaps);
  gstbaseaudiosrc_class->create_ringbuffer =
      GST_DEBUG_FUNCPTR (gst_spotify_create_ringbuffer);


  /* ref class from a thread-safe context to work around missing bit of
   * thread-safety in GObject */
  g_type_class_ref (GST_TYPE_SPOTIFY);

  g_object_class_install_property (gobject_class, PROP_USER,
      g_param_spec_string ("user", "Username", "Username for premium spotify account", "unknown",
          G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_PASS,
      g_param_spec_string ("pass", "Password", "Password for premium spotify account", "unknown",
          G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_URI,
      g_param_spec_string ("uri", "URI", "A spotify URI currently only track uri:s are supported", "unknown",
          G_PARAM_READWRITE));
  //FIXME maybe init spotify stuff here
}

static void
gst_spotify_init (GstSpotify * spotify, GstSpotifyClass * gclass)
{
  //gst_base_src_set_live(GST_BASE_SRC (src), TRUE);
  GST_SPOTIFY_USER (spotify) = g_strdup (DEFAULT_USER);
  GST_SPOTIFY_PASS (spotify) = g_strdup (DEFAULT_PASS);
  GST_SPOTIFY_URI (spotify) = g_strdup (DEFAULT_URI);
}

static void
gst_spotify_finalize (GObject *object)
{
  GstSpotify *src = GST_SPOTIFY (object);
  g_free (GST_SPOTIFY_USER (src));
  g_free (GST_SPOTIFY_PASS (src));
  g_free (GST_SPOTIFY_URI (src));
  //call parent
}

static void
gst_spotify_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  GstSpotify *src = GST_SPOTIFY (object);

  //FIXME how to handle if a prop i reset, this might be handled be reset function
  GST_OBJECT_LOCK (src);
  switch (prop_id) {
    case PROP_USER:
      g_free (GST_SPOTIFY_USER (src));
      GST_SPOTIFY_USER (src) = g_strdup (g_value_get_string (value));
      break;
    case PROP_PASS:
      g_free (GST_SPOTIFY_PASS (src));
      GST_SPOTIFY_PASS (src) = g_strdup (g_value_get_string (value));
      break;
    case PROP_URI:
      g_free (GST_SPOTIFY_URI (src));
      GST_SPOTIFY_URI (src) = g_strdup (g_value_get_string (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
  GST_OBJECT_UNLOCK (src);
}

static void
gst_spotify_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{
  GstSpotify *src = GST_SPOTIFY (object);

  GST_OBJECT_LOCK (src);
  switch (prop_id) {
    case PROP_USER:
      g_value_set_string (value, GST_SPOTIFY_USER (src));
      break;
    case PROP_PASS:
      g_value_set_string (value, GST_SPOTIFY_PASS (src));
      break;
    case PROP_URI:
      g_value_set_string (value, GST_SPOTIFY_URI (src));
      break;
    default:
     G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
     break;
  }
  GST_OBJECT_UNLOCK (src);
}

static GstCaps *
gst_spotify_getcaps (GstBaseSrc * bsrc)
{
  GstElementClass *element_class;
  GstCaps *caps = NULL;
  GstPadTemplate *pad_template;
  GstSpotify *src;


  src = GST_SPOTIFY (bsrc);

  //FIXME check if this code is correct
  element_class = GST_ELEMENT_GET_CLASS (src);
  pad_template = gst_element_class_get_pad_template (element_class, "src");
  g_return_val_if_fail (pad_template != NULL, NULL);

  caps = gst_caps_ref (gst_pad_template_get_caps (pad_template));
  return caps;
}

static GstRingBuffer *
gst_spotify_create_ringbuffer (GstBaseAudioSrc * src)
{
  GstRingBuffer *buffer;

  buffer = g_object_new (GST_TYPE_SPOTIFY_RING_BUFFER, NULL);

  ring_buffer = buffer;
  return buffer;
}
/* end spotify */

