/*****************************************************************************
**
**  Name:           bsa_mse_api.h
**
**  Description:    This is the public interface file for MAP client part of
**                  the Bluetooth simplified API
**
**  Copyright (c) 2015, Broadcom Corp., All Rights Reserved.
**  Broadcom Bluetooth Core. Proprietary and confidential.
**
*****************************************************************************/
#ifndef BSA_MSE_API_H
#define BSA_MSE_API_H

#include "uipc.h"

/* for tBSA_STATUS */
#include "bsa_status.h"
#include "bsa_dm_api.h"

/*****************************************************************************
**  Constants and Type Definitions
*****************************************************************************/
#define BSA_MSE_FILENAME_MAX                255
#define BSA_MSE_SERVICE_NAME_LEN_MAX        150
#define BSA_MSE_ROOT_PATH_LEN_MAX           255
#define BSA_MSE_MAX_VALUE_LEN               512
#define BSA_MSE_HANDLE_SIZE                 8
#define BSA_MSE_MN_MAX_MSG_EVT_OBECT_SIZE   960
#define BSA_MSE_MAX_FILTER_TEXT_SIZE        255
#define BSA_MSE_MAX_MAS_INSTANCE_NAME_LEN   21
#define BSA_MSE_MAX_MAS_INSTANCES           4

typedef UINT16  tBSA_MSE_SESS_HANDLE;
typedef UINT8   tBSA_MSE_INST_ID;
typedef UINT8   tBSA_MSE_MSG_HANDLE[BSA_MSE_HANDLE_SIZE];
typedef UINT32  tBSA_MSE_FEAT;

#ifndef BSA_MSE_INS_INFO_MAX_LEN
#define BSA_MSE_INS_INFO_MAX_LEN    200  /* Instance info cannot be longer than 200 according to spec (including NULL termination) */
#endif

typedef char tBSA_MSE_MAS_INS_INFO[BSA_MSE_INS_INFO_MAX_LEN];


/* ============================================================================ */
/* BSA MAP Client callback events */
typedef enum
{
    BSA_MSE_START_EVT,
    BSA_MSE_STOP_EVT,
    BSA_MSE_OPEN_EVT,
    BSA_MSE_CLOSE_EVT,
    BSA_MSE_MA_OPEN_EVT,
    BSA_MSE_MA_CLOSE_EVT,
    BSA_MSE_MN_OPEN_EVT,
    BSA_MSE_MN_CLOSE_EVT,
    BSA_MSE_NOTIF_EVT,
    BSA_MSE_NOTIF_REG_EVT,
    BSA_MSE_SET_MSG_STATUS_EVT,
    BSA_MSE_UPDATE_INBOX_EVT,
    BSA_MSE_SET_FOLDER_EVT,
    BSA_MSE_FOLDER_LIST_EVT,
    BSA_MSE_MSG_LIST_EVT,
    BSA_MSE_GET_MSG_EVT,
    BSA_MSE_PUSH_MSG_EVT,
    BSA_MSE_PUSH_MSG_DATA_REQ_EVT,
    BSA_MSE_MSG_PROG_EVT,
    BSA_MSE_OBEX_PUT_RSP_EVT,
    BSA_MSE_OBEX_GET_RSP_EVT,
    BSA_MSE_ENABLE_EVT,
    BSA_MSE_DISABLE_EVT,
    BSA_MSE_ABORT_EVT,
    BSA_MSE_GET_MAS_INS_INFO_EVT,
    BSA_MSE_GET_MAS_INSTANCES_EVT,
    BSA_MSE_INVALID_EVT
} tBSA_MSE_EVT;

/* Supported feature type */
#define BSA_MSE_SUP_FEA_NOTIF_REG        0x00000001
#define BSA_MSE_SUP_FEA_NOTIF            0x00000002
#define BSA_MSE_SUP_FEA_BROWSING         0x00000004
#define BSA_MSE_SUP_FEA_UPLOADING        0x00000008
#define BSA_MSE_SUP_FEA_DELETE           0x00000010
#define BSA_MSE_SUP_FEA_INST_INFO        0x00000020
#define BSA_MSE_SUP_FEA_EXT_EVENT_REP    0x00000040

/* Message type see SDP supported message type */
#define BSA_MSE_MSG_TYPE_EMAIL                (1<<0)
#define BSA_MSE_MSG_TYPE_SMS_GSM              (1<<1)
#define BSA_MSE_MSG_TYPE_SMS_CDMA             (1<<2)
#define BSA_MSE_MSG_TYPE_MMS                  (1<<3)

typedef UINT8 tBSA_MSE_MSG_TYPE;

/* Message type mask for FilterMessageType in Application parameter */
#define BSA_MSE_MSG_TYPE_MASK_SMS_GSM         (1<<0)
#define BSA_MSE_MSG_TYPE_MASK_SMS_CDMA        (1<<1)
#define BSA_MSE_MSG_TYPE_MASK_EMAIL           (1<<2)
#define BSA_MSE_MSG_TYPE_MASK_MMS             (1<<3)

typedef UINT8 tBSA_MSE_MSG_TYPE_MASK;

