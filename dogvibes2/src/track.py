class Track:
    def __init__(self, uri):
        self.name = "Name"
        self.artist = "Artist"
        self.album = "Album"
        self.uri = uri
        self.duration = 0

    def __str__(self):
        return self.artist + ' - ' + self.name

    def to_dict(self):
        return dict(title = self.name,
                    artist = self.artist,
                    album = self.album,
                    uri = self.uri,
                    duration = self.duration)
