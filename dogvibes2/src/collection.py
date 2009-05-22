import sys
import os
import tagpy
import sqlite3

from track import Track

class Collection:
    def __init__(self, path):
        if os.path.exists(path):
            self.conn = sqlite3.connect(path)
            c = self.conn.cursor()
        else:
            self.conn = sqlite3.connect(path)
            c = self.conn.cursor()
            c.execute('''create table collection (id INTEGER PRIMARY KEY, name TEXT, artist TEXT, album TEXT, uri TEXT, duration INTEGER)''')
        self.conn.commit()

    def add_track(self, track):
        c = self.conn.cursor()
        c.execute('''select * from collection where uri = ?''', [track.uri])
        if c.fetchone() == None:
            print track.uri
            c.execute('''insert into collection (name, artist, album, uri, duration) values (?, ?, ?, ?, ?)''',
                      (track.name, track.artist, track.album, track.uri, track.duration))
            self.conn.commit()

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
                    t.duration = a.length * 1000 # to msec
                    self.add_track(t)


#c = Collection('dogvibes.db')
#c.index('/home/brizz/music')