/* Parameter Mask for Messages-Listing */
#define BSA_MSE_ML_MASK_SUBJECT               (1<<0)
#define BSA_MSE_ML_MASK_DATETIME              (1<<1)
#define BSA_MSE_ML_MASK_SENDER_NAME           (1<<2)
#define BSA_MSE_ML_MASK_SENDER_ADDRESSING     (1<<3)
#define BSA_MSE_ML_MASK_RECIPIENT_NAME        (1<<4)
#define BSA_MSE_ML_MASK_RECIPIENT_ADDRESSING  (1<<5)
#define BSA_MSE_ML_MASK_TYPE                  (1<<6)
#define BSA_MSE_ML_MASK_SIZE                  (1<<7)
#define BSA_MSE_ML_MASK_RECEPTION_STATUS      (1<<8)
#define BSA_MSE_ML_MASK_TEXT                  (1<<9)
#define BSA_MSE_ML_MASK_ATTACHMENT_SIZE       (1<<10)
#define BSA_MSE_ML_MASK_PRIORITY              (1<<11)
#define BSA_MSE_ML_MASK_READ                  (1<<12)
#define BSA_MSE_ML_MASK_SENT                  (1<<13)
#define BSA_MSE_ML_MASK_PROTECTED             (1<<14)
#define BSA_MSE_ML_MASK_REPLYTO_ADDRESSING    (1<<15)

typedef UINT32 tBSA_MSE_ML_MASK;

/* Read status used for  message list */
enum
{
    BSA_MSE_READ_STATUS_NO_FILTERING = 0,
    BSA_MSE_READ_STATUS_UNREAD       = 1,
    BSA_MSE_READ_STATUS_READ         = 2
};
typedef UINT8 tBSA_MSE_READ_STATUS;

/* Priority status used for filtering message list */
enum
{
    BSA_MSE_PRI_STATUS_NO_FILTERING = 0,
    BSA_MSE_PRI_STATUS_HIGH         = 1,
    BSA_MSE_PRI_STATUS_NON_HIGH     = 2
};
typedef UINT8 tBSA_MSE_PRI_STATUS;

#define BSA_MSE_LTIME_LEN 15
typedef struct
{
    tBSA_MSE_ML_MASK        parameter_mask;
    UINT16                  max_list_cnt;
    UINT16                  list_start_offset;
    UINT8                   subject_length; /* valid range 1...255 */
    tBSA_MSE_MSG_TYPE_MASK  msg_mask;
    char                    period_begin[BSA_MSE_LTIME_LEN+1]; /* "yyyymmddTHHMMSS", or "" if none */
    char                    period_end[BSA_MSE_LTIME_LEN+1]; /* "yyyymmddTHHMMSS", or "" if none */
    tBSA_MSE_READ_STATUS    read_status;
    char                    recipient[BSA_MSE_MAX_FILTER_TEXT_SIZE+1]; /* "" if none */
    char                    originator[BSA_MSE_MAX_FILTER_TEXT_SIZE+1];/* "" if none */
    tBSA_MSE_PRI_STATUS     pri_status;
} tBSA_MSE_MSG_LIST_FILTER_PARAM;

/* enum for charset used in GetMessage */
enum
{
    BSA_MSE_CHARSET_NATIVE = 0,
    BSA_MSE_CHARSET_UTF_8  = 1,
    BSA_MSE_CHARSET_UNKNOWN,
    BSA_MSE_CHARSET_MAX
};
typedef UINT8 tBSA_MSE_CHARSET;

/* enum for fraction request used in GetMEssage */
enum
{
    BSA_MSE_FRAC_REQ_FIRST = 0,
    BSA_MSE_FRAC_REQ_NEXT  = 1,
    BSA_MSE_FRAC_REQ_NO,/* this is not a fraction request */
    BSA_MSE_FRAC_REQ_MAX
};
typedef UINT8 tBSA_MSE_FRAC_REQ;

/* enum for fraction delivery used in GetMEssage */
enum
{
    BSA_MSE_FRAC_DELIVER_MORE  = 0,
    BSA_MSE_FRAC_DELIVER_LAST  = 1,
    BSA_MSE_FRAC_DELIVER_NO,    /* this is not a fraction deliver*/
    BSA_MSE_FRAC_DELIVER_MAX
};
typedef UINT8 tBSA_MSE_FRAC_DELIVER;

typedef struct
{
    BOOLEAN                 attachment;
    tBSA_MSE_MSG_HANDLE     handle;
    tBSA_MSE_CHARSET        charset;
    tBSA_MSE_FRAC_REQ       fraction_request;
} tBSA_MSE_GET_MSG_PARAM;

/* message status inficator */
#define BSA_MSE_STS_INDTR_READ       0
#define BSA_MSE_STS_INDTR_DELETE     1
typedef UINT8 tBSA_MSE_STS_INDCTR;

/* message status value */
#define BSA_MSE_STS_VALUE_NO         0
#define BSA_MSE_STS_VALUE_YES        1
typedef UINT8 tBSA_MSE_STS_VALUE;

#define BSA_MSE_RETRY_OFF        0
#define BSA_MSE_RETRY_ON         1
#define BSA_MSE_RETRY_UNKNOWN    0xff
typedef UINT8   tBSA_MSE_RETRY_TYPE;

#define BSA_MSE_TRANSP_OFF        0
#define BSA_MSE_TRANSP_ON         1
#define BSA_MSE_TRANSP_UNKNOWN    0xff
typedef UINT8   tBSA_MSE_TRANSP_TYPE;

