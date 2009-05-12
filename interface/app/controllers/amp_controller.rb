class AmpController < ApplicationController
  require "dbus"

  def connectSpeaker
    get_amp(params[:id]).ConnectSpeaker(params[:nbr].to_i)
    render(:text => "Connected speaker <b>" + params[:nbr]+ "</b>")
  end

  def disconnectSpeaker
    get_amp(params[:id]).DisconnectSpeaker(params[:nbr].to_i)
    render(:text => "Disconnected speaker <b>" + params[:nbr]+ "</b>")
  end

  def getAllTracksInQueue
    render(:text => get_amp(params[:id]).GetAllTracksInQueue()[0].to_json)
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
    render(:text => "Playing")
  end

  def playTrack
    get_amp(params[:id]).PlayTrack(params[:nbr])
    render(:text => "Play track " + params[:nbr])
  end

  def previousTrack
    get_amp(params[:id]).PreviousTrack()
    render(:text => "Previous track")
  end

  def queue
    get_amp(params[:id]).Queue(params[:uri])
    render(:text => "Queued track with uri=" + params[:uri])
  end

  def removeFromQueue
    get_amp(params[:id]).RemoveFromQueue(params[:track].to_i)
    render(:text => "Removed track <b>" + params[:mseconds]+ "</b> from queue")
  end

  def seek
    get_amp(params[:id]).Seek(params[:mseconds].to_i)
    render(:text => "Seek to position <b>" + params[:mseconds]+ "</b>")
  end

  def setVolume
    get_amp(params[:id]).SetVolume(params[:volume].to_i)
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
