using Gst;

public class SpotifySource : GLib.Object, Source {
  public Bin bin;
  public string user;
  public string pass;
  private Element spotify;
  private bool created;

  construct {
    try {
      var gc = GConf.Client.get_default ();
      user = gc.get_string ("/apps/dogvibes/spotify/username");
      pass = gc.get_string ("/apps/dogvibes/spotify/password");
      stdout.printf ("Creating spotify source with %s %s\n", user, pass);
    } catch (GLib.Error e) {
      stderr.printf ("Oops: %s\n", e.message);
    }
    this.user = user;
    this.pass = pass;

    created = false;
  }

  /* ugly */
  private void runsearch () {
  }

  public Bin get_src () {
    /* Ugly this should be solved when we start using decodebin */
    if (!created) {
      bin = new Bin("spotifybin");
      this.spotify = ElementFactory.make ("spotify", "spotify");
      stdout.printf ("Logging on to Spotify\n");
      spotify.set ("user", user);
      spotify.set ("pass", pass);
      spotify.set ("buffer-time", (int64) 100000000);
      bin.add (this.spotify);
      GhostPad gpad = new GhostPad ("src", this.spotify.get_static_pad("src"));
      bin.add_pad (gpad);
      created = true;
    }
    return bin;
  }

  public GLib.List<Track> search (string query) {
    GLib.List<Track> tracks = new GLib.List<Track> ();

    try {
      string[] argus = {"search", user, pass, query};
      string[] envps = {"LD_LIBRARY_PATH=/home/gyllen/X11bin/lib/"};
      string uris;
      GLib.Process.spawn_sync (".", argus, envps, 0, runsearch, out uris);

      foreach (string t in uris.split ("\n")) {
        string[] s = t.split (",");
        if (s.length >= 2) {
          Track track = new Track ();
          stdout.printf ("%s\n", s[2]);
          track.name = s[1];
          track.artist = s[0];
          track.album = "<dogvibes>";
          track.key = s[2];
          tracks.append (track);
        }
      }

    } catch (GLib.Error e) {
      stdout.printf ("ERROR SO INTERNAL: %s\n", e.message);
    }

    return tracks;
  }

  public void set_key (string key) {
    spotify.set ("uri", key);
  }
}