typedef struct
{
    char                    folder[BSA_MSE_ROOT_PATH_LEN_MAX];  /* current or child folder
                                                            for current folder set */
    tBSA_MSE_TRANSP_TYPE    transparent;
    tBSA_MSE_RETRY_TYPE     retry;
    tBSA_MSE_CHARSET        charset;

} tBSA_MSE_PUSH_MSG_PARAM;

/* BSA MSE Access Response */
#define BSA_MSE_ACCESS_ALLOW    0  /* Allow access to operation */
#define BSA_MSE_ACCESS_FORBID   1  /* Deny access to operation */
typedef UINT8 tBSA_MSE_ACCESS_TYPE;

/* BSA MSE Access operation type */
typedef enum
{
    BSA_MSE_OPER_NONE,
    BSA_MSE_OPER_GET_MSG,
    BSA_MSE_OPER_PUSH_MSG
} tBSA_MSE_OPER;

/* Types for Get operation */
enum
{
    BSA_MSE_GET_MSG = 0,            /* Get Message */
    BSA_MSE_GET_FOLDER_LIST,        /* Get Folder List*/
    BSA_MSE_GET_MSG_LIST,           /* Get Message List */
    BSA_MSE_GET_MAS_INST_INFO,      /* Get MAS Instance Info */
    BSA_MSE_GET_MAS_INSTANCES       /* Get MAS Instances */
    /* ADD NEW GET TYPES HERE */
};
typedef UINT8 tBSA_MSE_GET_TYPE;

/* Types for Set operation */
enum
{
    BSA_MSE_SET_MSG_STATUS = 0,     /* Set Message Status*/
    BSA_MSE_SET_FOLDER,             /* Set Folder */
    /* ADD NEW GET TYPES HERE */
};
typedef UINT8 tBSA_MSE_SET_TYPE;

typedef UINT8 tBSA_MSE_BD_NAME[100];


/* BSA_MSE_DISABLE_EVT callback event data */
typedef struct
{
    tBSA_STATUS     status;
} tBSA_MSE_ENABLE_MSG;


/* BSA_MSE_DISABLE_EVT callback event data */
typedef struct
{
    tBSA_STATUS     status;
} tBSA_MSE_DISABLE_MSG;

/* BSA_MSE_ABORT_EVT callback event data */
typedef struct
{
    tBSA_STATUS     status;
} tBSA_MSE_ABORT_MSG;

/* Structure associated with BSA_MSE_OPEN_EVT or BSA_MSE_CLOSE_EVT*/
typedef struct
{
    tBSA_STATUS             status;
    tBSA_MSE_SESS_HANDLE    session_handle;
    tBSA_MSE_INST_ID        instance_id;
    BD_ADDR                 bd_addr;            /* Address of device */
    BOOLEAN                 initiator;          /* connection initiator, local TRUE, peer FALSE */
    tUIPC_CH_ID             uipc_mse_rx_channel;       /* in/out: uipc MSE Receive channel */
    tUIPC_CH_ID             uipc_mse_tx_channel;       /* in/out: uipc MSE Transmit channel */
} tBSA_MSE_OPEN_CLOSE_MSG;

/* Structure associated with BSA_MSE_START_EVT or BSA_MSE_STOP_EVT*/
typedef struct
{
    tBSA_STATUS             status;
    tBSA_MSE_SESS_HANDLE    session_handle;
} tBSA_MSE_MN_START_STOP_MSG;

/* Structure associated with BSA_MSE_MN_OPEN_EVT or BSA_MSE_MN_CLOSE_EVT*/
typedef struct
{
    tBSA_STATUS             status;
    BD_ADDR                 bd_addr;     /* Address of device */
    tBSA_MSE_SESS_HANDLE    session_handle;
} tBSA_MSE_MN_OPEN_CLOSE_MSG;


/* Structure associated with BSA_MSE_START_EVT or BSA_MSE_STOP_EVT*/
typedef struct
{
    tBSA_STATUS             status;
    UINT8					mas_instance_id;
} tBSA_MSE_MA_START_STOP_MSG;

/* Structure associated with BSA_MSE_MA_OPEN_EVT or BSA_MSE_MA_CLOSE_EVT*/
typedef struct
{	
    tBSA_MSE_INST_ID		mas_instance_id;	
    tBSA_MSE_SESS_HANDLE	mas_session_id; 	 												  
    tBSA_MSE_BD_NAME		dev_name;			
    BD_ADDR					bd_addr;
    tBSA_STATUS             status;
} tBSA_MSE_MA_OPEN_CLOSE_MSG;

/* Structure associated with BSA_MSE_NOTIF_EVT  */
typedef struct
{
    UINT8                   data[BSA_MSE_MN_MAX_MSG_EVT_OBECT_SIZE]; /* event report data */
    tBSA_MSE_SESS_HANDLE    session_handle;     /* MSE connection handle */
    UINT16                  len;                /* length of the event report */
    BOOLEAN                 final;
    tBSA_STATUS             status;
    UINT8                   instance_id;            /* MAS instance ID */
} tBSA_MSE_NOTIF_MSG;

typedef union
{
    UINT16              fld_list_size;
    struct
    {
        UINT16          msg_list_size;
        UINT8           new_msg;
    } msg_list_param;
}tBSA_MSE_LIST_APP_PARAM_MSG;

