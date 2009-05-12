public class Track : GLib.Object {
  public string name { get; construct set; }
  public string artist { get; construct set; }
  public string album { get; construct set; }
  public string duration { get; construct set; }
  public string uri { get; construct set; }
  public Track (string? uri = "undefined", string? name = "undefined",
                string? artist = "undefined", string? album = "undefined",
                string? duration = "0") {
    this.name = name;
    this.artist = artist;
    this.album = album;
    this.uri = uri;
    this.duration = duration;
  }
}

