class AmpController < ApplicationController
  require "dbus"

  def connectSpeaker
    get_amp(params[:id]).ConnectSpeaker(params[:nbr])
    render(:text => "Connected speaker <b>" + params[:nbr]+ "</b>")
  end

  def disconnectSpeaker
    get_amp(params[:id]).DisconnectSpeaker(params[:nbr])
    render(:text => "Disconnected speaker <b>" + params[:nbr]+ "</b>")
  end

  def getAllTracksInQueue
    ret = get_amp(params[:id]).GetAllTracksInQueue()
    render(:text => ret)
  end

  def getPlayedSeconds
    ret = get_amp(params[:id]).GetPlayedSeconds()
    render(:text => ret)
  end

  def getQueuePosition
    ret = get_amp(params[:id]).GetQueuePosition()
    render(:text => ret)
  end

  def nextTrack
    get_amp(params[:id]).NextTrack()
    render(:text => "Next track")
  end

  def pause
    get_amp(params[:id]).Pause()
    render(:text => "Paused")
  end

  def play
    get_amp(params[:id]).Play()
    render(:text => "Playing...")
  end

  def playTrack
    get_amp(params[:id]).Queue(params[:track])
    render(:text => "Play track <b>" + params[:track] + "</b>")
  end

  def previousTrack
    get_amp(params[:id]).PreviuosTrack()
    render(:text => "Previous track")
  end

  def queue
    get_amp(params[:id]).Queue(params[:track])
    render(:text => "Queued track with id <b>" + params[:track] + "</b>")
  end

  def removeFromQueue
    get_amp(params[:id]).RemoveFromQueue(params[:track])
    render(:text => "Removed track <b>" + params[:mseconds]+ "</b> from queue")
  end

  def seek
    get_amp(params[:id]).Seek(params[:mseconds])
    render(:text => "Seek to position <b>" + params[:mseconds]+ "</b>")
  end

  def setVolume
    get_amp(params[:id]).SetVolume(params[:volume])
    render(:text => "Set volume to <b>" + params[:volume]+ "</b>")
  end

  def stop
    get_amp(params[:id]).Stop()
    render(:text => "Stopped")
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
