using Gst;

public class RadioSource : GLib.Object, Source {
  public static GLib.List<Track> tracks;

  construct {
    stdout.printf ("Creating radio source with\n");
  }

  public weak GLib.List<Track> search (string query) {
    stdout.printf ("I did a search on %s\n", query);
    stdout.printf ("NOT IMPLEMENTED! \n");
    return tracks;
  }
}
