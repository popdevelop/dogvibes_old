using GConf;

public void main (string[] args) {
  try {
    var gc = GConf.Client.get_default ();
    gc.set_string ("/apps/dogvibes/spotify/username", "username");
    gc.set_string ("/apps/dogvibes/spotify/password", "password");
	} catch (GLib.Error e) {
		stderr.printf ("Oops: %s\n", e.message);
	}
}
