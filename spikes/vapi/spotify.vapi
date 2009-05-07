[CCode (lower_case_cprefix = "", cheader_filename = "spotify/api.h")]
namespace Spotify {

  [CCode (cprefix = "SPOTIFY_")]
  public const int SPOTIFY_API_VERSION;

  [CCode (cname = "sp_error", cprefix = "SP_ERROR_")]
  public enum Error {
    OK, BAD_API_VERSION, API_INITIALIZATION_FAILED, TRACK_NOT_PLAYABLE,
    RESOURCE_NOT_LOADED, BAD_APPLICATION_KEY, BAD_USERNAME_OR_PASSWORD,
    USER_BANNED, UNABLE_TO_CONTACT_SERVER, CLIENT_TOO_OLD, OTHER_PERMAMENT,
    BAD_USER_AGENT, MISSING_CALLBACK, INVALID_INDATA, INDEX_OUT_OF_RANGE,
    USER_NEEDS_PREMIUM, OTHER_TRANSIENT, IS_LOADING
  }

  [CCode (cname = "sp_error_message")]
  public string message (Error error);

  public static delegate void LoggedIn (Session session, Error error);
  public static delegate void LoggedOut (Session session);
	public static delegate void MetadataUpdated (Session session);
	public static delegate void ConnectionError (Session session);
	public static delegate void MessageToUser (Session session, string message);
	public static delegate void NotifyMainThread (Session session);
	//public static delegate int MusicDelivery (Session session, const sp_audioformat *format, const void *frames, int num_frames);
	public static delegate void PlayTokenLost (Session session);
	public static delegate void LogMessage (Session session, string data);

  [CCode (cname = "sp_session_callbacks", destroy_function = "")]
  public struct SessionCallbacks {
    public LoggedIn logged_in;
    public LoggedOut logged_out;
    public MetadataUpdated metadata_updated;
    /*
      public connection_error)(sp_session *session, sp_error error);
      public message_to_user)(sp_session *session, const char *message);
      public notify_main_thread)(sp_session *session);
      public usic_delivery)(sp_session *session, const sp_audioformat *format, const void *frames, int num_frames);
      public play_token_lost)(sp_session *session);
      public log_message)(sp_session *session, const char *data);
    */
  }

  [CCode (cname = "sp_session_config", destroy_function = "")]
  public struct SessionConfig {
    public int api_version;
    public string cache_location;
    public string settings_location;
    [CCode (array_length = false)]
    public uint8[] application_key;
    public int application_key_size;
    public string user_agent;
    public SessionCallbacks *callbacks;
    [CCode (cname = "sp_session_init")]
    public Error init_session (Session *session);
  }

  [CCode (cname = "sp_session", cprefix = "sp_session_", unref_function = "")]
  public class Session {
    public Error login (string username, string password);
    public Error logout ();
    public void process_events (int *next_timeout);
  }
}
