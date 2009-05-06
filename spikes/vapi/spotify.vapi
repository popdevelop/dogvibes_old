[CCode (lower_case_cprefix = "", cheader_filename = "spotify/api.h")]
namespace Spotify {

	[CCode (cprefix = "SPOTIFY_")]
	public const int SPOTIFY_API_VERSION;

	[CCode (cprefix = "SP_ERROR_")]
  public enum Error {
    OK, BAD_API_VERSION, API_INITIALIZATION_FAILED, TRACK_NOT_PLAYABLE,
    RESOURCE_NOT_LOADED, BAD_APPLICATION_KEY, BAD_USERNAME_OR_PASSWORD,
    USER_BANNED, UNABLE_TO_CONTACT_SERVER, CLIENT_TOO_OLD, OTHER_PERMAMENT,
    BAD_USER_AGENT, MISSING_CALLBACK, INVALID_INDATA, INDEX_OUT_OF_RANGE,
    USER_NEEDS_PREMIUM, OTHER_TRANSIENT, IS_LOADING
  }

	[CCode (cname = "struct sp_session_config")]
	public struct SessionConfig {
    public int api_version;
    public string cache_location;
    public string settings_location;
    public uint8[] application_key;
    public int application_key_size;
    public string user_agent;
		[CCode (cname = "sp_session_init")]
		public Error session_init (Session session);
	}

	[CCode (cprefix = "sp_session_")]
  public class Session {
    public Error login (string username, string password);
    public Error logout ();
  }
}
