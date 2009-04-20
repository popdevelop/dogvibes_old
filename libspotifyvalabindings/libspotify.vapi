/* libspotify.vapi
 *
 * Copyright (C) 2009  Johan Gyllenspetz
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.

 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.

 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
 *
 * Authors:
 * 	Johan  Gyllenspetz <johan.gyllenspetz@gmail.com>
 */

namespace LibSpotify
{
	[CCode (cprefix = "SPOTIFY_SP_ERROR_", cheader_filename = "spotify/api.h")]
	public enum sp_error {
		SP_ERROR_OK,
		SP_ERROR_BAD_API_VERSION,
		SP_ERROR_API_INITIALIZATION_FAILED,
		SP_ERROR_TRACK_NOT_PLAYABLE,
		SP_ERROR_RESOURCE_NOT_LOADED,
		SP_ERROR_BAD_APPLICATION_KEY,
		SP_ERROR_BAD_USERNAME_OR_PASSWORD,
		SP_ERROR_USER_BANNED,
		SP_ERROR_UNABLE_TO_CONTACT_SERVER,
		SP_ERROR_CLIENT_TOO_OLD,
		SP_ERROR_OTHER_PERMAMENT,
		SP_ERROR_BAD_USER_AGENT,
		SP_ERROR_MISSING_CALLBACK,
		SP_ERROR_INVALID_INDATA,
		SP_ERROR_INDEX_OUT_OF_RANGE,
		SP_ERROR_USER_NEEDS_PREMIUM,
		SP_ERROR_OTHER_TRANSIENT,
		SP_ERROR_IS_LOADING,
    };

	[CCode (lower_case_cprefix = "", cheader_filename = "spotify/api.h")]
	namespace CType
	{
		const char *sp_error_message(sp_error error);
	}
}