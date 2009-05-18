/* GStreamer
 * Copyright (C) 2009 Joel Larsson
 *                    Johan Gyllenspetz
 *
 *
 * gstspotesrc.h:
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

#ifndef __GST_SPOT_SRC_H__
#define __GST_SPOT_SRC_H__

#include <sys/types.h>

#include <gst/gst.h>
#include <spotify/api.h>
#include <gst/base/gstadapter.h>
#include <gst/base/gstbasesrc.h>

G_BEGIN_DECLS

#define GST_TYPE_SPOT_SRC \
  (gst_spot_src_get_type())
#define GST_SPOT_SRC(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_SPOT_SRC,GstSpotSrc))
#define GST_SPOT_SRC_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_SPOT_SRC,GstSpotSrcClass))
#define GST_IS_SPOT_SRC(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_SPOT_SRC))
#define GST_IS_SPOT_SRC_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_SPOT_SRC))

typedef struct _GstSpotSrc GstSpotSrc;
typedef struct _GstSpotSrcClass GstSpotSrcClass;

#define GST_SPOT_SRC_USER(src) ((src)->user)
#define GST_SPOT_SRC_PASS(src) ((src)->pass)
#define GST_SPOT_SRC_URI(src) ((src)->uri)
#define GST_SPOT_SRC_SPOTIFY_URI(src) ((src)->spotify_uri)
#define GST_SPOT_SRC_BUFFER_TIME(src) ((src)->buffer_time)
#define GST_SPOT_SRC_ADAPTER(src) ((src)->adapter)
#define GST_SPOT_SRC_ADAPTER_MUTEX(src) ((src)->adapter_mutex)
#define GST_SPOT_SRC_FORMAT(src) ((src)->format)
//remove this?
#define GST_SPOT_SRC_BUFFER_INTERNAL_BYTES(src) ((src)->buffer_internal_bytes)


/**
 * GstSpotSrc:
 *
 * Opaque #GstSpotSrc structure.
 */
struct _GstSpotSrc {
  GstBaseSrc element;

  /*< private >*/

  gchar *user;
  gchar *pass;
  gchar *spotify_uri;
  gchar *uri;
  guint64 read_position;
  guint64 buffer_time;
  GstAdapter *adapter;
  GMutex *adapter_mutex;
  sp_audioformat *format;
};

struct _GstSpotSrcClass {
  GstBaseSrcClass parent_class;
};

GType gst_spot_src_get_type (void);

G_END_DECLS

#endif /* __GST_SPOT_SRC_H__ */

