#!/usr/bin/env ruby

require "dbus"

# System Bus is for OS events, shared for all users 
bus = DBus::SystemBus.instance
service = bus.service("com.Dogvibes")

########################################
### TESTS FOR THE DOGVIBES INTERFACE ###
########################################

#obj = service.object("/com/dogvibes/dogvibes")
#obj.introspect
#obj.default_iface = "com.Dogvibes.Dogvibes"
#
## TEST: Searching for tracks
#query = "dylan"
#puts "TEST: Searching for #{query}"
#key = nil
#obj.Search(query)[0].each { |song|
#  unless(song.empty?)
#    puts song
#    if key.nil?
#      key = song.split(",")[2]
#    end
#  end
#}

###################################
### TESTS FOR THE AMP INTERFACE ###
###################################

obj = service.object("/com/dogvibes/amp/0")
obj.introspect
obj.default_iface = "com.Dogvibes.Amp"

# TEST: Playing the first track
puts "TEST: Playing the first track without speakers"
obj.Queue("spotify:track:3A3WCIkkm5MqGRnc4LT6fz")
sleep 5

# TEST: Connecting the active speaker
puts "TEST: Connecting the active speaker"
obj.ConnectSpeaker(0)
sleep 3

# TEST: Pausing the track
puts "TEST: Pausing the track"
obj.Pause()
sleep 3

# TEST: Resuming the track
puts "TEST: Resuming the track"
obj.Resume()
sleep 3

# TEST: Disconnecting the active speaker
puts "TEST: Disconnecting the active speaker"
obj.DisconnectSpeaker(0)
sleep 3

# TEST: Stopping the track
puts "TEST: Stopping the track"
obj.Stop()
