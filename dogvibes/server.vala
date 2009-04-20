using Gst;
using GLib;

[DBus (name = "com.DogVibes.www")]
public class TestServer : GLib.Object {
    private Pipeline pipeline;
    private Element src;
    private Element sink;

    construct {
        string[] bup = {"test", "test"};
        Gst.init (ref bup);
        this.pipeline = (Pipeline) new Pipeline ("test");
        this.src = ElementFactory.make ("spotify", "spotify");
		this.src.set ("buffer-time", (int64) 10000000);
        this.sink = ElementFactory.make ("alsasink", "alsasink");
        this.pipeline.add_many (this.src, this.sink);
        this.src.link (this.sink);
	}

    public void play (string user, string pass, string uri) {
		stdout.printf("Logging on as user %s playing %s\n", "gyllen", uri);
		this.pipeline.set_state (State.NULL);
		this.src.set ("user", user);
		this.src.set ("pass", pass);
		this.src.set ("buffer-internal-bytes", 1);
		this.src.set ("buffer-time", (int64) 10000000);
		this.src.set ("uri", uri);
		this.sink.set ("sync", false);
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
