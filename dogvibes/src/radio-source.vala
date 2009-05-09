using Gst;

public class RadioSource : GLib.Object, Source {
  construct {
    stdout.printf ("Creating radio source with\n");
  }

  public GLib.List<Track> search (string query) {
    stdout.printf ("I did a search on %s\n", query);
    stdout.printf ("NOT IMPLEMENTED! \n");
    return new GLib.List<Track> ();
  }
}
