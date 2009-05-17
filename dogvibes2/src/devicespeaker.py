import gst

class DeviceSpeaker:
    def __init__(self, name):
        self.created = False
        self.name = name
        print "Initiated device speaker"

    def get_speaker(self):
        if self.created == False:
            self.bin = gst.Bin(self.name)
            self.devicesink = gst.element_factory_make("alsasink", "alsasink")
            self.queue2 = gst.element_factory_make("queue2", "queue2")
            self.devicesink.set_property("sync", False)
            self.bin.add(self.queue2)
            self.bin.add(self.devicesink)
            self.queue2.link(self.devicesink)
            gpad = gst.GhostPad("sink", self.queue2.get_static_pad("sink"))
            self.bin.add_pad(gpad)
            print "Created device speaker"
            self.created = True
        return self.bin
