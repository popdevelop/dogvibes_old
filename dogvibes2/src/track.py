class Track:
    def __init__(self, uri):
        self.title = "Name"
        self.artist = "Artist"
        self.album = "Album"
        self.uri = uri
        self.duration = 0

    def __str__(self):
        return self.artist + ' - ' + self.title
