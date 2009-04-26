using Gst;
using GConf;

[DBus (name = "com.DogVibes.www")]
public class DogvibesServer : GLib.Object {

  private Pipeline pipeline;
  private Input spotify;

  construct {
    /* FIXME all of this should be in a list */
    this.spotify = new SpotifyInput();
  }

  public void play (int input, int output, string key) {
	/* elements for final pipeline */
	Element filter = null;
	Element src = null;
	Element sink = null;
	/* inputs */
	Element localmp3;
	Element srse;
	//Element lastfm;
	/* outputs */
	Element apexsink;
	Element alsasink;
	/* filter */
	Element madfilter;

	bool use_filter = false;
	bool use_playbin = false;

	stdout.printf ("PLAYING ");

	/* FIXME, one pipeline isn't enough */
	this.pipeline = (Pipeline) new Pipeline ("dogvibes");
	this.pipeline.set_state (State.NULL);

	/* inputs */
	if (input == 0) {
      src = this.spotify.get_src(key);
	} else if (input == 1) {
	  use_filter = true;
	  stdout.printf("MP3 input ");
	  localmp3 = ElementFactory.make ("filesrc", "file reader");
	  madfilter = ElementFactory.make ("mad" , "mp3 decoder");
	  localmp3.set("location", "../testmedia/beep.mp3");
	  src = localmp3;
	  filter = madfilter;
	} else if (input == 2) {
	  use_playbin = true;
	  /* swedish webradio */
	  stdout.printf("Internet radio ");
	  srse = ElementFactory.make ("playbin", "Internet radio");
	  srse.set ("uri", "mms://wm-live.sr.se/SR-P3-High");
	  src = srse;
	} else {
	  stdout.printf("Error not correct input %d\n", input);
	  return;
	}

	stdout.printf("on");

	if (output == 0){
	  stdout.printf(" ALSA sink \n");
	  alsasink = ElementFactory.make ("alsasink", "alsasink");
	  alsasink.set ("sync", false);
	  sink = alsasink;
	} else if (output == 1) {
	  stdout.printf(" APEX sink \n");
	  apexsink = ElementFactory.make ("apexsink", "apexsink");
	  apexsink.set ("host", "192.168.1.3");
	  apexsink.set ("volume", 100);
	  apexsink.set ("sync", false);
	  sink = apexsink;
	} else {
	  stdout.printf("Error not correct output %d\n", output);
	  return;
	}

	/* ugly */
	if (use_filter) {
	  if (src != null && sink != null && filter != null) {
		this.pipeline.add_many (src, filter, sink);
		src.link (filter);
		filter.link (sink);
		this.pipeline.set_state (State.PLAYING);
	  }
	} else if (use_playbin) {
	  if (src != null && sink != null) {
		((Bin)this.pipeline).add (src);
		this.pipeline.set_state (State.PLAYING);
	  }
	} else {
	  if (src != null && sink != null) {
		this.pipeline.add_many (src, sink);
		src.link (sink);
		this.pipeline.set_state (State.PLAYING);
	  }
	}

	use_playbin = false;
	use_filter = false;

  }

  public void pause () {
	this.pipeline.set_state (State.PAUSED);
  }

  public void resume () {
	this.pipeline.set_state (State.PLAYING);
  }

  public void stop () {
	this.pipeline.set_state (State.READY);
  }

  public void runsearch () {
  }

  public string[] search (string user, string pass, string searchstring) {
    stdout.printf ("%s %s not used anymore\n", user, pass);
    return spotify.search (searchstring);
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
	uint request_name_result = bus.request_name ("com.DogVibes.www", (uint) 0);

	if (request_name_result == DBus.RequestNameReply.PRIMARY_OWNER) {
	  var server = new DogvibesServer ();

      conn.register_object ("/com/dogvibes/www", server);
      loop.run ();
	}
  } catch (GLib.Error e) {
	stderr.printf ("Oops: %s\n", e.message);
  }
}
