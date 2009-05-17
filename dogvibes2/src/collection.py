import sys
import os
import tagpy
import sqlite3

from track import Track

class Collection:
    def index(self, path):
        for top, dirnames, filenames in os.walk(path):
            for filename in filenames:
                if filename.endswith('.mp3'):
                    full_path = os.path.join(top, filename);
                    f = tagpy.FileRef(full_path)
                    t = Track("file://" + full_path)
                    t.name = f.tag().title
                    t.artist = f.tag().artist
                    a = f.audioProperties()
                    t.duration = a.length * 1000
                    print t

c = Collection()
c.index('/home/brizz/music')
