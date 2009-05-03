using Gst;

public class FakeSpeaker : GLib.Object, Speaker {
  private Element fakesink;
  private Element queue2;

  construct {
  }

  public Bin get_speaker () {
    Bin bin = new Bin("fakebin");
    this.fakesink = ElementFactory.make ("fakesink", "fakesink");
    this.queue2 = ElementFactory.make ("queue2", "queue2");
    stdout.printf ("Creating fake sink\n");
    this.fakesink.set ("dump", true);
    bin.add_many (this.queue2, this.fakesink);
    this.queue2.link (this.fakesink);
    GhostPad gpad = new GhostPad ("sink", this.queue2.get_static_pad("sink"));
    bin.add_pad (gpad);
    return bin;
  }
}