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


/* src args */

enum
{
  ARG_0,
  ARG_SPOTIFY_URI,
  ARG_USER,
  ARG_PASS,
  ARG_URI,
  ARG_BUFFER_TIME
};

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
static void* spotify_thread_func (void *ptr);

/* basesrc stuff */
static void gst_spot_src_finalize (GObject * object);
static void gst_spot_src_set_property (GObject * object, guint prop_id, const GValue * value, GParamSpec * pspec);
static void gst_spot_src_get_property (GObject * object, guint prop_id, GValue * value, GParamSpec * pspec);
static gboolean gst_spot_src_start (GstBaseSrc * basesrc);
static gboolean gst_spot_src_stop (GstBaseSrc * basesrc);
static gboolean gst_spot_src_is_seekable (GstBaseSrc * src);
static gboolean gst_spot_src_get_size (GstBaseSrc * src, guint64 * size);
static GstFlowReturn gst_spot_src_create (GstBaseSrc * src, guint64 offset, guint length, GstBuffer ** buffer);
static gboolean gst_spot_src_query (GstBaseSrc * src, GstQuery * query);

/* uri interface */
static gboolean gst_spot_src_set_spotifyuri (GstSpotSrc * src, const gchar * spotify_uri);
static void gst_spot_src_uri_handler_init (gpointer g_iface, gpointer iface_data);
static gboolean gst_spot_src_uri_set_uri (GstURIHandler * handler, const gchar * uri);
static const gchar *gst_spot_src_uri_get_uri (GstURIHandler * handler);
static gchar **gst_spot_src_uri_get_protocols (void);
static GstURIType gst_spot_src_uri_get_type (void);

static gboolean spot_obj_create_session (SpotObj *self);
static gboolean spot_obj_login (SpotObj *self);

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
  &spotify_cb_log_message
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

/* signal the thread_func using the cond so it can process_events
 * when needed */
static GMutex *process_events_mutex;
static GThread *process_events_thread;
static GCond *process_events_cond;

/* libspotify functions are not threadsafe, protect them with
 * this mutex */
static GMutex *spotifylib_mutex;

/* change this to let the spotify thread that invokes
 * sp_process_events exit, use this FIXME: */
static gboolean keep_spotify_thread = TRUE;

static GstSpotSrc *spot;

/*****************************************************************************/
/*** LIBSPOTIFY FUNCTIONS ****************************************************/

static void
spotify_cb_metadata_updated (sp_session *session)
{
  GST_DEBUG_OBJECT (spot, "metadata updated");
}

static void
spotify_cb_message_to_user (sp_session *session, const char *msg)
{
  GST_DEBUG_OBJECT (spot, "message to user: %s", msg);
}

static void
spotify_cb_play_token_lost (sp_session *session)
{
  GST_DEBUG_OBJECT (spot, "play token lost");
}

static void
spotify_cb_connection_error (sp_session *spotify_session, sp_error error)
{
  GST_ERROR ("connection to Spotify failed: %s\n",
      sp_error_message (error));
}

static void
spotify_cb_logged_in (sp_session *spotify_session, sp_error error)
{
  if (SP_ERROR_OK != error) {
    GST_ERROR ("failed to log in to Spotify: %s\n", sp_error_message (error));
    return;
  }

  sp_user *me = sp_session_user (spotify_session);
  const char *my_name = (sp_user_is_loaded (me) ?
                         sp_user_display_name (me) :
                         sp_user_canonical_name (me));

  //FIXME debug
  GST_DEBUG ("Logged in to Spotify as user %s", my_name);
  SPOT_OBJ_LOGGED_IN (spot_instance) = TRUE;
}

static void
spotify_cb_logged_out (sp_session *spotify_session)
{
  //FIXME debug
  GST_DEBUG_OBJECT (spot, "logged out from spotify");
}

static void
spotify_cb_log_message (sp_session *spotify_session, const char *data)
{
  //FIXME debug
  GST_DEBUG_OBJECT (spot, "log_message:'%s'", data);
}

static void
spotify_cb_notify_main_thread (sp_session *spotify_session)
{
  GST_DEBUG_OBJECT (spot, "broadcast cond");
  /* signal thread to process events */
  g_cond_broadcast (process_events_cond);
}

