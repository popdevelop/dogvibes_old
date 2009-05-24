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

    def create_track_from_uri(self, uri):
        c = self.conn.cursor()
        c.execute('''select * from collection where uri = ?''', (uri,))
        row = c.fetchone()
        if row != None:
            return self.row_to_track(row)
        return None

    def row_to_track(self, row):
        #FIXME pleaz find out how this works
        track = Track(str(row[4]))
        track.title = str(row[1])
        track.artist = str(row[2])
        track.album = str(row[3])
        return track


    def index(self, path):
        for top, dirnames, filenames in os.walk(path):
            for filename in filenames:
                if filename.endswith('.mp3'):
                    full_path = os.path.join(top, filename);
                    f = tagpy.FileRef(full_path)
                    t = Track("file://" + full_path)
                    t.title = f.tag().title
                    t.artist = f.tag().artist
                    t.album = f.tag().album
                    a = f.audioProperties()
                    t.duration = a.length * 1000 # to msec
                    self.add_track(t)

    def search(self, query):
        ret = []
        c = self.conn.cursor()
        query = "%" + query + "%"
        c.execute('''select * from collection where title LIKE ? or artist LIKE ? or album LIKE ? or uri LIKE ?''', (query, query, query, query))
        for row in c:
            ret += [self.row_to_track(row).__dict__]

        return ret

