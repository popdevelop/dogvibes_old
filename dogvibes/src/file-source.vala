using Gst;

public class FileSource : GLib.Object, Source {
  construct {
    stdout.printf("Creating file source\n");

    Collection c = new Collection();
    //c.index("../testmedia");
  }

  public GLib.List<Track> search (string query) {
    Collection collection = new Collection ();
    return collection.search (query);
  }
}