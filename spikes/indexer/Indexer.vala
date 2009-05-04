/*
 * Compile: valac --pkg gio-2.0 Indexer.vala -o indexer
 */

using GLib;

public class Indexer : GLib.Object {

  private static void parse_dir (File file) {

    if (!file.query_exists (null)) {
      stderr.printf ("File '%s' doesn't exist.\n", file.get_path ());
      return;
    }

    if (file.query_file_type (0, null) != FileType.DIRECTORY) {
      return;
    }

    try {
      FileEnumerator iter = file.enumerate_children ("standard::name",
                                                     FileQueryInfoFlags.NONE,
                                                     null);

      FileInfo info = iter.next_file (null);

      while (info != null) {
        stdout.printf ("name: %s \n", info.get_name ());

        if (info.get_file_type () == FileType.DIRECTORY) {
          stdout.printf ("Directory\n");
          //parse_dir (???);
        } else {
          stdout.printf ("File\n");
        }

        info = iter.next_file (null);
      }

    } catch (IOError e) {
      error ("%s", e.message);
    }

  }

  public static int main (string[] args) {

    File file = File.new_for_path (args[1]);
    Indexer.parse_dir (file);

    return 0;
  }
}
