#ifndef SPOT_OBJ_H
#define SPOT_OBJ_H

#include <glib-object.h>

#define SPOT_OBJ_TYPE		  (spot_obj_get_type ())
#define SPOT_OBJ(obj)		  (G_TYPE_CHECK_INSTANCE_CAST ((obj), SPOT_OBJ_TYPE, SpotObj))
#define SPOT_OBJ_CLASS(klass)	  (G_TYPE_CHECK_CLASS_CAST ((klass), SPOT_OBJ_TYPE, SpotObjClass))
#define IS_SPOT_OBJ(obj)	  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SPOT_TYPE_OBj))
#define IS_SPOT_OBJ_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), SPOT_OBJ_TYPE))
#define SPOT_OBJ_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), SPOT_OBJ_TYPE, SpotObjClass))

#define SPOT_OBJ_USER(o) ((o)->user)
#define SPOT_OBJ_PASS(o) ((o)->pass)
#define SPOT_OBJ_SPOTIFY_URI(o) ((o)->spotify_uri)
#define SPOT_OBJ_CURRENT_TRACK(o) ((o)->current_track)
#define SPOT_OBJ_LOGGED_IN(o) ((o)->logged_in)
#define SPOT_OBJ_SPOTIFY_SESSION(o) ((o)->spotify_session)

typedef struct _SpotObj SpotObj;
typedef struct _SpotObjClass SpotObjClass;

struct _SpotObj {
	GObject parent;
	/* instance members */
        gchar *user;
        gchar *pass;
        gchar *spotify_uri;
        sp_track *current_track;
        sp_session *spotify_session;
        gboolean logged_in;
};

struct _SpotObjClass {
	GObjectClass parent;

	/* class members */
};

GType spot_obj_get_type (void);

/* API. */

void spot_obj_do_action_public (SpotObj *self, guint8 i);

#endif /* SPOT_OBJ_H */



