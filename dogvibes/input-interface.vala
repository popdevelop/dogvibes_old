using Gst;

public interface Input : GLib.Object {
  public abstract Element get_src (string key);
  public abstract string[] search (string searchstring);
}