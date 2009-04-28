using Gst;
using GConf;

[DBus (name = "com.Dogvibes.Dogvibes")]
public class Dogvibes : GLib.Object {
  public string[] search (string user, string pass, string searchstring) {
    string[] hepp = {};
    //stdout.printf ("%s %s not used anymore\n", user, pass);
    //return spotify.search (searchstring);
    return hepp;
  }
}

[DBus (name = "com.Dogvibes.Amp")]
public class Amp : GLib.Object {
  private Pipeline pipeline = null;
  private Input spotify;
  private Element src = null;
  private Element sink = null;
  //private List playqueue;

  construct {
    /* FIXME all of this should be in a list */
    this.spotify = new SpotifyInput();
    //playqueue = new List("queuelist");
  }

  public void queue(string key) {
    stdout.printf ("PLAY\n");
    this.src = this.spotify.get_src (key);
  }

  public void play () {
    stdout.printf ("PLAY\n");

    if (this.src == null) {
      stdout.printf("You must enqueue a track\n");
      return;
    }

    this.pipeline = (Pipeline) new Pipeline ("dogvibes");
	this.pipeline.set_state (State.NULL);
    this.sink = ElementFactory.make ("alsasink", "alsasink");
    this.sink.set ("sync", false);
    this.pipeline.add_many (src, sink);
    this.src.link (sink);
    this.pipeline.set_state (State.PLAYING);
  }

  public void pause () {
	this.pipeline.set_state (State.PAUSED);
  }

  public void resume () {
    this.pipeline.set_state (State.PLAYING);
  }

  public void stop () {
    this.pipeline.set_state (State.NULL);
    //this.pipeline.unref();
    stdout.printf ("STOP\n");
  }
}

public void main (string[] args) {
    // Creating a GLib main loop with a default context
  var loop = new MainLoop (null, false);

  // Initializing GStreamer
  Gst.init (ref args);

  try {
	var conn = DBus.Bus.get (DBus.BusType. SYSTEM);

	dynamic DBus.Object bus = conn.get_object ("org.freedesktop.DBus",
											   "/org/freedesktop/DBus",
											   "org.freedesktop.DBus");
	// try to register service in session bus
	uint request_name_result = bus.request_name ("com.Dogvibes", (uint) 0);

	if (request_name_result == DBus.RequestNameReply.PRIMARY_OWNER) {
	  var dogvibes = new Dogvibes ();
      conn.register_object ("/com/dogvibes/dogvibes", dogvibes);
	  var amp = new Amp ();
      conn.register_object ("/com/dogvibes/amp/0", amp);

      loop.run ();
	}
  } catch (GLib.Error e) {
	stderr.printf ("Oops: %s\n", e.message);
  }
}


/* Maybe neede later on */
//	/* elements for final pipeline */
//	Element filter = null;
//	Element src = null;
//	Element sink = null;
//	/* inputs */
//	Element localmp3;
//	Element srse;
//	//Element lastfm;
//	/* outputs */
//	Element apexsink;
//	Element alsasink;
//	/* filter */
//	Element madfilter;
//
//	bool use_filter = false;
//	bool use_playbin = false;
//
//	stdout.printf ("PLAYING ");
//
//	/* FIXME, one pipeline isn't enough */
//	this.pipeline = (Pipeline) new Pipeline ("dogvibes");
//	this.pipeline.set_state (State.NULL);
//
//	/* inputs */
//	if (input == 0) {
//      src = this.spotify.get_src(key);
//	} else if (input == 1) {
//	  use_filter = true;
//	  stdout.printf("MP3 input ");
//	  localmp3 = ElementFactory.make ("filesrc", "file reader");
//	  madfilter = ElementFactory.make ("mad" , "mp3 decoder");
//	  localmp3.set("location", "../testmedia/beep.mp3");
//	  src = localmp3;
//	  filter = madfilter;
//	} else if (input == 2) {
//	  use_playbin = true;
//	  /* swedish webradio */
//	  stdout.printf("Internet radio ");
//	  srse = ElementFactory.make ("playbin", "Internet radio");
//	  srse.set ("uri", "mms://wm-live.sr.se/SR-P3-High");
//	  src = srse;
//	} else {
//	  stdout.printf("Error not correct input %d\n", input);
//	  return;
//	}
//
//	stdout.printf("on");
//
//	if (output == 0){
//	  stdout.printf(" ALSA sink \n");
//	  alsasink = ElementFactory.make ("alsasink", "alsasink");
//	  alsasink.set ("sync", false);
//	  sink = alsasink;
//	} else if (output == 1) {
//	  stdout.printf(" APEX sink \n");
//	  apexsink = ElementFactory.make ("apexsink", "apexsink");
//	  apexsink.set ("host", "192.168.1.3");
//	  apexsink.set ("volume", 100);
//	  apexsink.set ("sync", false);
//	  sink = apexsink;
//	} else {
//	  stdout.printf("Error not correct output %d\n", output);
//	  return;
//	}
//
//	/* ugly */
//	if (use_filter) {
//	  if (src != null && sink != null && filter != null) {
//		this.pipeline.add_many (src, filter, sink);
//		src.link (filter);
//		filter.link (sink);
//		this.pipeline.set_state (State.PLAYING);
//	  }
//	} else if (use_playbin) {
//	  if (src != null && sink != null) {
//		((Bin)this.pipeline).add (src);
//		this.pipeline.set_state (State.PLAYING);
//	  }
//	} else {
//	  if (src != null && sink != null) {
//		this.pipeline.add_many (src, sink);
//		src.link (sink);
//		this.pipeline.set_state (State.PLAYING);
//	  }
//	}