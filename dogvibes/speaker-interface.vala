using Gst;

public interface Speaker : GLib.Object {
  public abstract string name { get; set; }
  public abstract Bin get_speaker ();
}