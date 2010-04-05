var t1 = new Tester('one', 'ws://localhost:9999/');

initTest();

function run() {
    // do stuff with non-existing playlist
    t1.dogvibes("getAllTracksInPlaylist?playlist_id=999999");
    t1.dogvibes("removePlaylist?id=999999");
    t1.dogvibes("addTrackToPlaylist?playlist_id=999999&uri=spotify:track:5WO8Vzz5hFWBGzJaNI5U5n");
    t1.dogvibes("getAllTracksInPlaylist?playlist_id=999999");
    t1.dogvibes("removeTrackFromPlaylist?playlist_id=999999&track_id=1");

    t1.dogvibes("addTrackToPlaylist?playlist_id=2&uri=error_uri");

    // do stuff with non-existing track
    t1.dogvibes("removeTrackFromPlaylist?playlist_id=14&track_id=999999");

    // create without name
    t1.dogvibes("createPlaylist");
}
