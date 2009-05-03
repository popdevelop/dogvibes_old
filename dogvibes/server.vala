using Gst;
using GConf;

[DBus (name = "com.Dogvibes.Dogvibes")]
public class Dogvibes : GLib.Object {
  private void runsearch () {
  }

  public string[] search (string query) {
    /* FIXME this should use the spotify-source search method */

    string[] test = {};
    string user;
    string pass;

    try {
      var gc = GConf.Client.get_default ();
      user = gc.get_string ("/apps/dogvibes/spotify/username");
      pass = gc.get_string ("/apps/dogvibes/spotify/password");
      stdout.printf ("Creating spotify source with %s %s\n", user, pass);
    } catch (GLib.Error e) {
      stderr.printf ("Oops: %s\n", e.message);
    }

    try {
      string[] argus = {"search", user, pass, query};
      string[] envps = {"LD_LIBRARY_PATH=/home/gyllen/X11bin/lib/"};
      string uris;
      GLib.Process.spawn_sync (".", argus, envps, 0, runsearch, out uris);
      test = uris.split ("\n");
      stdout.printf ("%s\n", uris);
    } catch (GLib.Error e) {
      stdout.printf ("ERROR SO INTERNAL: %s\n", e.message);
    }

    stdout.printf ("I did a search on %s\n", query);

    return test;
  }
}

[DBus (name = "com.Dogvibes.Amp")]
public class Amp : GLib.Object {
  /* the amp pipeline */
  private Pipeline pipeline = null;

  /* sources */
  private Source spotify;

  /* speakers */
  private Speaker fakespeaker = null;
  private Speaker devicespeaker = null;

  /* elements */
  private Element src = null;
  private Element sink1 = null;
  private Element sink2 = null;
  private Element tee = null;

  construct {
    /* FIXME all of this should be in a list */
    this.spotify = new SpotifySource ();
    this.fakespeaker = new FakeSpeaker ();
    this.devicespeaker = new DeviceSpeaker ();
    this.src = this.spotify.get_src ();
    this.sink1 = this.devicespeaker.get_speaker ();
    this.sink2 = this.fakespeaker.get_speaker ();
    this.tee = ElementFactory.make ("tee" , "tee");

    this.pipeline = (Pipeline) new Pipeline ("dogvibes");
    /* uncomment if you want multiple speakers */
    //this.pipeline.add_many (src, tee, sink1, sink2);
    this.pipeline.add_many (src, tee, sink1);
    this.src.link (tee);
    this.tee.link (this.sink1);
    /* uncomment if you want multiple speakers */
    //this.tee.link (this.sink2);

    /* State IS already NULL */
    this.pipeline.set_state (State.NULL);
  }

  /* Speaker API */
  public void connect_speaker (int speaker) {
    stdout.printf("NOT IMPLEMENTED %d\n", speaker);
  }

  public void disconnect_speaker (int speaker) {
    stdout.printf("NOT IMPLEMENTED %d\n", speaker);
  }

  /* Play Queue API */
  public void pause () {
    this.pipeline.set_state (State.PAUSED);
  }

  public void play () {
    this.pipeline.set_state (State.PLAYING);
  }

  public void queue(string key) {
    this.pipeline.set_state (State.NULL);
    this.spotify.set_key (key);
    this.pipeline.set_state (State.PLAYING);
  }

  public void resume () {
    this.pipeline.set_state (State.PLAYING);
  }

  public void stop () {
    this.pipeline.set_state (State.NULL);
  }
}

public void main (string[] args) {
  var loop = new MainLoop (null, false);
  Gst.init (ref args);

  try {
    /* register DBus session */
    var conn = DBus.Bus.get (DBus.BusType. SYSTEM);
    dynamic DBus.Object bus = conn.get_object ("org.freedesktop.DBus",
                                               "/org/freedesktop/DBus",
                                               "org.freedesktop.DBus");
    uint request_name_result = bus.request_name ("com.Dogvibes", (uint) 0);

    if (request_name_result == DBus.RequestNameReply.PRIMARY_OWNER) {
      /* register dogvibes server */
      var dogvibes = new Dogvibes ();
      conn.register_object ("/com/dogvibes/dogvibes", dogvibes);

      /* register amplifier */
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