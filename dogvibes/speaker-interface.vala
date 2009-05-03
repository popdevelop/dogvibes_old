using Gst;

public interface Speaker : GLib.Object {
  public abstract Bin get_speaker ();
}