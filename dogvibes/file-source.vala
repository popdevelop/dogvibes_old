using Gst;

public class FileSource : GLib.Object, Source {
  public string file;
  private Element localmp3;
  private Element madfilter;

  construct {
    this.file = "../testmedia/beep.mp3";
    stdout.printf("Creating file input\n");
  }

  public Bin get_src () {
    Bin bin = new Bin("filebin");
    this.localmp3 = ElementFactory.make ("filesrc", "reader");
    this.madfilter = ElementFactory.make ("mad" , "decoder");
    this.localmp3.set ("location", this.file);
    bin.add_many (this.localmp3, this.madfilter);
    this.localmp3.link (this.madfilter);
    GhostPad gpad = new GhostPad ("src", this.madfilter.get_static_pad("src"));
    bin.add_pad (gpad);
    return bin;
  }

  public string[] search (string searchstring) {
	string[] test = {"beep"};
	return test;
  }

  public void set_key (string key) {
    stdout.printf("Not implemented\n");
  }
}