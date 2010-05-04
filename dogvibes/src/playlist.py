from database import Database
from track import Track
import logging

class Playlist():
    def __init__(self, id, name, db):
        self.id = int(id)
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
            raise ValueError('Could not get playlist with id=' + str(id))
        return Playlist(id, row['name'], db)

    @classmethod
    def get_by_name(self, name):
        db = Database()
        db.commit_statement('''select * from playlists where name = ?''', [name])
        row = db.fetchone()
        if row == None:
            raise ValueError('Could not get playlist with id=' + str(id))
        return Playlist(row['id'], row['name'], db)

    @classmethod
    def name_exists(self, name):
        db = Database()
        db.commit_statement('''select * from playlists where name = ?''', [name])
        row = db.fetchone()
        if row == None:
            return False
        else:
            return True

    @classmethod
    def get_all(self):
        db = Database()
        db.commit_statement('''select * from playlists''')
        row = db.fetchone()
        playlists = []
        while row != None:
            playlists.append(Playlist(row['id'], row['name'], db))
            row = db.fetchone()
        return playlists

    @classmethod
    def create(self, name):
        db = Database()
        db.commit_statement('''insert into playlists (name) values (?)''', [name])
        logging.debug ("Adding playlist '" + name + "'")
        return Playlist(db.inserted_id(), name, db)

    @classmethod
    def remove(self, id):
        db = Database()
        db.commit_statement('''select * from playlists where id = ?''', [int(id)])
        row = db.fetchone()
        if row == None:
            raise ValueError('Could not get playlist with id=' + id)

        db.add_statement('''delete from playlist_tracks where playlist_id = ?''', [int(id)])
        db.add_statement('''delete from playlists where id = ?''', [int(id)])
        db.commit()

    @classmethod
    def rename(self, playlist_id, name):
        db = Database()
        db.commit_statement('''select * from playlists where id = ?''', [int(playlist_id)])
        row = db.fetchone()
        if row == None:
            raise ValueError('Could not get playlist with id=' + playlist_id)

        db.add_statement('''update playlists set name = ? where id = ?''', [name, int(playlist_id)])
        db.commit()

    # returns: the id so client don't have to look it up right after add
    def add_track(self, track):
        track_id = track.store()
        self.db.commit_statement('''select max(position) from playlist_tracks where playlist_id = ?''', [self.id])
        row = self.db.fetchone()

        if row['max(position)'] == None:
            position = 1
        else:
            position = row['max(position)'] + 1

        self.db.commit_statement('''insert into playlist_tracks (playlist_id, track_id, position) values (?, ?, ?)''', [self.id, track_id, position])
        return self.db.inserted_id()

#    def remove_track(self, id):
#        self.db.commit_statement('''select * from tracks where id = ?''', [int(id)])
#        row = self.db.fetchone()
#        if row == None:
#            raise ValueError('Could not find track with id=' + id)