/* Structure associated with BSA_MSE_GET_FOLDER_LIST_EVT  */
typedef struct
{
    UINT8                       data[BSA_MSE_MAX_VALUE_LEN];
    tBSA_MSE_LIST_APP_PARAM_MSG param;
    tBSA_MSE_SESS_HANDLE        session_handle;
    UINT16                      len;        /* 0 if object not included */
    BOOLEAN                     is_final;   /* TRUE - no more pkt to come */
    tBSA_STATUS                 status;

    UINT16                      num_entry; /* number of entries listed */
    BOOLEAN                     is_xml;
} tBSA_MSE_LIST_DATA_MSG;

/* Structure associated with BSA_MSE_GET_MSG_EVT  */
typedef struct
{
    tBSA_STATUS                 status;
    tBSA_MSE_SESS_HANDLE        session_handle;
    tBSA_MSE_FRAC_DELIVER       frac_deliver;
    char                        msg_name[BSA_MSE_FILENAME_MAX];
} tBSA_MSE_GETMSG_MSG;

/* Structure associated with BSA_MSE_GET_MAS_INS_INFO */
typedef struct
{
    tBSA_STATUS                 status;
    tBSA_MSE_SESS_HANDLE        session_handle;
    tBSA_MSE_INST_ID            instance_id;
    tBSA_MSE_MAS_INS_INFO       mas_ins_info;
} tBSA_MSE_GET_MAS_INS_INFO_MSG;

/* Structure associated with BSA_MSE_MSG_PROG_EVT  */
typedef struct
{
    UINT32                  read_size;
    UINT32                  obj_size;
    tBSA_MSE_SESS_HANDLE    session_handle;
    tBSA_MSE_OPER           operation;
} tBSA_MSE_MSG_PROG_MSG;

/* Structure associated with BSA_MSE_PUSH_MSG_DATA_REQ_EVT */
typedef struct{
    UINT32                  bytes_req;
    tBSA_MSE_SESS_HANDLE    session_handle;
}tBSA_MSE_PUSH_MSG_DATA_REQ_MSG;

/* Structure associated with BSA_MSE_PUSH_MSG_EVT */
typedef struct
{
    tBSA_MSE_SESS_HANDLE    session_handle;
    tBSA_STATUS             status;
    tBSA_MSE_MSG_HANDLE     msg_handle;
} tBSA_MSE_PUSHMSG_MSG;

/* Structure associated with BSA_MSE_UPDATE_INBOX_EVT  */
typedef struct
{
    tBSA_MSE_SESS_HANDLE    session_handle;
    tBSA_STATUS             status;
} tBSA_MSE_UPDATEINBOX_MSG;

/* Structure associated with BSA_MSE_SET_MSG_STATUS_EVT  */
typedef struct
{
    tBSA_MSE_SESS_HANDLE    session_handle;
    tBSA_STATUS             status;
} tBSA_MSE_SET_MSG_STATUS_MSG;

/* Structure associated with BSA_MSE_SET_FOLDER_EVT  */
typedef struct
{
    tBSA_MSE_SESS_HANDLE    session_handle;
    tBSA_STATUS             status;
} tBSA_MSE_SET_FOLDER_MSG;

/* Structure associated with BSA_MSE_NOTIF_REG_EVT */
typedef struct
{
    tBSA_MSE_SESS_HANDLE    session_handle;
    tBSA_STATUS             status;
} tBSA_MSE_NOTIFREG_MSG;

/* Structure associated with BSA_MSE_OBEX_PUT_RSP_EVT
and BSA_MSE_OBEX_GET_RSP_EVT                        */
typedef struct
{
    tBSA_MSE_SESS_HANDLE    session_handle;
    tBSA_MSE_INST_ID        instance_id;
    UINT8                   rsp_code;
} tBSA_MSE_OBEX_RSP_MSG;

typedef struct
{
    tBSA_MSE_INST_ID            instance_id;
    UINT8                       instance_name[BSA_MSE_MAX_MAS_INSTANCE_NAME_LEN + 1];
    tBSA_MSE_MSG_TYPE_MASK      msg_type;
    UINT16                      version;
    UINT32                      peer_features;
} tBSA_MSE_MAS_INSTANCE;

/* Structure associated with BSA_MSE_GET_MAS_INSTANCES */
typedef struct
{
    int                         count;
    tBSA_STATUS                 status;
    BD_ADDR                     bd_addr;
    tBSA_MSE_MAS_INSTANCE       mas_ins[BSA_MSE_MAX_MAS_INSTANCES];
} tBSA_MSE_GET_MAS_INSTANCES_MSG;

