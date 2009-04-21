#!/usr/bin/env ruby

require "dbus"

# System Bus is for OS events, shared for all users 
bus = DBus::SystemBus.instance

service = bus.service("com.DogVibes.www")
obj = service.object("/com/dogvibes/www")

obj.introspect
if obj.has_iface? "com.DogVibes.www"
    obj.default_iface = "com.DogVibes.www"
    obj.Search("gyllen", "bobdylan", "dylan")[0].each { |song|
      unless(song.empty?)
        puts song.split(",")[1]
      end
    }
end
