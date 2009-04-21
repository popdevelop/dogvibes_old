using Gst;
using GLib;

public interface InPut : GLib.Object {
    public abstract Element get_src(string key);
    public abstract string[] search(string searchstring);
}

public interface OutPut : GLib.Object {
    public abstract Element get_sink(string key);
}

[DBus (name = "com.DogVibes.www")]
public class TestServer : GLib.Object {

	private Pipeline pipeline;

    construct {
		/* initialize globals */
	}

    public void play (int input, int output, string key) {
		/* elements for final pipeline */
		Element filter = null;
        Element src = null;
        Element sink = null;
		/* inputs */
		Element spotify;
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
			stdout.printf ("SPOTIFY ");
			spotify = ElementFactory.make ("spotify", "spotify");
			stdout.printf("Logging on: playing %s\n", key);
			spotify.set ("user", "gyllen");
			spotify.set ("pass", "bobidob");
			spotify.set ("buffer-time", (int64) 10000000);
			spotify.set ("uri", key);
            src = spotify;
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

    public void stop () {
        stdout.printf("BEF\n");
        this.pipeline.set_state (State.NULL);
        stdout.printf("AF\n");
	}

	public void runsearch () {
	}

    public string[] search (string user, string pass, string searchstring) {
        string[] test = {};

        try {
            string[] argus = {"search", user, pass, searchstring};
            string[] envps = {"LD_LIBRARY_PATH=/home/johan/spotroot/lib/"};
            string uris;
            GLib.Process.spawn_sync(".", argus, envps, 0, runsearch, out uris);
            test = uris.split("\n");
            stdout.printf("%s\n", uris);
		} catch (Error e) {
            stdout.printf("ERROR SO INTERNAL: %s\n", e.message);
		}

        stdout.printf("I did a search on %s\n", searchstring);

        return test;
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
			// start server

			var server = new TestServer ();
			conn.register_object ("/com/dogvibes/www", server);
			loop.run ();
		}
	} catch (Error e) {
		stderr.printf ("Oops: %s\n", e.message);
	}
}
