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

#include "config.h"
#include <gst/gst.h>
#include "gstspotsrc.h"
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <gst/base/gstadapter.h>
#include <glib.h>

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
  ARG_LOCATION,
};

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
static GThread *spotify_thread;
static gboolean keep_threads = TRUE;
static GstAdapter *adapter;
static GMutex *adapter_mutex;

static int music_delivery (const void *frames, int num_frames)
{
  guint bufsize = num_frames * 4;
  GstBuffer *buffer;
  int sample_rate = 44100;
  int availible;
  g_print ("%s - start %p with %d frames\n",__FUNCTION__, frames, num_frames);
  buffer = gst_buffer_new_and_alloc (bufsize);

  GST_BUFFER_DATA(buffer) = (guint8*)frames;

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

void* spotify_stub_thread (void *unused)
{
  #define READ_SIZE 8192
  #define FILENAME "miljon.wav"
  int fd;
  int position = 0;
  int sleeptime;
  gchar *frames;

  g_print ("%s - start\n",__FUNCTION__);
  frames = g_malloc0 (READ_SIZE);
  adapter = gst_adapter_new();
  adapter_mutex = g_mutex_new();

  /* open the file first */
  fd = open (FILENAME, O_RDONLY, 0);
  if (fd < 0)
    goto open_failed;

  do {
    gint ret, rewind_len, consumed;
    gint num_frames = READ_SIZE / 4;
    /* read a spotsize of data here */
    g_print ("%s - reading %d bytes\n", __FUNCTION__, READ_SIZE);
    ret = read (fd, frames, READ_SIZE);
    if (G_UNLIKELY (ret < 0))
      goto could_not_read;
    /* seekable regular files should have given us what we expected */
    if (G_UNLIKELY ((guint) ret < READ_SIZE))
      goto unexpected_eof;

    position += ret;
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
      position += rewind_len;
      seek = lseek (fd, rewind_len, SEEK_CUR);
      if (G_UNLIKELY (seek < 0 || seek != position))
        goto seek_failed;
    }

    sleeptime = G_USEC_PER_SEC/1000;
    g_print ("%s - sleep %d usec\n", __FUNCTION__, sleeptime);
    //g_usleep (10);

  } while (keep_threads);

  g_print ("%s - exit\n",__FUNCTION__);
  g_thread_exit(NULL);
could_not_read:
  {
    g_print ("%s - could not read\n", __FUNCTION__);
    g_thread_exit(NULL);
  }
unexpected_eof:
  {
    g_print ("%s - eof\n", __FUNCTION__);
    g_thread_exit(NULL);
  }
seek_failed:
  {
    g_print ("%s - seek failed\n", __FUNCTION__);
    g_thread_exit(NULL);
  }
open_failed:
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
  GError *err = NULL;
  gobject_class = G_OBJECT_CLASS (klass);
  gstbasesrc_class = GST_BASE_SRC_CLASS (klass);

  gobject_class->set_property = gst_spot_src_set_property;
  gobject_class->get_property = gst_spot_src_get_property;

  g_object_class_install_property (gobject_class, ARG_LOCATION,
      g_param_spec_string ("location", "File Location",
          "Location of the file to read", NULL,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

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

  if ((spotify_thread = g_thread_create ((GThreadFunc)spotify_stub_thread, (void *)NULL, TRUE, &err)) == NULL) {
     printf ("g_thread_create failed: %s!!\n", err->message );
     g_error_free (err) ;
  }

}

static void
gst_spot_src_init (GstSpotSrc * src, GstSpotSrcClass * g_class)
{
  src->filename = NULL;
  src->uri = NULL;
  src->fd = -1;
  src->read_position = 0;
}

static void
gst_spot_src_finalize (GObject * object)
{
  GstSpotSrc *src;

  src = GST_SPOT_SRC (object);

  g_free (src->filename);
  g_free (src->uri);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static gboolean
gst_spot_src_set_location (GstSpotSrc * src, const gchar * location)
{
  GstState state;

  /* the element must be stopped in order to do this */
  GST_OBJECT_LOCK (src);
  state = GST_STATE (src);
  if (state != GST_STATE_READY && state != GST_STATE_NULL)
    goto wrong_state;
  GST_OBJECT_UNLOCK (src);

  g_free (src->filename);
  g_free (src->uri);

  src->filename = g_strdup (location);
  src->uri = gst_uri_construct ("spot", src->filename);

  gst_uri_handler_new_uri (GST_URI_HANDLER (src), src->uri);

  return TRUE;

  /* ERROR */
wrong_state:
  {
    GST_DEBUG_OBJECT (src, "setting location in wrong state");
    GST_OBJECT_UNLOCK (src);
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
    case ARG_LOCATION:
      GST_DEBUG_OBJECT (src, "setting location to: %s", g_value_get_string (value));
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
    case ARG_LOCATION:
      g_value_set_string (value, src->filename);
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
  g_print ("filename = %s\n", src->filename);
  if (strcmp (src->filename, "miljon.wav") != 0) {

    GstFlowReturn ret = GST_FLOW_OK;

    /* see if we have bytes to write */
    g_mutex_lock(adapter_mutex);             /* lock adapter */
    g_print ("%s - size=%u offset=%llu end=%llu read_position=%llu availible=%d\n", __FUNCTION__,
              length, offset, offset+length, src->read_position, gst_adapter_available (adapter));
    if (gst_adapter_available (adapter) >= length) {
      GstBuffer *buf;
      buf = gst_buffer_try_new_and_alloc (length);

      GST_BUFFER_SIZE (buf) = length;
      GST_BUFFER_OFFSET (buf) = offset;
      GST_BUFFER_OFFSET_END (buf) = offset + length;
      GST_BUFFER_DATA (buf) = (guint8*) gst_adapter_take (adapter, length);
      src->read_position += length;
      *buffer = buf;

      g_print ("%s - GST_FLOW_OK return\n", __FUNCTION__);
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
        goto seek_failed;

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
      goto could_not_read;

    // seekable regular files should have given us what we expected
    if (G_UNLIKELY ((guint) ret < length))
      goto unexpected_eos;

    // other files should eos if they read 0 and more was requested
    if (G_UNLIKELY (ret == 0 && length > 0))
      goto eos;

    length = ret;

    GST_BUFFER_SIZE (buf) = length;
    GST_BUFFER_OFFSET (buf) = offset;
    GST_BUFFER_OFFSET_END (buf) = offset + length;

    *buffer = buf;

    src->read_position += length;

    return GST_FLOW_OK;

  seek_failed:
    {
      GST_ELEMENT_ERROR (src, RESOURCE, READ, (NULL), GST_ERROR_SYSTEM);
      return GST_FLOW_ERROR;
    }
  could_not_read:
    {
      GST_ELEMENT_ERROR (src, RESOURCE, READ, (NULL), GST_ERROR_SYSTEM);
      gst_buffer_unref (buf);
      return GST_FLOW_ERROR;
    }
  unexpected_eos:
    {
      GST_ELEMENT_ERROR (src, RESOURCE, READ, (NULL), ("unexpected end of file."));
      gst_buffer_unref (buf);
      return GST_FLOW_ERROR;
    }
  eos:
    {
      GST_DEBUG ("non-regular file hits EOS");
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
  GstSpotSrc *src = GST_SPOT_SRC (basesrc);
  struct stat stat_results;

  if (src->filename == NULL || src->filename[0] == '\0')
    goto no_filename;

  GST_INFO_OBJECT (src, "opening file %s", src->filename);

  /* open the file */
  src->fd = open (src->filename, O_RDONLY, 0);

  if (src->fd < 0)
    goto open_failed;

  /* check if it is a regular file, otherwise bail out */
  if (fstat (src->fd, &stat_results) < 0)
    goto no_stat;

  src->read_position = 0;

  return TRUE;

  /* ERROR */
no_filename:
  {
    GST_ELEMENT_ERROR (src, RESOURCE, NOT_FOUND, (("No file name specified for reading.")), (NULL));
    return FALSE;
  }
open_failed:
  {
    switch (errno) {
      case ENOENT:
        GST_ELEMENT_ERROR (src, RESOURCE, NOT_FOUND, (NULL), ("No such file \"%s\"", src->filename));
        break;
      default:
        GST_ELEMENT_ERROR (src, RESOURCE, OPEN_READ, (("Could not open file \"%s\" for reading."), src->filename),GST_ERROR_SYSTEM);
        break;
    }
    return FALSE;
  }
no_stat:
  {
    GST_ELEMENT_ERROR (src, RESOURCE, OPEN_READ, (("Could not get info on \"%s\"."), src->filename), (NULL));
    close (src->fd);
    return FALSE;
  }
}

/* unmap and close the spot */
static gboolean
gst_spot_src_stop (GstBaseSrc * basesrc)
{
  GstSpotSrc *src = GST_SPOT_SRC (basesrc);

  /* close the file */
  close (src->fd);

  /* zero out a lot of our state */
  src->fd = 0;
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

