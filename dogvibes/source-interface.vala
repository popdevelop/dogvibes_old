using Gst;

public interface Source : GLib.Object {
  public abstract Bin get_src (string key);
  public abstract string[] search (string searchstring);
}