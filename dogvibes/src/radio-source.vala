using Gst;

public class RadioSource : GLib.Object, Source {
  public string radiourl;
  private Element radiosrc;
  private Element filter;
  private Element audioconv;

  construct {
    stdout.printf ("Creating radio source with\n");
  }

  public Bin get_src () {
    Bin bin = new Bin("radiobin");
    this.radiosrc = ElementFactory.make ("mmssrc", "Radio source!");
    this.filter = ElementFactory.make ("decodebin", "decoder");
    this.audioconv = ElementFactory.make ("audioconvert", "converter");
    this.radiosrc.set ("location", "mms://wm-live.sr.se/SR-P3-High");
    bin.add_many (this.radiosrc, this.filter, this.audioconv);
    this.radiosrc.link (this.filter);
    this.filter.link (this.audioconv);
    stdout.printf ("Creating radio source, playing: \n");
    GhostPad gpad = new GhostPad ("src", this.audioconv.get_static_pad("src"));
    bin.add_pad (gpad);
    return bin;
  }

  public GLib.List<Track> search (string query) {
    string[] test = {};
    stdout.printf ("I did a search on %s\n", query);
    stdout.printf ("NOT IMPLEMENTED! \n");
    return new GLib.List<Track> ();
  }

  public void set_key (string key) {
    this.radiosrc.set ("location", key);
  }
}
