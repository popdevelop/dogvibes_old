using Gst;

public interface Source : GLib.Object {
  public abstract GLib.List<Track> search (string query);
}
