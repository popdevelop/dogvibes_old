public class Track : GLib.Object {
  public string album { get; set; }
  public string artist { get; set; }
  public int duration { get; set; }
  public string name { get; set; }
  public string uri { get; set; }

  public Track (string uri) {
    this.uri = uri;
  }

  construct {
  }
}
