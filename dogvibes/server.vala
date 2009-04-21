using Gst;
using GLib;

[DBus (name = "com.DogVibes.www")]
public class TestServer : GLib.Object {
    private Pipeline pipeline;
    /* Inputs */
	private Element spotify;
    private Element filesrc;

    /* Outputs */
    private Element alsasink;
    //private Element apexsink;

    construct {
		//All of this should not be intiated here nono

        /* make a global pipeline, a really bad idea */
        this.pipeline = (Pipeline) new Pipeline ("test");

        /* Inputs */
        /* init spotify gstreamer elements */
        this.spotify = ElementFactory.make ("spotify", "spotify");

        /* init file gstreamer elements */
        this.filesrc = ElementFactory.make ("filesrc", "filesrc");

        /* Outputs */
        /* init alsasink out element */
        this.alsasink = ElementFactory.make ("alsasink", "alsasink");

        /* init apexsink out element */
	}

    public void play (int input, int output, string key) {
        Element src;
        Element sink;
        Element apexsink;
        stdout.printf ("PLAY\n");

        if (input == 0) {
			stdout.printf("Logging on: playing %s\n", key);
			this.pipeline.set_state (State.NULL);
			this.spotify.set ("user", "username");
			this.spotify.set ("pass", "password");
			this.spotify.set ("buffer-time", (int64) 10000000);
			this.spotify.set ("uri", key);
			this.alsasink.set ("sync", false);
            src = this.spotify;
		} else if (input == 1) {
			stdout.printf("Disc command\n");
		} else {
   			stdout.printf("Error not correct input %d\n", input);
            return;
		}

        if (output == 0){
            sink = this.alsasink;
            this.pipeline.set_state (State.PLAYING);
		} else if (output == 1) {
            apexsink = ElementFactory.make ("apexsink", "apexsink");
			apexsink.set ("host", "ADDYOURAIRPORTEXRESSIPHERE");
            sink = apexsink;
		} else {
   			stdout.printf("Error not correct output %d\n", output);
            return;
		}

        this.pipeline.add_many (src, sink);
        src.link (sink);
		this.pipeline.set_state (State.PLAYING);
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
            string[] envps = {"LD_LIBRARY_PATH=/home/spotifoil/sandbox/lib/"};
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