#        self.db.commit_statement('''delete from playlist_tracks where id = ?''', [int(id)])
#        db.add_statement('''update playlist_tracks set position = position - 1 where position > ? and playlist_id''', [position, self.id])

    # returns: an array of Track objects
    def get_all_tracks(self):
        self.db.commit_statement('''select * from playlist_tracks where playlist_id = ? order by position''', [int(self.id)])
        row = self.db.fetchone()
        tracks = []
        while row != None:
            tracks.append((str(row['id']), row['track_id'])) # FIXME: broken!
            row = self.db.fetchone()

        ret_tracks = []
        for track in tracks:
            # TODO: replace with an SQL statement that instantly creates a Track object
            self.db.commit_statement('''select * from tracks where id = ?''', [track[1]])
            row = self.db.fetchone()
            del row['id']
            t = Track(**row)
            t.id = track[0]
            ret_tracks.append(t)

        return ret_tracks

    def get_track_nbr(self, nbr):
        self.db.commit_statement('''select * from playlist_tracks where playlist_id = ? order by position limit ?,1''', [int(self.id), nbr])

        row = self.db.fetchone()
        tid = row['track_id']
        ptid = row['id']

        self.db.commit_statement('''select * from tracks where id = ?''', [row['track_id']])
        row = self.db.fetchone()
        del row['id']
        t = Track(**row)
        t.id = tid
        t.ptid = ptid
        return t

    def get_track_id(self, id):
        self.db.commit_statement('''select * from playlist_tracks where id = ?''', [id])

        row = self.db.fetchone()
        tid = row['track_id']
        position = row['position']

        self.db.commit_statement('''select * from tracks where id = ?''', [row['track_id']])
        row = self.db.fetchone()
        del row['id']
        t = Track(**row)
        t.id = tid
        t.position = position
        return t

    def move_track(self, id, position):
        self.db.commit_statement('''select position from playlist_tracks where playlist_id = ? and id = ?''', [self.id, id])
        row = self.db.fetchone()
        if row == None:
            raise ValueError('Could not find track with id=%d in playlist with id=%d' % (id, self.id))
        old_position = row['position']
        logging.debug("Move track from %s to %s" % (old_position,position))

        self.db.commit_statement('''select max(position) from playlist_tracks where playlist_id = ?''', [self.id])
        row = self.db.fetchone()

        if position > row['max(position)'] or position < 1:
            raise ValueError('Position %d is out of bounds (%d, %d)' % (position, 1, row['max(position)']))

        if position > old_position:
            self.db.commit_statement('''update playlist_tracks set position = position - 1 where playlist_id = ? and position > ? and position <= ?''', [self.id, old_position, position])
            self.db.commit_statement('''update playlist_tracks set position = ? where playlist_id = ? and id = ?''', [position, self.id, id])
        else:
            self.db.commit_statement('''update playlist_tracks set position = position + 1 where playlist_id = ? and position >= ?''', [self.id, position])
            self.db.commit_statement('''update playlist_tracks set position = ? where playlist_id = ? and id = ?''', [position, self.id, id])
            self.db.commit_statement('''update playlist_tracks set position = position - 1 where playlist_id = ? and position > ?''', [self.id, old_position])

    def remove_track_nbr(self, nbr):
        self.db.commit_statement('''select * from playlist_tracks where playlist_id = ? limit ?,1''', [int(self.id), int(nbr)-1])
        self.db.commit_statement('''select * from playlist_tracks where id = ?''', [nbr])

        row = self.db.fetchone()
        if row == None:
            raise ValueError('Could not find track with id=%s' % (int(nbr)))

        id = row['id']
        self.db.commit_statement('''delete from playlist_tracks where id = ?''', [row['id']])
        self.db.commit_statement('''update playlist_tracks set position = position - 1 where playlist_id = ? and position > ?''', [self.id, row['position']])

    def remove_track_id(self, id):
        self.db.commit_statement('''select * from playlist_tracks where id = ?''', [id])

        row = self.db.fetchone()
        if row == None:
            raise ValueError('Could not find track with id=%s' % (int(id)))

        id = row['id']
        self.db.commit_statement('''delete from playlist_tracks where id = ?''', [row['id']])
        self.db.commit_statement('''update playlist_tracks set position = position - 1 where playlist_id = ? and position > ?''', [self.id, row['position']])

    def length(self):
        # FIXME this is insane we need to do a real sql count here
        i = 0
        self.db.commit_statement('''select * from playlist_tracks where playlist_id = ?''', [int(self.id)])

        row = self.db.fetchone()

        while row != None:
            i = i + 1
            row = self.db.fetchone()

        return i

if __name__ == '__main__':

#    p = Playlist.create("testlist 1")
#    p = Playlist.create("testlist 2")
#    p = Playlist.create("testlist 3")
#    print p.name
#    t = Track("dummy-uri0")
#    p.add_track(t)
#    print p.get_all_tracks()
#    try:
#        p = Playlist.get('1000') # should not crash
#    except ValueError: pass
#    p = Playlist.get('2')
#    print p.name
#    ps = Playlist.get_all()
#    print ps[1].name
#    t = Track("dummy-uri1")
#    ps[0].add_track(t)
#    t = Track("dummy-uri2")
#    ps[0].add_track(t)
#    t = Track("dummy-uri3")
#    ps[0].add_track(t)
#    print ps[0].get_all_tracks()
#    ps[0].remove_track('2')
#    print ps[0].get_all_tracks()
#    print [playlist.to_dict() for playlist in Playlist.get_all()]

#    import pprint
#    pp = pprint.PrettyPrinter(indent=2)

    from dogvibes import Dogvibes
    global dogvibes
    dogvibes = Dogvibes()

#    p = Playlist.create("Sortable")
    playlist = Playlist.get(1)
#    playlist.move_track(2,4)

    playlist.remove_track_nbr(6)

#    t = dogvibes.create_track_from_uri('spotify:track:4lnFwlk4m7hhFHCWmMbLqW')
#    playlist.add_track(t)
#    t = dogvibes.create_track_from_uri('spotify:track:3XjhVyOmumNu3uY5DrB6cj')
#    playlist.add_track(t)
#    t = dogvibes.create_track_from_uri('spotify:track:7FKhuZtIPchBVNIhFnNL5W')
#    playlist.add_track(t)


#    playlist.add_track(Track('spotify:track:4lnFwlk4m7hhFHCWmMbLqW'))
#    playlist.add_track(Track('spotify:track:3XjhVyOmumNu3uY5DrB6cj'))
#    playlist.add_track(Track('spotify:track:7FKhuZtIPchBVNIhFnNL5W'))


#    t = playlist.get_track_nbr(0)
#    print t
