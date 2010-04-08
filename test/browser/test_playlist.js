var t1 = new Tester('one', 'ws://localhost:9999/');

initTest();


function run() {
    // do stuff with non-existing playlist

//    t1.sendCmd("search?query=oasis");
    t1.sendCmd({ cmd: '/amp/0/setVolume?vol=0.1' });

    t1.sendCmd({ cmd: '/dogvibes/getAllTracksInPlaylist?playlist_id=999999',
                 onReply: function(reply){
                     alert(reply.error);
                 }});
    t1.sendCmd({ cmd: '/dogvibes/removePlaylist?id=999999' });
    t1.sendCmd({ cmd: '/dogvibes/addTrackToPlaylist?playlist_id=999999&uri=spotify:track:5WO8Vzz5hFWBGzJaNI5U5n' });
    t1.sendCmd({ cmd: '/dogvibes/getAllTracksInPlaylist?playlist_id=999999' });
    t1.sendCmd({ cmd: '/dogvibes/removeTrackFromPlaylist?playlist_id=999999&track_id=1' });

    t1.sendCmd({ cmd: '/dogvibes/addTrackToPlaylist?playlist_id=2&uri=error_uri' });

    // do stuff with non-existing track
    t1.sendCmd({ cmd: '/dogvibes/removeTrackFromPlaylist?playlist_id=14&track_id=999999' });

    // create without name
    t1.sendCmd({ cmd: '/dogvibes/createPlaylist' });

//    t1.dogvibes("getAllPlaylists");
//    t1.dogvibes("renamePlaylist?playlist_id=16&name=NewName333");
//    t1.dogvibes("getAllPlaylists");
}
