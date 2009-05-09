using Gst;

public class SpotifySource : GLib.Object, Source, SingleSource {
  public Bin bin;
  public string user;
  public string pass;

  public weak Amplifier owner { get; set; }

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

    owner = null;
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
    string[] test = {};

    try {
      string[] argus = {"search", user, pass, query};
      string[] envps = {"LD_LIBRARY_PATH=/home/gyllen/X11bin/lib/"};
      string uris;
      GLib.Process.spawn_sync (".", argus, envps, 0, runsearch, out uris);
      test = uris.split ("\n");
      //stdout.printf ("%s\n", uris);
    } catch (GLib.Error e) {
      stdout.printf ("ERROR SO INTERNAL: %s\n", e.message);
    }

    //stdout.printf ("I did a search on %s\n", query);

    GLib.List<Track> tracks = new GLib.List<Track> ();
    Track track = new Track ();
    track.name = "A Spotify Track";
    track.artist = "A Spotify Artist";
    track.album = "A Spotify Album";
    track.uri = "spotify:track:1H5tvpoApNDxvxDexoaAUo";
    tracks.append (track);
    return tracks;
  }

  public void set_track (Track track) {
    spotify.set ("spotifyuri", track.uri);
  }

  public string supported_uris () {
    return "spotify";
  }
}
