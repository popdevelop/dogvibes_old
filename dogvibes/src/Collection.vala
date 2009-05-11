using GLib;
using Sqlite;

public class Collection : GLib.Object {

  private Database db;
  private int ugly_mutex = 0;
  public static List<Track> tracks = new List<Track> ();

  construct {
    int rc;
    string datafile = "dogvibes.db";

    if (!FileUtils.test (datafile, FileTest.IS_REGULAR)) {
      //stdout.printf ("Collection: Creating empty database %s\n", datafile);
      rc = Database.open (datafile, out this.db);
      rc = this.db.exec ("create table collection (id INTEGER PRIMARY KEY," +
                         "name TEXT, artist TEXT, album TEXT, k TEXT," +
                         "duration INTEGER)", null, null);
    } else
      rc = Database.open (datafile, out this.db);

    if (rc != Sqlite.OK) {
      stderr.printf ("Can't open database: %d, %s\n", rc, this.db.errmsg ());
      //return 1; // todo: not allowed here...
    }
  }


  private int callback (int n_columns, string[] values,
                        string[] column_names)
  {
    Track track = new Track (values[4]);
    track.name = values[1];
    track.artist = values[2];
    track.album = values[3];
    this.tracks.append (track);

    this.ugly_mutex = 0;

    return 0;
  }


  public void add_track (string name, string artist, string album,
                         string key, int duration) {
    string db_query =
    "insert into collection (name, artist, album, k, duration) " +
    "values ('%s', '%s', '%s', '%s', %d)".printf (name, artist, album,
                                                  key, duration);

    stdout.printf ("Collection: Added '%s - %s'\n", artist, name);

    this.db.exec (db_query, null, null);
  }


  public List<Track> search (string query) {
    this.ugly_mutex = 1;

    string db_query = "select * from collection where artist LIKE '%" + query + "%'";
    this.db.exec (db_query, callback, null);

    while (this.ugly_mutex == 1); // todo: beware! busy-wait...

    return this.tracks.copy ();
  }


  private void parse_directory (string path) {

    File file = File.new_for_path (path);

    if (!file.query_exists (null)) {
      stderr.printf ("File '%s' doesn't exist.\n", file.get_path ());
      return;
    }

    if (file.query_file_type (0, null) != FileType.DIRECTORY) {
      return;
    }

    try {
      string attributes = FILE_ATTRIBUTE_STANDARD_NAME + "," +
                          FILE_ATTRIBUTE_STANDARD_TYPE;

      FileEnumerator iter =
        file.enumerate_children (attributes, FileQueryInfoFlags.NONE, null);

      FileInfo info = iter.next_file (null);

      while (info != null) {
        string full_path = file.get_path () + "/" + info.get_name ();

        if (info.get_file_type () == FileType.DIRECTORY) {
          parse_directory (full_path);
        } else {
          if (full_path.substring (-4, -1) == ".mp3")
            this.add_track (info.get_name ().split (".")[0],
                            "Bob Dylan",
                            "Greatest Hits",
                            "file://" + full_path,
                            190000);
        }

        info = iter.next_file (null);
      }

    } catch (IOError e) {
      error ("%s", e.message);
    }

  }

  public void index (string path) {
    parse_directory (path);
  }

  /*
    public static int main (string[] args) {
    Collection collection = new Collection ();

    collection.add_track ("Johnny", "Memories in Mono", "Pikes & Perches", "file:///mim.ogg", 190);
    collection.add_track ("Marathon", "Memories in Mono", "Pikes & Perches", "file:///Mara_.mp3", 190);
    collection.add_track ("Wonderwall", "Oasis", "Standing...", "file:///oasis.mp3", 190);
    List<string> tracks = collection.search ("");

    foreach (string t in tracks) {
    stdout.printf ("... %s\n", t);
    }

    return 0;
    }
  */
}
