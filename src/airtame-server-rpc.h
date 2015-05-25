/*
 * airtame-server-rpc.h
 *
 *  Created on: Feb 2, 2015
 *      Author: elisescu
 */

#ifndef AIRTAME_SERVER_RPC_H_
#define AIRTAME_SERVER_RPC_H_

/**
{
    "title": "RPC notification schema",
    "type": "object",
    "properties": {
        "method": {
            "type": "string"
            "description": "the name of the notification"
        },
        "params": {
            "type": "array"
            "description": "an array with the parameters.
                 even positions the name of the parameter.
                 the value right after the parameter name"
        },
        "id": {
            "type": "integer",
            "description": "the ID should be null"
        }
    },
    "required": ["method", "params", "id"]
}
*/

/**
{
    "title": "RPC call method schema",
    "type": "object",
    "properties": {
        "method": {
            "type": "string"
            "description": "the name of the remote method to be called"
        },
        "params": {
            "type": "array"
            "description": "an array with the parameters.
                 even positions the name of the parameter.
                 the value right after the parameter name"
        },
        "id": {
            "type": "integer",
            "description": "the ID of the call, returned in the RPC response in
                  order to map the response back to the originating call"
        }
    },
    "required": ["method", "params", "id"]
}
*/

/**
 * Register a listener for connection state and streaming state changes.
 *
 * name: registerListener
 * params: ip   - the ip of the listener listening for changes
 *         port - the port on which the listener listens for notifications
 * result: ok if the RPC call succeeded, error otherwise
 */
#define REGISTER_LISTENER_METHOD            ("registerListener")

/**
 * Initialize the streamer. Is the method to be called after registerListener has been called.
 *
 * name: initStreamer
 * params: no params
 * result: ok if the RPC call succeeded, error otherwise. The actual
 *         initialization status is got through the listener
 */
#define INIT_STREAMER_METHOD                ("initStreamer")


/**
 * Closes the streamer. Is the method to be called to cleanup the streamer state. It is
 * the semantically opposite of the initSreamer.
 *
 * name: closeStreamer
 * params: no params
 * result: ok if the RPC call succeeded, error otherwise. The actual
 *         cleaning status is got through the listener
 */
#define CLOSE_STREAMER_METHOD               ("closeStreamer")

/**
 * Connect the streamer to a client. Should be called only after the STREAMING_STATE_INIT
 * has been received.
 *
 * name: connect
 * params: ip   - the ip of the client
 *         port - the port of the client
 * result: ok if the RPC call succeeded, error otherwise. The actual
 *         connection status is got through the listener
 */
#define CONNECT_METHOD                      ("connect")


/**
 * Disconnect the streamer from a client.
 *
 * name: disconnect
 * params: ip   - the ip of the client
 *         port - the port of the client
 * result: ok if the RPC call succeeded, error otherwise. The actual
 *         connection status is got through the listener
 */
#define DISCONNECT_METHOD                   ("disconnect")


/**
 * Unregister the listener. Should be called before closing the RPC client.
 *
 * name: unregisterListener
 * params: ip   - the ip of the listener listening for changes
 *         port - the port on which the listener listens for notifications
 * result: ok if the RPC call succeeded, error otherwise
 */
#define UNREGISTER_LISTENER_METHOD          ("unregisterListener")

/**
 * Get the settings for a specified mode
 *
 * name: getModeSettings
 * params: streaming_mode - the name of the mode (e.g., work, video, present)
 * result: mode_settings JSON object if RPC succeeded, error otherwise
 */
#define GET_MODE_SETTINGS_METHOD ("getModeSettings")

/**
 * Get the internal state of the streamer.
 *
 * name: getState
 * params: none
 * result: internal_state JSON object if RPC succeeded, error otherwise.
 * (@see internal_state JSON schema)
 */
#define GET_STATE_METHOD                    ("getState")
/**
 * {
  "$schema": "http://json-schema.org/draft-04/schema#",
  "id": "internal_state",
  "type": "object",
  "properties": {
    "clients": {
      "id": "/clients",
      "type": "array",
      "items": {
        "id": "/clients/0",
        "type": "object",
        "properties": {
          "ip": {
            "id": "/clients/0/ip",
            "type": "string"
          },
          "state": {
            "id": "/clients/0/state",
            "type": "string"
          }
        },
        "required": [
          "ip",
          "state"
        ]
      },
      "required": [
        "0"
      ]
    },
    "streaming_machine": {
      "id": "/streaming_machine",
      "type": "object",
      "properties": {
        "current_state": {
          "id": "/streaming_machine/current_state",
          "type": "string"
        },
        "no_queue_messages": {
          "id": "/streaming_machine/no_queue_messages",
          "type": "integer"
        },
        "last_received_msg": {
          "id": "/streaming_machine/last_received_msg",
          "type": "integer"
        }
      }
    },
    "network_machine": {
      "id": "/network_machine",
      "type": "object",
      "properties": {
        "current_state": {
          "id": "/network_machine/current_state",
          "type": "string"
        },
        "no_queue_messages": {
          "id": "/network_machine/no_queue_messages",
          "type": "integer"
        },
        "last_received_msg": {
          "id": "/network_machine/last_received_msg",
          "type": "integer"
        }
      }
    }
  },
  "required": [
    "clients",
    "streaming_machine",
    "network_machine"
  ]
}
*/

