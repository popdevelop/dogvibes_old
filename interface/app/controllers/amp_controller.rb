class AmpController < ApplicationController
  require "dbus"

  def connectSpeaker
    get_amp(params[:id]).ConnectSpeaker(params[:nbr].to_i)
    render(:text => "Connected speaker " + params[:nbr])
  end

  def disconnectSpeaker
    get_amp(params[:id]).DisconnectSpeaker(params[:nbr].to_i)
    render(:text => "Disconnected speaker " + params[:nbr])
  end

  def getAllTracksInQueue
    if params[:callback].nil?
      render(:json => get_amp(params[:id]).GetAllTracksInQueue()[0].to_json)
    else
      render(:json => params[:callback] + '(' + get_amp(params[:id]).GetAllTracksInQueue()[0].to_json + ')')
    end
  end

  def getStatus
    ret = get_amp(params[:id]).GetStatus()[0]
    status = {
      :uri => ret[0],
      :title => ret[1],
      :artist => ret[2],
      :time => ret[3],
      :album => ret[4],
      :state => ret[5],
      :albumart => ret[6],
      :duration => ret[7],
      :playqueue => ret[8]
    }
    if params[:callback].nil?
      render(:json => status.to_json)
    else
      render(:json => params[:callback] + '(' + status.to_json + ')')
    end
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
    get_amp(params[:id]).PlayTrack(params[:nbr].to_i)
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
    get_amp(params[:id]).RemoveFromQueue(params[:nbr].to_i)
    render(:text => "Removed track " + params[:nbr]+ " from queue")
  end

  def seek
    get_amp(params[:id]).Seek(params[:mseconds].to_i)
    render(:text => "Seek to position " + params[:mseconds])
  end

  def setVolume
    get_amp(params[:id]).SetVolume(params[:level].to_f)
    render(:text => "Set volume to " + params[:level])
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
