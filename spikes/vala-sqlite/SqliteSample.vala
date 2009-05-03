/**
 * Using SQLite in Vala Sample Code
 * Port of an example found on the SQLite site.
 * http://www.sqlite.org/quickstart.html
 *
 * sudo apt-get install libsqlite3-dev
 *
 * valac --pkg sqlite3 -o sqlitesample SqliteSample.vala
 * ./sqlitesample test.db "select * from t1 limit 2"
 */

using GLib;
using Sqlite;

public class SqliteSample : GLib.Object {

  public static int callback (int n_columns, string[] values,
                              string[] column_names)
  {
    for (int i = 0; i < n_columns; i++) {
      stdout.printf ("%s = %s\n", column_names[i], values[i]);
    }
    stdout.printf ("\n");

    return 0;
  }

  public static int main (string[] args) {
    Database db;
    int rc;

    if (args.length != 3) {
      stderr.printf ("Usage: %s DATABASE SQL-STATEMENT\n", args[0]);
      return 1;
    }

    if (!FileUtils.test (args[1], FileTest.IS_REGULAR)) {
      stderr.printf ("Database %s does not exist or is directory\n", args[1]);
      return 1;
    }

    rc = Database.open (args[1], out db);

    if (rc != Sqlite.OK) {
      stderr.printf ("Can't open database: %d, %s\n", rc, db.errmsg ());
      return 1;
    }

    rc = db.exec (args[2], callback, null);

    if (rc != Sqlite.OK) { 
      stderr.printf ("SQL error: %d, %s\n", rc, db.errmsg ());
      return 1;
    }

    return 0;
  }
}