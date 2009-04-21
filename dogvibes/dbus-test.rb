#!/usr/bin/env ruby

require "dbus"

# System Bus is for OS events, shared for all users 
bus = DBus::SystemBus.instance

service = bus.service("com.DogVibes.www")
obj = service.object("/com/dogvibes/www")

obj.introspect
obj.default_iface = "com.DogVibes.www"

# !TEST: Configure inputs, outputs, Spotify, ...
# !TEST: Get configured inputs
# !TEST: Get configured outputs

# TEST: Searching for tracks
query = "dylan"
puts "TEST: Searching for #{query}"
key = nil
obj.Search(query)[0].each { |song|
  unless(song.empty?)
    puts song
    if key.nil?
      key = song.split(",")[2]
  end
}

# TEST: Playing the first track
puts "TEST: Playing the first track"
obj.Play(key)
sleep 5

# TEST: Stopping the track
puts "TEST: Stopping the track"
obj.Stop()
