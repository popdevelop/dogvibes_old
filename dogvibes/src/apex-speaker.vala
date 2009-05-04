using Gst;

public class ApexSpeaker : GLib.Object, Speaker, RemoteSpeaker {
  private Bin bin;
  private Element apexsink;
  private Element queue2;
  private bool created;

  public string name { get; set; }
  public string host { get; set; }

  construct {
    this.name = "apexbin";
	this.host = "192.168.1.3";
    created = false;
  }

  public Bin get_speaker () {
    if (!created) {
      bin = new Bin(this.name);
      this.apexsink = ElementFactory.make ("apexsink", "apexsink");
      this.queue2 = ElementFactory.make ("queue2", "queue2");
      this.apexsink.set ("sync", false);
	  this.apexsink.set ("host", this.host);
	  this.apexsink.set ("volume", 100);
      stdout.printf ("Creating apex sink\n");
      bin.add_many (this.queue2, this.apexsink);
      this.queue2.link (this.apexsink);
      GhostPad gpad = new GhostPad ("sink", this.queue2.get_static_pad("sink"));
      bin.add_pad (gpad);
      created = true;
    }
    return bin;
  }
}
