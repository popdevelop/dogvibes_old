class Track:
    def __init__(self, uri):
        # Currently a track is only an uri
        self.uri = uri

    def to_dict(self):
        return {"uri": self.uri}