typedef union
{
    tBSA_MSE_ENABLE_MSG             enable;
    tBSA_MSE_DISABLE_MSG            disable;
    tBSA_MSE_OPEN_CLOSE_MSG         open;
    tBSA_MSE_OPEN_CLOSE_MSG         close;
    tBSA_MSE_MA_START_STOP_MSG      ma_start;
    tBSA_MSE_MA_START_STOP_MSG      ma_stop;
    tBSA_MSE_MA_OPEN_CLOSE_MSG      ma_open;
    tBSA_MSE_MA_OPEN_CLOSE_MSG      ma_close;
    tBSA_MSE_MN_OPEN_CLOSE_MSG      mn_open;
    tBSA_MSE_MN_OPEN_CLOSE_MSG      mn_close;
    tBSA_MSE_NOTIF_MSG              notif;
    tBSA_STATUS                     status;         /* ENABLE and DISABLE event */
    tBSA_MSE_UPDATEINBOX_MSG        upd_ibx;
    tBSA_MSE_SET_MSG_STATUS_MSG     set_msg_sts;
    tBSA_MSE_SET_FOLDER_MSG         set_folder;
    tBSA_MSE_NOTIFREG_MSG           notif_reg;
    tBSA_MSE_LIST_DATA_MSG          list_data;      /*  BSA_MSE_FOLDER_LIST_EVT,
                                                        BSA_MSE_MSG_LIST_EVT */
    tBSA_MSE_GETMSG_MSG             getmsg_msg;     /*  BSA_MSE_GET_MSG_EVT  */
    tBSA_MSE_PUSHMSG_MSG            pushmsg_msg;    /*  BSA_MSE_PUSH_MSG_EVT */
    tBSA_MSE_PUSH_MSG_DATA_REQ_MSG  push_data_req_msg;   /* BSA_MSE_PUSH_MSG_DATA_REQ_EVT */
    tBSA_MSE_MSG_PROG_MSG           prog;           /*  BSA_MSE_MSG_PROG_EVT */
    tBSA_MSE_OBEX_RSP_MSG           ma_put_rsp;
    tBSA_MSE_OBEX_RSP_MSG           ma_get_rsp;
    tBSA_MSE_GET_MAS_INS_INFO_MSG   get_mas_ins_info; /* BSA_MSE_GET_MAS_INS_INFO_EVT */
    tBSA_MSE_ABORT_MSG              abort;
    tBSA_MSE_GET_MAS_INSTANCES_MSG  mas_instances;

} tBSA_MSE_MSG;

/* BSA MSE callback function */
typedef void tBSA_MSE_CBACK(tBSA_MSE_EVT event, tBSA_MSE_MSG *p_data);

/*
* Structures used to pass parameters to BSA API functions
*/
typedef struct
{
    tBSA_MSE_CBACK  *p_cback;
} tBSA_MSE_ENABLE;

typedef struct
{
    int dummy;
} tBSA_MSE_DISABLE;

typedef struct
{
    tBSA_MSE_INST_ID    instance_id;
} tBSA_MSE_CANCEL;

/*
**          sec_mask - The security setting for the message access server.
**          p_service_name - The name of the Message Notification service, in SDP.
**          Maximum length is 35 bytes.
*/
typedef struct
{
    char                    service_name[BSA_MSE_SERVICE_NAME_LEN_MAX];
    tBSA_SEC_AUTH           sec_mask;
    tBSA_MSE_FEAT           features;
} tBSA_MSE_MN_START;

/*
**          sec_mask - The security setting for the message access server.
**          p_service_name - The name of the Message Notification service, in SDP.
**          Maximum length is 35 bytes.
*/
typedef struct
{
    char                    service_name[BSA_SERVICE_NAME_LEN];
	char                    root_path[BSA_MSE_ROOT_PATH_LEN_MAX];
	char                    mas_inst_info[BSA_MSE_INS_INFO_MAX_LEN];
    tBSA_SEC_AUTH           sec_mask;
    tBSA_MSE_FEAT           features;
	tBSA_MSE_INST_ID        mas_inst_id;
	tBSA_MSE_MSG_TYPE       sup_msg_type;
} tBSA_MSE_MA_START;

typedef struct
{
    int     dummy;
	tBSA_MSE_INST_ID        mas_inst_id;
} tBSA_MSE_MA_STOP;


typedef struct
{
    int     dummy;
} tBSA_MSE_MN_STOP;

/*
**      instance_id - MAS instance ID on server device.
**      bd_addr: MAS server bd address.
**      sec_mask: security mask used for this connection.
*/
typedef struct
{
    BD_ADDR             bd_addr;
    tBSA_MSE_INST_ID    instance_id;
    tBSA_SEC_AUTH       sec_mask;
} tBSA_MSE_OPEN;

/*
**  session_handle - MAS session ID
*/
typedef struct
{
    tBSA_MSE_SESS_HANDLE    session_handle;
} tBSA_MSE_CLOSE;

/* notification status */
enum
{
    BSA_MSE_NOTIF_OFF = 0,
    BSA_MSE_NOTIF_ON,
    BSA_MSE_NOTIF_MAX

};
typedef UINT8 tBSA_MSE_NOTIF_STATUS;

/*
**          status - BSA_MSE_NOTIF_ON if notification required
**                   BSA_MSE_NOTIF_OFF if no notification
**          session_handle - MAS session ID
*/
typedef struct
{
    tBSA_MSE_SESS_HANDLE    session_handle;
    tBSA_MSE_NOTIF_STATUS   status;
} tBSA_MSE_NOTIFYREG;

typedef struct
{
    tBSA_MSE_SESS_HANDLE    session_handle;
} tBSA_MSE_UPDATEINBOX;

/* definitions for directory navigation */
#define BSA_MSE_DIR_NAV_ROOT_OR_DOWN_ONE_LVL     2
#define BSA_MSE_DIR_NAV_UP_ONE_LVL               3

typedef UINT8 tBSA_MSE_DIR_NAV;

