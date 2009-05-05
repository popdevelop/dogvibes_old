using Gst;

public interface Source : GLib.Object {
  public abstract Bin get_src ();
  public abstract GLib.List<Track> search (string query);
  public abstract void set_key (string key);
}
