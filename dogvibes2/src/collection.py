import sys
import os
import tagpy

class Track:
    def __init__(self):
        self.name = "Name"
        self.artist = "Artist"
        self.artist = "Album"
        self.uri = "URI"
        self.duration = 0
    def __str__(self):
        return self.artist + ' - ' + self.name
    def to_dict(self):
        return dict(name = self.name, artist = self.artist)

class Collection:
    def index(self, path):
        for top, dirnames, filenames in os.walk(path):
            for filename in filenames:
                if filename.endswith('.mp3'):
                    full_path = os.path.join(top, filename);
                    f = tagpy.FileRef(full_path)
                    t = Track()
                    t.uri = "file://" + full_path
                    t.name = f.tag().title
                    t.artist = f.tag().artist
                    a = f.audioProperties()
                    t.duration = a.length * 1000
                    print t

c = Collection()
c.index('/home/brizz/music')
