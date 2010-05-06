from database import Database
import logging

class Source():
    def __init__(self, id, user, passw, type, db):
        self.id = int(id)
        self.user = user
        self.passw = passw
        self.type = type
        self.db = db

    @classmethod
    def get(self, id):
        db = Database()
        db.commit_statement('''select * from sources where id = ?''', [int(id)])
        row = db.fetchone()
        if row == None:
            raise ValueError('Could not get source with id=' + str(id))
        return Source(id, row['user'], row['password'], row['type'], db)

    @classmethod
    def get_all(self):
        db = Database()
        db.commit_statement('''select * from sources''')
        row = db.fetchone()
        sources = []
        while row != None:
            sources.append(Source(row['id'], row['user'], row['password'], row['type'], db))
            row = db.fetchone()
        return sources

    @classmethod
    def add(self, user, passw, type):
        db = Database()
        db.commit_statement('''insert into sources (user, password, type) values (?, ?, ?)''', [user, passw, type])
        logging.debug ("Adding source '" + user + " " + passw + " " + type + "'")
        return Source(db.inserted_id(), user, passw, type, db)

    @classmethod
    def remove(self, id):
        db = Database()
        db.add_statement('''delete from sources where id = ?''', [int(id)])
        db.commit()

    @classmethod
    def length(self):
        db = Database()
        # FIXME this is insane we need to do a real sql count here
        i = 0
        db.commit_statement('''select * from sources''')

        row = db.fetchone()

        while row != None:
            i = i + 1
            row = db.fetchone()

        return i