static int
spotify_cb_music_delivery (sp_session *spotify_session, const sp_audioformat *format,const void *frames, int num_frames)
{
  GstBuffer *buffer;
  guint sample_rate = format->sample_rate;
  guint channels = format->channels;
  guint bufsize = num_frames * sizeof (int16_t) * channels;
  guint availible;

  /*FIXME: can this change? when? */
  if (G_UNLIKELY (GST_SPOT_SRC_FORMAT (spot) == NULL)) {
    GST_SPOT_SRC_FORMAT (spot) = g_malloc0 (sizeof (sp_audioformat));
    memcpy (GST_SPOT_SRC_FORMAT (spot), format, sizeof (sp_audioformat));
  }

  GST_DEBUG_OBJECT (spot,"%s - start %p with %d frames with size=%d\n",__FUNCTION__, frames, num_frames, bufsize);

  //FIXME: send EOS

  buffer = gst_buffer_new_and_alloc (bufsize);

  memcpy (GST_BUFFER_DATA (buffer), (guint8*)frames, bufsize);

  /* see if we have buffertime us of audio */
  g_mutex_lock (GST_SPOT_SRC_ADAPTER_MUTEX (spot));            /* lock adapter */
  availible = gst_adapter_available (GST_SPOT_SRC_ADAPTER (spot));
  GST_DEBUG_OBJECT (spot,"%s - availiable before push = %d\n", __FUNCTION__, availible);
  if (availible >= (GST_SPOT_SRC_BUFFER_TIME (spot)/1000000) * sample_rate * 4) {
    GST_DEBUG_OBJECT (spot,"%s - return 0, adapter is full = %d\n", __FUNCTION__, availible);
    gst_buffer_unref (buffer);
    g_mutex_unlock (GST_SPOT_SRC_ADAPTER_MUTEX (spot));        /* unlock adapter */
    return 0;
  }

  gst_adapter_push (GST_SPOT_SRC_ADAPTER (spot), buffer);
  availible = gst_adapter_available (GST_SPOT_SRC_ADAPTER (spot));
  GST_DEBUG_OBJECT (spot,"%s - availiable after push = %d\n", __FUNCTION__, availible);
  g_mutex_unlock (GST_SPOT_SRC_ADAPTER_MUTEX (spot));          /* unlock adapter */

  GST_DEBUG_OBJECT (spot,"%s - return %d\n",__FUNCTION__, num_frames);
  return num_frames;
}

/* only used to trigger sp_session_process_events when needed,
 * looks like about once a second */
static void*
spotify_thread_func (void *ptr)
{
   GTimeVal t;
   gboolean in_time;
   int timeout = -1;
   /* wait for first broadcast */
   g_cond_wait (process_events_cond, process_events_mutex);
   do {
     g_mutex_lock (spotifylib_mutex);
     sp_session_process_events (SPOT_OBJ_SPOTIFY_SESSION (spot_instance), &timeout);
     g_mutex_unlock (spotifylib_mutex);
     g_get_current_time (&t);
     g_time_val_add (&t, timeout*1000);
     GST_DEBUG ("\nWAITING FOR BROADCAST (timeout = %d ms)\n", timeout);
     in_time = g_cond_timed_wait (process_events_cond, process_events_mutex, &t);
     GST_DEBUG ("GOT %s\n", in_time ? "BROADCAST" : "TIMEOUT");
   } while (keep_spotify_thread);
   //FIXME
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

static gboolean spot_obj_create_session (SpotObj *self)
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
          GST_ERROR ("failed to create spotify_session: %s\n", sp_error_message (error));
          return FALSE;
        }

        return TRUE;
}
static gboolean spot_obj_login (SpotObj *self)
{
  sp_error error;
  if (SPOT_OBJ_LOGGED_IN (spot_instance)) {
    g_print ("already logged in..\n");
    return TRUE;
  }

  /* Login using the credentials given on the command line */
  g_mutex_lock (spotifylib_mutex);
  error = sp_session_login (SPOT_OBJ_SPOTIFY_SESSION (spot_instance), SPOT_OBJ_USER (spot_instance) , SPOT_OBJ_PASS (spot_instance));
  g_mutex_unlock (spotifylib_mutex);

  if (SP_ERROR_OK != error) {
    GST_ERROR ("failed to login: %s\n", sp_error_message (error));
    return FALSE;
  }

  //FIXME this is probably not the best way to wait to be logged in
  g_cond_broadcast (process_events_cond);
  while (!SPOT_OBJ_LOGGED_IN (spot_instance)) {
    usleep (10000);
  }
  g_print ("success - logged in!\n");

 return TRUE;
}