/**
 * Set parameters to the streamer (framerate, mode, quality and buffer period).
 *
 * name: setStreamerSettings
 * params: name_of_parameters - a string specifying the name of param (see bellow).
 *         value_of_parameters - a number or a string with the value of the param.
 *         (see few examples of jSON objects for setting the params)
 * result: internal_state JSON object if RPC succeeded, error otherwise.
 * (@see internal_state JSON schema)
 */
#define SET_STREAMER_SETTINGS_METHOD                  ("setStreamerSettings")

#define SET_STREAMER_SETTINGS_QUALITY_PNAME           ("quality")
#define SET_STREAMER_SETTINGS_FRAMERATE_PNAME         ("framerate")
#define SET_STREAMER_SETTINGS_BUFF_PNAME              ("buffer")
/**
 * Audio/Video capabilities flags. Int value, bit positions used:
 * 0 - video flag (0 - video disabled, 1 - video enabled)
 * 1 - audio flag (0 - audio disabled, 1 - audio enabled)
 */
#define SET_STREAMER_SETTINGS_AV_CAPS_FLAGS_PNAME      ("av_flags")
/**
 * Video jitterbuffer flags. Int value, bit positions used:
 * 0 - fluent playback flag (0 - fluent playback disabled, 1 - fluent playback enabled)
 */
#define SET_STREAMER_SETTINGS_VJB_FLAGS_PNAME                 ("video_jb_flags")
/**
 * Audio jitterbuffer flags. Int value, bit positions used:
 * unused right now
 */
#define SET_STREAMER_SETTINGS_AJB_FLAGS_PNAME                 ("audio_jb_flags")
/**
 * Jitter buffer delay
 */
#define SET_STREAMER_SETTINGS_JB_BUFFER_DELAY_PNAME           ("jb_delay")

/* Reliable transport */
#define SET_STREAMER_SETTINGS_RELIABLE_TRANSPORT ("reliable_transport")

/* Streaming mode parameters */
#define STREAMING_MODE_PNAME ("streaming_mode")
#define STREAMING_MODE_MANUAL ("manual")
#define STREAMING_MODE_VIDEO ("video")
#define STREAMING_MODE_WORK ("work")
#define STREAMING_MODE_PRESENT ("present")

/**
set framerate to 23
{
  "method": "setStreamerSettings",
  "params": [
    "framerate",
    23
  ],
  "id": 23
}

enable both audio and video streaming
{
  "method": "setStreamerSettings",
  "params": [
    "av_flags",
    "3"
  ],
  "id": 23
}
 */

/************************************************ NOTIFICATIONS *******************************************************/
/**
 * Show a notification on the listener side.
 *
 * name: show_notification
 * params: notification (@see the Notification Object JSON Schema)
 * result: ok if the RPC call succeeded, error otherwise. The actual
 *         cleaning status is got through the listener
 */
#define SHOW_NOTIFICATION_METHOD               ("show_notification")

/**
 * Dismiss a notification on the listener side.
 *
 * name: dismiss_notification
 * params: the ID of the notification to dismiss
 * result: ok if the RPC call succeeded, error otherwise. The actual
 *         cleaning status is got through the listener
 */
#define DISMISS_NOTIFICATION_METHOD               ("dismiss_notification")

/**
    "title": "Notification Object JSON Schema",
    "type": "object",
    "properties": {
        "type": {
            "type": "enum(string)"
            "description": "The type of the notification. Can be: INFO, WARNING or ERROR."
        },
        "category": {
            "type": "enum(string)"
            "description": "The category of the notification. Can be: STREAMING, SETUP, CONNECTION or STATE."
        },
        "title": {
            "type": "string"
            "description": "The title of the notification."
        },
        "message": {
            "type": "string"
            "description": "The message to be displayed in the notification."
        },
        "priority": {
            "type": "number"
            "description": "The priority of the notification."
        },
        "action": {
            "type": "enum(string)"
            "description": "The action of the notification. Can specify for example what buttons the notification
                             can have."
        },
        "dismiss_type": {
            "type": "enum(string)"
            "description": "The way to dismiss the notification. Can be: TIMEOUT or TRIGGER or both.
                    TIMEOUT: the notification will be dismissed after a timeout
                    TRIGGER: the notification will be dismissed when dismiss_notification is called"
        },
        "id": {
            "type": "number"
            "description": "The ID of the notification. Will be used later to dismiss the notification, instead of
                            passing the full notification object, the dismiss_notificatoin will have only the ID
                            as a parameter."
        },
    },
}
 */
#endif /* AIRTAME_SERVER_RPC_H_ */
