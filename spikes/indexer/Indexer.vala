/*
 * indexer.c will be ported into Vala here.
 *
 * Compile: valac --pkg posix Indexer.vala -o indexer
 */

using GLib;
using Posix;

public class Inotify : GLib.Object {
  public static int main (string[] args) {
    stdout.printf ("Hello!\n");
    return 0;
  }
}
