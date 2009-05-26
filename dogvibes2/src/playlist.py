from database import Database
from track import Track

class DogError(Exception):
    def __init__(self, value):
        self.value = value
    def __str__(self):
        return repr(self.value)

class Playlist():
    def __init__(self, id, name, db):
        self.id = str(id)
        self.name = name
        self.db = db

    def to_dict(self):
        return dict(name = self.name, id = self.id)

    @classmethod
    def get(self, id):
        db = Database()
        db.commit_statement('''select * from playlists where id = ?''', [int(id)])
        row = db.fetchone()
        if row == None:
            raise DogError, 'Could not get playlist with id=' + id
        return Playlist(id, row[1], db)

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
        return Playlist(db.inserted_id(), name, db)

    @classmethod
    def remove(self, id):
        db = Database()
        db.add_statement('''delete from playlist_tracks where playlist_id = ?''', [id])
        db.add_statement('''delete from playlists where id = ?''', [id])
        db.commit()

    # returns: the id so client don't have to look it up right after add
    def add_track(self, track):
        track_id = track.store()
        self.db.commit_statement('''insert into playlist_tracks (playlist_id, track_id) values (?, ?)''', [int(self.id), int(track_id)])
        return self.db.inserted_id()

    def remove_track(self, id):
        # There'll be no notification if the track doesn't exists
        self.db.commit_statement('''delete from playlist_tracks where id = ?''', [int(id)])

    # TODO: returns tuples of (track_id, uri), should return Tracks
    def get_all_tracks(self):
        self.db.commit_statement('''select * from playlist_tracks where playlist_id = ?''', [self.id])
        row = self.db.fetchone()
        tracks = []
        while row != None:
            tracks.append((str(row[0]), row[2]))
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
    try:
        p = Playlist.get('1000') # should not crash
    except DogError: pass
    p = Playlist.get('2')
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