static void
spot_obj_instance_init (GTypeInstance   *instance,
                         gpointer         g_class)
{
        SpotObj *self = (SpotObj *)instance;
        self->user = NULL;
        self->pass = NULL;
        self->spotify_uri = NULL;


}


static void
spot_obj_set_property (GObject      *object,
                        guint         property_id,
                        const GValue *value,
                        GParamSpec   *pspec)
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
          printf ("set spotifyuri=%s\n", g_value_get_string (value));
          break;
        default:
           G_OBJECT_WARN_INVALID_PROPERTY_ID (object,property_id,pspec);
           break;
        }
}

static void
spot_obj_get_property (GObject      *object,
                        guint         property_id,
                        GValue       *value,
                        GParamSpec   *pspec)
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
spot_obj_constructor (GType type,
                       guint n_construct_properties,
                       GObjectConstructParam *construct_properties)
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
                        NULL,   /* base_init */
                        NULL,   /* base_finalize */
                        spot_obj_class_init,   /* class_init */
                        NULL,   /* class_finalize */
                        NULL,   /* class_data */
                        sizeof (SpotObj),
                        0,      /* n_preallocs */
                        spot_obj_instance_init    /* instance_init */
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
  GST_DEBUG_CATEGORY_INIT (gst_spot_src_debug, "spotsrc", 0, "spotsrc element");
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
      "joelbits@gmail.com & johan.gyllenspetz@gmail.com");
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

   process_events_cond = g_cond_new ();
   process_events_mutex = g_mutex_new ();
   spotifylib_mutex = g_mutex_new ();

   spot_instance = g_object_new (SPOT_OBJ_TYPE, NULL);

   if (!spot_obj_create_session (spot_instance))
     g_print ("session error\n");

}

static void
gst_spot_src_init (GstSpotSrc * src, GstSpotSrcClass * g_class)
{
  GError *err;
  src->read_position = 0;

  //move
  spot = GST_SPOT_SRC (src);

  GST_SPOT_SRC_URI (spot) = g_strdup (DEFAULT_URI);

  GST_SPOT_SRC_BUFFER_TIME (spot) = BUFFER_TIME_DEFAULT;

  GST_SPOT_SRC_ADAPTER_MUTEX (spot) = g_mutex_new ();
  GST_SPOT_SRC_ADAPTER (spot) = gst_adapter_new ();

  GST_SPOT_SRC_FORMAT (spot) = NULL;

  if ((process_events_thread = g_thread_create ((GThreadFunc)spotify_thread_func, (void *)NULL, TRUE, &err)) == NULL) {
     GST_DEBUG_OBJECT (spot,"g_thread_create failed: %s!!\n", err->message );
     g_error_free (err) ;
  }
}

static void
gst_spot_src_finalize (GObject * object)
{
  GstSpotSrc *src;

  src = GST_SPOT_SRC (object);

  GST_DEBUG_OBJECT (spot,"finalized\n");
  
  keep_spotify_thread = FALSE;
  g_thread_join (process_events_thread);

  g_object_unref (spot_instance);

  g_free (GST_SPOT_SRC_URI (src));

  g_free (GST_SPOT_SRC_FORMAT (spot));

  g_cond_free (process_events_cond);
  g_mutex_free (process_events_mutex);
  g_mutex_free (spotifylib_mutex);

  g_mutex_free (GST_SPOT_SRC_ADAPTER_MUTEX (spot));
  g_object_unref (GST_SPOT_SRC_ADAPTER (spot));

  G_OBJECT_CLASS (parent_class)->finalize (object);

}
void                gst_base_src_set_format             (GstBaseSrc *src,
                                                         GstFormat format);
