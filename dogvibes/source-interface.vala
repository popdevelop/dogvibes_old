using Gst;

public interface Source : GLib.Object {
  public abstract Bin get_src ();
  public abstract string[] search (string searchstring);
  public abstract void set_key (string key);
}