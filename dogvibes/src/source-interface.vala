using Gst;

public interface Source : GLib.Object {
  public abstract weak GLib.List<Track> search (string query);
}