static void
gst_spot_src_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  GstSpotSrc *src;

  g_return_if_fail (GST_IS_SPOT_SRC (object));

  src = GST_SPOT_SRC (object);

  switch (prop_id) {
    case ARG_USER:
      g_object_set (spot_instance, "user", g_value_get_string (value), NULL);
      break;
    case ARG_PASS:
      g_object_set (spot_instance, "pass", g_value_get_string (value), NULL);
      break;
    case ARG_URI:
      gst_spot_src_set_spotifyuri (src, g_value_get_string (value));
      break;
    case ARG_SPOTIFY_URI:
      g_object_set (spot_instance, "spotifyuri", g_value_get_string (value), NULL);
      printf ("set spotifyuri=%s\n", g_value_get_string (value));
      break;
    case ARG_BUFFER_TIME:
      GST_SPOT_SRC_BUFFER_TIME (src) = (g_value_get_uint64 (value));
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
      g_value_set_string (value, SPOT_OBJ_USER (spot_instance));
      break;
    case ARG_PASS:
      g_value_set_string (value, SPOT_OBJ_PASS (spot_instance));
      break;
    case ARG_SPOTIFY_URI:
      g_value_set_string (value, SPOT_OBJ_SPOTIFY_URI (spot_instance));
      break;
    case ARG_URI:
      g_value_set_string (value, GST_SPOT_SRC_URI (src));
      break;
    case ARG_BUFFER_TIME:
      g_value_set_uint64 (value, GST_SPOT_SRC_BUFFER_TIME (src));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static GstFlowReturn
gst_spot_src_create_read (GstSpotSrc * src, guint64 offset, guint length, GstBuffer ** buffer)
{
    GstFlowReturn ret = GST_FLOW_OK;
    GstBuffer *buf;
    guint avail;
    if (G_UNLIKELY (src->read_position != offset)) {
      sp_error error;
      /* implement spotify seek here */
      gint sample_rate = GST_SPOT_SRC_FORMAT (spot)->sample_rate;
      gint channels = GST_SPOT_SRC_FORMAT (spot)->channels;
      gint64 frames = offset / (channels * sizeof (int16_t));
      g_print ("offset (%lld) / channels (%d) * samplesize (%d) = frames (%lld)\n", offset, channels, sizeof (int16_t), frames);
      gint64 seek_usec = frames / ((float)sample_rate/1000);
      g_print ("seek_usec = (%lld) = frames (%lld) /  sample_rate (%d/1000)\n", seek_usec, frames, sample_rate);
      g_print ("perform seek to %lld bytes and %lld usec\n", offset, seek_usec);
      error = sp_session_player_seek (SPOT_OBJ_SPOTIFY_SESSION (spot_instance), seek_usec);
      if (error != SP_ERROR_OK) {
        g_print ("seek error!!\n");
        goto create_seek_failed;
      }
      src->read_position = offset;
    }

    /* see if we have bytes to write */
    GST_DEBUG_OBJECT (spot,"%s - length=%u offset=%llu end=%llu read_position=%llu\n", __FUNCTION__,
              length, offset, offset+length, src->read_position);
    do {
      g_mutex_lock (GST_SPOT_SRC_ADAPTER_MUTEX (spot));             /* lock adapter */
      avail = gst_adapter_available (GST_SPOT_SRC_ADAPTER (spot));
      g_mutex_unlock (GST_SPOT_SRC_ADAPTER_MUTEX (spot));           /* unlock adapter */
      //FIXME: ?h
      g_usleep (40);
      //g_print ("%s - avail = %d length = %u\n", __FUNCTION__, avail, length);
    } while (avail <= length);

    buf = gst_buffer_try_new_and_alloc (length);

    GST_BUFFER_SIZE (buf) = length;
    GST_BUFFER_OFFSET (buf) = offset;
    GST_BUFFER_OFFSET_END (buf) = offset + length;
    memcpy (GST_BUFFER_DATA (buf), gst_adapter_peek (GST_SPOT_SRC_ADAPTER (spot), length), length);
    gst_adapter_flush (GST_SPOT_SRC_ADAPTER (spot), length);
    src->read_position += length;
    *buffer = buf;

    //g_print ("%s - GST_FLOW_OK return\n", __FUNCTION__);
    return ret;

  create_seek_failed:
    {
      g_print ("%s - could seek failed\n", __FUNCTION__);
      return GST_FLOW_ERROR;
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
  gboolean ret = TRUE;
  GstSpotSrc *src = GST_SPOT_SRC (basesrc);
  gint samplerate = GST_SPOT_SRC_FORMAT (spot)->sample_rate;

  switch (GST_QUERY_TYPE (query)) {
    case GST_QUERY_URI:
      gst_query_set_uri (query, src->uri);
    break;

    case GST_QUERY_DURATION:{
      GstFormat format;
      gint duration;
      gint64 value;
      gst_query_parse_duration (query, &format, &value);
      g_mutex_lock (spotifylib_mutex);
      /* duration in ms */
      duration = sp_track_duration (SPOT_OBJ_CURRENT_TRACK (spot_instance));
      g_mutex_unlock (spotifylib_mutex);

      switch (format) {
        case GST_FORMAT_BYTES:
          gst_query_set_duration (query, format, (duration/1000) * samplerate * 4);
          break;

        case GST_FORMAT_TIME:
          /* set it to nanoseconds */
          gst_query_set_duration (query, format, duration*1000);
          break;

        default:
          ret = FALSE;
          break;
      }
      break;
    }
    case GST_QUERY_CONVERT:
    {
      GstFormat src_fmt, dest_fmt;
      gint64 src_val, dest_val;

      gst_query_parse_convert (query, &src_fmt, &src_val, &dest_fmt, &dest_val);
      g_print ("convert src_fmt %d dst_fmt %d\n", src_fmt, dest_fmt);
      g_print ("        src_val %lld dst_val %lld\n", src_val, dest_val);

      if (src_fmt == dest_fmt) {
        dest_val = src_val;
        goto done;
      }

      switch (src_fmt) {
        case GST_FORMAT_BYTES:
          g_print ("dst_fmt == %d == FORMAT_DEFAULT\n", dest_fmt);
          switch (dest_fmt) {
            case GST_FORMAT_TIME:
              /* samples to time */
              dest_val = src_val / ((float)samplerate * 4 / 1000000);
              g_print ("dest_val = %lld\n", dest_val);
              break;

            default:
              ret = FALSE;
              break;
          }
          break;

        case GST_FORMAT_TIME:
          g_print ("dst_fmt == %d == FORMAT_TIME\n", dest_fmt);
          switch (dest_fmt) {
            case GST_FORMAT_BYTES:
              /* time to samples */
              dest_val = src_val/1000000 * samplerate * 4;
              g_print ("dest_val = %lld\n", dest_val);
              break;

            default:
              g_print ("default = %lld\n", dest_val);
              ret = FALSE;
              break;
          }
          break;

        default:
          ret = FALSE;
          break;
      }
    done:
      gst_query_set_convert (query, src_fmt, src_val, dest_fmt, dest_val);
      break;
    }
    default:
      ret = FALSE;
      break;
  } 

  if (!ret) {
    ret = GST_BASE_SRC_CLASS (parent_class)->query (basesrc, query);
  } 

  if (!ret) {
    GST_DEBUG_OBJECT (src, "query failed");
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
  GstSpotSrc *src;
  int duration;

  src = GST_SPOT_SRC (basesrc);

  g_mutex_lock (spotifylib_mutex);
  /* duration in ms */
  duration = sp_track_duration (SPOT_OBJ_CURRENT_TRACK (spot_instance));
  g_mutex_unlock (spotifylib_mutex);


  if (!duration)
    goto no_duration;

  *size = (duration/1000) * 44100 * 4;
  g_print ("%s - duration=%d, size=%lld\n", __FUNCTION__, duration, *size);

  return TRUE;

  /* ERROR */
no_duration:
  {
    g_print ("%s - error? no duration\n", __FUNCTION__);
    return FALSE;
  }
}

static gboolean
gst_spot_src_start (GstBaseSrc * basesrc)
{
  GST_DEBUG ("%s - enter", __FUNCTION__);

  if (!spot_obj_login (spot_instance))
    g_print ("login error\n");


  GST_DEBUG_OBJECT (basesrc, "uri = %s\n", SPOT_OBJ_SPOTIFY_URI (spot_instance));

  g_mutex_lock (spotifylib_mutex);
  sp_link *link = sp_link_create_from_string (SPOT_OBJ_SPOTIFY_URI (spot_instance));
  g_mutex_unlock (spotifylib_mutex);

  if (!link) {
    GST_ERROR_OBJECT (spot, "Incorrect track ID");
    return FALSE;
  }

  g_mutex_lock (spotifylib_mutex);
  SPOT_OBJ_CURRENT_TRACK (spot_instance) = sp_link_as_track (link);
  g_mutex_unlock (spotifylib_mutex);
  if (!SPOT_OBJ_CURRENT_TRACK (spot_instance)) {
    GST_DEBUG_OBJECT (spot, "Only track ID:s are currently supported");
    return FALSE;
  }

  g_mutex_lock (spotifylib_mutex);
  sp_track_add_ref (SPOT_OBJ_CURRENT_TRACK (spot_instance));
  g_mutex_unlock (spotifylib_mutex);
  g_mutex_lock (spotifylib_mutex);
  sp_link_release (link);
  g_mutex_unlock (spotifylib_mutex);

  //FIXME not the best way to wait for a track to be loaded
  g_cond_broadcast (process_events_cond);
  g_mutex_lock (spotifylib_mutex);
  while (sp_track_is_loaded (SPOT_OBJ_CURRENT_TRACK (spot_instance)) == 0) {
    g_mutex_unlock (spotifylib_mutex);
    usleep (10000);
    g_mutex_lock (spotifylib_mutex);
  }
  g_mutex_unlock (spotifylib_mutex);
  GST_DEBUG_OBJECT (basesrc, "track loaded!\n");

  g_mutex_lock (spotifylib_mutex);
  GST_DEBUG_OBJECT (spot, "Now playing \"%s\"...\n", sp_track_name (SPOT_OBJ_CURRENT_TRACK (spot_instance)));
  g_mutex_unlock (spotifylib_mutex);

  g_mutex_lock (spotifylib_mutex);
  sp_session_player_load (SPOT_OBJ_SPOTIFY_SESSION (spot_instance), SPOT_OBJ_CURRENT_TRACK (spot_instance));
  g_mutex_unlock (spotifylib_mutex);
  g_mutex_lock (spotifylib_mutex);
  sp_session_player_play (SPOT_OBJ_SPOTIFY_SESSION (spot_instance), 1);
  g_mutex_unlock (spotifylib_mutex);

  /* FIXME! remove */
//  g_usleep(GST_SPOT_SRC_BUFFER_TIME(spot));
  return TRUE;

}

static gboolean
gst_spot_src_stop (GstBaseSrc * basesrc)
{
  GST_DEBUG_OBJECT (basesrc, "STOP\n");
  g_mutex_lock (spotifylib_mutex);
  sp_session_player_unload (SPOT_OBJ_SPOTIFY_SESSION (spot_instance));
  g_mutex_unlock (spotifylib_mutex);

  //FIXME someone is holding references
  g_mutex_lock (spotifylib_mutex);
  sp_track_release (SPOT_OBJ_CURRENT_TRACK (spot_instance));
  g_mutex_unlock (spotifylib_mutex);

  /* clear adapter (we are stopped and do not continue from same place) */
  gst_adapter_clear (GST_SPOT_SRC_ADAPTER (spot));

  return TRUE;
}

static gboolean
gst_spot_src_set_spotifyuri (GstSpotSrc * src, const gchar * spotify_uri)
{
  GstState state;

  gchar *s_uri = SPOT_OBJ_SPOTIFY_URI (spot_instance);
  /* the element must be stopped in order to do this */
  state = GST_STATE (src);
  if (state != GST_STATE_READY && state != GST_STATE_NULL) {
    goto wrong_state;
  }

  g_free (s_uri);
  g_free (src->uri);

  /* clear the both uri/spotify_uri if we get a NULL (is that possible?) */
  if (spotify_uri == NULL) {
    s_uri = NULL;
    src->uri = NULL;
  } else {
    /* we store the spotify_uri as received by the application. On Windoes this
     * should be UTF8 */
    s_uri = g_strdup (spotify_uri);
    src->uri = gst_uri_construct ("spotify", s_uri);
  }
  g_object_notify (G_OBJECT (src), "spotifyuri"); /* why? */
  gst_uri_handler_new_uri (GST_URI_HANDLER (src), src->uri);

  g_print ("s_uri = %s\n", s_uri);
  return TRUE;

  /* ERROR */
wrong_state:
  {
    GST_DEBUG_OBJECT (src, "setting spotify_uri in wrong state");
    return FALSE;
  }
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

  ret = gst_spot_src_set_spotifyuri (src, location);

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

