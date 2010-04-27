import sys, re
import logging
import os

# -- Loads a config file, creates and returns a dictionary
def load(filename):

     defaults = {
          'ENABLE_SPOTIFY_SOURCE': '1',
          'ENABLE_LASTFM_SOURCE': '1',
          'ENABLE_FILE_SOURCE': '1',
          'DOGVIBES_USER': 'user',
          'DOGVIBES_PASS': 'pass',
          'SPOTIFY_USER': 'user',
          'SPOTIFY_PASS': 'pass',
          'LASTFM_USER': 'user',
          'LASTFM_PASS': 'pass',
          'HTTP_PORT': '2000',
          'WS_PORT': '9999',
          'FILE_SOURCE_ROOT': '/home/user/music'
          }

     cfg = dict()

     # Try to open a file handle on the config file and read the text
     # Possible exception #1 - the file might not be found
     # Possible exception #2 - the file might have read-protection
     # If any of these occur, just continue and use default values
     try:
          configfile = open(filename, "r")
          configtext = configfile.read()

          # Compile a pattern that matches our key-value line structure
          pattern = re.compile("\\n([\w_]+)[\t ]*([\w: \\\/~.-]+)")
          # Find all matches to this pattern in the text of the config file
          tuples = re.findall(pattern, configtext)


          # Create a new dictionary and fill it: for every tuple (key, value) in
          # the'tuples' list, set cfg[key] to value
          for x in tuples:
               cfg[x[0]] = x[1]

     except Exception, e:
          pass

     for d in defaults:
          if d in os.environ: # if set as env variable, pick that one first
               cfg[d] = os.environ[d]
          if d not in cfg: # if still not present, choose a default value
               cfg[d] = defaults[d]

     logging.debug(cfg)

     # return the fully-loaded dictionary object
     return cfg
