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

  public string[] search (string query) {

    Collection collection = new Collection ();

    collection.add_track ("Johnny", "Memories in Mono", "Pikes & Perches", "file:///mim.ogg", 190);
    collection.add_track ("Marathon", "Memories in Mono", "Pikes & Perches", "file:///Mara_.mp3", 190);
    collection.add_track ("Wonderwall", "Oasis", "Standing...", "file:///oasis.mp3", 190);
    GLib.List<string> tracks = collection.search (query);

    string[] test = new string[0];
    int nbr_tracks = 0;

    foreach (string t in tracks) {
      nbr_tracks++;
      test.resize(nbr_tracks);
      test[nbr_tracks - 1] = t;
    }

    return test;
  }

  public void set_key (string key) {
    stdout.printf("Not implemented\n");
  }
}