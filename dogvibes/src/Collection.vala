using GLib;
using Sqlite;
using TagLib;

public class Collection : GLib.Object {

  private Database db;
  public static List<Track> tracks;

  construct {
    int rc;
    string datafile = "dogvibes.db";

    if (!FileUtils.test (datafile, FileTest.IS_REGULAR)) {
      stdout.printf ("Collection: Creating empty database %s\n", datafile);
      rc = Database.open (datafile, out this.db);
      rc = this.db.exec ("create table collection (id INTEGER PRIMARY KEY," +
                         "name TEXT, artist TEXT, album TEXT, uri TEXT," +
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
    Track track = new Track (Uri.unescape_string(values[4], ""));
    track.name = Uri.unescape_string(values[1], "");
    track.artist = Uri.unescape_string(values[2], "");
    track.album = Uri.unescape_string(values[3], "");
    track.duration = values[5]; //.to_int ();
    this.tracks.append (track);

    return 0;
  }


  public void add_track (string name, string artist, string album,
                         string uri, int duration) {
    string name_e = Uri.escape_string(name, "", true);
    string artist_e = Uri.escape_string(artist, "", true);
    string album_e = Uri.escape_string(album, "", true);
    string uri_e = Uri.escape_string(uri, "", true);

    string db_query = "select * from collection where uri = '" + uri_e + "'";
    Statement stmt;
		this.db.prepare (db_query, -1, out stmt);
    if (stmt.step () == Sqlite.DONE) { /* No more results, i.e. no results */
      stmt.reset ();

      db_query =
      "insert into collection (name, artist, album, uri, duration) " +
      "values ('%s', '%s', '%s', '%s', %d)".printf (name_e, artist_e, album_e,
                                                    uri_e, duration);

      stdout.printf ("Collection: Added '%s: %s', %s (%d) [%s]\n",
                     artist, name, album, duration, uri);

      this.db.exec (db_query, null, null);
    }
  }


  public weak List<Track> search (string query) {
    tracks = new List<Track> ();

    string db_query = "select * from collection where artist LIKE '%" + query + "%'";
    this.db.exec (db_query, callback, null);

    //    this.db.prepare_v2(STMT_TRACKNUMBER_FOR_TRACK, -1,
    //                 out this.tracknumber_for_track_statement);
    //get_artist_statement.reset();
    //    while(get_artist_statement.step() == Sqlite.ROW) {
    //  val += get_artist_statement.column_text(0);
    //xs }

    /* FIXME: ugly sleep */
    Thread.usleep (10000);

    return this.tracks;
  }


  private void parse_directory (string path) {

    GLib.File file = GLib.File.new_for_path (path);

    if (!file.query_exists (null)) {
      stderr.printf ("Tried to index '%s' but it doesn't exist.\n", file.get_path ());
      return;
    }

    if (file.query_file_type (0, null) != GLib.FileType.DIRECTORY) {
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

        if (info.get_file_type () == GLib.FileType.DIRECTORY) {
          parse_directory (full_path);
        } else {
          if (full_path.substring (-4, -1) == ".mp3") {
            TagLib.File f = new TagLib.File(full_path);
            unowned Tag t = f.tag;
            unowned string name = t.title;
            unowned string artist = t.artist;
            unowned string album = t.album;
            unowned AudioProperties audioproperties = f.audioproperties;
            int duration = audioproperties.length * 1000;

            this.add_track (name,
                            artist,
                            album,
                            "file://" + full_path,
                            duration);

            // TODO: what happens to the unowned strings here?
          }
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
}
