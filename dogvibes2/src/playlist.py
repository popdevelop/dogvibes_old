from database import Database
from track import Track

class Playlist():
    def __init__(self, id, name, db):
        self.id = id
        self.name = name
        self.db = db

    def to_dict(self):
        return dict(name = self.name, id = self.id)

    @classmethod
    def get(self, id):
        db = Database()
        db.commit_statement('''select * from playlists where id = ?''', [int(id)])
        row = db.fetchone()
        return Playlist(id, row[1], db) if row != None else None

    @classmethod
    def get_all(self):
        db = Database()
        db.commit_statement('''select * from playlists''')
        row = db.fetchone()
        playlists = []
        while row != None:
            playlists.append(Playlist(row[0], row[1], db))
            row = db.fetchone()
        return playlists

    @classmethod
    def create(self, name):
        db = Database()
        db.commit_statement('''insert into playlists (name) values (?)''', [name])
        print "Adding playlist '" + name + "'"
        return Playlist(db.lastid(), name, db)

    def add_track(self, track):
        self.db.commit_statement('''insert into playlist_tracks (playlist_id, track_uri) values (?, ?)''', [int(self.id), track.uri])

    def remove_track(self, id):
        self.db.commit_statement('''delete from playlist_tracks where id = ?''', [int(id)])

    # TODO: returns tuples of (track_id, uri), should return Tracks
    def get_all_tracks(self):
        self.db.commit_statement('''select * from playlist_tracks where playlist_id = ?''', [self.id])
        row = self.db.fetchone()
        tracks = []
        while row != None:
            tracks.append((row[0], row[2]))
            row = self.db.fetchone()
        return tracks


if __name__ == '__main__':
    p = Playlist.create("testlist 1")
    p = Playlist.create("testlist 2")
    p = Playlist.create("testlist 3")
    print p.name
    t = Track("dummy-uri0")
    p.add_track(t)
    print p.get_all_tracks()
    p = Playlist.get(1000) # should not crash
    p = Playlist.get(2)
    print p.name
    ps = Playlist.get_all()
    print ps[1].name
    t = Track("dummy-uri1")
    ps[0].add_track(t)
    t = Track("dummy-uri2")
    ps[0].add_track(t)
    t = Track("dummy-uri3")
    ps[0].add_track(t)
    print ps[0].get_all_tracks()
    ps[0].remove_track('2')
    print ps[0].get_all_tracks()
    print [playlist.to_dict() for playlist in Playlist.get_all()]