/*
** Description      This SET operation is used to navigate the folders of the MSE for
**                  the specified MAS instance
**
** Parameter        a combination of direction_flag and p_folder specify how to nagivate the
**                  folders on the MSE
**                  case 1 direction_flag = 2 folder = empty - reset to the default directory "telecom"
**                  case 2 direction_flag = 2 folder = name of child folder - go down 1 level into
**                  this directory name
**                  case 3 direction_flag = 3 folder = name of child folder - go up 1 level into
**                  this directory name (same as cd ../name)
**                  case 4 direction_flag = 3 folder = empty - go up 1 level to the parent directory
**                  (same as cd ..)
*/
typedef struct
{
    tBSA_MSE_SESS_HANDLE    session_handle;
    tBSA_MSE_DIR_NAV        direction_flag;
    char                    folder[BSA_MSE_ROOT_PATH_LEN_MAX];
} tBSA_MSE_SETFOLDER;

/*
** Description      This GET operation structure is used to retrieve the folder list object from
**                  the current folder
**
** Parameter        session_handle - MAS session ID
**                  max_list_count - maximum number of foldr-list objects allowed
**                            The maximum allowed value for this filed is 1024
**                  start_offset - offset of the from the first entry of the folder-list
**                                 object
*/
typedef struct
{
    tBSA_MSE_SESS_HANDLE    session_handle;
    UINT16                  max_list_count;
    UINT16                  start_offset;
} tBSA_MSE_GETFOLDERLIST;

/*
** Description      This GET operation structure is used to retrieve list of messages
**                  in the specified folder
**
** Parameter        session_handle -  session handle
**                  p_folder        - folder name
**                  p_filter_param - message listing filter parameters
*/
typedef struct
{
    tBSA_MSE_SESS_HANDLE            session_handle;
    char                            folder[BSA_MSE_ROOT_PATH_LEN_MAX];
    tBSA_MSE_MSG_LIST_FILTER_PARAM  filter_param;
} tBSA_MSE_GETMSGLIST;

/*
**          This GET operation is used to get bMessage or bBody of the
**          specified message handle fromMSE
**          session_handle - session ID
**          p_param - get message parameters, it shall not be NULL.
*/
typedef struct
{
    tBSA_MSE_SESS_HANDLE    session_handle;
    tBSA_MSE_GET_MSG_PARAM  msg_param;
} tBSA_MSE_GETMSG;

/*
**          This GET operation is used to get bMessage or bBody of the
**          specified message handle from MSE
**          session_handle - session ID
**          p_param - get message parameters, it shall not be NULL.
*/
typedef struct
{
    tBSA_MSE_SESS_HANDLE    session_handle;
    tBSA_MSE_INST_ID        instance_id;
} tBSA_MSE_GET_MAS_INFO;

/*
**          This GET operation is used to get MAS instances available on specified peer device
**          bd_addr: MAS server bd address.
*/
typedef struct
{
    BD_ADDR             bd_addr;
} tBSA_MSE_GET_MAS_INSTANCES;

/*
** Description      This SET operation is used to set the message status of the
**                  specified message handle
**
** Parameter        session_handle - MAS session ID
**                  status_indicator : read/delete message
**                  status_value : on/off
*/
typedef struct
{
    tBSA_MSE_SESS_HANDLE        session_handle;
    tBSA_MSE_MSG_HANDLE         msg_handle;
    tBSA_MSE_STS_INDCTR         status_indicator;
    tBSA_MSE_STS_VALUE          status_value;
} tBSA_MSE_SETMSGSTATUS;

/*
**      session_handle - MAS session ID
**      p_param - push message parameters, it shall not be NULL.
*/
typedef struct
{
    tBSA_MSE_SESS_HANDLE        session_handle;
    tBSA_MSE_PUSH_MSG_PARAM     msg_param;
} tBSA_MSE_PUSHMSG;

/*
**      bd_addr: MAS server bd address.
**      instance_id - MAS instance ID on server device.
*/
typedef struct
{
    BD_ADDR             bd_addr;
    tBSA_MSE_INST_ID    instance_id;
} tBSA_MSE_ABORT;

/* Get operation is used for GetFolderList, GetMsgList, GetMsg */
typedef struct
{
    /* This defines what the data in this structure corresponds to GetFolderList, GetMsgList, GetMsg */

    tBSA_MSE_GET_TYPE type;

    union {
        tBSA_MSE_GETFOLDERLIST      folderlist;
        tBSA_MSE_GETMSGLIST         msglist;
        tBSA_MSE_GETMSG             msg;
        tBSA_MSE_GET_MAS_INFO       mas_info;
        tBSA_MSE_GET_MAS_INSTANCES  mas_instances;
    }param;

} tBSA_MSE_GET;

/*  Set operation is used for ChDir */
typedef struct
{
    /*  This defines what the data in this structure corresponds to. */
    tBSA_MSE_SET_TYPE type;
    union{
        tBSA_MSE_SETMSGSTATUS   msg_status;
        tBSA_MSE_SETFOLDER      folder;
        /* Add new set types here... */
    } param;

} tBSA_MSE_SET;

