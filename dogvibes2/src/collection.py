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
            c.execute('''create table collection (id INTEGER PRIMARY KEY, title TEXT, artist TEXT, album TEXT, uri TEXT, duration INTEGER)''')
        self.conn.commit()

    def add_track(self, track):
        c = self.conn.cursor()
        c.execute('''select * from collection where uri = ?''', [track.uri])
        if c.fetchone() == None:
            c.execute('''insert into collection (title, artist, album, uri, duration) values (?, ?, ?, ?, ?)''',
                      (track.title, track.artist, track.album, track.uri, track.duration))
            self.conn.commit()

    def index(self, path):
        for top, dirnames, filenames in os.walk(path):
            for filename in filenames:
                if filename.endswith('.mp3'):
                    full_path = os.path.join(top, filename);
                    f = tagpy.FileRef(full_path)
                    t = Track("file://" + full_path)
                    t.title = f.tag().title
                    t.artist = f.tag().artist
                    a = f.audioProperties()
                    t.duration = a.length * 1000 # to msec
                    self.add_track(t)
