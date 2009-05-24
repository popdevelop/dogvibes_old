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

    def AddTrack(self, track):
        c = self.conn.cursor()
        c.execute('''select * from collection where uri = ?''', [track.uri])
        if c.fetchone() == None:
            c.execute('''insert into collection (title, artist, album, uri, duration) values (?, ?, ?, ?, ?)''',
                      (track.title, track.artist, track.album, track.uri, track.duration))
            self.conn.commit()

    def CreateTrackFromUri(self, uri):
        c = self.conn.cursor()
        # Fixme
        c.execute('''select * from collection where uri = ?''', [track.uri])
        if c.fetchone() == None:
            print "found it"


    def Index(self, path):
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
                    self.AddTrack(t)

    def search(self, query):
        c = self.conn.cursor()
        # Fixme
        #c.execute('''select * from collection where name LIKE %?% or artist LIKE %?% or album LIKE %?% or uri LIKE %?%''', query, query, query, query)
        return []