/*****************************************************************************
**  External Function Declarations
*****************************************************************************/
#ifdef __cplusplus
extern "C"
{
#endif

/**************************
**  Server Functions
***************************/

/*******************************************************************************
**
** Function            BSA_MseEnableInit
**
** Description         Initialize structure containing API parameters with default values
**
** Parameters          Pointer to structure containing API parameters
**
** Returns             tBSA_STATUS
**
*******************************************************************************/

tBSA_STATUS BSA_MseEnableInit(tBSA_MSE_ENABLE* p_enable);

/*******************************************************************************
**
** Function         BSA_MseEnable
**
** Description      Enable the MSE subsystem.  This function must be
**                  called before any other functions in the MSE API are called.
**                  When the enable operation is complete the callback function
**                  will be called with an BSA_MSE_ENABLE_EVT event.
**
** Parameter        p_enable: Pointer to structure containing API parameters
**
** Returns         tBSA_STATUS
**
*******************************************************************************/

tBSA_STATUS BSA_MseEnable(tBSA_MSE_ENABLE* p_enable);

/*******************************************************************************
**
** Function            BSA_MseDisableInit
**
** Description         Initialize structure containing API parameters with default values
**
** Parameters          Pointer to structure containing API parameters
**
** Returns             tBSA_STATUS
**
*******************************************************************************/

tBSA_STATUS BSA_MseDisableInit(tBSA_MSE_DISABLE* p_disable);

/*******************************************************************************
**
** Function         BSA_MseDisable
**
** Description      Disable the MSE subssytem.  If the client is currently
**                  connected to a peer device the connection will be closed.
**
** Parameter        p_disable: Pointer to structure containing API parameters
**
** Returns         tBSA_STATUS
**
*******************************************************************************/

tBSA_STATUS BSA_MseDisable(tBSA_MSE_DISABLE* p_disable);

/*******************************************************************************
**
** Function            BSA_MseMnStartInit
**
** Description         Initialize structure containing API parameters with default values
**
** Parameters          p_mn_start - Pointer to structure containing API parameters
**
** Returns             tBSA_STATUS
**
*******************************************************************************/

tBSA_STATUS BSA_MseMnStartInit(tBSA_MSE_MN_START* p_mn_start);

/*******************************************************************************
**
** Function         BSA_MseMnStart
**
** Description      Start the Message Notification service server.
**                  When the Start operation is complete the callback function
**                  will be called with an BSA_MSE_START_EVT event.
**                  Note: Mas always enable (BSA_SEC_AUTHENTICATE | BSA_SEC_ENCRYPT)
**
**  Parameters     p_mn_start - Pointer to structure containing API parameters
**
** Returns         tBSA_STATUS
**
*******************************************************************************/

tBSA_STATUS BSA_MseMnStart(tBSA_MSE_MN_START* p_mn_start);

/*******************************************************************************
**
** Function            BSA_MseMnStopInit
**
** Description         Initialize structure containing API parameters with default values
**
** Parameters          Pointer to structure containing API parameters
**
** Returns             tBSA_STATUS
**
*******************************************************************************/

tBSA_STATUS BSA_MseMnStopInit(tBSA_MSE_MN_STOP* p_mn_stop);

/*******************************************************************************
**
** Function         BSA_MseStop
**
** Description      Stop the Message Access service server.  If the server is currently
**                  connected to a peer device the connection will be closed.
**
** Parameter        p_mn_stop: Pointer to structure containing API parameters
**
** Returns         tBSA_STATUS
**
*******************************************************************************/

tBSA_STATUS BSA_MseMnStop(tBSA_MSE_MN_STOP* p_mn_stop);

/**************************
**  Client Functions
***************************/

/*******************************************************************************
**
** Function            BSA_MseOpenInit
**
** Description         Initialize structure containing API parameters with default values
**
** Parameters          Pointer to structure containing API parameters
**
** Returns             tBSA_STATUS
**
*******************************************************************************/

tBSA_STATUS BSA_MseOpenInit(tBSA_MSE_OPEN* p_open);

/*******************************************************************************
**
** Function         BSA_MseOpen
**
** Description      Open a connection to an Message Access service server
**                  based on specified instance_id
**
**                  When the connection is open the callback function
**                  will be called with a BSA_MSE_OPEN_EVT.  If the connection
**                  fails or otherwise is closed the callback function will be
**                  called with a BSA_MSE_CLOSE_EVT.
**
**                  Note: MAS always enable (BSA_SEC_AUTHENTICATE | BSA_SEC_ENCRYPT)
**
** Parameter        p_open: Pointer to structure containing API parameters
**
** Returns         tBSA_STATUS
**
*******************************************************************************/

tBSA_STATUS BSA_MseOpen(tBSA_MSE_OPEN* p_open);

/*******************************************************************************
**
** Function            BSA_MseCloseInit
**
** Description         Initialize structure containing API parameters with default values
**
** Parameters          Pointer to structure containing API parameters
**
** Returns             tBSA_STATUS
**
*******************************************************************************/

tBSA_STATUS BSA_MseCloseInit(tBSA_MSE_CLOSE* p_close);

/*******************************************************************************
**
** Function         BSA_MseClose
**
** Description      Close the specified MAS session to the server.
**
** Parameter        p_close: Pointer to structure containing API parameters
**
** Returns         tBSA_STATUS
**
*******************************************************************************/

tBSA_STATUS BSA_MseClose(tBSA_MSE_CLOSE* p_close);

/*******************************************************************************
**
** Function            BSA_MseNotifRegInit
**
** Description         Initialize structure containing API parameters with default values
**
** Parameters          Pointer to structure containing API parameters
**
** Returns             tBSA_STATUS
**
*******************************************************************************/

tBSA_STATUS BSA_MseNotifRegInit(tBSA_MSE_NOTIFYREG* p_notifyreg);

/*******************************************************************************
**
** Function         BSA_MseNotifReg
**
** Description      Set the Message Notification status to On or OFF on the MSE.
**                  When notification is registered, message notification service
**                  must be enabled by calling API BSA_MseMnStart().
**
** Parameter        p_notifyreg - A pointer to the structure containing API parameters
**
** Returns          tBSA_STATUS
**
*******************************************************************************/

tBSA_STATUS BSA_MseNotifReg(tBSA_MSE_NOTIFYREG* p_notifyreg);

/*******************************************************************************
**
** Function            BSA_MseUpdateInboxInit
**
** Description         Initialize structure containing API parameters with default values
**
** Parameters          Pointer to structure containing API parameters
**
** Returns             tBSA_STATUS
**
*******************************************************************************/

tBSA_STATUS BSA_MseUpdateInboxInit(tBSA_MSE_UPDATEINBOX* p_update_inbox);

/*******************************************************************************
**
** Function         BSA_MseUpdateInbox
**
** Description      This function is used to update the inbox for the
**                  specified MAS session.
**
** Parameter        p_notifyreg - A pointer to the structure containing API parameters
**
** Returns         tBSA_STATUS
**
*******************************************************************************/

tBSA_STATUS BSA_MseUpdateInbox(tBSA_MSE_UPDATEINBOX* p_update_inbox);

/*******************************************************************************
**
** Function            BSA_MseGetInit
**
** Description         Initialize structure containing API parameters with default values
**                     Following GET operations are supported
**                     1) Get Message 2) Get Message List 3) Get Folder List
**
** Parameters          Pointer to structure containing API parameters
**
** Returns             tBSA_STATUS
**
*******************************************************************************/

tBSA_STATUS BSA_MseGetInit(tBSA_MSE_GET* p_get);

/*******************************************************************************
**
** Function         BSA_MseGet
**
** Description      Performs a Get Operation based on the specified get type
**                  and parameters in the tBSA_MSE_GET structure.
**                      Following GET operations are supported
**                      1) Get Message 2) Get Message List 3) Get Folder List
**
** Parameter        p_get - A pointer to the structure containing API parameters
**
** Returns         tBSA_STATUS
**
*******************************************************************************/

tBSA_STATUS BSA_MseGet(tBSA_MSE_GET* p_get);

/*******************************************************************************
**
** Function            BSA_MseSet
**
** Description         Initialize structure containing API parameters with default values
**                     Performs a Set Operation based on the specified set type and parameters in the tBSA_MSE_SET structure.
**                     Following set operations are supported: 1) Set message status 2) Set folder
**
** Parameters          Pointer to structure containing API parameters
**
** Returns             tBSA_STATUS
**
*******************************************************************************/

tBSA_STATUS BSA_MseSetInit(tBSA_MSE_SET* p_set);

/*******************************************************************************
**
** Function         BSA_MseSet
**
** Description      This function is used to set the message status of the
**                  specified message handle
**
** Parameter        p_set - A pointer to the structure containing API parameters
**
** Returns         tBSA_STATUS
**
*******************************************************************************/

tBSA_STATUS BSA_MseSet(tBSA_MSE_SET* p_set);

/*******************************************************************************
**
** Function            BSA_MsePushMsgInit
**
** Description         Initialize structure containing API parameters with default values
**
** Parameters          Pointer to structure containing API parameters
**
** Returns             tBSA_STATUS
**
*******************************************************************************/

tBSA_STATUS BSA_MsePushMsgInit(tBSA_MSE_PUSHMSG* p_pushmsg);

/*******************************************************************************
**
** Function         BSA_MsePushMsg
**
** Description      This function is used to upload a message
**                  to the specified folder in MSE
**
** Parameter        p_pushmsg - A pointer to the structure containing API parameters
**
** Returns         tBSA_STATUS
**
*******************************************************************************/

tBSA_STATUS BSA_MsePushMsg(tBSA_MSE_PUSHMSG* p_pushmsg);

/*******************************************************************************
**
** Function            BSA_MseAbortInit
**
** Description         Initialize structure containing API parameters with default values
**
** Parameters          Pointer to structure containing API parameters
**
** Returns             tBSA_STATUS
**
*******************************************************************************/

tBSA_STATUS BSA_MseAbortInit(tBSA_MSE_ABORT* p_abort);

/*******************************************************************************
**
** Function         BSA_MseAbort
**
** Description      This function is used to abort the current OBEX multi-packet
**                  operation
**
** Parameter         p_abort - A pointer to the structure containing API parameters
**
** Returns         tBSA_STATUS
**
*******************************************************************************/

tBSA_STATUS BSA_MseAbort(tBSA_MSE_ABORT* p_abort);

/*******************************************************************************
**
** Function         BSA_MseCancelInit
**
** Description      Init a structure tBSA_MSE_CANCEL to be used with BSA_MseCancel
**
** Returns          tBSA_STATUS
**
*******************************************************************************/

tBSA_STATUS BSA_MseCancelInit(tBSA_MSE_CANCEL *pCancel);

/*******************************************************************************
**
** Function         BSA_MseCancel
**
** Description      Send a command to cancel connection.
**
** Returns          tBSA_STATUS
**
*******************************************************************************/
tBSA_STATUS BSA_MseCancel(tBSA_MSE_CANCEL *pCancel);

#ifdef __cplusplus
}
#endif

#endif
