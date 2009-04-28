class AmpController < ApplicationController
  require "dbus"

  def queue
    render(:text => "Queued track with id <b>" + params[:track] + "</b>")
    get_amp(params[:id]).Queue(params[:track])
  end

  def play
    render(:text => "Playing...")
    get_amp(params[:id]).Play()
  end

  def stop
    render(:text => "Stopped")
    get_amp(params[:id]).Stop()
  end

  def pause
    render(:text => "Paused")
    get_amp(params[:id]).Pause()
  end

  def resume
    render(:text => "Resumed")
    get_amp(params[:id]).Resume()
  end

  private

  def get_amp(id)
    bus = DBus::SystemBus.instance
    service = bus.service("com.Dogvibes")
    obj = service.object("/com/dogvibes/amp/0")
    obj.introspect
    obj.default_iface = "com.Dogvibes.Amp"
    return obj
  end

end
