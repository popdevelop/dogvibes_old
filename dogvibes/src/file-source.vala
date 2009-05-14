using Gst;

public class FileSource : GLib.Object, Source {
  private string dir;

  public FileSource (string dir) {
    this.dir = dir;
  }

  construct {
    stdout.printf("Creating file source\n");

    Collection c = new Collection();
    c.index("/home/brizz/music");
  }

  public weak GLib.List<Track> search (string query) {
    Collection collection = new Collection ();
    return collection.search (query);
  }
}