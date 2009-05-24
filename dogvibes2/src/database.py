import sqlite3

class Database():
    statements = []

    def __init__(self):
        self.connection = sqlite3.connect('dogvibes.db')
        self.cursor = self.connection.cursor()
        self.add_statement('''create table if not exists collection (id INTEGER PRIMARY KEY, title TEXT, artist TEXT, album TEXT, uri TEXT, duration INTEGER)''')
        self.add_statement('''create table if not exists playlists (id INTEGER PRIMARY KEY, name TEXT)''')
        self.add_statement('''create table if not exists playlist_tracks (id INTEGER PRIMARY KEY, playlist_id INTEGER, track_uri TEXT)''')
        self.commit()

    def commit_statement(self, statement, args = []):
        self.cursor.execute(statement, args)
        self.connection.commit()

    def add_statement(self, statement, args = []):
        self.statements.append((statement, args))

    def commit(self):
        map(lambda x: self.cursor.execute(x[0], x[1]), self.statements)
        self.connection.commit()
        self.statements = []

    def fetchone(self):
        return self.cursor.fetchone()

    # TODO: check that this is always the same as the 'id' field after an insert
    def lastid(self):
        return self.cursor.lastrowid

if __name__ == '__main__':
    db = Database()
    db.add_statement('''create table if not exists playlists (id INTEGER PRIMARY KEY, name TEXT)''')
    db.commit()
    db.add_statement('''insert into playlists (name) values (?)''', ['sax'])
    db.add_statement('''insert into playlists (name) values (?)''', ['agaton'])
    db.commit()
    db.commit_statement('''insert into playlists (name) values (?)''', ['musungen'])
    db.add_statement('''insert into playlists (name) values (?)''', ['should not be saved'])
