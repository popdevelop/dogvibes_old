class AmpController < ApplicationController
  require "dbus"

  def queue
    get_amp(params[:id]).Queue(params[:track])
    render(:text => "Queued track with id <b>" + params[:track] + "</b>")
  end

  def play
    get_amp(params[:id]).Play()
    render(:text => "Playing...")
  end

  def stop
    get_amp(params[:id]).Stop()
    render(:text => "Stopped")
  end

  def pause
    get_amp(params[:id]).Pause()
    render(:text => "Paused")
  end

  def resume
    get_amp(params[:id]).Resume()
    render(:text => "Resumed")
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
